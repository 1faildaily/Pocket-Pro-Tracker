#!/usr/bin/env python3
"""Add Pocket Pro Tracker input adapter without replacing PT2 v1.92 graphics/audio."""
import sys, pathlib, shutil, re
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


# PPT v0.3.0: leave PT2's original renderer alone. The user prefers the
# earlier full-screen stretch to letterbox/composite modes on 640x480 muOS.
# Source patch only augments PT2's controller, note-audition and modal input.
print('PPT v0.3.0 original PT2 fullscreen renderer retained')
