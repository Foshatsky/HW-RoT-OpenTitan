# Zephyr на OpenTitan EarlGrey в Docker

## Назначение

Развёртывание среды для сборки и запуска `Zephyr OS` на `OpenTitan EarlGrey`
в `Verilator` через Docker.

## Подтверждённый результат

Проверено:

- Docker-образ собирается;
- `samples/hello_world` из Zephyr для `opentitan_earlgrey` собирается;
- Verilator-симуляция запускается;
- Zephyr загружается;
- smoke test проходит.

UART-вывод smoke test:

```text
I00002 test_rom.c:267] Test ROM complete, jumping to flash (addr: 20000404)!
*** Booting Zephyr OS build v3.7.0 ***
Hello World! opentitan_earlgrey/opentitan
```

## Состав среды

Зафиксированные версии:

- Ubuntu 22.04
- Verilator `4.210`
- Zephyr SDK `0.16.5-1`
- Zephyr `v3.7.0`
- OpenTitan `fee8a58ebe`
- RISC-V toolchain `20220210-1`
- Bazelisk `v1.24.1`

В инфраструктурном образе должны быть подготовлены:

- системные зависимости для Zephyr и OpenTitan;
- `west`;
- Zephyr SDK;
- исходники Zephyr;
- исходники OpenTitan;
- RISC-V toolchain для OpenTitan;
- `test_rom_sim_verilator`;
- `img_rma`;
- `Vchip_sim_tb`;
- патчи совместимости;
- launcher-скрипты.

## Модель использования

Инфраструктурный образ собирается один раз.

Дальнейшая работа:

1. исходники приложения редактируются на хосте;
2. сборка выполняется внутри контейнера;
3. ELF сохраняется вне контейнера;
4. симуляция запускается внутри контейнера;
5. stdout и UART логи сохраняются вне контейнера.

Изменение файлов приложения не требует пересборки Docker-образа.

## Структура проекта

Корень проекта (контекст сборки образа) должен иметь такой вид:

```text
.
├── Dockerfile
├── scripts/                  # host-side: запускаются на хосте
│   ├── common.sh
│   ├── build-image.sh
│   ├── build-app.sh
│   ├── smoke-test.sh
│   ├── run-app.sh
│   └── shell.sh
├── container/
│   └── scripts/              # копируются в образ (COPY container/scripts/)
│       ├── fix_rules_python.sh
│       ├── patch_dtsi.py
│       ├── patch_kconfig.py
│       ├── run_smoke.sh
│       └── run_program.sh
└── <app>/                    # каталог приложения (см. структуру ниже)
```

Каталоги `build/` и `logs/` создаются автоматически при первом запуске.
Расположение фиксировано: `build-image.sh` использует корень как контекст
сборки, а Dockerfile копирует `container/scripts/` именно из корня.

## Минимальная структура приложения

```text
myapp/
├── CMakeLists.txt
├── prj.conf
└── src/
    └── main.c
```

`CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(myapp)
target_sources(app PRIVATE src/main.c)
```

`prj.conf`:

```text
CONFIG_UART_CONSOLE=y
CONFIG_PRINTK=y
```

`src/main.c`:

```c
#include <zephyr/kernel.h>

int main(void)
{
    printk("Hello from myapp!\n");
    return 0;
}
```

## Команды

### Сборка образа

```bash
sudo ./scripts/build-image.sh
```

Результат:

```text
Docker image: ot-zephyr-lab
```

#### Требования к памяти (возможный OOM)

Самый тяжёлый шаг сборки — компиляция Verilator-модели чипа
(`//hw:verilator` → `Vchip_sim_tb`). Verilator генерирует огромную C++-модель
всего EarlGrey, и её компиляция (g++) потребляет много гигабайт RAM. При
нехватке памяти ядро убивает компилятор по OOM, и сборка падает в самом конце:

```text
Killed
make: *** [Makefile:15: Vchip_sim_tb.mk] Error 137
ERROR: Build did NOT complete successfully
```

`Error 137` (= `SIGKILL`, 128 + 9) и строка `Killed` — это и есть OOM, а не
ошибка Dockerfile. Что помогает:

- выделить под сборку **≥ 16 ГБ** RAM (в Docker Desktop — поднять лимит памяти;
  на нативном Linux — добавить swap как страховку);
- снизить параллелизм, чтобы одновременно работало меньше компиляторов:
  `--jobs=1 --local_cpu_resources=1` для `bazelisk build`;
- если waveform не нужен — собирать без трассировки (`--trace*`-флаги Verilator
  сильно увеличивают потребление памяти и время).

### Smoke test

```bash
sudo ./scripts/smoke-test.sh
```

Сценарий:

1. сборка `samples/hello_world`;
2. запуск Verilator;
3. ожидание строки `Hello World` в UART-логе;
4. завершение симуляции.

При успехе скрипт завершается с кодом `0` и печатает содержимое UART-лога;
при неуспехе — последние строки sim-лога и код `1`. Ожидаемый UART-вывод:

```text
*** Booting Zephyr OS build v3.7.0 ***
Hello World! opentitan_earlgrey/opentitan
```

### Сборка приложения

```bash
sudo ./scripts/build-app.sh <app-dir> hello_demo
```

`<app-dir>` — путь к каталогу приложения **относительно корня проекта**
(например, `myapp` или `apps/myapp`); скрипт конкатенирует его с корнем
(`${OTZL_ROOT}/<app-dir>`). Абсолютный путь или путь вне корня проекта
приведёт к ошибке `app directory not found`.

Результат:

```text
build/hello_demo/zephyr/zephyr.elf
```

### Запуск приложения

```bash
sudo ./scripts/run-app.sh hello_demo
```

### Shell в контейнере

```bash
sudo ./scripts/shell.sh
```

## Ручной режим в контейнере

Готовые скрипты (`build-app.sh`, `run-app.sh`) рассчитаны на типовой сценарий
«собрать ELF → посмотреть UART-вывод». Для специфических приложений —
интерактивных, с собственным протоколом поверх UART, с двусторонним обменом
(хост ↔ устройство), с нестандартными аргументами симулятора — удобнее зайти
в контейнер и управлять `west` и симулятором вручную.

### Вход

```bash
sudo ./scripts/shell.sh
```

Открывается shell от пользователя `dev` в каталоге `/workspace`.

Важно: каждый вызов `shell.sh` — это **новый** контейнер (`docker run --rm`).
Если приложению нужны два процесса одновременно (например, симулятор + клиент,
общающийся с ним), их надо запускать в **одной** сессии shell (симулятор —
фоном через `&`), либо подключаться к уже запущенному контейнеру через
`docker exec` (по его ID из `docker ps` — `shell.sh` имя контейнеру не задаёт).
Разные `shell.sh` друг друга не видят (отдельные `/dev/pts`, PID, файлы в `/tmp`).

### Что где лежит (внутри контейнера)

| Путь | Что это |
| --- | --- |
| `/workspace` | корень проекта с хоста (правки на хосте видны сразу) |
| `/home/dev/zephyrproject/zephyr` | Zephyr, он же `$ZEPHYR_BASE`; внутри `samples/` |
| `/home/dev/opentitan` | исходники OpenTitan |
| `/home/dev/.cache/bazel` | Bazel cache (volume); тут лежат собранные `Vchip_sim_tb`, ROM, OTP |
| `/home/dev/scripts` | container-side скрипты (`run_smoke.sh`, `run_program.sh`, `patch_*`) |
| `uart0.log` | UART-лог; пишется в **текущем каталоге** запуска симулятора (готовые скрипты делают `cd /home/dev`, поэтому там это `/home/dev/uart0.log`) |

Полезные переменные окружения: `$ZEPHYR_BASE`, `$ZEPHYR_SDK_INSTALL_DIR`.

Бинарь симулятора и образы памяти лежат в Bazel cache под хэш-путями — их ищут
через `find`:

```bash
VBIN=$(find /home/dev/opentitan/bazel-bin/hw /home/dev/.cache/bazel \
         -name Vchip_sim_tb -type f 2>/dev/null | head -1)
ROM=$(find /home/dev/.cache/bazel -name 'test_rom_sim_verilator*.scr.vmem' 2>/dev/null | head -1)
OTP=$(find /home/dev/.cache/bazel -name 'img_rma*.vmem' 2>/dev/null | head -1)
```

### Ручная сборка

```bash
cd "$ZEPHYR_BASE"
west build -b opentitan_earlgrey /workspace/<app-dir> \
    --build-dir /workspace/build/<name> \
    --pristine \
    -- -DPython3_EXECUTABLE=/usr/bin/python3.12
```

Флаг `-DPython3_EXECUTABLE=/usr/bin/python3.12` обязателен (сборка
`opentitan_earlgrey` требует Python 3.12). ELF появится в
`/workspace/build/<name>/zephyr/zephyr.elf` — он же виден на хосте в `build/`.

### Ручной запуск симуляции

```bash
ELF=/workspace/build/<name>/zephyr/zephyr.elf
"$VBIN" \
    --verbose-mem-load \
    --meminit=rom0,"$ROM" \
    --meminit=otp,"$OTP" \
    --meminit=flash0,"$ELF"
```

Симулятор инициализирует память из ELF при **каждом** запуске — состояние
выполнения между запусками не сохраняется.

### Интерактивный UART (двусторонний обмен)

Verilator-тестбенч OpenTitan выставляет UART как псевдотерминал и печатает его
путь в свой stdout (строка вида `UART: ... /dev/pts/N`). Чтобы поймать путь,
запускают симулятор с перенаправлением вывода в файл и читают его оттуда:

```bash
"$VBIN" --verbose-mem-load \
    --meminit=rom0,"$ROM" --meminit=otp,"$OTP" --meminit=flash0,"$ELF" \
    > /tmp/sim.log 2>&1 &
SIM_PID=$!
sleep 5
PTY=$(grep -o '/dev/pts/[0-9]*' /tmp/sim.log | head -1)
echo "UART PTY: $PTY"
```

К `$PTY` можно подключаться своим клиентом (например, Python-скрипт через
`os.open`/`os.read`/`os.write`, либо `picocom`/`screen`, если установлены).
Однонаправленный поток UART дополнительно пишется в файл `uart0.log` в текущем
рабочем каталоге (откуда запущен `$VBIN`; готовые скрипты для этого делают
`cd /home/dev`).

Остановить симулятор: `kill "$SIM_PID"`.

## Артефакты

Типичные артефакты после сборки и запуска:

```text
build/<name>/zephyr/zephyr.elf
logs/<name>.sim.log
logs/<name>.uart0.log
```

Для smoke test:

```text
build/smoke_hello_world/zephyr/zephyr.elf
logs/smoke_hello_world.sim.log
logs/smoke_hello_world.uart0.log
```

## Пересборка образа

Пересборка **не требуется** при изменении:

- файлов приложения;
- параметров конкретной сборки;
- повторного запуска уже собранного ELF.

Пересборка **требуется** при изменении:

- версий Zephyr, OpenTitan, Verilator, Zephyr SDK;
- базового Dockerfile;
- встроенных системных зависимостей или патчей.

## Необходимые исправления

### `rules_python`

При сборке OpenTitan возможна ошибка:

```text
AttributeError: 'NoneType' object has no attribute 'replace'
```

Исправление:

- патч `wheel.py` в Bazel cache;
- добавление проверки `metadata.name is None`.

### Device tree

Для `opentitan_earlgrey` требуется исправление описания `RV_TIMER` в device tree.

### Частота таймера

Для Verilator требуется исправление частоты системного таймера в Zephyr.

### UART лог

Симулятор OpenTitan пишет UART-лог во внутренний файл контейнера.

Требуется перенаправление или symlink этого файла во внешний лог.

### Запуск через `sudo`

Если Docker используется через `sudo`, host-side каталоги для build-артефактов
и логов должны создаваться с владельцем исходного пользователя, а не root.

### Владелец каталога сборки приложения (`sudo` / WSL)

Подтверждённая конфигурация: rootful Docker, исходный пользователь с `uid=1000`,
команды через `sudo`.

Внутри контейнера работает пользователь `dev` с фиксированным `uid=1000`
(см. `useradd -u 1000` в Dockerfile), а рабочий каталог монтируется как volume.
Под `sudo` каталог, созданный на хосте, принадлежит root (`uid=0`), и
контейнерный `dev` (`uid=1000`) не может в него писать — сборка падает:

```text
file problem creating directory: /workspace/build/<name>/Kconfig
CMake Error: ... CMakeCache.txt ... System Error: Permission denied
```

`common.sh` уже создаёт верхние каталоги (`build/`, `logs/`)
с владельцем исходного пользователя через `install -d -o ${OTZL_OWNER_UID}`.
Подкаталог конкретной сборки в `build-app.sh` должен создаваться так же, а не
обычным `mkdir` от root.

Иные окружения (rootless Docker, `uid` хоста ≠ `1000`, Docker Desktop, WSL)
имеют собственную модель прав на volume и здесь не рассматриваются — их
особенности разбираются отдельно.

## Текст Dockerfile

```dockerfile
FROM ubuntu:22.04

ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC
ENV LC_ALL=en_US.UTF-8
ENV LANG=en_US.UTF-8

SHELL ["/bin/bash", "-o", "pipefail", "-c"]

RUN apt-get update && apt-get install -y --no-install-recommends \
    autoconf \
    brotli \
    build-essential \
    ca-certificates \
    ccache \
    cmake \
    curl \
    device-tree-compiler \
    dfu-util \
    file \
    g++ \
    gcc \
    git \
    gperf \
    lcov \
    libelf-dev \
    libelf1 \
    libftdi1-2 \
    libftdi1-dev \
    libmagic1 \
    libncursesw5 \
    libpcsclite-dev \
    libssl-dev \
    libtool \
    libudev-dev \
    libusb-1.0-0 \
    libusb-1.0-0-dev \
    locales \
    locales-all \
    lrzsz \
    lsb-release \
    make \
    ninja-build \
    openssl \
    perl \
    pkgconf \
    python-is-python3 \
    python3 \
    python3-dev \
    python3-pip \
    python3-setuptools \
    python3-tk \
    python3-wheel \
    srecord \
    software-properties-common \
    tree \
    wget \
    xmlstarlet \
    xsltproc \
    xxd \
    xz-utils \
    zip \
    zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

RUN python3 -m pip install --upgrade pip "setuptools<66.0.0" wheel

ARG VERILATOR_VERSION=4.210
RUN mkdir -p /tools/verilator \
    && curl -sSfL "https://storage.googleapis.com/verilator-builds/verilator-v${VERILATOR_VERSION}.tar.gz" \
    | tar -C /tools/verilator -xzf -
ENV PATH="/tools/verilator/v${VERILATOR_VERSION}/bin:${PATH}"

ARG BAZELISK_VERSION=v1.24.1
RUN curl -fsSL "https://github.com/bazelbuild/bazelisk/releases/download/${BAZELISK_VERSION}/bazelisk-linux-amd64" \
    -o /usr/local/bin/bazelisk \
    && chmod +x /usr/local/bin/bazelisk \
    && ln -sf /usr/local/bin/bazelisk /usr/local/bin/bazel

RUN groupadd -g 1000 dev \
    && useradd -m -u 1000 -g dev -s /bin/bash dev \
    && mkdir -p /tools /workspace \
    && chown dev:dev /tools /workspace

USER dev
WORKDIR /home/dev
ENV PATH="/home/dev/.local/bin:${PATH}"

RUN python3 -m pip install --user --upgrade pip "setuptools<66.0.0" wheel \
    && python3 -m pip install --user west

ARG ZEPHYR_SDK_VERSION=0.16.5-1
RUN wget -q \
    "https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v${ZEPHYR_SDK_VERSION}/zephyr-sdk-${ZEPHYR_SDK_VERSION}_linux-x86_64.tar.xz" \
    -O /tmp/zephyr-sdk.tar.xz \
    && tar xf /tmp/zephyr-sdk.tar.xz -C /tools/ \
    && rm /tmp/zephyr-sdk.tar.xz \
    && /tools/zephyr-sdk-${ZEPHYR_SDK_VERSION}/setup.sh -t riscv64-zephyr-elf -h
ENV ZEPHYR_SDK_INSTALL_DIR=/tools/zephyr-sdk-${ZEPHYR_SDK_VERSION}

ARG ZEPHYR_VERSION=v3.7.0
RUN git clone --branch ${ZEPHYR_VERSION} --depth 1 \
       https://github.com/zephyrproject-rtos/zephyr /home/dev/zephyrproject/zephyr \
    && cd /home/dev/zephyrproject \
    && west init -l zephyr/ \
    && success=0 \
    && for attempt in 1 2 3; do \
         if west update; then \
           success=1; \
           break; \
         fi; \
         if [ "${attempt}" -lt 3 ]; then \
           echo "west update failed on attempt ${attempt}, retrying..."; \
           sleep 15; \
         fi; \
       done \
    && [ "${success}" -eq 1 ] \
    && west zephyr-export
ENV ZEPHYR_BASE=/home/dev/zephyrproject/zephyr

ARG OPENTITAN_COMMIT=fee8a58ebe
RUN git clone https://github.com/lowRISC/opentitan.git /home/dev/opentitan \
    && cd /home/dev/opentitan \
    && git checkout ${OPENTITAN_COMMIT}

ARG RISCV_TOOLCHAIN_VERSION=20220210-1
RUN python3 -m pip install --user urllib3 \
    && cd /home/dev/opentitan \
    && python3 util/get-toolchain.py -r ${RISCV_TOOLCHAIN_VERSION}

COPY --chown=dev:dev container/scripts/ /home/dev/scripts/
RUN chmod +x /home/dev/scripts/*.sh

RUN cd /home/dev/opentitan \
    && ./bazelisk.sh fetch //sw/device/lib/testing/test_rom:test_rom_sim_verilator 2>&1 || true \
    && /home/dev/scripts/fix_rules_python.sh

RUN cd /home/dev/opentitan \
    && ./bazelisk.sh build //sw/device/lib/testing/test_rom:test_rom_sim_verilator

RUN cd /home/dev/opentitan \
    && ( ./bazelisk.sh build //hw/ip/otp_ctrl/data:img_rma \
         || ./bazelisk.sh build //hw/top_earlgrey/data/otp:img_rma )

RUN cd /home/dev/opentitan \
    && ./bazelisk.sh build //hw:verilator

USER root
RUN curl -fsSL "https://keyserver.ubuntu.com/pks/lookup?op=get&search=0xF23C5A6CF475977595C89F51BA6932366A755776" \
    | gpg --dearmor -o /etc/apt/trusted.gpg.d/deadsnakes-ppa.gpg \
    && echo "deb https://ppa.launchpadcontent.net/deadsnakes/ppa/ubuntu jammy main" \
    > /etc/apt/sources.list.d/deadsnakes.list \
    && apt-get update \
    && apt-get install -y --no-install-recommends python3.12 python3.12-dev python3.12-venv \
    && rm -rf /var/lib/apt/lists/*
USER dev

RUN python3.12 -m ensurepip --upgrade \
    && python3.12 -m pip install --user --ignore-installed "setuptools>=67.0.0" wheel \
    && python3.12 -m pip install --user --no-build-isolation docopt \
    && python3.12 -m pip install --user packaging PyYAML pyelftools jsonschema west

RUN python3 /home/dev/scripts/patch_dtsi.py \
    /home/dev/zephyrproject/zephyr/dts/riscv/lowrisc/opentitan_earlgrey.dtsi

RUN python3 /home/dev/scripts/patch_kconfig.py /home/dev/zephyrproject/zephyr

RUN python3 -m pip install --user \
    -r /home/dev/zephyrproject/zephyr/scripts/requirements-base.txt

WORKDIR /workspace
CMD ["/bin/bash"]
```

## Host-side scripts

### `common.sh`

```bash
#!/bin/bash

set -euo pipefail

OTZL_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OTZL_IMAGE_NAME="${OTZL_IMAGE_NAME:-ot-zephyr-lab}"
OTZL_BAZEL_VOLUME="${OTZL_BAZEL_VOLUME:-ot-zephyr-lab-bazel-cache}"
OTZL_CONTAINER_WORKSPACE="/workspace"
OTZL_LOGS_DIR="${OTZL_ROOT}/logs"
OTZL_BUILD_DIR="${OTZL_ROOT}/build"
OTZL_OWNER_UID="${SUDO_UID:-$(id -u)}"
OTZL_OWNER_GID="${SUDO_GID:-$(id -g)}"

ensure_host_dirs() {
    if [[ "$(id -u)" -eq 0 ]]; then
        install -d -m 0775 -o "${OTZL_OWNER_UID}" -g "${OTZL_OWNER_GID}" \
            "${OTZL_LOGS_DIR}" "${OTZL_BUILD_DIR}"
        chown -R "${OTZL_OWNER_UID}:${OTZL_OWNER_GID}" \
            "${OTZL_LOGS_DIR}" "${OTZL_BUILD_DIR}"
    else
        mkdir -p "${OTZL_LOGS_DIR}" "${OTZL_BUILD_DIR}"
    fi
}

ensure_host_dirs

docker_ready() {
    docker info >/dev/null 2>&1
}

require_docker() {
    if ! command -v docker >/dev/null 2>&1; then
        echo "ERROR: docker not found in PATH." >&2
        exit 1
    fi
    if ! docker_ready; then
        echo "ERROR: Docker daemon is unavailable." >&2
        echo "Start Docker and rerun the command." >&2
        exit 1
    fi
}

ensure_image() {
    require_docker
    if ! docker image inspect "${OTZL_IMAGE_NAME}" >/dev/null 2>&1; then
        echo "ERROR: image ${OTZL_IMAGE_NAME} is missing." >&2
        echo "Run ./scripts/build-image.sh first." >&2
        exit 1
    fi
}

run_in_container() {
    local options=()

    while [[ $# -gt 0 ]]; do
        if [[ "$1" == "--" ]]; then
            shift
            break
        fi
        options+=("$1")
        shift
    done

    docker run --rm "${options[@]}" \
        -v "${OTZL_ROOT}:${OTZL_CONTAINER_WORKSPACE}" \
        -v "${OTZL_BAZEL_VOLUME}:/home/dev/.cache/bazel" \
        -w "${OTZL_CONTAINER_WORKSPACE}" \
        "${OTZL_IMAGE_NAME}" \
        "$@"
}
```

### `build-image.sh`

```bash
#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/common.sh"

require_docker

echo "Building image ${OTZL_IMAGE_NAME}"
echo "Context: ${OTZL_ROOT}"

docker build --progress=plain "$@" -t "${OTZL_IMAGE_NAME}" "${OTZL_ROOT}"
```

### `build-app.sh`

```bash
#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/common.sh"

if [[ $# -lt 1 || $# -gt 2 ]]; then
    echo "Usage: $0 <app-dir> [build-name]" >&2
    exit 1
fi

APP_DIR="$1"
APP_PATH="${OTZL_ROOT}/${APP_DIR}"

if [[ ! -d "${APP_PATH}" ]]; then
    echo "ERROR: app directory not found: ${APP_DIR}" >&2
    exit 1
fi

BUILD_NAME="${2:-$(basename "${APP_DIR}")}"
BUILD_PATH="${OTZL_BUILD_DIR}/${BUILD_NAME}"
# Под sudo каталог должен принадлежать исходному пользователю (uid 1000),
# иначе контейнерный dev (uid 1000) не сможет писать в него.
if [[ "$(id -u)" -eq 0 ]]; then
    install -d -m 0775 -o "${OTZL_OWNER_UID}" -g "${OTZL_OWNER_GID}" "${BUILD_PATH}"
else
    mkdir -p "${BUILD_PATH}"
fi

ensure_image

run_in_container -- \
    /bin/bash -lc "cd /home/dev/zephyrproject/zephyr && \
    west build -b opentitan_earlgrey \
      ${OTZL_CONTAINER_WORKSPACE}/${APP_DIR} \
      --build-dir ${OTZL_CONTAINER_WORKSPACE}/build/${BUILD_NAME} \
      --pristine \
      -- -DPython3_EXECUTABLE=/usr/bin/python3.12"

echo "Build complete: ${BUILD_PATH}/zephyr/zephyr.elf"
```

### `smoke-test.sh`

```bash
#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/common.sh"

BUILD_NAME="smoke_hello_world"

ensure_image
run_in_container -- \
    /bin/bash -lc "cd /home/dev/zephyrproject/zephyr && \
    west build -b opentitan_earlgrey \
      samples/hello_world \
      --build-dir ${OTZL_CONTAINER_WORKSPACE}/build/${BUILD_NAME} \
      --pristine \
      -- -DPython3_EXECUTABLE=/usr/bin/python3.12"

docker run --rm \
    -e UART_LOG="${OTZL_CONTAINER_WORKSPACE}/logs/${BUILD_NAME}.uart0.log" \
    -e SIM_LOG="${OTZL_CONTAINER_WORKSPACE}/logs/${BUILD_NAME}.sim.log" \
    -v "${OTZL_ROOT}:${OTZL_CONTAINER_WORKSPACE}" \
    -v "${OTZL_BAZEL_VOLUME}:/home/dev/.cache/bazel" \
    -w "${OTZL_CONTAINER_WORKSPACE}" \
    "${OTZL_IMAGE_NAME}" \
    /workspace/container/scripts/run_smoke.sh "${OTZL_CONTAINER_WORKSPACE}/build/${BUILD_NAME}/zephyr/zephyr.elf"
```

### `run-app.sh`

```bash
#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/common.sh"

if [[ $# -lt 1 || $# -gt 2 ]]; then
    echo "Usage: $0 <elf-path|build-name> [container-name]" >&2
    exit 1
fi

TARGET="$1"
CONTAINER_NAME="${2:-ot-zephyr-sim}"

if [[ -f "${TARGET}" ]]; then
    ELF_PATH="$(cd "$(dirname "${TARGET}")" && pwd)/$(basename "${TARGET}")"
else
    ELF_PATH="${OTZL_BUILD_DIR}/${TARGET}/zephyr/zephyr.elf"
fi

if [[ ! -f "${ELF_PATH}" ]]; then
    echo "ERROR: ELF not found: ${ELF_PATH}" >&2
    exit 1
fi

LOG_BASENAME="$(basename "$(dirname "$(dirname "${ELF_PATH}")")")"
mkdir -p "${OTZL_LOGS_DIR}"

ensure_image

docker run --rm -it \
    --name "${CONTAINER_NAME}" \
    -e UART_LOG="${OTZL_CONTAINER_WORKSPACE}/logs/${LOG_BASENAME}.uart0.log" \
    -e SIM_LOG="${OTZL_CONTAINER_WORKSPACE}/logs/${LOG_BASENAME}.sim.log" \
    -v "${OTZL_ROOT}:${OTZL_CONTAINER_WORKSPACE}" \
    -v "${OTZL_BAZEL_VOLUME}:/home/dev/.cache/bazel" \
    -w "${OTZL_CONTAINER_WORKSPACE}" \
    "${OTZL_IMAGE_NAME}" \
    /workspace/container/scripts/run_program.sh "${OTZL_CONTAINER_WORKSPACE}${ELF_PATH#${OTZL_ROOT}}"
```

### `shell.sh`

```bash
#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/common.sh"

ensure_image

run_in_container -it -- /bin/bash
```

## Container-side scripts

### `fix_rules_python.sh`

```bash
#!/bin/bash
set -e

BAZEL_CACHE="${HOME}/.cache/bazel"

if [ ! -d "${BAZEL_CACHE}" ]; then
    echo "[fix_rules_python] Bazel cache not found at ${BAZEL_CACHE}, skipping."
    exit 0
fi

WHEEL_PY_FILES=$(find "${BAZEL_CACHE}" \
    -path "*/rules_python/python/pip_install/extract_wheels/lib/wheel.py" \
    2>/dev/null)

if [ -z "${WHEEL_PY_FILES}" ]; then
    echo "[fix_rules_python] wheel.py not found in bazel cache, skipping."
    exit 0
fi

for WHEEL_PY in ${WHEEL_PY_FILES}; do
    echo "[fix_rules_python] Patching: ${WHEEL_PY}"
    if grep -q "metadata.name is None" "${WHEEL_PY}" 2>/dev/null; then
        echo "[fix_rules_python] Already patched, skipping."
        continue
    fi

    python3 - "${WHEEL_PY}" <<'PYEOF'
import sys
path = sys.argv[1]
with open(path, 'r') as f:
    content = f.read()

old_patterns = [
    'name = "{}-{}".format(metadata.name.replace("-", "_"), metadata.version)',
    '            name = "{}-{}".format(metadata.name.replace("-", "_"), metadata.version)',
]

new_code_template = '''{indent}if metadata.name is None:
{indent}    continue
{indent}name = "{{}}-{{}}".format(metadata.name.replace("-", "_"), metadata.version)'''

patched = False
for old in old_patterns:
    if old in content:
        indent = len(old) - len(old.lstrip())
        indent_str = ' ' * indent
        new_code = new_code_template.format(indent=indent_str)
        content = content.replace(old, new_code, 1)
        patched = True
        break

if not patched:
    import re
    pattern = r'(\s+)(name\s*=\s*".*?"\s*\.format\s*\(\s*metadata\.name\.replace\(".*?".*?\))'
    match = re.search(pattern, content)
    if match:
        indent = match.group(1)
        line = match.group(2)
        new_code = f"{indent}if metadata.name is None:\n{indent}    continue\n{indent}{line}"
        content = re.sub(pattern, new_code, content, count=1)
        patched = True

if patched:
    with open(path, 'w') as f:
        f.write(content)
PYEOF
done

echo "[fix_rules_python] Done."
```

### `patch_dtsi.py`

```python
#!/usr/bin/env python3
import sys
import re

def patch(path: str) -> None:
    with open(path, "r") as f:
        content = f.read()

    if "timer@40100000" in content:
        print(f"[patch_dtsi] Already patched: {path}")
        return

    if "timer@40100110" not in content:
        print(f"[patch_dtsi] WARNING: expected pattern 'timer@40100110' not found in {path}")
        return

    content = content.replace("timer@40100110", "timer@40100000")
    content = re.sub(
        r'reg\s*=\s*<0x40100110 0x8 0x40100118 0x8>',
        'reg = <0x40100000 0x4 0x40100110 0x8 0x40100118 0x8>',
        content
    )
    content = re.sub(
        r'reg-names\s*=\s*"mtime",\s*"mtimecmp"',
        'reg-names = "alert_test", "mtime", "mtimecmp"',
        content
    )

    with open(path, "w") as f:
        f.write(content)

    print(f"[patch_dtsi] Patched successfully: {path}")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(1)
    patch(sys.argv[1])
```

### `patch_kconfig.py`

```python
#!/usr/bin/env python3
import sys
import os

def find_kconfig(zephyr_base: str) -> str | None:
    candidates = [
        "soc/lowrisc/opentitan/Kconfig.defconfig",
        "soc/riscv/opentitan/Kconfig.defconfig",
        "soc/riscv32/opentitan/Kconfig.defconfig",
        "arch/riscv/soc/opentitan/Kconfig.defconfig",
    ]
    for candidate in candidates:
        full = os.path.join(zephyr_base, candidate)
        if os.path.exists(full):
            return full
    for root, dirs, files in os.walk(zephyr_base):
        if "Kconfig.defconfig" in files and "opentitan" in root.lower():
            return os.path.join(root, "Kconfig.defconfig")
    return None

def patch(path: str) -> None:
    with open(path, "r") as f:
        content = f.read()

    if "default 5000000" in content:
        print(f"[patch_kconfig] Already patched (5MHz): {path}")
        return

    original = content

    if "default 1000000" in content:
        content = content.replace("default 1000000", "default 5000000")

    if "SYS_CLOCK_TICKS_PER_SEC" not in content:
        content = content.replace(
            "default 5000000",
            "default 5000000\n\nconfig SYS_CLOCK_TICKS_PER_SEC\n\tdefault 1000",
            1,
        )

    if content != original:
        with open(path, "w") as f:
            f.write(content)

if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(1)

    arg = sys.argv[1]
    if os.path.isfile(arg):
        patch(arg)
    elif os.path.isdir(arg):
        found = find_kconfig(arg)
        if found:
            patch(found)
        else:
            sys.exit(1)
    else:
        sys.exit(1)
```

### `run_smoke.sh`

```bash
#!/bin/bash
set -e

OPENTITAN="/home/dev/opentitan"
BAZEL_CACHE="/home/dev/.cache/bazel"
ZEPHYR_ELF="${1:-/home/dev/zephyrproject/zephyr/build/zephyr/zephyr.elf}"
UART_LOG="${UART_LOG:-/home/dev/uart0.log}"
SIM_LOG="${SIM_LOG:-/tmp/sim_stdout.txt}"
UART_DEFAULT_LOG="/home/dev/uart0.log"
TIMEOUT=300

VERILATOR_BIN=$(find "${OPENTITAN}/bazel-bin/hw" -name "Vchip_sim_tb" -type f 2>/dev/null | head -1)
if [ -z "${VERILATOR_BIN}" ]; then
    VERILATOR_BIN=$(find "${BAZEL_CACHE}" -name "Vchip_sim_tb" -type f 2>/dev/null | head -1)
fi
if [ -z "${VERILATOR_BIN}" ]; then echo "ERROR: Vchip_sim_tb not found"; exit 1; fi

ROM_VMEM=$(find "${BAZEL_CACHE}" -name "test_rom_sim_verilator*.scr.vmem" -type f 2>/dev/null | head -1)
if [ -z "${ROM_VMEM}" ]; then echo "ERROR: ROM vmem not found"; exit 1; fi

OTP_VMEM=$(find "${BAZEL_CACHE}" -name "img_rma*.vmem" -type f 2>/dev/null | head -1)
if [ -z "${OTP_VMEM}" ]; then echo "ERROR: OTP vmem not found"; exit 1; fi

if [ ! -f "${ZEPHYR_ELF}" ]; then
    echo "ERROR: Zephyr ELF not found at ${ZEPHYR_ELF}"
    exit 1
fi

cd /home/dev
mkdir -p "$(dirname "${UART_LOG}")" "$(dirname "${SIM_LOG}")"
rm -f "${SIM_LOG}"
if [ "${UART_LOG}" != "${UART_DEFAULT_LOG}" ]; then
    rm -f "${UART_LOG}" "${UART_DEFAULT_LOG}"
    touch "${UART_LOG}"
    ln -s "${UART_LOG}" "${UART_DEFAULT_LOG}"
else
    rm -f "${UART_DEFAULT_LOG}"
fi

"${VERILATOR_BIN}" \
    --verbose-mem-load \
    --meminit=rom0,"${ROM_VMEM}" \
    --meminit=otp,"${OTP_VMEM}" \
    --meminit=flash0,"${ZEPHYR_ELF}" \
    > "${SIM_LOG}" 2>&1 &
SIM_PID=$!

FOUND=0
for i in $(seq 1 ${TIMEOUT}); do
    sleep 1
    if ! kill -0 "${SIM_PID}" 2>/dev/null; then
        break
    fi
    if [ -f "${UART_LOG}" ] && grep -q "Hello World" "${UART_LOG}" 2>/dev/null; then
        FOUND=1
        break
    fi
done

kill "${SIM_PID}" 2>/dev/null || true
wait "${SIM_PID}" 2>/dev/null || true

if [ "${FOUND}" -eq 1 ]; then
    cat "${UART_LOG}"
    exit 0
fi

tail -20 "${SIM_LOG}"
exit 1
```

### `run_program.sh`

```bash
#!/bin/bash
set -e

OPENTITAN="/home/dev/opentitan"
BAZEL_CACHE="/home/dev/.cache/bazel"
UART_LOG="${UART_LOG:-/home/dev/uart0.log}"
SIM_LOG="${SIM_LOG:-/tmp/program_sim_stdout.txt}"
UART_DEFAULT_LOG="/home/dev/uart0.log"

if [ -z "${1}" ]; then
    echo "Usage: $0 <path/to/zephyr.elf>"
    exit 1
fi
APP_ELF="${1}"

VERILATOR_BIN=$(find "${OPENTITAN}/bazel-bin/hw" -name "Vchip_sim_tb" -type f 2>/dev/null | head -1)
if [ -z "${VERILATOR_BIN}" ]; then
    VERILATOR_BIN=$(find "${BAZEL_CACHE}" -name "Vchip_sim_tb" -type f 2>/dev/null | head -1)
fi
if [ -z "${VERILATOR_BIN}" ]; then echo "ERROR: Vchip_sim_tb not found"; exit 1; fi

ROM_VMEM=$(find "${BAZEL_CACHE}" -name "test_rom_sim_verilator*.scr.vmem" -type f 2>/dev/null | head -1)
if [ -z "${ROM_VMEM}" ]; then echo "ERROR: ROM vmem not found"; exit 1; fi

OTP_VMEM=$(find "${BAZEL_CACHE}" -name "img_rma*.vmem" -type f 2>/dev/null | head -1)
if [ -z "${OTP_VMEM}" ]; then echo "ERROR: OTP vmem not found"; exit 1; fi

if [ ! -f "${APP_ELF}" ]; then
    echo "ERROR: ELF not found: ${APP_ELF}"
    exit 1
fi

cd /home/dev
mkdir -p "$(dirname "${UART_LOG}")" "$(dirname "${SIM_LOG}")"
rm -f "${SIM_LOG}"
if [ "${UART_LOG}" != "${UART_DEFAULT_LOG}" ]; then
    rm -f "${UART_LOG}" "${UART_DEFAULT_LOG}"
    touch "${UART_LOG}"
    ln -s "${UART_LOG}" "${UART_DEFAULT_LOG}"
else
    rm -f "${UART_DEFAULT_LOG}"
fi

"${VERILATOR_BIN}" \
    --verbose-mem-load \
    --meminit=rom0,"${ROM_VMEM}" \
    --meminit=otp,"${OTP_VMEM}" \
    --meminit=flash0,"${APP_ELF}" \
    > "${SIM_LOG}" 2>&1 &
SIM_PID=$!

for i in $(seq 1 30); do
    sleep 1
    if ! kill -0 "${SIM_PID}" 2>/dev/null; then
        cat "${SIM_LOG}"
        exit 1
    fi
    if [ -f "${UART_LOG}" ]; then
        break
    fi
done

touch "${UART_LOG}" 2>/dev/null || true
tail -f "${UART_LOG}" &
TAIL_PID=$!

trap "kill ${SIM_PID} 2>/dev/null; exit 0" INT TERM

wait "${SIM_PID}" 2>/dev/null || true
kill "${TAIL_PID}" 2>/dev/null || true
```
