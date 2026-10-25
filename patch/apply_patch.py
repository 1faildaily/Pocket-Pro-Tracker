#!/usr/bin/env python3
"""Add Pocket Pro Tracker input adapter without replacing PT2 v1.92 graphics/audio."""
import sys, pathlib, shutil
root=pathlib.Path(sys.argv[1]).resolve() if len(sys.argv)>1 else pathlib.Path.cwd()
main=root/'src'/'pt2_main.c'
assert main.exists(), f'Not a ProTracker 2 source directory: {root}'
for name in ['ppt_input.c','ppt_input.h']:
    shutil.copy2(pathlib.Path(__file__).with_name(name),root/'src'/name)
s=main.read_text()
if 'ppt_input_init' not in s:
    a='#include "pt2_keyboard.h"'
    assert a in s, 'PT2 main include anchor changed'
    s=s.replace(a,a+'\n#include "ppt_input.h"',1)
    a='\tSDL_EventState(SDL_DROPFILE, SDL_ENABLE);'
    assert a in s, 'PT2 initialization anchor changed'
    s=s.replace(a,a+'\n\tppt_input_init();',1)
    a='\t\thandleInput();'
    assert a in s, 'PT2 main input loop anchor changed'
    s=s.replace(a,'\t\tppt_input_update();\n'+a,1)
    a='\tcleanUp();\n\tSDL_Quit();\n\n\treturn 0;'
    assert a in s, 'PT2 shutdown anchor changed'
    s=s.replace(a,'\tppt_input_close();\n'+a,1)
    main.write_text(s)
else:
    print('PT2 main loop already patched')

# Critical fix: askBox() has its OWN blocking render/event loop. Without this
# extra call, all evdev input is frozen during 'ARE YOU SURE?' prompts.
ask=root/'src'/'pt2_askbox.c'
assert ask.exists(), f'Missing modal source: {ask}'
s=ask.read_text()
if 'ppt_input_update();' not in s:
    a='#include "pt2_askbox.h"'
    assert a in s, 'PT2 askbox include anchor changed'
    s=s.replace(a,a+'\n#include "ppt_input.h"',1)
    a='\t\treadMouseXY();'
    assert a in s, 'PT2 askBox render-loop anchor changed'
    s=s.replace(a,a+'\n\t\tppt_input_update();',1)
    ask.write_text(s)
else:
    print('PT2 askBox loop already patched')

print('Patched PT2 v1.92: main and modal loops updated, original GUI/audio retained')


# The keyboard note-preview path already runs through PT2's real Paula mixer.
# Reuse it rather than implementing a second (possibly inaccurate) audio engine.
# Previewing must not modify the pattern or advance a row.
edit=root/'src'/'pt2_edit.c'
assert edit.exists(), f'Missing upstream PT2 edit source: {edit}'
es=edit.read_text()
if 'pptPreviewInsertedNote' not in es:
    def once(old,new):
        global es
        assert es.count(old)==1, f'PT2 edit anchor changed ({old[:65]!r}): {es.count(old)}'
        es=es.replace(old,new,1)
    once('static void jamAndPlaceSample(SDL_Scancode scancode, bool normalMode)',
         'static void jamAndPlaceSample(SDL_Scancode scancode, bool normalMode, int8_t forcedNote, bool previewOnly)')
    once('int8_t noteVal = normalMode ? keyToNote(scancode) : pNoteTable[editor.currSample];',
         'int8_t noteVal = forcedNote >= 0 ? forcedNote : (normalMode ? keyToNote(scancode) : pNoteTable[editor.currSample]);')
    once('if (normalMode || editor.pNoteFlag != 0)',
         'if (!previewOnly && (normalMode || editor.pNoteFlag != 0))')
    assert es.count('jamAndPlaceSample(scancode, normalMode);')==2
    es=es.replace('jamAndPlaceSample(scancode, normalMode);',
                  'jamAndPlaceSample(scancode, normalMode, -1, false);')
    anchor='\nvoid handleEditKeys(SDL_Scancode scancode, bool normalMode)'
    assert anchor in es, 'PT2 edit insertion anchor changed'
    preview=r'''
/* PPT v0.2.5: Preview notes entered from the pad through the original
 * jamAndPlaceSample() Paula/audio path. Do not edit a pattern or advance ROW. */
void pptPreviewInsertedNote(int8_t pitch, int8_t sampleIndex)
{
    if (song == NULL || pitch < 0 || pitch >= 36 || sampleIndex < 0 || sampleIndex >= MOD_SAMPLES)
        return;
    if (editor.songPlaying || song->samples[sampleIndex].length <= 2)
        return;
    const int8_t savedSample = editor.currSample;
    editor.currSample = sampleIndex;
    jamAndPlaceSample(SDL_SCANCODE_UNKNOWN, false, pitch, true);
    editor.currSample = savedSample;
}
'''
    es=es.replace(anchor, '\n'+preview+anchor,1)
    edit.write_text(es)
else:
    print('PT2 native sample audition already patched')


# PPT v0.2.8: on RG35XX Pro create a native 640x480 SDL window, then use
# the actual SDL renderer output (not OpenGL drawable size) to compute the
# destination rectangle. Avoid nested 320x255 -> 640x480 compositor scaling.
mainSrc = main.read_text()
forced = '/* PPT v0.2.8: RG35XX Pro native fullscreen output. */'
if forced not in mainSrc:
    anchor = '\tloadConfig();'
    assert mainSrc.count(anchor) == 1, 'PT2 main loadConfig anchor changed'
    override = '\n'.join([
        '\t' + forced,
        '\tconfig.autoFitVideoScale = false;',
        '\tconfig.videoScaleFactor = 1;',
        '\tconfig.startInFullscreen = true;',
        '\tconfig.fullScreenStretch = false;',
        '\tconfig.integerScaling = false;',
        '\tconfig.pixelFilter = PIXELFILTER_NEAREST;',
    ])
    mainSrc = mainSrc.replace(anchor, anchor + '\n' + override, 1)
    main.write_text(mainSrc)

visuals = root / 'src' / 'pt2_visuals.c'
assert visuals.exists(), f'Missing PT2 video source: {visuals}'
vs = visuals.read_text()
marker = '/* PPT v0.2.8: native SDL window and renderer coordinates. */'
if marker not in vs:
    anchor = '\tint32_t screenW = SCREEN_W * config.videoScaleFactor;\n\tint32_t screenH = SCREEN_H * config.videoScaleFactor;'
    replacement = anchor + '\n\t' + marker + '''
\tif (config.startInFullscreen)
\t{
\t\tscreenW = 640;
\t\tscreenH = 480;
\t}'''
    assert vs.count(anchor) == 1, 'PT2 setupVideo window size anchor changed'
    vs = vs.replace(anchor, replacement, 1)

    anchor = '\tSDL_GetWindowSize(video.window, &video.windowW, &video.windowH);'
    replacement = anchor + '''
\t/* SDL renderer output may differ from the window on KMSDRM. */
\tint32_t pptOutputW = 0, pptOutputH = 0;
\tif (SDL_GetRendererOutputSize(video.renderer, &pptOutputW, &pptOutputH) != 0 ||
\t    pptOutputW <= 0 || pptOutputH <= 0)
\t{
\t\tpptOutputW = video.windowW;
\t\tpptOutputH = video.windowH;
\t}
\tif (video.fullscreen)
\t{
\t\tvideo.windowW = pptOutputW;
\t\tvideo.windowH = pptOutputH;
\t}'''
    assert vs.count(anchor) == 1, 'PT2 updateRenderSizeVars window-size anchor changed'
    vs = vs.replace(anchor, replacement, 1)

    anchor = '''\t\t\tconst int32_t maxUpscaleFactor = MIN(video.windowW / SCREEN_W, video.windowH / SCREEN_H);
\t\t\tvideo.renderW = SCREEN_W * maxUpscaleFactor;
\t\t\tvideo.renderH = SCREEN_H * maxUpscaleFactor;'''
    replacement = '''\t\t\t/* Preserve aspect ratio while filling the renderer output height. */
\t\t\tif (config.integerScaling)
\t\t\t{
\t\t\t\tconst int32_t maxUpscaleFactor = MIN(video.windowW / SCREEN_W, video.windowH / SCREEN_H);
\t\t\t\tvideo.renderW = SCREEN_W * maxUpscaleFactor;
\t\t\t\tvideo.renderH = SCREEN_H * maxUpscaleFactor;
\t\t\t}
\t\t\telse if ((int64_t)video.windowW * SCREEN_H >= (int64_t)video.windowH * SCREEN_W)
\t\t\t{
\t\t\t\tvideo.renderH = video.windowH;
\t\t\t\tvideo.renderW = (int32_t)((int64_t)video.windowH * SCREEN_W / SCREEN_H);
\t\t\t}
\t\t\telse
\t\t\t{
\t\t\t\tvideo.renderW = video.windowW;
\t\t\t\tvideo.renderH = (int32_t)((int64_t)video.windowW * SCREEN_H / SCREEN_W);
\t\t\t}'''
    assert vs.count(anchor) == 1, 'PT2 fullscreen integer scaling anchor changed'
    vs = vs.replace(anchor, replacement, 1)

    anchor = '\t\t\tSDL_GL_GetDrawableSize(video.window, &widthInPixels, &heightInPixels);'
    replacement = '\t\t\twidthInPixels = pptOutputW;\n\t\t\theightInPixels = pptOutputH;'
    assert vs.count(anchor) == 1, 'PT2 OpenGL drawable size anchor changed'
    vs = vs.replace(anchor, replacement, 1)

    # A new log reports how SDL actually describes fullscreen on the device.
    anchor = '\t// "hardware mouse" calculations'
    log = '''\t/* PPT v0.2.8: collect real SDL dimensions to diagnose letterboxing. */
\tconst char *pptVideoLog = getenv("PPT_VIDEO_DIAG_LOG");
\tif (pptVideoLog != NULL && *pptVideoLog != '\\0')
\t{
\t\tFILE *pptFile = fopen(pptVideoLog, "a");
\t\tif (pptFile != NULL)
\t\t{
\t\t\tfprintf(pptFile,
\t\t\t\t"PPT VIDEO v0.2.8 fullscreen=%d output=%dx%d target=%d,%d,%d,%d window=%dx%d\\n",
\t\t\t\t(int)video.fullscreen, pptOutputW, pptOutputH,
\t\t\t\tvideo.renderRect.x, video.renderRect.y,
\t\t\t\tvideo.renderRect.w, video.renderRect.h,
\t\t\t\tvideo.windowW, video.windowH);
\t\t\tfclose(pptFile);
\t\t}
\t}
'''
    assert vs.count(anchor) == 1, 'PT2 hardware mouse anchor changed'
    vs = vs.replace(anchor, log + anchor, 1)
    if '#include <stdlib.h>' not in vs:
        vs = vs.replace('#include <stdio.h>', '#include <stdio.h>\n#include <stdlib.h>', 1)
    visuals.write_text(vs)
else:
    print('PPT v0.2.8 native SDL scaling patch already installed')
print('PPT v0.2.8 output-size patch applied')
