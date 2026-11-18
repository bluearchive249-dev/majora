// Copyright (c) 2026 Grimsfrost. All rights reserved. See AI-POLICY.md in the Blastwright repository.
#ifndef GDF_COMPAT_H
#define GDF_COMPAT_H

// Play as Ganondorf (2S2H): the Ship of Harkinian mod's code (z_boss_ganon.c, "Play as Ganondorf") is compiled here
// mostly as it is there. This maps the names it uses from Ocarina of Time's engine to Majora's Mask's.

#include "global.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ------------------------------------------------------------------------------------------------
// Link (Player)
// ------------------------------------------------------------------------------------------------

#define linearVelocity speedXZ       // his speed
#define unk_6AD unk_AA5              // first person / camera modes
#define unk_862 getItemDrawIdPlusOne // the item he holds up

// Ocarina of Time's state flags, by what they mean, as Majora's Mask numbers them
#define PLAYER_STATE1_CLIMBING_LEDGE PLAYER_STATE1_4
#define PLAYER_STATE1_INPUT_DISABLED PLAYER_STATE1_20
#define PLAYER_STATE1_GETTING_ITEM PLAYER_STATE1_400
#define PLAYER_STATE1_HANGING_OFF_LEDGE PLAYER_STATE1_2000
#define PLAYER_STATE1_FIRST_PERSON PLAYER_STATE1_100000
#define PLAYER_STATE1_CLIMBING_LADDER PLAYER_STATE1_200000
#define PLAYER_STATE1_ON_HORSE PLAYER_STATE1_800000
#define PLAYER_STATE1_IN_WATER PLAYER_STATE1_8000000
#define PLAYER_STATE1_IN_ITEM_CS PLAYER_STATE1_10000000
#define PLAYER_STATE1_IN_CUTSCENE PLAYER_STATE1_20000000
#define PLAYER_STATE2_FOOTSTEP PLAYER_STATE2_8
#define PLAYER_STATE2_CRAWLING PLAYER_STATE2_40000
#define PLAYER_STATE2_OCARINA_PLAYING PLAYER_STATE2_USING_OCARINA
#define PLAYER_STATE2_DISABLE_DRAW PLAYER_STATE2_20000000

// ------------------------------------------------------------------------------------------------
// Collision
// ------------------------------------------------------------------------------------------------

#define ColliderInfo ColliderElement
#define COLTYPE_NONE COL_MATERIAL_NONE
#define COLTYPE_HIT3 COL_MATERIAL_HIT3
#define COLTYPE_METAL COL_MATERIAL_METAL
#define ELEMTYPE_UNK0 ELEM_MATERIAL_UNK0
#define ELEMTYPE_UNK2 ELEM_MATERIAL_UNK2
#define ELEMTYPE_UNK6 ELEM_MATERIAL_UNK6
#define TOUCH_NONE ATELEM_NONE
#define TOUCH_ON ATELEM_ON
#define TOUCH_SFX_NORMAL ATELEM_SFX_NORMAL
#define TOUCH_SFX_NONE ATELEM_SFX_NONE
#define BUMP_NONE ACELEM_NONE
#define BUMP_ON ACELEM_ON
#define BUMP_HOOKABLE ACELEM_HOOKABLE

// ------------------------------------------------------------------------------------------------
// Sounds: Majora's Mask has no Ganondorf, so his voice and magic use sounds it does have (Igos du Ikana's voice,
// Majora's magic) for now
// ------------------------------------------------------------------------------------------------

#define NA_SE_EN_GANON_LAUGH NA_SE_EN_BOSU_LAUGH
#define NA_SE_EN_GANON_DAMAGE1 NA_SE_EN_BOSU_DAMAGE
#define NA_SE_EN_GANON_DAMAGE2 NA_SE_EN_BOSU_DAMAGE
#define NA_SE_EN_GANON_THROW NA_SE_EN_BOSU_ATTACK
#define NA_SE_EN_GANON_THROW_BIG NA_SE_EN_BOSU_ATTACK
#define NA_SE_EN_GANON_AT_RETURN NA_SE_EN_BOSU_SWORD
#define NA_SE_EN_GANON_THROW_MASIC NA_SE_EN_LAST2_FIRE_OLD
#define NA_SE_EN_GANON_CHARGE_MASIC NA_SE_EN_LAST1_BEAM_OLD
#define NA_SE_EN_GANON_HIT_THUNDER NA_SE_EN_COMMON_THUNDER
#define NA_SE_EN_GANON_SPARK NA_SE_EN_BOSU_SHOCK
#define NA_SE_EN_GANON_FIRE NA_SE_EN_LAST2_FIRE_OLD
#define NA_SE_EN_GANON_WAVE_GND NA_SE_EN_LAST3_ROD_HARD
#define NA_SE_EN_GANON_DARKWAVE NA_SE_EN_LAST1_BEAM_OLD
#define NA_SE_EN_GANON_BIGMASIC NA_SE_EN_LAST1_BEAM_OLD
#define NA_SE_EN_GANON_HIT_GND NA_SE_EN_IRONNACK_HIT_GND
#define NA_SE_EN_GANON_HIT_GND_IMP NA_SE_IT_BOMB_EXPLOSION
#define NA_SE_EV_GANON_MANTLE NA_SE_EV_WIND_TRAP
#define NA_SE_EN_FANTOM_FLOAT NA_SE_EN_LAST1_FLOAT_OLD
#define NA_SE_EN_FANTOM_THUNDER NA_SE_EN_COMMON_THUNDER
#define NA_SE_EV_TRIFORCE_MARK NA_SE_EV_TRIFORCE
#define NA_SE_EN_FANTOM_LAUGH NA_SE_EN_GANON_LAUGH
#define NA_SE_EN_FANTOM_DAMAGE2 NA_SE_EN_FANTOM_DAMAGE
#define NA_SE_EN_FANTOM_SPARK NA_SE_EN_GANON_SPARK
#define NA_SE_EN_FANTOM_MASIC2 NA_SE_EN_GANON_THROW_MASIC
#define NA_SE_EN_FANTOM_HIT_THUNDER NA_SE_EN_GANON_HIT_THUNDER
#define NA_SE_EN_FANTOM_FIRE NA_SE_EN_GANON_FIRE

// His animation "objects" (his animations are read by name, see BossGanon_SetAnimationObject)
#define OBJECT_GANON_ANIME1 0
#define OBJECT_GANON_ANIME2 0
#define OBJECT_GANON_ANIME3 0

// ------------------------------------------------------------------------------------------------
// 2Ship
// ------------------------------------------------------------------------------------------------

void FrameInterpolation_RecordOpenChild(const void* a, int b);

// ------------------------------------------------------------------------------------------------
// Engine functions by their Ocarina of Time names
// ------------------------------------------------------------------------------------------------

#define Matrix_RotateX Matrix_RotateXF
#define Matrix_RotateY Matrix_RotateYF
#define Matrix_RotateZ Matrix_RotateZF
#define MATRIX_NEWMTX(gfxCtx) Matrix_Finalize(gfxCtx)
#define MATRIX_TOMTX(dest) Matrix_ToMtx(dest)
#define Graph_Alloc(gfxCtx, size) GRAPH_ALLOC(gfxCtx, size)
#define Gfx_SetupDL_25Opa Gfx_SetupDL25_Opa
#define Gfx_SetupDL_25Xlu Gfx_SetupDL25_Xlu
#define SEGMENTED_TO_VIRTUAL(addr) Lib_SegmentedToVirtual((void*)(addr))
#define RADF_TO_BINANG RAD_TO_BINANG
void SkelAnime_InterpFrameTable(s32 limbCount, Vec3s* dst, Vec3s* start, Vec3s* target, f32 weight);

// ------------------------------------------------------------------------------------------------
// Parts of the Ship of Harkinian mod that aren't in this port (yet)
// ------------------------------------------------------------------------------------------------

// Invasion mode (online)
#define GdfInvasion_IsPacified() false
#define GdfInvasion_IsVanished() false
#define GdfInvasion_HandleCButtons(play) false
#define GdfInvasion_PayAttack(...) true
#define GdfInvasion_DummyClientId(actor) 0
#define GdfInvasion_CanHurtLinks() false
#define GdfInvasion_CanHurtLink(...) false
#define GdfInvasion_CanAct() true
#define GdfInvasion_BallHitGanondorf(...) false
// The organ and his songs, the dance, the strut's song, killing NPCs
#define GanondorfSongs_IsOcarinaOut() false
#define GanondorfDance_TakeRequest() false
#define GanondorfStrut_Strutting() ((void)0)
#define GanondorfKills_IsEnabled() false

// PlayAsGanondorf.cpp
s32 PlayAsGanondorf_KeepItems(void);
s32 PlayAsGanondorf_IsSprinting(void);
s32 PlayAsGanondorf_IsLevitating(void);
s32 PlayAsGanondorf_UseOrganOcarina(void);
s32 PlayAsGanondorf_OrganButtonPressed(void);
s32 PlayAsGanondorf_IsGanonOut(struct PlayState* play);
s32 PlayAsGanondorf_IsCrawlFlame(struct PlayState* play, struct Player* player);
void FrameInterpolation_RecordCloseChild(void);

#endif
