# Pocket Pro Tracker v0.3.46 — edycja POSITION-ED

POSITION-ED ma oddzielne sterowanie: A oraz A+D-pad nie wpisują nut ani efektów w ukrytym patternie, L3 nie włącza RECORD, a klawiaturowa edycja patternu jest w tym widoku zablokowana. Lista jest przesunięta o 2 px w dół, łącznie z karuzelą nazw i obsługą myszy.

## Sterowanie POSITION-ED

| Skrót | Działanie |
|---|---|
| D-pad góra/dół | Kursor pozycji, także APPEND za końcem utworu |
| D-pad lewo/prawo | Numer patternu −1/+1; na APPEND tworzy pozycję |
| A+B lub B+A | Usuń pojedynczy wpis, przesuń resztę w górę |
| SELECT+B, D-pad, B | Zaznacz zakres, skopiuj pozycje i ustaw loop |
| MENU+A po zaznaczeniu | Przygotuj kopię do nowych pustych patternów |
| SELECT+A | Wklej przed kursorem albo na APPEND, przesuwając dalsze wpisy |
| Y / SELECT+Y | Usuń zaznaczenie i loop, zachowaj schowek |
| SELECT | Podczas odtwarzania: swobodny kursor / powrót do śledzenia |
| START podczas swobodnego przeglądania | Uruchom SONG od kursora, ROW 00 |
| START w śledzeniu / zwykłym odtwarzaniu | Stop |
| START na zatrzymanym utworze | Odtwarzaj wybrany pattern jak wcześniej |
| MENU+START na zatrzymanym utworze | SONG; jeśli jest loop, od początku loopa |

Poruszenie kursora lub rozpoczęcie zaznaczenia podczas SONG także odłącza kursor od odtwarzania. Czerwień wskazuje faktycznie odtwarzaną pozycję, biały kwadracik wskazuje swobodny kursor. Wyjście poza zaznaczony loop samym kursorem nie zmienia dźwięku. Dopiero START uruchamia utwór w wybranym miejscu; jeżeli leży ono poza starym loopem, loop jest wtedy wyłączany. Ponowny SELECT przywraca śledzenie, po czym START zatrzymuje.

Zmiana numerów patternów, usuwanie i wklejanie wymagają STOP. Nie można usunąć ostatniej pozycji ani pustego APPEND.

Usuwanie nie kasuje danych ani nazw patternów. Usunięty numer pozostaje tuż za skróconą aktywną listą w istniejącej 128-elementowej tabeli MOD. Natywny zapis uwzględnia tę tabelę przy ustalaniu liczby zapisanych patternów, więc także usunięty najwyższy pattern przetrwa SAVE/LOAD. Funkcja zapisująca MOD pozostaje bez zmian.

## Kanały i kopiowanie

SELECT+L1 = kanał 1; SELECT+L2 = kanał 2; SELECT+R1 = kanał 3; SELECT+R2 = kanał 4. To ten sam mute co oryginalne przyciski PT2, działa także podczas odtwarzania. W głównym oknie pozostają natywne oznaczenia, w POSITION-ED status pokazuje 1+/2+/3+/4+ lub czerwone X, a w dużym PATTERN-ED są wskaźniki 1–4. Mute nie zmienia nut ani efektów w źródle.

Zwykła kopia pozycji z wszystkimi aktywnymi kanałami zachowuje numery patternów. Jeśli podczas kopiowania któryś kanał jest wyciszony, schowek przygotowuje nowe patterny zawierające tylko aktywne kanały. MENU+A wymusza nowe patterny także przy wszystkich kanałach aktywnych. Najpierw zaznacz zakres; MENU+A może zatwierdzić trwające zaznaczenie albo użyć nadal zaznaczonego zakresu. Wklejanie zawsze SELECT+A.

Dla nowych patternów kopia zapisuje nuty, sample, FX i nazwy w chwili kopiowania. Zmiana mute po skopiowaniu nie zmienia zawartości schowka. Wyciszone kanały są całkiem puste w nowych kopiach. Każdy zaznaczony wpis otrzymuje osobny nowy pattern, nawet jeśli źródłowe numery się powtarzają. Cztery wpisy dają cztery nowe patterny; nie jest to stały limit liczby kopii.

Wolne miejsce oznacza pattern pusty, bez nazwy, nieużywany w tabeli pozycji ani jako źródło schowka. Zajęte i niewykorzystane, ale edytowane patterny są chronione. Brak odpowiedniej liczby wolnych patternów lub pozycji przerywa całe wklejanie bez częściowych zmian. Powtórne wklejenie nowej kopii przydziela kolejne wolne numery.

## Zgodność i sprawdzenie

Format MOD, natywna funkcja modSave, Paula i efekty odtwarzania bez zmian. W pt2_replayer.c rozszerzona jest wyłącznie istniejąca synchronizacja widoku: zapamiętanie numeru odtwarzanej pozycji i pominięcie nadpisania swobodnego kursora. Nowe uruchomienie korzysta z oryginalnych modStop/modPlay.

Testy kontrolera obejmują blokadę ukrytej edycji, obie kolejności A+B, przesunięcie listy, mute, kopie z nowymi numerami i maską kanałów, ochronę źródeł i brak częściowego wklejenia przy braku miejsca. Test natywny sprawdza niezależny kursor podczas prawdziwego przechodzenia SONG i loopa oraz SAVE/LOAD usuniętego patternu 99. Fizyczny Anbernic wymaga sprawdzenia po buildzie ARM64.

Podmień pliki wraz z .github → Commit → Push → artefakt PPT_v0.3.46_Position_Editing_ARM64.

## Zachowane funkcje poprzedniej wersji


Pod QUADRASCOPE: POCKET PRO TRACKER. Nagłówek POSITION-ED: POSITION / EDIT. Nagłówek DISK-OP: DISK / OPERATOR. Natywna czcionka i obramowania, bez zmian playera i formatu MOD.


Usunięto wyciek pikseli QUADRASCOPE przy EXIT w POSITION-ED. Przechwycenie waveformów przywraca teraz cały interfejs przed narysowaniem widoku. Poprawka wyłącznie graficzna; player i format MOD bez zmian.


SAVE MODULE po udanym natywnym zapisie tworzy dodatkowy plik tekstowy w tym samym katalogu. Do pełnej nazwy modułu dopisywane jest .patterns.txt:

- break.mod → break.mod.patterns.txt
- mod.break → mod.break.patterns.txt

Przy załadowaniu modułu nazwy są automatycznie odczytywane z pasującego pliku, także po ponownym uruchomieniu programu. Przenoś lub zmieniaj nazwę obu plików razem. SAVE AS / zapis pod inną nazwą tworzy pasujący nowy plik nazw. Bez pliku tekstowego moduł ładuje się normalnie, z pustymi nazwami patternów.

Format tekstowy (numery dziesiętne, tabulator, nazwa do 15 znaków):

# Pocket Pro Tracker pattern names v1
00<TAB>INTRO
01<TAB>BREAK 1
17<TAB>OUTRO

W samym pliku zamiast <TAB> jest znak tabulacji. Pierwsza linia jest nagłówkiem wersji. Zapisywane są tylko niepuste nazwy. Pusty zestaw nazw zapisuje sam nagłówek, dzięki czemu dawne nazwy nie wracają przy następnym ładowaniu.

Nazwy dotyczą numerów patternów, a nie konkretnych pozycji utworu. Pliki .patterns.txt są pomijane na listach MODULE i SAMPLE. Plik zapisuje się najpierw do tymczasowego pliku obok celu, a potem zastępuje poprzedni. Gdy zapis samych nazw się nie uda, MOD pozostaje zapisany i pojawia się MOD OK NAMES ERR. Błędny plik nazw nie przerywa ładowania modułu.

## Zgodność

Odtwarzacz, Paula i audio są identyczne z poprzednią wersją. Natywna funkcja modSave zapisująca bajty MOD-a także jest identyczna; dodany jest wyłącznie niezależny zapis metadanych po jej poprawnym zakończeniu oraz odczyt po załadowaniu modułu. Nic nie jest dopisywane do MOD-a.

Test integracyjny zapisuje moduł, zmienia nazwy, zapisuje ponownie i porównuje pliki MOD bajt w bajt. Przechodzi też rzeczywiste modLoad + setupLoadedMod i odzyskanie nazw 00, 17 i 99, w tym 15 znaków. Sprawdzone brakujące, nieprawidłowe i zduplikowane wpisy, puste nazwy oraz błąd zapisu. Testy kontrolera i patcha oraz ASan/UBSan przeszły.

Pozostałe funkcje v0.3.42 pozostają zachowane. Podmień wszystkie pliki wraz z .github → Commit → Push → artefakt ARM64. Sprawdzenie na fizycznym Anbernicu pozostaje do wykonania.
