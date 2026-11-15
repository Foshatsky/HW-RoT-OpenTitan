# Основные требования для развертывания Zephyr на OpenTitan

### 1. Версии компонентов

| Компонент        | Версия                                        | Комментарий                                                                                                                                   |
| ---------------- | --------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------- |
| Проект OpenTitan | `commit fee8a58ebe`                           | *В ходе работы были попытки запуска Zephyr на OpenTitan как локально, так и внутри контейнера. Версия использовалась одна в двух случаях.*    |
| Zephyr workspace | `commit cfa8a7bc6a`                           | *Локальный каталог: `~/zephyrproject`<br>Структура: `bootloader/`, `modules/`, `tools/`, `zephyr/`<br>Cборка для платы: `opentitan_earlgrey`* |
| Zephyr SDK       | `v1.0.0`<br>`v0.16.5-1`                       | *`v1.0.0` - локальная установка<br>`v0.16.5-1` - установка внутри контейнера.*                                                               |

### 2. Архитектура стенда

- Хост (Debian 12): хранит исходники `~/opentitan` и `~/zephyrproject`
- Docker образ `opentitan:latest`: содержит toolchain OpenTitan (Bazelisk, Verilator, Python env и т. п.)
- Docker контейнер `opentitan-dev`: состояние контейнера сохраняется между сессиями

### 3. Cтруктура каталогов

На хосте:
- `~/opentitan` — исходники OpenTitan
- `~/zephyrproject` — workspace Zephyr (west init/update)

В контейнере (через bind mounts):
- `/home/dev/opentitan`  → `~/opentitan`
- `/home/dev/zephyrproject` → `~/zephyrproject`
