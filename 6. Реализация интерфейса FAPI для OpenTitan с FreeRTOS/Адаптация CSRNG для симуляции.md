# Адаптация CSRNG для симуляции

### 1. Обнаруженная особенность

При первоначальной реализации использовался стандартный подход с hardware entropy source:

```c
// Исходный код пытался использовать entropy_src
TRY(dif_entropy_src_init(...));
TRY(dif_entropy_src_configure(...));
TRY(dif_csrng_instantiate(&csrng, kDifCsrngEntropySrcToggleEnable, NULL));
```

В логах наблюдалось:

```bash
E00004 fapi_getrandom_server.c:113] CSRNG init failed!
```

Данная ошибка возникла, так как hardware entropy source требует физические источники случайности (ring oscillator noise, thermal noise), которые недоступны в Verilator симуляции. Это ожидаемое поведение для RTL симуляторов.

### 2. Найденное решение

При исследовании кодовой базы OpenTitan был обнаружен стандартный подход для тестирования CSRNG в симуляции, используемый в официальных тестах (например, `csrng_kat_test.c`).

Поэтому в качестве решения особенности, описанной в первом пункте, было принято использовать детерминированный seed из NIST Known-Answer-Tests (KAT).

```c
static status_t csrng_init(void) {
  LOG_INFO("Initializing CSRNG for simulation...");

  // Инициализация CSRNG DIF
  TRY(dif_csrng_init(
      mmio_region_from_addr(TOP_EARLGREY_CSRNG_BASE_ADDR),
      &csrng));

  // Конфигурация CSRNG
  TRY(dif_csrng_configure(&csrng));
  LOG_INFO("CSRNG configured");

  // NIST CTR_DRBG Known-Answer-Test seed material
  // Источник: NIST CAVP - CTR_DRBG AES-256 no DF
  dif_csrng_seed_material_t seed = {
      .seed_material = {
          0x73bec010, 0x9262474c, 0x16a30f76, 0x531b51de,
          0x2ee494e5, 0xdfec9db3, 0xcb7a879d, 0x5600419c,
          0xca79b0b0, 0xdda33b5c, 0xa468649e, 0xdf5d73fa
      },
      .seed_material_len = 12,
  };

  LOG_INFO("Instantiating CSRNG with test seed...");

  // Инстанциация с отключенным hardware entropy source
  TRY(dif_csrng_instantiate(&csrng,
      kDifCsrngEntropySrcToggleDisable, &seed));

  LOG_INFO("CSRNG instantiated successfully");

  return OK_STATUS();
}
```

### 3. Демонстрация результата

После применения решения CSRNG инициализируется успешно:

```bash
I00003 fapi_getrandom_server.c:52] Initializing CSRNG for simulation...
I00004 fapi_getrandom_server.c:60] CSRNG configured
I00005 fapi_getrandom_server.c:73] Instantiating CSRNG with test seed...
I00006 fapi_getrandom_server.c:76] CSRNG instantiated successfully
I00007 fapi_getrandom_server.c:113] CSRNG initialization: OK
```

**Примечание:** Для production deployment на реальном hardware будет использоваться `kDifCsrngEntropySrcToggleEnable` с физическим entropy source. Детерминированный seed используется исключительно для тестирования.