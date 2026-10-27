# Pocket Pro Tracker v0.3.36 — QUADRASCOPE w dużym PATTERN

Tylko pełnoekranowy PATTERN EDIT (R3) pokazuje w tle każdej kolumny przebieg odpowiadającego jej kanału. Ciemnoszare linie używają istniejącego renderera QUADRASCOPE; nie ma nowych odczytów danych PCM ani zmian odtwarzania.

Przebiegi są rysowane wyłącznie na pikselach tła, dzięki czemu nuty, cyfry, ramki, zaznaczenia, ROW odtwarzania i kursor zachowują swoje kolory. Nieaktywne i wyciszone kanały nie pokazują przebiegu. Po opuszczeniu dużego PATTERN żaden inny widok nie otrzymuje tej nakładki.

Wszystkie funkcje v0.3.35 pozostają zachowane: import WAV na 1/2/4 sloty, BPM sampla, DISK-OP na SD2 i pozostałe sterowanie. Pliki playera, Pauli, audio, oscyloskopów oraz zapisu MOD pozostały identyczne.

Podmień wszystkie pliki wraz z .github → Commit → Push → pobierz artefakt ARM64.

Weryfikacja: testy sterowania i patcha, kompilacja lokalna, integracja rzeczywistego PT2 z ASan/UBSan, kontrola granic kolumn, kanału wyciszonego, zachowania tekstu i podświetleń, niezmienionego stanu scopes oraz przywracania framebufferu. Sprawdzono render podglądu. Kompilacja ARM64 i płynność na fizycznej konsoli wymagają potwierdzenia na urządzeniu.
