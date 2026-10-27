/* Full view uses PT2's original frame slices, bitmap font and cursor assets.
   Presentation only: framebuffer restored immediately after texture upload. */
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
#include "pt2_bmp.h"
#include "ppt_input.h"
#include "ppt_sample_browser.h"

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
static void drawNativeCursor(int x,int y)
{
    /* Exact 11x14 frame from createBitmaps(), including transparent interior. */
    for(int yy=0;yy<14;yy++)for(int xx=0;xx<11;xx++){
        const uint32_t pixel=patternCursorBMP[yy*11+xx];
        if(pixel!=video.palette[PAL_COLORKEY] && x+xx>=0 && x+xx<SCREEN_W && y+yy>=0 && y+yy<SCREEN_H)
            video.frameBuffer[(y+yy)*SCREEN_W+x+xx]=pixel;
    }
}
static void drawNativeMouse(void)
{
    /* Same paletted 16x16 pointer as PT2's SPRITE_MOUSE_POINTER. */
    for(int yy=0;yy<16;yy++)for(int xx=0;xx<16;xx++){
        int x=mouse.x+xx,y=mouse.y+yy;
        const uint8_t index=mousePointerBMP[yy*16+xx];
        if(index!=PAL_COLORKEY && index<PALETTE_NUM && x>=0 && x<SCREEN_W && y>=0 && y<SCREEN_H)
            video.frameBuffer[y*SCREEN_W+x]=video.palette[index];
    }
}
static void fullscreenPattern(void)
{
    const uint32_t bg=video.palette[PAL_BACKGRD],fg=video.palette[PAL_PATTXT];
    const uint32_t bar=video.palette[PAL_GENBKG],text=video.palette[PAL_GENTXT];
    const char **names=(const char **)(config.pattDots?
        (config.accidental?noteNames4:noteNames3):(config.accidental?noteNames2:noteNames1));
    const note_t *pattern=song->patterns[song->currPattern];
    if(!pattern || !trackerFrameBMP)return;
    fillRect(0,0,SCREEN_W,SCREEN_H,bg);
    drawFramework1(0,0,SCREEN_W,11);
    textOut(3,2,"POS",text);printThreeDecimals(27,2,song->currPos,text);
    textOut(64,2,"PAT",text);printTwoDecimals(88,2,song->currPattern,text);
    textOut(120,2,"LEN",text);printThreeDecimals(144,2,song->header.songLength,text);
    textOut(184,2,"SMP",text);printTwoHex(208,2,editor.currSample+1,text);
    textOut(240,2,ppt_live_edit_active()?"LIVE":"EDIT",text);
    const int first=firstVisibleRow(),ghost=ppt_ghost_row(song->currPattern);
    int y=12,currentY=12;
    for(int i=0;i<32;i++){
        const int row=first+i;
        const bool current=row==song->currRow;
        const int height=current?14:7,ty=y+(current?2:0);
        /* Native bevels and the active row's horizontal/vertical junctions. */
        blit32(0,y,SCREEN_W,height,&trackerFrameBMP[(current?188:140)*SCREEN_W]);
        if(current)currentY=y;
        const uint32_t rowBg=current?bar:(row==ghost?0x00333333:bg);
        const uint32_t rowFg=current?text:fg;
        if(current)printTwoDecimalsBigBg(8,ty,row,rowFg,rowBg);
        else printTwoDecimalsBg(8,ty,row,rowFg,rowBg);
        for(int channel=0;channel<4;channel++){
            const note_t *n=&pattern[row*4+channel];
            const int x=32+channel*72;
            uint32_t fieldBg=ppt_selection_contains(song->currPattern,row,channel,0)?0x00555555:rowBg;
            if(current)textOutBigBg(x,ty,names[noteIndex(n->period)],rowFg,fieldBg);
            else textOutBg(x,ty,names[noteIndex(n->period)],rowFg,fieldBg);
            const uint8_t digits[5]={n->sample>>4,n->sample&15,n->command&15,n->param>>4,n->param&15};
            for(int j=0;j<5;j++){
                fieldBg=ppt_selection_contains(song->currPattern,row,channel,j+1)?0x00555555:rowBg;
                char ch=hexTable[digits[j]];
                if(j==0 && !digits[j] && config.blankZeroFlag)ch=' ';
                if(config.pattDots && ((j<2 && !n->sample) || (j>=2 && !n->command && !n->param)))ch=2;
                if(current)charOutBigBg(x+24+j*8,ty,ch,rowFg,fieldBg);
                else charOutBg(x+24+j*8,ty,ch,rowFg,fieldBg);
            }
        }
        y+=height;
    }
    const int field=cursor.pos%6;
    drawNativeCursor(30+(cursor.pos/6)*72+(field==0?0:(field+2)*8),currentY);
    drawFramework1(0,244,SCREEN_W,11);
    textOut(3,247,"STATUS:",text);textOutN(64,247,ui.statusMessage,17,text);
    drawNativeMouse();
}
void ppt_views_begin(void)
{
    overlaySaved=false;
    const bool full=ppt_full_pattern_active();
    const int row=(!full && !ui.askBoxShown && !ui.editTextFlag)?ppt_disk_selected_row():-1;
    const bool sample=ui.diskOpScreenShown && diskop.mode==DISKOP_MODE_SMP && !ui.askBoxShown && !ui.editTextFlag;
    if(!full && row<0 && !sample)return;
    memcpy(savedFrame,video.frameBuffer,sizeof(savedFrame));overlaySaved=true;
    if(full){fullscreenPattern();return;}
    /* Solid soft bar only: no contrasting one-pixel underline. */
    if(sample)ppt_browser_draw();
    if(row<0){drawNativeMouse();return;}
    const int y=35+row*6;
    const uint32_t bg=video.palette[PAL_BACKGRD];
    for(int yy=y;yy<y+6;yy++)for(int xx=8;xx<303;xx++){
        uint32_t *pixel=&video.frameBuffer[yy*SCREEN_W+xx];
        if((*pixel&0xFFFFFF)==(bg&0xFFFFFF))*pixel=0x00333333;
    }
    drawNativeMouse();
}
void ppt_views_end(void)
{
    if(overlaySaved){memcpy(video.frameBuffer,savedFrame,sizeof(savedFrame));overlaySaved=false;}
}
