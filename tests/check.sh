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
 (p/'src/pt2_edit.c').write_text('static void jamAndPlaceSample(SDL_Scancode scancode, bool normalMode){\n int8_t noteVal = normalMode ? keyToNote(scancode) : pNoteTable[editor.currSample];\n if (normalMode || editor.pNoteFlag != 0) {}\n}\nvoid handleEditKeys(SDL_Scancode scancode, bool normalMode){\n jamAndPlaceSample(scancode, normalMode);\n jamAndPlaceSample(scancode, normalMode);\n}\n')
 for _ in range(2):subprocess.run(['python3',str(root/'patch/apply_patch.py'),str(p)],check=True)
 assert 'ppt_input_update();' in (p/'src/pt2_askbox.c').read_text()
 assert 'pptPreviewInsertedNote' in (p/'src/pt2_edit.c').read_text()
 assert 'ppt_input_update();' in (p/'src/pt2_main.c').read_text()
 assert not (p/'src/pt2_visuals.c').exists()
print('Source integration and idempotency OK')
PY
python3 - <<'PY'
from pathlib import Path
s=Path('.github/workflows/build-ppt-arm64.yml').read_text()
assert 'FULLSCREENSTRETCH=TRUE' in s and 'INTEGERSCALING=FALSE' not in s
assert 'ppt_scale' not in Path('patch/apply_patch.py').read_text()
print('Legacy fullscreen configuration OK')
PY
