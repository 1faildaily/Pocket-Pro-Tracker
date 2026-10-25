# Pocket Pro Tracker v0.2.5 – pixel-perfect, odsłuch nut, L1+MENU

To **SourceKit** do GitHub Desktop i GitHub Actions, nie gotowa paczka na konsolę. Bazuje na Twojej działającej integracji PT2 Clone 1.92.

## Zmiany

1. **Obraz:** poprawiono wymuszające zniekształcenie `FULLSCREENSTRETCH=TRUE` na `FALSE`, dodano `INTEGERSCALING=TRUE`, `PIXELFILTER=NEAREST` i `VIDEOSCALE=1X`. Oryginalny raster PT2 Clone ma 320×255. Przy ekranie 640×480 jedyna pełna skala całkowita to 1×, co może dać **duże czarne marginesy**. 2× (640×510) nie mieści całego UI. Ustawienie to traktuj jako test rzeczywistej ostrości, a nie kompromis wielkości. Zastosowanie INTEGERSCALING wymaga sprawdzenia w pełnoekranowym trybie SDL na RG35XX Pro. Jeśli ekran będzie za mały, można wrócić do poprzedniego trybu skalowania bez utraty innych zmian.
2. **Odsłuch:** wstawienie nuty przyciskiem A w polu NOTE wywołuje autentyczną ścieżkę audio `jamAndPlaceSample()` z PT2 Clone, korzystając z tej samej próbki i wysokości. Odsłuch nie przesuwa ROW, nie wpisuje nuty drugi raz i nie odtwarza niezdefiniowanego sampla. Podczas trwającego PLAY nie uruchamia dodatkowej nuty, żeby nie zakłócać utworu.
3. **L1 + M/MENU:** wywołuje oryginalną funkcję ESC/PT2 `REALLY QUIT?`. Kod fizycznego przycisku M przyjmujemy testowo jako `354`; nie ma go w `controls.json`, więc nie jest jeszcze potwierdzony na tej konsoli. muOS może przechwytywać go systemowo. **L1 + START zachowano awaryjnie** jako drogę wyjścia z potwierdzeniem na wypadek, gdyby M był niewidoczny.
4. Pozostają START jako PLAY PATTERN/STOP, SELECT+START SONG od ROW, D-pad LSDJ, mysz pod lewym analogiem 70% wcześniejszej szybkości, tryb modalny A/B i obecne mapowanie przycisków.

## Instalacja przez GitHub Desktop

1. Rozpakuj ZIP i skopiuj **zawartość** (`.github`, `patch`, `port`, `tests`, `README_PL.md`) do katalogu swojego repozytorium `Pocket-Pro-Tracker`, zastępując stare pliki.
2. GitHub Desktop → Summary: `PPT v0.2.5 - Pixel perfect, note preview and MENU exit`.
3. Commit to main → Push origin.
4. GitHub → Actions → po udanej kompilacji pobierz Artifact, rozpakuj zewnętrzne archiwum i wgraj wewnętrzny `pocket_pro_tracker_pt2.zip` do używanego katalogu `autoinstall` PortMastera.

## Test na konsoli

- Porównaj czcionki, przyciski, pionowe linie i krawędzie interfejsu. Sprawdź, czy cały UI jest widoczny; spodziewaj się **czarnych pasów** zamiast rozciągania.
- Załaduj MOD z prawdziwymi samplami. Ustaw kursor na NOTE; wstaw nutę A i sprawdź odsłuch tej samej próbki i wysokości. Samplowi o zerowej długości nie będzie towarzyszył dźwięk.
- Wciśnij **L1 + M**, najlepiej najpierw L1, potem M. Jeśli system przejmie M, użyj L1 + START jako bezpiecznej alternatywy.
- Sprawdź START/STOP, SELECT+START i okna potwierdzenia.

Testy automatyczne i kompilacja w lokalnym środowisku obejmują kod adaptera i testy mock, **nie pełną binarkę ARM64**. Pierwszą pełną kompilację PT2 i próbę na muOS wykonasz przez GitHub Actions i konsolę. W razie nieudanego builda przekaż log Actions.
