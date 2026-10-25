#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "../patch/ppt_scale.h"
int main(void)
{
    uint32_t *src=malloc(320*255*4);
    uint32_t *dst=malloc(640*480*4);
    assert(src && dst);
    for (int y=0;y<255;y++) for(int x=0;x<320;x++) src[y*320+x]=0xFF0044DDu;
    PPTScaleRect r={0};
    assert(ppt_compose_frame(src,320,255,dst,640,480,&r));
    assert(r.x==19 && r.y==0 && r.w==602 && r.h==480);
    for(int y=0;y<480;y++) {
        assert(dst[y*640+18]==0xFF000000u);
        assert(dst[y*640+19]==0xFF0044DDu);
        assert(dst[y*640+620]==0xFF0044DDu);
        assert(dst[y*640+621]==0xFF000000u);
    }
    for(int x=19;x<621;x++) {
        assert(dst[x] == 0xFF0044DDu); // no top black bar
        assert(dst[479*640+x] == 0xFF0044DDu); // no bottom black bar
    }
    free(src);free(dst);
    puts("PPT v0.2.9 frame composition OK: black only at x 0-18 and 621-639");
    return 0;
}
