# Pocket Pro Tracker v0.3.1 — poprawka LIVE EDIT, SAMPLE, myszy

Ta paczka zastępuje SourceKit v0.3.0 w repozytorium GitHub Desktop. To źródła, nie gotowa binarka. Zachowuje oryginalny pełnoekranowy interfejs i playback PT2 Clone 1.92.

## Zmiany

- **SELECT podczas PLAY:** eksperymentalne włączanie/wyłączanie edycji pada bez zatrzymywania utworu i bez zmiany wewnętrznego `editor.currMode` na `MODE_RECORD`. W v0.3.0 to przełączenie było niebezpieczne; rzeczywistej przyczyny powrotu do menu nie potwierdzono bez `launch.log`.
- **A na polu NOTE:** nadpisuje ZAWSZE nutę ostatnio wybraną oraz numer aktualnie wybranego sampla (L2/R2), także na zajętym wierszu. Dźwięk preview wykorzystuje właśnie nowy sample.
- **A na polu SAMPLE:** nadpisuje numer sampla wybranym (L2/R2). Gdy w wierszu jest nuta, odtwarza jej podgląd na nowym samplu.
- **Odsłuch:** odbywa się po zwolnieniu blokady audio. Przy grającym MOD silnik podglądu z v0.3.0 pomija jam, aby nie naruszyć odtwarzania; nowe dane zabrzmią przy ponownym odczycie wiersza przez replayer.
- **Mysz:** precyzyjny, nieliniowy ruch lewego analoga, z akumulacją subpikseli. Na małych wychyleniach wolniejszy, przy mocnym wychyleniu szybszy.
- **Pozostałe:** FULLSCREEN oryginalny, D-pad, L1/R1 patter(n), L2/R2 sample, SELECT+START od bieżącego ROW i L1+M ESC zachowane.

## Instalacja

1. Rozpakuj ZIP.
2. W GitHub Desktop → Repository → Show in Explorer skopiuj jego zawartość do katalogu repozytorium, nadpisując stare pliki.
3. Summary: `PPT v0.3.1 - Safe live edit, sample overwrite and precision mouse`
4. Commit to main → Push origin.
5. Pobierz ZIP z GitHub Actions, rozpakuj artefakt, a wewnętrzny `pocket_pro_tracker_pt2.zip` wgraj do autoinstall PortMaster.

## Testy na RG35XX Pro

1. Wczytaj KOPIĘ utworu, START, SELECT: muzyka powinna dalej grać. Naciśnij A na NOTE, sprawdź zmianę zawartości wiersza. SELECT ponownie wyłącza eksperymentalną edycję. START zatrzymuje.
2. Po zatrzymaniu wybierz sample 05 przez L2/R2. Na wierszu zawierającym inny sample i nutę, ustaw NOTE i naciśnij A: nuta powinna zostać zastąpiona zapamiętaną nutą, sample → 05 i słychać preview sampla 05.
3. Na SAMPLE wstaw A: sample powinien zostać nadpisany aktualnie wybranym samplem, nuta zachowana i podgląd odtworzony.
4. Zrób krótkie, niewielkie wychylenia lewego analoga w poziomie i pionie: kursor powinien ruszać powoli, bez dużych skoków.

Jeśli SELECT nadal zamyka aplikację, prześlij `ports/pocket_pro_tracker_pt2/launch.log` oraz `ppt_input.log` (końcowe fragmenty). Bez nich nie da się przesądzić, czy to crash PT2, błąd biblioteki SDL, czy zdarzenie systemowe.

**Zastrzeżenie:** testy jednostkowe/makietowe przeprowadzane lokalnie. Pełna kompilacja PT2 na GitHub Actions oraz zachowanie na konsoli wymagają sprawdzenia.
