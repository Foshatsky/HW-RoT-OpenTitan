// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0.
// SPDX-License-Identifier: Apache-2.0

/**
 * CSRNG Unit Test for FAPI GetRandom Server
 *
 * This test verifies that CSRNG initialization and random generation
 * works correctly in Verilator simulation without FreeRTOS/UART dependencies.
 */

#include <string.h>
#include <stdbool.h>

#include "sw/device/lib/runtime/log.h"
#include "sw/device/lib/testing/test_framework/check.h"
#include "sw/device/lib/testing/test_framework/ottf_main.h"
#include "sw/device/lib/dif/dif_csrng.h"
#include "hw/top_earlgrey/sw/autogen/top_earlgrey.h"

OTTF_DEFINE_TEST_CONFIG();

static dif_csrng_t csrng;

/**
 * Initialize CSRNG with deterministic seed (for simulation)
 */
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

/**
 * Check if buffer is all zeros (indicates failure)
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
 * Print buffer as hex
 */
static void print_hex(const char *label, const uint8_t *buf, size_t len) {
  LOG_INFO("%s (%zu bytes):", label, len);
  for (size_t i = 0; i < len; ++i) {
    if (i % 16 == 0) {
      LOG_INFO("  %04zx:", i);
    }
    // Note: LOG_INFO doesn't support inline formatting well, so we log separately
  }

  // Print as single line for small buffers
  if (len <= 32) {
    LOG_INFO("  Data: %02x %02x %02x %02x %02x %02x %02x %02x "
             "%02x %02x %02x %02x %02x %02x %02x %02x",
             buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7],
             len > 8 ? buf[8] : 0, len > 9 ? buf[9] : 0,
             len > 10 ? buf[10] : 0, len > 11 ? buf[11] : 0,
             len > 12 ? buf[12] : 0, len > 13 ? buf[13] : 0,
             len > 14 ? buf[14] : 0, len > 15 ? buf[15] : 0);
  }
}

/**
 * Test: Generate different sizes of random data
 */
static status_t test_various_sizes(void) {
  LOG_INFO("=== Test: Various buffer sizes ===");

  const size_t test_sizes[] = {16, 32, 64, 128, 256};
  const size_t num_tests = sizeof(test_sizes) / sizeof(test_sizes[0]);

  for (size_t i = 0; i < num_tests; ++i) {
    size_t size = test_sizes[i];
    uint8_t buffer[256];
    memset(buffer, 0, sizeof(buffer));

    LOG_INFO("Test %zu/%zu: Generating %zu bytes...", i + 1, num_tests, size);
    TRY(csrng_get_random(buffer, size));

    // Verify data is not all zeros
    TRY_CHECK(!is_all_zeros(buffer, size),
              "Generated data is all zeros for size %zu", size);

    // Print first few bytes
    print_hex("Generated data", buffer, size > 32 ? 32 : size);

    LOG_INFO("Test %zu/%zu: PASSED (size=%zu)", i + 1, num_tests, size);
  }

  return OK_STATUS();
}

/**
 * Test: Verify randomness - two consecutive calls should produce different data
 */
static status_t test_randomness(void) {
  LOG_INFO("=== Test: Verify randomness ===");

  uint8_t buf1[64];
  uint8_t buf2[64];

  memset(buf1, 0, sizeof(buf1));
  memset(buf2, 0, sizeof(buf2));

  LOG_INFO("Generating first 64 bytes...");
  TRY(csrng_get_random(buf1, 64));
  print_hex("First generation", buf1, 16);

  LOG_INFO("Generating second 64 bytes...");
  TRY(csrng_get_random(buf2, 64));
  print_hex("Second generation", buf2, 16);

  // Verify they are different
  TRY_CHECK(memcmp(buf1, buf2, 64) != 0,
            "Two consecutive generations produced identical data!");

  LOG_INFO("Test PASSED: Data is different between calls");
  return OK_STATUS();
}

/**
 * Test: Edge cases
 */
static status_t test_edge_cases(void) {
  LOG_INFO("=== Test: Edge cases ===");

  // Test: Zero length request
  LOG_INFO("Test: Zero length request");
  uint8_t dummy;
  TRY(csrng_get_random(&dummy, 0));
  LOG_INFO("Zero length: PASSED");

  // Test: Single byte
  LOG_INFO("Test: Single byte");
  uint8_t single;
  TRY(csrng_get_random(&single, 1));
  LOG_INFO("Single byte: 0x%02x", single);
  LOG_INFO("Single byte: PASSED");

  // Test: Odd number of bytes (not word-aligned)
  LOG_INFO("Test: 17 bytes (not word-aligned)");
  uint8_t odd[17];
  TRY(csrng_get_random(odd, 17));
  TRY_CHECK(!is_all_zeros(odd, 17), "Odd-sized buffer is all zeros");
  LOG_INFO("Odd size: PASSED");

  return OK_STATUS();
}

/**
 * Test: Statistical properties (basic check)
 */
static status_t test_statistics(void) {
  LOG_INFO("=== Test: Basic statistical properties ===");

  uint8_t buffer[256];
  TRY(csrng_get_random(buffer, 256));

  // Count bit distribution
  uint32_t bit_count = 0;
  for (size_t i = 0; i < 256; ++i) {
    uint8_t byte = buffer[i];
    for (int bit = 0; bit < 8; ++bit) {
      if (byte & (1 << bit)) {
        bit_count++;
      }
    }
  }

  // Total bits: 256 * 8 = 2048
  // Expected ~50% ones: ~1024
  // Accept range: 900-1148 (roughly 44%-56%)
  uint32_t percentage = (bit_count * 100) / 2048;
  LOG_INFO("Bit count: %u / 2048 (%u%%)", bit_count, percentage);

  TRY_CHECK(bit_count >= 900 && bit_count <= 1148,
            "Bit distribution outside acceptable range: %u", bit_count);

  LOG_INFO("Statistics test: PASSED");
  return OK_STATUS();
}

bool test_main(void) {
  status_t result;

  LOG_INFO("======================================");
  LOG_INFO("  FAPI CSRNG Unit Test");
  LOG_INFO("======================================");

  // Initialize CSRNG
  LOG_INFO("Initializing CSRNG...");
  result = csrng_init();
  CHECK_STATUS_OK(result);
  LOG_INFO("CSRNG initialized successfully");
  LOG_INFO("");

  // Run tests
  result = test_various_sizes();
  CHECK_STATUS_OK(result);
  LOG_INFO("");

  result = test_randomness();
  CHECK_STATUS_OK(result);
  LOG_INFO("");

  result = test_edge_cases();
  CHECK_STATUS_OK(result);
  LOG_INFO("");

  result = test_statistics();
  CHECK_STATUS_OK(result);
  LOG_INFO("");

  LOG_INFO("======================================");
  LOG_INFO("  ALL TESTS PASSED");
  LOG_INFO("======================================");

  return true;
}
