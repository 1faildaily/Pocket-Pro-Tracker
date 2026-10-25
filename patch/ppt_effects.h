#pragma once
/* PPT v0.3.9 effect reference: parameters are stored as raw MOD hex bytes,
   except Dxx which encodes a DECIMAL row in BCD. Only pad writes and
   field-level pastes are range limited; loading a historical MOD never
   modifies its data. Names fit the PT2 17-character STATUS display. */
#include <stdint.h>

static const char *pptEffectName(uint8_t cmd, uint8_t param){
  static const char *const base[16]={
    "0 ARPEGGIO", "1 SLIDE UP", "2 SLIDE DOWN", "3 TONE PORTA",
    "4 VIBRATO", "5 PORTA+VOL SL", "6 VIB+VOL SL", "7 TREMOLO",
    "8 UNUSED", "9 SAMPLE OFFSET", "A VOLUME SLIDE", "B POSITION JUMP",
    "C SET VOLUME", "D PATTERN BREAK", "E EXTENDED", "F SPEED / BPM"};
  static const char *const ex[16]={
    "E0 LED FILTER", "E1 FINE SLIDE UP", "E2 FINE SLIDE DN", "E3 GLISSANDO",
    "E4 VIBRATO WAVE", "E5 FINETUNE", "E6 PATTERN LOOP", "E7 TREMOLO WAVE",
    "E8 KARPLUS STR", "E9 RETRIGGER", "EA FINE VOL UP", "EB FINE VOL DN",
    "EC NOTE CUT", "ED NOTE DELAY", "EE PATT DELAY", "EF INVERT LOOP"};
  return (cmd&15)==0xE?ex[(param>>4)&15]:base[cmd&15];
}
static uint8_t pptEffectClamp(uint8_t cmd, int value){
  if(value<0)value=0;
  if(value>255)value=255;
  switch(cmd&15){
    case 8: return 0; /* reference: unused effect 8 */
    case 0xB: return (uint8_t)(value>0x7F?0x7F:value);
    case 0xC: return (uint8_t)(value>0x40?0x40:value);
    case 0xD:{
      int tens=(value>>4)&15, ones=value&15;
      if(tens>6)return 0x63;
      if(ones>9)ones=9;
      if(tens==6 && ones>3)ones=3;
      return (uint8_t)((tens<<4)|ones);
    }
    case 0xE:{
      const int sub=(value>>4)&15;
      int x=value&15;
      /* E4/E7: 0..3 retrigger, 4..7 no-retrigger waveforms. */
      if(sub==3) {if(x>1)x=1;}
      if(sub==4 || sub==7) {if(x>7)x=7;}
      /* E0: 0=on, 1..F=off. E8: all x values accepted and equivalent. */
      return (uint8_t)((sub<<4)|x);
    }
    default:return (uint8_t)value;
  }
}
static uint8_t pptEffectStep(uint8_t cmd,uint8_t param,int field,int dir){
  /* Fields: PARAM1=high nibble, PARAM2=low nibble; up/down move
     selected digit while LEFT/RIGHT fine tune by one unit. */
  const int sign=(dir==0 || dir==3)?1:-1;
  const int high=field==4;
  if((cmd&15)==0xD){
    const int valid=pptEffectClamp(cmd,param);
    const int old=(valid>>4)*10+(valid&15);
    const int change=(dir<2 && high)?10:1;
    int next=old+sign*change;
    if(next<0)next=0;
    if(next>63)next=63;
    return (uint8_t)(((next/10)<<4)|(next%10));
  }
  const int change=(dir<2 && high)?16:1;
  int value=(int)param+sign*change;
  if(value<0)value=0;
  if(value>255)value=255;
  /* For E commands, changing the high nibble changes which extended
     command is selected; keep high digit inside [0..F], reset invalid
     low digits rather than wrapping to a different subcommand. */
  return pptEffectClamp(cmd,value);
}
