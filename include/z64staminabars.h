#ifndef STAMINA_BARS_H
#define STAMINA_BARS_H

#include "z64play.h"

#define STAMINA_BAR_R 255
#define STAMINA_BAR_G 165
#define STAMINA_BAR_B 0

// each stamina bar has less stamina than the previous stamina bar
#define STAMINA_PER_BAR(bar) (100 / (bar + 1))

#define STAMINA_REFILL_RATE 2

void StaminaBar_DrawAll(PlayState* play);
void StaminaBar_Update(PlayState* play);

// doesn't belong here
Gfx* Gfx_TextureIA8(Gfx* displayListHead, void* texture, s16 textureWidth, s16 textureHeight, s16 rectLeft, s16 rectTop,
    s16 rectWidth, s16 rectHeight, u16 dsdx, u16 dtdy);

#endif
