[EN](../README.md) | RU

# xv_meca

![C++](https://img.shields.io/badge/C++-00599C?style=flat-square&logo=cplusplus&logoColor=white)


Internal для **Meccha Chameleon** (Steam): ESP, teleport, web-панель, single-exe launcher с embedded DLL.

**Версия игры:** 2.5.0

---

## Структура

```
client/          исходники DLL (форк MecchaCheatV + ребренд xv_meca)
injector/        GUI-лаунчер, embed DLL в exe
build/           CMake + MinGW cross-compile
pack/            config.json, menu.ini
release/         готовый билд
scripts/         embed_payload.py, patch_sdk_for_gcc.py, strip_comments.py
res/             шрифты и ресурсы
```

## Release

`release/`:

| Файл | Описание |
|------|----------|
| `xv_meca.exe` | Лаунчер + инжектор (DLL внутри) |
| `config.json` | Настройки чита |
| `menu.ini` | Позиция/размер меню |

### Запуск

1. Steam должен быть установлен.
2. Запусти `xv_meca.exe`.
3. Лаунчер стартует **Meccha Chameleon** с `-dx11` и авто-инжектом.
4. Меню: **INS**, **HOME** или **Right Shift**.

Процесс игры: `PenguinHotel-Win64-Shipping.exe`

## Сборка из исходников

Требуется macOS/Linux с MinGW:

```bash
brew install cmake ninja mingw-w64
cd build
./build.sh
```

Артефакт: `dist/xv_meca.exe`

При сборке `embed_payload.py` вшивает DLL в exe.

## Возможности

- Player ESP, Decoy ESP
- Always visible, no detection
- Auto disable shadow
- Teleport, sprint multiplier
- Set name, web-only mode
- Web-панель (локальный сервер + overlay)

## Безопасность

- Секретов в репо нет
- Кэш сборки и сгенерированный `embedded_payload.*` в `.gitignore`

## Disclaimer

Только для образовательных целей. Использование в онлайн-игре - на свой риск.
