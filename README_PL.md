# Pocket Pro Tracker v0.2.1 (PT2 Clone): poprawka sterowania

**To paczka ŹRÓDŁOWA do GitHub Desktop i GitHub Actions.** Nie instaluj jej ZIP bezpośrednio przez PortMaster.

## Co zostało poprawione

- D-pad z evdev `ABS_HAT0X`/`ABS_HAT0Y`, również obsługa `KEY_*` i `BTN_DPAD_*`.
- Przytrzymanie D-pada: 280 ms opóźnienia, następnie ruch co 85 ms.
- Prawy analog steruje myszą; zakres osi jest odczytywany z urządzenia zamiast zakładać ±32767.
- Jeżeli mysz nie zadziała analogiem, trzymaj **R1 + D-pad**, aby ją przesuwać.
- **R2** = lewy klik myszy, **L2** = prawy klik myszy.
- Wirtualny kursor myszy aktualizuje bezpośrednio kursor graficzny PT2 (uniezależnienie od wad `SDL_WarpMouseInWindow` w KMSDRM).
- Zapis diagnostyki `ports/pocket_pro_tracker_pt2/ppt_input.log` (zdarzenia/zakresy osi; maks. 24 na grupę).
- Zachowane mapowanie fizycznych przycisków z Twojego controls.json i oryginalne GUI PT2 1.92.
- Zachowane A do wstawiania, B do edycji i A+D-pad do zmiany wartości. Nie ma jeszcze pełnej edycji blokowej LSDJ.

## Jak zaktualizować projekt na GitHubie przez Desktop

1. Rozpakuj `PPT_v0.2.1_PT2_InputFix_SourceKit.zip` na PC.
2. W GitHub Desktop otwórz repozytorium Pocket-Pro-Tracker przez **Repository → Show in Explorer**.
3. Skopiuj do repozytorium **zawartość** rozpakowanego folderu, zastępując pliki o tych samych nazwach (zwłaszcza `patch/ppt_input.c`, skrypt `.sh`, pliki `tests` i workflow).
4. W GitHub Desktop: **Commit to main**, następnie **Push origin**.
5. Wejdź w repozytorium na GitHubie → **Actions**. Po zielonym buildzie pobierz artefakt.
6. Rozpakuj artefakt, a znajdujący się wewnątrz `pocket_pro_tracker_pt2.zip` umieść w sprawdzonym katalogu `autoinstall` PortMastera.
7. Przetestuj D-pad, mysz przez analog i **R1+D-pad**, kliknięcia L2/R2.
8. W razie problemu wyślij `ports/pocket_pro_tracker_pt2/ppt_input.log` oraz `launch.log`.

Testy w obecnym środowisku obejmują jedynie kompilację modułu na atrapach PT2, testy jednostkowe i poprawność paczki. Pełną kompilację ARM64 wykonuje GitHub Actions. Nie ma możliwości potwierdzenia rzeczywistego uruchomienia na RG35XX Pro w tym środowisku.

*Podstawowa aplikacja: 8bitbubsy/pt2-clone v1.92, BSD-3-Clause. Zachowaj plik LICENSE-PT2-clone.*
