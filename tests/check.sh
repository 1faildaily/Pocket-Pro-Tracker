#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
gcc -D_POSIX_C_SOURCE=200809L -std=c11 -Wall -Wextra -Werror -I tests/stubs -fsyntax-only patch/ppt_input.c
gcc -D_POSIX_C_SOURCE=200809L -std=c11 -Wall -Wextra -Werror -I tests/stubs tests/controller_test.c -o /tmp/ppt_input_tests
/tmp/ppt_input_tests
python3 - <<'PY'
from pathlib import Path
import json
p=Path('port/pocket_pro_tracker_pt2/controls.json')
a=json.loads(p.read_text())
assert {x['name']:x['raw'] for x in a['buttons']}==dict(SELECT=310,START=311,L1=308,L2=314,R1=309,R2=315,A=304,B=305,X=307,Y=306)
print('Calibration JSON validated')
PY
bash -n 'port/Pocket Pro Tracker PT2.sh'
python3 -m py_compile patch/apply_patch.py

python3 - <<'PY'
from pathlib import Path
import tempfile,subprocess
root=Path.cwd()
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'src').mkdir()
 (p/'src/pt2_main.c').write_text('#include "pt2_keyboard.h"\nvoid f(){\n\tloadConfig();\n\tSDL_EventState(SDL_DROPFILE, SDL_ENABLE);\n\twhile (1) {\n\t\thandleInput();\n\t}\n\tcleanUp();\n\tSDL_Quit();\n\n\treturn 0;\n}\n')
 (p/'src/pt2_askbox.c').write_text('#include "pt2_askbox.h"\nvoid f(){\n\t\treadMouseXY();\n}\n')
 (p/'src/pt2_replayer.c').write_text('#include "pt2_visuals_sync.h"\nstatic void updateUIPositions(void)\n{\n\tif (editor.mod2WavOngoing || editor.pat2SmpOngoing)\n\t\treturn;\n\tsong->currRow = song->row;\n\tsong->currPos = modPos;\n\tsong->currPattern = modPattern;\n}\nstatic void nextPosition(void)\n{\n\t\tmodPos = (modPos + 1) & 127;\n\t\tif (modPos >= song->header.songLength) {}\n\t\tmodPattern = (int8_t)song->header.patternTable[modPos];\n\t\tif (modPattern > MAX_PATTERNS-1) {}\n}\n')
 (p/'src/pt2_pattern_viewer.c').write_text((root/'tests/fixtures/pt2_pattern_viewer_sample.c').read_text())
 (p/'src/pt2_posed.c').write_text((root/'tests/fixtures/pt2_position_editor_sample.c').read_text())
 (p/'src/pt2_edit.c').write_text('static void jamAndPlaceSample(SDL_Scancode scancode, bool normalMode){\n int8_t noteVal = normalMode ? keyToNote(scancode) : pNoteTable[editor.currSample];\n if (normalMode || editor.pNoteFlag != 0) {}\n}\nvoid handleEditKeys(SDL_Scancode scancode, bool normalMode){\n jamAndPlaceSample(scancode, normalMode);\n jamAndPlaceSample(scancode, normalMode);\n}\n')
 for _ in range(2):subprocess.run(['python3',str(root/'patch/apply_patch.py'),str(p)],check=True)
 assert 'ppt_input_update();' in (p/'src/pt2_askbox.c').read_text()
 assert 'pptPreviewInsertedNote' in (p/'src/pt2_edit.c').read_text()
 assert 'ppt_input_update();' in (p/'src/pt2_main.c').read_text()
 assert 'if (ppt_live_edit_active() || ppt_position_browse_active())' in (p/'src/pt2_replayer.c').read_text()
 assert (p/'src/pt2_replayer.c').read_text().count('if (ppt_live_edit_active() || ppt_position_browse_active())')==1
 assert 'ppt_loop_redirect(modPos, song->header.songLength)' in (p/'src/pt2_replayer.c').read_text()
 assert (p/'src/pt2_replayer.c').read_text().index('ppt_loop_redirect(modPos') < (p/'src/pt2_replayer.c').read_text().index('if (modPos >= song->header.songLength)')
 assert 'ppt_loop_entry_style(entry)' in (p/'src/pt2_posed.c').read_text()
 assert 'ppt_set_playhead(modPattern, song->row);' in (p/'src/pt2_replayer.c').read_text()
 assert (p/'src/pt2_pattern_viewer.c').read_text().count('const bool pptIsGhost')==2
 assert (p/'src/pt2_pattern_viewer.c').read_text().count('fillRect(8, y, 16, 7, pptGhostBg)')==2
 assert (p/'src/pt2_pattern_viewer.c').read_text().count('pptSelectionBg(row, j,')>=24
 assert not (p/'src/pt2_visuals.c').exists()
print('Source integration and idempotency OK')
PY
python3 - <<'PY'
from pathlib import Path
s=Path('.github/workflows/build-ppt-arm64.yml').read_text()
assert 'FULLSCREENSTRETCH=TRUE' in s and 'INTEGERSCALING=FALSE' not in s
assert 'ppt_scale' not in Path('patch/apply_patch.py').read_text()
print('Legacy fullscreen configuration OK (v0.3.8)')
PY
