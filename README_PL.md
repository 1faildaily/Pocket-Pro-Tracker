# Pocket Pro Tracker v0.3.34 — DISK-OP na SD2

Przy każdym uruchomieniu PPT skrypt sprawdza obecność zamontowanej drugiej karty pod /mnt/sdcard (alternatywnie /sdcard). LOAD/SAVE MODULE oraz LOAD/SAVE SAMPLE startują w głównym katalogu tej karty. Potem można normalnie przeglądać katalogi, a MOD i SAMPLE zapamiętują bieżącą lokalizację w ramach sesji.

Gdy druga karta nie jest zamontowana, program wraca do swoich katalogów mods i samples. Sam pusty katalog /mnt/sdcard nie jest uznawany za kartę. Skrypt nie tworzy katalogów na SD2 i nie przenosi plików. Ustawia wyłącznie DEFAULTDIR i DEFAULTSMPDIR w protracker.ini; reszta ustawień pozostaje zachowana.

## Instalacja

Na działającej v0.3.33 wystarczy podmienić skrypt Pocket Pro Tracker PT2.sh w Ports — nie trzeba kompilacji ani zmiany pliku wykonywalnego.

Dostarczony pełny SourceKit zachowuje wszystkie funkcje v0.3.33. Można też użyć standardowego Commit → Push → artefakt ARM64 po podmianie plików razem z .github.
