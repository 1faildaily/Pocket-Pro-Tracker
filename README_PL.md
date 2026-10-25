# Pocket Pro Tracker v0.3.0: FULLSCREEN + LIVE EDIT (eksperymentalny)

To **SourceKit**, nie gotowa binarka. Skopiuj zawartość ZIP do repozytorium przez GitHub Desktop, uruchom Actions. Potem pobierz artefakt i zainstaluj wewnętrzny `pocket_pro_tracker_pt2.zip` przez PortMaster.

## Zmiany

- Przywrócono pierwotny **FULLSCREENSTRETCH=TRUE** na Anbernicu. Bez 602x480 ani dodatkowego renderera.
- **L1/R1** uruchamiają natywne `decPatt()` / `incPatt()` (oryginalne ALT + LEFT/RIGHT PT2). Nie zmieniają tabeli pozycji SONG.
- **A+UP** podwyższa, **A+DOWN** obniża nuty/wartości; A+LEFT/RIGHT pozostaje -1/+1.
- **SELECT wciśnięty i puszczony SAM podczas odtwarzania** przełącza PLAY <-> LIVE EDIT. Bazuje na MODE_RECORD PT2, bez restartu dźwięku. Edycje patternu są zabezpieczone blokadą audio.
- SELECT+START nadal PLAY SONG od wybranego ROW; nie wywołuje LIVE EDIT.
- START = PLAY PATTERN / STOP, L1+M = ESC, L2/R2 = wybór sampla, X/Y = mysz.

**Ograniczenie LIVE EDIT:** wiersz edycji może podążać za playheadem tak jak w trybie RECORD. Zmiany wejdą przy kolejnym odczycie nuty, nie zmieniają brzmienia aktualnie wybrzmiewających dźwięków. Eksperyment testuj na KOPII pliku MOD.

## GitHub Desktop

1. Wypakuj zawartość SourceKit i skopiuj do repozytorium (nie sam folder).
2. Summary: `PPT v0.3.0 - Restore fullscreen, native pattern switch and live edit`
3. Commit to main, potem Push origin. Po zielonej kompilacji wgraj wewnętrzny ZIP przez PortMaster.

Testy kontrolera i integracji kodu przeprowadzone lokalnie. **Nie testowano jeszcze na fizycznym Anbernicu.**
