# Pocket Pro Tracker v0.3.22 - sample do 128 KB

W paczce wynikowej protracker.ini ma 64K_LIMIT=FALSE.
Limit jednego sampla: 131070 bajtów (128 KB minus 2 bajty).
Zmiana dotyczy ładowania i edycji, dzięki czemu zapis zachowuje dłuższe sample.
Skrót MENU + X przez 2 sekundy -> 0% MONO i pozostałe funkcje są zachowane.

WAŻNE: skopiuj także nowy protracker.ini z paczki PortMaster.
Po aktualizacji uruchom PPT ponownie i wczytaj ORYGINALNY moduł.
Wcześniej uciętych danych nie odzyskasz z już skróconego zapisu.
Sample >64 KB wymagają odtwarzacza obsługującego ten rozszerzony zakres.

GitHub Desktop: zastąp pliki wraz z .github -> Commit -> Push.
Artefakt: PPT_v0.3.22_128KB_Samples_ARM64.

Weryfikacja: testy kontrolera i integracji, kontrola konfiguracji w workflow.
GitHub Actions dodatkowo sprawdza 64K_LIMIT=FALSE w końcowym ZIP-ie.
Kompilacja ARM64 i test zapis/odczyt na urządzeniu wymagają sprawdzenia.

---
Dokumentacja poprzedniej wersji:

# Pocket Pro Tracker v0.3.21 - MENU + X: MONO

Baza: v0.3.20, wszystkie dotychczasowe funkcje zachowane.

Przytrzymaj MENU, następnie X, razem przez 2 sekundy.
Separacja stereo zostanie ustawiona na 0% przez natywny mikser PT2.
STATUS: MONO - STEREO SEP 0%.
To ustawienie mono, nie przełącznik mono/stereo. Kolejne użycie ponownie ustawia 0%.
Działa podczas odtwarzania bez zatrzymywania transportu.
Nie zmienia pliku MOD ani konfiguracji startowej; po restarcie obowiązuje protracker.ini.
X wciśnięty przy trzymanym MENU nie klika ani nie przesuwa karuzeli.
Puszczenie dowolnego przycisku przed 2 sekundami anuluje odliczanie.
Jeśli X naciśnięto przed MENU, jego wcześniejszy klik już wystąpił: używaj MENU jako pierwszego.
MENU musi docierać do programu (system nie może go przechwytywać).

## Instalacja
Rozpakuj całą paczkę do repozytorium, włącznie z .github, zastępując pliki.
GitHub Desktop: Commit to main -> Push origin.
Artefakt GitHub Actions: PPT_v0.3.21_MenuX_Mono_ARM64.
W nim pocket_pro_tracker_pt2.zip do instalacji przez PortMaster.

## Weryfikacja
Testy kontrolera obejmują próg 1999/2000 ms, jednokrotne wykonanie,
anulowanie po puszczeniu, ponowne uzbrojenie i blokadę kliknięcia X.
Kompilacja ARM64 i test na urządzeniu wymagają sprawdzenia.

---
Dokumentacja funkcji bazowych v0.3.20:

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
