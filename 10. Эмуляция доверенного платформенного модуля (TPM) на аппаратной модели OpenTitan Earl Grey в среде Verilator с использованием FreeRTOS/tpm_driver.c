#include "tpm_hal.h"
#include "sw/device/lib/runtime/log.h"
#include <string.h>

static inline uint16_t be16_to_cpu(const uint8_t *p) {
    return (uint16_t)(((uint16_t)p[0] << 8) | p[1]);
}
static inline uint32_t be32_to_cpu(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
            ((uint32_t)p[2] << 8)  | p[3];
}
static inline void cpu_to_be16(uint16_t val, uint8_t *p) {
    p[0] = (val >> 8) & 0xFF;
    p[1] = val & 0xFF;
}
static inline void cpu_to_be32(uint32_t val, uint8_t *p) {
    p[0] = (val >> 24) & 0xFF;
    p[1] = (val >> 16) & 0xFF;
    p[2] = (val >> 8) & 0xFF;
    p[3] = val & 0xFF;
}

static bool tpm_started = false;
static uint8_t last_cmd[256];
static size_t last_len = 0;
static uint8_t pcr[24][32] = {0};
static uint8_t rsp[1024];

// Функция чтения аппаратного счетчика циклов процессора Ibex (RISC-V)
static inline uint32_t read_cpu_cycles(void) {
    uint32_t cycles;
    // Читаем CSR-регистр mcycle (счетчик тактов ядра)
    asm volatile("csrr %0, mcycle" : "=r"(cycles));
    return cycles;
}

void tpm_init(void) {
    LOG_INFO("TPM init: Native RISC-V Cycle Counter Entropy Source active!");
    tpm_started = false;
    memset(pcr, 0, sizeof(pcr));
}

void tpm_send(const uint8_t *cmd, size_t len) {
    LOG_INFO("TPM send, len=%d", (int)len);
    if (len < 10) {
        LOG_INFO("Command too short, ignore");
        return;
    }
    uint16_t tag = be16_to_cpu(cmd);
    uint32_t size = be32_to_cpu(cmd + 2);
    uint32_t code = be32_to_cpu(cmd + 6);
    LOG_INFO("  tag=0x%04X, size=%u, code=0x%08X", tag, size, code);

    if (len <= sizeof(last_cmd)) {
        memcpy(last_cmd, cmd, len);
        last_len = len;
    }
}

void tpm_recv(uint8_t *resp, size_t *len) {
    LOG_INFO("TPM recv (build response)");
    uint32_t rc = 0x00000501;
    uint16_t tag = 0x8001;
    uint32_t code = 0;
    size_t resp_data_len = 0;

    if (last_len >= 10) {
        uint16_t cmd_tag = be16_to_cpu(last_cmd);
        uint32_t cmd_code = be32_to_cpu(last_cmd + 6);
        tag = cmd_tag;
        code = cmd_code;

        if (cmd_code == 0x00000144) {   // Startup
            tpm_started = true;
            rc = 0;
            LOG_INFO("TPM2_Startup accepted, TPM started");
        }
        else if (cmd_code == 0x0000017B) { // GetRandom
            if (!tpm_started) {
                rc = 0x00000541;
                LOG_INFO("GetRandom before Startup - error");
            } else if (last_len < 14) {
                rc = 0x00000502;
                LOG_INFO("GetRandom command too short");
            } else {
                uint32_t req_bytes = be32_to_cpu(last_cmd + 10);
                if (req_bytes > 1024 - 14) req_bytes = 1024 - 14;
                LOG_INFO("GetRandom: requesting %u bytes", req_bytes);

                // Заполняем массив случайными байтами из регистра mcycle
                uint32_t pool = 0;
                for (size_t i = 0; i < req_bytes; i++) {
                    if ((i % 4) == 0) {
                        pool = read_cpu_cycles();
                    }
                    rsp[14 + i] = (pool >> ((i % 4) * 8)) & 0xFF;
                }
                
                LOG_INFO("GetRandom: generated %u bytes using Native RISC-V mcycle Entropy", req_bytes);
                resp_data_len = req_bytes;
                rc = 0;
            }
        }
        else if (cmd_code == 0x0000016A) { // PCR_Extend
            if (!tpm_started) {
                rc = 0x00000541;
                LOG_INFO("PCR_Extend before Startup - error");
            } else if (last_len < 14 + 32) {
                rc = 0x00000502;
                LOG_INFO("PCR_Extend too short (need 32 bytes)");
            } else {
                uint32_t pcr_index = be32_to_cpu(last_cmd + 10);
                if (pcr_index >= 24) {
                    rc = 0x00000182;
                    LOG_INFO("PCR_Extend invalid index %u", pcr_index);
                } else {
                    const uint8_t *extend_data = last_cmd + 14;
                    for (int i = 0; i < 32; i++) {
                        pcr[pcr_index][i] ^= extend_data[i];
                    }
                    LOG_INFO("PCR[%u] extended (XOR)", pcr_index);
                    rc = 0;
                }
            }
        }
        else if (cmd_code == 0x0000017E) { // PCR_Read
            if (!tpm_started) {
                rc = 0x00000541;
                LOG_INFO("PCR_Read before Startup - error");
            } else if (last_len < 14 + 4) { 
                rc = 0x00000502;
                LOG_INFO("PCR_Read too short");
            } else {
                uint32_t pcr_index = be32_to_cpu(last_cmd + 10);
                if (pcr_index >= 24) {
                    rc = 0x00000182;
                    LOG_INFO("PCR_Read invalid index %u", pcr_index);
                } else {
                    resp_data_len = 32;
                    memcpy(rsp + 14, pcr[pcr_index], 32);
                    LOG_INFO("PCR[%u] read successfully", pcr_index);
                    rc = 0;
                }
            }
        }
        else {
            LOG_INFO("Unsupported command 0x%08X", cmd_code);
        }
    } else {
        LOG_INFO("No valid command stored");
    }

    cpu_to_be16(tag, rsp);
    cpu_to_be32(14 + resp_data_len, rsp + 2);
    cpu_to_be32(code, rsp + 6);
    cpu_to_be32(rc, rsp + 10);
    *len = 14 + resp_data_len;
    LOG_INFO("Response length = %d, rc=0x%08X", (int)*len, rc);
    memcpy(resp, rsp, *len);
}
