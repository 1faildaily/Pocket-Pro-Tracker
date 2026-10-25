# Pocket Pro Tracker v0.2.3 — poprawka analogów i PLAY

**To pakiet źródłowy dla GitHub Desktop i GitHub Actions, nie ZIP do bezpośredniej instalacji przez PortMaster.**

## Najważniejsze zmiany

- **START → PLAY PATTERN** (to samo co przycisk PATTERN w oryginalnym PT2 Clone).
- **SELECT (SHIFT) + START → PLAY SONG** (to samo co przycisk PLAY w oryginalnym PT2 Clone).
- Zachowany dotychczasowy skrót **L1 + START → wyjście z programu** (UWAGA: bez automatycznego zapisu MOD).
- **Prawy analog → ruch myszy X/Y:** na podstawie Twojego logu i układu osi Anbernica używamy `ABS_RY` (kod 4) dla X i `ABS_RZ` (kod 5) dla Y. Nie pobieramy już X z osi `ABS_RX` (kod 3), która u Ciebie pochodziła z pionowego ruchu lewego drążka. **Rzeczywistą oś kod 5 trzeba zweryfikować na konsoli.**
- Nadal działają oryginalne funkcje PT2, sterowanie D-padem, wprowadzanie A/B, myszy R1+D-pad, R2/L2 i obsługa okien dialogowych z v0.2.2.
- Zapamiętane kody przycisków z dostarczonego `controls.json` pozostają bez zmian.

## Instalacja przez GitHub Desktop

1. Rozpakuj `PPT_v0.2.3_PT2_Analog_Play_SourceKit.zip` na komputerze.
2. W GitHub Desktop wybierz **Repository → Show in Explorer** w repozytorium `Pocket-Pro-Tracker`.
3. Skopiuj **zawartość paczki** do katalogu repozytorium, zachowując `.github/workflows`, `patch`, `port`, `tests`.
4. Kliknij **Commit to main → Push origin**.
5. Po ukończonej na zielono kompilacji w GitHub Actions pobierz **Artifacts**, rozpakuj pobrany ZIP.
6. Do PortMaster `autoinstall` skopiuj **wewnętrzny** plik `pocket_pro_tracker_pt2.zip`, bez rozpakowywania tego wewnętrznego ZIP-a.
7. Uruchom PPT. Sprawdź START, SELECT+START i ruch prawego drążka **osobno we wszystkich czterech kierunkach**.

## Oczekiwane zachowanie i diagnostyka

| Fizyczne sterowanie | Funkcja |
|---|---|
| START | PLAY PATTERN |
| SELECT + START | PLAY SONG |
| L1 + START | Zakończenie PPT, bez zapisu |
| Prawy analog lewo/prawo | Mysz lewo/prawo |
| Prawy analog góra/dół | Mysz góra/dół (oś 5, do weryfikacji) |
| R1 + D-pad | Awaryjny ruch myszy |
| R2 | Lewy klik myszy |
| L2 | Prawy klik myszy |

Jeśli w pionie prawy analog nadal nie działa, prześlij `ports/pocket_pro_tracker_pt2/ppt_input.log` po poruszeniu **tylko prawym analogiem** we wszystkich kierunkach. Kod zbiera próbki z osi 0..5, w tym ABS_RZ (5), co pozwoli potwierdzić ostateczne przypisanie.

Nie można zagwarantować uruchomienia portu ani dźwięku na RG35XX Pro na podstawie testów lokalnych. Testy `bash tests/check.sh` weryfikują adapter i skrypt nakładający poprawkę, pełną kompilację PT2 ARM64 wykonuje GitHub Actions. Oryginalny PT2 Clone 1.92: 8bitbubsy, BSD-3-Clause.
