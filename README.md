EN | [RU](docs/README_RU.md)

# xv_meca

![C++](https://img.shields.io/badge/C++-00599C?style=flat-square&logo=cplusplus&logoColor=white)


Internal for **Meccha Chameleon** (Steam): ESP, teleport, web panel, single-exe launcher with embedded DLL.

**Game version:** 2.5.0

---

## Structure

```
client/          DLL sources (MecchaCheatV fork + xv_meca rebrand)
injector/        GUI launcher, embed DLL in exe
build/           CMake + MinGW cross-compile
pack/            config.json, menu.ini
release/         ready build
scripts/         embed_payload.py, patch_sdk_for_gcc.py, strip_comments.py
res/             fonts and resources
```

## Release

`release/`:

| File | Description |
|------|----------|
| `xv_meca.exe` | Launcher + injector (DLL inside) |
| `config.json` | Cheat settings |
| `menu.ini` | Menu position/size |

### Launch

1. Steam must be installed.
2. Run `xv_meca.exe`.
3. The launcher starts **Meccha Chameleon** with `-dx11` and auto-inject.
4. Menu: **INS**, **HOME**, or **Right Shift**.

Game process: `PenguinHotel-Win64-Shipping.exe`

## Build from source

Requires macOS/Linux with MinGW:

```bash
brew install cmake ninja mingw-w64
cd build
./build.sh
```

Artifact: `dist/xv_meca.exe`

During build, `embed_payload.py` embeds the DLL into the exe.

## Features

- Player ESP, Decoy ESP
- Always visible, no detection
- Auto disable shadow
- Teleport, sprint multiplier
- Set name, web-only mode
- Web panel (local server + overlay)

## Security

- No secrets in the repo
- Build cache and generated `embedded_payload.*` are in `.gitignore`

## Disclaimer

For educational purposes only. Use in online games at your own risk.
