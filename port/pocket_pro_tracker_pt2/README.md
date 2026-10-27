# Pocket Pro Tracker v0.3.28 - wszystkie nuty importu, E-3 domyślnie

Baza: v0.3.27. Anbernic RG35XX Pro / muOS / PortMaster.

## Import WAV - karuzela C-1...B-3
- A na WAV w LOAD SAMPLE otwiera natywne okno importu.
- Przy KAŻDYM otwarciu początkowo wybrana jest E-3 (indeks 28).
- Dostępne wszystkie 36 nut oryginalnego PT2, C-1...B-3, finetune 0.
- D-pad góra / dół: półton w górę / dół.
- D-pad lewo / prawo: oktawa w dół / górę.
- Na końcach zakresu wybór zatrzymuje się, nie zawija.
- A / przycisk LOAD: przelicz i załaduj do aktualnego slotu.
- B / CANCEL: anuluj bez zmiany zawartości sampla.
- Widoczne: nuta, rzeczywista częstotliwość PAL z tabeli okresów PT2,
  wynikowa liczba bajtów, limit i numer slotu.
- Częstotliwość = oryginalny PAL_PAULA_CLK / periodTable[nuta].
  E-3 to ok. 20864.09 Hz. Nie używamy stroju PC ani przybliżenia z równych półtonów.
- Po imporcie właściwa oryginalna prędkość WAV jest uzyskiwana na wybranej nucie.
- Zbyt długi wynik (ponad 131070 bajtów) zostaje odrzucony, bez obcinania.

## Wybór SAMPLE NUMBER podczas PLAY i importu
- W LOAD SAMPLE L2 / R2 wybierają poprzedni / następny slot 01...1F.
- Działa podczas odtwarzania i w otwartym oknie resamplingu.
- Zmiana slotu nie wysyła lewego/prawego kliknięcia i nie potwierdza/anuluje okna.
- Widoczny numer SLOT odświeża się natychmiast.
- LOAD zapisuje do slotu wybranego w momencie zatwierdzenia.
- X / Y pozostają właściwymi przyciskami myszy, A zatwierdza, B anuluje.

## Tło LOAD SAMPLE
- Dolny pattern nie jest odświeżany w LOAD SAMPLE, również podczas PLAY.
- Normalnie dolny obszar zajmuje podgląd WAV.
- Podczas wyboru nuty jest czyste tło i karuzela, bez patternu i fragmentów przebiegu.
- Natywne okno jest odrysowywane nad końcowym podglądem, po aktualizacji GUI.
- Po wyjściu z trybu sampli wymuszamy odświeżenie normalnego patternu.
- Zmiana dotyczy rysowania, nie zegara ani efektów odtwarzacza.

## Podgląd WAV zachowany
D-pad wybiera plik, B uruchamia / zatrzymuje odsłuch z prędkością źródłową.
Przebieg jest widoczny na dole DISK-OP. Podgląd używa osobnego bufora,
nie nadpisuje slotu i nie oznacza modułu jako zmienionego.
Zmiana pliku / katalogu / widoku zatrzymuje odsłuch.
Nowa ścieżka dotyczy WAV. Pozostałe formaty zachowują loader PT2 bez karuzeli.
Sprawdzone PCM 8/16/24/32-bit i float32; stereo jest mieszane do mono.
Limit pliku źródłowego to 32 MiB. Nie gwarantujemy float64 przez decoder SDL2.
Prosta pętla smpl z WAV jest przeliczana do nowej częstotliwości.
Import używa mono signed 8-bit, finetune 0, resamplingu windowed-sinc.
Kod miksera, replayera, Pauli i zapisu MOD nie zmienił się względem v0.3.27.

## Zachowane
MONO przy starcie, sample do 128 KB, pełny pattern pod R3,
A+B NOTE+sample, Y usuwa zaznaczenie, schowek, LIVE EDIT i POSITION LOOP.
MENU nie przełącza pełnego patternu; kombinacje MENU zachowane.

## Instalacja
Podmień pliki repozytorium, również .github -> Commit -> Push.
Artefakt: PPT_v0.3.28_AllNotes_E3_SampleSlots_ARM64.
W nim pocket_pro_tracker_pt2.zip do instalacji przez PortMaster.

## Sprawdzone
- Testy kontrolera, integracji i idempotencji patcha.
- Wszystkie 36 nut rzeczywiście zaimportowane przez nową ścieżkę.
- E-3 przy każdym otwarciu, ograniczenia karuzeli, pamięć nuty po imporcie.
- L2/R2 podczas PLAY oraz przy otwartym oknie, bez zdublowanych kliknięć.
- Import do zmienionego slotu, poprzedni slot pozostaje niezmieniony.
- Brak rysowania patternu pod oknem i podglądem w LOAD SAMPLE.
- Anulowanie, błędny / za długi WAV, długość i ton 440 Hz, pętle WAV.
- Pełna lokalna kompilacja x86-64; dodatkowy test ASan/UBSan.
- Kontrola rzeczywistego renderu okna i czystego tła.

Do potwierdzenia pozostają GitHub Actions ARM64 i działanie na Anbernicu.
