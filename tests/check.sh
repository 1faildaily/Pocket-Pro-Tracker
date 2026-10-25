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
gcc -std=c11 -Wall -Wextra -Werror tests/scale_test.c -o /tmp/ppt_scale_tests
/tmp/ppt_scale_tests

python3 - <<'PY'
from pathlib import Path
import tempfile,subprocess
root=Path.cwd()
with tempfile.TemporaryDirectory() as d:
    base=Path(d); (base/'src').mkdir()
    main=(base/'src'/'pt2_main.c')
    main.write_text("""#include "pt2_keyboard.h"
int main() {
\tloadConfig();
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
    edit=(base/'src'/'pt2_edit.c')
    edit.write_text("""static void jamAndPlaceSample(SDL_Scancode scancode, bool normalMode)
{
    int8_t noteVal = normalMode ? keyToNote(scancode) : pNoteTable[editor.currSample];
    if (normalMode || editor.pNoteFlag != 0) { }
}
void handleEditKeys(SDL_Scancode scancode, bool normalMode)
{
    jamAndPlaceSample(scancode, normalMode);
    jamAndPlaceSample(scancode, normalMode);
}
""")
    (base/'src'/'pt2_visuals.c').write_text("""#include <stdio.h>
#include "pt2_visuals.h"
void updateRenderSizeVars(void) {
\tSDL_GetWindowSize(video.window, &video.windowW, &video.windowH);
\t\t\tconst int32_t maxUpscaleFactor = MIN(video.windowW / SCREEN_W, video.windowH / SCREEN_H);
\t\t\tvideo.renderW = SCREEN_W * maxUpscaleFactor;
\t\t\tvideo.renderH = SCREEN_H * maxUpscaleFactor;
\t\t\tSDL_GL_GetDrawableSize(video.window, &widthInPixels, &heightInPixels);
\t// "hardware mouse" calculations
}
void flipFrame(void)
{
	SDL_UpdateTexture(video.texture, NULL, video.frameBuffer, SCREEN_W * sizeof (int32_t));

	// SDL 2.0.14 bug on Windows (?): This function consumes ever-increasing memory if the program is minimized
	if (!minimized)
		SDL_RenderClear(video.renderer);
	if (video.useCustomRenderRect)
		SDL_RenderCopy(video.renderer, video.texture, NULL, &video.renderRect);
	else
		SDL_RenderCopy(video.renderer, video.texture, NULL, NULL);

	SDL_RenderPresent(video.renderer);
}
bool setupVideo(void) {
\tint32_t screenW = SCREEN_W * config.videoScaleFactor;
\tint32_t screenH = SCREEN_H * config.videoScaleFactor;
}
""")
    # Simulate harmless source format/comment drift that broke v0.2.9.
    vis=base/'src'/'pt2_visuals.c'
    original=vis.read_text()
    original=original.replace('\t// SDL 2.0.14 bug on Windows (?): This function consumes ever-increasing memory if the program is minimized',
        '\t// SDL 2.0.14 bug on Windows (?): memory grows when minimized\n\t// Additional upstream comment')
    original=original.replace('\tif (video.useCustomRenderRect)', '    if (video.useCustomRenderRect)')
    vis.write_text(original)
    for _ in range(2): # idempotency after a second run
        subprocess.run(['python3',str(root/'patch/apply_patch.py'),str(base)],check=True)
    ms=main.read_text();asrc=ask.read_text();editSrc=edit.read_text()
    assert ms.count('ppt_input_update();')==1
    assert asrc.count('ppt_input_update();')==1
    assert asrc.index('readMouseXY();')<asrc.index('ppt_input_update();')<asrc.index('SDL_PollEvent')
    assert '#include "ppt_input.h"' in asrc
    assert editSrc.count('pptPreviewInsertedNote')==1
    assert 'jamAndPlaceSample(SDL_SCANCODE_UNKNOWN, false, pitch, true)' in editSrc
    assert editSrc.count('jamAndPlaceSample(scancode, normalMode, -1, false);')==2
    visualsSrc=(base/'src'/'pt2_visuals.c').read_text()
    assert visualsSrc.count('PPT v0.2.8: native SDL window and renderer coordinates.')==1
    assert 'config.integerScaling' in visualsSrc
    assert 'SDL_GetRendererOutputSize(video.renderer' in visualsSrc
    assert 'screenW = 640;' in visualsSrc
    assert 'SDL_GL_GetDrawableSize(video.window' not in visualsSrc
    assert 'PPT VIDEO v0.2.8' in visualsSrc
    assert 'PPT VIDEO v0.2.9 COMPOSED' in visualsSrc
    assert 'pptPresentFinalFrame' in visualsSrc
    assert 'SDL_RenderCopy(video.renderer, pptFinalTexture, NULL, NULL)' in visualsSrc
    assert 'SDL_RenderSetLogicalSize(video.renderer, 0, 0)' in visualsSrc
    assert 'PPT v0.2.8: RG35XX Pro native fullscreen output.' in ms
    assert (base/'src'/'ppt_scale.h').exists()
    print('PT2 main/askBox patch anchors and idempotency OK')
PY

python3 - <<'PY'
from pathlib import Path
w=Path('.github/workflows/build-ppt-arm64.yml').read_text()
for x in ['VIDEOSCALE=1X','FULLSCREEN=TRUE','FULLSCREENSTRETCH=FALSE','INTEGERSCALING=FALSE','PIXELFILTER=NEAREST']:
    assert x in w, x
assert 'FULLSCREENSTRETCH=TRUE' not in w
p=Path('patch/apply_patch.py').read_text()
assert 'pptPreviewInsertedNote' in p
assert 'jamAndPlaceSample' in p
print('Aspect-fit config and native note audition integration checked')
PY

python3 - <<'PYTEST027'
w,h=640,480
rw=(h*320)//255
assert rw==602
assert (w-rw)//2==19
print('RG35XX Pro: image 602x480, 19px black sidebars confirmed mathematically')
PYTEST027
