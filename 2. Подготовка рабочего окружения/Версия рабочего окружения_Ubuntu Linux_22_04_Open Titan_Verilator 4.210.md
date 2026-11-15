
## 0. Предварительная проверка и базовая установка на чистом Ubuntu

**Где выполняем:** домашний каталог `~`, без venv.

### 0.1. Версия ОС

```bash
vlad@vlad:~$ lsb_release -a
No LSB modules are available.
Distributor ID: Ubuntu
Description:    Ubuntu 22.04.4 LTS
Release:        22.04
Codename:       jammy
```


### 0.2. Архитектура и ядро

```bash
vlad@vlad:~$ uname -m && uname -r
x86_64
5.15.0-113-generic
```


### 0.3. RAM и диск

```bash
vlad@vlad:~$ free -h && df -h /
               total        used        free      shared  buff/cache   available
Mem:            15Gi       3.0Gi        9.5Gi       356Mi       2.6Gi        11Gi
Swap:          2.0Gi          0B       2.0Gi
Filesystem      Size  Used Avail Use% Mounted on
/dev/sda2       100G   40G   57G  42% /
```


### 0.4. Проверка и установка базовых инструментов (git, gcc, Python)

Проверяем наличие:

```bash
vlad@vlad:~$ which git || echo "git: NOT INSTALLED"
git: NOT INSTALLED

vlad@vlad:~$ which gcc || echo "gcc: NOT INSTALLED"
gcc: NOT INSTALLED

vlad@vlad:~$ which python3 || echo "python3: NOT INSTALLED"
python3: NOT INSTALLED
```

Если что-то не найдено — ставим:

```bash
vlad@vlad:~$ sudo apt update
vlad@vlad:~$ sudo apt install -y git build-essential python3 python3-venv python3-pip
```

Проверяем:

```bash
vlad@vlad:~$ git --version
git version 2.34.1

vlad@vlad:~$ gcc --version | head -1
gcc (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0

vlad@vlad:~$ python3 --version
Python 3.10.12

vlad@vlad:~$ pip3 --version
pip 22.0.2 from /usr/lib/python3/dist-packages/pip (python 3.10)
```


### 0.5. Дополнительные проверки

Проверка отсутствия «лишних» инструментов:

```bash
vlad@vlad:~$ which verilator && verilator --version || echo "verilator: NOT INSTALLED (OK)"
verilator: NOT INSTALLED (OK)

vlad@vlad:~$ which ccache && ccache --version || echo "ccache: NOT INSTALLED (OK)"
ccache: NOT INSTALLED (OK)

vlad@vlad:~$ which bazel || echo "bazel: NOT INSTALLED (OK)"
bazel: NOT INSTALLED (OK)
```

Setuptools (информационно):

```bash
vlad@vlad:~$ python3 -c "import pkg_resources; print('setuptools:', pkg_resources.get_distribution('setuptools').version)" 2>/dev/null || echo "setuptools: not installed system-wide (OK)"
setuptools: 59.6.0
```

GCC‑11/g++‑11 (если нет — поставим позже):

```bash
vlad@vlad:~$ gcc-11 --version 2>/dev/null | head -1 || echo "gcc-11: NOT INSTALLED (will be installed later)"
gcc-11: NOT INSTALLED (will be installed later)

vlad@vlad:~$ g++-11 --version 2>/dev/null | head -1 || echo "g++-11: NOT INSTALLED (will be installed later)"
g++-11: NOT INSTALLED (will be installed later)
```

Проверяем интернет-доступ:

```bash
vlad@vlad:~$ curl -s --max-time 5 https://github.com > /dev/null && echo "github.com: REACHABLE (OK)" || echo "github.com: UNREACHABLE (problem!)"
github.com: REACHABLE (OK)

vlad@vlad:~$ curl -s --max-time 5 https://storage.googleapis.com > /dev/null && echo "storage.googleapis.com: REACHABLE (OK)" || echo "storage.googleapis.com: UNREACHABLE (problem!)"
storage.googleapis.com: REACHABLE (OK)
```


***

## 1. Цель и предпосылки

**Цель:** на Ubuntu 22.04 дойти до успешного запуска:

```bash
./bazelisk.sh test --test_output=streamed \
  //sw/device/tests:uart_smoketest_sim_verilator
```

с итогом `PASS!` по UART и статусом `PASSED` в Bazel.[^1][^2]

Будем считать, что репозиторий будет в `~/opentitan`. Эквивалентно можно определить:

```bash
vlad@vlad:~$ export REPO_TOP=$HOME/opentitan
```

и использовать `$REPO_TOP` вместо явного пути.[^3][^4]

***

## 2. Клонирование OpenTitan и установка apt-зависимостей

### 2.1. Клонируем репозиторий

**Где выполняем:** `~`.

```bash
vlad@vlad:~$ git clone https://github.com/lowRISC/opentitan.git
Cloning into 'opentitan'...
remote: Enumerating objects: ...
remote: Counting objects: ...
remote: Compressing objects: ...
remote: Total ... (delta ...), reused ...
Receiving objects: 100% (...), done.
Resolving deltas: 100% (...), done.

vlad@vlad:~$ export REPO_TOP=$HOME/opentitan
vlad@vlad:~$ cd $REPO_TOP
vlad@vlad:~/opentitan$
```


### 2.2. Ставим пакеты из `apt-requirements.txt`

**Где выполняем:** `$REPO_TOP`.[^5][^3]

```bash
vlad@vlad:~/opentitan$ sed '/^#/d' ./apt-requirements.txt | xargs sudo apt install -y
Reading package lists... Done
Building dependency tree... Done
Reading state information... Done
The following additional packages will be installed:
  ...
After this operation, 650 MB of additional disk space will be used.
Do you want to continue? [Y/n] y
...
Setting up clang-14 ...
...
```

Если:

```text
E: Unable to locate package some-package
```

— включаем `universe`/`multiverse`, делаем `sudo apt update` и повторяем.

***

## 3. Python-окружение (venv) и Python-зависимости

### 3.1. Создаём venv

**Где выполняем:** `$REPO_TOP`.

```bash
vlad@vlad:~/opentitan$ python3 -m venv .venv
vlad@vlad:~/opentitan$ ls .venv
bin  include  lib  pyvenv.cfg
```


### 3.2. Активируем venv

**Где выполняем:** `$REPO_TOP`.

```bash
vlad@vlad:~/opentitan$ source .venv/bin/activate
(.venv) vlad@vlad:~/opentitan$
```


### 3.3. Ставим Python-зависимости

**Где выполняем:** `$REPO_TOP`, venv активирован.[^4][^3]

```bash
(.venv) vlad@vlad:~/opentitan$ pip3 install "setuptools<66.0.0"
Collecting setuptools<66.0.0
  Downloading setuptools-65.7.0-py3-none-any.whl (1.2 MB)
Installing collected packages: setuptools
Successfully installed setuptools-65.7.0
```

```bash
(.venv) vlad@vlad:~/opentitan$ pip3 install -r python-requirements.txt --require-hashes
Collecting ...
  Using cached ...
...
Successfully installed ...
```


***

## 4. RISC‑V toolchain в `~/ot_tools/riscv`

### 4.1. Скачиваем и ставим toolchain

**Где выполняем:** `$REPO_TOP`.

```bash
(.venv) vlad@vlad:~/opentitan$ ./util/get-toolchain.py --install-dir ~/ot_tools/riscv
INFO: Downloading toolchain...
INFO: Extracting to /home/vlad/ot_tools/riscv
INFO: Toolchain installation completed.
```

Проверяем:

```bash
(.venv) vlad@vlad:~/opentitan$ ls ~/ot_tools/riscv/bin
ld.lld  riscv32-unknown-elf-as  riscv32-unknown-elf-gcc  riscv32-unknown-elf-objdump  ...
```


### 4.2. Добавляем toolchain в PATH и проверяем

**Где выполняем:** `~`.

```bash
vlad@vlad:~$ echo 'export TOOLCHAIN_PATH=$HOME/ot_tools/riscv' >> ~/.bashrc
vlad@vlad:~$ echo 'export PATH=$TOOLCHAIN_PATH/bin:$PATH' >> ~/.bashrc

vlad@vlad:~$ export TOOLCHAIN_PATH=$HOME/ot_tools/riscv
vlad@vlad:~$ export PATH=$TOOLCHAIN_PATH/bin:$PATH
```

Проверки:

```bash
vlad@vlad:~$ which riscv32-unknown-elf-gcc
/home/vlad/ot_tools/riscv/bin/riscv32-unknown-elf-gcc

vlad@vlad:~$ ls $TOOLCHAIN_PATH/bin/riscv32-unknown-elf-as
/home/vlad/ot_tools/riscv/bin/riscv32-unknown-elf-as

vlad@vlad:~$ which ld.lld
/home/vlad/ot_tools/riscv/bin/ld.lld
```


***

## 5. Сборка и установка Verilator 4.210

### 5.1. Устанавливаем gcc‑11/g++‑11 (если не стоят)

**Где выполняем:** `~`.

```bash
vlad@vlad:~$ sudo apt install -y gcc-11 g++-11
Reading package lists... Done
Building dependency tree... Done
Reading state information... Done
...
Setting up gcc-11 ...
Setting up g++-11 ...
```


### 5.2. Клонируем и собираем Verilator

**Где выполняем:** `~`.[^6][^7]

```bash
vlad@vlad:~$ export VERILATOR_VERSION=4.210
vlad@vlad:~$ git clone https://github.com/verilator/verilator.git
Cloning into 'verilator'...
...

vlad@vlad:~$ cd verilator
vlad@vlad:~/verilator$ git checkout v$VERILATOR_VERSION
Note: switching to 'v4.210'.
...
```

```bash
vlad@vlad:~/verilator$ autoconf
vlad@vlad:~/verilator$ CC=gcc-11 CXX=g++-11 ./configure --prefix=/tools/verilator/$VERILATOR_VERSION
checking for gcc... gcc-11
checking for g++... g++-11
...
config.status: creating Makefile
```

```bash
vlad@vlad:~/verilator$ CC=gcc-11 CXX=g++-11 make -j"$(nproc)"
[  1%] Building ...
...
[100%] Built target verilator_bin
```

```bash
vlad@vlad:~/verilator$ sudo CC=gcc-11 CXX=g++-11 make install
...
Installing verilator in /tools/verilator/4.210/bin
...
```


### 5.3. Добавляем Verilator в PATH и проверяем, что он правильный

**Где выполняем:** `~`.

```bash
vlad@vlad:~$ echo 'export VERILATOR_VERSION=4.210' >> ~/.bashrc
vlad@vlad:~$ echo 'export PATH=/tools/verilator/$VERILATOR_VERSION/bin:$PATH' >> ~/.bashrc

vlad@vlad:~$ export VERILATOR_VERSION=4.210
vlad@vlad:~$ export PATH=/tools/verilator/$VERILATOR_VERSION/bin:$PATH
```

Проверки:

```bash
vlad@vlad:~$ verilator --version
Verilator 4.210 2021-10-09 rev v4.210

vlad@vlad:~$ which verilator
/tools/verilator/4.210/bin/verilator
```


***

## 6. Проверка Bazelisk

### 6.1. Запускаем `./bazelisk.sh version`

**Где выполняем:** `$REPO_TOP`.[^8][^9]

```bash
vlad@vlad:~$ cd $REPO_TOP
vlad@vlad:~/opentitan$ ./bazelisk.sh version
Starting local Bazel server (8.0.1) and connecting to it...
Build label: 8.0.1
Build target: bazel-out/k8-fastbuild/bin/src/main/java/com/google/devtools/build/lib/bazel/BazelServer_deploy.jar
Build time: ...
...
```

Если:

```text
./bazelisk.sh: Permission denied
```

— добавляем права:

```bash
vlad@vlad:~/opentitan$ chmod +x bazelisk.sh
vlad@vlad:~/opentitan$ ./bazelisk.sh version
...
```

Примечание: доки используют `bazel`, но при работе из исходников лучше `./bazelisk.sh`, чтобы попадать в нужную версию Bazel.[^8]

***

## 7. Подготовка окружения перед сборкой и тестами

Каждый раз в новом терминале:

**Где выполняем:** `$REPO_TOP`.

```bash
vlad@vlad:~$ cd $REPO_TOP
vlad@vlad:~/opentitan$ source .venv/bin/activate
(.venv) vlad@vlad:~/opentitan$ export TOOLCHAIN_PATH=$HOME/ot_tools/riscv
(.venv) vlad@vlad:~/opentitan$ export PATH=$TOOLCHAIN_PATH/bin:$PATH
(.venv) vlad@vlad:~/opentitan$ export VERILATOR_VERSION=4.210
(.venv) vlad@vlad:~/opentitan$ export PATH=/tools/verilator/$VERILATOR_VERSION/bin:$PATH
```

Проверки:

```bash
(.venv) vlad@vlad:~/opentitan$ which riscv32-unknown-elf-gcc
/home/vlad/ot_tools/riscv/bin/riscv32-unknown-elf-gcc

(.venv) vlad@vlad:~/opentitan$ which ld.lld
/home/vlad/ot_tools/riscv/bin/ld.lld

(.venv) vlad@vlad:~/opentitan$ which verilator
/tools/verilator/4.210/bin/verilator
```


***

## 8. Промежуточная сборка Verilator-симулятора

Этот шаг явно собирает Verilator‑стенд (`//hw:verilator`) без запуска тестов — удобно для диагностики проблем сборки.

### 8.1. Команда сборки

**Где выполняем:** `$REPO_TOP`, venv активен, PATH настроен.

```bash
(.venv) vlad@vlad:~/opentitan$ ./bazelisk.sh build //hw:verilator
Starting local Bazel server (8.0.1) and connecting to it...
INFO: Analyzed target //hw:verilator (XXX packages loaded, YYY targets configured).
INFO: Found 1 target...
Target //hw:verilator up-to-date:
  bazel-bin/hw/verilator/...
INFO: Elapsed time: 172.156s, Critical Path: 169.24s
INFO: N processes: ... action cache hit, ... internal.
INFO: Build completed successfully, N total actions
```


### 8.2. Типичные ошибки на этом шаге

Пример реальной проблемы, с которой ты сталкивался:

```text
ccache error Failed to create temporary file ... Read-only file system
make[^1]: Vchipsimtb.mk:118: gpiodpi.o: Error 1
make[^1]: Vchipsimtb.mk:120: jtagdpi.o: Error 1
...
make: Makefile:13: Vchipsimtb: Error 2
ERROR Failed to build lowrisc:dv:top_earlgrey:chip_verilator:sim:0.1
ERROR Build did NOT complete successfully
```

Интерпретация:

- Ошибка на уровне окружения сборки (ccache, права на файловую систему, tmpfs), а не в коде OpenTitan.
- Пока `./bazelisk.sh build //hw:verilator` не проходит успешно, идти к smoketest бессмысленно.

После устранения этих ошибок и успешного `build` можно переходить к запуску теста.

***

## 9. Запуск `uart_smoketest_sim_verilator`

### 9.1. Основной запуск

**Где выполняем:** `$REPO_TOP`, venv активен, PATH настроен.

```bash
(.venv) vlad@vlad:~/opentitan$ ./bazelisk.sh test --test_output=streamed \
  //sw/device/tests:uart_smoketest_sim_verilator
Starting local Bazel server (8.0.1) and connecting to it...
WARNING: Streamed test output requested. All tests will be run locally, without sharding, one at a time
DEBUG: /home/vlad/opentitan/rules/autogen.bzl:536:14: NOTE: stamping is disabled, the build_info section will use a fixed version string
INFO: Analyzed target //sw/device/tests:uart_smoketest_sim_verilator (846 packages loaded, 33204 targets configured).
INFO: Found 1 test target...
Target //sw/device/tests:uart_smoketest_sim_verilator up-to-date:
  bazel-bin/sw/device/tests/uart_smoketest_sim_verilator.bash
...
[2026-06-24T14:06:19Z INFO  ot_transport_verilator::subprocess::stdout] UART: Created /dev/pts/2 for uart0. Connect to it with any terminal program, e.g.
[2026-06-24T14:06:19Z INFO  ot_transport_verilator::subprocess::stdout] $ screen /dev/pts/2
[2026-06-24T14:06:19Z INFO  ot_transport_verilator::subprocess::stdout] UART: Additionally writing all UART output to 'uart0.log'.
[2026-06-24T14:06:19Z INFO  ot_transport_verilator::subprocess::stdout] JTAG: Virtual JTAG interface dmi0 is listening on port 44853. Use
...
[2026-06-24T14:06:19Z INFO  ot_transport_verilator::subprocess::stdout] Simulation running, end by pressing CTRL-c.
[2026-06-24T14:06:19Z INFO  ot_transport_verilator::transport] Verilator started with the following interfaces:
[2026-06-24T14:06:19Z INFO  ot_transport_verilator::transport] gpio_read = /home/vlad/.cache/bazel/_bazel_vlad/.../gpio0-read
[2026-06-24T14:06:19Z INFO  ot_transport_verilator::transport] gpio_write = /home/vlad/.cache/bazel/_bazel_vlad/.../gpio0-write
[2026-06-24T14:06:19Z INFO  ot_transport_verilator::transport] uart = /dev/pts/2
[2026-06-24T14:06:19Z INFO  ot_transport_verilator::transport] spi = /dev/pts/1
[2026-06-24T14:06:19Z WARN  opentitanlib::app] Could not read UART parity to check it is None
[2026-06-24T14:06:19Z INFO  opentitanlib::io::uart] set_flow_control to false
I00001 test_rom.c:190] kBuildInfo: scm_revision=54697461
I00002 test_rom.c:267] Test ROM complete, jumping to flash (addr: 20000480)!
I00001 ottf_main.c:175] Running sw/device/tests/uart_smoketest.c
I00002 ottf_main.c:182] Enabling OTTF alert catcher
I00003 ottf_main.c:114] Finished sw/device/tests/uart_smoketest.c
[2026-06-24T14:12:45Z INFO  opentitantool::command::console] ExitSuccess("PASS!\r\n")
[2026-06-24T14:12:45Z INFO  opentitantool] Command result: success.
[2026-06-24T14:12:45Z INFO  opentitantool] Command result: success.

INFO: Found 1 test target...
Target //sw/device/tests:uart_smoketest_sim_verilator up-to-date:
  bazel-bin/sw/device/tests/uart_smoketest_sim_verilator.bash
INFO: Elapsed time: 1076.560s, Critical Path: 835.46s
INFO: 432 processes: 1561 action cache hit, 2 internal, 430 linux-sandbox.
INFO: Build completed successfully, 432 total actions
//sw/device/tests:uart_smoketest_sim_verilator                           PASSED in 386.2s

Executed 1 out of 1 test: 1 test passes.
```


### 9.2. Если первый запуск дал TIMEOUT

Возможный финал:

```text
-- Test timed out at 2026-06-24 12:56:40 UTC --

TIMEOUT: //sw/device/tests:uart_smoketest_sim_verilator (Summary)
      /home/vlad/.cache/bazel/_bazel_vlad/f34fc18e2d0026c6729e7d027a265a16/execroot/_main/bazel-out/k8-fastbuild-ST-1df456420242/testlogs/sw/device/tests/uart_smoketest_sim_verilator/test.log
INFO: Found 1 test target...
INFO: Elapsed time: 4791.397s, Critical Path: 4640.33s
INFO: Build completed, 1 test FAILED, 1819 total actions
//sw/device/tests:uart_smoketest_sim_verilator                          TIMEOUT in 300.1s

Executed 1 out of 1 test: 1 fails locally.
```

Это значит: симуляция запустилась, но тест не уложился в дефолтный тайм-аут (~300 секунд).[^8]

**Шаг 1: смотрим лог теста**

```bash
(.venv) vlad@vlad:~/opentitan$ less /home/vlad/.cache/bazel/_bazel_vlad/f34fc18e2d0026c6729e7d027a265a16/execroot/_main/bazel-out/k8-fastbuild-ST-1df456420242/testlogs/sw/device/tests/uart_smoketest_sim_verilator/test.log
```

**Шаг 2: перезапуск с увеличенным тайм-аутом**

```bash
(.venv) vlad@vlad:~/opentitan$ ./bazelisk.sh test \
  --test_output=streamed \
  --test_timeout=900 \
  --verbose_failures \
  //sw/device/tests:uart_smoketest_sim_verilator
```

Ожидаемый результат — как в 9.1: `ExitSuccess("PASS!\r\n")` и `PASSED`.

### 9.3. Мини-чек-лист успешного smoketest

Считаем тест успешно пройденным, если:

1. Есть сообщения транспорта Verilator:
    - `Simulation running, end by pressing CTRL-c.`
    - `Verilator started with the following interfaces:` + `gpio_read/gpio_write/uart/spi`.[^7][^6]
2. Есть сообщения ROM и OTTF:
    - `Test ROM complete, jumping to flash (addr: 20000480)!`
    - `Running sw/device/tests/uart_smoketest.c`
    - `Finished sw/device/tests/uart_smoketest.c`.[^10]
3. Есть `ExitSuccess("PASS!\r\n")` и `Command result: success.` от opentitantool.
4. В резюме Bazel:
    - `//sw/device/tests:uart_smoketest_sim_verilator  PASSED in ...`
    - `Executed 1 out of 1 test: 1 test passes.`[^2][^1]

Если 1–2 есть, а 3–4 заменены на `TIMEOUT`, увеличиваем тайм‑аут как в 9.2.

***

В таком виде документ полностью отражает твой реальный путь: с чистого Ubuntu → установка инструментов → Verilator → промежуточная сборка `//hw:verilator` → успешный `uart_smoketest_sim_verilator`.

<div align="center">⁂</div>

[^1]: https://opentitan.org/book/sw/doc/build_software.html

[^2]: https://opentitan.org/earlgrey_1.0.0/book/sw/doc/build_software.html

[^3]: https://opentitan.org/earlgrey_1.0.0/book/doc/getting_started/index.html

[^4]: https://opentitan.org/book/doc/getting_started/index.html

[^5]: https://github.com/lowRISC/opentitan/blob/master/apt-requirements.txt

[^6]: https://opentitan.org/book/doc/getting_started/setup_verilator.html

[^7]: https://opentitan.org/earlgrey_1.0.0/book/doc/getting_started/setup_verilator.html

[^8]: https://opentitan.org/book/doc/getting_started/build_sw.html

[^9]: https://opentitan.org/book/doc/contributing/bazel_notes.html

[^10]: https://opentitan.org/book/sw/device/lib/testing/test_framework/index.html

