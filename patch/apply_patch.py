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


# PPT v0.3.8: PT2's audio tick ordinarily copies playback ROW/PATTERN/POS
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
          '\tif (ppt_live_edit_active() || ppt_position_browse_active())\n'
          '\t{\n'
          '\t\tui.updatePatternData = true; /* animate ghost each music row */\n'
          '\t\treturn;\n'
          '\t}\n\n')
    rs=rs.replace(target,sync+target,1)
    replayer.write_text(rs)
else:
    print('PT2 frozen editor/playhead already patched')


# PPT v0.3.8: gray ghost bar is part of the original PATTERN glyph renderer.
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


# PPT v0.3.8: highlight individual NOTE, sample nibble, FX command, and
# parameter nibble fields during SELECT+B rectangular selection. Use the
# same native PT2 background-aware glyph functions, NOT translucent overlay
# above the text. That keeps digits readable and leaves gray playback ghost
# visible in all unselected cells.
ps=viewer.read_text()
if 'pptSelectionBg(' not in ps:
    import re
    # v1.92 draws normal and dotted patterns through 4 channel loops.
    needle='for (int32_t j = 0; j < PAULA_VOICES; j++, note++)'
    if ps.count(needle)!=4:
        raise AssertionError(f'PT2 pattern-view channel loops changed: found {ps.count(needle)}, expected 4')
    helper="""
/* PPT v0.3.8 selection tint behind individual native glyphs. */
static uint32_t pptSelectionBg(int row, int chan, int field, uint32_t original)
{
    return ppt_selection_contains(song->currPattern, row, chan, field) ? 0xFF858585u : original;
}
static uint32_t pptSelectionFg(int row, int chan, int field, uint32_t original)
{
    return ppt_selection_contains(song->currPattern, row, chan, field) ? video.palette[PAL_GENTXT] : original;
}
"""
    # before note formatting routine so all 4 render branches can use helpers.
    anchor='static int32_t periodToNote('
    assert ps.count(anchor)==1, 'PT2 note-name formatter changed'
    ps=ps.replace(anchor,helper+'\n'+anchor,1)
    # Isolate 4 brace-delimited per-channel render bodies without altering
    # palette and gray playback ghost of ROW numbers.
    regions=[];start=0
    for _ in range(4):
        at=ps.index(needle,start);op=ps.index('{',at)
        depth=1;k=op+1
        while depth and k<len(ps):
            if ps[k]=='{':depth+=1
            elif ps[k]=='}':depth-=1
            k+=1
        assert depth==0
        regions.append((op,k))
        start=k
    for op,k in reversed(regions):
        block=ps[op:k]
        big='textOutBigBg' in block
        font='BigBg' if big else 'Bg'
        # Guard source assumptions on NOTE/SMP/FX/param for both normal/dotted.
        assert 'noteNames[periodToNote(note->period)]' in block
        assert 'note->sample' in block and 'note->command' in block and 'note->param' in block
        assert 'note->param,' in block
        # Selection is rendered at character/field resolution. Two-digit
        # PARAM must be split into two separate glyph draws.
        font_names=['textOut'+font,'charOut'+font,'printOneHex'+font,'printTwoHex'+font]
        stmt=re.compile(r'(?m)^(?P<indent>\s*)(?P<fn>'+ '|'.join(font_names) +r')\(x\s*,\s*y\s*,\s*(?P<arg>.*?),\s*(?P<fg>video\.palette\[PAL_GENTXT\]|pptGhostText|video\.palette\[PAL_PATTXT\]),\s*(?P<bg>video\.palette\[PAL_GENBKG\]|pptGhostBg|video\.palette\[PAL_BACKGRD\])\);')
        def wrap(fn,coord,arg,field,fg,bg,indent):
            return (indent+f'{fn}({coord}, y, {arg}, '
                    f'pptSelectionFg(row, j, {field}, {fg}), '
                    f'pptSelectionBg(row, j, {field}, {bg}));')
        seen={'note':0,'sample':0,'command':0,'param':0,'dots':0}
        def sub(m):
            fn,arg,fg,bg,indent=m.group('fn','arg','fg','bg','indent')
            if 'noteNames[' in arg:
                seen['note']+=1;return wrap(fn,'x',arg,0,fg,bg,indent)
            if 'emptyDottedSample' in arg:
                seen['dots']+=1
                cfn='charOut'+font
                return ('\n'.join([wrap(cfn,'x','0x02',1,fg,bg,indent),wrap(cfn,'x+8','0x02',2,fg,bg,indent)]))
            if 'emptyDottedEffect' in arg:
                seen['dots']+=1
                cfn='charOut'+font
                return '\n'.join([wrap(cfn,'x','0x02',3,fg,bg,indent),wrap(cfn,'x+8','0x02',4,fg,bg,indent),wrap(cfn,'x+16','0x02',5,fg,bg,indent)])
            if arg=='smpChar':
                seen['sample']+=1;return wrap(fn,'x',arg,1,fg,bg,indent)
            if 'note->sample' in arg:
                seen['sample']+=1;return wrap(fn,'x',arg,2,fg,bg,indent)
            if 'note->command' in arg:
                seen['command']+=1;return wrap(fn,'x',arg,3,fg,bg,indent)
            if arg=='note->param':
                seen['param']+=1
                cfn='printOneHex'+font
                return '\n'.join([wrap(cfn,'x','(note->param >> 4)',4,fg,bg,indent),wrap(cfn,'x+8','(note->param & 0x0F)',5,fg,bg,indent)])
            return m.group(0)
        block,num=stmt.subn(sub,block)
        assert seen['note']==1 and seen['param']>=1, f'PT2 selection glyph patch failed: {seen}'
        assert num>=4, f'PT2 render glyph changes unexpected: {num}'
        ps=ps[:op]+block+ps[k:]
    assert ps.count('pptSelectionBg(row, j,')>=24
    viewer.write_text(ps)
else:
    print('PT2 field-accurate rectangular selection already patched')

# PPT v0.3.8: leave PT2's original renderer alone. The user prefers the
# earlier full-screen stretch to letterbox/composite modes on 640x480 muOS.
# Source patch only augments PT2's controller, note-audition and modal input.

# PPT v0.3.8: constrain the next SONG position inside the marked range,
# after original Bxx/Dxx and song end handling. Keep full PT2 pattern mode.
rs=replayer.read_text()
if 'ppt_loop_redirect(modPos' not in rs:
    # Place redirect BEFORE the native end-of-song stop check. Otherwise
    # config.compoMode could stop playback before our loop gets to wrap.
    anchor='\t\tmodPos = (modPos + 1) & 127;\n\t\tif (modPos >= song->header.songLength)'
    assert rs.count(anchor)==1, 'PT2 nextPosition increment anchor changed'
    addition=(
      '\t\tmodPos = (modPos + 1) & 127;\n'
      '\t\t/* PPT LOOP: route before the native song-end stop logic. */\n'
      '\t\tconst int pptNextPos = ppt_loop_redirect(modPos, song->header.songLength);\n'
      '\t\tif (pptNextPos != modPos)\n'
      '\t\t{\n'
      '\t\t\tmodPos = pptNextPos;\n'
      '\t\t\tsong->row = 0;\n'
      '\t\t}\n'
      '\t\tif (modPos >= song->header.songLength)'
    )
    rs=rs.replace(anchor,addition,1)
    replayer.write_text(rs)
else:
    print('PT2 POSITION LOOP already patched')

# Highlight selection within PT2's existing upper-right POS editor. Draw
# beneath the text, and keep the program's original fullscreen rendering.
posed=root/'src'/'pt2_posed.c'
assert posed.exists(), 'Missing upstream PT2 POS editor'
ps=posed.read_text()
if 'ppt_loop_entry_style(entry)' not in ps:
    inc='#include "pt2_posed.h"'
    assert ps.count(inc)==1, 'PT2 POS editor include changed'
    ps=ps.replace(inc,inc+'\n#include "ppt_input.h"',1)
    anchor='\t\tprintThreeDecimals(128, yPos, entry, textColor);'
    assert ps.count(anchor)==1, 'PT2 POS renderer entry anchor changed'
    tint=(
      '\t\tconst int loopStyle = ppt_loop_entry_style(entry);\n'
      '\t\tif (loopStyle)\n'
      '\t\t\tfillRect(128, yPos, (22*FONT_CHAR_W)-1, FONT_CHAR_H,\n'
      '\t\t\t         loopStyle == 2 ? 0xFF686868u : 0xFF555555u);\n'
    )
    ps=ps.replace(anchor,tint+anchor,1)
    posed.write_text(ps)
else:
    print('PT2 POSITION LOOP highlight already patched')

print('PPT v0.3.8 original PT2 fullscreen renderer retained')
