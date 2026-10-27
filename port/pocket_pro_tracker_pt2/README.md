# Pocket Pro Tracker v0.3.45 — napisy interfejsu

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
