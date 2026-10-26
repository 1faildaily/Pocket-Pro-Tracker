Pocket Pro Tracker v0.3.16: XCOPY palette, channel VU meters hidden, audio unchanged.

PPT v0.3.13 XCOPY custom palette: colors.ini in this folder is loaded at startup.

PPT v0.3.12: Nazwy MOD NAME i SAMPLE NAME: X wejście/znak dalej, D-pad wybór, B usuń znak, Y zatwierdź. Pozostałe pola bez karuzeli.

PPT v0.3.10: POSITION follows normal playback; SELECT+Y clears loop and highlight.

# Pocket Pro Tracker v0.3.9 — EFEKTY: STATUS + BEZPIECZNE ZAKRESY

Zmiany względem v0.3.8:
- Gdy kursor znajduje się w FX lub parametrze efektu, pasek STATUS wyświetla krótką nazwę komendy ProTrackera (`C SET VOLUME`, `D PATTERN BREAK`, `E4 VIBRATO WAVE` itd.). STATUS zmienia się wraz z kursorem/efektem, również gdy edytujesz istniejący MOD.
- D-pad przy edycji parametru przestrzega zakresów: Bxx 00–7F; Cxx 00–40 (HEX); Dxx 00–63 (CYFRY DZIESIĘTNE zapisane jako BCD); E3 0/1, E4/E7 0–7 (0–3 normalnie, 4–7 bez restartu fali), E8 0–F (wszystkie działają identycznie); E0 0–F (0=on, 1–F=off); pozostałe Ex 0–F. Pozostałe komendy przyjmują pełny zakres 00–FF.
- D-pad ↑/↓ zmienia zaznaczony parametr dodatnio/ujemnie, a strzałki ←/→ pozwalają regulować o 1. W przypadku Dxx korekta automatycznie przeskakuje z D59 na D60, nie przechodzi przez nielegalne D5A–D5F.
- A przy wstawianiu efektu/parametru także ogranicza wartość do dozwolonego zakresu. Niezmienione: LIVE EDIT, POSITION LOOP, kopiowanie LSDJ, ghost, sample, karuzela, mysz (-30%), fullscreen.
- Nie przepisujemy efektów w już istniejących, załadowanych MOD-ach. Korekta dotyczy ręcznej edycji za pomocą kontrolera, nie całego pliku. Wklejenie pól FX/PARAM stosuje limit, lecz zapis innych pól w archiwalnych modułach pozostaje bez zmian.

**Uwaga:** `E8x` i `EFx` modyfikują dane sampla w pamięci podczas odtwarzania (patrz dokumentacja PT2). `F00` jest poprawną komendą STOP z dokumentacji. `F20–FFF` (32–255) służy do tempa BPM w trybie CIA. Komendy `E4x`/`E7x` dopuszczają także 4–7 mimo opisu „0–3” w nagłówku, bo warianty bez ponownego wyzwalania są legalne.

GitHub Desktop Summary: `PPT v0.3.9 - Effect names in status and safe parameter ranges`

## Poprzednie funkcje (v0.3.8)

# Pocket Pro Tracker v0.3.6 · Ghost Playhead + SONG od POSITION

SourceKit bazuje na poprzednim, potwierdzonym na urządzeniu PPT v0.3.2, z oryginalnym silnikiem ProTracker 2 Clone **v1.92**. ZIP zawiera źródła do GitHub Desktop, nie gotowy program ARM64.

## Nowość v0.3.6: Backspace pod B w karuzeli SONGNAME

- Gdy edytujesz nazwę SONGNAME, **B** usuwa znak przed kursorem, przesuwa resztę nazwy w lewo i cofa kursor.
- Na pierwszej pozycji B niczego nie usuwa. **Y** kończy edycję, zachowując aktualną nazwę.
- W pozostałych miejscach **B** nadal ma dotychczasową funkcję kopiowania pola.
- Pozostałe funkcje v0.3.5, w tym zmniejszona prędkość myszy, są zachowane.

## Zmiana v0.3.6: precyzyjniejsza mysz (−30% każdej prędkości)

- Lewy analog przesuwa mysz z szybkością **70% poprzedniej wersji** w całym zakresie wychylenia, również przy maksymalnym wychyleniu.
- Nie zmieniamy martwej strefy ani krzywej przyspieszania. Wolne, średnie i szybkie ruchy są proporcjonalnie spowolnione.
- Alternatywna mysz **R1 + D-pad**, a także D-pad myszy w oknach dialogowych, są spowolnione o dokładnie tyle samo.
- Ułamki pikseli są akumulowane, aby zachować płynny ruch przy małym wychyleniu.
- Pełny ekran, LIVE EDIT, ghost playhead, transport, sample i karuzela SONGNAME są zachowane.

## Poprzednie funkcje zachowane z v0.3.4

**MENU + START**: uruchamia odtwarzanie całego SONG od *aktualnie ustawionej pozycji POS*, zaczynając od ROW 00. Odtwarzany pattern jest pobierany z `song->header.patternTable[song->currPos]` (przez oryginalną funkcję `modPlay(DONT_SET_PATTERN, song->currPos, 0)`). Song dalej przechodzi po następnych pozycjach tabeli.

**SELECT podczas PLAY**: LIVE EDIT nadal nie przesuwa edytora za odtwarzaczem. Dodatkowo w widoku patternu będzie widoczny niezależny **szary ghost playhead**, czyli delikatniejsza, szara belka w wierszu faktycznie odtwarzanym. Wybrany ROW kursora pozostaje bez zmian.

Ghost jest rysowany jako szare tło *pod* tekstem patternu, w dwóch trybach wyświetlania ProTrackera (normal/dotted), bez nakładania przeźroczystej warstwy na fonty. Klatka odświeża się na każdy odtwarzany wiersz. Poprzednia szara belka zostaje wyczyszczona bez naruszania pionowych separatorów kanałów. Kiedy playhead znajdzie się poza widocznym oknem 15 wierszy, belka nie jest widoczna (edytor nie przewija się sam). Gdy edytujesz inny pattern niż grający, belka się nie pojawia. Kiedy oba kursory znajdą się w tym samym wierszu, pierwszeństwo ma wyróżnienie głównego kursora PT2.

Źródłem pozycji ghosta jest migawka `modPattern` + `song->row` z oryginalnej pętli audio PT2, publikowana atomowo (bez naruszania `song->currRow`, `song->currPattern`, zegara ani mixera). Pozostałe zachowania i oryginalny FULL SCREEN bez zmian.

## Sterowanie transportem

| Skrót | Funkcja |
|---|---|
| START | PLAY PATTERN od ROW 00, w czasie PLAY = STOP |
| SELECT + START | PLAY PATTERN od zaznaczonego ROW |
| MENU + START | PLAY SONG od bieżącego POSITION, ROW 00 |
| SELECT podczas odtwarzania | LIVE EDIT wł./wył., niezależny kursor i ghost |
| L1 / R1 | Wybór PATTERN −1/+1, podczas LIVE EDIT bez ruszania grającego patternu |
| L2 / R2 | Wybór aktywnego sampla |
| A | Wstaw nutę i bieżący sample, z podglądem |
| L1 + MENU | ESC i oryginalne potwierdzenie wyjścia |

## Instalacja

1. Rozpakuj SourceKit do repozytorium w GitHub Desktop, zastępując stare pliki (również w ukrytym folderze `.github`).
2. Summary: `PPT v0.3.6 - SONGNAME Backspace on B`
3. Commit to main → Push origin.
4. Po zielonej kompilacji pobierz artefakt ZIP, rozpakuj, a wewnętrzny `pocket_pro_tracker_pt2.zip` zainstaluj przez PortMaster.

## Testy

`bash tests/check.sh` uruchamia lokalne testy sterowania, idempotencji patchera i kodu ghosta. GitHub Actions dopiero zbuduje prawdziwe PT2 na ARM64. Przed użyciem na występie sprawdź na kopii MOD, czy ghost porusza się prawidłowo i nic nie zatrzymuje odtwarzania.

Song title: click SONGNAME with X, D-pad UP/DOWN cycles characters, LEFT/RIGHT navigates, X confirms/advances, Y finishes and keeps name. SAVE MODULE persists it.

### v0.3.7
SELECT+B then D-pad to expand gray selection and B to copy. SELECT+B+L1 copies channel; SELECT+B+R1 copies pattern. SELECT+A pastes saved block/channel/pattern.


## PPT v0.3.8 — POSITION LOOP
Kliknij X na POS, by otworzyć POSITION EDITOR. Góra/dół przegląda SONG order, nie zmienia przypisania patternów. SELECT+B rozpoczyna zaznaczenie pozycji, D-pad je rozszerza, B zatwierdza loop, SELECT+Y włącza/wyłącza loop. MENU+START zaczyna SONG od zaznaczonego początku, jeśli loop jest aktywny. W PATTERN EDIT pozostaje schowek LSDJ.


## PPT v0.3.20
Sampler RANGE: SHIFT+LEFT/RIGHT = beginning, MENU+LEFT/RIGHT = end. A tap changes by 1 byte; hold accelerates 1/2/5/10/50/100. Select RESAMPLE NOTE with X then choose C-1 through B-3 using D-pad UP/DOWN (semitone) or LEFT/RIGHT (octave). X/Y confirms, B cancels. Resampling is performed only by PT2 RESAMPLE.
