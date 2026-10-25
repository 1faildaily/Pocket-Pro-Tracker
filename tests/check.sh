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
with tempfile.TemporaryDirectory() as d:
    base=Path(d); (base/'src').mkdir()
    main=(base/'src'/'pt2_main.c')
    main.write_text("""#include "pt2_keyboard.h"
int main() {
\tSDL_EventState(SDL_DROPFILE, SDL_ENABLE);
\twhile(1) {
\t\thandleInput();
\t}
\tcleanUp();
\tSDL_Quit();

\treturn 0;
}
""")
    ask=(base/'src'/'pt2_askbox.c')
    ask.write_text("""#include "pt2_askbox.h"
uint32_t askBox(uint32_t t, const char*s) {
\twhile(1) {
\t\treadMouseXY();
\t\twhile(SDL_PollEvent(&event)){}
\t}
}
""")
    for _ in range(2): # idempotency after a second run
        subprocess.run(['python3',str(root/'patch/apply_patch.py'),str(base)],check=True)
    ms=main.read_text();asrc=ask.read_text()
    assert ms.count('ppt_input_update();')==1
    assert asrc.count('ppt_input_update();')==1
    assert asrc.index('readMouseXY();')<asrc.index('ppt_input_update();')<asrc.index('SDL_PollEvent')
    assert '#include "ppt_input.h"' in asrc
    print('PT2 main/askBox patch anchors and idempotency OK')
PY
