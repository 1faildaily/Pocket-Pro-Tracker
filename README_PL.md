# Pocket Pro Tracker v0.2.9 – natywna kompozycja obrazu 640×480

**To SourceKit do GitHub Desktop, a nie gotowa binarka dla PortMastera.**

## Co ustaliliśmy z prawdziwego `ppt_video.log`

v0.2.8 poprawnie liczy prostokąt 19,0,602,480 na ekranie 640×480. Skoro fizyczny ekran nadal miał czarne pasy również u góry i u dołu, samo dalsze poprawianie prostokąta nic nie da.

## Co zmienia v0.2.9

Zamiast ponownie polegać na końcowym skalowaniu w SDL, kod wypełnia pełną **klatkę ARGB8888 640×480** w pamięci RAM. W tej klatce renderuje obraz 320×255 w proporcjach zachowanych jako **602×480**, czarny zostaje tylko lewy i prawy margines po 19 pikseli. SDL otrzymuje już gotową teksturę **640×480**, którą wyświetla 1:1, bez drugiego prostokąta skalującego. Podczas renderowania wyłączamy SDL logical scaling i dodatkowy viewport.

Przy zachowaniu pełnego obrazu ProTrackera i wysokości 480 mamy skalę niecałkowitą (około 1,882), więc najmniejsze piksele obrazu mogą mieć różną szerokość. To jest matematyczne ograniczenie, a nie błąd. Nie ma już jednak powodu, żeby pojawiały się poziome czarne pasy generowane przez nasz kod.

**Nie zmieniamy** mapowania przycisków, obsługi okien dialogowych, myszki na lewym analogu, tempa myszki, odtwarzania, sampli ani odsłuchu nut. Jeśli mimo tego wyświetli się czarny margines u góry/dołu, potrzebne będą pomiary podsystemu ekranu muOS lub zdjęcie aktualnej wersji.

## Kompilacja

1. Rozpakuj i skopiuj całą zawartość do swojego repozytorium w GitHub Desktop, zastępując stare pliki.
2. **Summary:** `PPT v0.2.9 - Native 640x480 composed video output`.
3. **Commit to main**, następnie **Push origin**.
4. Po zielonym Actions pobierz artefakt, rozpakuj ZIP artefaktu i wewnętrzny `pocket_pro_tracker_pt2.zip` wgraj do katalogu autoinstall PortMastera.
5. Zachowaj poprzednią paczkę jako kopię zapasową.

## Jak sprawdzić

Po uruchomieniu otwórz `ports/pocket_pro_tracker_pt2/ppt_video.log`. Nowa implementacja powinna dopisać linię `PPT VIDEO v0.2.9 COMPOSED frame=640x480 rect=19,0,602,480 logical=0x0 ...`. Jeśli jej nie ma, nowa ścieżka renderowania się nie uruchomiła. Jeśli jest, ale obraz nadal ma czarne pasy u góry/dołu, wyślij także **aktualne zdjęcie ekranu**. Będzie to oznaczało, że ograniczenie znajduje się poza składaniem pikseli w PT2 (np. warstwa KMSDRM/wyświetlacz).

## Status

Przetestowano programowo kompozycję obrazu 640×480, łącznie z pierwszym i ostatnim rzędem pikseli. Przeprowadzono test struktury źródeł i nakładania poprawki na imitację kodu upstream. Pełna kompilacja ARM64 GitHub Actions oraz test fizycznego Anbernica nadal są potrzebne.
