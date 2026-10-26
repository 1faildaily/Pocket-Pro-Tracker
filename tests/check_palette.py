from pathlib import Path
import re
root=Path(__file__).resolve().parent.parent
p=root/'port/pocket_pro_tracker_pt2/colors.ini'
assert p.is_file() and p.stat().st_size > 80, 'colors.ini missing or empty'
sections={}
name=None
for line in p.read_text(encoding='utf-8-sig').splitlines():
    line=line.strip()
    if not line or line.startswith(';'):
        continue
    if line.startswith('[') and line.endswith(']'):
        name=line
        assert name not in sections, f'duplicate section {name}'
        sections[name]=[]
        continue
    assert name in sections, 'value before section name'
    value=line.split(';', 1)[0].strip()
    assert re.fullmatch(r'[0-9a-fA-F]{3}',value), (name,value)
    sections[name].append(value)
assert {key:len(val) for key,val in sections.items()} == {
    '[Palette]':8, '[VU-meter]':48, '[SpectrumAnalyzer]':36,
}, 'invalid PT2 palette section counts'
wf=(root/'.github/workflows/build-ppt-arm64.yml').read_text()
assert 'pocket_pro_tracker_pt2/colors.ini' in wf
assert 'cmp - port/pocket_pro_tracker_pt2/colors.ini' in wf
print('XCOPY colors.ini: palette=8, VU-meter=48, spectrum=36; build packaging check OK')
