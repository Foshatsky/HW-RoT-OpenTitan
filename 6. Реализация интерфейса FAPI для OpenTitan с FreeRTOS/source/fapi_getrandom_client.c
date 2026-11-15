// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

/**
 * FAPI GetRandom Client
 *
 * Simple C client to communicate with FAPI GetRandom server via UART PTY.
 * Tests the FreeRTOS-based server running in Verilator simulation.
 *
 * Usage:
 *   gcc -o fapi_client fapi_getrandom_client.c
 *   ./fapi_client /dev/pts/X
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <stdint.h>

// Protocol constants (must match server)
#define CMD_GET_RANDOM 0x01
#define STATUS_SUCCESS 0x00
#define STATUS_ERROR 0x01

/**
 * Open and configure UART PTY for raw mode communication.
 *
 * @param pty_path Path to PTY device (e.g., "/dev/pts/5")
 * @return File descriptor or -1 on error
 */
int open_uart_pty(const char *pty_path) {
    int fd = open(pty_path, O_RDWR | O_NOCTTY);
    if (fd < 0) {
        perror("Failed to open PTY");
        return -1;
    }

    // Configure PTY for raw mode
    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) {
        perror("tcgetattr");
        close(fd);
        return -1;
    }

    // Set raw mode (no echo, no canonical processing)
    cfmakeraw(&tty);

    // Set baud rate to 115200
    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        perror("tcsetattr");
        close(fd);
        return -1;
    }

    return fd;
}

/**
 * Send GetRandom request to server.
 *
 * Request format: [cmd][size_lo][size_hi] (3 bytes)
 *
 * @param fd File descriptor for PTY
 * @param size Number of random bytes requested (1-256)
 * @return 0 on success, -1 on error
 */
int send_request(int fd, uint16_t size) {
    uint8_t request[3] = {
        CMD_GET_RANDOM,
        (uint8_t)(size & 0xFF),         // size_lo
        (uint8_t)((size >> 8) & 0xFF)   // size_hi
    };

    ssize_t written = write(fd, request, 3);
    if (written != 3) {
        fprintf(stderr, "Failed to write request (wrote %zd/3 bytes)\n", written);
        return -1;
    }

    printf("Sent request: cmd=0x%02x, size=%u\n", CMD_GET_RANDOM, size);
    return 0;
}

/**
 * Receive and validate response from server.
 *
 * Response format: [status][size_lo][size_hi][data...] (3+ bytes)
 *
 * @param fd File descriptor for PTY
 * @param expected_size Expected number of random bytes
 * @return 0 on success, -1 on error
 */
int receive_response(int fd, uint16_t expected_size) {
    // Read response header (3 bytes)
    uint8_t header[3];
    ssize_t bytes_read = read(fd, header, 3);
    if (bytes_read != 3) {
        fprintf(stderr, "Failed to read response header (read %zd/3 bytes)\n", bytes_read);
        return -1;
    }

    uint8_t status = header[0];
    uint16_t response_size = (uint16_t)header[1] | ((uint16_t)header[2] << 8);

    printf("Received header: status=0x%02x, size=%u\n", status, response_size);

    // Validate status
    if (status != STATUS_SUCCESS) {
        fprintf(stderr, "ERROR: Server returned error status: 0x%02x\n", status);
        return -1;
    }

    // Validate size
    if (response_size != expected_size) {
        fprintf(stderr, "ERROR: Size mismatch (expected %u, got %u)\n",
                expected_size, response_size);
        return -1;
    }

    // Read random data
    uint8_t *data = malloc(response_size);
    if (!data) {
        perror("malloc");
        return -1;
    }

    bytes_read = read(fd, data, response_size);
    if (bytes_read != (ssize_t)response_size) {
        fprintf(stderr, "Failed to read random data (read %zd/%u bytes)\n",
                bytes_read, response_size);
        free(data);
        return -1;
    }

    // Print random data (first 32 bytes or all if smaller)
    unsigned int print_size = response_size < 32 ? response_size : 32;
    printf("Random data (first %u bytes):\n", print_size);
    for (unsigned int i = 0; i < print_size; i++) {
        printf("%02x ", data[i]);
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }
    if (print_size % 16 != 0) {
        printf("\n");
    }

    // Verify not all zeros
    int all_zero = 1;
    for (unsigned int i = 0; i < response_size; i++) {
        if (data[i] != 0) {
            all_zero = 0;
            break;
        }
    }

    free(data);

    if (all_zero) {
        fprintf(stderr, "ERROR: All random bytes are zero!\n");
        return -1;
    }

    printf("✓ Random data verified (not all zeros)\n");
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "FAPI GetRandom Client\n");
        fprintf(stderr, "Usage: %s <pty_path>\n", argv[0]);
        fprintf(stderr, "Example: %s /dev/pts/5\n", argv[0]);
        fprintf(stderr, "\n");
        fprintf(stderr, "This client connects to the FAPI GetRandom server via UART PTY\n");
        fprintf(stderr, "and tests random number generation with different request sizes.\n");
        return 1;
    }

    const char *pty_path = argv[1];
    printf("=== FAPI GetRandom Client ===\n");
    printf("Connecting to PTY: %s\n", pty_path);

    // Open PTY
    int fd = open_uart_pty(pty_path);
    if (fd < 0) {
        return 1;
    }

    printf("Connected successfully\n");
    printf("Waiting for server to be ready...\n\n");

    // Wait for server to initialize
    sleep(2);

    // Test multiple request sizes
    uint16_t sizes[] = {16, 32, 64, 128, 256};
    int num_tests = sizeof(sizes) / sizeof(sizes[0]);
    int passed = 0;

    for (int i = 0; i < num_tests; i++) {
        printf("=== Test %d/%d: Requesting %u bytes ===\n",
               i + 1, num_tests, sizes[i]);

        if (send_request(fd, sizes[i]) != 0) {
            fprintf(stderr, "Test %d FAILED (send error)\n", i + 1);
            close(fd);
            return 1;
        }

        if (receive_response(fd, sizes[i]) != 0) {
            fprintf(stderr, "Test %d FAILED (receive error)\n", i + 1);
            close(fd);
            return 1;
        }

        printf("Test %d PASSED\n\n", i + 1);
        passed++;

        // Give server time between requests
        sleep(1);
    }

    close(fd);

    printf("=== Test Summary ===\n");
    printf("Passed: %d/%d\n", passed, num_tests);

    if (passed == num_tests) {
        printf("✓ All tests PASSED\n");
        return 0;
    } else {
        printf("✗ Some tests FAILED\n");
        return 1;
    }
}
