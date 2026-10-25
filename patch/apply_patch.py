#!/usr/bin/env python3
"""Add Pocket Pro Tracker input adapter without replacing PT2 v1.92 graphics/audio."""
import sys, pathlib, shutil
root=pathlib.Path(sys.argv[1]).resolve() if len(sys.argv)>1 else pathlib.Path.cwd()
main=root/'src'/'pt2_main.c'
assert main.exists(), f'Not a ProTracker 2 source directory: {root}'
for name in ['ppt_input.c','ppt_input.h']:
    shutil.copy2(pathlib.Path(__file__).with_name(name),root/'src'/name)
s=main.read_text()
if 'ppt_input_init' in s:
    print('Already patched; no changes')
    sys.exit(0)
a='#include "pt2_keyboard.h"'
assert a in s
s=s.replace(a,a+'\n#include "ppt_input.h"',1)
a='\tloadConfig();'
assert a in s
# call init before entering loop (after window and sound initialized)
a='\tSDL_EventState(SDL_DROPFILE, SDL_ENABLE);'
assert a in s
s=s.replace(a,a+'\n\tppt_input_init();',1)
a='\t\thandleInput();'
assert a in s
s=s.replace(a,'\t\tppt_input_update();\n'+a,1)
a='\tcleanUp();\n\tSDL_Quit();\n\n\treturn 0;'
assert a in s
s=s.replace(a,'\tppt_input_close();\n'+a,1)
main.write_text(s)
print('Patched PT2 v1.92: original GUI and audio retained, PPT input overlay added')
