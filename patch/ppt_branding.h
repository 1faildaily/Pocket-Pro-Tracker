/* Static bitmap labels; same font, colors and one-pixel shadow as charOut2(). */
static void pptLabelClear(uint32_t *bmp, int stride, int x, int y, int w, int h)
{
    for(int yy=y;yy<y+h;yy++)for(int xx=x;xx<x+w;xx++)
        bmp[yy*stride+xx]=video.palette[PAL_GENBKG];
}
static void pptLabelText(uint32_t *bmp,int stride,int x,int y,const char *text)
{
    for(;*text;text++,x+=8){
        const uint8_t *glyph=&fontBMP[((uint8_t)*text&127)*8];
        for(int yy=0;yy<5;yy++)for(int xx=0;xx<8;xx++)if(glyph[yy*127*8+xx]){
            bmp[(y+yy+1)*stride+x+xx+1]=video.palette[PAL_GENBKG2];
            bmp[(y+yy)*stride+x+xx]=video.palette[PAL_BORDER];
        }
    }
}
static void pptBranding(void)
{
    pptLabelClear(trackerFrameBMP,320,122,89,196,9);
    pptLabelText(trackerFrameBMP,320,128,91,"POCKET PRO TRACKER");
    pptLabelClear(posEdBMP,200,53,2,94,18);
    pptLabelText(posEdBMP,200,68,5,"POSITION");
    pptLabelText(posEdBMP,200,84,12,"EDIT");
    pptLabelClear(diskOpScreenBMP,320,2,2,76,18);
    pptLabelText(diskOpScreenBMP,320,24,5,"DISK");
    pptLabelText(diskOpScreenBMP,320,8,12,"OPERATOR");
}
