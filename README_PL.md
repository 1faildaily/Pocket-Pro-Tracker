# Pocket Pro Tracker v0.3.33 — SAMPLE BPM

Wejście do SAMPLE EDITOR otwiera na górze EDIT.OP stronę 3 (SAMPLE EDITOR).
W miejscu UPSAMP i DNSAMP są trzycyfrowe pola: BPM źródłowy i BPM docelowy. Pomiędzy nimi a VOL znajduje się DO. Pozostałe przyciski zachowują swoje położenie.

## Sterowanie

- X na pierwszym / drugim polu: edycja BPM źródłowego / docelowego.
- Góra / dół: wybrana cyfra 0–9, lewo / prawo lub X: pozycja, B: poprzednia pozycja, Y: zatwierdzenie pola.
- Klawiatura: tylko cyfry, Enter zatwierdza, Escape anuluje.
- X na DO: przeliczenie całego aktualnego sampla.
- Pola startują od 000; DO wymaga obu BPM w zakresie 001–999. BPM wpisujesz ręcznie, program go nie wykrywa.
- Po wykonaniu źródłowe BPM przyjmuje docelowe, żeby ponowne DO nie nakładało tej samej zmiany jeszcze raz.

## Działanie

Długość wynikowa = długość wejściowa × BPM źródłowy / BPM docelowy (wyrównanie do parzystych bajtów MOD).
Np. 168 → 170 skraca i przyspiesza sampel. Jest to resampling, który zmienia również wysokość dźwięku, bez zachowywania stroju jak w time stretch.

Konwersja używa filtrowanego resamplingu z przeglądarki WAV, przelicza granice pętli i zachowuje nazwę, finetune oraz głośność. Przetwarza cały sampel. Wynik powstaje w osobnym buforze przed zmianą danych. Zerowe BPM, pusty sampel lub przekroczenie limitu długości kończą się komunikatem bez zmiany sampla; nie ma przycinania ani automatycznego podziału. Natywna funkcja przywracania sampla pozostaje zachowana.

Kod playera, Pauli, audio i zapisu MOD jest identyczny z poprzednią wersją. Zmiana jest zapisywana jako zwykły 8-bitowy sampel MOD; pola BPM nie dodają danych do formatu MOD. Sama podmiana danych używa natywnego wyciszenia głosów, tak jak inne operacje SAMPLE EDITOR.

## Instalacja

Podmień wszystkie pliki razem z .github → Commit → Push → pobierz artefakt ARM64. To zmiana kodu, sama podmiana colors.ini nie wystarczy.

## Weryfikacja

Testy sterowania, nakładania i ponownego nakładania patcha, pełna kompilacja lokalna oraz testy rzeczywistego PT2 z ASan/UBSan. Przetestowane: pola numeryczne, klik DO, zmiana 168/170 w obu kierunkach, pętle, finetune/głośność/nazwa, nietknięty sąsiedni slot, odrzucanie zera i przekroczenia limitu, anulowanie i sample powyżej 64 KB. GitHub Actions ARM64 i fizyczne sterowanie wymagają potwierdzenia na Anbernicu.
