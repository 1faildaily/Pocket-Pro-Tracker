# Pocket Pro Tracker v0.3.37 — ręczny podział WAV / POSITION COPY / kontrast scopes

## WAV

Wybór nuty: góra/dół. Lewo/prawo wybiera teraz 1, 2 lub 4 części (zamiast zmiany oktawy). Domyślnie proponowana jest najmniejsza liczba części mieszcząca wynik dla E-3, do maksymalnie 4; możesz ją zmienić ręcznie. L2/R2 nadal wybiera pierwszy slot.

Okno pokazuje cały rozmiar po resamplingu oraz liczbę i długość części. Jeśli wybrana liczba slotów nie pomieści wyniku, pojawia się TRIM END i import zachowuje maksymalny początkowy fragment. Gdy nawet 4 części są za małe, zostaje zapisane dokładnie tyle początku, ile mieści się w czterech samplach. Pozostała końcówka jest pomijana. Przy ręcznym zmniejszeniu liczby części obowiązuje pojemność wybranej liczby slotów, także z widocznym TRIM END.

Części są równe i wyrównane do parzystych bajtów MOD; na końcu możliwe minimalne dopełnienie zerami. Pierwszy slot może zostać zastąpiony, pozostałe muszą być puste. Brak pustych slotów nadal przerywa całą operację bez zmian. Nazwy /1…/4 i zasady pętli pozostają: pętla jest zachowywana tylko, jeśli w całości mieści się w jednej zachowanej części. Nie są dopisywane nuty w patternie.

Sam podział nie zmienia częstotliwości próbkowania: zależy ona od wybranej nuty. Podział może służyć uzyskaniu krótszych fragmentów również z małego pliku.

## POSITION-ED

1. SELECT+B zaczyna zaznaczenie zakresu; góra/dół rozszerza zakres.
2. B zatwierdza zaznaczenie/pętlę i zapamiętuje kopię numerów patternów z tego zakresu.
3. Y lub SELECT+Y usuwa pętlę, zachowując kopię.
4. Ustaw pozycję docelową i naciśnij SELECT+A: kopia jest wstawiana PRZED bieżącą pozycją, dalsze wpisy przesuwają się w dół.

Nie są tworzone duplikaty danych patternów, tylko ich wpisy w kolejności utworu. Wklejanie wymaga zatrzymania odtwarzania. Gdy wynik przekroczyłby 128 pozycji, operacja jest odrzucana bez ucinania końca. Po wklejeniu stara pętla jest usuwana, żeby nie wskazywała przesuniętych pozycji.

## Przebiegi w dużym PATTERN

Zwiększony kontrast linii do szarości 606060 oraz porównanie kolorów RGB niezależne od dodatkowego bajtu piksela. Lokalne testy i render potwierdzają widoczne linie, zachowanie nut, granic kanałów i podświetleń. Fizyczny Anbernic nadal wymaga sprawdzenia; sam lokalny podgląd nie potwierdza przyczyny braku przebiegów na urządzeniu.

## Weryfikacja i instalacja

Testy sterowania, schowka pozycji z przesuwaniem i limitami, integracja WAV/BPM/scopes z ASan/UBSan i kompilacja lokalna przeszły. Testowane ręczne dzielenie małego WAV na 4, przycinanie dużego do 4 pełnych części, zgodność PCM, ochrona zajętych slotów. Kod playera, Pauli, audio i zapisu MOD bez zmian.

Podmień wszystkie pliki wraz z .github → Commit → Push → artefakt ARM64. Zachowano BPM sampla, SD2 jako katalog startowy i wcześniejsze funkcje.
