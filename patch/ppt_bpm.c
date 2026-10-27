/* Offline PCM conversion only. No changes to Paula, player or MOD encoding. */
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "pt2_header.h"
#include "pt2_mouse.h"
#include "pt2_structs.h"
#include "pt2_audio.h"
#include "pt2_config.h"
#include "pt2_sampler.h"
#include "pt2_helpers.h"
#include "pt2_textedit.h"
#include "pt2_textout.h"
#include "pt2_visuals.h"
#include "pt2_palette.h"
#include "ppt_wav_resample.h"
#include "ppt_bpm.h"
static uint16_t sourceBpm=0,targetBpm=0;
bool ppt_bpm_editing(void){return ui.editTextFlag &&
 (textEdit.object==PTB_EO_UPSAMP || textEdit.object==PTB_EO_DNSAMP);}
void ppt_bpm_begin(bool target){
 textEdit.tmpDisp16=target?targetBpm:sourceBpm;
 textEdit.numPtr16=&textEdit.tmpDisp16;textEdit.numDigits=3;
 textEdit.cursorStartX=target?160:124;textEdit.cursorStartY=96;
 enterNumberEditMode(TEXT_EDIT_DECIMAL,target?PTB_EO_DNSAMP:PTB_EO_UPSAMP);
}
void ppt_bpm_commit(bool apply){
 if(!apply)return;
 uint16_t v=textEdit.tmpDisp16>999?999:textEdit.tmpDisp16;
 if(textEdit.object==PTB_EO_UPSAMP)sourceBpm=v;else targetBpm=v;
}
void ppt_bpm_next(void){editTextNextChar();}
void ppt_bpm_previous(void){editTextPrevChar();}
void ppt_bpm_direction(int d){
 if(d==2){editTextPrevChar();return;}if(d==3){editTextNextChar();return;}
 unsigned place=textEdit.cursorBlock==0?100:textEdit.cursorBlock==1?10:1;
 unsigned digit=(textEdit.tmpDisp16/place)%10;
 unsigned next=(digit+(d==0?1:9))%10;
 textEdit.tmpDisp16=(uint16_t)(textEdit.tmpDisp16-digit*place+next*place);
}
void ppt_bpm_draw(void){
 if(!ui.editOpScreenShown || ui.editOpScreen!=2)return;
 /* 36 + 36 + 21 pixels: leaves VOL and all other controls untouched. */
 drawButton1(120,88,36,11,"");drawButton1(156,88,36,11,"");drawButton1(192,88,21,11,"DO");
 const uint32_t fg=video.palette[PAL_GENTXT],bg=video.palette[PAL_GENBKG];
 unsigned a=sourceBpm,b=targetBpm;
 if(ppt_bpm_editing()){
  if(textEdit.object==PTB_EO_UPSAMP)a=textEdit.tmpDisp16;else b=textEdit.tmpDisp16;
 }
 printThreeDecimalsBg(124,91,a,fg,bg);printThreeDecimalsBg(160,91,b,fg,bg);
 if(ppt_bpm_editing())fillRect(textEdit.cursorStartX+8*textEdit.cursorBlock,96,7,2,video.palette[PAL_TEXTMARK]);
}
bool ppt_bpm_apply(void){
 if(!song || editor.sampleZero){displayErrorMsg("SELECT SAMPLE");return false;}
 if(sourceBpm==0 || targetBpm==0){displayErrorMsg("BPM MUST BE 1-999");return false;}
 moduleSample_t *s=&song->samples[editor.currSample];
 if(s->length<2){displayErrorMsg("SAMPLE IS EMPTY");return false;}
 if(sourceBpm==targetBpm){displayMsg("SAME BPM");return true;}
 /* duration(new)=duration(old)*sourceBpm/targetBpm */
 int n=ppt_resample_length(s->length,targetBpm,sourceBpm);
 if(n<2 || n>config.maxSampleLength){displayErrorMsg("SAMPLE TOO LONG");return false;}
 float *src=malloc(s->length*sizeof(float));int8_t *dst=malloc(n);
 if(!src || !dst){free(src);free(dst);displayErrorMsg("OUT OF MEMORY");return false;}
 for(int i=0;i<s->length;i++)src[i]=song->sampleData[s->offset+i]/128.0f;
 ppt_resample_pcm(src,s->length,targetBpm,sourceBpm,dst,n);free(src);
 int start=0,end=2;
 if(s->loopStart+s->loopLength>2){
  start=(int)llround((double)s->loopStart*sourceBpm/targetBpm)&~1;
  end=(int)llround((double)(s->loopStart+s->loopLength)*sourceBpm/targetBpm)&~1;
  if(start<0 || end>n || end-start<2){start=0;end=2;}
 }
 /* Keep PT2's existing sample-restore snapshot, like its native resample. */
 lockAudio();turnOffVoices();memcpy(&song->sampleData[s->offset],dst,n);
 memset(&song->sampleData[s->offset+n],0,config.maxSampleLength-n);
 s->length=n;s->loopStart=start;s->loopLength=end-start;
 fixSampleBeep(s);unlockAudio();free(dst);
 editor.samplePos=0;updateCurrSample();ui.updateSongSize=true;
 updateWindowTitle(MOD_IS_MODIFIED);displayMsg("BPM CONVERTED");return true;
}
