# Riscos e pendências

## Pendências (follow-ups)

- [ ] **Validar todos os endereços** contra o seu `Kaentake.exe` ([enderecos-v83.md](enderecos-v83.md)) — antes do 1º run.
- [ ] **Preencher `DISCORD_APPLICATION_ID` + `LARGE_IMAGE_KEY`** em `src/discordintegration.cpp` (Discord Developer Portal).
- [ ] **Testar in-game** as duas features (não validado em runtime — só compila).
- [ ] (Opcional) Ligar quest-markers (`g_questMarkersEnabled = true`) + validar os ~10 endereços de quest.
- [ ] (Opcional) Mostrar canal no Discord (`CWvsContext + 0x2058`).
- [ ] (Opcional) Login instantâneo via `bypass.cpp` em vez de self-poll.
- [ ] **Commit** — nada foi commitado ainda. Sugestão: branch novo (estamos na `main`).

## Riscos conhecidos

1. **Endereços específicos do binário.** Tudo hardcoded pra um build v83. Diferença de binário quebra as features. Os singletons compartilhados batem com o vanilla (bom sinal), mas os endereços de WorldMap/tooltip/discord ⚠️ não são exercitados pelo vanilla → validar.

2. **`SetToolTip_String2 0x008E7150` errado = crash** no primeiro hover do World Map.

3. **Spot array é layout reverso** (`OFF_SPOTS=366`, `STRIDE=17`, `base=ecx-4`…). Build-specific; se o layout diferir, lê lixo (mas falha sem crashar).

4. **WzLib off-thread.** `getMapName` chama `GetMapById` (WzLib/COM) na thread de refresh do Discord, igual ao Orion. Tem try/catch interno, mas acesso COM off-thread é teoricamente racy. Se der problema: cachear o nome do mapa numa variável atualizada pela thread do jogo.

5. **Leituras de memória off-thread (Discord).** A thread de refresh lê memória viva do char sem lock. Mitigado por: gate `seh_isLoggedIn()`, `WARMUP_TICKS`, e todos os reads em `__try/__except`. Nunca chamar objetos COM/engine fora isso.

6. **Buffer estático do tooltip** `g_ttBuf[0x600]` assume `sizeof(CUIToolTip) <= 0x600` neste build.

7. **Encoding.** Strings do MS são CP1252/ANSI; Discord quer UTF-8 (`ANSItoUTF8`). Os defines `_CRT_SECURE_NO_WARNINGS`/`_CRT_NON_CONFORMING_SWPRINTFS` importam pros readers de WZ (`swprintf`/`_itow_s`).

8. **Rate limit do Discord** (~1 update / 15s). `REFRESH_SECONDS = 15` + dedupe respeitam isso. Não baixar.

9. **Shutdown.** `Discord_Shutdown()` dá join na thread de IO → **não** chamar em `DLL_PROCESS_DETACH` (loader-lock deadlock). Sem shutdown limpo (igual às referências).

10. **C2712 (SEH + unwinding).** Já tratado: leituras cruas isoladas em helpers `seh_*` POD-only; composição de `std::string` fica fora de qualquer `__try`. Manter esse padrão ao mexer.

## Notas de manutenção

- Padrão de feature: `Attach<Nome>()` em `hook.h` → `AttachClientHooks()`. Não mexer no `injector.cpp` pra estas duas.
- Fontes de terceiros do discord usam `SKIP_PRECOMPILE_HEADERS` (não recebem `pch.h`). Manter ao adicionar arquivos do discord.
- `getwzinfo.cpp` é **compartilhado** pelas duas features (tooltip e nome do mapa no Discord) — aparece uma vez só no CMake.
