# Pocket Pro Tracker v0.3.8 — POSITION SELECT + POSITION LOOP

**SourceKit do GitHub Desktop**, nie gotowy plik wykonywalny ARM64. Bazuje na działającym v0.3.7 z zaznaczaniem LSDJ. Pozostałe skróty, ghost LIVE EDIT i prędkość myszy pozostają bez zmian.

## Jak używać POSITION SELECT

1. Kliknij **X** w pole **POS** w oryginalnym interfejsie PT2, aby otworzyć jego **POSITION EDITOR** (górna prawa część ekranu).
2. Kiedy okno POSITION EDITOR jest otwarte, **D-pad GÓRA/DÓŁ** wybiera poprzednią/następną **pozycję SONG**. Nie edytuje numerów patternów w tablicy utworu i nie zmienia odtwarzania podczas przeglądania. Lewo/prawo są w tym trybie zarezerwowane i nie modyfikują patternu.
3. Ustaw np. **POS 08** i naciśnij **SELECT + B**: rozpocznie się zaznaczanie od tej pozycji, widoczne na szaro.
4. D-padem dojdź do **POS 11**. Zaznaczony zostanie zakres 08, 09, 10, 11, łącznie cztery pozycje utworu. Naciśnij **B**, aby zatwierdzić i **włączyć pętlę**.
5. **MENU + START (PLAY)** odtworzy SONG od pierwszej pozycji aktywnej pętli, czyli od POS 08 / ROW 00. Przy końcu POS 11 wróci do 08.
6. **SELECT + Y** w POSITION EDITOR włącza lub wyłącza pętlę bez utraty zapamiętanego zakresu. Wyłączona pętla zachowuje ciemnoszary znacznik, a muzyka kontynuuje normalny SONG.
7. Ponownie kliknij **POS** lub **EXIT** w oryginalnym edytorze, żeby wrócić do PATTERN EDIT. D-pad znowu pracuje na ROW i polach nut.

## Rozróżnienie POS i PATTERN

- **POS** to numer kolejnego miejsca w *order table*, czyli w układzie utworu. Każda pozycja wskazuje numer patternu. Dwa różne POS mogą wskazywać ten sam pattern.
- **D-pad w POSITION EDITOR** tylko wybiera miejsce SONG. Zmiana POS może pokazać inny pattern do edycji, ale *nie zmienia przypisań* w module.
- **D-pad w PATTERN EDIT** pozostaje sterowaniem ROW/pól, a SELECT+B/B/SELECT+A pozostaje kopiowaniem/wklejaniem pól.
- Zaznaczenie POSITION LOOP działa tylko w POSITION EDITOR; schowek patternów z v0.3.7 pozostaje osobny.

## Jak działa loop

- Pętla jest **tymczasowa i istnieje w odtwarzaczu**, nie jest zapisywana jako komendy MOD i nie modyfikuje tablicy SONG.
- Audio zawija na początku pozycji przy wyjściu poza zakres, także wskutek skoku Bxx. Przejście z powrotem do początku pętli ustawia ROW 00. Skoki do pozycji **wewnątrz** zaznaczenia zachowują oryginalną semantykę PT2.
- Zapamiętany zakres można włączać/wyłączać podczas sesji. Po załadowaniu nowego MOD-a jest czyszczony.
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
