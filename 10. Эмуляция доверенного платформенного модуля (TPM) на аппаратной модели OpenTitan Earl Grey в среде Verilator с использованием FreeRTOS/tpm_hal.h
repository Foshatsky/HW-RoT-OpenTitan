#ifndef TPM_HAL_H_
#define TPM_HAL_H_

#include <stdint.h>
#include <stddef.h>

// Инициализация TPM
void tpm_init(void);

// Отправить команду (массив байт) на TPM
void tpm_send(const uint8_t *cmd, size_t len);

// Получить ответ от TPM (заглушка возвращает фиксированный ответ)
void tpm_recv(uint8_t *resp, size_t *len);

#endif
