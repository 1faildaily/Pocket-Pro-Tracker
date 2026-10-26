# Pocket Pro Tracker v0.3.15 — RIGHT STICK QUICK VIEWS + R1 MOUSE BOOST

**Baza: v0.3.14**, z paletą XCOPY RED i bez czterech kanałowych VU-meterów. Pozostają wszystkie skróty odtwarzania, LIVE EDIT, ghost, POSITION LOOP, LSDJ clipboard, EFFECT GUIDE oraz karuzele wyłącznie dla MOD NAME i SAMPLE NAME.

## Prawy drążek: cztery ekrany

| Wychylenie prawego analoga | Otwierany widok |
|---|---|
| Maksymalnie w lewo | POSITION-ED (POS) |
| Maksymalnie w prawo | EDIT PATTERN, główny ekran patternu |
| Maksymalnie do góry | DISK-OP |
| Maksymalnie w dół | SAMPLER |

- Przełączenie następuje dopiero przy **ponad ok. 91% wychylenia**, nie przy niewielkim poruszeniu gałką.
- **Tylko raz na wychylenie**: puść drążek w pobliżu środka, żeby uzbroić następny skrót. Skos pod kątem 45° nie przełącza ekranu.
- Ruch nie uruchamia ani nie zatrzymuje MOD-a; nie zmienia pozycji ROW, pozycji SONG ani ustawień sampla.
- Nie przełączaj ekranu podczas edycji nazwy ani w modalnym pytaniu PT2, wtedy prawy drążek pozostaje ignorowany.
- Osi **4 / 5** użyto według wcześniejszych logów muOS (prawy drążek: poziom / pion). Jeśli rzeczywiste osie działają odwrotnie, dołącz `ppt_input.log` z nowego testu.

## Mysz

- **R1 + D-pad przesuwa kursor myszy 2× szybciej niż w v0.3.14.**
- Lewy analog oraz jego precyzyjna krzywa/prędkość nie uległy zmianie.
- Zwykły D-pad i przyciski funkcyjne nie zmieniły działania. W oknach potwierdzenia mysz D-padem zachowuje poprzednią prędkość.

## Kompilacja

Rozpakuj SourceKit i zastąp jego zawartością repozytorium w GitHub Desktop.

Summary: `PPT v0.3.15 - Right stick quick views and faster R1 mouse`

**Commit to main → Push origin**. Po poprawnym GitHub Actions pobierz `PPT_v0.3.15_RightStickScreens_FastR1Mouse_ARM64` i zainstaluj wewnętrzne `pocket_pro_tracker_pt2.zip` przez PortMaster.

Testy lokalne (`bash tests/check.sh`) obejmują logikę wychyleń, zatrzask przeciw powtórkom, wykluczenie przekątnych, bezpieczeństwo okien dialogowych, dwukrotną prędkość myszy R1+D-pad oraz ponowne nałożenie pozostałych poprawek na przykładowe źródła. Pełna kompilacja ARM64 i test na urządzeniu pozostają do wykonania.

---

# Pocket Pro Tracker v0.3.14 — XCOPY RED bez VU-meterów na kanałach

Baza: **v0.3.13**, zachowane wszystkie skróty, LIVE EDIT, ghost playhead,
POSITION LOOP, kopiowanie LSDJ, ograniczenie prędkości myszy, MOD NAME i
SAMPLE NAME, EFFECT GUIDE i oryginalna paleta XCOPY RED (`colors.ini`).

**Jedyna zmiana wizualna:** wyłączone pionowe, animowane VU-metry przy
czterech kanałach patternu. Spektrum i oscyloskopy w górnym wizualizatorze,
wszystkie elementy patternu oraz silnik odtwarzania audio są bez zmian.

Technicznie nakładka na `src/pt2_visuals.c` zamienia wyłącznie wywołanie
`renderVuMeters()` wewnątrz `renderSprites()` na
`fillToVuMetersBgBuffer()`; zachowujemy bufor tła potrzebny do
bezpiecznego odrysowywania interfejsu. Funkcja renderVuMeters
pozostaje w kodzie upstream, a audio nadal liczy poziomy jak dotychczas.
Sekcja `[VU-meter]` w colors.ini pozostaje nietknięta; nie tworzymy
nieprawidłowego pliku palety.

Instalacja: rozpakuj SourceKit i skopiuj pliki do repozytorium GitHub Desktop,
zastępując starszą wersję. Commit Summary:
`PPT v0.3.14 - Hide channel VU meters, keep XCOPY palette`
Commit to main → Push origin → GitHub Actions. Pobierz artefakt
`PPT_v0.3.14_XCOPY_No_Channel_VU_ARM64`, rozpakuj wewnętrzny
`pocket_pro_tracker_pt2.zip` i zainstaluj w muOS/PortMaster.

Lokalny test `bash tests/check.sh` obejmuje kontroler, paletę, podwójne
nakładanie poprawek i obecność wyłącznika VU w źródłach; pełny build ARM64
oraz test fizyczny na Anbernicu są po stronie GitHub Actions/urządzenia.

---

# Pocket Pro Tracker v0.3.13 — PALETA XCOPY (colors.ini)

**Baza: v0.3.12 bez zmian w kodzie sterowania, audio i interfejsie.**

Zmiana: dodano niezmodyfikowany plik `colors(2).ini` użytkownika jako
`port/pocket_pro_tracker_pt2/colors.ini`. Jest kopiowany automatycznie
podczas GitHub Actions do gotowego pakietu PortMaster.

- Paleta interfejsu: 8 barw (czerwienie, czerń, biel, turkusowy kursor).
- VU-meter: 48 barw.
- SpectrumAnalyzer: 36 barw.
- PT2 1.92 ładuje `colors.ini` z bieżącego katalogu przed sprawdzeniem lokalizacji HOME.
- W launcherze już istnieje `cd "$GAMEDIR"`, więc plik w katalogu programu
  zostanie znaleziony automatycznie po uruchomieniu.
- `tests/check_palette.py` sprawdza strukturę pliku, a komenda `cmp` w workflow
  sprawdza, że gotowy ZIP zawiera identyczny plik.

### Instalacja przez GitHub Desktop

Skopiuj zawartość SourceKitu do repozytorium i zastąp stare pliki.
Summary: `PPT v0.3.13 - XCOPY custom palette bundled`
Commit to main → Push origin → GitHub Actions → pobierz artefakt
`PPT_v0.3.13_XCOPY_Palette_PortMaster_ARM64` → zainstaluj wewnętrzny
`pocket_pro_tracker_pt2.zip` przez PortMaster.

### Alternatywa bez rekompilacji

PT2 wczytuje paletę z pliku konfiguracyjnego: wystarczy skopiować
`colors.ini` do katalogu `ports/pocket_pro_tracker_pt2/` i zrestartować aplikację.
Osobny plik ZIP z nakładką kolorystyczną jest dołączony do odpowiedzi.

**Uwaga:** w Twojej palecie `[Palette]` kolor tekstu patternu to `000`
(czarny). Zachowałem go dokładnie tak, jak w pliku. Jeśli napisy będą
zbyt ciemne, zmień ostatnią wartość w tej sekcji na jaśniejszą (np. `FFF`).

**Testy:** lokalne testy kodu PPT i walidacji palety; nie wykonano
natywnej kompilacji ARM64, ponieważ zwykły GitHub Actions runner ARM64
jest w Twoim workflow.

---

# Pocket Pro Tracker v0.3.12 — KARUZELA TYLKO MOD NAME I SAMPLE NAME

**Baza: v0.3.10**, bez uniwersalnej karuzeli z v0.3.11. Ta poprawka
rozszerza istniejącą karuzelę MOD NAME na **SAMPLE NAME**, pozostawiając
inne pola tekstowe i liczbowe na natywnej obsłudze oryginalnego PT2.

- Kliknij **X** w nazwę MOD-a albo bieżącego sampla, aby wejść do edycji.
- **D-pad ↑/↓**: zmień literę/cyfrę; **←/→**: zmień pozycję.
- **X**: zatwierdź znak i przejdź dalej, **B**: Backspace,
  **Y**: zakończ i zachowaj nazwę w pamięci edytora.
- MOD NAME: do **20 znaków**; SAMPLE NAME: do **22 znaków**.
- POSITION, nazwy patternów, DISK OP, pola liczbowe i pozostałe pola
  **nie otrzymują karuzeli**. Wszystkie funkcje, sterowanie i ekran
  z v0.3.10 pozostają bez zmian.
- **SAVE MODULE** nadal wymagane, aby utrwalić nazwy w pliku MOD.

**GitHub Desktop Summary:** `PPT v0.3.12 - Carousel only for MOD and SAMPLE names`

**Testy:** lokalne testy obsługi obu nazw, Backspace, długości buforów,
wyłączenia w innych polach i patchera. Wymagany jeszcze build ARM64 w GitHub
Actions oraz sprawdzenie na konsoli.

---

# Pocket Pro Tracker v0.3.10 — POSITION PLAYBACK FOLLOW + RESET LOOP

Zmiany względem v0.3.9:

- Po otwarciu POSITION EDITOR i włączeniu SONG lub PATTERN (START) oryginalna synchronizacja PT2 przesuwa **POSITION, PATTERN i ROW** wraz z muzyką. Wyeliminowano zablokowanie wyświetlania przez `ppt_position_browse_active()`. W samym PATTERN EDITOR podczas LIVE EDIT kursor nadal pozostaje niezależny od odtwarzania.
- **SELECT + Y w POSITION EDITOR czyści pętlę w całości**: usuwa zakres z pamięci, wyłącza przekierowywanie AUDIO i znika całe szare zaznaczenie. Bez automatycznego ponownego włączenia starego zakresu przy ponownym SELECT+Y. Można zaznaczyć od nowa SELECT+B, D-pad, B.
- Zaznaczanie pętli podczas odtwarzania przechowuje własny koniec zakresu, niezależny od zmieniającego się PLAYHEAD. Po zatwierdzeniu B zakres odpowiada użyciu D-pada, nie temu, na jaki POS w międzyczasie przeszła muzyka. W czasie trwania zaznaczania i odtwarzania widok POSITION podąża za utworem; dostępny fragment zaznaczenia widać na szaro. W STOP zaznaczanie nadal przewija listę D-padem.
- Wszystkie funkcje v0.3.9 zachowane: EFFECT GUIDE i limity parametrów, LIVE EDIT, ghost, BLOCK CLIPBOARD, SONGNAME, kontroler, mysz, fullscreen.

**GitHub Desktop Summary:** `PPT v0.3.10 - Restore POSITION playback follow and clear loop selection`

## Test

1. Otwórz POSITION EDITOR, uruchom MENU+START. POSITION i widoczny PATTERN/ROW powinny się aktualizować zgodnie z odtwarzaniem tak jak w PT2.
2. Wybierz 4 pozycje SELECT+B, D-pad, B. Pętla powinna odtwarzać wybrany zakres, a w POSITION EDITOR podświetlenie pozostawać widoczne.
3. Wciśnij SELECT+Y. Cały szary zakres musi zniknąć i muzyka po końcu zakresu ma przejść dalej bez zapętlania.
4. Ponownie zaznacz inny zakres. Wyjdź z POSITION EDITOR, użyj LIVE EDIT. Podstawowy niezależny kursor w patternie nadal musi działać.

**Zakres testowania:** lokalne testy kodu, patchowania i emulowanej logiki kontrolera; pełna kompilacja na GitHub Actions ARM64 oraz fizyczny test Anbernica nie były wykonane.

---

# Pocket Pro Tracker v0.3.9 — EFEKTY: STATUS + BEZPIECZNE ZAKRESY

Zmiany względem v0.3.8:
- Gdy kursor znajduje się w FX lub parametrze efektu, pasek STATUS wyświetla krótką nazwę komendy ProTrackera (`C SET VOLUME`, `D PATTERN BREAK`, `E4 VIBRATO WAVE` itd.). STATUS zmienia się wraz z kursorem/efektem, również gdy edytujesz istniejący MOD.
- D-pad przy edycji parametru przestrzega zakresów: Bxx 00–7F; Cxx 00–40 (HEX); Dxx 00–63 (CYFRY DZIESIĘTNE zapisane jako BCD); E3 0/1, E4/E7 0–7 (0–3 normalnie, 4–7 bez restartu fali), E8 0–F (wszystkie działają identycznie); E0 0–F (0=on, 1–F=off); pozostałe Ex 0–F. Pozostałe komendy przyjmują pełny zakres 00–FF.
- D-pad ↑/↓ zmienia zaznaczony parametr dodatnio/ujemnie, a strzałki ←/→ pozwalają regulować o 1. W przypadku Dxx korekta automatycznie przeskakuje z D59 na D60, nie przechodzi przez nielegalne D5A–D5F.
- A przy wstawianiu efektu/parametru także ogranicza wartość do dozwolonego zakresu. Niezmienione: LIVE EDIT, POSITION LOOP, kopiowanie LSDJ, ghost, sample, karuzela, mysz (-30%), fullscreen.
- Nie przepisujemy efektów w już istniejących, załadowanych MOD-ach. Korekta dotyczy ręcznej edycji za pomocą kontrolera, nie całego pliku. Wklejenie pól FX/PARAM stosuje limit, lecz zapis innych pól w archiwalnych modułach pozostaje bez zmian.

**Uwaga:** `E8x` i `EFx` modyfikują dane sampla w pamięci podczas odtwarzania (patrz dokumentacja PT2). `F00` jest poprawną komendą STOP z dokumentacji. `F20–FFF` (32–255) służy do tempa BPM w trybie CIA. Komendy `E4x`/`E7x` dopuszczają także 4–7 mimo opisu „0–3” w nagłówku, bo warianty bez ponownego wyzwalania są legalne.

GitHub Desktop Summary: `PPT v0.3.9 - Effect names in status and safe parameter ranges`

## Poprzednie funkcje (v0.3.8)

# Pocket Pro Tracker v0.3.8 — POSITION SELECT + POSITION LOOP

**SourceKit do GitHub Desktop**, nie gotowy plik wykonywalny ARM64. Bazuje na działającym v0.3.7 z zaznaczaniem LSDJ. Pozostałe skróty, ghost LIVE EDIT i prędkość myszy pozostają bez zmian.

## Jak używać POSITION SELECT

1. Kliknij **X** w pole **POS** w oryginalnym interfejsie PT2, aby otworzyć jego **POSITION EDITOR** (górna prawa część ekranu).
2. Kiedy okno POSITION EDITOR jest otwarte, **D-pad GÓRA/DÓŁ** wybiera poprzednią/następną **pozycję SONG**. Nie edytuje numerów patternów w tablicy utworu i nie zmienia odtwarzania podczas przeglądania. Lewo/prawo są w tym trybie zarezerwowane i nie modyfikują patternu.
3. Ustaw np. **POS 08** i naciśnij **SELECT + B**: rozpocznie się zaznaczanie od tej pozycji, widoczne na szaro.
4. D-padem dojdź do **POS 11**. Zaznaczony zostanie zakres 08, 09, 10, 11, łącznie cztery pozycje utworu. Naciśnij **B**, aby zatwierdzić i **włączyć pętlę**.
5. **MENU + START (PLAY)** odtworzy SONG od pierwszej pozycji aktywnej pętli, czyli od POS 08 / ROW 00. Przy końcu POS 11 wróci do 08.
6. **SELECT + Y** w POSITION EDITOR kasuje pętlę oraz całe szare zaznaczenie. Aby zapętlić ponownie, zaznacz nowy zakres.
7. Ponownie kliknij **POS** lub **EXIT** w oryginalnym edytorze, żeby wrócić do PATTERN EDIT. D-pad znowu pracuje na ROW i polach nut.

## Rozróżnienie POS i PATTERN

- **POS** to numer kolejnego miejsca w *order table*, czyli w układzie utworu. Każda pozycja wskazuje numer patternu. Dwa różne POS mogą wskazywać ten sam pattern.
- **D-pad w POSITION EDITOR** tylko wybiera miejsce SONG. Zmiana POS może pokazać inny pattern do edycji, ale *nie zmienia przypisań* w module.
- **D-pad w PATTERN EDIT** pozostaje sterowaniem ROW/pól, a SELECT+B/B/SELECT+A pozostaje kopiowaniem/wklejaniem pól.
- Zaznaczenie POSITION LOOP działa tylko w POSITION EDITOR; schowek patternów z v0.3.7 pozostaje osobny.

## Jak działa loop

- Pętla jest **tymczasowa i istnieje w odtwarzaczu**, nie jest zapisywana jako komendy MOD i nie modyfikuje tablicy SONG.
- Audio zawija na początku pozycji przy wyjściu poza zakres, także wskutek skoku Bxx. Przejście z powrotem do początku pętli ustawia ROW 00. Skoki do pozycji **wewnątrz** zaznaczenia zachowują oryginalną semantykę PT2.
- Aktywny zakres można usunąć skrótem SELECT + Y podczas sesji. Po załadowaniu nowego MOD-a jest czyszczony.
- **START** nadal odtwarza wybrany PATTERN, **SELECT + START** pattern od aktualnego ROW, **MENU + START** SONG. Jeśli loop jest nieaktywny, MENU+START startuje od bieżącego POS, jak w v0.3.7.
- Cały fragment LOOP zostanie wyświetlony szarą belką w oryginalnym POSITION EDITOR. Widok POS pokazuje jedynie aktualnie widoczne pozycje listy.
- Dla bezpieczeństwa przy pierwszych testach użyj kopii MOD-a. Komendy efektów F00 (stop) pozostają zgodne z PT2, a mechanizm LOOp działa w SONG, nie w PLAY PATTERN.

## Instalacja

1. Rozpakuj zawartość archiwum w repozytorium Pocket-Pro-Tracker, zastępując stare pliki.
2. W GitHub Desktop: Summary `PPT v0.3.8 - Position selection and song range loop`.
3. Kliknij **Commit to main → Push origin**.
4. Po zielonym GitHub Actions pobierz artefakt, rozpakuj go i zainstaluj wewnętrzny `pocket_pro_tracker_pt2.zip` w muOS PortMaster.

## Kontrola i ograniczenia

Lokalnie: kompilacja kontrolera w GCC (`-Wall -Wextra -Werror`), testy przejść POS/LOOP, ponowne nakładanie patcha, testy spójności rendererów i kontroli kalibracji. **Nie uruchamiano pełnego GitHub Actions ARM64 ani fizycznego RG35XX Pro**. Podczas testów na urządzeniu warto sprawdzić, czy wejście do POSITION EDITOR rzeczywiście aktywuje sterowanie D-padem oraz czy pętla działa po skoku Bxx/Dxx.
