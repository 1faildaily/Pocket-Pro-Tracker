"""Exercise launcher storage selection without mounting or touching real cards."""
from pathlib import Path
import tempfile, subprocess, os
root=Path(__file__).resolve().parent.parent
launcher=(root/'port/Pocket Pro Tracker PT2.sh').read_text()
block=launcher.split('# PPT_SD2_BEGIN:',1)[1].split('# PPT_SD2_END',1)[0]
block=block[block.index('\nppt_sd2_root()'):]
with tempfile.TemporaryDirectory() as td:
 t=Path(td);game=t/'game';card=t/'card';alias=t/'alias';game.mkdir();card.mkdir();alias.mkdir()
 (game/'mods').mkdir();(game/'samples').mkdir()
 ini=game/'protracker.ini'
 original='; personal settings\nSTEREOSEPARATION=0\n64K_LIMIT=FALSE\nFULLSCREEN=TRUE\nDEFAULTDIR=old\nDEFAULTDIR=duplicate\nDEFAULTSMPDIR=old-samples\n'
 ini.write_text(original)
 # Runtime mountpoint command is replaced only in this subprocess.
 script='''set -eu
mountpoint() { [ "$2" = "${TEST_MOUNTED:-none}" ]; }
'''+block.replace('/mnt/sdcard',str(card)).replace('/sdcard',str(alias))
 env=dict(os.environ,GAMEDIR=str(game),TEST_MOUNTED=str(card))
 subprocess.run(['bash','-c',script],env=env,check=True)
 lines=ini.read_text().splitlines()
 assert lines.count('DEFAULTDIR='+str(card))==1
 assert lines.count('DEFAULTSMPDIR='+str(card))==1
 assert lines[:4]==original.splitlines()[:4]
 # Mounted alias is accepted if the usual mount is absent.
 env['TEST_MOUNTED']=str(alias);subprocess.run(['bash','-c',script],env=env,check=True)
 assert 'DEFAULTDIR='+str(alias) in ini.read_text().splitlines()
 # An existing but unmounted card directory must fall back safely.
 env['TEST_MOUNTED']='none';subprocess.run(['bash','-c',script],env=env,check=True)
 lines=ini.read_text().splitlines()
 assert 'DEFAULTDIR='+str(game/'mods') in lines
 assert 'DEFAULTSMPDIR='+str(game/'samples') in lines
 assert lines[:4]==original.splitlines()[:4]
 assert not list(game.glob('*.ppt.*'))
 assert not list(card.iterdir()) and not list(alias.iterdir())
print('SD2: mounted card, alias, absent/unmounted card, preserved INI settings and duplicate removal OK')
