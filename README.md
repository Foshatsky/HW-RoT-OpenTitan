# HW-RoT-OpenTitan

Материалы курса по Hardware Root of Trust: OpenTitan (Verilator / Earl Grey) и работа с RTOS **Zephyr** и **FreeRTOS**.

## Структура (ветка `master` — витрина)

| Путь | Содержание |
|------|------------|
| [`theory/`](theory/) | Теория: книги и конспекты |
| [`zephyr/`](zephyr/) | Трек Zephyr на OpenTitan |
| [`freertos/`](freertos/) | Трек FreeRTOS на OpenTitan |

## Ветки

- **`master`** — витрина (то, что видит организация)
- **`dev-z` / `dev-f`** — предвитрина треков Zephyr / FreeRTOS (содержимое может отличаться от master)
- Рабочие ветки создаются **от** `dev-z` или `dev-f` → merge в предвитрину → затем в `master`

## Лицензия

Собственные материалы репозитория — [Apache License 2.0](LICENSE).

При использовании фрагментов апстримов сохраняйте их лицензии: OpenTitan и Zephyr — Apache-2.0; FreeRTOS Kernel — MIT.
