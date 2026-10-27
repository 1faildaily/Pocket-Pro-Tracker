#pragma once
#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#define PPT_PI 3.14159265358979323846
static int ppt_resample_length(int frames,double from,double to){
    double n=round((double)frames*to/from);
    if(n<2 || n>2147483644.0)return 0;
    int length=(int)n;return (length+1)&~1;
}
/* Windowed sinc low-pass before rate reduction. Float input, signed 8-bit MOD output. */
static void ppt_resample_pcm(const float *source,int frames,double from,double to,int8_t *out,int length){
    const double ratio=from/to,cutoff=fmin(1.0,to/from)*0.94;
    const int radius=32;
    for(int i=0;i<length;i++){
        const double pos=(double)i*ratio;const int center=(int)floor(pos);
        double sum=0,weight=0;
        for(int j=center-radius+1;j<=center+radius;j++){
            const double distance=(double)j-pos;
            if(fabs(distance)>=radius)continue;
            const double arg=PPT_PI*distance*cutoff;
            const double sinc=fabs(arg)<1e-12?1.0:sin(arg)/arg;
            const double w=cutoff*sinc*(0.5+0.5*cos(PPT_PI*distance/radius));
            const int index=j<0?0:j>=frames?frames-1:j;
            sum+=(double)source[index]*w;weight+=w;
        }
        double v=weight!=0?sum/weight:0;
        if(!isfinite(v))v=0;
        int value=(int)lrint(fmax(-1.0,fmin(1.0,v))*128.0);
        if(value>127)value=127;
        if(value<-128)value=-128;
        out[i]=(int8_t)value;
    }
}
