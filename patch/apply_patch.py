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


# PPT v0.3.3: PT2's audio tick ordinarily copies playback ROW/PATTERN/POS
# into the on-screen editor at each tick. Prevent that only during LIVE EDIT.
# The Paula transport continues on song->row/modPattern/modPos independently.
replayer=root/'src'/'pt2_replayer.c'
assert replayer.exists(), f'Missing upstream PT2 replayer source: {replayer}'
rs=replayer.read_text()
if 'ppt_live_edit_active()' not in rs:
    inc='#include "pt2_visuals_sync.h"'
    assert rs.count(inc)==1, 'PT2 replayer include anchor changed'
    rs=rs.replace(inc,inc+'\n#include "ppt_input.h"',1)
    target='\tsong->currRow = song->row;\n\tsong->currPos = modPos;\n\tsong->currPattern = modPattern;'
    assert rs.count(target)==1, 'PT2 UI sync anchor changed; do not patch audio transport blindly'
    sync=('\t/* PPT LIVE EDIT: snapshot REAL playback position independently of\n'
          '\t   displayed currRow/currPattern; keep audio transport untouched. */\n'
          '\tppt_set_playhead(modPattern, song->row);\n'
          '\tif (ppt_live_edit_active())\n'
          '\t{\n'
          '\t\tui.updatePatternData = true; /* animate ghost each music row */\n'
          '\t\treturn;\n'
          '\t}\n\n')
    rs=rs.replace(target,sync+target,1)
    replayer.write_text(rs)
else:
    print('PT2 frozen editor/playhead already patched')


# PPT v0.3.3: gray ghost bar is part of the original PATTERN glyph renderer.
# The playback row is visible only when it belongs to the CURRENTLY EDITED
# pattern, and is inside the fixed 15-row viewport. Draw behind notes so no
# alpha-overlay can wash out the ProTracker font.
viewer=root/'src'/'pt2_pattern_viewer.c'
assert viewer.exists(), f'Missing upstream PT2 pattern renderer: {viewer}'
ps=viewer.read_text()
if 'ppt_ghost_row(' not in ps:
    inc='#include "pt2_visuals.h"'
    assert ps.count(inc)==1, 'PT2 pattern-viewer include anchor changed'
    ps=ps.replace(inc,inc+'\n#include "ppt_input.h"',1)
    anchor='else // non-middle rows'
    assert ps.count(anchor)==2, 'Expected the normal and dotted pattern-view blocks'
    # Patch each brace-delimited row-drawing block independently. Guard every
    # renderer anchor before replacement; abort on unexpected upstream changes.
    offsets=[]
    start=0
    for _ in range(2):
        at=ps.index(anchor,start)
        op=ps.index('{',at)
        depth=1
        k=op+1
        while depth and k<len(ps):
            if ps[k]=='{':depth+=1
            elif ps[k]=='}':depth-=1
            k+=1
        assert depth==0, 'Malformed non-middle row block'
        offsets.append((op,k))
        start=k
    for op,k in reversed(offsets):
        block=ps[op:k]
        assert block.count('video.palette[PAL_PATTXT]')>=4, 'PT2 pattern foreground anchor changed'
        assert block.count('video.palette[PAL_BACKGRD]')>=4, 'PT2 pattern background anchor changed'
        block=block.replace('video.palette[PAL_PATTXT]','pptGhostText')
        block=block.replace('video.palette[PAL_BACKGRD]','pptGhostBg')
        insert=('\n\t\t\t\t/* Ghost is painted BEHIND font glyphs. Redraw full text cells on\n'
                '\t\t\t\t   every frame to remove the previous ghost without erasing vertical\n'
                '\t\t\t\t   track separators from ProTracker itself. */\n'
                '\t\t\t\tconst bool pptIsGhost = row == ppt_ghost_row(song->currPattern);\n'
                '\t\t\t\tconst uint32_t pptGhostBg = pptIsGhost ? 0xFF696969u : video.palette[PAL_BACKGRD];\n'
                '\t\t\t\tconst uint32_t pptGhostText = pptIsGhost ? video.palette[PAL_GENTXT] : video.palette[PAL_PATTXT];\n'
                '\t\t\t\tfillRect(8, y, 16, 7, pptGhostBg);\n'
                '\t\t\t\tfor (int pptCh = 0; pptCh < 4; pptCh++)\n'
                '\t\t\t\t\tfillRect(32 + pptCh*72, y, 64, 7, pptGhostBg);\n')
        block=block[0]+insert+block[1:]
        ps=ps[:op]+block+ps[k:]
    assert ps.count('const bool pptIsGhost')==2
    # PT2 also renders blank rows outside 00..63 while scrolling. Clear
    # those old ghost cells completely, including the two pixel row spacing.
    invalid='if (row < 0 || row >= MOD_ROWS)'
    assert ps.count(invalid)==2, 'PT2 pattern edge-of-viewport anchor changed'
    for _ in range(2):
        at=ps.index(invalid) if _==0 else ps.index(invalid, at+len(invalid))
        op=ps.index('{',at)
        clear=('\n\t\t\t/* PPT ghost cleanup beyond pattern boundaries. */\n'
               '\t\t\tfillRect(8, y, 16, 7, video.palette[PAL_BACKGRD]);\n'
               '\t\t\tfor (int pptCh = 0; pptCh < 4; pptCh++)\n'
               '\t\t\t\tfillRect(32 + pptCh*72, y, 64, 7, video.palette[PAL_BACKGRD]);\n')
        ps=ps[:op+1]+clear+ps[op+1:]
    viewer.write_text(ps)
else:
    print('PT2 ghost pattern-view already patched')

# PPT v0.3.3: leave PT2's original renderer alone. The user prefers the
# earlier full-screen stretch to letterbox/composite modes on 640x480 muOS.
# Source patch only augments PT2's controller, note-audition and modal input.
print('PPT v0.3.3 original PT2 fullscreen renderer retained')
