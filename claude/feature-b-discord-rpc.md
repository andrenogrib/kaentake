# Feature B — Discord Rich Presence

## O que faz

Mostra no perfil do Discord (Rich Presence), atualizando a cada ~15s:
- **Logado:** `details` = `Job | Lv. N` (e nome se ligado); `state` = `In <mapa>`
- **Fora do jogo:** `details` = `In the Menus`
- Imagem grande (do Discord Developer Portal) + tempo decorrido desde o start

## Por que a lib leve (e não o Game SDK do Orion)

O Orion usou o **Discord Game SDK** (precisa shippar `discord_game_sdk.dll`, wrapper de 31 arquivos). Usamos o **discord-rpc leve** (`D:\Dropbox\projetos\discord-rpc`): named-pipe IPC, **sem DLL em runtime**, rapidjson embutido, `Discord_Initialize` não bloqueia (thread de IO própria). Mais leve e a própria lib traz guia "para Kaentake".

**Estratégia híbrida:** plumbing da lib leve **+** leituras de memória validadas do Orion.

## Arquivos

| Arquivo | Papel |
|---|---|
| `src/discordintegration.cpp/.h` | nossa integração (config, leituras, thread de refresh) |
| `src/discord/` | lib discord-rpc win32: `discord_rpc`, `connection_win`, `rpc_connection`, `serialization`, `discord_register_win` (+ headers) |
| `src/discord/rapidjson/` | rapidjson (header-only, embutido) |

> Não copiamos os arquivos `*_unix/*_linux/*_osx` (não compilam em win32 e não estão no CMake).

## Sequência da lib (discord-rpc)

```
Discord_Initialize(appId, &handlers, false, nullptr);   // 1x no start
... loop na thread de refresh:
    Discord_RunCallbacks();                              // dispatch
    Discord_UpdatePresence(&presence);                   // quando muda (dedupe)
```
`AttachDiscordRPC()` → `DiscordIntegration::Start()` é chamado de `AttachClientHooks()` (Fase 2). **`injector.cpp` não é tocado** (mais seguro que o Orion, que chamava do `DllMain`).

## Detecção de login: self-poll

A thread de refresh checa o estado sozinha (sem editar `bypass.cpp`):
```cpp
seh_isLoggedIn()  // CUserLocal (*(BYTE**)0x00BEBF98) -> level byte [+0x33] em [1,250]
```
Trade-off aceito: latência de até ~15s pra detectar login. (Alternativa instantânea: flip por tick em `bypass.cpp::CWvsApp::CallUpdate_hook` — não usada.)

## Leituras de memória (portadas do Orion, isoladas em SEH POD)

Todas as leituras cruas ficam em helpers `seh_*` (SEH, só POD) → os compositores de `std::string` nunca misturam `__try` com unwinding de objeto (evita **C2712**).

| Dado | Função | Endereço/offset |
|---|---|---|
| Login (level) | `seh_isLoggedIn` | CUserLocal `0x00BEBF98` + `0x33` |
| Level (display) | `seh_level` | CharacterStat `0x00BF3CD8` deref + `0x33` (fuse ZtlSecure: `b[0]^b[1]`) |
| Job id | `seh_jobId` | `GetJobCode 0x0095FFC3`(*`0x00BEBF98`, 0) |
| Job name | `seh_jobName` | `GetJobName 0x004A77EF`(jobId) |
| Nome do char | `seh_charName` | CharacterStat deref + `0x4` |
| Map id | `seh_mapId` | CUIMiniMap `0x00BED788` deref + `0x668` |
| Nome do mapa | `getMapName` | `GetMapById(id)` (WzLib, reusa Feature A) |

Strings ANSI(CP1252)→UTF-8 via `ANSItoUTF8` antes de mandar pro Discord.

> ⚠️ `getMapName` chama `GetMapById` (WzLib) **na thread de refresh** — igual ao Orion. Tecnicamente é acesso COM off-thread; `GetMapById` se protege com try/catch interno e retorna "Unknown" em falha. Risco baixo, mas existe (ver [riscos-e-pendencias.md](riscos-e-pendencias.md)).

## Config — PREENCHER À MÃO

No topo de `src/discordintegration.cpp`:
```cpp
static const char* DISCORD_APPLICATION_ID = "000000000000000000"; // TODO: seu app id
static const char* LARGE_IMAGE_KEY        = "logo";               // TODO: key do art asset (minúsculo)
static const char* SERVER_NAME            = "Kaentake";           // hover da imagem grande
static const int   REFRESH_SECONDS = 15;   // >= 15 (rate limit do Discord)
static const int   WARMUP_TICKS    = 3;    // ignora os primeiros ticks no startup
```
Passos: discord.com/developers/applications → New Application → copia **Application ID**; Rich Presence → Art Assets → sobe imagem → usa a **key** (minúscula).

Flags de exibição (estado estático, default igual ao Orion):
```cpp
show_charname  = false;
show_charlevel = true;
show_charjob   = true;
show_map       = true;
```

## Como adicionar o canal (opcional)

Vanilla não tem `g_nChannelPort`. O Orion lê de `CWvsContext 0x00BE7918 + 0x2058`. Para mostrar canal, adicione um `seh_channel()` lendo esse offset e concatene em `state` (ex.: `In <mapa>  |  CH <n>`). Por padrão não foi incluído.

## Shutdown

`Discord_Shutdown()` dá join na thread de IO → **não** chamar em `DLL_PROCESS_DETACH` (deadlock de loader-lock). Igual às referências, não há shutdown limpo (o fim do processo libera o pipe).
