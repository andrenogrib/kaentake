# Usando ida-pro-mcp + IDA Pro 9.3 pra criar features novas no client

> Guia de método: como reverter o client v83 com o IDA (via ida-pro-mcp) e
> transformar o achado em feature no padrão Kaentake. Setup do tooling em
> [build-e-testes.md](build-e-testes.md) e no submodule `external/ida-pro-mcp`.

## A ideia central

O ida-pro-mcp transforma o IDA 9.3 num "backend" que o Claude consulta direto:
abre o binário do client em modo **headless (idalib)**, decompila funções, segue
cross-references, lê bytes/offsets — sem copiar/colar do IDA. Isso fecha o loop:

```
você descreve a feature  ->  Claude acha a função/offset no IDA (MCP)
   ->  escreve o .cpp no padrão Kaentake  ->  compila  ->  testa
   ->  anota o achado no .idb (rename/comments) pra reusar depois
```

## O loop de criar uma feature (mapeado neste projeto)

Toda feature do Kaentake é: **achar um ponto no client -> hookar/ler -> reagir**.

1. **Definir o gancho** — que momento interceptar? (abriu item, mudou de mapa,
   recebeu pacote, subiu level, calculou dano…)
2. **Localizar no IDA** — achar função/endereço/offset com as tools do MCP.
3. **Traduzir pro idioma Kaentake** — `MEMBER_AT` (offsets),
   `MEMBER_HOOK`+`ATTACH_HOOK` (hook `__thiscall`), `reinterpret_cast` pra chamar
   funções do client, registrar em `AttachClientHooks()` (em `hook.h`).
4. **Compilar + testar.**

## Qual tool do MCP pra cada tarefa de RE

| Quero… | Tool do ida-pro-mcp |
|---|---|
| Achar função por **nome/string** | `find_regex` (strings) -> `xrefs_to` -> `lookup_funcs` |
| Entender **o que a função faz** | `decompile(addr)`, `disasm(addr)`, `analyze_funcs` |
| Achar **quem chama / é chamado** | `xrefs_to`, `callees`, `callgraph` |
| Descobrir **offset de struct** (`this+0xNN`) | `decompile` e olhar acessos `[reg+0xNN]`; `read_struct`/`search_structs` |
| Achar **endereço estável** (resiste a rebuild) | `find_bytes` / `find_insns` / **sigmaker** (assinatura de bytes) |
| Ler **valor/constante/global** | `get_bytes`, `get_int`, `get_global_value` |
| **Anotar** o achado (salva no .idb) | `set_comments`, `rename`, `set_type`, `add_bookmark` |
| Converter número (nunca de cabeça) | `int_convert` |

## Exemplos concretos pro nosso client

**1) Validar um endereço que já usamos** (follow-up pendente)
`decompile(0x008E7150)` pra confirmar que `SetToolTip_String2` é `__thiscall` com
10 args que monta um tooltip — batendo com `wvs/tooltip.h`. Idem pros hooks do
World Map (`0x009EE2B3` / `0x009EB94A`) e as leituras do Discord. Ver
[enderecos-v83.md](enderecos-v83.md) pra lista completa.

**2) Descobrir um offset novo**
Ex.: "map id atual" em `CUIMiniMap (0xBED788 + 0x668)`: `xrefs_to(0xBED788)` ->
`decompile` -> procurar `*(int*)(this + 0x668)` pra confirmar/corrigir o offset.

**3) Criar uma feature do zero** — ex.: *"mensagem no chat ao subir de level"*
- `find_regex("LevelUp|level up")` ou a função de efeito de level-up ->
  `xrefs_to` -> `decompile` pra entender os args.
- Identificar a função `__thiscall` + endereço.
- `levelup.cpp` com `MEMBER_HOOK(void, 0x........, OnLevelUp, ...)`; no `_hook`
  chama o original e dispara a msg; registra `AttachLevelUpMod()` em
  `AttachClientHooks()`.
- Compila, testa.

## A jogada mais valiosa: assinaturas em vez de endereços fixos

O risco nº1 do projeto é endereço hardcoded quebrar entre builds (ver
[riscos-e-pendencias.md](riscos-e-pendencias.md)). Com `api_sigmaker` dá pra gerar
um **padrão de bytes** da função (ex.: `48 8B ?? ?? E8 ?? ?? ?? ??`) e usar o
`GetAddressByPattern(...)` que o Kaentake **já tem** em `hook.h`, em vez do
endereço cru. Assim a feature sobrevive a recompiles do client. É o caminho pra
deixar tudo robusto.

## Fluxo prático (com o Claude)

1. Você diz a feature ("quero X quando Y").
2. Indica o binário: o ideal é um **`.idb` já analisado** do v83 (com nomes = ouro);
   senão o `MapleStory.exe` cru e o IDA analisa (mais lento).
3. Claude faz `idb_open(...)`, navega, traz a função + offsets (com a
   decompilação como prova).
4. Escreve o `.cpp` no padrão Kaentake, compila, testa.
5. Vai **anotando no .idb** (rename/comments/types) — cada feature deixa o
   database mais rico pra próxima.

## Pré-condições pra rodar

- Tools do MCP **vivas na sessão** (se não aparecerem, conferir `uv` no PATH que o
  Claude Code enxerga; reiniciar o Claude Code). Setup já feito: `uv` instalado,
  idalib ativado pro IDA em `D:\Dropbox\games\ms_server\tools\IDA Pro 9.3`.
- Saber **qual binário/idb** abrir.

## Ideias de primeira feature

Validar/assinaturar os endereços que já usamos · auto-pickup · dano total no chat ·
EXP/hora na tela · damage skin custom.
