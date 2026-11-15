// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0.
// SPDX-License-Identifier: Apache-2.0

/**
 * FAPI GetRandom Server
 *
 * FreeRTOS-based server that provides random number generation via UART.
 * Implements a simple protocol:
 *   Request:  [cmd][size_lo][size_hi]
 *   Response: [status][size_lo][size_hi][data...]
 */

#include <string.h>
#include <stdbool.h>

#include "sw/device/lib/runtime/log.h"
#include "sw/device/lib/testing/test_framework/check.h"
#include "sw/device/lib/testing/test_framework/ottf_main.h"

#include "sw/device/lib/dif/dif_csrng.h"
#include "sw/device/silicon_creator/lib/drivers/uart.h"

#include "hw/top_earlgrey/sw/autogen/top_earlgrey.h"

// Enable FreeRTOS concurrency
OTTF_DEFINE_TEST_CONFIG(.enable_concurrency = true);

// ================= Constants =================

#define CMD_GET_RANDOM 0x01
#define MAX_RANDOM_SIZE 256
#define STATUS_SUCCESS 0x00
#define STATUS_ERROR 0x01

// ================= Global State =================

static dif_csrng_t csrng;

// Shared communication (volatile for task safety)
static volatile uint8_t request_cmd;
static volatile uint16_t request_size;
static volatile bool request_ready = false;
static volatile bool response_ready = false;
static volatile uint8_t response_status;
static volatile uint8_t response_buffer[MAX_RANDOM_SIZE];
static volatile uint16_t response_size;

// ================= CSRNG Functions =================

static status_t csrng_init(void) {
  LOG_INFO("Initializing CSRNG for simulation...");

  // Initialize CSRNG (without entropy_src for simulation)
  TRY(dif_csrng_init(
      mmio_region_from_addr(TOP_EARLGREY_CSRNG_BASE_ADDR),
      &csrng));

  TRY(dif_csrng_configure(&csrng));
  LOG_INFO("CSRNG configured");

  // For simulation: use deterministic seed (disable entropy_src)
  // This matches the approach used in csrng_kat_test.c
  dif_csrng_seed_material_t seed = {
      .seed_material = {
          0x73bec010, 0x9262474c, 0x16a30f76, 0x531b51de,
          0x2ee494e5, 0xdfec9db3, 0xcb7a879d, 0x5600419c,
          0xca79b0b0, 0xdda33b5c, 0xa468649e, 0xdf5d73fa
      },
      .seed_material_len = 12,
  };

  LOG_INFO("Instantiating CSRNG with test seed...");
  // Use kDifCsrngEntropySrcToggleDisable for simulation
  TRY(dif_csrng_instantiate(&csrng, kDifCsrngEntropySrcToggleDisable, &seed));
  LOG_INFO("CSRNG instantiated successfully");

  return OK_STATUS();
}

static status_t csrng_get_random(uint8_t *buf, size_t len) {
  if (len == 0) {
    return OK_STATUS();
  }

  size_t num_words = (len + 3) / 4;
  TRY(dif_csrng_generate_start(&csrng, num_words));

  uint32_t word;
  size_t written = 0;

  for (size_t i = 0; i < num_words && written < len; ++i) {
    TRY(dif_csrng_generate_read(&csrng, &word, 1));
    for (size_t b = 0; b < 4 && written < len; ++b) {
      buf[written++] = (word >> (8 * b)) & 0xff;
    }
  }

  return OK_STATUS();
}

// ================= Hardware Initialization =================

static status_t hardware_init(void) {
  LOG_INFO("=== FAPI GetRandom Server ===");

  // Initialize CSRNG
  status_t status = csrng_init();
  if (!status_ok(status)) {
    LOG_ERROR("CSRNG init failed!");
    return status;
  }
  LOG_INFO("CSRNG initialization: OK");

  // Initialize UART
  LOG_INFO("Initializing UART...");
  uart_init((uint32_t)kUartBaudrate);
  uart_enable_receiver();
  LOG_INFO("UART initialization: OK");

  return OK_STATUS();
}

// ================= FreeRTOS Tasks =================

static void uart_task(void *params) {
  (void)params;

  LOG_INFO("UART task started");

  while (true) {
    // Read 3-byte request with timeout
    uint8_t req[3];
    size_t bytes_read = uart_read(req, 3, 1000);

    if (bytes_read == 3) {
      // Parse request
      request_cmd = req[0];
      request_size = (uint16_t)((uint16_t)req[1] | ((uint16_t)req[2] << 8));

      LOG_INFO("Received request: cmd=0x%02x, size=%d", request_cmd, request_size);

      // Signal crypto task
      request_ready = true;

      // Wait for response
      while (!response_ready) {
        ottf_task_yield();
      }

      // Send response header
      uint8_t resp[3] = {
          response_status,
          (uint8_t)(response_size & 0xFF),
          (uint8_t)(response_size >> 8)
      };
      uart_write(resp, 3);

      // Send response data if successful
      if (response_status == STATUS_SUCCESS && response_size > 0) {
        uart_write((const uint8_t*)response_buffer, response_size);
      }

      LOG_INFO("Sent response: status=0x%02x, size=%d", response_status, response_size);

      // Reset for next request
      response_ready = false;
    }

    ottf_task_yield();
  }
}

static void crypto_task(void *params) {
  (void)params;

  LOG_INFO("Crypto task started");

  while (true) {
    // Wait for request
    if (!request_ready) {
      ottf_task_yield();
      continue;
    }

    LOG_INFO("Processing request...");

    // Validate and process request
    if (request_cmd == CMD_GET_RANDOM && request_size > 0 && request_size <= MAX_RANDOM_SIZE) {
      // Generate random bytes
      status_t status = csrng_get_random((uint8_t*)response_buffer, request_size);

      if (status_ok(status)) {
        response_status = STATUS_SUCCESS;
        response_size = request_size;
        LOG_INFO("Generated %d random bytes", request_size);
      } else {
        response_status = STATUS_ERROR;
        response_size = 0;
        LOG_ERROR("CSRNG generation failed!");
      }
    } else {
      // Invalid request
      response_status = STATUS_ERROR;
      response_size = 0;
      LOG_ERROR("Invalid request: cmd=0x%02x, size=%d", request_cmd, request_size);
    }

    // Signal completion
    request_ready = false;
    response_ready = true;

    ottf_task_yield();
  }
}

// ================= Main Test Function =================

bool test_main(void) {
  // Initialize hardware
  status_t status = hardware_init();
  if (!status_ok(status)) {
    LOG_ERROR("Hardware initialization failed!");
    return false;
  }

  LOG_INFO("Creating FreeRTOS tasks...");

  // Create UART task
  CHECK(ottf_task_create(uart_task, "uart", kOttfFreeRtosMinStackSize, 1));
  LOG_INFO("UART task created");

  // Create crypto task
  CHECK(ottf_task_create(crypto_task, "crypto", kOttfFreeRtosMinStackSize, 1));
  LOG_INFO("Crypto task created");

  LOG_INFO("Server ready and waiting for requests...");

  // Keep running forever
  while (true) {
    ottf_task_yield();
  }

  return true;
}
