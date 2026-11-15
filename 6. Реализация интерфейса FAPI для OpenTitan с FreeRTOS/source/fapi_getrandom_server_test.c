// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0.
// SPDX-License-Identifier: Apache-2.0

/**
 * FAPI GetRandom Server Test
 *
 * Tests the server logic: request processing, CSRNG generation, protocol handling
 * Tests run without UART, directly invoking server functions.
 */

#include <string.h>
#include <stdbool.h>

#include "sw/device/lib/runtime/log.h"
#include "sw/device/lib/testing/test_framework/check.h"
#include "sw/device/lib/testing/test_framework/ottf_main.h"
#include "sw/device/lib/dif/dif_csrng.h"
#include "hw/top_earlgrey/sw/autogen/top_earlgrey.h"

OTTF_DEFINE_TEST_CONFIG();

// Protocol constants (from server)
#define CMD_GET_RANDOM 0x01
#define MAX_RANDOM_SIZE 256
#define STATUS_SUCCESS 0x00
#define STATUS_ERROR 0x01

// Global state
static dif_csrng_t csrng;

/**
 * Initialize CSRNG (copied from server)
 */
static status_t csrng_init(void) {
  LOG_INFO("Initializing CSRNG for simulation...");

  TRY(dif_csrng_init(
      mmio_region_from_addr(TOP_EARLGREY_CSRNG_BASE_ADDR),
      &csrng));

  TRY(dif_csrng_configure(&csrng));
  LOG_INFO("CSRNG configured");

  dif_csrng_seed_material_t seed = {
      .seed_material = {
          0x73bec010, 0x9262474c, 0x16a30f76, 0x531b51de,
          0x2ee494e5, 0xdfec9db3, 0xcb7a879d, 0x5600419c,
          0xca79b0b0, 0xdda33b5c, 0xa468649e, 0xdf5d73fa
      },
      .seed_material_len = 12,
  };

  LOG_INFO("Instantiating CSRNG with test seed...");
  TRY(dif_csrng_instantiate(&csrng, kDifCsrngEntropySrcToggleDisable, &seed));
  LOG_INFO("CSRNG instantiated successfully");

  return OK_STATUS();
}

/**
 * Generate random bytes (copied from server)
 */
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

/**
 * Server request processing logic (extracted from crypto_task)
 */
static void process_request(uint8_t cmd, uint16_t size,
                           uint8_t *response_status,
                           uint8_t *response_buffer,
                           uint16_t *response_size) {
  // Validate and process request (same logic as server)
  if (cmd == CMD_GET_RANDOM && size > 0 && size <= MAX_RANDOM_SIZE) {
    status_t status = csrng_get_random(response_buffer, size);

    if (status_ok(status)) {
      *response_status = STATUS_SUCCESS;
      *response_size = size;
      LOG_INFO("Generated %d random bytes", size);
    } else {
      *response_status = STATUS_ERROR;
      *response_size = 0;
      LOG_ERROR("CSRNG generation failed!");
    }
  } else {
    // Invalid request
    *response_status = STATUS_ERROR;
    *response_size = 0;
    LOG_ERROR("Invalid request: cmd=0x%02x, size=%d", cmd, size);
  }
}

/**
 * Helper: Check if buffer is all zeros
 */
static bool is_all_zeros(const uint8_t *buf, size_t len) {
  for (size_t i = 0; i < len; ++i) {
    if (buf[i] != 0) {
      return false;
    }
  }
  return true;
}

/**
 * Test 1: Valid request processing
 */
static status_t test_valid_requests(void) {
  LOG_INFO("=== Test 1: Valid request processing ===");

  const uint16_t test_sizes[] = {16, 32, 64, 128, 256};
  const size_t num_tests = sizeof(test_sizes) / sizeof(test_sizes[0]);

  for (size_t i = 0; i < num_tests; ++i) {
    uint16_t req_size = test_sizes[i];
    uint8_t resp_status;
    uint8_t resp_buffer[MAX_RANDOM_SIZE];
    uint16_t resp_size;

    memset(resp_buffer, 0, sizeof(resp_buffer));

    LOG_INFO("Test 1.%d: Request %d bytes", i + 1, req_size);

    // Process request (simulates crypto_task)
    process_request(CMD_GET_RANDOM, req_size, &resp_status, resp_buffer, &resp_size);

    // Verify response
    TRY_CHECK(resp_status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got 0x%02x", resp_status);
    TRY_CHECK(resp_size == req_size, "Expected size %d, got %d", req_size, resp_size);
    TRY_CHECK(!is_all_zeros(resp_buffer, resp_size), "Response buffer is all zeros");

    LOG_INFO("Test 1.%d: PASSED - received %d non-zero bytes", i + 1, resp_size);
  }

  LOG_INFO("Test 1: All valid requests PASSED");
  return OK_STATUS();
}

/**
 * Test 2: Invalid command
 */
static status_t test_invalid_command(void) {
  LOG_INFO("=== Test 2: Invalid command ===");

  uint8_t resp_status;
  uint8_t resp_buffer[MAX_RANDOM_SIZE];
  uint16_t resp_size;

  // Test invalid command (0x99 instead of 0x01)
  process_request(0x99, 16, &resp_status, resp_buffer, &resp_size);

  TRY_CHECK(resp_status == STATUS_ERROR, "Expected STATUS_ERROR for invalid cmd");
  TRY_CHECK(resp_size == 0, "Expected size 0 for error, got %d", resp_size);

  LOG_INFO("Test 2: PASSED - invalid command rejected");
  return OK_STATUS();
}

/**
 * Test 3: Invalid sizes
 */
static status_t test_invalid_sizes(void) {
  LOG_INFO("=== Test 3: Invalid sizes ===");

  uint8_t resp_status;
  uint8_t resp_buffer[MAX_RANDOM_SIZE];
  uint16_t resp_size;

  // Test size = 0
  LOG_INFO("Test 3.1: Size = 0");
  process_request(CMD_GET_RANDOM, 0, &resp_status, resp_buffer, &resp_size);
  TRY_CHECK(resp_status == STATUS_ERROR, "Expected STATUS_ERROR for size=0");
  TRY_CHECK(resp_size == 0, "Expected resp_size=0");
  LOG_INFO("Test 3.1: PASSED");

  // Test size > MAX_RANDOM_SIZE
  LOG_INFO("Test 3.2: Size = 300 (>256)");
  process_request(CMD_GET_RANDOM, 300, &resp_status, resp_buffer, &resp_size);
  TRY_CHECK(resp_status == STATUS_ERROR, "Expected STATUS_ERROR for size=300");
  TRY_CHECK(resp_size == 0, "Expected resp_size=0");
  LOG_INFO("Test 3.2: PASSED");

  LOG_INFO("Test 3: All invalid size tests PASSED");
  return OK_STATUS();
}

/**
 * Test 4: Protocol format verification
 */
static status_t test_protocol_format(void) {
  LOG_INFO("=== Test 4: Protocol format verification ===");

  // Simulate full request/response cycle
  uint8_t request[3];
  uint8_t response[3 + MAX_RANDOM_SIZE];

  // Build request: [cmd=0x01][size_lo=32][size_hi=0]
  request[0] = CMD_GET_RANDOM;
  request[1] = 32;  // size_lo
  request[2] = 0;   // size_hi

  LOG_INFO("Request: cmd=0x%02x, size=%d", request[0], (uint16_t)(request[1] | (request[2] << 8)));

  // Parse request
  uint8_t cmd = request[0];
  uint16_t size = (uint16_t)((uint16_t)request[1] | ((uint16_t)request[2] << 8));

  // Process
  uint8_t resp_status;
  uint8_t resp_data[MAX_RANDOM_SIZE];
  uint16_t resp_size;

  process_request(cmd, size, &resp_status, resp_data, &resp_size);

  // Build response: [status][size_lo][size_hi][data...]
  response[0] = resp_status;
  response[1] = (uint8_t)(resp_size & 0xFF);
  response[2] = (uint8_t)(resp_size >> 8);
  memcpy(&response[3], resp_data, resp_size);

  // Verify
  TRY_CHECK(response[0] == STATUS_SUCCESS, "Response status not SUCCESS");
  TRY_CHECK(response[1] == 32, "Response size_lo incorrect");
  TRY_CHECK(response[2] == 0, "Response size_hi incorrect");
  TRY_CHECK(!is_all_zeros(&response[3], resp_size), "Response data is all zeros");

  LOG_INFO("Response: status=0x%02x, size=%d",
           response[0], (uint16_t)(response[1] | (response[2] << 8)));
  LOG_INFO("Test 4: PASSED - protocol format correct");

  return OK_STATUS();
}

/**
 * Test 5: Consecutive requests (randomness check)
 */
static status_t test_consecutive_requests(void) {
  LOG_INFO("=== Test 5: Consecutive requests ===");

  uint8_t resp1_status, resp2_status;
  uint8_t resp1_buffer[64], resp2_buffer[64];
  uint16_t resp1_size, resp2_size;

  // First request
  LOG_INFO("Request 1: 64 bytes");
  process_request(CMD_GET_RANDOM, 64, &resp1_status, resp1_buffer, &resp1_size);
  TRY_CHECK(resp1_status == STATUS_SUCCESS, "Request 1 failed");

  // Second request
  LOG_INFO("Request 2: 64 bytes");
  process_request(CMD_GET_RANDOM, 64, &resp2_status, resp2_buffer, &resp2_size);
  TRY_CHECK(resp2_status == STATUS_SUCCESS, "Request 2 failed");

  // Verify responses are different (randomness)
  TRY_CHECK(memcmp(resp1_buffer, resp2_buffer, 64) != 0,
            "Two consecutive requests produced identical data!");

  LOG_INFO("Test 5: PASSED - consecutive requests produce different data");
  return OK_STATUS();
}

/**
 * Test 6: Boundary values
 */
static status_t test_boundary_values(void) {
  LOG_INFO("=== Test 6: Boundary values ===");

  uint8_t resp_status;
  uint8_t resp_buffer[MAX_RANDOM_SIZE];
  uint16_t resp_size;

  // Test minimum valid size (1)
  LOG_INFO("Test 6.1: Minimum size (1 byte)");
  process_request(CMD_GET_RANDOM, 1, &resp_status, resp_buffer, &resp_size);
  TRY_CHECK(resp_status == STATUS_SUCCESS, "Size=1 should be valid");
  TRY_CHECK(resp_size == 1, "Expected 1 byte response");
  LOG_INFO("Test 6.1: PASSED");

  // Test maximum valid size (256)
  LOG_INFO("Test 6.2: Maximum size (256 bytes)");
  process_request(CMD_GET_RANDOM, 256, &resp_status, resp_buffer, &resp_size);
  TRY_CHECK(resp_status == STATUS_SUCCESS, "Size=256 should be valid");
  TRY_CHECK(resp_size == 256, "Expected 256 byte response");
  TRY_CHECK(!is_all_zeros(resp_buffer, 256), "256 byte buffer is all zeros");
  LOG_INFO("Test 6.2: PASSED");

  LOG_INFO("Test 6: Boundary value tests PASSED");
  return OK_STATUS();
}

bool test_main(void) {
  status_t result;

  LOG_INFO("======================================");
  LOG_INFO("  FAPI GetRandom Server Test");
  LOG_INFO("======================================");
  LOG_INFO("");

  // Initialize CSRNG
  LOG_INFO("Initializing CSRNG...");
  result = csrng_init();
  CHECK_STATUS_OK(result);
  LOG_INFO("CSRNG initialized successfully");
  LOG_INFO("");

  // Run all tests
  result = test_valid_requests();
  CHECK_STATUS_OK(result);
  LOG_INFO("");

  result = test_invalid_command();
  CHECK_STATUS_OK(result);
  LOG_INFO("");

  result = test_invalid_sizes();
  CHECK_STATUS_OK(result);
  LOG_INFO("");

  result = test_protocol_format();
  CHECK_STATUS_OK(result);
  LOG_INFO("");

  result = test_consecutive_requests();
  CHECK_STATUS_OK(result);
  LOG_INFO("");

  result = test_boundary_values();
  CHECK_STATUS_OK(result);
  LOG_INFO("");

  LOG_INFO("======================================");
  LOG_INFO("  ALL SERVER TESTS PASSED!");
  LOG_INFO("======================================");

  return true;
}
