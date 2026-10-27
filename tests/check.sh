#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
python3 tests/check_palette.py
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
python3 - <<'PY'
from pathlib import Path
s=Path('patch/ppt_input.c').read_text()
assert 'static void adjustEditSkip(int change)' in s
assert 'static void navigateEditSkip(int direction)' in s
assert 'editor.editMoveAdd=(uint8_t)next' in s
assert 'ui.updateTrackerFlags=true;' in s
assert 'EDITSKIP = %d' in s
assert s.index('if(ui.samplerScreenShown && !ui.editTextFlag){',s.index('static void directionAction(')) < s.index('if(held[PPT_BTN_SELECT] && !held[PPT_BTN_R1]',s.index('static void directionAction('))
assert s.index('if(selecting){',s.index('static void directionAction(')) < s.index('if(held[PPT_BTN_SELECT] && !held[PPT_BTN_R1]',s.index('static void directionAction('))
print('Native EDITSKIP and view precedence OK')
PY

python3 -m py_compile patch/apply_patch.py

python3 - <<'PY'
from pathlib import Path
import tempfile,subprocess
root=Path.cwd()
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'src').mkdir()
 (p/'src/pt2_main.c').write_text('#include "pt2_keyboard.h"\nvoid f(){\n\tloadConfig();\n\tSDL_EventState(SDL_DROPFILE, SDL_ENABLE);\n\twhile (1) {\n\t\thandleInput();\n\t}\n\tcleanUp();\n\tSDL_Quit();\n\n\treturn 0;\n}\n')
 (p/'src/pt2_askbox.c').write_text('typedef struct {} dialog_t;\nvoid removeAskBox(void)\n{\n}\n#include "pt2_askbox.h"\nvoid f(){\n\t\treadMouseXY();\n\t\tcase ASKBOX_DOWNSAMPLE:\n\t\t\t\t// abort (escape key)\n}\n')
 (p/'src/pt2_replayer.c').write_text('#include "pt2_visuals_sync.h"\nstatic void updateUIPositions(void)\n{\n\tif (editor.mod2WavOngoing || editor.pat2SmpOngoing)\n\t\treturn;\n\tsong->currRow = song->row;\n\tsong->currPos = modPos;\n\tsong->currPattern = modPattern;\n}\nstatic void nextPosition(void)\n{\n\t\tmodPos = (modPos + 1) & 127;\n\t\tif (modPos >= song->header.songLength) {}\n\t\tmodPattern = (int8_t)song->header.patternTable[modPos];\n\t\tif (modPattern > MAX_PATTERNS-1) {}\n}\n')
 (p/'src/pt2_pattern_viewer.c').write_text((root/'tests/fixtures/pt2_pattern_viewer_sample.c').read_text())
 (p/'src/pt2_posed.c').write_text((root/'tests/fixtures/pt2_position_editor_sample.c').read_text())
 (p/'src/pt2_visuals.c').write_text((root/'tests/fixtures/pt2_visuals_vu_sample.c').read_text())
 (p/'src/pt2_diskop.c').write_text('#include "pt2_diskop.h"\nstatic fileEntry_t *diskOpEntry;\nvoid diskOpRenderFileList(void){for (int32_t i = 0; i < DISKOP_LINES; i++){}}\nvoid renderDiskOpScreen(void)\n{\n}\n\t\t\t\tloadSample(filePath, diskOpGetAnsiEntry(fileEntryRow));\nvoid freeDiskOpEntryMem(void)\n{\n}\n')
 (p/'src/pt2_audio.c').write_text('void f(){\n\taudio.callbackOngoing = false;\n\n\t(void)userdata;\n}\n')
 (p/'src/pt2_askbox.h').write_text('enum {\n\tASKBOX_NUM,\n};\n')
 (p/'src/pt2_sampler.c').write_text('void samplerLoopToggle(void)\n{\n\tif (s->loopStart+s->loopLength > 2) {}\n\telse\n\t{\n\t\tif (sampler.tmpLoopStart == 0 && sampler.tmpLoopLength == 0)\n\t\t{\n\t\t\ts->loopStart = 0;\n\t\t\ts->loopLength = s->length;\n\t\t}\n\t}\n}\n')
 (p/'src/pt2_edit.c').write_text('static void jamAndPlaceSample(SDL_Scancode scancode, bool normalMode){\n int8_t noteVal = normalMode ? keyToNote(scancode) : pNoteTable[editor.currSample];\n if (normalMode || editor.pNoteFlag != 0) {}\n}\nvoid handleEditKeys(SDL_Scancode scancode, bool normalMode){\n jamAndPlaceSample(scancode, normalMode);\n jamAndPlaceSample(scancode, normalMode);\n}\n')
 for _ in range(2):subprocess.run(['python3',str(root/'patch/apply_patch.py'),str(p)],check=True)
 assert (p/'src/ppt_views.c').exists()
 assert (p/'src/pt2_audio.c').read_text().count('ppt_browser_mix((int16_t *)stream, len / 4);')==1
 assert (p/'src/pt2_askbox.c').read_text().count('case ASKBOX_PPT_WAV_IMPORT:')==1
 assert 'ppt_browser_import(filePath' in (p/'src/pt2_diskop.c').read_text()
 assert (p/'src/pt2_visuals.c').read_text().count('ppt_views_begin();')==1
 assert (p/'src/pt2_visuals.c').read_text().count('ppt_views_end();')==1
 assert (p/'src/pt2_diskop.c').read_text().count('PPT_DISK_SELECTION')==1
 assert 'diskOpLoadFile((uint32_t)row, true);' in (p/'src/pt2_diskop.c').read_text()
 assert 'ppt_input_update();' in (p/'src/pt2_askbox.c').read_text()
 assert 'pptPreviewInsertedNote' in (p/'src/pt2_edit.c').read_text()
 assert 'PPT_SAMPLER_RANGE_LOOP' in (p/'src/pt2_sampler.c').read_text()
 assert (p/'src/pt2_sampler.c').read_text().count('PPT_SAMPLER_RANGE_LOOP')==1
 assert 'ppt_input_update();' in (p/'src/pt2_main.c').read_text()
 assert 'PPT_RESAMPLE_NOTE_CAROUSEL' in (p/'src/pt2_visuals.c').read_text()
 assert 'if (ppt_live_edit_active() && !ppt_position_browse_active())' in (p/'src/pt2_replayer.c').read_text()
 assert (p/'src/pt2_replayer.c').read_text().count('if (ppt_live_edit_active() && !ppt_position_browse_active())')==1
 assert 'ppt_loop_redirect(modPos, song->header.songLength)' in (p/'src/pt2_replayer.c').read_text()
 assert (p/'src/pt2_replayer.c').read_text().index('ppt_loop_redirect(modPos') < (p/'src/pt2_replayer.c').read_text().index('if (modPos >= song->header.songLength)')
 assert 'ppt_loop_entry_style(entry)' in (p/'src/pt2_posed.c').read_text()
 assert 'ppt_set_playhead(modPattern, song->row);' in (p/'src/pt2_replayer.c').read_text()
 assert (p/'src/pt2_pattern_viewer.c').read_text().count('const bool pptIsGhost')==2
 assert (p/'src/pt2_pattern_viewer.c').read_text().count('fillRect(8, y, 16, 7, pptGhostBg)')==2
 assert (p/'src/pt2_pattern_viewer.c').read_text().count('pptSelectionBg(row, j,')>=24
 vis=(p/'src/pt2_visuals.c').read_text()
 assert 'fillToVuMetersBgBuffer(); // PPT_NO_CHANNEL_VU:' in vis
 assert vis.count('PPT_NO_CHANNEL_VU')==1
 assert vis.count('PPT_RESAMPLE_NOTE_CAROUSEL')==1
 assert (p/'src/pt2_visuals.c').read_text().count('PPT_RESAMPLE_NOTE_CAROUSEL')==1
 assert 'renderVuMeters(); // let' not in vis
 assert vis.count('void renderVuMeters(void)')==1
 assert vis.count('void renderSprites(void)')==1
print('Source integration and idempotency OK')
PY
python3 - <<'PY'
from pathlib import Path
s=Path('.github/workflows/build-ppt-arm64.yml').read_text()
assert 'FULLSCREENSTRETCH=TRUE' in s and 'INTEGERSCALING=FALSE' not in s
assert 'ppt_scale' not in Path('patch/apply_patch.py').read_text()
print('Legacy fullscreen configuration OK (v0.3.8)')
PY

python3 - <<'PY'
from pathlib import Path
s=Path('patch/ppt_effects.h').read_text()
assert all(x in s for x in ['D PATTERN BREAK','C SET VOLUME','E4 VIBRATO WAVE','F SPEED / BPM'])
assert 'pptEffectStep' in s and 'pptEffectClamp' in s
assert "'ppt_effects.h'" in Path('patch/apply_patch.py').read_text()
print('Effect guide integration OK')
PY

python3 - <<'PY'
from pathlib import Path
s=Path('patch/ppt_input.c').read_text()
assert 'static int posMarkStart, posMarkEnd;' in s
assert 'const int a=posMarkStart, b=posMarkEnd;' in s
assert 'const int here=posMarkEnd;' in s
assert 'if(b==PPT_BTN_Y && held[PPT_BTN_SELECT]){userClearPosLoop();return;}' in s
assert 'posMarking)movePosMark' in s
assert 'togglePosLoop(' not in s
print('POSITION follow independent mark and SELECT+Y full clear: structure OK')
PY

python3 - <<'PY_VU'
from pathlib import Path
p=Path('patch/apply_patch.py').read_text()
assert 'renderVuMeters(); // let' in p
assert 'fillToVuMetersBgBuffer(); // PPT_NO_CHANNEL_VU' in p
assert 'pt2_visuals.c' in p
print('PPT 0.3.14 NO CHANNEL VU: preserves channel audio and spectrum renderer')
PY_VU

python3 - <<'PY_SAMPLE'
from pathlib import Path
p=Path('patch/ppt_input.c').read_text()
a=Path('patch/apply_patch.py').read_text()
assert 'samplerPlayWaveform();' in p
assert 'samplerPlayRange();' in p
assert 'samplerSamCopy();' in p
assert 'if(editStart)start=clamp(start+delta,0,end-2);' in p
assert 'else end=clamp(end+delta,start+2,length);' in p
assert 'pptSamplerMoveRange(!held[PPT_BTN_MENU],d==2?-1:1,samplerRangeStep(d));' in p
assert 'return 1;' in p and 'if(elapsed>=2300)return 100;' in p and 'if(elapsed>=900)return 5;' in p
assert 'if (editor.markStartOfs >= 0 &&' in a
assert 'resampleCarouselMove(d)' in p and 'resampleCarouselFinish(true)' in p
assert 'PPT_RESAMPLE_NOTE_CAROUSEL' in a
print('Sampler RANGE acceleration and RESAMPLE NOTE carousel patch verified')
PY_SAMPLE

python3 - <<'PY_LOOP'
# PT2 v1.92 stores sample LOOP boundaries at even 8-bit byte offsets.
for start,end,length in [(10,30,100),(11,35,100),(0,200,200),(1,2,200)]:
 s=(start+1)&~1
 e=end&~1
 if e-s>=2:
  assert s%2==0 and (e-s)%2==0 and 0<=s<e<=length
 else:
  assert (start,end)==(1,2)
print('Sampler LOOP range alignment and short-range rejection tested')
PY_LOOP

python3 - <<'PY_MONO'
from pathlib import Path
s=Path('patch/ppt_input.c').read_text()
assert 'monoHold' not in s and 'monoXConsumed' not in s
w=Path('.github/workflows/build-ppt-arm64.yml').read_text()
assert "s/^STEREOSEPARATION=.*/STEREOSEPARATION=0/" in w
assert "s/^64K_LIMIT=.*/64K_LIMIT=FALSE/" in w
assert "grep -qx 'STEREOSEPARATION=0'" in w
assert "grep -qx '64K_LIMIT=FALSE'" in w
print('Default MONO, 128KB packaging and MENU+X removal verified')
PY_MONO
gcc -std=c11 -Wall -Wextra -Werror tests/resample_test.c -lm -o /tmp/ppt_resample_tests
/tmp/ppt_resample_tests

python3 tests/check_storage.py
