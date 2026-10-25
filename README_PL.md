# Pocket Pro Tracker v0.3.7 — zaznaczanie LSDJ i wspólny schowek

Źródła do repozytorium GitHub Desktop (nie gotowy port). Pełne testy ARM64 oraz fizyczna konsola wymagają weryfikacji.

## Skróty

- **SELECT + B**: rozpocznij zaznaczanie od bieżącego pola; szary prostokąt pokazuje aktualny obszar.
- **D-pad**: rozszerz zaznaczenie w górę/dół po wierszach i w lewo/prawo po polach wszystkich czterech kanałów.
- **B**: zatwierdź i kopiuj zaznaczony obszar (również jedno pole).
- **SELECT + B + L1**: natychmiast skopiuj cały bieżący kanał (64 wiersze). Kolejność naciskania L1/B przy trzymanym SELECT może być dowolna.
- **SELECT + B + R1**: natychmiast skopiuj cały pattern (4 kanały × 64 wiersze). Kolejność naciskania R1/B przy trzymanym SELECT może być dowolna.
- **SELECT + A**: wklej ostatnią kopię. Blok od wybranego ROW i pola; kanał do wybranego kanału, a pattern do wybranego patternu (od ROW 00).

W trybie LIVE EDIT można kopiować i wklejać bez przerywania odtwarzania. Operacje zapisu są otoczone blokadą audio. Osobny szary ghost playhead nie porusza kursorem ani zaznaczeniem.

**Uwaga:** prostokąt dotyczy pól NOTE, cyfr numeru sampla, komendy i cyfr parametru. Wklejanie do innych typów pól nie zmienia niezgodnych danych (np. nuta nie stanie się komendą efektu). Przy wklejaniu przy końcu patternu nadmiar wierszy/pól jest obcinany, nigdy nie zawijany. B poza zaznaczaniem nie kopiuje już pojedynczego pola. W SONGNAME B nadal działa jako Backspace.

## Instalacja

Rozpakuj do repozytorium Pocket-Pro-Tracker, podmień pliki, Commit to main → Push origin; GitHub Actions skompiluje paczkę do PortMaster. Summary: `PPT v0.3.7 - LSDJ block selection and unified clipboard`.

Bazą jest v0.3.6. Zachowano analog zwolniony o 30%, karuzelę SONGNAME, START / SELECT+START / MENU+START, LIVE EDIT, szary ghost playhead i FULLSCREENSTRETCH.

**Implementacja:** schowek PPT jest niezależny od klawiaturowego schowka oryginalnego PT2, ale kopiowanie/wklejanie ma równoważną funkcjonalność. W zaznaczeniu pojedynczego pola można kopiować oddzielnie cyfrę numeru sampla i cyfrę parametru.
