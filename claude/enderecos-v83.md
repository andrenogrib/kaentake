# Endereços e offsets v83 usados (validar antes de rodar!)

Todos hardcoded para **um build específico** do `WvsClient` v83 (image base `0x400000`). O Orion é fork do Kaentake → alta chance de baterem, mas **valide num disassembler** contra o **seu** `Kaentake.exe`, especialmente os marcados com ⚠️ (não são exercitados pelo client vanilla, então não há corroboração indireta).

## Feature A — World Map Tooltip (`worldmapinfo.cpp`)

| Símbolo | Endereço | Uso | Crítico |
|---|---|---|---|
| WM `OnMouseMove` | `0x009EE2B3` | hook (mostra tooltip) | ⚠️ |
| WM `OnDestroy` | `0x009EB94A` | hook (limpa tooltip) | ⚠️ |
| `GetField` | `0x00437A0C` | gate "em campo" | ⚠️ |
| `CUIToolTip` ctor | `0x008E49B5` | constrói buffer estático | (já no vanilla) |
| `ClearToolTip` | `0x008E6E23` | limpa | (já no vanilla) |
| `SetToolTip_String2` | `0x008E7150` | monta corpo do tooltip | ⚠️ **crash se errado** |
| font init | `0x0046341A` | cria fontes "Dotum" | ⚠️ |

Spot array (layout reverso, namespace `wm`): `base = ecx-4`, `OFF_SPOTS=366`, `STRIDE=17`, `X=0`, `Y=1`, `FIELD_ARR=11`, origem `(13,35)`, `HIT_RADIUS=8`.

### Quest-markers (desligados — só validar se for ligar)

| Símbolo | Endereço |
|---|---|
| `GetQuestByNpc` | `0x0071DDEC` |
| `CheckStartDemand` | `0x00721163` |
| `CheckCompleteDemand` | `0x00721D2C` |
| `ZMap::GetPos` | `0x00500EA5` |
| `GetCurFieldID` | `0x00A1238B` |
| CWvsContext inst | `0x00BE7918` |
| CQuestMan inst | `0x00BED614` |
| Off CharData | `+0x20B8` |
| Off InProgressMap | `+0x5FF` |
| Off SecondaryStat | `+0x2134` |
| Off TamingMobLevel | `+0x37C0` |

Ícones quest: `UI/UIWindow.img/QuestIcon/{0,1,2}/0`.

## Feature B — Discord RPC (`discordintegration.cpp`, namespace `addr`)

| Símbolo | Endereço/offset | Uso | Crítico |
|---|---|---|---|
| `CUserLocal` inst | `0x00BEBF98` | login (level raw +0x33) e `*this` do GetJobCode | (vanilla usa) |
| `CharacterStat` | `0x00BF3CD8` | nome (+0x4), level fuse (+0x33) | (vanilla usa) |
| `CUIMiniMap` inst | `0x00BED788` | map id (+0x668) | ⚠️ |
| `GetJobCode` | `0x0095FFC3` | `int __fastcall(this, 0)` | ⚠️ |
| `GetJobName` | `0x004A77EF` | `char* __cdecl(jobId)` | ⚠️ |
| OFS_Ign | `+0x4` | nome do char | |
| OFS_Level | `+0x33` | level | |
| OFS_MapId | `+0x668` | map id | |

### Opcional (canal, não usado)

| Símbolo | Endereço/offset |
|---|---|
| CWvsContext | `0x00BE7918` |
| OFS_Channel | `+0x2058` |

## Singletons que batem entre vanilla e Orion (corroboração)

`CUserLocal 0x00BEBF98`, `CWvsContext 0x00BE7918`, `CharacterStat 0x00BF3CD8` — usados identicamente por `resolution.cpp`/`bypass.cpp` do vanilla. Isso dá confiança no mapa de endereços compartilhado, mas **não cobre** os endereços ⚠️ acima.
