// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0.
// SPDX-License-Identifier: Apache-2.0

/**
 * FAPI GetRandom Demo - Simple demonstration of 16 byte generation
 *
 * This is a minimal demonstration that shows:
 * 1. CSRNG initialization
 * 2. Generation of exactly 16 random bytes
 * 3. Visual output of the generated data
 */

#include <string.h>
#include <stdbool.h>

#include "sw/device/lib/runtime/log.h"
#include "sw/device/lib/testing/test_framework/check.h"
#include "sw/device/lib/testing/test_framework/ottf_main.h"
#include "sw/device/lib/dif/dif_csrng.h"
#include "hw/top_earlgrey/sw/autogen/top_earlgrey.h"

OTTF_DEFINE_TEST_CONFIG();

// Global CSRNG state
static dif_csrng_t csrng;

/**
 * Initialize CSRNG with deterministic seed (for simulation)
 */
static status_t csrng_init(void) {
  LOG_INFO("Initializing CSRNG...");

  TRY(dif_csrng_init(
      mmio_region_from_addr(TOP_EARLGREY_CSRNG_BASE_ADDR),
      &csrng));

  TRY(dif_csrng_configure(&csrng));

  // NIST CTR_DRBG Known-Answer-Test seed
  dif_csrng_seed_material_t seed = {
      .seed_material = {
          0x73bec010, 0x9262474c, 0x16a30f76, 0x531b51de,
          0x2ee494e5, 0xdfec9db3, 0xcb7a879d, 0x5600419c,
          0xca79b0b0, 0xdda33b5c, 0xa468649e, 0xdf5d73fa
      },
      .seed_material_len = 12,
  };

  TRY(dif_csrng_instantiate(&csrng, kDifCsrngEntropySrcToggleDisable, &seed));
  LOG_INFO("CSRNG initialized successfully");

  return OK_STATUS();
}

/**
 * Generate random bytes using CSRNG
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

bool test_main(void) {
  LOG_INFO("===========================================");
  LOG_INFO("  FAPI GetRandom Demo: 16 Bytes");
  LOG_INFO("===========================================");
  LOG_INFO("");

  // Initialize CSRNG
  CHECK_STATUS_OK(csrng_init());
  LOG_INFO("");

  // Generate 16 random bytes
  LOG_INFO("Generating 16 random bytes...");
  uint8_t buffer[16];
  memset(buffer, 0, sizeof(buffer));

  CHECK_STATUS_OK(csrng_get_random(buffer, 16));

  // Display the generated data
  LOG_INFO("");
  LOG_INFO("Generated random data (16 bytes):");
  LOG_INFO("");
  LOG_INFO("  Hex: %02x %02x %02x %02x %02x %02x %02x %02x "
           "%02x %02x %02x %02x %02x %02x %02x %02x",
           buffer[0], buffer[1], buffer[2], buffer[3],
           buffer[4], buffer[5], buffer[6], buffer[7],
           buffer[8], buffer[9], buffer[10], buffer[11],
           buffer[12], buffer[13], buffer[14], buffer[15]);
  LOG_INFO("");
  LOG_INFO("  Dec: %3u %3u %3u %3u %3u %3u %3u %3u "
           "%3u %3u %3u %3u %3u %3u %3u %3u",
           buffer[0], buffer[1], buffer[2], buffer[3],
           buffer[4], buffer[5], buffer[6], buffer[7],
           buffer[8], buffer[9], buffer[10], buffer[11],
           buffer[12], buffer[13], buffer[14], buffer[15]);
  LOG_INFO("");

  // Verify not all zeros
  bool all_zeros = true;
  for (size_t i = 0; i < 16; ++i) {
    if (buffer[i] != 0) {
      all_zeros = false;
      break;
    }
  }

  CHECK(!all_zeros, "ERROR: All bytes are zero!");

  LOG_INFO("===========================================");
  LOG_INFO("  SUCCESS: Generated 16 non-zero bytes");
  LOG_INFO("===========================================");

  return true;
}
