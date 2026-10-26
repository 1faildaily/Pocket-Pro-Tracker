# Pocket Pro Tracker v0.3.19 — niezależne krawędzie RANGE w SAMPLERZE

Baza: pełny **PPT v0.3.18**. Bez zmian w odtwarzaniu, LIVE EDIT, ghost playhead, POSITION LOOP, palecie XCOPY RED, braku VU-metrów, kopiowaniu LSDJ, edycji tekstu i EDITSKIP.

## SAMPLER: sterowanie zakresem RANGE

| Skrót | Działanie |
|---|---|
| **SELECT (SHIFT) + D-pad ←** | Lewa krawędź (START) przesuwa się w lewo |
| **SELECT (SHIFT) + D-pad →** | Lewa krawędź (START) przesuwa się w prawo |
| **MENU (M) + D-pad ←** | Prawa krawędź (END) przesuwa się w lewo |
| **MENU (M) + D-pad →** | Prawa krawędź (END) przesuwa się w prawo |

- Każde pojedyncze kliknięcie: **10 offsetów (bajtów)**.
- Po 280 ms przytrzymania: automatyczne powtarzanie co 85 ms.
- Od 600 ms: krok 20. Od 1200 ms: krok 50. Od 2000 ms: krok 100.
- Po puszczeniu kierunku krok wraca do 10.
- Pierwsza i ostatnia krawędź są **niezależne**: przesuwanie jednej nie porusza drugą.
- Zaznaczenie nie może wyjść poza sample ani mieć mniej niż 2 bajty.
- Jeśli przytrzymasz równocześnie SELECT i MENU, **MENU ma pierwszeństwo** i porusza końcem zakresu.
- Pozostają: **A** odsłuch całości, **SELECT + A** odsłuch RANGE, **B** kopiowanie do pustego slotu i **LOOP ON** na wybranym RANGE.

W PATTERN EDIT `SELECT + D-pad` nadal obsługuje EDITSKIP, a w POSITION EDITOR dotychczasowe sterowanie pozostaje bez zmian. `L1 + M` w dalszym ciągu wywołuje ESC.

## Instalacja

1. Rozpakuj SourceKit, skopiuj wszystkie pliki do istniejącego repozytorium GitHub Desktop.
2. Summary: `PPT v0.3.19 - Independent sampler range edges`.
3. **Commit to main → Push origin**.
4. Po udanej kompilacji GitHub Actions pobierz artefakt `PPT_v0.3.19_Sampler_Independent_Range_Edges_ARM64` i zainstaluj wewnętrzne `pocket_pro_tracker_pt2.zip` przez PortMaster.

## Testy

`bash tests/check.sh` testuje niezależność granic, wszystkie cztery skróty, początkowy krok 10, progi czasowe 20/50/100, ograniczenia zaznaczenia i kolizje z EDITSKIP/pozostałymi trybami. Pełna kompilacja ARM64 i zachowanie na konsoli wymagają osobnej weryfikacji.
