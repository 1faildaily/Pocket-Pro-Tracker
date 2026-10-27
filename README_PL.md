# Pocket Pro Tracker v0.3.39 — DISK SELECT / oryginalny POSITION / czerwone scopes

## DISK-OP

SELECT przełącza MODULE ↔ SAMPLE (pary LOAD/SAVE MODULE oraz LOAD/SAVE SAMPLE). Natywna strzałka po lewej od LOAD wskazuje bieżący tryb. Odsłuch WAV zatrzymuje się przy przełączaniu; lista jest odświeżana, a kursor przewijania wraca na początek. W aktywnym oknie dialogowym lub edycji tekstu SELECT nie przełącza trybu.

## Duży PATTERN

Przebiegi nadal mają rozmiar 40×33 jak w QUADRASCOPE. Są jasnoczerwone (FF5555) i leżą na dole kolumn, z odstępem dwóch rzędów pikselowych patternu (14 px) nad paskiem STATUS. Nie zasłaniają cyfr, kursora, zaznaczeń ani szarego ROW.

## POSITION-ED

Przywrócony oryginalny interfejs: górny panel PLAY/STOP/PATTERN i INSERT/DELETE, lewa część programu oraz natywne boczne przyciski listy, w tym EXIT. Wydłużona jest tylko prawa lista pozycji: od oryginalnego początku do paska STATUS (do 31 wpisów). Dolny obszar patternu jest zasłonięty, aby nie mieszał się z wydłużoną listą.

Zachowane D-pad góra/dół, lewo/prawo zmiana numeru patternu, zaznaczanie i pętle, SELECT+A wklejanie z przesunięciem oraz APPEND na końcu. Edycja i wklejanie wymagają zatrzymania odtwarzania. Natywne PLAY/STOP działają myszą/X jak wcześniej.

## Instalacja i weryfikacja

Pozostałe funkcje v0.3.38 zachowane: NOTE importu w bieżącej sesji, stały źródłowy BPM, ręczne 1/2/4 części WAV z przycinaniem i SD2 na starcie. Player, Paula, audio i zapis MOD pozostają bez zmian.

Podmień wszystkie pliki wraz z .github → Commit → Push → artefakt ARM64.

Przeszły testy sterowania i patcha, integracja natywnego PT2 z ASan/UBSan oraz kompilacja lokalna. Sprawdzane przełączenie trybu DISK, flagi natywnej strzałki, zachowanie PLAY/STOP w POSITION, blokada ukrytego patternu i położenie/kolor przebiegów. Sterowanie i płynność na fizycznej konsoli wymagają sprawdzenia.
