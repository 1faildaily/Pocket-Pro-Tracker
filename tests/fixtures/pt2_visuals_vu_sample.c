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

void updateSampler(void)
{
	if (ui.updateResampleNote)
	{
		ui.updateResampleNote = false;
		if (ui.changingSmpResample)
		{
			textOutBg(288, 236, "---", video.palette[PAL_GENTXT], video.palette[PAL_GENBKG]);
		}
		else
		{
			ASSERT(editor.resampleNote < 36);
			textOutBg(288, 236,
				config.accidental ? noteNames2[2+editor.resampleNote] : noteNames1[2+editor.resampleNote],
				video.palette[PAL_GENTXT], video.palette[PAL_GENBKG]);
		}
	}
}

void flipFrame(void)
{
	SDL_UpdateTexture(video.texture, NULL, video.frameBuffer, SCREEN_W * sizeof (int32_t));
}
