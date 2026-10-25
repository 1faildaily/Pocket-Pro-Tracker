# Pocket Pro Tracker v0.3.2 · Frozen Cursor / Transport

SourceKit dla **ProTracker 2 Clone v1.92** na RG35XX Pro / muOS.
To paczka źródłowa do GitHub Desktop. NIE zawiera gotowego pliku wykonywalnego ARM64.

## Nowe tryby PLAY

| Skrót | Funkcja |
|---|---|
| START | PLAY PATTERN od ROW 00; jeśli gra, STOP |
| SELECT + START | PLAY bieżącego PATTERN od zaznaczonego ROW, nie SONG |
| MENU (M) + START | PLAY SONG od pierwszej pozycji (order 00, ROW 00) |
| SELECT podczas PLAY | Przełącz LIVE EDIT, bez restartu/STOP |
| L1 + M | Oryginalny ESC (potwierdzenie wyjścia) |
| L1 + START | Awaryjny ESC |

## LIVE EDIT: kursor niezależny od odtwarzania

W starszej wersji PT2 `updateUIPositions()` w `pt2_replayer.c` nadpisywał przy każdym takcie `song->currRow` (wiersz edytora) wartością `song->row` (wiersz odtwarzacza). W tej wersji nakładka blokuje **tylko tę synchronizację UI** w trybie LIVE EDIT. Nie zmienia zegara audio. Można przesuwać kursor D-padem do innego ROW, a odtwarzanie płynie dalej.

W LIVE EDIT przełączanie L1/R1 zmienia tylko `song->currPattern` (wyświetlany/edytowany pattern), a nie `modPattern` silnika audio. To umożliwia edycję innego patternu bez przestawienia grającego patternu. Po wyłączeniu LIVE EDIT, przy następnym takcie obraz ponownie śledzi odtwarzacz.

Pozostałe sterowanie v0.3.1 zachowane: L2/R2 = wybór sampla; A = wstaw nutę i aktywny sample (nadpisując istniejący); A + D-pad = zmiana wysokości/wartości; X/Y = kliknięcia myszy; lewy analog = precyzyjna mysz. FULL SCREEN jak na początku prac.

**Eksperymentalne:** testuj na kopii MOD. Synchronizacja zmian z audio jest zabezpieczona przez lockAudio() tylko na czas modyfikacji; kompilację ARM64 i działanie na fizycznym Anbernicu trzeba jeszcze potwierdzić.

## GitHub Desktop

1. Wypakuj SourceKit i skopiuj jego zawartość (w tym ukryty `.github`) do katalogu repozytorium.
2. Summary: `PPT v0.3.2 - Frozen live edit cursor and independent song transport`
3. Commit to main → Push origin.
4. Po zielonym Actions pobierz artefakt, włóż wewnętrzny ZIP do muOS PortMaster `autoinstall`.

## Testy lokalne

`bash tests/check.sh` sprawdza sterowanie i nakładanie patcha na wzorcowe fragmenty kodu, nie pełną kompilację PT2.
