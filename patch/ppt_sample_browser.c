/* WAV preview/import owns separate buffers. The original MOD loader is untouched. */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>
#include "pt2_header.h"
#include "pt2_helpers.h"
#include "pt2_structs.h"
#include "pt2_audio.h"
#include "pt2_paula.h"
#include "pt2_config.h"
#include "pt2_diskop.h"
#include "pt2_sampler.h"
#include "pt2_visuals.h"
#include "pt2_textout.h"
#include "pt2_askbox.h"
#include "pt2_palette.h"
#include "pt2_tables.h"
#include "ppt_input.h"
#include "ppt_sample_browser.h"
#include "ppt_wav_resample.h"

static float *pcm;
static int frames,sourceHz;
static double previewPos;
static bool previewPlaying,importDialog;
static module_t *noteOwner;
static int8_t importedNotes[MOD_SAMPLES];
static void checkNoteOwner(void){
    if(noteOwner!=song){noteOwner=song;memset(importedNotes,-1,sizeof(importedNotes));}
}
int ppt_browser_sample_note(void){
    checkNoteOwner();
    return editor.currSample>=0 && editor.currSample<MOD_SAMPLES?importedNotes[editor.currSample]:-1;
}
static int importParts=1;
static int importPitch=28; /* E-3: default on EVERY import */
static char cachedPath[PATH_MAX+1],cachedName[PATH_MAX+1];
static char message[35]="SELECT A WAV";
static int peakLo[304],peakHi[304];
static int sourceLoopStart=-1,sourceLoopEnd=-1;
bool ppt_browser_sample_mode(void){return ui.diskOpScreenShown && diskop.mode==DISKOP_MODE_SMP;}
static void showBrowserError(void){
    /* PT2 statusMessage is 18 bytes including its terminator. */
    char status[18];snprintf(status,sizeof(status),"%.17s",message);
    displayErrorMsg(status);
}
static bool isWav(const char *path){size_t n=strlen(path);return n>=4 && !strcasecmp(path+n-4,".wav");}
static void stopPreview(void){lockAudio();previewPlaying=false;previewPos=0;unlockAudio();}
static void clearPCM(void){
    lockAudio();previewPlaying=false;previewPos=0;
    float *old=pcm;pcm=NULL;frames=0;sourceHz=0;
    unlockAudio();SDL_free(old);
    sourceLoopStart=sourceLoopEnd=-1;
}
static uint32_t readLE32(const unsigned char *p){return p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static void readLoop(const char *path){
    FILE *f=fopen(path,"rb");if(!f)return;
    unsigned char h[12];
    if(fread(h,1,12,f)!=12 || memcmp(h,"RIFF",4) || memcmp(h+8,"WAVE",4)){fclose(f);return;}
    while(fread(h,1,8,f)==8){
        const uint32_t size=readLE32(h+4);
        const long start=ftell(f);
        if(!memcmp(h,"smpl",4) && size>=60){
            unsigned char smpl[60];
            if(fread(smpl,1,60,f)==60 && readLE32(smpl+28)>0 && readLE32(smpl+40)==0){
                uint32_t a=readLE32(smpl+44),b=readLE32(smpl+48);
                if(a<b && b<(uint32_t)frames){sourceLoopStart=(int)a;sourceLoopEnd=(int)b+1;}
            }
            break;
        }
        if(size>32*1024*1024 || fseek(f,start+(long)size+(size&1),SEEK_SET))break;
    }
    fclose(f);
}
static bool prepare(const char *path,const char *name){
    if(!strcmp(path,cachedPath))return pcm!=NULL;
    clearPCM();
    snprintf(cachedPath,sizeof(cachedPath),"%s",path);
    snprintf(cachedName,sizeof(cachedName),"%s",name);
    snprintf(message,sizeof(message),"WAV PREVIEW ONLY");
    if(!isWav(path))return false;
    struct stat info;
    if(stat(path,&info) || info.st_size<=0 || info.st_size>32*1024*1024){snprintf(message,sizeof(message),"WAV FILE TOO LARGE / I/O ERROR");return false;}
    SDL_AudioSpec spec;Uint8 *bytes=NULL;Uint32 length=0;
    if(!SDL_LoadWAV(path,&spec,&bytes,&length)){snprintf(message,sizeof(message),"WAV FORMAT UNSUPPORTED");return false;}
    if(spec.freq<1 || spec.freq>384000 || !spec.channels || !length){SDL_FreeWAV(bytes);snprintf(message,sizeof(message),"INVALID WAV");return false;}
    SDL_AudioCVT cvt;
    if(SDL_BuildAudioCVT(&cvt,spec.format,spec.channels,spec.freq,AUDIO_F32SYS,1,spec.freq)<0 ||
       cvt.len_mult<1 || (uint64_t)length*cvt.len_mult>128*1024*1024){
        SDL_FreeWAV(bytes);snprintf(message,sizeof(message),"WAV CONVERSION ERROR");return false;
    }
    cvt.len=(int)length;cvt.buf=SDL_malloc((size_t)length*cvt.len_mult);
    if(!cvt.buf){SDL_FreeWAV(bytes);snprintf(message,sizeof(message),"NOT ENOUGH MEMORY");return false;}
    memcpy(cvt.buf,bytes,length);SDL_FreeWAV(bytes);
    if(SDL_ConvertAudio(&cvt)<0){SDL_free(cvt.buf);snprintf(message,sizeof(message),"WAV CONVERSION ERROR");return false;}
    const int count=cvt.len_cvt/(int)sizeof(float);
    if(count<2){SDL_free(cvt.buf);snprintf(message,sizeof(message),"WAV TOO SHORT");return false;}
    float *data=(float *)cvt.buf;
    for(int i=0;i<count;i++)if(!isfinite(data[i]))data[i]=0;
    lockAudio();pcm=data;frames=count;sourceHz=spec.freq;unlockAudio();
    for(int x=0;x<304;x++){
        int a=(int)((int64_t)x*frames/304),b=(int)((int64_t)(x+1)*frames/304);
        if(b<=a)b=a+1;if(b>frames)b=frames;
        float lo=0,hi=0;
        for(int j=a;j<b;j++){if(data[j]<lo)lo=data[j];if(data[j]>hi)hi=data[j];}
        peakLo[x]=(int)lrint(fmax(-1.0,lo)*29);peakHi[x]=(int)lrint(fmin(1.0,hi)*29);
    }
    readLoop(path);snprintf(message,sizeof(message),"B PREVIEW / A LOAD");return true;
}
void ppt_browser_update(void){
    if(ui.askBoxShown || ui.editTextFlag || importDialog)return;
    if(!ppt_browser_sample_mode()){
        if(pcm || cachedPath[0]){clearPCM();cachedPath[0]=cachedName[0]=0;}
        return;
    }
    const int row=ppt_disk_selected_row();
    if(row<0 || diskOpEntryIsDir(row)){
        if(pcm || cachedPath[0]){clearPCM();cachedPath[0]=cachedName[0]=0;}
        snprintf(message,sizeof(message),row<0?"READING DIRECTORY":"SELECT A WAV");return;
    }
    const char *name=diskOpGetUnicodeEntry(row);
    if(!name)return;
    char cwd[PATH_MAX+1],path[PATH_MAX+1];
    if(!getcwd(cwd,sizeof(cwd)))return;
    if(snprintf(path,sizeof(path),"%s/%s",cwd,name)>=(int)sizeof(path))return;
    prepare(path,diskOpGetAnsiEntry(row));
}
void ppt_browser_preview(void){
    ppt_browser_update();
    if(!pcm){showBrowserError();return;}
    lockAudio();previewPlaying=!previewPlaying;previewPos=0;unlockAudio();
}
void ppt_browser_mix(int16_t *stream,int count){
    if(!previewPlaying || !pcm || frames<2 || !audio.outputRate)return;
    const double step=(double)sourceHz/audio.outputRate;
    for(int i=0;i<count;i++){
        if(previewPos>=frames){previewPlaying=false;break;}
        int index=(int)previewPos,next=index+1<frames?index+1:index;
        double frac=previewPos-index;
        double value=(pcm[index]*(1.0-frac)+pcm[next]*frac)*0.7;
        int add=(int)lrint(fmax(-1.0,fmin(1.0,value))*32767.0);
        for(int ch=0;ch<2;ch++){
            int sum=stream[i*2+ch]+add;
            stream[i*2+ch]=(int16_t)(sum>32767?32767:sum<-32768?-32768:sum);
        }
        previewPos+=step;
    }
}
int ppt_browser_target_length(int pitch){
    if(!pcm || !sourceHz || pitch<0 || pitch>35)return 0;
    return ppt_resample_length(frames,sourceHz,PAULA_PAL_CLK/periodTable[pitch]);
}
/* Equal even-byte MOD chunks; only the final chunk may need zero padding. */
static int splitPlan(int length,int *chunk){
    if(length<2 || config.maxSampleLength<2)return 0;
    int parts=importParts;
    int size=(int)(((int64_t)length+parts*2-1)/(parts*2))*2;
    if(size>config.maxSampleLength)size=config.maxSampleLength&~1;
    *chunk=size;return parts;
}
static bool splitSlots(int parts,int slots[4]){
    slots[0]=editor.currSample;int found=1;
    for(int step=1;step<MOD_SAMPLES && found<parts;step++){
        int slot=(editor.currSample+step)%MOD_SAMPLES;
        if(song->samples[slot].length==0)slots[found++]=slot;
    }
    return found==parts;
}
int ppt_browser_import_parts(void){return importParts;}
void ppt_browser_adjust_parts(int direction){
    if(direction<0 && importParts>1)importParts/=2;
    if(direction>0 && importParts<4)importParts*=2;
}
bool ppt_browser_import_dialog(void){return importDialog;}
int ppt_browser_import_pitch(void){return importPitch;}
void ppt_browser_adjust_pitch(int semitones){
    importPitch+=semitones;
    if(importPitch<0)importPitch=0;
    if(importPitch>35)importPitch=35;
}
void ppt_browser_draw_import_choice(void){
    char line[38];
    const uint32_t bg=video.palette[PAL_GENBKG];
    fillRect(18,154,284,44,bg);
    snprintf(line,sizeof(line),"SLOT %02X   NOTE %s",editor.currSample+1,noteNames1[2+importPitch]);
    textOut2(20,155,line);
    snprintf(line,sizeof(line),"%.2f HZ  %d BYTES",PAULA_PAL_CLK/periodTable[importPitch],ppt_browser_target_length(importPitch));
    textOut2(20,166,line);
    textOut2(20,177,"UP/DOWN NOTE  LEFT/RIGHT PARTS");
    int chunk=0,slots[4];int parts=splitPlan(ppt_browser_target_length(importPitch),&chunk);
    if(!parts)snprintf(line,sizeof(line),"TOO LARGE - MAX 4 SLOTS");
    else if(!splitSlots(parts,slots))snprintf(line,sizeof(line),"NEED %d SLOTS - NOT ENOUGH FREE",parts);
    else snprintf(line,sizeof(line),"%d X %d B%s",parts,chunk,
        (int64_t)parts*chunk<ppt_browser_target_length(importPitch)?" - TRIM END":"");
    textOut2(20,188,line);
    textOut2(20,202,"L2/R2 SLOT  A LOAD  B CANCEL");
}

bool ppt_browser_import(const char *path,const char *name){
    if(!isWav(path))return false; /* retain original loader for other formats */
    if(editor.sampleZero){statusNotSampleZero();return true;}
    char absolute[PATH_MAX+1];
    if(path[0]=='/')snprintf(absolute,sizeof(absolute),"%s",path);
    else{
        char cwd[PATH_MAX+1];if(!getcwd(cwd,sizeof(cwd)))return true;
        if(snprintf(absolute,sizeof(absolute),"%s/%s",cwd,path)>=(int)sizeof(absolute)){displayErrorMsg("PATH TOO LONG");return true;}
    }
    if(!prepare(absolute,name)){showBrowserError();return true;}
    stopPreview();
    importPitch=28;importParts=1;
    while(importParts<4 && ppt_browser_target_length(importPitch)>importParts*config.maxSampleLength)importParts*=2;
    importDialog=true;
    uint32_t pitch=askBox(ASKBOX_PPT_WAV_IMPORT,"WAV RESAMPLE");
    importDialog=false;
    if(pitch>35)return true;
    const int length=ppt_browser_target_length((int)pitch);
    int chunk=0,slots[4];const int parts=splitPlan(length,&chunk);
    if(!parts){displayErrorMsg("SAMPLE TOO LONG !");return true;}
    if(!splitSlots(parts,slots)){displayErrorMsg("NOT ENOUGH SLOTS");return true;}
    int8_t *converted=calloc((size_t)chunk*parts,1);
    if(!converted){displayErrorMsg("OUT OF MEMORY !");return true;}
    const double target=PAULA_PAL_CLK/periodTable[pitch];
    const int kept=length<chunk*parts?length:chunk*parts;
    ppt_resample_pcm(pcm,frames,sourceHz,target,converted,kept);
    int loopA=-1,loopB=-1;
    if(sourceLoopStart>=0){
        loopA=(int)llround(sourceLoopStart*target/sourceHz)&~1;
        loopB=(int)llround(sourceLoopEnd*target/sourceHz)&~1;
    }
    checkNoteOwner();
    /* All buffers and slots validated before any existing sample is changed. */
    lockAudio();turnOffVoices();
    for(int part=0;part<parts;part++){
        moduleSample_t *s=&song->samples[slots[part]];
        const int begin=part*chunk;
        memcpy(&song->sampleData[s->offset],converted+begin,chunk);
        memset(&song->sampleData[s->offset+chunk],0,config.maxSampleLength-chunk);
        importedNotes[slots[part]]=(int8_t)pitch;
        s->length=chunk;s->fineTune=0;s->volume=64;s->loopStart=0;s->loopLength=2;
        /* MOD cannot represent a loop spanning two different sample slots. */
        if(loopA>=begin && loopB<=begin+chunk && loopB<=kept && loopB-loopA>=2){
            s->loopStart=loopA-begin;s->loopLength=loopB-loopA;
        }
        memset(s->text,0,sizeof(s->text));
        size_t n=strlen(cachedName);if(n>=4)n-=4;
        if(n>(parts>1?18:21))n=parts>1?18:21;
        memcpy(s->text,cachedName,n);
        if(parts>1)snprintf(s->text+n,sizeof(s->text)-n,"/%d",part+1);
        fixSampleBeep(s);
    }
    unlockAudio();free(converted);
    editor.sampleZero=false;editor.samplePos=0;
    for(int part=0;part<parts;part++)fillSampleRedoBuffer(slots[part]);
    updateCurrSample();
    ppt_set_import_note((int)pitch);updateWindowTitle(MOD_IS_MODIFIED);
    char loaded[18];if(parts>1)snprintf(loaded,sizeof(loaded),"%s",parts==2?"LOADED 2 PARTS":"LOADED 4 PARTS");
    else snprintf(loaded,sizeof(loaded),"LOADED FOR %s",noteNames1[2+pitch]);
    displayMsg(loaded);return true;
}
void ppt_browser_draw(void){
    const uint32_t fg=video.palette[PAL_GENTXT],bg=video.palette[PAL_GENBKG];
    drawFramework1(0,136,320,119);fillRect(5,138,310,114,bg);
    textOutN(8,140,message,34,fg);
    textOutN(8,148,cachedName,37,fg);
    if(!pcm)return;
    const int zero=194;
    hLine(8,zero,304,video.palette[PAL_BORDER]);
    for(int x=0;x<304;x++)vLine(x+8,zero-peakHi[x],peakHi[x]-peakLo[x]+1,fg);
    char info[40];snprintf(info,sizeof(info),"%d HZ  %.2f SEC",sourceHz,(double)frames/sourceHz);
    textOutN(8,230,info,37,fg);
    snprintf(info,sizeof(info),"SLOT %02X  L2/R2 SELECT",editor.currSample+1);
    textOutN(8,240,info,38,fg);
}
void ppt_browser_close(void){clearPCM();cachedPath[0]=cachedName[0]=0;}
