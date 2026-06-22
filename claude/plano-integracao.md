# Plano de integração — visão geral e arquitetura

## O que é o Kaentake

Client edit de MapleStory **v83** que injeta uma DLL no `WvsClient` via **Microsoft Detours**, dá hook em funções do jogo em endereços fixos e lê membros de objetos do jogo em offsets fixos. WZ é lido via **WzLib** (COM, gerado por MIDL a partir de `.idl`).

Estrutura:
- `src/<feature>.cpp` — cada feature, autocontida.
- `src/wvs/*.h` — classes do client reconstruídas (offsets/endereços fixos).
- `src/ztl/*.h` — reimplementação da ZTL da Nexon (ZArray, ZXString, TSingleton…).
- `external/Detours`, `external/WzLib` — submodules.

## Como uma feature é "plugada" (o padrão)

1. Cria `src/<feature>.cpp` com os hooks **+ um** instalador `void Attach<Nome>();`.
2. Declara o instalador em `src/hook.h` (bloco de forward-decls).
3. Chama em `AttachClientHooks()` (em `hook.h`) — roda na **Fase 2**, disparada por `SetUnhandledExceptionFilter_hook` em `system.cpp` (gated no return address `0x00796FDD`), já com heap/COM do jogo prontas.
4. Adiciona o `.cpp` em `src/CMakeLists.txt`.

Primitivas (em `hook.h`): `ATTACH_HOOK(target, detour)`, `MEMBER_AT(T, offset, nome)`, `MEMBER_HOOK(...)`, `PatchJmp/PatchCall`. WZ via `get_rm()->GetObjectA(L"...")` (`wvs/util.h`).

## A referência do Orion (e a divergência)

`Orion/Orion/` é um fork do Kaentake do amigo, com as duas features já feitas. Foi a referência principal. **Divergência decisiva**: o Orion usou o **Discord Game SDK** (pesado: `.dll`+`.lib`+wrapper de 31 arquivos), mas a lib que o usuário tem (`D:\Dropbox\projetos\discord-rpc`) é o **discord-rpc leve** (named-pipe, sem DLL, rapidjson embutido) — e ela traz um guia "para Kaentake". Adotamos a leve, **reusando as leituras de memória validadas do Orion**.

Como o Orion é fork do próprio Kaentake, o **mapa de endereços é o mesmo binário** — vários singletons batem idênticos entre vanilla e Orion (forte indício de que os endereços do tooltip/discord funcionam direto). Mesmo assim: **validar antes de rodar** (ver [enderecos-v83.md](enderecos-v83.md)).

## As duas features (resumo)

| | Feature A — Tooltip | Feature B — Discord RPC |
|---|---|---|
| Arquivos | `worldmapinfo.cpp`, `getwzinfo.cpp/.h`, `iconprovider.cpp/.h` | `discordintegration.cpp/.h`, `discord/*` (lib + rapidjson) |
| Hook/entrada | Detours em WorldMap OnMouseMove/OnDestroy | thread de refresh + named-pipe |
| Edição no host | `SetToolTip_String2` em `wvs/tooltip.h` | nenhuma (self-poll, sem `bypass.cpp`) |
| Lib externa nova | nenhuma (usa WzLib já linkado) | nenhuma via CMake (fontes compiladas direto) |
| Registro | `AttachMapInfoToolTip()` em `AttachClientHooks()` | `AttachDiscordRPC()` em `AttachClientHooks()` |

## Mudanças de build (consolidado)

- `.gitmodules` / `external/` — **sem mudança**.
- `src/CMakeLists.txt`:
  - +8 fontes (`worldmapinfo`, `getwzinfo`, `iconprovider`, `discordintegration`, `discord/discord_rpc`, `discord/connection_win`, `discord/rpc_connection`, `discord/serialization`, `discord/discord_register_win`).
  - `target_include_directories(injector PRIVATE src/discord)` (rapidjson embutido).
  - `target_compile_definitions(injector PRIVATE _CRT_NON_CONFORMING_SWPRINTFS _CRT_SECURE_NO_WARNINGS)`.
  - `SKIP_PRECOMPILE_HEADERS` nas fontes de terceiros do discord (autocontidas, não recebem o `pch.h` forçado).
- `src/hook.h` — forward-decls + chamadas de `AttachMapInfoToolTip()` e `AttachDiscordRPC()`.
- `src/wvs/tooltip.h` — `SetToolTip_String2`.

## Arquivos modificados vs novos

**Modificados:** `src/CMakeLists.txt`, `src/hook.h`, `src/wvs/tooltip.h`.

**Novos:** `src/worldmapinfo.cpp`, `src/getwzinfo.cpp/.h`, `src/iconprovider.cpp/.h`, `src/discordintegration.cpp/.h`, `src/discord/` (12 arquivos da lib + `rapidjson/`).
