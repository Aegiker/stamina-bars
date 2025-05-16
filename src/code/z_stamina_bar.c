#include "array_count.h"
#include "attributes.h"
#include "controller.h"
#include "flag_set.h"
#include "gfx.h"
#include "gfx_setupdl.h"
#include "language_array.h"
#include "main.h"
#include "map.h"
#include "printf.h"
#include "regs.h"
#include "segment_symbols.h"
#include "segmented_address.h"
#include "sequence.h"
#include "sfx.h"
#include "sys_matrix.h"
#include "terminal.h"
#include "translation.h"
#include "versions.h"
#include "z64audio.h"
#include "z64lifemeter.h"
#include "z64horse.h"
#include "z64ocarina.h"
#include "z64play.h"
#include "z64player.h"
#include "z64save.h"
#include "libu64/gfxprint.h"
#include "libc64/sprintf.h"
#include "gfxalloc.h"
#include "z64staminabars.h"

#include "assets/textures/parameter_static/parameter_static.h"
#include "assets/textures/do_action_static/do_action_static.h"
#include "assets/textures/icon_item_static/icon_item_static.h"

// leftmost 8 bits are the number of stamina bars. If this number does not match then the max is invalid and needs to be updated
static u32 sMaxMagic = 0;

// don't @ me about the number of times the save data is accessed, it should already be in the data cache
// checks if the leftmost 8 bits are the count of stamina bars, if true then the maximum has already been calculated
// otherwise the value is invalid and needs to be recalculated
// this is only expensive one time, bitwise operations are much faster
u16 StaminaBar_GetMax(void) {
    s32 i;
    u16 max;
    if (((sMaxMagic >> 24) & 0xFF) != gSaveContext.save.info.playerData.staminaBars) {
        for (i = 0; i < gSaveContext.save.info.playerData.staminaBars; i++) {
            max += STAMINA_PER_BAR(i);
        }
        sMaxMagic = ((gSaveContext.save.info.playerData.staminaBars << 24) | (max & 0xFFFF));

    }

    return (u16)(sMaxMagic & 0xFFFF);
}

// returns the number of filled stamina bars, which is also the index of the last not full stamina bar
// also sets lastBarFillAmount to the amount of fill of the last not empty stamina bar
// each bar before the return index is full and each bar after it is empty
u8 StaminaBar_GetFullBarCount(f32* lastBarFillAmount, u32* maxStamina) {
    s32 i;
    s32 max = 0;

    for (i = 0; i < gSaveContext.save.info.playerData.staminaBars; i++) {
        if ((gSaveContext.save.info.playerData.stamina ) > (max + STAMINA_PER_BAR(i))) {
            max += STAMINA_PER_BAR(i);
        } else {
            *lastBarFillAmount = 100.0f - ((((max + STAMINA_PER_BAR(i)) - gSaveContext.save.info.playerData.stamina) / (f32)STAMINA_PER_BAR(i)) * 100.0f);
            *maxStamina = max;
            return i;
        }
    }

    *maxStamina = max;
    *lastBarFillAmount = 100.0f; // all bars are full
    return i;
}

void StaminaBar_DrawBar(PlayState* play, s16 x, s16 y, s16 length, f32 fill) {
    InterfaceContext* interfaceCtx = &play->interfaceCtx;

    OPEN_DISPS(play->state.gfxCtx, "../z_parameter.c", 2650);

    Gfx_SetupDL_39Overlay(play->state.gfxCtx);

    gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 255, 255, 255, interfaceCtx->magicAlpha);
    gDPSetEnvColor(OVERLAY_DISP++, 100, 50, 50, 255);

    OVERLAY_DISP = Gfx_TextureIA8(OVERLAY_DISP, gMagicMeterEndTex, 8, 16, x, y, 8, 16,
                                    1 << 10, 1 << 10);

    OVERLAY_DISP = Gfx_TextureIA8(OVERLAY_DISP, gMagicMeterMidTex, 24, 16, x + 8, y,
                                    length, 16, 1 << 10, 1 << 10);

    gDPLoadTextureBlock(OVERLAY_DISP++, gMagicMeterEndTex, G_IM_FMT_IA, G_IM_SIZ_8b, 8, 16, 0,
                        G_TX_MIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, 3, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);

    gSPTextureRectangle(OVERLAY_DISP++, (x + 8 + length) << 2, y << 2,
                        (x + 8 + length + 8) << 2, (y + 16) << 2,
                        G_TX_RENDERTILE, 256, 0, 1 << 10, 1 << 10);

    gDPPipeSync(OVERLAY_DISP++);
    gDPSetCombineLERP(OVERLAY_DISP++, PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, 0, 0, 0, PRIMITIVE, PRIMITIVE,
                        ENVIRONMENT, TEXEL0, ENVIRONMENT, 0, 0, 0, PRIMITIVE);
    gDPSetEnvColor(OVERLAY_DISP++, 0, 0, 0, 255);

    // Fill the whole meter with the normal magic color
    gDPSetPrimColor(OVERLAY_DISP++, 0, 0, STAMINA_BAR_R, STAMINA_BAR_G, STAMINA_BAR_B,
                    interfaceCtx->magicAlpha);

    gDPLoadMultiBlock_4b(OVERLAY_DISP++, gMagicMeterFillTex, 0x0000, G_TX_RENDERTILE, G_IM_FMT_I, 16, 16, 0,
                            G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK,
                            G_TX_NOLOD, G_TX_NOLOD);

    gSPTextureRectangle(OVERLAY_DISP++, (x + 8) << 2, (y + 3) << 2,
                        ((x + 8) + (s32)(length * (fill / 100.0f))) << 2,
                        (y + 10) << 2, G_TX_RENDERTILE, 0, 0, 1 << 10, 1 << 10);

    CLOSE_DISPS(play->state.gfxCtx, "../z_parameter.c", 2731);
}

void StaminaBar_DrawAll(PlayState* play) {
    s16 x = R_MAGIC_METER_X;
    s16 y = R_MAGIC_METER_Y_LOWER + 12;
    u32 i;
    f32 incompleteBarFill = 0.0f;
    u32 maxStamina;
    u8 currentIncompleteBar = StaminaBar_GetFullBarCount(&incompleteBarFill, &maxStamina);

    for (i = 0; i < gSaveContext.save.info.playerData.staminaBars; i++) {
        u8 barLength = STAMINA_PER_BAR(i) * 0.4f;
        if (i == currentIncompleteBar) {
            StaminaBar_DrawBar(play, x, y, barLength, incompleteBarFill);
        } else if (i > currentIncompleteBar) {
            StaminaBar_DrawBar(play, x, y, barLength, 0.0f);
        } else {
            StaminaBar_DrawBar(play, x, y, barLength, 100.0f);
        }
        x += ((barLength / 2) + 6);
        y += 6;
    }
}

void StaminaBar_Update(PlayState* play) {
    gSaveContext.save.info.playerData.stamina = CLAMP(gSaveContext.save.info.playerData.stamina + STAMINA_REFILL_RATE, 0, StaminaBar_GetMax());
}
