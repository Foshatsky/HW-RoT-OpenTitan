# Инструкция по сборке FreeRTOS

### 1. Установка зависимостей

Установка системных зависимостей:

```bash
sudo apt-get update
sudo apt-get install -y $(cat apt-requirements.txt)
```

Установка Python зависимостей:

```bash
pip3 install -r python-requirements.txt
```

**Примечание:** файлы `apt-requirements.txt` и `python-requirements.txt` находятся в репозитории базового проекта OpenTitan.

### 2. Изучение структуры FreeRTOS

Поиск файлов FreeRTOS в проекте осуществляется при помощи следующей команды:

```bash
find . -type f -name "*freertos*" -o -name "*FreeRTOS*" | head -20
```

Результат вывода команды:

```bash
./third_party/freertos/BUILD.freertos.bazel
./sw/vendor/freertos_freertos_kernel/include/FreeRTOS.h
./sw/vendor/freertos_freertos_kernel/GitHub-FreeRTOS-Kernel-Home.url
./sw/vendor/freertos_freertos_kernel.lock.hjson
./sw/vendor/freertos_freertos_kernel.vendor.hjson
./sw/device/lib/testing/test_framework/freertos_hooks.c
./sw/device/lib/testing/test_framework/FreeRTOSConfig.h
./sw/device/lib/testing/test_framework/freertos_port.S
./sw/device/lib/testing/test_framework/freertos_port.c
./sw/device/tests/freertos_tpm_vault.c
./sw/device/tests/freertos_test.c
./sw/device/freertos/FreeRTOSConfig.h
./sw/device/freertos/FreeRTOS-Kernel
./sw/device/freertos/FreeRTOS-Kernel/include/newlib-freertos.h
./sw/device/freertos/FreeRTOS-Kernel/include/picolibc-freertos.h
./sw/device/freertos/FreeRTOS-Kernel/include/FreeRTOS.h
./sw/device/freertos/FreeRTOS-Kernel/examples/template_configuration/FreeRTOSConfig.h
./sw/device/freertos/FreeRTOS-Kernel/examples/coverity/FreeRTOSConfig.h
./sw/device/freertos/FreeRTOS-Kernel/portable/IAR/RISC-V/chip_specific_extensions/RV32I_CLINT_no_extensions/freertos_risc_v_chip_specific_extensions.h
./sw/device/freertos/FreeRTOS-Kernel/portable/WizC/PIC18/addFreeRTOS.h
```

Просмотр доступных FreeRTOS тестов осуществляется при помощи следующей команды:

```bash
ls sw/device/tests/ | grep freertos
```

Результат вывода команды:

```bash
freertos_test.c
freertos_tpm_vault.c
```

Проверка конфигурации FreeRTOS осуществляется при помощи следующей команды:

```bash
cat sw/device/lib/testing/test_framework/FreeRTOSConfig.h
```

Результат вывода команды:

```
// Copyright lowRISC contributors (OpenTitan project).
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#ifndef OPENTITAN_SW_DEVICE_LIB_TESTING_TEST_FRAMEWORK_FREERTOSCONFIG_H_
#define OPENTITAN_SW_DEVICE_LIB_TESTING_TEST_FRAMEWORK_FREERTOSCONFIG_H_

// These macros configure FreeRTOS. A description of each macro can be found
// here: https://www.freertos.org/a00110.html

// NOTE: the macro names below do NOT, and cannot, conform to the style
// guide, since they are specific to FreeRTOS.

// Debugging
#define configGENERATE_RUN_TIME_STATS 0
#define configUSE_APPLICATION_TASK_TAG 0
#define configUSE_TRACE_FACILITY 0

// Hooks
#define configUSE_IDLE_HOOK 0
#define configUSE_MALLOC_FAILED_HOOK 1
#define configUSE_TICK_HOOK 0

// Memory
#define configAPPLICATION_ALLOCATED_HEAP 1
#define configCHECK_FOR_STACK_OVERFLOW 1
#define configMINIMAL_STACK_SIZE 256  // in words
#define configSTACK_DEPTH_TYPE uint16_t
#define configTOTAL_HEAP_SIZE ((size_t)0x8000u)
#define configUSE_MALLOC_FAILED_HOOK 1

// Other
#define configENABLE_BACKWARD_COMPATIBILITY 0
#define configMAX_TASK_NAME_LEN 16
#define configQUEUE_REGISTRY_SIZE 0
#define configUSE_TASK_NOTIFICATIONS 0

// Scheduler
#define configIDLE_SHOULD_YIELD 0
#define configMAX_PRIORITIES 5
#define configTICK_RATE_HZ ((TickType_t)10)  // 100ms tick rate
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 1
#define configUSE_PREEMPTION 0
#define configUSE_TIME_SLICING 0
#define configUSE_16_BIT_TICKS 0

// Software timers.
#define configUSE_TIMERS 0

// Synchronization
#define configUSE_COUNTING_SEMAPHORES 0
#define configUSE_MUTEXES 0
#define configUSE_RECURSIVE_MUTEXES 0

// FreeRTOS API functions to include in build image.
#define INCLUDE_vTaskPrioritySet 1
#define INCLUDE_uxTaskPriorityGet 1
#define INCLUDE_vTaskDelete 1
#define INCLUDE_vTaskSuspend 1
#define INCLUDE_eTaskGetState 1
#define INCLUDE_xTaskAbortDelay 1
#define INCLUDE_xTaskGetHandle 1

#endif  // OPENTITAN_SW_DEVICE_LIB_TESTING_TEST_FRAMEWORK_FREERTOSCONFIG_H_
```

### 3. Поиск доступных тестов с FreeRTOS

Поиск всех тестов с FreeRTOS:

```bash
./bazelisk.sh query 'filter("freertos", //sw/device/tests/...)'
```

Результат вывода команды:

```bash
$TEST_TMPDIR defined: output root default is '/home/bazel_cache' and max_idle_secs default is '15'.
Starting local Bazel server (8.0.1) and connecting to it...
//sw/device/tests:freertos_test
//sw/device/tests:freertos_test_sim_verilator
//sw/device/tests:freertos_tpm_vault
//sw/device/tests:freertos_tpm_vault_fpga_cw310_rom_with_fake_keys
//sw/device/tests:freertos_tpm_vault_fpga_cw310_sival_rom_ext
//sw/device/tests:freertos_tpm_vault_silicon_owner_sival_rom_ext
//sw/device/tests:freertos_tpm_vault_sim_dv
//sw/device/tests:freertos_tpm_vault_sim_qemu_rom_with_fake_keys
//sw/device/tests:freertos_tpm_vault_sim_verilator
```

Поиск тестов с конкурентностью (на основе FreeRTOS):

```bash
./bazelisk.sh query 'filter("example_concurrency", //sw/device/tests/...)'
```

Результат вывода команды:

```bash
$TEST_TMPDIR defined: output root default is '/home/bazel_cache' and max_idle_secs default is '15'.
Starting local Bazel server (8.0.1) and connecting to it...
//sw/device/tests:example_concurrency_test
//sw/device/tests:example_concurrency_test_fpga_cw310_rom_with_fake_keys
//sw/device/tests:example_concurrency_test_fpga_cw310_sival_rom_ext
//sw/device/tests:example_concurrency_test_silicon_owner_sival_rom_ext
//sw/device/tests:example_concurrency_test_sim_dv
//sw/device/tests:example_concurrency_test_sim_qemu_rom_with_fake_keys
//sw/device/tests:example_concurrency_test_sim_verilator
```

### 4. Сборка тестового примера

Тестовый пример:

```c
#include "sw/device/lib/runtime/log.h"
#include "sw/device/lib/testing/test_framework/check.h"
#include "sw/device/lib/testing/test_framework/ottf_macros.h"
#include "sw/device/lib/testing/test_framework/ottf_main.h"

OTTF_DEFINE_TEST_CONFIG(.enable_concurrency = true);
/* Shared state */
static volatile uint32_t shared_counter = 0;
static void task_1(void *task_parameters) {
  LOG_INFO("task_1 start");

  shared_counter += 1;
  ottf_task_yield();

  LOG_INFO("task_1 after first yield");
  shared_counter += 10;
  ottf_task_yield();

  LOG_INFO("task_1 done");
  OTTF_TASK_DELETE_SELF_OR_DIE;
}
static void task_2(void *task_parameters) {
  LOG_INFO("task_2 start");

  shared_counter += 100;
  ottf_task_yield();

  LOG_INFO("task_2 done");
  OTTF_TASK_DELETE_SELF_OR_DIE;
}
static void task_3(void *task_parameters) {
  LOG_INFO("task_3 start");

  shared_counter += 1000;

  LOG_INFO("task_3 done");
  OTTF_TASK_DELETE_SELF_OR_DIE;
}
bool test_main(void) {
  LOG_INFO("test_main start");

  CHECK(ottf_task_create(task_1, "task_1",
                         kOttfFreeRtosMinStackSize, 1));
  CHECK(ottf_task_create(task_2, "task_2",
                         kOttfFreeRtosMinStackSize, 1));
  CHECK(ottf_task_create(task_3, "task_3",
                         kOttfFreeRtosMinStackSize, 1));

  LOG_INFO("test_main yielding");
  ottf_task_yield();

  LOG_INFO("All tasks done, shared_counter = %d", shared_counter);

  // Ожидаем:
  // task_1: +1 +10
  // task_2: +100
  // task_3: +1000
  CHECK(shared_counter == 1111,
        "Unexpected shared_counter value");

  return true;
}
```

Сборка теста с поддержкой конкурентности осуществляется при помощи следующей команды:

```bash
root@den:~/opentitan$ ./bazelisk.sh build //sw/device/tests:example_concurrency_test_sim_verilator

INFO: Analyzed target //sw/device/tests:example_concurrency_test_sim_verilator
INFO: Found 1 target...
Target //sw/device/tests:example_concurrency_test_sim_verilator up-to-date:
  bazel-bin/sw/device/tests/example_concurrency_test_sim_verilator
INFO: Elapsed time: 125.432s, Critical Path: 45.23s
INFO: Build completed successfully, 1234 total actions
```

### 5. Запуск теста в симуляторе

Запуск теста конкурентности с выводом логов осуществляется при помощи следующей команды:

```bash
./bazelisk.sh run //sw/device/tests:example_concurrency_test_sim_verilator
```

Результат работы команды:

```bash
�I00001 test_rom.c:190] kChipInfo: scm_revision=54697461
I00002 test_rom.c:267] Test ROM complete, jumping to flash (addr: 20000480)!
I00001 ottf_main.c:168] Running sw/device/tests/example_concurrency_test.c
I00002 example_concurrency_test.c:70] test_main start
I00003 example_concurrency_test.c:79] test_main yielding
I00004 example_concurrency_test.c:40] task_1 start
I00005 example_concurrency_test.c:53] task_2 start
I00006 example_concurrency_test.c:62] task_3 start
I00007 example_concurrency_test.c:66] task_3 done
I00008 example_concurrency_test.c:45] task_1 after first yield
I00009 example_concurrency_test.c:58] task_2 done
I00010 example_concurrency_test.c:49] task_1 done
I00011 example_concurrency_test.c:82] All tasks done, shared_counter = 1111
I00012 ottf_main.c:110] Finished sw/device/tests/example_concurrency_test.c
I00013 status.c:37] PASS!
[2026-01-10T20:30:07Z INFO  opentitantool::command::console] ExitSuccess("PASS!\r\n")
[2026-01-10T20:30:07Z INFO  opentitantool] Command result: success.
[2026-01-10T20:30:07Z INFO  opentitantool] Command result: success.
```

### 6. Проверка результатов

Проверка UART вывода:

```bash
root@den:~/opentitan# ls -la uart0.log
-rw-r--r-- 1 root root 0 ноя 21 21:53 uart0.log
```

Просмотр артефактов сборки:

```bash
root@den:~/opentitan# ls -la bazel-bin/sw/device/tests/
итого 7528
drwxr-xr-x 11 root root   4096 янв 10 14:46 .
drwxr-xr-x  5 root root   4096 дек 18 02:49 ..
-r-xr-xr-x  1 root root  37149 дек 21 16:01 example_concurrency_test_sim_verilator.64.vmem
-r-xr-xr-x  1 root root   1089 дек 18 02:49 example_concurrency_test_sim_verilator.bash
-r-xr-xr-x  1 root root    423 дек 20 01:20 example_concurrency_test_sim_verilator.bash.repo_mapping
drwxr-xr-x  3 root root   4096 дек 18 02:49 example_concurrency_test_sim_verilator.bash.runfiles
-r-xr-xr-x  1 root root   4335 дек 18 02:49 example_concurrency_test_sim_verilator.bash.runfiles_manifest
-r-xr-xr-x  1 root root  15224 дек 21 16:01 example_concurrency_test_sim_verilator.bin
-r-xr-xr-x  1 root root 530086 дек 21 16:01 example_concurrency_test_sim_verilator.dis
-r-xr-xr-x  1 root root 260440 дек 21 16:01 example_concurrency_test_sim_verilator.elf
-r-xr-xr-x  1 root root 196690 дек 21 16:01 example_concurrency_test_sim_verilator.map
-r-xr-xr-x  1 root root  41049 янв 10 13:11 fapi_csrng_unit_test_sim_verilator.64.vmem
-r-xr-xr-x  1 root root   1081 янв 10 13:11 fapi_csrng_unit_test_sim_verilator.bash
-r-xr-xr-x  1 root root    423 янв 10 13:11 fapi_csrng_unit_test_sim_verilator.bash.repo_mapping
drwxr-xr-x  3 root root   4096 янв 10 13:11 fapi_csrng_unit_test_sim_verilator.bash.runfiles
-r-xr-xr-x  1 root root   4283 янв 10 13:11 fapi_csrng_unit_test_sim_verilator.bash.runfiles_manifest
-r-xr-xr-x  1 root root  16824 янв 10 13:11 fapi_csrng_unit_test_sim_verilator.bin
-r-xr-xr-x  1 root root 547239 янв 10 13:11 fapi_csrng_unit_test_sim_verilator.dis
-r-xr-xr-x  1 root root 269016 янв 10 13:11 fapi_csrng_unit_test_sim_verilator.elf
-r-xr-xr-x  1 root root 193498 янв 10 13:11 fapi_csrng_unit_test_sim_verilator.map
-r-xr-xr-x  1 root root  33717 янв 10 14:26 fapi_getrandom_demo_sim_verilator.64.vmem
-r-xr-xr-x  1 root root   1079 янв 10 14:26 fapi_getrandom_demo_sim_verilator.bash
-r-xr-xr-x  1 root root    423 янв 10 14:26 fapi_getrandom_demo_sim_verilator.bash.repo_mapping
drwxr-xr-x  3 root root   4096 янв 10 14:26 fapi_getrandom_demo_sim_verilator.bash.runfiles
-r-xr-xr-x  1 root root   4270 янв 10 14:26 fapi_getrandom_demo_sim_verilator.bash.runfiles_manifest
-r-xr-xr-x  1 root root  13816 янв 10 14:26 fapi_getrandom_demo_sim_verilator.bin
-r-xr-xr-x  1 root root 470336 янв 10 14:26 fapi_getrandom_demo_sim_verilator.dis
-r-xr-xr-x  1 root root 248360 янв 10 14:26 fapi_getrandom_demo_sim_verilator.elf
-r-xr-xr-x  1 root root 166618 янв 10 14:26 fapi_getrandom_demo_sim_verilator.map
-r-xr-xr-x  1 root root  43935 янв 10 14:46 fapi_getrandom_server_demo_sim_verilator.64.vmem
-r-xr-xr-x  1 root root   1093 янв 10 14:46 fapi_getrandom_server_demo_sim_verilator.bash
-r-xr-xr-x  1 root root    423 янв 10 14:46 fapi_getrandom_server_demo_sim_verilator.bash.repo_mapping
drwxr-xr-x  3 root root   4096 янв 10 14:46 fapi_getrandom_server_demo_sim_verilator.bash.runfiles
-r-xr-xr-x  1 root root   4361 янв 10 14:46 fapi_getrandom_server_demo_sim_verilator.bash.runfiles_manifest
-r-xr-xr-x  1 root root  18008 янв 10 14:46 fapi_getrandom_server_demo_sim_verilator.bin
-r-xr-xr-x  1 root root 584122 янв 10 14:46 fapi_getrandom_server_demo_sim_verilator.dis
-r-xr-xr-x  1 root root 283980 янв 10 14:46 fapi_getrandom_server_demo_sim_verilator.elf
-r-xr-xr-x  1 root root 217540 янв 10 14:46 fapi_getrandom_server_demo_sim_verilator.map
-r-xr-xr-x  1 root root  42080 янв 10 13:00 fapi_getrandom_server_sim_verilator.64.vmem
-r-xr-xr-x  1 root root   1083 янв  9 22:53 fapi_getrandom_server_sim_verilator.bash
-r-xr-xr-x  1 root root    423 янв 10 13:00 fapi_getrandom_server_sim_verilator.bash.repo_mapping
drwxr-xr-x  3 root root   4096 янв  9 22:53 fapi_getrandom_server_sim_verilator.bash.runfiles
-r-xr-xr-x  1 root root   4296 янв  9 22:53 fapi_getrandom_server_sim_verilator.bash.runfiles_manifest
-r-xr-xr-x  1 root root  17244 янв 10 13:00 fapi_getrandom_server_sim_verilator.bin
-r-xr-xr-x  1 root root 600243 янв 10 13:00 fapi_getrandom_server_sim_verilator.dis
-r-xr-xr-x  1 root root 291116 янв 10 13:00 fapi_getrandom_server_sim_verilator.elf

```

Проверка артефактов в `bazel-bin:*`:

```bash
root@den:~/opentitan# ls -lh bazel-bin/sw/device/tests/*.elf
-r-xr-xr-x 1 root root 255K дек 21 16:01 bazel-bin/sw/device/tests/example_concurrency_test_sim_verilator.elf
-r-xr-xr-x 1 root root 263K янв 10 13:11 bazel-bin/sw/device/tests/fapi_csrng_unit_test_sim_verilator.elf
-r-xr-xr-x 1 root root 243K янв 10 14:26 bazel-bin/sw/device/tests/fapi_getrandom_demo_sim_verilator.elf
-r-xr-xr-x 1 root root 278K янв 10 14:46 bazel-bin/sw/device/tests/fapi_getrandom_server_demo_sim_verilator.elf
-r-xr-xr-x 1 root root 285K янв 10 13:00 bazel-bin/sw/device/tests/fapi_getrandom_server_sim_verilator.elf
-r-xr-xr-x 1 root root 277K янв 10 13:25 bazel-bin/sw/device/tests/fapi_getrandom_server_test_sim_verilator.elf
-r-xr-xr-x 1 root root 254K янв  9 19:39 bazel-bin/sw/device/tests/freertos_tpm_vault_test_sim_verilator.elf
```

Проект OpenTitan был успешно скомпилирован с поддержкой FreeRTOS для платформы RISC-V. Реализованы базовые тесты, демонстрирующие работу RTOS с аппаратными криптографическими модулями. Система готова для разработки более сложных многозадачных приложений на платформе OpenTitan.