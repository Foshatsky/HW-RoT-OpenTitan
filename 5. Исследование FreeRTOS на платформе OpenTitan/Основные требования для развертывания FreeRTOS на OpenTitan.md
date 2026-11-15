# Основные требования для развертывания FreeRTOS на OpenTitan

### 1. Версии компонентов

| Компонент        | Версия                                        | Комментарий                                                                              |
| ---------------- | --------------------------------------------- | ---------------------------------------------------------------------------------------- |
| Проект OpenTitan | `Earlgrey_silver_release_v5-9599-g6a3c2e988e` | *Сборка и запуск тестов FreeRTOS выполняются через Bazel*                                |
| FreeRTOS         | `v11.2.0`                                     | *Ядро присутствует в дереве исходников OpenTitan: `sw/vendor/freertos_freertos_kernel/`* |

### 2. Архитектура стенда

- Хост (Debian 12): хранит исходники `~/opentitan`
- Сборка/запуск выполняются через `bazelisk.sh`
- Симуляция выполняется на Verilator‑таргетах (`*_sim_verilator`), результаты доступны в:
  - `bazel-bin/` - артефакты сборки (`elf/bin/vmem/dis`)
  - `bazel-testlogs/` - логи тестов (при запуске через `bazel test`)
  - `uart0.log` - UART‑вывод
 
### 3. Cтруктура проекта

```bash
opentitan/
├── sw/
│   ├── device/
│   │   ├── lib/
│   │   │   └── testing/
│   │   │       └── test_framework/
│   │   │           ├── FreeRTOSConfig.h          # Конфигурация FreeRTOS
│   │   │           ├── freertos_port.c            # RISC-V порт (C)
│   │   │           ├── freertos_port.S            # RISC-V порт (Assembly)
│   │   │           └── freertos_hooks.c           # Хуки (callbacks)
│   │   │
│   │   ├── tests/
│   │   │   ├── freertos_test.c                    # Базовый тест FreeRTOS
│   │   │   ├── freertos_tpm_vault.c               # Тест CSRNG с FreeRTOS
│   │   │   ├── example_concurrency_test.c         # Тест конкурентности
│   │   │   └── BUILD.bazel                        # Bazel правила для тестов
│   │   │
│   │   ├── examples/
│   │   │   └── freertos_minimal/                  # Минимальный пример (созданный)
│   │   │       ├── main.c
│   │   │       └── BUILD.bazel
│   │   │
│   │   └── freertos/
│   │       ├── FreeRTOSConfig.h                   # Дополнительная конфигурация
│   │       └── FreeRTOS-Kernel/                   # Клонированное ядро (optional)
│   │
│   └── vendor/
│       └── freertos_freertos_kernel/              # Vendored FreeRTOS
│           ├── include/
│           │   ├── FreeRTOS.h
│           │   ├── task.h
│           │   ├── queue.h
│           │   ├── semphr.h
│           │   └── ...
│           ├── portable/
│           │   └── GCC/
│           │       └── RISC-V/
│           │           ├── port.c
│           │           ├── portASM.S
│           │           └── portmacro.h
│           └── [kernel source files]
│
├── third_party/
│   └── freertos/
│       ├── BUILD                                  # Главный BUILD файл
│       ├── BUILD.freertos.bazel                   # Конфигурация для FreeRTOS
│       ├── patches/                               # Патчи для OpenTitan
│       └── portable/
│           └── GCC/
│               └── RISC-V/
│                   └── portmacro.h
│
├── bazel-bin/                                     # Артефакты сборки (симлинк)
│   └── sw/device/tests/
│       ├── *.elf                                  # Исполняемые ELF файлы
│       ├── *.bin                                  # Бинарные образы
│       ├── *.vmem                                 # Файлы памяти для Verilator
│       └── *.dis                                  # Дизассемблированный код
│
├── bazel-testlogs/                                # Логи тестов (симлинк)
│   └── sw/device/tests/
│       └── example_concurrency_test_sim_verilator/
│           └── test.log
│
├── .bazelrc                                       # Конфигурация Bazel
├── .bazelversion                                  # Версия Bazel
├── MODULE.bazel                                   # Bazel модули
├── bazelisk.sh                                    # Обертка для Bazel
└── uart0.log                                      # UART вывод симулятора
```

### 4. Система сборки

Проект использует систему сборки Bazel для управления зависимостями FreeRTOS:

```python
# third_party/freertos/BUILD.freertos.bazel

cc_library(
    name = "portmacro",
    hdrs = ["portable/GCC/RISC-V/portmacro.h"],
    target_compatible_with = ["@platforms//cpu:riscv32"],
)

cc_library(
    name = "hdrs",
    hdrs = ["include/FreeRTOS.h", "include/task.h", ...],
    deps = [":portmacro"],
)
```

Архитектурная совместимость:
- **Целевая платформа**: RISC-V 32-бит
- **Компилятор**: GCC для RISC-V
- **Порт**: `portable/GCC/RISC-V`




