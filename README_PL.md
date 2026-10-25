# Pocket Pro Tracker v0.2.6 – dopasowanie do wysokości i diagnostyka MENU

To **SourceKit** do GitHub Desktop i Actions, a nie gotowy ZIP do PortMastera. Bazuje na działającym PPT v0.2.5 i PT2 Clone 1.92.

## Zmiany

- **Duży ekran bez deformacji:** PT2 ma 320×255, a ekran RG35XX Pro 640×480. W pełnym ekranie obraz ma zostać wyświetlony w **602×480**, na środku, z **czarnymi pasami po około 19 px** z obu stron. `PIXELFILTER=NEAREST` zachowuje ostre krawędzie. Wymagana skala 1,882× nie jest skalą całkowitą. W kodzie PT2 zmieniamy `updateRenderSizeVars()`, ponieważ sam parametr `INTEGERSCALING=FALSE` nie wystarcza; upscaling był wymuszany na liczbach całkowitych. Współrzędne myszy nadal są przeliczane z `video.renderW/renderH/renderX/renderY`.
- **L1 + MENU/M:** Adapter rozpoznaje dodatkowo evdev `KEY_MENU` (139), `BTN_MODE` (316) i dotychczasowe 354. Kombinacja emituje natywny ESC w PT2, z potwierdzeniem zamknięcia. **Uwaga:** fizycznego M nie było w wcześniejszej kalibracji `controls.json`; muOS może całkowicie przechwycić ten przycisk. Żaden patch aplikacji nie naprawi brakującego zdarzenia wejścia; w tej wersji zapisujemy niesklasyfikowane kody `OTHER-KEY` w `ppt_input.log`, by to sprawdzić.
- **L1 + START:** zachowany jako działający, awaryjny ESC z potwierdzeniem, dopóki nie potwierdzimy MENU na konsoli.
- **Nie zmieniamy** odsłuchu nut przy A, lewego analoga, D-pada, PLAY/STOP przez START, SELECT + START = SONG od bieżącego ROW oraz reszty edycji.

## Instalacja GitHub Desktop

1. Rozpakuj SourceKit, skopiuj jego zawartość (`.github`, `patch`, `port`, `tests`, `README_PL.md`) do katalogu repozytorium przez `Repository → Show in Explorer`.
2. W **Summary** wpisz `PPT v0.2.6 - Fit to height and MENU diagnostics`.
3. `Commit to main` → `Push origin`.
4. Po zielonej kompilacji pobierz artefakt, rozpakuj zewnętrzne archiwum i wgraj **wewnętrzny** `pocket_pro_tracker_pt2.zip` do PortMaster `autoinstall`.

## Test na konsoli

- Sprawdź, czy obraz ma pełną wysokość 480 px i boczne czarne paski, a górna i dolna krawędź nie są obcięte.
- Sprawdź kursorem myszy lewy oraz prawy skraj graficznego interfejsu i kliknięcie w oba końce przycisków.
- Przytrzymaj L1 i naciśnij M. Jeśli nie otwiera się `REALLY QUIT?`, prześlij nowy `ports/pocket_pro_tracker_pt2/ppt_input.log` (zwłaszcza `OTHER-KEY` i linie BUTTON po wciśnięciu M).
- L1 + START powinno nadal wywołać oryginalny ESC (bez natychmiastowego zamknięcia programu).

## Status

Zostały sprawdzone składnia C, mock testy i poprawność patchowania kodu. **Pełna kompilacja PT2 ARM64 przez GitHub Actions i test na konsoli nadal są potrzebne.**
