#include "pt2_posed.h"
void updatePosEd(void){
  for(int32_t i=0;i<POSED_LIST_SIZE;i++){
    const int32_t entry=song->currPos-(5-i);
    const int32_t yPos=23+(i*(FONT_CHAR_H+1));
    const uint32_t textColor=0;
		printThreeDecimals(128, yPos, entry, textColor);
  }
}
