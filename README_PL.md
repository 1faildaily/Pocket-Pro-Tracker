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
