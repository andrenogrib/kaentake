# Feature A — World Map Tooltip

## O que faz

Ao passar o mouse sobre um mapa no **World Map**, mostra um tooltip customizado com:
- Nome do mapa (street + mapName) + ícone do mapa
- Lista de **monstros** do mapa (ícone, `[Lv. N] Nome (xQtd)`), ordenados por level
- Lista de **NPCs** (ícone + nome)
- (Opcional, desligado) **quest-markers** por NPC (bolha branca = disponível / livro marrom = entregável / livro aberto = em progresso)

Tudo lido do WZ do próprio client (`String.wz`, `Mob.wz`, `Npc.wz`, `Map.wz`, `UI.wz`) — **nenhum asset extra pra distribuir**.

## Arquivos

| Arquivo | Papel |
|---|---|
| `src/worldmapinfo.cpp` | hooks + render + hit-test do spot array + (quest state) |
| `src/getwzinfo.cpp/.h` | nomes via WZ: `GetMapById`, `GetMobNameById`, `GetMobLevelById`, `GetNpcById` |
| `src/iconprovider.cpp/.h` | canvases: `GetMobIcon`, `GetNpcIcon`, `GetMapIcon`, `GetQuestMarkerIcon` |

Origem: `D:\Dropbox\games\ms_server\v83\dev\features\tooltip\new\` (versão `new/` — escolhida por ter clamp de tela, sombra, contagem de mobs, try/catch em volta dos draws e quest-markers protegidos por SEH).

## Hooks (Detours)

Instalados em `AttachMapInfoToolTip()`:
- `OnMouseMove` `0x009EE2B3` — `__fastcall` (this=ecx); calcula spot sob o cursor e desenha o tooltip.
- `OnDestroy` `0x009EB94A` — limpa o tooltip.
- `GetField` `0x00437A0C` — usado pra só mostrar quando em campo.

## Edição no host (única necessária)

`src/wvs/tooltip.h` ganhou em `CUIToolTip`:
```cpp
void SetToolTip_String2(int x, int y, ZXString<char> title, ZXString<char> desc,
                        int bUpDir, int bLogin, int bObjectToolTip, int nWidth,
                        int bDoubleOutline, int bCharToolTip)  // -> 0x008E7150
```
`m_nHeight (0x8)`, `m_nWidth (0xC)`, `m_pLayer (0x10)`, `ClearToolTip (0x008E6E23)` e o ctor (`0x008E49B5`) já existiam no vanilla.

## "Spot array" — constantes mágicas de RE

No `worldmapinfo.cpp`, namespace `wm` (layout reverso da janela do World Map):
```
base = (char*)ecx - 4
OFF_SPOTS = 366   STRIDE = 17   X=0  Y=1   FIELD_ARR = 11
ORIGIN = (13, 35)   HIT_RADIUS = 8
```
Idênticas às do Orion. São específicas do build — se o layout do client diferir, lê lixo (mas falha sem crashar, pois mapId<=0 só limpa o tooltip).

## Quest-markers (desligados por padrão)

O renderer `new/` tem suporte a bolhas de quest por NPC, mas elas dependem de **~10 endereços extras frágeis** (`GetQuestByNpc 0x0071DDEC`, `CheckStartDemand 0x00721163`, `CheckCompleteDemand 0x00721D2C`, `GetPos 0x00500EA5`, `GetCurFieldID 0x00A1238B`, offsets CharData `+0x20B8/+0x5FF`, SecondaryStat `+0x2134`, taming `+0x37C0`). Tudo protegido por SEH (falha → sem marcador, nunca crasha).

**Como estão hoje:** desligados. Em `worldmapinfo.cpp`:
```cpp
bool g_questMarkersEnabled = false;   // <- flip para true depois de validar os endereços
int GetNpcQuestState(int npcId) {
    if (!g_questMarkersEnabled) return 0;
    ...
}
```
**Para ligar:** mude para `true`, valide os endereços acima contra o seu binário, e teste num mapa com NPC que dá quest. A polaridade start-vs-complete é sutil (ver comentários no `namespace quest`).

## Buffer estático do tooltip

`g_ttBuf[0x600]` é construído como `CUIToolTip` via call cru ao ctor. Assume `sizeof(CUIToolTip) <= 0x600` neste build — se a classe for maior, corrompe memória adjacente. (Verificar se mexer no client.)
