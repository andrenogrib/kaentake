# Build e testes

## Pré-requisitos do build

1. **Submodules** (Detours + WzLib) precisam estar presentes:
   ```bash
   git submodule update --init --recursive
   ```
   WzLib gera os headers `IWz*.h` via **MIDL** a partir dos `.idl` no build.

2. **Toolchain**: VS Build Tools 2022 (sem IDE completa). cmake vem junto:
   ```
   C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe
   ```

## Compilar (x86)

Gerador "Visual Studio 17 2022", arch Win32 — o MSBuild monta o ambiente (cl/midl), **não precisa de vcvars**:
```powershell
$cmake = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
Set-Location "d:\Dropbox\projetos\kaentake"
& $cmake --preset release-win32          # ou debug-win32
& $cmake --build --preset release-win32
```
Saída: `build/Release/Kaentake.dll` + `Kaentake.exe`. (`build/` é gitignored.)

### Pasta `dist/` (pronta pra copiar)

Todo build copia automaticamente os 3 arquivos prontos pra **`dist/`** na raiz do projeto (via POST_BUILD no `CMakeLists.txt`):
```
dist/
├── Kaentake.dll    (a DLL com as features)
├── Kaentake.exe    (launcher)
└── Custom.wz
```
É só copiar a **`dist/` inteira** pra dentro da pasta do seu client v83 (onde está o `MapleStory.exe`) e rodar `Kaentake.exe` como admin. (`dist/` é gitignored.)

> Dica: o log do MSBuild sai em **UTF-16** — pra filtrar erros/warnings use PowerShell `Select-String`, não `grep` do bash.

Último build: **exit 0, 0 erros, 0 warnings**, todas as features compilaram.

## ⚠️ Pré-requisitos manuais ANTES de rodar no client

1. **Discord** — preencher em `src/discordintegration.cpp`: `DISCORD_APPLICATION_ID` e `LARGE_IMAGE_KEY` (criar app + subir arte no Developer Portal). Sem isso, presence aparece sem imagem ou no-op.
2. **Validar endereços** contra o seu `Kaentake.exe` (ver [enderecos-v83.md](enderecos-v83.md)). Errar `SetToolTip_String2 0x008E7150` = crash no 1º hover.
3. Instalar a DLL/EXE numa instalação v83 limpa (ver README do projeto) e rodar como admin.

## Roteiro de teste sugerido

**Feature A (tooltip)** — mais isolada, testar primeiro:
1. Logar, abrir o **World Map** (tecla padrão).
2. Passar o mouse sobre um mapa → deve aparecer o tooltip com nome + monstros + NPCs.
3. Se crashar no hover → endereço errado (provável `SetToolTip_String2`).
4. Quest-markers ficam **desligados** — não esperar bolhas ainda.

**Feature B (Discord)** — depois:
1. Ter o **Discord desktop aberto** na mesma máquina.
2. Milestone "presence aparece": com o app id preenchido, abrir o client → perfil do Discord deve mostrar `In the Menus`.
3. Logar num char → após ~15s, deve virar `Job | Lv. N` + `In <mapa>`.
4. Trocar de mapa → o `state` acompanha (com dedupe, respeitando o rate limit).

## Habilitar extras

- **Quest-markers**: `g_questMarkersEnabled = true` em `worldmapinfo.cpp` (validar os ~10 endereços antes).
- **Canal no Discord**: adicionar `seh_channel()` lendo `CWvsContext 0x00BE7918 + 0x2058` e concatenar em `state`.
- **Login instantâneo (Discord)**: flip por tick em `bypass.cpp` (em vez do self-poll) — ver Orion como referência.
