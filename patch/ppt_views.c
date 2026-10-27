/* Transient presentation only. Native framebuffer is restored after upload. */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "pt2_header.h"
#include "pt2_structs.h"
#include "pt2_palette.h"
#include "pt2_config.h"
#include "pt2_tables.h"
#include "pt2_textout.h"
#include "pt2_visuals.h"
#include "ppt_input.h"

static uint32_t savedFrame[SCREEN_W * SCREEN_H];
static bool overlaySaved;
static int firstVisibleRow(void)
{
    int first=(int)song->currRow-15;
    if(first<0)first=0;
    if(first>32)first=32;
    return first;
}
static int noteIndex(uint16_t period)
{
    if(!period)return 0;
    for(int i=0;i<36;i++)if(periodTable[i]==period)return i+2;
    return 1;
}
static void fullscreenPattern(void)
{
    const uint32_t bg=video.palette[PAL_BACKGRD];
    const uint32_t fg=video.palette[PAL_PATTXT];
    const uint32_t bar=video.palette[PAL_GENBKG];
    const uint32_t text=video.palette[PAL_GENTXT];
    const char **names=(const char **)(config.accidental?noteNames2:noteNames1);
    const note_t *pattern=song->patterns[song->currPattern];
    if(!pattern)return;
    fillRect(0,0,SCREEN_W,SCREEN_H,bg);
    fillRect(0,0,SCREEN_W,11,bar);
    textOut(3,2,"POS",text);printThreeDecimals(27,2,song->currPos,text);
    textOut(64,2,"PAT",text);printTwoDecimals(88,2,song->currPattern,text);
    textOut(120,2,"LEN",text);printThreeDecimals(144,2,song->header.songLength,text);
    textOut(184,2,"SMP",text);printTwoHex(208,2,editor.currSample+1,text);
    textOut(240,2,ppt_live_edit_active()?"LIVE":"EDIT",text);
    const int first=firstVisibleRow(),ghost=ppt_ghost_row(song->currPattern);
    for(int i=0;i<32;i++){
        const int row=first+i,y=14+i*7;
        const bool current=row==song->currRow;
        const uint32_t rowBg=current?bar:(row==ghost?0x00333333:bg);
        const uint32_t rowFg=current?text:fg;
        fillRect(1,y,318,6,rowBg);
        printTwoDecimalsBg(8,y,row,rowFg,rowBg);
        for(int channel=0;channel<4;channel++){
            const note_t *n=&pattern[row*4+channel];
            const int x=32+channel*72;
            const int index=noteIndex(n->period);
            uint32_t fieldBg=ppt_selection_contains(song->currPattern,row,channel,0)?0x00555555:rowBg;
            textOutBg(x,y,names[index],rowFg,fieldBg);
            const uint8_t digits[5]={n->sample>>4,n->sample&15,n->command&15,n->param>>4,n->param&15};
            for(int j=0;j<5;j++){
                fieldBg=ppt_selection_contains(song->currPattern,row,channel,j+1)?0x00555555:rowBg;
                char ch=hexTable[digits[j]];
                if(j==0 && !digits[j] && config.blankZeroFlag)ch=' ';
                if(config.pattDots && ((j<2 && !n->sample) || (j>=2 && !n->command && !n->param)))ch=2;
                charOutBg(x+24+j*8,y,ch,rowFg,fieldBg);
            }
        }
    }
    for(int channel=0;channel<5;channel++){
        int x=channel==0?27:27+channel*72;
        fillRect(x,12,1,228,bar);
    }
    /* Underline the active field without covering its note/effect text. */
    const int field=cursor.pos%6;
    const int x=32+(cursor.pos/6)*72+(field==0?0:(field+2)*8);
    fillRect(x,14+((int)song->currRow-first)*7+5,field==0?23:7,1,video.palette[PAL_PATCURSOR]);
    fillRect(0,244,SCREEN_W,11,bar);
    textOut(3,247,"STATUS:",text);
    textOutN(64,247,ui.statusMessage,17,text);
    /* Native pointer sprites belong to the normal UI, so draw our own cross. */
    if(mouse.x>=1 && mouse.x<SCREEN_W-2 && mouse.y>=1 && mouse.y<SCREEN_H-2){
        fillRect(mouse.x-1,mouse.y,3,1,text);
        fillRect(mouse.x,mouse.y-1,1,3,text);
    }
}
void ppt_views_begin(void)
{
    overlaySaved=false;
    const bool full=ppt_full_pattern_active();
    const int row=(!full && !ui.askBoxShown && !ui.editTextFlag)?ppt_disk_selected_row():-1;
    if(!full && row<0)return;
    memcpy(savedFrame,video.frameBuffer,sizeof(savedFrame));overlaySaved=true;
    if(full){fullscreenPattern();return;}
    /* Recolor only background pixels, preserving native filename/date text. */
    const int y=35+row*6;
    const uint32_t bg=video.palette[PAL_BACKGRD];
    for(int yy=y;yy<y+5;yy++)for(int xx=8;xx<303;xx++){
        uint32_t *p=&video.frameBuffer[yy*SCREEN_W+xx];
        if((*p&0xFFFFFF)==(bg&0xFFFFFF))*p=0x00333333;
    }
    fillRect(8,y+5,295,1,video.palette[PAL_GENTXT]);
}
void ppt_views_end(void)
{
    if(overlaySaved){memcpy(video.frameBuffer,savedFrame,sizeof(savedFrame));overlaySaved=false;}
}
