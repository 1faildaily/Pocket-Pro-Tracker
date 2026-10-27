# Pocket Pro Tracker v0.3.30 — SONGNAME / SAVE / L3

- X na SONGNAME w pełnym widoku MOD otwiera edycję nazwy w dolnej belce.
- D-pad zmienia znak / pozycję, X przechodzi do następnego znaku, B cofa, Y kończy edycję.
- Pełna lista pozostaje podczas edycji i potwierdzenia zapisu; natywne okno potwierdzenia jest wyświetlane nad listą.
- L3: dodano alternatywny kod evdev 313, zachowano standardowy 317. Fizyczny klik wymaga sprawdzenia na urządzeniu; 313 jest wariantem do weryfikacji, nie pomiarem z tej konsoli.
- LOAD SAMPLE, audio i format zapisu MOD bez zmian względem 0.3.29.

Wgraj wszystkie pliki, również .github, Commit / Push i pobierz artefakt ARM64.

---
Poprzednie funkcje:

# Pocket Pro Tracker v0.3.30 - EDITSKIP 16 / L3 RECORD / pełna lista MOD

Baza: v0.3.28. Anbernic RG35XX Pro / muOS / PortMaster.

## 1. EDITSKIP 0...16 i przycisk A
- SELECT + lewo/prawo zmienia EDITSKIP w zakresie 0...16.
- Zwykłe A w polu NOTE wpisuje zapamiętaną nutę i bieżący numer sampla,
  a po puszczeniu A przesuwa kursor o EDITSKIP wierszy.
- Kolejne A wpisuje tę samą nutę w następnej pozycji.
- Przykład dla kroku 4: ROW 00 -> 04 -> 08 -> 12.
- EDITSKIP 0 pozostawia kursor w miejscu; przejście za ROW 63 zawija do 00.
- A + kierunek nadal zmienia pole w miejscu, bez dodatkowego skoku.
- A+B usuwa w miejscu, bez dodatkowego skoku.
- Wstawianie parametrów i numeru sampla pod A nie zyskuje skoku NOTE.
- Oryginalny wskaźnik kroku pokazuje dwie cyfry; obie flagi M zachowane.
- SELECT + góra/dół nadal nawiguje według EDITSKIP (minimum 1 dla kroku 0).

## 2. Klik lewego analoga L3 - natywny RECORD
- L3 inicjuje RECORD bieżącego patternu od ROW 00, tak jak natywne RECORD / prawy SHIFT PT2.
- Ponowne L3 zatrzymuje nagrywanie i odtwarzanie. START również zatrzymuje.
- A podczas RECORD wpisuje zapamiętaną nutę i bieżący sample na wybranym kanale.
- W tym trybie wiersz prowadzi odtwarzacz; EDITSKIP nie przeskakuje ręcznie.
- Używany jest oryginalny jamAndPlaceSample(), jego kwantyzacja i odsłuch.
- RECORD jest uruchamiany pełną natywną inicjalizacją modPlay(), nie przez
  zmianę MODE_PLAY -> MODE_RECORD na działającym odtwarzaczu.
- LIVE EDIT nie zamraża kursora podczas RECORD. Pełny pattern pokazuje REC.
- L3 z DISK-OP / POSITION / SAMPLER przechodzi do patternu i uruchamia RECORD.
- Okna potwierdzeń i edycja tekstu zachowują pierwszeństwo.
- Mapowanie L3: Linux BTN_THUMBL (317) oraz alternatywny kod 313 do weryfikacji na konsoli.

## 3. LOAD MODULE / SAVE MODULE - lista na pełną wysokość
- W trybie MOD lista ma 32 pozycje na ekranie, od nagłówka do dolnego paska.
- Zachowane natywne przyciski LOAD MODULE / SAVE MODULE i obsługa zapisu.
- D-pad przewija wybór, A aktywuje pozycję, X pozostaje lewym kliknięciem myszy.
- Kliknięcia w rozszerzonej części listy i przyciski przewijania mają poprawione obszary trafienia.
- Przewijanie klawiaturą, myszą i kontrolerem korzysta z nowego rozmiaru strony.
- Na dole widoczne SONGNAME i STATUS, po prawej przewijanie oraz EXIT.
- X na SONGNAME edytuje nazwę w dolnej belce. Pełna lista pozostaje także podczas potwierdzeń.
- Struktura pliku MOD i algorytm SAVE MODULE nie są zmieniane.

## LOAD SAMPLE pozostaje jak w v0.3.28
- Nadal 10 pozycji na liście i podgląd WAV na dole.
- B odsłuchuje WAV bez zmiany slotu, A otwiera import C-1...B-3, domyślnie E-3.
- L2/R2 zmienia slot również podczas PLAY i w oknie importu.
- Brak animowanego patternu pod podglądem i oknem.
- MONO, sample do 128 KB, karuzela, resampling, schowek i POSITION LOOP zachowane.

## Instalacja
Podmień pliki repozytorium, także .github -> Commit -> Push.
Artefakt: PPT_v0.3.30_EditSkip_Record_FullModList_ARM64.
W nim pocket_pro_tracker_pt2.zip do instalacji przez PortMaster.

## Weryfikacja
- Testy kontrolera: krok do 16, powtarzanie A, zawijanie, A+kierunek i A+B bez skoku.
- L3 inicjuje RECORD, A trafia do natywnej funkcji nagrywania, ponowne L3 zatrzymuje.
- Test na rzeczywistym kodzie PT2: wpis nuty do bieżącego ROW, kwantyzacja do ROW 08,
  zachowanie efektów i brak ręcznego EDITSKIP w RECORD.
- MOD lista: 32 pozycje, granice przewijania i trafienia myszy w dolne pozycje,
  SAVE MODULE, EXIT i przewijanie. LOAD SAMPLE nadal ma stronę 10 pozycji.
- Regresja podglądu/importu WAV dla wszystkich 36 nut i domyślnego E-3.
- Pełna lokalna kompilacja x86-64, ASan/UBSan i kontrola rzeczywistego renderu.
- Replayer, Paula, mikser, zapis MOD i moduł importu WAV bez zmian względem v0.3.28.

Do potwierdzenia: GitHub Actions ARM64 i fizyczny Anbernic, szczególnie kod kliku L3.
Przed nagrywaniem na ważnym utworze użyj kopii MOD-a.
