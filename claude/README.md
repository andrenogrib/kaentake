# Kaentake — Documentação das features (tooltip + Discord RPC)

Esta pasta documenta a integração de duas features no client **Kaentake** (MapleStory v83, DLL injection via Detours + WzLib), feita em Junho/2026.

## Status

| Feature | Estado | Build |
|---|---|---|
| **A — World Map Tooltip** | Implementada (renderer `new/`, inglês, quest-markers OFF) | ✅ compila |
| **B — Discord Rich Presence** | Implementada (lib leve discord-rpc, self-poll, inglês) | ✅ compila |

Build x86 release: **exit 0, 0 erros, 0 warnings** → `build/Release/Kaentake.dll`.

> ⚠️ Compila, mas **não foi validado em runtime** (precisa do client + servidor rodando) e **depende de 3 passos manuais** antes do primeiro uso. Ver [build-e-testes.md](build-e-testes.md) e [riscos-e-pendencias.md](riscos-e-pendencias.md).

## Índice

1. [plano-integracao.md](plano-integracao.md) — visão geral, arquitetura, decisões
2. [feature-a-world-map-tooltip.md](feature-a-world-map-tooltip.md) — tooltip de mapa
3. [feature-b-discord-rpc.md](feature-b-discord-rpc.md) — Discord Rich Presence
4. [enderecos-v83.md](enderecos-v83.md) — todos os endereços hardcoded usados (validar!)
5. [build-e-testes.md](build-e-testes.md) — como compilar + pré-requisitos manuais + roteiro de teste
6. [riscos-e-pendencias.md](riscos-e-pendencias.md) — riscos conhecidos e follow-ups

## Decisões tomadas (pelo usuário)

- Implementar as **duas** features, na ordem (baseline → A → B).
- Tooltip: renderer **`new/` rico** (clamp de tela, sombra, contagem de mobs, try/catch); quest-markers **começam desligados**.
- Idioma: **inglês**.
- Discord: detecção de login por **self-poll** (sem editar `bypass.cpp`).
- Discord: lib **leve discord-rpc** (a que o usuário apontou), **não** o Discord Game SDK pesado do Orion.

## Referência

- `Orion/Orion/` — fork do amigo, implementação de referência das duas features.
- `D:\Dropbox\games\ms_server\v83\dev\features\tooltip\new\` — fonte do tooltip.
- `D:\Dropbox\projetos\discord-rpc\` — lib discord-rpc (+ `guides/discord_rpc_guide.md`).
