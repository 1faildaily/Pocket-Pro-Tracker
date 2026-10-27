# Pocket Pro Tracker v0.3.27 - WAV preview + import C-3/B-3

Baza: v0.3.26 (Anbernic RG35XX Pro / muOS / PortMaster).

## LOAD SAMPLE w DISK-OP
1. D-pad wybiera plik. Katalogi nadal otwierasz A, '..' wraca wyżej.
2. Dla wskazanego WAV na dole, w miejscu patternu, pojawia się przebieg,
   nazwa pliku, częstotliwość źródłowa i czas trwania.
3. B uruchamia odsłuch z oryginalną prędkością. Ponowne B go zatrzymuje.
   Zmiana pliku / katalogu / widoku zatrzymuje odsłuch.
4. A na WAV otwiera wybór importu, nie nadpisuje jeszcze aktywnego sampla:
   C-3 = PAL_PAULA_CLK / 214 = ok. 16574.28 Hz
   B-3 = PAL_PAULA_CLK / 113 = ok. 31388.45 Hz
   Wartości liczone są z tego samego zegara PAL i tabeli okresów co PT2.
5. D-pad lewo/góra wybiera C-3; prawo/dół B-3. A zatwierdza, B anuluje.
   Można też kliknąć przycisk X / myszą w natywnym oknie.
6. Dopiero zatwierdzenie przelicza WAV do mono 8-bit i zapisuje go w aktywnym
   slocie. Długość i wysokość pozostają zgodne z WAV, gdy grasz wybraną nutą
   (C-3 lub B-3), finetune 0. Ta nuta zostaje także nutą odsłuchu / wstawiania.

W oknie widzisz wynikową liczbę bajtów dla obu nut i aktualny limit.
Jeżeli wynik przekracza limit (131070 bajtów w naszej konfiguracji), import
zostaje odrzucony z komunikatem. Dotychczasowy sample pozostaje bez zmian.
Nie ma obcinania ani automatycznego podziału na sloty.

## Zakres i dane
- Nowy podgląd / wybór częstotliwości dotyczy WAV w LOAD SAMPLE.
- Sprawdzone: PCM 8/16/24/32-bit, float32, stereo -> mono.
- Decoder WAV: SDL2; format nieobsługiwany przez SDL2 zgłasza błąd i nie
  zmienia slotu. Nie obiecujemy obsługi float64 tą nową ścieżką.
- Pozostałe formaty (IFF/RAW/AIFF/FLAC) zachowują dotychczasowy import PT2,
  bez nowego podglądu i bez pytania C-3/B-3.
- Podgląd korzysta z osobnego bufora: nie zmienia sampla, patternu ani
  znacznika modyfikacji modułu. Dopuszczalny plik źródłowy: do 32 MiB.
- Pętle WAV smpl (pierwsza prosta pętla forward) są przeliczane do nowego
  zakresu i parzystych offsetów MOD. Nazwa sampla pochodzi z nazwy pliku.
- Resampling to filtr dolnoprzepustowy windowed-sinc, nie sama zmiana
  deklarowanej częstotliwości pliku ani usuwanie co drugiej próbki.
- Podgląd mono jest dodawany wyłącznie na wyjściu urządzenia SDL, poza
  odtwarzaczem MOD. Nie trafia do eksportów MOD2WAV/PAT2SMP.
- Bez aktywnego podglądu dodatkowa funkcja nie modyfikuje bufora audio.
- Replayer, Paula, oryginalny loader WAV i zapis MOD pozostają bez zmian.
  Zmieniony jest callback wyjścia audio (wywołanie podglądu), DISK-OP
  (nowa ścieżka importu WAV) i natywne okno wyboru.

## Zachowane funkcje
R3 przełącza pełny pattern; MENU nie przełącza widoku.
DISK-OP A otwiera zaznaczenie, X pozostaje lewym kliknięciem myszy.
32 wiersze, natywny zoom ROW, bitmapowy kursor i oryginalna mysz PT2.
MONO przy starcie, sample do 128 KB, A+B usuwa NOTE wraz z numerem sampla,
Y usuwa aktywne zaznaczenie bez nadpisywania schowka, LIVE EDIT i POSITION LOOP.

## Instalacja
Zastąp pliki w repozytorium, także .github -> Commit -> Push.
Artefakt: PPT_v0.3.27_WAV_Preview_Import_ARM64.
W nim pocket_pro_tracker_pt2.zip do instalacji przez PortMaster.
Zachowaj nowe protracker.ini i oryginalne colors.ini XCOPY.

## Weryfikacja
- Testy kontrolera, integracji i ponownego nakładania patcha.
- C-3/B-3: zachowanie długości i tonu 440 Hz, parzyste rozmiary, filtr
  przeciw aliasingowi i wykrywanie przekroczenia limitu.
- Test na rzeczywistym kodzie PT2: preview i anulowanie bez zmiany slotu,
  prędkość odsłuchu, import obu nut, pętle WAV, błędne i zbyt długie pliki.
- Testy importu obejmują stereo16, mono8/24/32 i float32.
- Lokalna pełna kompilacja x86-64 i dodatkowa kontrola ASan/UBSan.
- Rzeczywisty render przebiegu i natywnego okna wyboru.
- GitHub Actions uruchamia również test importu/preview na ARM64 przed budowaniem.

Pozostaje potwierdzić kompilację ARM64 w Actions i odsłuch na Anbernicu.
