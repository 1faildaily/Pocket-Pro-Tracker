#include "ppt_bpm.h"
/* Full view uses PT2's original frame slices, bitmap font and cursor assets.
   Presentation only: framebuffer restored immediately after texture upload. */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "pt2_header.h"
#include "pt2_structs.h"
#include "pt2_palette.h"
#include "pt2_config.h"
#include "pt2_tables.h"
#include "pt2_textout.h"
#include "pt2_visuals.h"
#include "pt2_bmp.h"
#include "pt2_scopes.h"
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
/* Reuse the native QUADRASCOPE renderer and its existing synchronization.
   Capture only rendered points; never read PCM pointers or change scope state. */
static int scopeTrace[4][SCOPE_WIDTH];
static void capturePatternScopes(void)
{
    drawScopes();
    for(int ch=0;ch<4;ch++)for(int x=0;x<SCOPE_WIDTH;x++){
        scopeTrace[ch][x]=-1;
        if(!scope[ch].active || !scope[ch].volume || editor.muted[ch])continue;
        for(int y=0;y<SCOPE_HEIGHT;y++){
            if((video.frameBuffer[(55+y)*SCREEN_W+128+ch*(SCOPE_WIDTH+8)+x]&0xFFFFFF)==(video.palette[PAL_QADSCP]&0xFFFFFF)){
                scopeTrace[ch][x]=197+y;break;
            }
        }
    }
}
static void scopeBackgroundPixel(int x,int y,uint32_t bg)
{
    /* Text, selection, bevels and playback bar always keep their own pixels. */
    if(y>=12 && y<244 && (video.frameBuffer[y*SCREEN_W+x]&0xFFFFFF)==(bg&0xFFFFFF))
        video.frameBuffer[y*SCREEN_W+x]=0x00FF5555;
}
static void drawPatternScopes(uint32_t bg)
{
    for(int ch=0;ch<4;ch++)for(int i=1;i<SCOPE_WIDTH;i++){
        if(scopeTrace[ch][i-1]<0 || scopeTrace[ch][i]<0)continue;
        int x=32+ch*72+12+(i-1),y=scopeTrace[ch][i-1];
        const int endX=32+ch*72+12+i,endY=scopeTrace[ch][i];
        int dx=endX-x,dy=abs(endY-y),sy=y<endY?1:-1,error=dx-dy;
        for(;;){
            scopeBackgroundPixel(x,y,bg);
            if(x==endX && y==endY)break;
            int e2=error*2;if(e2>-dy){error-=dy;x++;}if(e2<dx){error+=dx;y+=sy;}
        }
    }
}
/* POSITION's existing empty frame: 0,99 .. 119,243.
   All channels share one plotting area; borders and transport remain native. */
static void drawPositionScopes(void)
{
    static const uint32_t colors[4]={0x888888,0xFF5555,0xFFFFFF,0x880000};
    for(int ch=0;ch<4;ch++)for(int i=1;i<SCOPE_WIDTH;i++){
        if(scopeTrace[ch][i-1]<0 || scopeTrace[ch][i]<0)continue;
        int x=2+(i-1)*115/(SCOPE_WIDTH-1),y=101+(scopeTrace[ch][i-1]-197)*140/(SCOPE_HEIGHT-1);
        const int endX=2+i*115/(SCOPE_WIDTH-1),endY=101+(scopeTrace[ch][i]-197)*140/(SCOPE_HEIGHT-1);
        int dx=endX-x,dy=abs(endY-y),sy=y<endY?1:-1,error=dx-dy;
        for(;;){
            if(x>=2 && x<=117 && y>=101 && y<=241)video.frameBuffer[y*SCREEN_W+x]=colors[ch];
            if(x==endX && y==endY)break;
            int e2=error*2;if(e2>-dy){error-=dy;x++;}if(e2<dx){error+=dx;y+=sy;}
        }
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
    capturePatternScopes();
    fillRect(0,0,SCREEN_W,SCREEN_H,bg);
    drawFramework1(0,0,SCREEN_W,11);
    textOut(3,2,"POS",text);printThreeDecimals(27,2,song->currPos,text);
    textOut(64,2,"PAT",text);printTwoDecimals(88,2,song->currPattern,text);
    textOut(120,2,"LEN",text);printThreeDecimals(144,2,song->header.songLength,text);
    textOut(184,2,"SMP",text);printTwoHex(208,2,editor.currSample+1,text);
    textOut(240,2,editor.currMode==MODE_RECORD?"REC":ppt_live_edit_active()?"LIVE":"EDIT",text);
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
        const uint32_t rowFg=(current || row==ghost)?text:fg;
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
    drawPatternScopes(bg);
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
    const bool modList=ui.diskOpScreenShown && diskop.mode==DISKOP_MODE_MOD ;
    const int row=(!full && !ui.askBoxShown && !ui.editTextFlag)?ppt_disk_selected_row():-1;
    const bool bpm=ui.editOpScreenShown && ui.editOpScreen==2 && !ui.askBoxShown && !ui.diskOpScreenShown && !ui.posEdScreenShown;
    const bool position=ui.posEdScreenShown && !ui.diskOpScreenShown && !ui.askBoxShown && (!ui.editTextFlag || textEdit.object==PTB_PE_PATTNAME);
    const bool samplerNote=ui.samplerScreenShown && !ui.askBoxShown && !ui.changingSmpResample;
    const bool sample=ui.diskOpScreenShown && diskop.mode==DISKOP_MODE_SMP;
    if(!full && row<0 && !sample && !modList && !bpm && !position && !samplerNote)return;
    memcpy(savedFrame,video.frameBuffer,sizeof(savedFrame));overlaySaved=true;
    if(full){fullscreenPattern();return;}
    if(position){
        capturePatternScopes();
        /* Native scope drawing also touches the EXIT strip. Keep only captured
           trace data and restore every UI pixel before composing POSITION. */
        memcpy(video.frameBuffer,savedFrame,sizeof(savedFrame));
        ppt_position_draw_full();drawPositionScopes();drawNativeMouse();return;
    }
    if(modList){
        ppt_disk_draw_full();
        if(ui.editTextFlag && textEdit.object==PTB_SONGNAME)
            fillRect(104+textEdit.cursorBlock*8,242,7,2,video.palette[PAL_TEXTMARK]);
        if(ui.askBoxShown)ppt_mod_restore_dialog(savedFrame);
        drawNativeMouse();return;
    }
    /* Solid soft bar only: no contrasting one-pixel underline. */
    if(sample){
        if(ui.askBoxShown && ppt_browser_import_dialog()){
            fillRect(0,136,320,119,video.palette[PAL_GENBKG]);
            ppt_browser_draw_import_dialog();
        }else ppt_browser_draw();
    }
    if(bpm)ppt_bpm_draw();
    if(samplerNote){
        int pitch=ppt_browser_sample_note();
        textOutBg(288,236,pitch<0?"---":(config.accidental?noteNames2:noteNames1)[pitch+2],video.palette[PAL_GENTXT],video.palette[PAL_GENBKG]);
    }
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
