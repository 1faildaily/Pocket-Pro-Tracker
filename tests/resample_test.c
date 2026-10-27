#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "../patch/ppt_wav_resample.h"
int main(void){
    const double paula=28375160.0/8.0;
    const double rates[2]={paula/214.0,paula/113.0};
    float *source=malloc(44100*sizeof(float));assert(source);
    for(int note=0;note<2;note++){
        int n=ppt_resample_length(44100,44100,rates[note]);
        assert(n%2==0 && fabs(n/rates[note]-1.0)<2.0/rates[note]);
        int8_t *out=malloc(n);assert(out);
        for(int i=0;i<44100;i++)source[i]=(float)(0.5*sin(2*PPT_PI*440*i/44100));
        ppt_resample_pcm(source,44100,44100,rates[note],out,n);
        int crosses=0;
        for(int i=1;i<n;i++)if(out[i-1]<=0 && out[i]>0)crosses++;
        assert(abs(crosses-440)<=2);
        /* Anti-alias: a tone above the new Nyquist should be strongly attenuated. */
        for(int i=0;i<44100;i++)source[i]=(float)(0.5*sin(2*PPT_PI*20000*i/44100));
        ppt_resample_pcm(source,44100,44100,rates[note],out,n);
        double rms=0;
        for(int i=64;i<n-64;i++)rms+=(double)out[i]*out[i];
        rms=sqrt(rms/(n-128));assert(rms<3.0);
        free(out);
    }
    assert(ppt_resample_length(441000,44100,rates[0])>131070);
    free(source);puts("C-3/B-3: duration, 440Hz pitch, even lengths, anti-alias and oversize OK");
}
