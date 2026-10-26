# Pocket Pro Tracker v0.3.17: PATTERN EDIT / SHIFT + D-pad (EDITSKIP)

Bazą jest **pełny SourceKit PPT v0.3.16**: oryginalny fullscreen, paleta XCOPY RED, brak kanałowych VU-meterów, prawy analog (cztery widoki), lewy analog (precyzyjna mysz), LIVE EDIT, ghost playhead, POSITION LOOP, kopiowanie LSDJ, nazwy MOD/SAMPLE i skróty samplera.

## Nowa funkcja w PATTERN EDIT

| Przycisk | Funkcja |
|---|---|
| **SELECT (SHIFT) + D-pad ↑** | Kursor przesuwa się w górę o EDITSKIP wierszy |
| **SELECT (SHIFT) + D-pad ↓** | Kursor przesuwa się w dół o EDITSKIP wierszy |
| **SELECT (SHIFT) + D-pad ←** | Zmniejsz EDITSKIP o 1 (minimum 0) |
| **SELECT (SHIFT) + D-pad →** | Zwiększ EDITSKIP o 1 (maksimum 9) |

Zmiana liczby wierszy pokazuje w oryginalnym pasku STATUS komunikat `EDITSKIP = X`, ten sam co PT2. Wykorzystujemy **natywną wartość `editor.editMoveAdd` PT2**, którą klawiaturą ustawia `CTRL+0..9`. Oznacza to, że zmiana kroku wpływa również na standardowe automatyczne przesuwanie pozycji po wpisaniu nut z podłączonej klawiatury.

Wartość **0** nadal wyłącza auto-przesuwanie podczas wpisywania nut w oryginalnym PT2. Natomiast ręczny skrót SELECT+↑/↓ przemieszcza kursor minimum o jeden wiersz, żeby nie utknąć w miejscu. Przejścia zawijają się w zakresie ROW 00–63. Ruch nie zmienia nut, sampli ani odtwarzania.

## Zgodność z wcześniejszymi funkcjami

- W **SAMPLERZE** SELECT+←/→ nadal zmienia RANGE (nie EDITSKIP).
- W **POSITION EDITOR** D-pad nadal przegląda i zaznacza pozycje do loopa.
- Podczas zaznaczania szarego obszaru kopiowania D-pad nadal rozszerza obszar.
- Przy edycji MOD/SAMPLE NAME D-pad nadal obsługuje karuzelę.
- R1+D-pad zachowuje szybszy ruch myszy.
- W **LIVE EDIT** kursor przesuwa się o EDITSKIP niezależnie od odtwarzania i ghosta.

## Instalacja (GitHub Desktop)

1. Rozpakuj SourceKit i skopiuj całą jego zawartość do repozytorium (zastąp poprzednie pliki).
2. Commit summary: `PPT v0.3.17 - Native EDITSKIP with SELECT D-pad`
3. **Commit to main → Push origin**; GitHub Actions kompiluje PT2 1.92 ARM64 z naszą nakładką.
4. Pobierz artefakt `PPT_v0.3.17_Pattern_EditSkip_ARM64`, rozpakuj jego wewnętrzny ZIP i zainstaluj przez PortMaster.

## Testy

`bash tests/check.sh` testuje nowy skrót, granice 0–9, zawijanie ROW 00–63, brak przypadkowego przełączenia LIVE EDIT oraz kompatybilność samplera. Testy lokalne nie zastępują kompilacji ARM64 i testu na fizycznym Anbernicu.
