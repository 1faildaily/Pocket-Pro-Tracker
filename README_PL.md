# Pocket Pro Tracker v0.3.38 — pełny POSITION / małe scopes / NOTE sampla

## PATTERN

Przebiegi w dużym PATTERN mają natywne wymiary QUADRASCOPE: 40×33 piksele, pośrodku każdej kolumny. Nie są już skalowane do wysokości ekranu. Zostaje jasnoszary kolor i ochrona tekstu, kursorów oraz podświetleń.

## POSITION-ED

Lista zajmuje cały ekran poniżej nagłówka i pokazuje do 32 pozycji. D-pad góra/dół przesuwa kursor, lewo zmniejsza numer patternu o 1, prawo zwiększa o 1 (00–99). Edycja kolejności wymaga zatrzymania odtwarzania.

Pod ostatnim wpisem znajduje się APPEND. Można tam ustawić kursor i wkleić SELECT+A skopiowany zakres na końcu. Lewo/prawo na APPEND tworzy pojedynczą nową pozycję i ustawia jej numer patternu. Limit pozostaje 128 pozycji, bez usuwania końca utworu.

SELECT+B → zaznaczenie → B: zapamiętanie zakresu i pętla. Y usuwa pętlę, zachowując kopię. SELECT+A wkleja przed kursorem i przesuwa kolejne wpisy. Mysz wskazuje wiersz, kółko przewija, EXIT wraca do głównego widoku. Inne widoki pozostają dostępne prawym analogiem.

## SAMPLER NOTE

NOTE nad OFFS pokazuje nutę wybraną przy imporcie WAV, osobno dla każdego slotu (również podzielonych części). L2/R2 przełącza wyświetlaną nutę wraz z samplem. Nieznana nuta to ---. Podczas natywnego wyboru resamplingu nadal wyświetlany jest wybierany cel.

To informacja pamiętana w bieżącej sesji, bez zmiany formatu MOD: zwykły MOD nie przechowuje nuty importu WAV.

## BPM

DO nie zmienia już źródłowego BPM po konwersji. Oba wpisane pola pozostają takie, jakie ustawił użytkownik — kolejne DO ponownie stosuje ich proporcję.

## Instalacja

Zachowane: ręczne 1/2/4 części WAV i przycinanie końca, SD2 jako katalog startowy oraz wcześniejsze sterowanie. Podmień wszystkie pliki wraz z .github → Commit → Push → artefakt ARM64.

Testy sterowania, dopisywania i edycji pozycji, wklejania na końcu, integracja WAV/BPM/scopes z ASan/UBSan oraz kompilacja lokalna. Player, Paula, audio i zapis MOD bez zmian. Działanie i płynność na fizycznym Anbernicu wymagają sprawdzenia.
