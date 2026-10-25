# Pocket Pro Tracker v0.2.8 – pattern / sample / odsłuch + fullscreen

**SourceKit, nie gotowa binarka.** Jest przeznaczony do tego samego repozytorium GitHub Desktop co działający port ProTracker 2 Clone 1.92 na RG35XX Pro/muOS. Zachowuje kod renderowania v0.2.7, który nadal WYMAGA sprawdzenia na fizycznej konsoli.

## Zmiany w sterowaniu

| Przycisk | Nowa funkcja |
| --- | --- |
| L1 (puszczenie) | PATTERN -1 (jak strzałka przy PATTERN w oryginalnym PT2) |
| R1 (puszczenie) | PATTERN +1 |
| L2 | Aktywny SAMPLE -1 |
| R2 | Aktywny SAMPLE +1 |
| X | Lewy klik myszy |
| Y | Prawy klik myszy |
| Lewy analog | Ruch myszy (70% dotychczasowej prędkości) |
| D-pad | Nawigacja po patternie |
| A | Wstaw nutę, odsłuch przez oryginalny silnik PT2 |
| A + kierunki na NOTE | Zmienia wysokość i odsłuchuje **każdą** zmianę |
| A + kierunki na SAMPLE | Wpisuje numer sampla i aktualizuje aktywny sample |
| START | PLAY PATTERN / STOP |
| SELECT + START | SONG od bieżącego ROW / STOP |
| L1 + M | Oryginalny ESC z potwierdzeniem wyjścia |
| L1 + START | Zapasowy ESC (jak wyżej) |
| R1 + D-pad | Awaryjny ruch myszy, bez zmiany PATTERN |

W okienkach potwierdzenia A = Enter, B = Escape. Dla zgodności, kliknięcia L2/R2 nadal działają w oknach modalnych i na ekranie operacji dyskowych. W zwykłym edytorze kliknięcia myszy przejęły X/Y.

### PATTERN kontra pozycja utworu

L1/R1 zmieniają **numer patternu przypisany do aktualnej pozycji utworu** (pole PATTERN, nie POS). To zachowanie oryginalnych strzałek ProTrackera: modyfikuje tablicę kolejności MOD i oznacza utwór jako niezapisany. Nie kopiuje, nie usuwa ani nie tworzy samych nut. Jeśli potrzebny będzie wyłącznie przegląd istniejących patternów bez zmiany kolejności, zaimplementujemy osobny tryb.

L1/R1 zmieniają pattern po puszczeniu, aby `L1 + M`, `L1 + START` i `R1 + D-pad` nie uruchamiały przypadkowego przełączania. Podczas odtwarzania oraz w modalach te skróty nie zmieniają patternu.

### Pamięć nuty i sampla

PPT zapamiętuje tylko ostatnią wybraną **wysokość nuty**. Nie ma już własnej zmiennej `lastSample`. Nowa nuta, jeśli pole SAMPLE jest puste, otrzymuje numer aktywnego sampla z oryginalnego PT2 (`editor.currSample`, 01–31), wybieranego przez L2/R2, oryginalny interfejs lub zmianę sampla przez A + kierunki. Istniejący numer sampla w wierszu nie jest automatycznie nadpisywany.

Podczas zmiany wysokości istniejącej nuty przez A + D-pad jest odtwarzany odsłuch, również przy przytrzymaniu. Korzysta z istniejącego mechanizmu Paula/`jamAndPlaceSample` w PT2, nie z dodatkowego generatora. Jeśli próbka nie została załadowana, nie będzie słychać odsłuchu.

## Wyświetlanie: bez zmian względem przygotowanej v0.2.7

Przy ekranie 640 × 480 celem jest render **602 × 480**, wycentrowany, z paskami **19 px po bokach** i bez pionowych pasków. Poprawka pracuje na wielkości wyjścia SDL (dane w `ppt_video.log`) i wymusza natywne okno 640 × 480, aby uniknąć podwójnego skalowania. Nadal jest to **skala niecałkowita** (480 / 255), zatem nie jest możliwy 2× pixel-perfect przy zachowaniu pełnej wysokości. Rzeczywistego rezultatu na muOS nie potwierdzono.

## Budowa przez GitHub Desktop

1. Rozpakuj archiwum. Skopiuj jego ZAWARTOŚĆ (`.github`, `patch`, `port`, `tests`, README) do katalogu repozytorium, zastępując poprzednie pliki.
2. Commit Summary: `PPT v0.2.8 - Pattern sample shortcuts and pitch preview`.
3. **Commit to main → Push origin**.
4. Po zielonym GitHub Actions pobierz artefakt, rozpakuj go, a **wewnętrzny** `pocket_pro_tracker_pt2.zip` przenieś do autoinstalacji PortMastera.
5. Zachowaj poprzedni ZIP, jeśli potrzebny będzie powrót do wersji działającej na Twojej konsoli.

## Sprawdź na urządzeniu

- L1/R1 zmienia numer PATTERN o 1, ale nie powoduje przeskoku przy L1+M albo R1+D-pad.
- L2/R2 zmienia aktywny numer sampla, a X/Y daje kliknięcia lewy/prawy.
- Nowo wpisana nuta używa ostatniej wysokości, ale **bieżącego, ręcznie wybranego** sampla.
- `A + lewo/prawo` na NOTE odsłuchuje wszystkie kolejne półtony (po załadowaniu próbki).
- Interfejs sięga górnej i dolnej krawędzi, marginesy są wyłącznie po bokach.
- `L1 + M` wywołuje oryginalny ESC.

Jeśli ekran nadal ma czarne paski na wszystkich bokach, prześlij `ports/pocket_pro_tracker_pt2/ppt_video.log` i fotografię ekranu. W razie problemów z przyciskami również `ppt_input.log`.

## Status

Testy mockowanej logiki, kompilacji pojedynczego modułu C i nakładania poprawki zostały przeprowadzone lokalnie. **Pełna kompilacja ARM64 z oryginalnym PT2 w GitHub Actions i test konsoli nie zostały tutaj wykonane.**
