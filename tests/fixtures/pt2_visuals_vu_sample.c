#include "pt2_visuals.h"
void renderVuMeters(void)
{
    if (ui.samplerScreenShown) return;
    fillToVuMetersBgBuffer();
    /* source function retained for future reversion */
}
void renderSprites(void)
{
	renderVuMeters(); // let's put it here even though it's not sprite-based
	for (int32_t i = 0; i < SPRITE_NUM; i++)
	{
		/* sprite code unchanged */
	}
}
