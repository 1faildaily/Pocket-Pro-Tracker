# Pocket Pro Tracker (PPT) v0.2 alpha: port ProTracker 2 Clone 1.92

**Status: kod integracji, NIE gotowy plik wykonywalny PortMaster.**

Nie jest to kontynuacja makiety interfejsu napisanej w Go. Punktem wyjścia jest oryginalny kod
[8bitbubsy/pt2-clone v1.92](https://github.com/8bitbubsy/pt2-clone), BSD-3-Clause.
Nie zmieniamy oryginalnego GUI, silnika audio, edycji sampli ani importu/eksportu MOD.
Dodajemy moduł `src/ppt_input.c`, który czyta fizyczne przyciski i wprowadza
wybrane działania do oryginalnego modelu patternów.

## Przygotowany zakres v0.2 alpha

- sterowanie D-padem z autorepeat (280 ms, potem 85 ms),
- `A`: wstaw nutę z ostatnio zapamiętaną wysokością i samplem w pustym polu,
- `A + kierunki`: edycja NOTE, numeru sampla 01..31, FX 0..F i parametru 00..FF,
- pamięć ostatniej nuty i numeru sampla w sesji,
- kopia/wklejenie pojedynczego pola (jeszcze nie bloków LSDJ),
- użycie Twoich rzeczywistych kodów przycisków z `controls.json`,
- eksperymentalny prawy analog do myszy (ABS_RX/ABS_RY, do weryfikacji),
- wirtualne kliknięcia myszy R2 (lewy) i L2 (prawy),
- wyjście L1+START po uprzednim zapisaniu MOD,
- zachowany wygląd PT2 Clone 320×255, skalowanie do 640×480 i oryginalny sampler,
- wbudowane oryginalne odtwarzanie MOD, jeżeli kompilacja PT2 i SDL2 uruchomią się na muOS.

## Ważne: co nadal NIE jest zrobione

- **Nie ma jeszcze gotowej binarki ARM64 PT2.** W obecnym środowisku nie udało się pobrać
  całego upstream repozytorium ani skompilować programu z wymaganym SDL2/toolchainem.
- Graficzne zaznaczanie i kopiowanie wielowierszowych bloków na wzór LSDJ 9.x.
- Fizyczny test startu, dźwięku, odczytu SD, myszy oraz prędkości grafiki.
- Pełna obsługa ekranu i skrótów przez pad w dialogach PT2; na razie wymagają myszy.
- Trzy oktawy oryginalnego PT2 (C-1..B-3), a nie etykiety oktaw z osobnego testu PPT.

## Jak otrzymać binarkę ARM64 z tej paczki (GitHub Actions)

1. Rozpakuj tę paczkę, a jej **zawartość** umieść w nowym publicznym repozytorium GitHub.
2. Wejdź w zakładkę **Actions → Build Pocket Pro Tracker PT2 ARM64 → Run workflow**.
3. GitHub skompiluje oryginalny ProTracker 2 Clone 1.92 i naniesie nakładkę PPT.
4. Gdy kompilacja zakończy się powodzeniem, pobierz paczkę z sekcji **Artifacts**.
5. W pobranym archiwum znajdziesz `pocket_pro_tracker_pt2.zip`. To właściwy ZIP
   do katalogu autoinstalacji PortMastera.
6. Jeśli nie wystartuje, prześlij `launch.log` z `ports/pocket_pro_tracker_pt2`.

**Uwaga:** pipeline i kod dodatku zostały przetestowane lokalnie tylko składniowo,
bez pełnego upstream i bez urządzenia. Działanie kompilacji GitHub Actions jest
na tę chwilę niezweryfikowane.

## Wykorzystane pliki

- `patch/ppt_input.c` i `.h`: warstwa sterowania Anbernica i edycji wewnętrznych pól PT2.
- `patch/apply_patch.py`: automatyczna integracja z 1.92.
- `.github/workflows/build-ppt-arm64.yml`: build na ARM64 w Ubuntu 22.04 ARM.
- `port/Pocket Pro Tracker PT2.sh`: skrypt uruchamiający PortMaster z logiem.
- `port/pocket_pro_tracker_pt2/controls.json`: mapowanie z Twojej konsoli.
- `tests/check.sh`: testy lokalne kompilacji modułu przy użyciu atrap struktur SDL/PT2.

## Bezpieczeństwo danych

Port PT2 ma używać odrębnego katalogu `pocket_pro_tracker_pt2` i **nie nadpisuje**
działającej wersji testowej PPT v0.1.7. Na początek korzystaj z kopii MOD-ów.
Wyjdź z programu i wyłącz konsolę przed wyjęciem karty SD.

## Podstawa projektu

Upstream PT2 v1.92: https://github.com/8bitbubsy/pt2-clone, licencja BSD-3-Clause.
PPT jest niezależną modyfikacją i nie jest oficjalną wersją autora PT2.
