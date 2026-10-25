# Pocket Pro Tracker v0.3.5 · Ghost Playhead + SONG od POSITION

SourceKit bazuje na poprzednim, potwierdzonym na urządzeniu PPT v0.3.2, z oryginalnym silnikiem ProTracker 2 Clone **v1.92**. ZIP zawiera źródła do GitHub Desktop, nie gotowy program ARM64.

## Zmiana v0.3.5: precyzyjniejsza mysz (−30% każdej prędkości)

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
2. Summary: `PPT v0.3.5 - All mouse speeds 30 percent slower`
3. Commit to main → Push origin.
4. Po zielonej kompilacji pobierz artefakt ZIP, rozpakuj, a wewnętrzny `pocket_pro_tracker_pt2.zip` zainstaluj przez PortMaster.

## Testy

`bash tests/check.sh` uruchamia lokalne testy sterowania, idempotencji patchera i kodu ghosta. GitHub Actions dopiero zbuduje prawdziwe PT2 na ARM64. Przed użyciem na występie sprawdź na kopii MOD, czy ghost porusza się prawidłowo i nic nie zatrzymuje odtwarzania.

Song title: click SONGNAME with X, D-pad UP/DOWN cycles characters, LEFT/RIGHT navigates, X confirms/advances, Y finishes and keeps name. SAVE MODULE persists it.
