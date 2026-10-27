# Pocket Pro Tracker v0.3.26 - R3 / native pattern graphics / DISK-OP A

Baza: v0.3.25. Zachowane MONO, sample 128 KB i wszystkie funkcje edytora.

## DISK-OP
- D-pad góra/dół wybiera pozycję na liście i przewija katalog.
- A otwiera zaznaczony katalog lub ładuje plik. A na '..' wraca wyżej.
- X ponownie działa jako zwykły lewy przycisk myszy, Y jako prawy.
- R1+D-pad nadal przesuwa wskaźnik.
- Belka wyboru jest jednolita; usunięto czerwoną linię pod nią.
- Pytanie o niezapisany moduł korzysta z natywnego PT2.

## Pełny PATTERN EDIT
- Klik R3 (wciśnięcie prawego analoga) przełącza pełny / poprzedni widok.
- Samo MENU nie otwiera ani nie zamyka pełnego widoku.
- MENU+START (SONG), MENU+strzałki (SAMPLER RANGE), L1+MENU zachowane.
- Nadal 32 widoczne wiersze, ale bieżący ROW jest wyższy, jak w oryginalnym PT2.
- Powiększone glify bieżącego ROW pochodzą z oryginalnych funkcji PT2.
- Ramka kursora to oryginalny bitmapowy znacznik (kolor z palety XCOPY).
- Separator i wcięcia aktywnego ROW skopiowane z oryginalnego trackerFrameBMP,
  nie są przybliżeniem za pomocą płaskich linii.
- Mysz to oryginalny mousePointerBMP, usunięto zastępczy krzyżyk.
- Trafianie X w pola uwzględnia powiększoną wysokość aktywnego ROW.
- Pozostałe skróty edycji, zaznaczenie, schowek i ghost zachowane.
- Okna potwierdzeń, edycja nazwy i RESAMPLE NOTE mają pierwszeństwo.

## R3: mapowanie do sprawdzenia na konsoli
Standardowy Linux BTN_THUMBR (318) oraz firmware'owy BTN_MODE (316)
są rozpoznawane jako R3. Usunięto BTN_MODE z aliasów MENU.
Dotychczasowy fizyczny MENU: 312 / 354 / KEY_MENU, bez przełączania widoku.
Starszy log użytkownika nie zawierał potwierdzonego kodu R3.
Jeżeli klik prawego analoga nie zadziała, potrzebny będzie nowy ppt_input.log
po kilku naciśnięciach samego prawego analoga; wtedy dopasujemy kod.
Nie zmieniaj na ślepo mapy barków L1/L2/R1/R2.

## Instalacja
Zamknij PPT. Zastąp w repozytorium pliki wraz z .github -> Commit -> Push.
Artefakt: PPT_v0.3.26_R3_NativePattern_DiskA_ARM64.
Zainstaluj zawarty pocket_pro_tracker_pt2.zip.
Ustawienia startowe: STEREOSEPARATION=0, 64K_LIMIT=FALSE, fullscreen stretch,
oryginalna dostarczona paleta colors.ini. Nie podmieniono palety.

## Sprawdzone
- Testy kontrolera: A aktywuje listę, X wysyła kliknięcie myszy.
- R3 przełącza raz na wciśnięcie, automatyczne powtórzenia nie przełączają ponownie.
- Samo MENU i kombinacje z MENU nie przełączają pełnego widoku.
- Standardowy i dodatkowy kod R3 są oddzielone od kodów MENU.
- Kompilacja pełnego PT2 v1.92 + patch na x86-64.
- Render fontami i bitmapami PT2: 32 wiersze, powiększenie ROW, ramka,
  normalna mysz, ROW 63 i oryginalne separatory.
- Przywracanie framebufferu po nakładce i granice nawigacji DISK-OP.
- Kod audio, Pauli, replayera i zapisu MOD bez zmian względem v0.3.25.

Do sprawdzenia: GitHub Actions ARM64 i fizyczny Anbernic (w szczególności kod R3).
