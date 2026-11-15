# Server-Side тестирование FAPI интерфейса

### 1. Концепция тестирования

Для верификации минимального интерфейса FAPI GetRandom был разработан comprehensive тестовый фреймворк.

**Файл:** [`sw/device/tests/fapi_getrandom_server_test.c`](source/fapi_getrandom_server_test.c)

**Ключевая идея:** Извлечь и протестировать ядро серверной логики - функцию обработки FAPI запросов.

```c
/**
 * Функция обработки FAPI GetRandom запроса
 * Извлечена из crypto_task для изолированного тестирования
 */
static void process_request(uint8_t cmd, uint16_t size,
                           uint8_t *response_status,
                           uint8_t *response_buffer,
                           uint16_t *response_size) {
  // Валидация команды и размера
  if (cmd == CMD_GET_RANDOM && size > 0 && size <= MAX_RANDOM_SIZE) {
    // Генерация через CSRNG
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
    // Невалидный запрос
    *response_status = STATUS_ERROR;
    *response_size = 0;
    LOG_ERROR("Invalid request: cmd=0x%02x, size=%d", cmd, size);
  }
}
```

Эта функция представляет минимальный интерфейс FAPI GetRandom - она принимает запрос, генерирует случайные данные и формирует ответ.

### 2. Тестовое покрытие

Разработано 6 категорий тестов, покрывающих все аспекты интерфейса:

**Тест 1: Валидные запросы**

```c
static status_t test_valid_requests(void) {
  const uint16_t test_sizes[] = {16, 32, 64, 128, 256};

  for (each size) {
    // 1. Отправка запроса
    process_request(CMD_GET_RANDOM, size, &status, buffer, &resp_size);

    // 2. Проверка статуса SUCCESS
    TRY_CHECK(status == STATUS_SUCCESS);

    // 3. Проверка размера ответа
    TRY_CHECK(resp_size == size);

    // 4. Проверка что данные не нулевые
    TRY_CHECK(!is_all_zeros(buffer, resp_size));
  }
}
```

**Тест 2: Невалидные команды**

```c
static status_t test_invalid_command(void) {
  // Отправка команды 0x99 вместо 0x01
  process_request(0x99, 16, &status, buffer, &resp_size);

  // Ожидается отклонение с STATUS_ERROR
  TRY_CHECK(status == STATUS_ERROR);
  TRY_CHECK(resp_size == 0);
}
```

**Тест 3: Невалидные размеры**

```c
static status_t test_invalid_sizes(void) {
  // Размер 0 - должен быть отклонен
  process_request(CMD_GET_RANDOM, 0, &status, buffer, &resp_size);
  TRY_CHECK(status == STATUS_ERROR);

  // Размер 300 (>256) - должен быть отклонен
  process_request(CMD_GET_RANDOM, 300, &status, buffer, &resp_size);
  TRY_CHECK(status == STATUS_ERROR);
}
```

**Тест 4: Формат протокола**

```c
static status_t test_protocol_format(void) {
  // Построение request в формате протокола
  uint8_t request[3] = {CMD_GET_RANDOM, 32, 0};

  // Парсинг request
  uint8_t cmd = request[0];
  uint16_t size = (uint16_t)(request[1] | (request[2] << 8));

  // Обработка
  process_request(cmd, size, &status, buffer, &resp_size);

  // Построение response в формате протокола
  uint8_t response[3 + MAX_RANDOM_SIZE];
  response[0] = status;
  response[1] = (uint8_t)(resp_size & 0xFF);
  response[2] = (uint8_t)(resp_size >> 8);
  memcpy(&response[3], buffer, resp_size);

  // Верификация формата
  TRY_CHECK(response[0] == STATUS_SUCCESS);
  TRY_CHECK(response[1] == 32);
}
```

**Тест 5: Случайность данных**

```c
static status_t test_consecutive_requests(void) {
  uint8_t buffer1[64], buffer2[64];

  // Два последовательных запроса одинакового размера
  process_request(CMD_GET_RANDOM, 64, &status1, buffer1, &size1);
  process_request(CMD_GET_RANDOM, 64, &status2, buffer2, &size2);

  // Данные должны различаться (свойство случайности)
  TRY_CHECK(memcmp(buffer1, buffer2, 64) != 0);
}
```

**Тест 6: Граничные значения**

```c
static status_t test_boundary_values(void) {
  // Минимальный размер: 1 байт
  process_request(CMD_GET_RANDOM, 1, &status, buffer, &resp_size);
  TRY_CHECK(status == STATUS_SUCCESS && resp_size == 1);

  // Максимальный размер: 256 байт
  process_request(CMD_GET_RANDOM, 256, &status, buffer, &resp_size);
  TRY_CHECK(status == STATUS_SUCCESS && resp_size == 256);
}
```

### 3. Сборка тестов

Команда сборки тестового фреймворка:

```bash
./bazelisk.sh build //sw/device/tests:fapi_getrandom_server_test_sim_verilator
```

Результат:

```bash
INFO: Analyzed target //sw/device/tests:fapi_getrandom_server_test_sim_verilator
      (832 packages loaded, 33775 targets configured).
INFO: Build completed successfully, 10 total actions
Target //sw/device/tests:fapi_getrandom_server_test_sim_verilator up-to-date:
  bazel-bin/sw/device/tests/fapi_getrandom_server_test_sim_verilator.bash
```

