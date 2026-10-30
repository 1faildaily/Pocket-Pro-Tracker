#!/usr/bin/env python3
"""Add Pocket Pro Tracker input adapter without replacing PT2 v1.92 graphics/audio."""
import sys, pathlib, shutil, re
root=pathlib.Path(sys.argv[1]).resolve() if len(sys.argv)>1 else pathlib.Path.cwd()
main=root/'src'/'pt2_main.c'
assert main.exists(), f'Not a ProTracker 2 source directory: {root}'
for name in ['ppt_input.c','ppt_input.h','ppt_effects.h','ppt_views.c','ppt_sample_browser.c','ppt_sample_browser.h','ppt_wav_resample.h','ppt_bpm.c','ppt_bpm.h']:
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
          '\tppt_set_position_playhead(modPos);\n'
          '\tif (ppt_live_edit_active() && !ppt_position_browse_active())\n'
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


# PPT v0.3.14: Disable ONLY the moving four-channel VU bars.
# renderSprites() previously called renderVuMeters() which snapshots the
# background and then draws channel levels into the framebuffer. Keep that
# snapshot (used by PT2's sprite/background restore), but do not paint the
# VU bars. Leave scopes, spectrum analyzer and audio engine untouched.
vis=root/'src'/'pt2_visuals.c'
assert vis.exists(), f'Missing upstream PT2 renderer: {vis}'
vs=vis.read_text()
vu_anchor='\trenderVuMeters(); // let\'s put it here even though it\'s not sprite-based'
vu_replacement='\tfillToVuMetersBgBuffer(); // PPT_NO_CHANNEL_VU: background snapshot only; no level bars'
if 'PPT_NO_CHANNEL_VU' not in vs:
    assert vs.count(vu_anchor)==1, 'PT2 v1.92 channel-VU rendering anchor changed'
    assert vs.count('void renderSprites(void)')==1, 'PT2 renderSprites anchor changed'
    assert vs.count('void renderVuMeters(void)')==1, 'PT2 renderVuMeters anchor changed'
    # Verify that the call to be replaced belongs to renderSprites(),
    # not the standalone renderVuMeters() function or another code path.
    tail=vs.split('void renderSprites(void)',1)[1]
    assert tail.count(vu_anchor)==1, 'VU call not found inside renderSprites()'
    vs=vs.replace(vu_anchor,vu_replacement,1)
    vis.write_text(vs)
    print('PPT v0.3.14: four-channel VU bars hidden (background preserved)')
else:
    assert vu_replacement in vs and vu_anchor not in vs
    print('PPT v0.3.14: VU renderer already patched')


# PPT v0.3.20: Native resample note carousel, shows the selected candidate.
vis=root/'src'/'pt2_visuals.c'
vs=vis.read_text()
if 'PPT_RESAMPLE_NOTE_CAROUSEL' not in vs:
    before='\t\tif (ui.changingSmpResample)\n\t\t{\n\t\t\ttextOutBg(288, 236, "---", video.palette[PAL_GENTXT], video.palette[PAL_GENBKG]);\n\t\t}\n\t\telse\n\t\t{\n\t\t\tASSERT(editor.resampleNote < 36);\n\t\t\ttextOutBg(288, 236,\n\t\t\t\tconfig.accidental ? noteNames2[2+editor.resampleNote] : noteNames1[2+editor.resampleNote],\n\t\t\t\tvideo.palette[PAL_GENTXT], video.palette[PAL_GENBKG]);\n\t\t}'
    assert vs.count(before)==1, 'PT2 RESAMPLE NOTE display anchor changed'
    after='\t\t/* PPT_RESAMPLE_NOTE_CAROUSEL: show candidate pitch (C-1..B-3)\n\t\t   even while PT2 waits for a note in the sampler UI. */\n\t\tASSERT(editor.resampleNote < 36);\n\t\ttextOutBg(288, 236,\n\t\t\tconfig.accidental ? noteNames2[2+editor.resampleNote] : noteNames1[2+editor.resampleNote],\n\t\t\tvideo.palette[PAL_GENTXT], video.palette[PAL_GENBKG]);'
    vs=vs.replace(before,after,1)
    vis.write_text(vs)
    print('PPT v0.3.20 sampler resample note preview installed')
else:
    print('PPT v0.3.20 sampler resample note preview already installed')

# PPT v0.3.16: Native PT2 LOOP ON uses an existing marked sampler RANGE.
# A .MOD loop must start and end on even byte boundaries. For an odd mouse
# selection the closest inner even-aligned bounds are used. LOOP OFF remains
# the upstream implementation, and unmarked samples still behave natively.
sampler_file=root/'src'/'pt2_sampler.c'
assert sampler_file.exists(), 'Missing PT2 sampler source'
ss=sampler_file.read_text()
if 'PPT_SAMPLER_RANGE_LOOP' not in ss:
    loop_anchor='\t\tif (sampler.tmpLoopStart == 0 && sampler.tmpLoopLength == 0)\n\t\t{\n\t\t\ts->loopStart = 0;\n\t\t\ts->loopLength = s->length;\n\t\t}'
    assert ss.count(loop_anchor)==1, 'PT2 sampler loop ON anchor changed'
    modified='\t\t/* PPT_SAMPLER_RANGE_LOOP: selected waveform region wins on LOOP ON.\n\t\t   Round inward to MOD-compatible even byte offsets. */\n\t\tconst int32_t pptRangeStart = (editor.markStartOfs+1) & ~1;\n\t\tconst int32_t pptRangeEnd = editor.markEndOfs & ~1;\n\t\tif (editor.markStartOfs >= 0 &&\n\t\t    pptRangeEnd-pptRangeStart >= 2 && pptRangeEnd <= s->length)\n\t\t{\n\t\t\ts->loopStart = pptRangeStart;\n\t\t\ts->loopLength = pptRangeEnd-pptRangeStart;\n\t\t}\n\t\telse if (editor.markStartOfs >= 0)\n\t\t{\n\t\t\tdisplayErrorMsg("SET LARGER RANGE");\n\t\t\treturn;\n\t\t}\n\t\telse if (sampler.tmpLoopStart == 0 && sampler.tmpLoopLength == 0)\n\t\t{\n\t\t\ts->loopStart = 0;\n\t\t\ts->loopLength = s->length;\n\t\t}'
    ss=ss.replace(loop_anchor,modified,1)
    sampler_file.write_text(ss)
    print('PPT v0.3.16 sampler LOOP ON follows marked range')
else:
    print('PPT v0.3.16 sampler LOOP ON already patched')

# PPT 0.3.25: render a transient overlay ONLY for texture upload, then restore
# the native framebuffer before sprite erase / next incremental UI update.
vis=root/'src'/'pt2_visuals.c'
vs=vis.read_text()
if 'ppt_views_begin();' not in vs:
    if '#include "ppt_input.h"' not in vs:
        vs='#include "ppt_input.h"\n'+vs
    anchor='\tSDL_UpdateTexture(video.texture, NULL, video.frameBuffer, SCREEN_W * sizeof (int32_t));'
    assert vs.count(anchor)==1, 'Texture upload anchor changed'
    vs=vs.replace(anchor,'\tppt_views_begin();\n'+anchor+'\n\tppt_views_end();',1)
    vis.write_text(vs)
# Keep file loading / directory navigation owned by native diskOpLoadFile().
disk=root/'src'/'pt2_diskop.c'
assert disk.exists(), f'Missing native disk operation source: {disk}'
if disk.exists():
    ds=disk.read_text()
    if 'PPT_DISK_SELECTION' not in ds:
        ds=ds.replace('#include "pt2_diskop.h"','#include "pt2_diskop.h"\n#include "ppt_input.h"\n#include <stdatomic.h>',1)
        anchor='static fileEntry_t *diskOpEntry;'
        assert ds.count(anchor)==1
        ds=ds.replace(anchor,anchor+'\nstatic _Atomic int32_t pptDiskSelection = -1;',1)
        ds=ds.replace('void freeDiskOpEntryMem(void)\n{','void freeDiskOpEntryMem(void)\n{\n\tpptDiskSelection = -1;',1)
        ds+=r'''
/* PPT_DISK_SELECTION: only GUI selection and scrolling, native file actions. */
int ppt_disk_selected_row(void)
{
    if (!ui.diskOpScreenShown || !diskop.cached || diskop.isFilling ||
        diskop.forceStopReading || diskOpEntry == NULL || diskop.numEntries <= 0)
        return -1;
    if (pptDiskSelection < diskop.scrollOffset ||
        pptDiskSelection >= diskop.scrollOffset + DISKOP_LINES ||
        pptDiskSelection >= diskop.numEntries)
        pptDiskSelection = diskop.scrollOffset;
    return pptDiskSelection - diskop.scrollOffset;
}
void ppt_disk_move(int direction)
{
    if (ppt_disk_selected_row() < 0) return;
    int32_t next = pptDiskSelection + direction;
    if (next < 0) next = 0;
    if (next >= diskop.numEntries) next = diskop.numEntries - 1;
    pptDiskSelection = next;
    if (next < diskop.scrollOffset) diskop.scrollOffset = next;
    if (next >= diskop.scrollOffset + DISKOP_LINES)
        diskop.scrollOffset = next - DISKOP_LINES + 1;
    ui.updateDiskOpFileList = true;
}
void ppt_disk_activate(void)
{
    const int row = ppt_disk_selected_row();
    if (row >= 0) diskOpLoadFile((uint32_t)row, true);
}
'''
        disk.write_text(ds)

# PPT 0.3.27 WAV sample browser. Preview is mixed only into the device callback,
# never outputAudio()/MOD2WAV or the replayer/Paula sample engine.
audio=root/'src'/'pt2_audio.c'
if audio.exists():
    au=audio.read_text()
    if 'ppt_browser_mix((int16_t *)stream' not in au:
        au='#include "ppt_sample_browser.h"\n'+au
        anchor='\taudio.callbackOngoing = false;\n\n\t(void)userdata;'
        assert au.count(anchor)==1, 'Audio callback anchor changed'
        au=au.replace(anchor,'\tppt_browser_mix((int16_t *)stream, len / 4);\n'+anchor,1)
        audio.write_text(au)
# All native DISK-OP WAV clicks also take the same import confirmation path.
ds=disk.read_text()
if 'ppt_browser_import(filePath' not in ds:
    anchor='\t\t\t\tloadSample(filePath, diskOpGetAnsiEntry(fileEntryRow));'
    assert ds.count(anchor)==1, 'Native sample import anchor changed'
    ds='#include "ppt_sample_browser.h"\n'+ds
    ds=ds.replace(anchor,'\t\t\t\tif (!ppt_browser_import(filePath, diskOpGetAnsiEntry(fileEntryRow)))\n'+anchor,1)
    disk.write_text(ds)
# Native modal dialog owns input/quit handling; arrows select, A confirms, B cancels.
ah=root/'src'/'pt2_askbox.h'
if ah.exists():
    hs=ah.read_text()
    if 'ASKBOX_PPT_WAV_IMPORT' not in hs:
        assert hs.count('\tASKBOX_NUM,')==1
        hs=hs.replace('\tASKBOX_NUM,','\tASKBOX_PPT_WAV_IMPORT,\n\tASKBOX_NUM,',1)
        ah.write_text(hs)
ask=root/'src'/'pt2_askbox.c'
s=ask.read_text()
if 'case ASKBOX_PPT_WAV_IMPORT:' not in s:
    s='#include <stdio.h>\n#include "ppt_sample_browser.h"\n#include "pt2_paula.h"\n#include "pt2_tables.h"\n#include "pt2_config.h"\n'+s
    anchor='\t\tcase ASKBOX_DOWNSAMPLE:'
    assert s.count(anchor)==1
    custom=r'''
        case ASKBOX_PPT_WAV_IMPORT:
        {
            SET_DIALOG(10, 138, 300, 105);
            INIT_BUTTONS(2, -1, 28);
            SET_BUTTON(0, true,  "LOAD", SDLK_RETURN, 28, 20, 139, 222, 233);
            SET_BUTTON(1, false, "CANCEL", SDLK_ESCAPE, -1, 160, 299, 222, 233);
            pptImportDialog=&d;
            ppt_browser_draw_import_dialog();
        }
        break;
'''
    s=s.replace(anchor,custom+anchor,1)
    typeAnchor='} dialog_t;'
    assert s.count(typeAnchor)==1
    s=s.replace(typeAnchor,typeAnchor+'\nstatic dialog_t *pptImportDialog;',1)
    redrawAnchor='\nvoid removeAskBox(void)'
    assert s.count(redrawAnchor)==1
    redraw=r'''
void ppt_browser_draw_import_dialog(void)
{
    if (pptImportDialog == NULL) return;
    pptImportDialog->defaultReturnValue=ppt_browser_import_pitch();
    pptImportDialog->buttons[0].returnValue=ppt_browser_import_pitch();
    drawDialog(pptImportDialog);
    textOut2(20, 144, "RESAMPLE - ALL PT2 NOTES");
    ppt_browser_draw_import_choice();
}
'''
    s=s.replace(redrawAnchor,'\n'+redraw+redrawAnchor,1)
    s=s.replace('void removeAskBox(void)\n{','void removeAskBox(void)\n{\n\tpptImportDialog=NULL;',1)

    anchor='\t\t\t\t// abort (escape key)'
    assert s.count(anchor)==1
    key=r'''
                if (dialogType == ASKBOX_PPT_WAV_IMPORT &&
                    (key == SDLK_LEFT || key == SDLK_RIGHT || key == SDLK_UP || key == SDLK_DOWN))
                {
                    if(key==SDLK_LEFT || key==SDLK_RIGHT)ppt_browser_adjust_parts(key==SDLK_LEFT?-1:1);
                    else ppt_browser_adjust_pitch(key==SDLK_UP?1:-1);
                    d.defaultReturnValue=ppt_browser_import_pitch();
                    d.buttons[0].returnValue=d.defaultReturnValue;
                    continue;
                }
'''
    s=s.replace(anchor,key+anchor,1)
    ask.write_text(s)

vs=vis.read_text()
if 'PPT_SAMPLE_BROWSER_NO_PATTERN' not in vs:
    anchor='\t\tif (!ui.samplerScreenShown)\n\t\t\tredrawPattern();'
    assert vs.count(anchor)==1, 'Pattern refresh anchor changed'
    vs=vs.replace(anchor,'\t\t/* PPT_SAMPLE_BROWSER_NO_PATTERN: no animated pattern behind waveform/dialog. */\n\t\tif (!ui.samplerScreenShown && !(ui.diskOpScreenShown && diskop.mode == DISKOP_MODE_SMP))\n\t\t\tredrawPattern();',1)
    vis.write_text(vs)

ds=disk.read_text()
if 'PPT_BROWSER_PATTERN_REFRESH' not in ds:
    anchor='void renderDiskOpScreen(void)\n{'
    assert ds.count(anchor)==1
    ds=ds.replace(anchor,anchor+'\n\tui.updatePatternData=true; /* PPT_BROWSER_PATTERN_REFRESH: restore after leaving sample mode. */',1)
    disk.write_text(ds)

# PPT 0.3.47: use the native quantized RECORD path for handheld A.
e=root/'src'/'pt2_edit.c'
es=e.read_text()
if 'void pptRecordInsertedNote(' not in es:
    anchor='\nvoid handleEditKeys(SDL_Scancode scancode, bool normalMode)'
    assert es.count(anchor)==1
    wrapper=r'''
void pptRecordInsertedNote(int8_t pitch, int8_t sampleIndex)
{
    if(song == NULL || !editor.songPlaying || editor.currMode != MODE_RECORD ||
       pitch < 0 || pitch >= 36 || sampleIndex < 0 || sampleIndex >= MOD_SAMPLES)
        return;
    const int8_t savedSample=editor.currSample;
    editor.currSample=sampleIndex;
    jamAndPlaceSample(SDL_SCANCODE_UNKNOWN, true, pitch, false);
    editor.currSample=savedSample;
}
'''
    es=es.replace(anchor,'\n'+wrapper+anchor,1)
    e.write_text(es)
# Fit decimal EDITSKIP 00..16 into the native 32px flag box; preserve M flags.
vs=vis.read_text()
if 'PPT_EDITSKIP_16' not in vs:
    old="\t\tcharOutBg(24, 102, '0' + editor.editMoveAdd, video.palette[PAL_GENTXT], video.palette[PAL_GENBKG]);"
    assert vs.count(old)==1
    vs=vs.replace("charOutBg(16, 102, ' '","charOutBg(8, 102, ' '",1)
    vs=vs.replace("charOut(16, 102, 'M'","charOut(8, 102, 'M'",1)
    vs=vs.replace(old,'\t\t/* PPT_EDITSKIP_16: two decimal digits. */\n\t\tprintTwoDecimalsBg(16, 102, editor.editMoveAdd, video.palette[PAL_GENTXT], video.palette[PAL_GENBKG]);',1)
    vs=vs.replace('!ui.samplerScreenShown && !(ui.diskOpScreenShown && diskop.mode == DISKOP_MODE_SMP)','!ui.samplerScreenShown && !ui.diskOpScreenShown',1)
    vis.write_text(vs)
# Runtime page size: sample view stays at ten lines, MOD view gets 32.
dh=root/'src'/'pt2_diskop.h'
if dh.exists():
    hs=dh.read_text()
    if '#define DISKOP_LINES 10' in hs:
        hs=hs.replace('#define DISKOP_LINES 10','int ppt_disk_visible_lines(void);\n#define DISKOP_LINES (ppt_disk_visible_lines())',1)
        dh.write_text(hs)
ds=disk.read_text()
if 'PPT_FULL_MOD_LIST' not in ds:
    # The native top list remains 10 lines in the base framebuffer. Extra rows
    # are painted only in the upload overlay; native scrolling uses runtime size.
    anchor='for (int32_t i = 0; i < DISKOP_LINES; i++)'
    idx=ds.index('void diskOpRenderFileList(void)')
    head,tail=ds[:idx],ds[idx:]
    assert tail.count(anchor)==1
    tail=tail.replace(anchor,'for (int32_t i = 0; i < 10; i++)',1)
    ds=head+tail
    ds+=r'''
/* PPT_FULL_MOD_LIST: presentation and list-page geometry, never MOD encoding. */
int ppt_disk_visible_lines(void){return diskop.mode==DISKOP_MODE_MOD?32:10;}
void ppt_disk_draw_full(void)
{
    fillRect(0,33,320,222,video.palette[PAL_GENBKG]);
    drawFramework1(0,33,307,197);
    if(diskop.cached && !diskop.isFilling && !diskop.forceStopReading && diskOpEntry != NULL)
    {
        const int selected=ppt_disk_selected_row();
        for(int i=0;i<32 && diskop.scrollOffset+i<diskop.numEntries;i++)
        {
            fileEntry_t *entry=&diskOpEntry[diskop.scrollOffset+i];
            char *name=diskOpGetAnsiEntry(i);
            if(name==NULL)continue;
            int y=35+i*6;
            if(i==selected)fillRect(8,y,295,6,0x00333333);
            int x=config.hideDiskOpDates?8:64;
            int maxChars=config.hideDiskOpDates?(entry->isDir?31:30):(entry->isDir?24:23);
            printEntryName(name,(int)strlen(name),maxChars,x,y);
            if(entry->isDir)textOut(264,y,"(DIR)",video.palette[PAL_QADSCP]);
            else{
                if(!config.hideDiskOpDates)textOut(8,y,entry->dateChanged,video.palette[PAL_QADSCP]);
                printFileSize(entry,256,y);
            }
        }
    }
    else textOut(8,36,"READING DIRECTORY",video.palette[PAL_QADSCP]);
    drawFramework1(307,33,13,197);
    drawUpButton(308,34);drawDownButton(308,208);drawButton1(308,220,11,10,"B");
    drawFramework1(308,46,11,160);
    textOut(310,110,"E",video.palette[PAL_GENTXT]);textOut(310,119,"X",video.palette[PAL_GENTXT]);
    textOut(310,128,"I",video.palette[PAL_GENTXT]);textOut(310,137,"T",video.palette[PAL_GENTXT]);
    drawFramework1(0,232,320,11);
    textOut(8,235,"SONGNAME:",video.palette[PAL_GENTXT]);
    for(int i=0;i<20;i++)charOut(104+i*8,235,song->header.name[i]?song->header.name[i]:'_',video.palette[PAL_GENTXT]);
    drawFramework1(0,244,320,11);textOutN(8,247,ui.statusMessage,17,video.palette[PAL_GENTXT]);
 /* Mute state remains visible with the native main-screen controls covered. */
 for(int ch=0;ch<4;ch++){
  char label[3]={(char)('1'+ch),editor.muted[ch]?'X':'+',0};
  textOutBg(244+ch*18,247,label,video.palette[editor.muted[ch]?PAL_GENTXT:PAL_QADSCP],video.palette[PAL_GENBKG]);
 }
}
'''
    disk.write_text(ds)
# Native mouse hit testing follows the extended list. Top controls are unchanged.
m=root/'src'/'pt2_mouse.c'
if m.exists():
    ms=m.read_text()
    if 'PPT_MOD_LIST_HIT_TEST' not in ms:
        anchor='static int32_t checkGUIButtons(void)\n{'
        assert ms.count(anchor)==1
        custom=r'''
    /* PPT_MOD_LIST_HIT_TEST: no hidden middle/pattern controls under the list. */
    if(ui.diskOpScreenShown && diskop.mode==DISKOP_MODE_MOD && !ui.askBoxShown && !ui.editTextFlag)
    {
        if(mouse.x>=2 && mouse.x<=304 && mouse.y>=34 && mouse.y<228)return PTB_DO_FILEAREA;
        if(mouse.x>=308 && mouse.x<320 && mouse.y>=34 && mouse.y<230){
            if(mouse.y<46)return PTB_DO_SCROLLUP;
            if(mouse.y<208)return PTB_DO_EXIT;
            if(mouse.y<220)return PTB_DO_SCROLLDOWN;
            return PTB_DO_SCROLLBOT;
        }
        if(mouse.y>=232 && mouse.y<243)return PTB_SONGNAME;
        if(mouse.y>=33)return -1;
    }
'''
        ms=ms.replace(anchor,anchor+custom,1)
        m.write_text(ms)

# PPT_MOD_NAME_EDIT: keep native editing, with footer hit/cursor coordinates.
m=root/'src'/'pt2_mouse.c'
ms=m.read_text() if m.exists() else 'PPT_MOD_NAME_EDIT'
if 'PPT_MOD_NAME_EDIT' not in ms:
    a='textEdit.cursorStartY = 107;'
    assert ms.count(a)==1
    ms=ms.replace(a,'textEdit.cursorStartY = (ui.diskOpScreenShown && diskop.mode==DISKOP_MODE_MOD)?242:107; /* PPT_MOD_NAME_EDIT */',1)
    m.write_text(ms)
# Keep modal pixels (including all native labels) over the extended MOD list.
a=root/'src'/'pt2_askbox.c'
s=a.read_text()
if 'PPT_MOD_DIALOG_OVERLAY' not in s:
    s=s.replace('static dialog_t *pptImportDialog;','static dialog_t *pptImportDialog;\nstatic dialog_t *pptActiveDialog;')
    s=s.replace('static void drawDialog(dialog_t *d)\n{','static void drawDialog(dialog_t *d)\n{\n    pptActiveDialog=d;')
    s=s.replace('void removeAskBox(void)\n{','void removeAskBox(void)\n{\n    pptActiveDialog=NULL;')
    s+=r'''
/* PPT_MOD_DIALOG_OVERLAY: reuse native rendered dialog and its text. */
void ppt_mod_restore_dialog(const uint32_t *frame)
{
    if(!ui.askBoxShown || !pptActiveDialog)return;
    dialog_t *d=pptActiveDialog;
    for(int y=d->y;y<d->y+d->h && y<SCREEN_H;y++)
        for(int x=d->x;x<d->x+d->w && x<SCREEN_W;x++)
            if(x>=0 && y>=0)video.frameBuffer[y*SCREEN_W+x]=frame[y*SCREEN_W+x];
}
'''
    a.write_text(s)

# PPT_SAMPLE_BPM: UI and offline processing; playback/save sources untouched.
p=root/'src'/'pt2_sampler.c'
if p.exists():
 s=p.read_text()
 if 'PPT_SAMPLE_BPM_OPEN' not in s and 'void samplerScreen(void)' in s:
  a='\tui.samplerScreenShown = true;'
  assert s.count(a)==1
  s=s.replace(a,a+'\n\t/* PPT_SAMPLE_BPM_OPEN */\n\tui.posEdScreenShown=false;ui.diskOpScreenShown=false;\n\tui.editOpScreenShown=true;ui.editOpScreen=2;displayMainScreen();')
  p.write_text(s)
p=root/'src'/'pt2_mouse.c'
if p.exists():
 s=p.read_text()
 if 'PPT_SAMPLE_BPM_MOUSE' not in s:
  s='#include "ppt_bpm.h"\n'+s
  a='static int32_t checkGUIButtons(void)\n{'
  s=s.replace(a,a+'''
    /* PPT_SAMPLE_BPM_MOUSE: three compact cells, VOL remains at x213. */
    if(ui.editOpScreenShown && ui.editOpScreen==2 && !ui.askBoxShown && !ui.editTextFlag &&
       !ui.diskOpScreenShown && !ui.posEdScreenShown && mouse.y>=88 && mouse.y<=98){
        if(mouse.x>=120 && mouse.x<156)return PTB_EO_UPSAMP;
        if(mouse.x>=156 && mouse.x<192)return PTB_EO_DNSAMP;
        if(mouse.x>=192 && mouse.x<213)return PTB_PPT_BPM_DO;
    }
''',1)
  start=s.index('\t\tcase PTB_EO_UPSAMP:',s.index('static bool handleGUIButtons(int32_t button) //'))
  end=s.index('\t\tcase PTB_EO_VOL_NUM:',start)
  s=s[:start]+'\t\tcase PTB_EO_UPSAMP: ppt_bpm_begin(false); return true;\n\t\tcase PTB_EO_DNSAMP: ppt_bpm_begin(true); return true;\n\t\tcase PTB_PPT_BPM_DO: ppt_bpm_apply(); return true;\n\n'+s[end:]
  p.write_text(s)
p=root/'src'/'pt2_textedit.c'
if p.exists():
 s=p.read_text()
 if 'PPT_SAMPLE_BPM_COMMIT' not in s:
  s='#include "ppt_bpm.h"\n'+s
  a='\t\t\tcase PTB_SA_FIL_LP_CUTOFF:'
  assert a in s
  s=s.replace(a,'\t\t\t/* PPT_SAMPLE_BPM_COMMIT */\n\t\t\tcase PTB_EO_UPSAMP: case PTB_EO_DNSAMP: ppt_bpm_commit(updateValue); break;\n'+a,1)
  p.write_text(s)

p=root/'src'/'pt2_mouse.h'
if p.exists():
 s=p.read_text()
 if 'PTB_PPT_BPM_DO' not in s:
  s=s.replace('\tPTB_SA_FIL_HP_CUTOFF\n};','\tPTB_SA_FIL_HP_CUTOFF,\n\tPTB_PPT_BPM_DO\n};',1)
  p.write_text(s)

# PPT_POSITION_FULL: presentation below a compact header, native order data.
p=root/'src'/'pt2_posed.c'
if p.exists():
 s=p.read_text()
 if 'PPT_POSITION_FULL' not in s:
  s+=r'''
/* PPT_POSITION_FULL */
void ppt_position_draw_full(void){
 int current=ppt_position_cursor(),first=ppt_position_first();
 /* Keep the native top panel and transport. Cover only the extended list
    and the old pattern area below it. */
 drawFramework1(0,99,120,145);
 drawFramework1(120,22,188,222);
 drawFramework1(308,99,12,145);
 for(int i=0;i<31;i++){
  int pos=first+i,y=25+i*7;if(pos>song->header.songLength || pos>=128)break;
  int style=ppt_loop_entry_style(pos);
  const bool playing=pos==ppt_position_playhead();
  uint32_t fg=video.palette[(playing || (!editor.songPlaying && pos==current))?PAL_GENTXT:PAL_QADSCP];
  if(pos==current || style || playing)fillRect(128,y,176,7,style?0x555555:0x333333);
  if(pos==current && editor.songPlaying && ppt_position_detached())
    fillRect(123,y+1,3,3,video.palette[PAL_QADSCP]);
  printThreeDecimals(128,y,pos,fg);
  if(pos==song->header.songLength){textOut(160,y,"+++ APPEND",fg);continue;}
  int pat=song->header.patternTable[pos];printTwoDecimals(160,y,pat,fg);
  if(pat>=0 && pat<MAX_PATTERNS)textOutN(184,y,posEdNames[pat],15,fg);
  if(pos==current && ui.editTextFlag && textEdit.object==PTB_PE_PATTNAME)
    fillRect(184+textEdit.cursorBlock*8,y+5,7,2,video.palette[PAL_TEXTMARK]);
 }
 drawFramework1(0,244,320,11);textOutN(8,247,ui.statusMessage,17,video.palette[PAL_GENTXT]);
 /* Native main-screen mute buttons are covered; show channel states here. */
 for(int ch=0;ch<4;ch++){
  char label[3]={(char)('1'+ch),editor.muted[ch]?'X':'+',0};
  textOutBg(244+ch*18,247,label,video.palette[editor.muted[ch]?PAL_GENTXT:PAL_QADSCP],video.palette[PAL_GENBKG]);
 }
}
'''
  p.write_text(s)
p=root/'src'/'pt2_mouse.c'
if p.exists():
 s=p.read_text()
 if 'PPT_POSITION_MOUSE' not in s:
  a='static bool handleLeftMouseButton(void)\n{'
  assert a in s
  s=s.replace(a,a+r'''
    /* PPT_POSITION_MOUSE: never click hidden native pattern controls. */
    if(ui.posEdScreenShown && !ui.askBoxShown && !ui.editTextFlag && !ui.diskOpScreenShown && mouse.leftButtonPressed){
        if(mouse.x>=120 && mouse.x<308 && mouse.y>=22 && mouse.y<244){int first=ppt_position_first();
            int pos=first+(mouse.y-25)/7;if(pos<=song->header.songLength){ppt_position_set_cursor(pos);
                if(mouse.x>=184 && pos<song->header.songLength)posEdEditName();}
            return true;}
        if(mouse.y>=99 && mouse.y<244)return true;
    }
''',1)
  p.write_text(s)
# Full POSITION overlay owns mouse/wheel; underlying pattern is not interactive.
p=root/'src'/'pt2_mouse.c'
if p.exists():
 s=p.read_text()
 if 'PPT_POSITION_HIT_GUARD' not in s:
  if '#include "ppt_input.h"' not in s:s='#include "ppt_input.h"\n'+s
  a='static int32_t checkGUIButtons(void)\n{'
  s=s.replace(a,a+'\n    /* PPT_POSITION_HIT_GUARD */\n    if(ui.posEdScreenShown && !ui.diskOpScreenShown && !ui.askBoxShown && !ui.editTextFlag && ((mouse.x>=120 && mouse.x<308 && mouse.y>=22) || mouse.y>=99))return PTB_DUMMY;',1)
  for name,step in [('mouseWheelUpHandler',-1),('mouseWheelDownHandler',1)]:
   a='void '+name+'(void)\n{'
   assert a in s
   s=s.replace(a,a+'\n    if(ui.posEdScreenShown && !ui.diskOpScreenShown && !ui.askBoxShown && !ui.editTextFlag){ppt_position_set_cursor(ppt_position_cursor()+('+str(step)+'));return;}',1)
  p.write_text(s)

# PPT_POSITION_PATTERN_CURSOR: UI-only distinction between pattern and song playback.
p=root/'src'/'pt2_replayer.c'
s=p.read_text()
if 'PPT_POSITION_PATTERN_CURSOR' not in s:
 a='\tsong->currRow = song->row;\n\tsong->currPos = modPos;\n\tsong->currPattern = modPattern;'
 assert s.count(a)==1
 s=s.replace(a,'''	/* PPT_POSITION_PATTERN_CURSOR: allow browsing while auditioning one pattern. */
    if(ppt_position_pattern_active() || (ppt_position_browse_active() && ppt_position_detached())){
        song->currRow=song->row;ui.updatePosEd=true;ui.updatePatternData=true;return;
    }
'''+a,1)
 p.write_text(s)

# PPT_POSITION_NAME_CAROUSEL: native 15-char name buffer, extended-list coordinates.
p=root/'src'/'pt2_posed.c'
if p.exists():
 s=p.read_text()
 if 'PPT_POSITION_NAME_CAROUSEL' not in s and 'void posEdEditName(void)' in s:
  a='\t\ttextEdit.cursorStartY = 58;\n\t\ttextEdit.scrollable = true;'
  assert s.count(a)==1
  s=s.replace(a,'\t\t/* PPT_POSITION_NAME_CAROUSEL */\n\t\ttextEdit.cursorStartY = 30+(ppt_position_cursor()-ppt_position_first())*7;\n\t\ttextEdit.scrollable = false;',1)
  p.write_text(s)
# PPT_PATTERN_NAMES_SIDECAR: metadata only, MOD serialization stays byte-for-byte original.
for name in ['ppt_pattern_names.c','ppt_pattern_names.h']:
 shutil.copy2(pathlib.Path(__file__).with_name(name),root/'src'/name)
p=root/'src'/'pt2_posed.c'
if p.exists():
 s=p.read_text()
 if 'char *ppt_pattern_name(' not in s:
  s+='\nchar *ppt_pattern_name(int pattern){return pattern>=0 && pattern<MAX_PATTERNS?posEdNames[pattern]:NULL;}\n'
  p.write_text(s)
p=root/'src'/'pt2_module_saver.c'
if p.exists():
 s=p.read_text()
 if 'PPT_PATTERN_NAMES_SAVE' not in s:
  s='#include "ppt_pattern_names.h"\n'+s
  a='\treturn modSave(fileName);'
  assert s.count(a)==1
  s=s.replace(a,'\t/* PPT_PATTERN_NAMES_SAVE: only after successful native MOD save. */\n\tbool saved=modSave(fileName);\n\tif(saved && !ppt_pattern_names_save(fileName))displayMsg("MOD OK NAMES ERR");\n\treturn saved;',1)
  p.write_text(s)
p=root/'src'/'pt2_module_loader.c'
if p.exists():
 s=p.read_text()
 if 'PPT_PATTERN_NAMES_LOAD' not in s:
  s='#include "ppt_pattern_names.h"\n'+s
  a='\treturn newMod;'
  assert s.count(a)==1
  s=s.replace(a,'\tppt_pattern_names_prepare(fileName); /* PPT_PATTERN_NAMES_LOAD */\n'+a,1)
  a='\tposEdClearNames();'
  assert s.count(a)==1
  s=s.replace(a,a+'\n\tif(!ppt_pattern_names_load())displayMsg("NAMES FILE ERROR");',1)
  p.write_text(s)
# Keep metadata out of MODULE and SAMPLE file lists, including mod.name prefixes.
p=root/'src'/'pt2_diskop.c'
if p.exists():
 s=p.read_text()
 if 'PPT_NAMES_FILTER' not in s and 'static bool listEntry(fileEntry_t *f)' in s:
  a='\tint32_t entryName = (int32_t)UNICHAR_STRLEN(f->nameU);'
  assert s.count(a)==1
  s=s.replace(a,a+'\n\t/* PPT_NAMES_FILTER */\n\tif(!f->isDir && entryName>=13 && !UNICHAR_STRNICMP(&f->nameU[entryName-13],".patterns.txt",13))return false;',1)
  p.write_text(s)

# PPT static interface titles: repaint only label interiors after unpacking.
p=root/'src'/'pt2_bmp.c'
if p.exists():
    shutil.copy2(pathlib.Path(__file__).with_name('ppt_branding.h'),root/'src'/'ppt_branding.h')
    s=p.read_text()
    if 'pptBranding();' not in s:
        anchor='bool unpackBMPs(void)'
        assert anchor in s
        s=s.replace(anchor,'#include "ppt_branding.h"\n\n'+anchor,1)
        anchor='\tcreateBitmaps();'
        assert s.count(anchor)==1
        s=s.replace(anchor,anchor+'\n\tpptBranding();',1)
        p.write_text(s)

# POSITION editor never forwards keyboard note/effect writes into hidden pattern.
p=root/'src'/'pt2_edit.c'
if p.exists():
 s=p.read_text()
 if 'PPT_POSITION_EDIT_GUARD' not in s:
  a=r'(void handleEditKeys\(SDL_Scancode scancode, bool normalMode\)\s*\{)'
  s,count=re.subn(a,lambda m:m.group(1)+'\n\t/* PPT_POSITION_EDIT_GUARD */\n\tif (ui.posEdScreenShown) return;',s,count=1)
  assert count==1
  p.write_text(s)
