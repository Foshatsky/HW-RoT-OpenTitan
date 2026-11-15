# Реализация серверной части с FAPI интерфейсом

### 1. Структура сервера

Разработан FreeRTOS сервер, реализующий минимальный интерфейс FAPI GetRandom.

**Файл:** [`sw/device/tests/fapi_getrandom_server.c`](source/fapi_getrandom_server.c)

Архитектурные компоненты:

```c
// Включение FreeRTOS concurrency для асинхронной обработки
OTTF_DEFINE_TEST_CONFIG(.enable_concurrency = true);

// Глобальное состояние CSRNG
static dif_csrng_t csrng;

// Две FreeRTOS задачи для разделения ответственности
static void uart_task(void *params);    // Коммуникационный уровень
static void crypto_task(void *params);   // Криптографический уровень
```

Преимущества архитектуры:
- Четкое разделение ответственности между задачами
- Асинхронная обработка запросов
- Потокобезопасная коммуникация через volatile переменные
- Масштабируемость для добавления новых FAPI функций

### 2. FAPI протокол коммуникации

Реализован простой и эффективный протокол для FAPI GetRandom.

**Формат запроса (3 байта):**

```
Байт 0: Команда (0x01 = GetRandom)
Байт 1: Размер (младший байт)
Байт 2: Размер (старший байт)
```

**Формат ответа (3 + N байт):**

```
Байт 0: Статус (0x00 = успех, 0x01 = ошибка)
Байт 1: Размер данных (младший байт)
Байт 2: Размер данных (старший байт)
Байты 3+: Случайные данные (если статус = успех)
```

**Поддерживаемые размеры:** 1-256 байт

**Обработка ошибок:**

| Условие          | Ответ                                   |
| ---------------- | --------------------------------------- |
| Неверная команда | STATUS_ERROR (0x01), size=0             |
| Размер = 0       | STATUS_ERROR (0x01), size=0             |
| Размер > 256     | STATUS_ERROR (0x01), size=0             |
| Ошибка CSRNG     | STATUS_ERROR (0x01), size=0             |
| Успех            | STATUS_SUCCESS (0x00), size=запрошенный |

### 3. Интеграция с аппаратным CSRNG

Реализована прямая интеграция с аппаратным CSRNG модулем OpenTitan:

```c
static status_t csrng_get_random(uint8_t *buf, size_t len) {
  if (len == 0) {
    return OK_STATUS();
  }

  // Вычисление необходимого количества 32-битных слов
  size_t num_words = (len + 3) / 4;

  // Запуск генерации через DIF (Device Interface Function)
  TRY(dif_csrng_generate_start(&csrng, num_words));

  uint32_t word;
  size_t written = 0;

  // Чтение сгенерированных слов
  for (size_t i = 0; i < num_words && written < len; ++i) {
    TRY(dif_csrng_generate_read(&csrng, &word, 1));

    // Побайтовое копирование в выходной буфер
    for (size_t b = 0; b < 4 && written < len; ++b) {
      buf[written++] = (word >> (8 * b)) & 0xff;
    }
  }

  return OK_STATUS();
}
```

Особенности реализации:
- Прямое обращение к hardware через DIF layer
- Корректная обработка не выровненных размеров
- Использование статусных кодов для error handling
- Минимальные overhead на побайтовое копирование

### 4. Сборка сервера

Команда для сборки сервера:

```bash
cd /root/opentitan
./bazelisk.sh build //sw/device/tests:fapi_getrandom_server_sim_verilator
```

Результат успешной сборки:

```bash
INFO: Analyzed target //sw/device/tests:fapi_getrandom_server_sim_verilator
      (832 packages loaded, 33780 targets configured).
INFO: Found 1 target...
Target //sw/device/tests:fapi_getrandom_server_sim_verilator up-to-date:
  bazel-bin/sw/device/tests/fapi_getrandom_server_sim_verilator.bash
INFO: Elapsed time: 37.527s, Critical Path: 2.18s
INFO: Build completed successfully, 7 total actions
```
