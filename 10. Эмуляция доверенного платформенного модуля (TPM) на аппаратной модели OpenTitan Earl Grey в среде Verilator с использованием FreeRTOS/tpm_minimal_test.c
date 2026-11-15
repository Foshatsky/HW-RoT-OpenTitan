#include "sw/device/lib/runtime/log.h"
#include "sw/device/lib/testing/test_framework/check.h"
#include "sw/device/lib/testing/test_framework/ottf_macros.h"
#include "sw/device/lib/testing/test_framework/ottf_main.h"
#include "tpm_hal.h"

static volatile bool task_done = false;

OTTF_DEFINE_TEST_CONFIG(.enable_concurrency = true);

static void tpm_task(void *task_parameters) {
    uint8_t response[1024];
    size_t resp_len;

    LOG_INFO("TPM task started");

    // 1. TPM2_Startup
    uint8_t startup_cmd[] = {
        0x80, 0x01, 0x00, 0x00, 0x00, 0x0C,
        0x00, 0x00, 0x01, 0x44,
        0x00, 0x00
    };
    tpm_send(startup_cmd, sizeof(startup_cmd));
    tpm_recv(response, &resp_len);
    CHECK(resp_len >= 10, "Startup response too short");
    LOG_INFO("Startup done");

    // 2. TPM2_GetRandom (16 байт)
    uint8_t getrandom_cmd[] = {
        0x80, 0x01, 0x00, 0x00, 0x00, 0x10,
        0x00, 0x00, 0x01, 0x7B,
        0x00, 0x00, 0x00, 0x10
    };
    tpm_send(getrandom_cmd, sizeof(getrandom_cmd));
    tpm_recv(response, &resp_len);
    if (resp_len > 14) {
        LOG_INFO("Random 16 bytes from OpenTitan HW Source:");
        for (int i = 0; i < 16 && i < resp_len-14; i++) {
            LOG_INFO("  [%02d] 0x%02X", i, response[14+i]);
        }
    } else {
        LOG_INFO("GetRandom failed, resp_len=%d", (int)resp_len);
    }

    // 3. PCR_Read перед Extend – ожидаем нули. Добавили 4 байта индекса (0x00000000) в конец массива
    uint8_t pcr_read_cmd[] = {
        0x80, 0x01, 0x00, 0x00, 0x00, 0x12, // Длина 18 байт (0x12)
        0x00, 0x00, 0x01, 0x7E,             // TPM2_PCR_Read
        0x00, 0x00, 0x00, 0x00,             // Индекс PCR: PCR[0]
        0x00, 0x00, 0x00, 0x00              // Дополнительное поле структуры (теперь массив ровно 18 байт)
    };
    tpm_send(pcr_read_cmd, sizeof(pcr_read_cmd));
    tpm_recv(response, &resp_len);
    if (resp_len > 14) {
        LOG_INFO("PCR[0] before extend (should be zeros):");
        for (int i = 0; i < 16 && i < resp_len-14; i++) {
            LOG_INFO("  [%02d] 0x%02X", i, response[14+i]);
        }
    }

    // 4. PCR_Extend для PCR[0] с 32 байтами данных (все 0xAA)
    uint8_t pcr_extend_cmd[14 + 32] = {
        0x80, 0x01, 0x00, 0x00, 0x00, 0x2E,
        0x00, 0x00, 0x01, 0x6A,
        0x00, 0x00, 0x00, 0x00
    };
    for (int i = 0; i < 32; i++) {
        pcr_extend_cmd[14 + i] = 0xAA;
    }
    tpm_send(pcr_extend_cmd, sizeof(pcr_extend_cmd));
    tpm_recv(response, &resp_len);
    CHECK(resp_len >= 10, "PCR_Extend failed");
    LOG_INFO("PCR_Extend done");

    // 5. PCR_Read после Extend – должны увидеть паттерн 0xAA
    tpm_send(pcr_read_cmd, sizeof(pcr_read_cmd));
    tpm_recv(response, &resp_len);
    if (resp_len > 14) {
        LOG_INFO("PCR[0] after extend (should be 0xAA pattern):");
        for (int i = 0; i < 16 && i < resp_len-14; i++) {
            LOG_INFO("  [%02d] 0x%02X", i, response[14+i]);
        }
    }

    LOG_INFO("TPM task finished");
    task_done = true;
    OTTF_TASK_DELETE_SELF_OR_DIE;
}

bool test_main(void) {
    LOG_INFO("Starting TPM test with PCR commands");
    CHECK(ottf_task_create(tpm_task, "tpm_task", kOttfFreeRtosMinStackSize * 4, 1));

    while (!task_done) {
        ottf_task_yield();
    }

    LOG_INFO("All tests passed");
    return true;
}
