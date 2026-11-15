# Standalone клиент (опциональный компонент)

Для возможного будущего E2E тестирования был также реализован standalone клиент:

**Файл:** [`/root/opentitan/fapi_getrandom_client.c`](source/fapi_getrandom_client.c)

```c
// Основная функциональность клиента
int main(int argc, char *argv[]) {
  // Открытие PTY (pseudo-terminal) к серверу
  int fd = open_uart_pty(pty_path);

  // Тестирование различных размеров
  const size_t test_sizes[] = {16, 32, 64, 128, 256};

  for (each size) {
    // Формирование FAPI запроса
    uint8_t request[3] = {CMD_GET_RANDOM, size & 0xFF, size >> 8};
    write(fd, request, 3);

    // Получение FAPI ответа
    uint8_t response[3 + 256];
    read(fd, response, 3 + size);

    // Валидация ответа
    validate_response(response, size);
  }
}
```

**Сборка клиента:**

```bash
gcc -o fapi_client fapi_getrandom_client.c -Wall -Wextra
```

**Результат (компиляция без ошибок и предупреждений):**

```bash
$ ls -lh fapi_client
-rwxr-xr-x 1 root root 17K Jan 10 10:00 fapi_client
```

Клиент готов к использованию, если потребуется E2E тестирование в будущем.