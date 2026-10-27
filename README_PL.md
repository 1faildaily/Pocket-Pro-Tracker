# Pocket Pro Tracker v0.3.25 - FULL PATTERN + DISK-OP navigation

Baza: v0.3.24. Zachowane MONO, sample 128 KB, A+B NOTE+sample,
usuwanie zaznaczenia przez Y, schowek, LIVE EDIT i POSITION LOOP.

## 1. MENU: pełnoekranowy PATTERN EDIT
- Naciśnij i puść samo MENU, aby otworzyć nowy widok.
- Ponowne MENU wraca do poprzedniego widoku (PATTERN / POSITION / DISK / SAMPLER).
- 32 wiersze naraz, 4 kanały, numery wierszy 00–63, przewijanie za kursorem.
- Wąski nagłówek POS / PAT / LEN / SMP i dolny STATUS.
- Kolory pobierane z dostarczonego colors.ini XCOPY, bez podmieniania palety.
- Zachowane wszystkie kontrolerowe skróty edycji, kopiowania, wklejania,
  usuwania, transportu, LIVE EDIT i ghost playhead.
- X w tym widoku ustawia pole i wiersz pod wskaźnikiem; Y obsługuje usuwanie zaznaczenia.
- Kombinacje MENU+START, MENU+strzałki i L1+MENU zachowują funkcje,
  nie przełączają dodatkowo widoku. ESC w pełnym widoku wraca z niego.
- Prawy analog może bezpośrednio przełączyć do innego widoku.
- Okna potwierdzeń, edycja tekstu i wybór RESAMPLE NOTE mają pierwszeństwo:
  zakończ je przed zmianą widoku MENU.
- Pełny widok jest nakładką tylko podczas przesyłania obrazu na ekran.
  Oryginalny framebuffer zostaje natychmiast przywrócony; renderer bazowy
  i jego przyrostowe odświeżanie zachowują swój stan.

## 2. DISK-OP: D-pad i X
- Góra / dół: wybierz katalog lub plik; lista przewija się na granicach.
- Wybór ma szarą belkę i kolorowe podkreślenie.
- X: otwórz katalog lub załaduj plik; zaznaczone '..' wraca o poziom wyżej.
- Przy zmianie katalogu wybór wraca do początku listy.
- Oryginalne pytanie o niezapisany moduł jest zachowane.
- R1+D-pad nadal steruje myszą. R2/L2 zachowują natywne kliknięcia
  w DISK-OP, więc możesz obsłużyć również ekranowe przyciski.
- X nie wysyła jednocześnie kliknięcia myszy pod wskaźnikiem.
- Podczas odczytywania katalogu nawigacja i aktywacja listy są wstrzymane.

## Instalacja
Zastąp pliki repozytorium, również .github -> Commit -> Push.
Artefakt: PPT_v0.3.25_FullPattern_DiskNavigation_ARM64.
Zainstaluj zawarty pocket_pro_tracker_pt2.zip wraz z protracker.ini i colors.ini.

## Sprawdzone
- Testy kontrolera i integracji, w tym ponowne nakładanie patcha.
- MENU: wejście/powrót, kombinacje transportu i kierunków bez przełączania.
- DISK-OP: D-pad, X bez zdublowanego kliknięcia, przewijanie i granice listy.
- Lokalna kompilacja pełnego PT2 v1.92 + patch na x86-64.
- Render rzeczywistymi fontami PT2: 32 wiersze, ROW 63, aktualna paleta XCOPY.
- Przywracanie framebufferu po nakładce, bez naruszania obrazu bazowego.
- Kod miksera, Pauli, obsługi 9xx i zapisu MOD nie został zmieniony w tej wersji.

Do sprawdzenia: kompilacja ARM64 w GitHub Actions i działanie na Anbernicu.
To nowy widok interfejsu; przed pracą na ważnym utworze przetestuj na kopii MOD-a.

---
Poprzednia dokumentacja:

# Pocket Pro Tracker v0.3.24 - usuwanie NOTE i zaznaczenia

Baza: v0.3.23. Zachowane domyślne MONO i sample do 131070 bajtów.
SAVE MODULE i struktura MOD pozostają bez zmian.

- A+B na NOTE usuwa nutę oraz numer sampla, pozostawia komendę i parametr FX.
- Na innych polach A+B działa jak wcześniej.
- SELECT+B rozpoczyna zaznaczanie, D-pad rozszerza blok.
- Y podczas aktywnego zaznaczania usuwa dokładnie zaznaczone pola i kończy zaznaczanie.
- B nadal kopiuje blok; SELECT+A wkleja. Usuwanie nie nadpisuje schowka.
- W bloku NOTE, cyfry SAMPLE i FX są niezależne: aby usunąć także sample,
  obejmij zaznaczeniem jego cyfry. Reguła NOTE+sample dotyczy skrótu A+B.
- W czasie odtwarzania edycja wymaga LIVE EDIT.
- POSITION-ED, SAMPLER, edycja nazw i potwierdzenia zachowują własną obsługę Y.

Instalacja: zastąp pliki wraz z .github -> Commit -> Push.
Artefakt: PPT_v0.3.24_Note_Block_Delete_ARM64.
Przy instalacji zachowaj ustawienia nowego protracker.ini: MONO oraz 128KB.
Testy kontrolera sprawdzają NOTE+sample bez zmiany FX, granice bloku,
usuwanie pojedynczej cyfry, zachowanie schowka i brak kliknięcia Y.
Test na urządzeniu i kompilacja ARM64 wymagają sprawdzenia.

---
# Pocket Pro Tracker v0.3.23 - domyślne MONO + sample 128 KB

Usunięto skrót MENU + X i całe odliczanie 2 sekund.
Przywrócono obsługę kontrolera z v0.3.20.
MONO jest ustawione w protracker.ini: STEREOSEPARATION=0.
Obsługa długich sampli zachowana: 64K_LIMIT=FALSE (do 131070 bajtów).
Pozostałe funkcje v0.3.20 pozostają bez zmian.

## Instalacja
Zastąp pliki repozytorium, włącznie z .github. Commit -> Push.
Artefakt: PPT_v0.3.23_DefaultMono_128KB_ARM64.
Zainstaluj zawarty pocket_pro_tracker_pt2.zip, koniecznie podmieniając także protracker.ini.
Zamknij PPT przed podmianą i uruchom ponownie po aktualizacji.
Wczytaj oryginalne MOD-y, jeśli wcześniejszy zapis uciął sample.

MENU + X nie ma już funkcji MONO. Jeśli system muOS przechwytuje tę kombinację,
usunięcie skrótu w aplikacji nie wyłącza skrótu systemowego; nie używaj jej.
Bez logu nie ustalono przyczyny zgłoszonego natychmiastowego zamknięcia.

## Weryfikacja
Testy kontrolera i integracji; kontrola usunięcia skrótu oraz obu ustawień.
GitHub Actions sprawdza oba ustawienia także w końcowym ZIP-ie.
Kompilacja ARM64 i odsłuch na urządzeniu pozostają do sprawdzenia.

## Funkcje bazowe

# Pocket Pro Tracker v0.3.20 — RANGE ACCELERATION + RESAMPLE NOTE

Baza: **PPT v0.3.19**. Zachowane są wszystkie poprzednie funkcje, w tym niezależne krawędzie RANGE, LIVE EDIT, ghost playhead, POSITION LOOP, kopiowanie LSDJ, EDITSKIP, pełnoekranowy widok, paleta XCOPY RED i ukryte cztery VU-metry kanałów.

## 1. SAMPLER: precyzyjna edycja RANGE

| Skrót | Działanie |
|---|---|
| SELECT (SHIFT) + D-pad ← | Przesuń początek RANGE w lewo |
| SELECT (SHIFT) + D-pad → | Przesuń początek RANGE w prawo |
| MENU + D-pad ← | Przesuń koniec RANGE w lewo |
| MENU + D-pad → | Przesuń koniec RANGE w prawo |

**Pojedyncze kliknięcie = 1 offset (bajt).** Po 280 ms przytrzymania zaczyna się powtarzanie co 85 ms; krok zależy od całkowitego czasu przytrzymania kierunku:

| Czas przytrzymania | Krok na powtórzenie |
|---|---|
| poniżej 450 ms | 1 |
| od 450 ms | 2 |
| od 900 ms | 5 |
| od 1350 ms | 10 |
| od 1800 ms | 50 |
| od 2300 ms | 100 |

Po zwolnieniu kierunku licznik przyspieszenia resetuje się. Krawędzie nie mogą się skrzyżować: zakres musi mieć minimum 2 bajty. Jeżeli MENU i SELECT są przytrzymane razem, MENU steruje prawą krawędzią. Działanie SHIFT + D-pad w PATTERN EDIT i RANGE LOOP bez zmian.

## 2. SAMPLER: karuzela RESAMPLING NOTE

Kliknij **X** na natywnym polu **RESAMPLE NOTE** (w SAMPLERZE). W karuzeli wybierasz nutę:

| Przycisk | Działanie |
|---|---|
| D-pad ↑ | Następny półton |
| D-pad ↓ | Poprzedni półton |
| D-pad ← | Oktawa w dół (−12 półtonów) |
| D-pad → | Oktawa w górę (+12 półtonów) |
| X | Zatwierdź i wróć do SAMPLERA |
| Y | Zakończ wybór, zachowując nutę |
| B | Anuluj, przywróć nutę sprzed edycji |

Obowiązuje natywny zakres **C-1…B-3 (36 nut, indeksy 0–35)**. Przy granicach karuzela zatrzymuje się. Nuta jest wyświetlana w polu RESAMPLE NOTE już podczas przesuwania D-padem. **Samo wybranie nuty nie przelicza sampla.** Resampling uruchamia natywny przycisk `RESAMPLE`, pozostawiony bez zmian.

Gdy karuzela jest aktywna, X/Y/B/D-pad nie sterują innymi funkcjami SAMPLERA. Poza karuzelą sterowanie bez zmian.

## Instalacja w GitHub Desktop

1. Rozpakuj cały SourceKit i zastąp zawartością repozytorium poprzednie pliki.
2. **Summary:** `PPT v0.3.20 - Precise sampler range and resample note carousel`
3. **Commit to main → Push origin**.
4. Po zielonym GitHub Actions pobierz artefakt `PPT_v0.3.20_Sampler_Range_ResampleNote_ARM64`. W środku znajduje się `pocket_pro_tracker_pt2.zip` do instalacji przez PortMaster.

## Testy

`bash tests/check.sh`: sterowanie obiema krawędziami, progi 1/2/5/10/50/100, zerowanie kroku po zwolnieniu, ograniczenia 2-bajtowego zaznaczenia, karuzela C-1…B-3, X/Y/B, brak przypadkowego kopiowania RANGE, nakładanie patcha na PT2 (włącznie z ponownym zastosowaniem) i bez zmian dotychczasowych funkcji. Pełna kompilacja ARM64 i działanie na fizycznym Anbernicu pozostają do weryfikacji.
