# Pocket Pro Tracker v0.2.9b – naprawa błędu GitHub Actions

Naprawia `AssertionError: PT2 flipFrame presentation anchor changed` w skrypcie `patch/apply_patch.py` z v0.2.9. Przyczyną było porównywanie całego fragmentu oryginalnego PT2 co do znaku (włączając białe znaki/komentarze). Nowy skrypt odnajduje blok między `SDL_UpdateTexture` i `SDL_RenderPresent` wyłącznie wewnątrz `flipFrame`, sprawdza jego strukturę, a następnie go zastępuje. Nie wyłącza kontroli poprawności zmian.

## GitHub Desktop

Rozpakuj zawartość tego ZIP-a **do katalogu głównego dotychczasowego repozytorium**, nadpisując wszystkie pliki z paczki. Następnie wpisz w Summary: `PPT v0.2.9b - Fix flipFrame patch matching`; Commit to main → Push origin. Po zielonym GitHub Actions pobierz jego artefakt i rozpakuj wewnętrzny ZIP do PortMaster `autoinstall`.

## Zakres

To **SourceKit**, a nie program gotowy do uruchomienia. Zachowano v0.2.9 natywną kompozycję obrazu 640×480, aktualne sterowanie i audio. Testy obejmują także warianty oryginalnego bloku renderowania z innym formatowaniem. Ostateczne działanie na fizycznym RG35XX Pro wymaga sprawdzenia.

## Jak sprawdzić

Po uruchomieniu otwórz `ports/pocket_pro_tracker_pt2/ppt_video.log`. Nowa implementacja powinna dopisać linię `PPT VIDEO v0.2.9 COMPOSED frame=640x480 rect=19,0,602,480 logical=0x0 ...`. Jeśli jej nie ma, nowa ścieżka renderowania się nie uruchomiła. Jeśli jest, ale obraz nadal ma czarne pasy u góry/dołu, wyślij także **aktualne zdjęcie ekranu**. Będzie to oznaczało, że ograniczenie znajduje się poza składaniem pikseli w PT2 (np. warstwa KMSDRM/wyświetlacz).

## Status

Przetestowano programowo kompozycję obrazu 640×480, łącznie z pierwszym i ostatnim rzędem pikseli. Przeprowadzono test struktury źródeł i nakładania poprawki na imitację kodu upstream. Pełna kompilacja ARM64 GitHub Actions oraz test fizycznego Anbernica nadal są potrzebne.
