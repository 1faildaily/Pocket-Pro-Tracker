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
 p=src/'ppt_input.c';p.write_text(p.read_text()+'''
void pptQaRecord(void){
 ui.diskOpScreenShown=false;ui.samplerScreenShown=false;ui.posEdScreenShown=false;
 ui.askBoxShown=false;ui.editTextFlag=false;editor.currMode=MODE_IDLE;editor.songPlaying=false;
 ASSERT(buttonFromRaw(BTN_THUMBL)==PPT_BTN_L3);
 lastPitch=24;editor.currSample=0;cursor.channel=0;cursor.pos=0;editor.editMoveAdd=16;
 config.quantizeValue=0;song->speed=6;paulaSetup(48000,MODEL_A1200);
 buttonEvent(PPT_BTN_L3,true);releaseButton(PPT_BTN_L3);
 ASSERT(editor.currMode==MODE_RECORD && editor.songPlaying);
 song->currRow=7;song->row=7;song->tick=1;
 note_t *note=&song->patterns[song->currPattern][7*4];note->command=0xA;note->param=0x0F;
 buttonEvent(PPT_BTN_A,true);releaseButton(PPT_BTN_A);
 ASSERT(note->period==214 && note->sample==1 && note->command==0xA && note->param==0x0F);
 ASSERT(song->currRow==7);
 config.quantizeValue=4;
 buttonEvent(PPT_BTN_A,true);releaseButton(PPT_BTN_A);
 ASSERT(song->patterns[song->currPattern][8*4].period==214 && song->currRow==7);
 buttonEvent(PPT_BTN_L3,true);releaseButton(PPT_BTN_L3);
 ASSERT(!editor.songPlaying);config.quantizeValue=0;
}
''')
 p=src/'ppt_input.c';p.write_text('#include "pt2_config.h"\n#include "pt2_paula.h"\n'+p.read_text())
 p=src/'pt2_mouse.c';p.write_text(p.read_text()+'''
#include <assert.h>
int pptQaHit(int x,int y){mouse.x=x;mouse.y=y;return checkGUIButtons();}
void pptQaNameClick(void){
 mouse.x=104;mouse.y=235;editor.errorMsgActive=false;editor.errorMsgBlock=false;
 mouseButtonDownHandler(SDL_BUTTON_LEFT);
 assert(ui.editTextFlag && textEdit.object==PTB_SONGNAME && textEdit.cursorStartY==242);
 mouseButtonUpHandler(SDL_BUTTON_LEFT);
}
''')
 p=src/'pt2_diskop.c';s=p.read_text()+'''
void pptQaDisk(void){
 diskop.cached=true;diskop.numEntries=1;diskop.scrollOffset=0;diskop.mode=DISKOP_MODE_SMP;
 diskOpEntry=calloc(1,sizeof(fileEntry_t));diskOpEntry[0].nameU=strdup("tone.wav");
}
''';p.write_text(s)
 p=src/'ppt_sample_browser.c';p.write_text(p.read_text()+'''
#include <assert.h>
void pptQaSplitVerify(int parts){
 int chunk=0,slots[4];int length=ppt_browser_target_length(28);
 assert(splitPlan(length,&chunk)==parts);
 /* Additional slots are now occupied, so use the expected test allocation. */
 slots[0]=0;for(int i=1;i<parts;i++)slots[i]=i+1;
 int8_t *expected=calloc(chunk*parts,1);assert(expected);
 ppt_resample_pcm(pcm,frames,sourceHz,PAULA_PAL_CLK/periodTable[28],expected,length);
 for(int i=0;i<parts;i++){
  moduleSample_t *s=&song->samples[slots[i]];assert(s->length==chunk);
  assert(!memcmp(&song->sampleData[s->offset+2],expected+i*chunk+2,chunk-2));
  assert(s->fineTune==0 && s->volume==64);
 }
 free(expected);
 int n=0;assert(splitPlan(131072,&n)==2 && n==65536);
 assert(splitPlan(262142,&n)==4 && n==65536);
 assert(splitPlan(4*131070+2,&n)==0);
}
''')
 p=src/'ppt_input.c';p.write_text(p.read_text()+'''
void pptQaSetFull(bool on){
 fullPattern=on;ui.diskOpScreenShown=false;ui.samplerScreenShown=false;
 ui.editOpScreenShown=false;ui.posEdScreenShown=false;ui.askBoxShown=false;ui.editTextFlag=false;
}
''')
 # Preserve native globals / helper functions defined by pt2_main.c.
 p=src/'pt2_diskop.c';p.write_text(p.read_text()+'''
void pptQaMany(void){
 freeDiskOpEntryMem();diskop.cached=true;diskop.isFilling=false;
 diskop.numEntries=70;diskop.scrollOffset=0;diskop.mode=DISKOP_MODE_MOD;
 diskOpEntry=calloc(70,sizeof(fileEntry_t));
 for(int i=0;i<70;i++){
  char name[32];snprintf(name,sizeof(name),"JUNGLE_%02d.MOD",i);
  diskOpEntry[i].nameU=strdup(name);strcpy(diskOpEntry[i].dateChanged,"271026");
  diskOpEntry[i].filesize=4096+i*1024;
 }
}
''')
 p=src/'pt2_askbox.c';p.write_text(p.read_text()+'''
void pptQaModDialog(void){
 dialog_t d;memset(&d,0,sizeof(d));d.x=160;d.y=51;d.w=104;d.h=39;
 drawDialog(&d);textOut2(166,59,"OVERWRITE FILE ?");
 uint32_t expected[104*39];
 for(int y=0;y<39;y++)memcpy(&expected[y*104],&video.frameBuffer[(y+51)*320+160],104*4);
 ui.askBoxShown=true;ppt_views_begin();
 for(int y=0;y<39;y++)assert(!memcmp(&expected[y*104],&video.frameBuffer[(y+51)*320+160],104*4));
 pptQaSnapshot("mod_save_dialog.ppm");ppt_views_end();pptActiveDialog=NULL;ui.askBoxShown=false;
}
''')
 p=src/'pt2_main.c';s=p.read_text().replace('int main(int argc, char *argv[])','int pptUnusedMain(int argc, char *argv[])',1)
 s+='''
#include <assert.h>
#include "ppt_sample_browser.h"
#include "pt2_palette.h"
#include "pt2_paula.h"
#include "ppt_bpm.h"
#include "pt2_scopes.h"
#include "pt2_tables.h"
unsigned int pptQaChoice;
int pptQaSwitchSlot;
void pptQaSlot(void);
void pptQaDisk(void);
void pptQaRecord(void);
void pptQaMany(void);
int pptQaHit(int,int);
void pptQaNameClick(void);
void pptQaSplitVerify(int);
void pptQaSetFull(bool);
void pptQaModDialog(void);
void pptQaSnapshot(const char *name){
 FILE *f=fopen(name,"wb");assert(f);fprintf(f,"P6\\n320 255\\n255\\n");
 for(int i=0;i<320*255;i++){uint32_t c=video.frameBuffer[i];fputc(c>>16,f);fputc(c>>8,f);fputc(c,f);}fclose(f);
}
int main(void){
 assert(SDL_Init(SDL_INIT_TIMER)==0);
 video.frameBuffer=calloc(320*255,sizeof(uint32_t));assert(video.frameBuffer);
 video.palette[PAL_BACKGRD]=0;video.palette[PAL_GENBKG]=0;video.palette[PAL_QADSCP]=0xffffff;
 video.palette[PAL_GENTXT]=0xff0000;video.palette[PAL_PATTXT]=0xffffff;
 video.palette[PAL_BORDER]=0xaa0000;video.palette[PAL_GENBKG2]=0x440000;
 video.palette[PAL_COLORKEY]=0x12345678;assert(unpackBMPs());
 setupSprites();config.maxSampleLength=131070;song=createEmptyMod();assert(song);
 assert(allocSamplerVars());assert(allocDiskOpVars());strcpy(editor.currPath,"MODS/");audio.outputRate=48000;
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
 pptQaChoice=24;assert(ppt_browser_import("huge.wav","huge.wav"));
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
 pptQaRecord();
 ui.diskOpScreenShown=true;ui.askBoxShown=false;ui.editTextFlag=false;pptQaMany();
 assert(ppt_disk_visible_lines()==32);
 for(int i=0;i<40;i++)ppt_disk_move(1);
 assert(diskop.scrollOffset==9 && ppt_disk_selected_row()==31);
 assert(pptQaHit(100,221)==PTB_DO_FILEAREA);
 assert(pptQaHit(250,5)==PTB_DO_SAVEMODULE);
 assert(pptQaHit(315,100)==PTB_DO_EXIT);
 assert(pptQaHit(315,213)==PTB_DO_SCROLLDOWN);
 assert(pptQaHit(315,225)==PTB_DO_SCROLLBOT);
 assert(pptQaHit(100,235)==PTB_SONGNAME);
 renderDiskOpScreen();updateDiskOp();
 ppt_views_begin();pptQaSnapshot("full_mod_list.ppm");ppt_views_end();
 pptQaNameClick();
 ppt_views_begin();assert(ui.editTextFlag);pptQaSnapshot("songname.ppm");ppt_views_end();
 leaveTextEditMode(EDIT_TEXT_UPDATE);
 pptQaModDialog();
 diskop.mode=DISKOP_MODE_SMP;assert(ppt_disk_visible_lines()==10);
 
 /* Split WAV imports: selected slot plus free slots, occupied neighbor retained. */
 editor.currSample=0;editor.sampleZero=false;pptQaChoice=28;
 for(int i=1;i<MOD_SAMPLES;i++)song->samples[i].length=0;
 song->samples[1].length=64;memset(&song->sampleData[song->samples[1].offset],0x33,64);
 assert(ppt_browser_import("long.wav","long.wav"));pptQaSplitVerify(2);
 assert(!strcmp(song->samples[0].text,"long/1") && !strcmp(song->samples[2].text,"long/2"));
 for(int i=0;i<64;i++)assert(song->sampleData[song->samples[1].offset+i]==0x33);
 for(int i=2;i<MOD_SAMPLES;i++)song->samples[i].length=0;
 assert(ppt_browser_import("four.wav","four.wav"));pptQaSplitVerify(4);
 assert(!strcmp(song->samples[4].text,"four/4"));
 for(int i=1;i<MOD_SAMPLES;i++)song->samples[i].length=64;
 song->modified=false;int savedSplitLength=song->samples[0].length;
 int8_t savedSplit[64];memcpy(savedSplit,song->sampleData,64);
 assert(ppt_browser_import("long.wav","long.wav"));
 assert(!song->modified && song->samples[0].length==savedSplitLength && !memcmp(savedSplit,song->sampleData,64));
 for(int i=1;i<MOD_SAMPLES;i++)song->samples[i].length=0;
 assert(ppt_browser_import("huge.wav","huge.wav"));
 assert(!song->modified && song->samples[0].length==savedSplitLength && !memcmp(savedSplit,song->sampleData,64));
 puts("WAV split: 2/4 equal parts, PCM continuity, occupied slots, no-space and >4 rejection OK");
 /* BPM UI and offline conversion: real mouse and native decimal editor. */
 {
 ui.diskOpScreenShown=false;ui.posEdScreenShown=false;ui.editOpScreenShown=false;
 ui.samplerScreenShown=false;samplerScreen();
 assert(ui.samplerScreenShown && ui.editOpScreenShown && ui.editOpScreen==2);
 editor.currSample=0;editor.sampleZero=false;s=&song->samples[0];
 s->length=10000;s->loopStart=1000;s->loopLength=2000;s->fineTune=3;s->volume=47;
 strcpy(s->text,"BPM TEST");
 for(int i=0;i<10000;i++)song->sampleData[s->offset+i]=(int8_t)lrint(80*sin(2*3.141592653589793*100*i/10000));
 uint32_t neighbor=song->samples[1].offset;memset(&song->sampleData[neighbor],0x55,64);
 uint16_t noteBefore=song->patterns[0][0].period;
 song->modified=false;
 mouse.x=126;mouse.y=93;editor.errorMsgActive=false;editor.errorMsgBlock=false;
 mouseButtonDownHandler(SDL_BUTTON_LEFT);mouseButtonUpHandler(SDL_BUTTON_LEFT);
 assert(ppt_bpm_editing());
 textEdit.tmpDisp16=168;leaveTextEditMode(EDIT_TEXT_UPDATE);
 ppt_bpm_begin(true);textEdit.tmpDisp16=170;leaveTextEditMode(EDIT_TEXT_UPDATE);
 assert(!song->modified);
 editor.samplePosDisp=&editor.samplePos;editor.sampleVolDisp=&editor.sampleVol;
 updateCurrSample();updateEditOp();
 ppt_views_begin();pptQaSnapshot("bpm_editor.ppm");ppt_views_end();
 mouse.x=200;mouse.y=93;mouseButtonDownHandler(SDL_BUTTON_LEFT);mouseButtonUpHandler(SDL_BUTTON_LEFT);
 assert(s->length==9882 && s->loopStart==988 && s->loopLength==1976);
 assert(s->fineTune==3 && s->volume==47 && !strcmp(s->text,"BPM TEST"));
 assert(song->patterns[0][0].period==noteBefore && song->modified);
 for(int i=0;i<64;i++)assert(song->sampleData[neighbor+i]==0x55);
 int crossings=0;for(int i=1;i<s->length;i++)if(song->sampleData[s->offset+i-1]<0 && song->sampleData[s->offset+i]>=0)crossings++;
 assert(crossings>=98 && crossings<=101);
 ppt_bpm_begin(false);textEdit.tmpDisp16=170;leaveTextEditMode(EDIT_TEXT_UPDATE);
 ppt_bpm_begin(true);textEdit.tmpDisp16=168;leaveTextEditMode(EDIT_TEXT_UPDATE);
 assert(ppt_bpm_apply() && s->length==10000);
 int before=s->length;int8_t saved[64];memcpy(saved,&song->sampleData[s->offset],64);
 song->modified=false;
 ppt_bpm_begin(true);textEdit.tmpDisp16=0;leaveTextEditMode(EDIT_TEXT_UPDATE);
 assert(!ppt_bpm_apply() && s->length==before && !song->modified);
 ppt_bpm_begin(true);textEdit.tmpDisp16=1;leaveTextEditMode(EDIT_TEXT_UPDATE);
 assert(!ppt_bpm_apply() && s->length==before && !song->modified);
 assert(!memcmp(saved,&song->sampleData[s->offset],64));
 ppt_bpm_begin(true);textEdit.tmpDisp16=123;leaveTextEditMode(EDIT_TEXT_NO_UPDATE);
 assert(!ppt_bpm_apply()); /* canceled edit retains target=1 */
 ppt_bpm_begin(false);textEdit.tmpDisp16=999;textEdit.cursorBlock=0;
 ppt_bpm_direction(0);assert(textEdit.tmpDisp16==99);
 ppt_bpm_direction(1);assert(textEdit.tmpDisp16==999);
 leaveTextEditMode(EDIT_TEXT_NO_UPDATE);
 s->length=60000;s->loopStart=0;s->loopLength=2;memset(&song->sampleData[s->offset],0x20,60000);
 ppt_bpm_begin(false);textEdit.tmpDisp16=200;leaveTextEditMode(EDIT_TEXT_UPDATE);
 ppt_bpm_begin(true);textEdit.tmpDisp16=100;leaveTextEditMode(EDIT_TEXT_UPDATE);
 config.maxSampleLength=65534;assert(!ppt_bpm_apply() && s->length==60000);
 config.maxSampleLength=131070;assert(ppt_bpm_apply() && s->length==120000 && s->loopLength==2);
 for(int i=0;i<64;i++)assert(song->sampleData[neighbor+i]==0x55);
 puts("BPM: native UI, 168/170 both directions, loops, metadata, neighbor slot, zero/oversize/cancel OK");
 }

 {
 int8_t traceData[512];for(int i=0;i<512;i++)traceData[i]=(int8_t)lrint(110*sin(i*0.31));
 for(int ch=0;ch<4;ch++){
  scope[ch].active=true;scope[ch].volume=64;scope[ch].data=traceData;scope[ch].newData=traceData;
  scope[ch].length=512;scope[ch].newLength=512;scope[ch].pos=ch*13;editor.muted[ch]=true;
 }
 uint32_t *base=malloc(320*255*4),*noWave=malloc(320*255*4);assert(base && noWave);
 memcpy(base,video.frameBuffer,320*255*4);pptQaSetFull(true);
 ppt_views_begin();memcpy(noWave,video.frameBuffer,320*255*4);ppt_views_end();
 assert(!memcmp(base,video.frameBuffer,320*255*4));
 editor.muted[0]=editor.muted[2]=editor.muted[3]=false;
 scope_t unchanged[4];memcpy(unchanged,scope,sizeof(unchanged));
 ppt_views_begin();int counts[4]={0};
 for(int y=0;y<255;y++)for(int x=0;x<320;x++){
  int at=y*320+x;if(video.frameBuffer[at]==noWave[at])continue;
  assert(noWave[at]==video.palette[PAL_BACKGRD] && video.frameBuffer[at]==0x1c1c1c);
  int ch=(x-32)/72;assert(x>=32 && ch<4 && x<32+ch*72+64 && y>=12 && y<244);counts[ch]++;
 }
 assert(counts[0]>20 && counts[1]==0 && counts[2]>20 && counts[3]>20);
 pptQaSnapshot("full_pattern_scopes.ppm");ppt_views_end();
 assert(!memcmp(base,video.frameBuffer,320*255*4) && !memcmp(unchanged,scope,sizeof(unchanged)));
 pptQaSetFull(false);memset(scope,0,sizeof(unchanged));memset(editor.muted,0,sizeof(editor.muted));
 free(base);free(noWave);
 puts("Fullscreen scopes: per-channel bounds, muted channel, preserved text/highlights, scope state and framebuffer restore OK");
 }
 ppt_browser_close();SDL_Quit();
 puts("Native WAV browser: cancel/preview preserve slot, source speed 440Hz, 36 notes, E3 default, live slot switch, masked background OK");
 return 0;
}
''';p.write_text(s)
 for name,seconds in [('tone.wav',1),('long.wav',10),('four.wav',20),('huge.wav',40)]:
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
  for name in ['dialog.ppm','waveform.ppm','full_mod_list.ppm','songname.ppm','mod_save_dialog.ppm','bpm_editor.ppm','full_pattern_scopes.ppm']:shutil.copy2(t/name,dest/name)
