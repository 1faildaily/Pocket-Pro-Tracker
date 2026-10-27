"""Compile real patched PT2 into an offline WAV import/preview integration test.
Usage: python3 tests/check_native_browser.py PATH_TO_PATCHED_PT2
SDL2_CFLAGS and SDL2_LIBS may override pkg-config for a local cross environment.
"""
from pathlib import Path
import tempfile,shutil,subprocess,sys,os,shlex,struct,math,wave
root=Path(sys.argv[1]).resolve()
with tempfile.TemporaryDirectory() as td:
 t=Path(td);src=t/'src';shutil.copytree(root/'src',src)
 # Only the modal wait loop is bypassed: the real native dialog is drawn first.
 p=src/'pt2_askbox.c';s=p.read_text();s='#include <assert.h>\nextern unsigned int pptQaChoice;\nextern int pptQaSwitchSlot;\nvoid updatePatternData(void);\nvoid pptQaSnapshot(const char *);\nvoid pptQaSlot(void);\n'+s
 anchor='\tpointerSetMode(POINTER_MODE_MSG1, NO_CARRY);'
 assert s.count(anchor)==1
 s=s.replace(anchor,'''    if(dialogType==ASKBOX_PPT_WAV_IMPORT){
        assert(ppt_browser_import_pitch()==28); /* E-3 on EVERY open */
        ppt_browser_adjust_pitch(-100);assert(ppt_browser_import_pitch()==0);
        ppt_browser_adjust_pitch(100);assert(ppt_browser_import_pitch()==35);
        ppt_browser_adjust_pitch(-7);assert(ppt_browser_import_pitch()==28);
        if(pptQaChoice<=35)ppt_browser_adjust_pitch((int)pptQaChoice-28);
        if(pptQaSwitchSlot)pptQaSlot();
        ppt_browser_draw_import_dialog();
        uint32_t lower[320*119];memcpy(lower,&video.frameBuffer[136*320],sizeof(lower));
        ui.updatePatternData=true;updatePatternData();
        assert(!memcmp(lower,&video.frameBuffer[136*320],sizeof(lower)));
        ppt_views_begin();pptQaSnapshot("dialog.ppm");ppt_views_end();
        pptImportDialog=NULL;ui.askBoxShown=false;return pptQaChoice;
    }
'''+anchor,1);p.write_text(s)
 p=src/'ppt_input.c';p.write_text(p.read_text()+'''
void pptQaSlot(void){
 int before=editor.currSample;bool playing=editor.songPlaying;
 buttonEvent(PPT_BTN_R2,true);releaseButton(PPT_BTN_R2);
 ASSERT(editor.currSample==before+1 && editor.songPlaying==playing);
 ASSERT(!mouseDownForwarded[PPT_BTN_R2]);
}
''')
 p=src/'pt2_diskop.c';s=p.read_text()+'''
void pptQaDisk(void){
 diskop.cached=true;diskop.numEntries=1;diskop.scrollOffset=0;diskop.mode=DISKOP_MODE_SMP;
 diskOpEntry=calloc(1,sizeof(fileEntry_t));diskOpEntry[0].nameU=strdup("tone.wav");
}
''';p.write_text(s)
 # Preserve native globals / helper functions defined by pt2_main.c.
 p=src/'pt2_main.c';s=p.read_text().replace('int main(int argc, char *argv[])','int pptUnusedMain(int argc, char *argv[])',1)
 s+='''
#include <assert.h>
#include "ppt_sample_browser.h"
#include "pt2_palette.h"
#include "pt2_paula.h"
#include "pt2_tables.h"
unsigned int pptQaChoice;
int pptQaSwitchSlot;
void pptQaSlot(void);
void pptQaDisk(void);
void pptQaSnapshot(const char *name){
 FILE *f=fopen(name,"wb");assert(f);fprintf(f,"P6\\n320 255\\n255\\n");
 for(int i=0;i<320*255;i++){uint32_t c=video.frameBuffer[i];fputc(c>>16,f);fputc(c>>8,f);fputc(c,f);}fclose(f);
}
int main(void){
 assert(SDL_Init(SDL_INIT_TIMER)==0);
 video.frameBuffer=calloc(320*255,sizeof(uint32_t));assert(video.frameBuffer);
 video.palette[PAL_BACKGRD]=0x660000;video.palette[PAL_GENBKG]=0;
 video.palette[PAL_GENTXT]=0xff0000;video.palette[PAL_PATTXT]=0;
 video.palette[PAL_BORDER]=0xaa0000;video.palette[PAL_GENBKG2]=0x440000;
 video.palette[PAL_COLORKEY]=0x12345678;assert(unpackBMPs());
 setupSprites();config.maxSampleLength=131070;song=createEmptyMod();assert(song);
 assert(allocSamplerVars());audio.outputRate=48000;
 editor.currSample=0;editor.sampleZero=false;ui.diskOpScreenShown=true;pptQaDisk();
 moduleSample_t *s=&song->samples[0];s->length=32;strcpy(s->text,"ORIGINAL");
 memset(song->sampleData,0x44,32);song->patterns[0][0].period=428;
 int16_t untouched[16];memset(untouched,0x11,sizeof(untouched));
 ppt_browser_mix(untouched,8);for(int i=0;i<16;i++)assert(untouched[i]==0x1111);
 pptQaChoice=UINT32_MAX;assert(ppt_browser_import("tone.wav","tone.wav"));
 assert(s->length==32 && !strcmp(s->text,"ORIGINAL") && !song->modified);
 for(int i=0;i<32;i++)assert(song->sampleData[i]==0x44);
 ppt_browser_draw();pptQaSnapshot("waveform.ppm");
 ppt_browser_preview();
 int16_t *sound=calloc(49000*2,sizeof(int16_t));assert(sound);
 ppt_browser_mix(sound,49000);
 int crossings=0;for(int i=1;i<48000;i++)if(sound[(i-1)*2]<=0 && sound[i*2]>0)crossings++;
 assert(abs(crossings-440)<=2);
 for(int i=48001;i<49000;i++)assert(sound[i*2]==0 && sound[i*2+1]==0);
 assert(s->length==32 && !song->modified);
 free(sound);
 pptQaChoice=24;assert(ppt_browser_import("tone.wav","tone.wav"));
 assert(s->length==16574 && editor.currPlayNote==24 && song->modified);
 assert(song->patterns[0][0].period==428 && song->samples[1].length==0);
 pptQaChoice=35;assert(ppt_browser_import("tone.wav","tone.wav"));
 assert(s->length==31388 && editor.currPlayNote==35);
 int8_t *saved=malloc(s->length);int savedLength=s->length;assert(saved);
 memcpy(saved,song->sampleData,savedLength);
 pptQaChoice=24;assert(ppt_browser_import("long.wav","long.wav"));
 assert(s->length==savedLength && !memcmp(saved,song->sampleData,savedLength));free(saved);
 assert(!ppt_browser_import("old.iff","old.iff"));
 pptQaChoice=24;
 const char *formats[]={"8bit.wav","24bit.wav","32bit.wav","float.wav"};
 for(int i=0;i<4;i++){
  assert(ppt_browser_import(formats[i],formats[i]));
  assert(s->length==16574);
 }
 assert(ppt_browser_import("loop.wav","loop.wav"));
 assert(s->loopStart>=1600 && s->loopStart<=1700 && !(s->loopStart&1));
 assert(s->loopLength>=1600 && !(s->loopLength&1));
 int lengthBefore=s->length;
 assert(ppt_browser_import("broken.wav","broken.wav"));assert(s->length==lengthBefore);
 for(int pitch=0;pitch<36;pitch++){
  pptQaChoice=(unsigned)pitch;assert(ppt_browser_import("tone.wav","tone.wav"));
  int expected=(int)llround(PAULA_PAL_CLK/periodTable[pitch]);expected=(expected+1)&~1;
  assert(s->length==expected && editor.currPlayNote==pitch);
 }
 editor.songPlaying=true;editor.currSample=0;
 int originalLength=s->length;
 pptQaSwitchSlot=1;pptQaChoice=28;
 assert(ppt_browser_import("tone.wav","tone.wav"));
 assert(editor.currSample==1 && song->samples[1].length==20864 && s->length==originalLength);
 assert(editor.songPlaying);pptQaSwitchSlot=0;editor.songPlaying=false;
 ppt_browser_close();SDL_Quit();
 puts("Native WAV browser: cancel/preview preserve slot, source speed 440Hz, 36 notes, E3 default, live slot switch, masked background OK");
 return 0;
}
''';p.write_text(s)
 for name,seconds in [('tone.wav',1),('long.wav',10)]:
  with wave.open(str(t/name),'wb') as w:
   w.setnchannels(2);w.setsampwidth(2);w.setframerate(44100)
   block=b''.join(struct.pack('<hh',int(12000*math.sin(2*math.pi*440*i/44100)),int(12000*math.sin(2*math.pi*440*i/44100))) for i in range(44100))
   w.writeframes(block*seconds)
 for bits in [8,24,32]:
  raw=bytearray()
  for i in range(44100):
   v=math.sin(2*math.pi*440*i/44100)*0.3
   if bits==8:raw.append(int(v*127)+128)
   else:raw.extend(int(v*((1<<(bits-1))-1)).to_bytes(bits//8,'little',signed=True))
  with wave.open(str(t/f'{bits}bit.wav'),'wb') as w:
   w.setnchannels(1);w.setsampwidth(bits//8);w.setframerate(44100);w.writeframes(raw)
 raw=b''.join(struct.pack('<f',0.3*math.sin(2*math.pi*440*i/44100)) for i in range(44100))
 fmt=struct.pack('<HHIIHH',3,1,44100,44100*4,4,32)
 body=b'WAVEfmt '+struct.pack('<I',len(fmt))+fmt+b'data'+struct.pack('<I',len(raw))+raw
 (t/'float.wav').write_bytes(b'RIFF'+struct.pack('<I',len(body))+body)
 raw=(t/'tone.wav').read_bytes()
 smpl=struct.pack('<15I',0,0,0,60,0,0,0,1,0,0,0,4410,8819,0,0)
 raw+=b'smpl'+struct.pack('<I',len(smpl))+smpl
 raw=raw[:4]+struct.pack('<I',len(raw)-8)+raw[8:]
 (t/'loop.wav').write_bytes(raw)
 (t/'broken.wav').write_bytes(b'RIFFbroken')
 cflags=os.environ.get('SDL2_CFLAGS')
 libs=os.environ.get('SDL2_LIBS')
 if cflags is None:cflags=subprocess.check_output(['pkg-config','--cflags','sdl2'],text=True)
 if libs is None:libs=subprocess.check_output(['pkg-config','--libs','sdl2'],text=True)
 files=[]
 for pat in ['*.c','modloaders/*.c','smploaders/*.c','gfx/*.c']:files+=list(src.glob(pat))
 exe=t/'qa'
 cmd=['gcc','-O1','-Wno-unused-result','-include','math.h','-include','string.h',*shlex.split(cflags),'-I',str(src),*[str(x) for x in files],*shlex.split(libs),'-lm','-o',str(exe)]
 if os.environ.get('PPT_QA_SANITIZE'):
  cmd[1:1]=['-g','-fsanitize=address,undefined']
 subprocess.run(cmd,check=True)
 runenv=os.environ.copy()
 if os.environ.get('PPT_QA_SANITIZE'):runenv['ASAN_OPTIONS']='detect_leaks=0'
 subprocess.run([str(exe)],cwd=t,check=True,env=runenv)
 # Optional local render inspection; the normal test leaves no artifacts.
 out=os.environ.get('PPT_QA_RENDER_DIR')
 if out:
  dest=Path(out);dest.mkdir(parents=True,exist_ok=True)
  for name in ['dialog.ppm','waveform.ppm']:shutil.copy2(t/name,dest/name)
