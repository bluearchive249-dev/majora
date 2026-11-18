// Copyright (c) 2026 Grimsfrost. All rights reserved. No copying, editing or additions, by people or AI tools, without
// the author's written consent. See AI-POLICY.md.
#include "z_player_gdf.h"
#include "gdf_compat.h"
#include "oot/overlays/ovl_Boss_Ganon/ovl_Boss_Ganon.h"
#include "overlays/actors/ovl_En_Ganon_Mant/z_en_ganon_mant.h"
#include "oot/objects/object_ganon/object_ganon.h"
#include "oot/objects/object_ganon_anime1/object_ganon_anime1.h"
#include "oot/objects/object_ganon_anime2/object_ganon_anime2.h"
#include "oot/objects/object_gndd/object_gndd.h"
#include <libultraship/bridge/resourcebridge.h>

// SoH: "Play as Ganondorf", Invasion mode: every sound the player's Ganondorf (or his light balls) makes is also noted
// for the other players, who hear it from his puppet (see GanondorfInvasion.cpp)
s32 PlayAsGanondorf_KeepItems(void);

#define Audio_PlayActorSound2 Actor_PlaySfx

#include <string.h>
#include <libultraship/bridge/consolevariablebridge.h>

#define FLAGS                                                                                 \
    (ACTOR_FLAG_ATTENTION_ENABLED | ACTOR_FLAG_HOSTILE | ACTOR_FLAG_UPDATE_CULLING_DISABLED | \
     ACTOR_FLAG_DRAW_CULLING_DISABLED)

void BossGanon_Init(Actor* thisx, PlayState* play);
void BossGanon_Destroy(Actor* thisx, PlayState* play);
void BossGanon_Update(Actor* thisx, PlayState* play);
void BossGanon_Draw(Actor* thisx, PlayState* play);
void func_808E1EB4(Actor* thisx, PlayState* play); // update
void func_808E2544(Actor* thisx, PlayState* play); // update
void BossGanon_LightBall_Update(Actor* thisx, PlayState* play);
void func_808E229C(Actor* thisx, PlayState* play); // draw
void func_808E324C(Actor* thisx, PlayState* play); // draw
void BossGanon_LightBall_Draw(Actor* thisx, PlayState* play);
void BossGanon_Reset(void);

void BossGanon_SetupIntroCutscene(BossGanon* this, PlayState* play);
void BossGanon_SetupTowerCutscene(BossGanon* this, PlayState* play);
void BossGanon_IntroCutscene(BossGanon* this, PlayState* play);
void BossGanon_DeathAndTowerCutscene(BossGanon* this, PlayState* play);
void BossGanon_Wait(BossGanon* this, PlayState* play);
void BossGanon_ChargeLightBall(BossGanon* this, PlayState* play);
void BossGanon_PlayTennis(BossGanon* this, PlayState* play);
void BossGanon_PoundFloor(BossGanon* this, PlayState* play);
void BossGanon_ChargeBigMagic(BossGanon* this, PlayState* play);
void BossGanon_Block(BossGanon* this, PlayState* play);
void BossGanon_HitByLightBall(BossGanon* this, PlayState* play);
void BossGanon_Vulnerable(BossGanon* this, PlayState* play);
void BossGanon_Damaged(BossGanon* this, PlayState* play);

void BossGanon_SetupWait(BossGanon* this, PlayState* play);
void BossGanon_SetupChargeLightBall(BossGanon* this, PlayState* play);
void BossGanon_SetupPlayTennis(BossGanon* this, PlayState* play);

void BossGanon_DrawEffects(PlayState* play);
void BossGanon_UpdateEffects(PlayState* play);

s32 BossGanon_CheckFallingPlatforms(BossGanon* this, PlayState* play, Vec3f* checkPos);

// SoH: "Play as Ganondorf"
// (also another player's Ganondorf in Invasion mode, drawn the same way: his puppet only takes its pose from the
// network)
#define GDF_IS_PLAYER(dorf) (((dorf)->actor.params == GDF_PARAMS_PLAYER) || ((dorf)->actor.params == GDF_PARAMS_PUPPET))
void BossGanon_Puppet_Init(BossGanon* this, PlayState* play);
void BossGanon_Puppet_Update(Actor* thisx, PlayState* play);
void BossGanon_PuppetBall_Init(BossGanon* this, PlayState* play);
void BossGanon_LightBall_Draw(Actor* thisx, PlayState* play);
void func_808E324C(Actor* thisx, PlayState* play);
// The arena floor is at y = 0. Outside the arena, use the floor under Link.
#define GDF_FLOOR_Y(dorf) (GDF_IS_PLAYER(dorf) ? (dorf)->playerFloorY : 0.0f)
// How far below the floor his organ starts and goes back to
#define GDF_ORGAN_SINK_DEPTH 450.0f
void BossGanon_Player_Init(BossGanon* this, PlayState* play);
void BossGanon_PlayerProjectile_Init(BossGanon* this, PlayState* play);
void BossGanon_Player_StopOrganMusic(BossGanon* this, PlayState* play);
static void BossGanon_Player_DrawOrgan(BossGanon* this, PlayState* play);
static void BossGanon_Player_DrawDarkAura(BossGanon* this, PlayState* play);
static void BossGanon_Player_ReleaseOrganCam(BossGanon* this, PlayState* play);
static void BossGanon_Player_DrawCrawlFlame(BossGanon* this, PlayState* play);
static void BossGanon_Init_Impl(Actor* thisx, PlayState* play);
static void BossGanon_Update_Impl(Actor* thisx, PlayState* play);
static void BossGanon_Draw_Impl(Actor* thisx, PlayState* play);
static void BossGanon_Player_DrawSword(struct BossGanon* this, PlayState* play);
static void BossGanon_Player_DrawPhantom(BossGanon* this, PlayState* play, MtxF* base);
static void BossGanon_Player_ApplyDoorKick(BossGanon* this, PlayState* play);
static void BossGanon_Player_ApplyBreakKick(BossGanon* this, PlayState* play);
static s16 sGdfBreakTimer = 0; // kicking in a locked door, bars or a grate
static Actor* sGdfBreakTarget = NULL;
static void BossGanon_Player_ApplyDoorDuck(BossGanon* this, PlayState* play);
static void BossGanon_Player_UpdateBodyguards(BossGanon* this, PlayState* play);
static void BossGanon_Player_ApplyIntroCs(BossGanon* this, PlayState* play);
void BossGanon_Player_DrawIntroProps(BossGanon* this, PlayState* play);
void BossGanon_Player_DrawMasterSword(BossGanon* this, PlayState* play);

// ---- His blink ----
// Every 2 to 7 seconds (now and then twice in a row), his eyes squeeze shut over four frames: half, shut, shut, half.
#define GDF_EYES_MID_Y -10.0f
static s16 sGdfBlinkWait = 60; // frames to the next blink
static s8 sGdfBlinkFrame = -1; // where in it he is (-1: eyes open)

static void BossGanon_Player_UpdateBlink(void) {
    if (sGdfBlinkFrame >= 0) {
        if (++sGdfBlinkFrame >= 4) {
            sGdfBlinkFrame = -1;
            sGdfBlinkWait = (Rand_ZeroOne() < 0.2f) ? 5 : (40 + (s16)Rand_ZeroFloat(100.0f));
        }
    } else if (--sGdfBlinkWait <= 0) {
        sGdfBlinkFrame = 0;
    }
}

static f32 BossGanon_Player_EyesOpen(void) {
    static const f32 sOpen[4] = { 0.45f, 0.0f, 0.0f, 0.45f };

    return (sGdfBlinkFrame >= 0) ? sOpen[sGdfBlinkFrame] : 1.0f;
}
static void BossGanon_Player_ApplyRunArms(BossGanon* this, f32 weight);
static void BossGanon_Player_ApplyRunLegs(BossGanon* this, f32 weight);
static u8 sGdfPgCapture = false;
static MtxF sGdfLimbMtx[25];
static void BossGanon_LightBall_Update_Impl(Actor* thisx, PlayState* play);
static void func_808E1EB4_Impl(Actor* thisx, PlayState* play);
static void func_808E2544_Impl(Actor* thisx, PlayState* play);
static void BossGanon_Player_Update_Impl(Actor* thisx, PlayState* play);
static void BossGanon_PlayerLightBall_Update_Impl(Actor* thisx, PlayState* play);
static void BossGanon_FireballFlames(PlayState* play, Vec3f* pos, s16 frame);
static void BossGanon_PlayerBigMagic_Update_Impl(Actor* thisx, PlayState* play);
static ActorFunc sPlayerCapeDraw = NULL;
// Phantom Ganon's energy ball uses the custom lens flare too, so only clear it if Ganondorf turned it on
static u8 sPlayerLensFlareOn = false;

static ColliderCylinderInit sDorfCylinderInit = {
    {
        COLTYPE_HIT3,
        AT_ON | AT_TYPE_ENEMY,
        AC_ON | AC_TYPE_PLAYER,
        OC1_ON | OC1_TYPE_ALL,
        OC2_TYPE_1,
        COLSHAPE_CYLINDER,
    },
    {
        ELEMTYPE_UNK0,
        { 0xFFCFFFFF, 0x00, 0x10 },
        { 0xFFCFFFFE, 0x00, 0x00 },
        TOUCH_ON | TOUCH_SFX_NORMAL,
        BUMP_ON | BUMP_HOOKABLE,
        OCELEM_ON,
    },
    { 20, 80, -50, { 0, 0, 0 } },
};

static ColliderCylinderInit sLightBallCylinderInit = {
    {
        COLTYPE_NONE,
        AT_ON | AT_TYPE_ENEMY,
        AC_ON | AC_TYPE_PLAYER,
        OC1_ON | OC1_TYPE_ALL,
        OC2_TYPE_1,
        COLSHAPE_CYLINDER,
    },
    {
        ELEMTYPE_UNK6,
        { 0x00100700, 0x00, 0x08 },
        { 0x0D900740, 0x00, 0x00 },
        TOUCH_ON | TOUCH_SFX_NORMAL,
        BUMP_ON,
        OCELEM_ON,
    },
    { 20, 30, -15, { 0, 0, 0 } },
};

static u8 D_808E4C58[] = { 0, 12, 10, 12, 14, 16, 12, 14, 16, 12, 14, 16, 12, 14, 16, 10, 16, 14 };
static Vec3f sZeroVec = { 0.0f, 0.0f, 0.0f };

static EnGanonMant* sCape;

static s32 sSeed1;
static s32 sSeed3;
static s32 sSeed2;

static BossGanon* sGanondorf;

static GanondorfEffect sEffects[200];

// SoH: "Play as Ganondorf". The player-controlled Ganondorf can be in the same room as the real one (the final
// fight), so each has its own Ganondorf/cape pointers and effect buffer. sGanondorf, sCape and sCurEffects point at
// whichever one's code is running; the entry points below switch them (see BossGanon_UseContext).
static GanondorfEffect sPlayerEffects[200];
static GanondorfEffect* sCurEffects = sEffects;
static BossGanon* sVanillaGanondorf;
static EnGanonMant* sVanillaCape;
static BossGanon* sPlayerGanondorf;
static s16 sGdfHoldUpTimer = 0; // (frames left of a hold-up that isn't one of Link's item gets)
static EnGanonMant* sPlayerCape;
// SoH: his cloak's other pieces, sewn onto his cape one after another (over his left shoulder, then down his front),
// the "Cloak" option
static EnGanonMant* sGdfCloak[GANON_MANT_MAX_LINKED] = { NULL, NULL };
// SoH: the capelet over his shoulders: three pieces, the first simulating and drawing the other two
static EnGanonMant* sGdfCapelet[3] = { NULL, NULL, NULL };
static ActorFunc sGdfCapeletDraw = NULL;

// ---- His cloak and hood ----
// The Cloak option: his cape as a cloak. The cape hangs from a collar along his shoulders and across his upper back,
// and a second cloth falls from it over his left shoulder and down his front to the middle of his chest, leaving his
// right side open (his right arm free). Both cloths are kept out of his body: capsules on his torso, pelvis, shoulders,
// arms, hands and legs, made from where they're drawn, so his left arm under the cloak carries it about instead of
// passing through it. The Hood option: a hood on his head (GanondorfHood.cpp), its brim casting a dark shadow over his
// brow and eyes (his eyes only just glowing through it); his nose and mouth below it in the light.
static MtxF sGdfLimbWorld[25];     // (from his last draw)
static f32 sGdfWalkSpeed;          // (defined with UpdateWalk)
static f32 sGdfCapeRunLift = 0.0f; // how much he's running (0 to 1, see UpdateWalk): his cape's hold rises with it
#define GDF_CAPE_RUN_LIFT 7.0f
#define GDF_CAPE_HEM_FLARE 1.6f // how far out his cape's hem flares (its edges, at the bottom; per step)
#define GDF_CAPE_BILLOW 2.6f    // how strongly it billows at a run
void BossGanon_Player_Float(BossGanon* this, PlayState* play);
// His torso where it's drawn (this draw, before his hair), and his hair kept off his collar (see HairOverride)
static MtxF sGdfHairTorso;
static u8 sGdfHairTorsoValid = false;
static void BossGanon_Player_HairOverride(BossGanon* this, Vec3s* rot);
static void BossGanon_Player_SwdKeepBack(PlayState* play, Vec3f* target);
// First person (C-Up): his body stays, seen from his eyes, with his head left out (the view is from inside it)
static u8 sGdfFpBodyQuery = false;
static s32 BossGanon_Player_FpBodyOn(void) {
    return CVarGetInteger("gModes.PlayAsGanondorf.FirstPersonBody", 1);
}
static s32 BossGanon_Player_HeadHidden(BossGanon* this, PlayState* play) {
    return (this == sPlayerGanondorf) && false && BossGanon_Player_FpBodyOn();
}
static u8 sGdfLimbWorldValid = false;
#define GDF_CLOAK_CAPSULES 12
static GanonMantCapsule sGdfCapsules[GDF_CLOAK_CAPSULES];
static Vec3f sGdfCollar[GANON_MANT_NUM_STRANDS]; // along his shoulders and across his upper back
static u8 sGdfCollarValid = false;

// (Set aside on 9 Nov 2026: too much swish and the cloth's texture clipping through him. The classic cape is back;
// the code stays, switched off.)
static s32 BossGanon_Player_CloakOn(void) {
    return false;
}

static s32 BossGanon_Player_HoodOn(void) {
    // Play as Ganondorf (2S2H): not ported yet
    return 0;
}

// For the hood (GanondorfHood.cpp): his face limb's matrix and the collar it drapes down to (only on his own cape: the
// other players' Ganondorfs in Invasion have capes of the same kind)

static Vec3f BossGanon_Player_LimbPoint(s32 limb, f32 x) {
    Vec3f local = { x, 0.0f, 0.0f };
    Vec3f out;

    SkinMatrix_Vec3fMtxFMultXYZ(&sGdfLimbWorld[limb], &local, &out);
    return out;
}

// Which way a cloth hanging at `p` is pushed: straight out from his torso's middle line (as a yaw for the cape's push,
// which is negative: pushed away from him)
static f32 BossGanon_Player_OutwardYaw(Vec3f* p) {
    GanonMantCapsule* torso = &sGdfCapsules[0];
    f32 abx = torso->b.x - torso->a.x;
    f32 aby = torso->b.y - torso->a.y;
    f32 abz = torso->b.z - torso->a.z;
    f32 len2 = SQ(abx) + SQ(aby) + SQ(abz);
    f32 t = (len2 > 0.0001f)
                ? CLAMP(((p->x - torso->a.x) * abx + (p->y - torso->a.y) * aby + (p->z - torso->a.z) * abz) / len2,
                        0.0f, 1.0f)
                : 0.0f;
    f32 ox = p->x - (torso->a.x + abx * t);
    f32 oz = p->z - (torso->a.z + abz * t);

    return atan2f(-ox, -oz);
}

// A point on the cloak's neckline: an oval round his shoulders and throat (`alpha` 0 at his throat, turning to his
// right shoulder, behind his neck at 180 and on round to his left), `grow` further out, then pushed out of his body
// onto its surface (on his shoulder pads, not in them)
static Vec3f BossGanon_Player_Neckline(Vec3f* center, Vec3f* fwd, Vec3f* right, f32 halfWidth, f32 alpha, f32 grow,
                                       GanonMantCapsule* caps) {
    f32 ca = cosf(alpha);
    f32 sa = sinf(alpha);
    f32 rf = 10.0f + grow;
    f32 rs = halfWidth + 3.0f + grow;
    Vec3f p;
    s32 i;

    p.x = center->x + fwd->x * ca * rf + right->x * sa * rs;
    p.y = center->y - 5.0f * MAX(ca, 0.0f) + 2.0f * MAX(-ca, 0.0f);
    p.z = center->z + fwd->z * ca * rf + right->z * sa * rs;
    for (i = 0; i < GDF_CLOAK_CAPSULES; i++) {
        GanonMantCapsule* c = &caps[i];
        f32 abx = c->b.x - c->a.x;
        f32 aby = c->b.y - c->a.y;
        f32 abz = c->b.z - c->a.z;
        f32 len2 = SQ(abx) + SQ(aby) + SQ(abz);
        f32 t = 0.0f;
        Vec3f q;
        Vec3f d;
        f32 len;

        if (len2 > 0.0001f) {
            t = CLAMP(((p.x - c->a.x) * abx + (p.y - c->a.y) * aby + (p.z - c->a.z) * abz) / len2, 0.0f, 1.0f);
        }
        q.x = c->a.x + abx * t;
        q.y = c->a.y + aby * t;
        q.z = c->a.z + abz * t;
        d.x = p.x - q.x;
        d.y = p.y - q.y;
        d.z = p.z - q.z;
        len = sqrtf(SQ(d.x) + SQ(d.y) + SQ(d.z));
        if ((len < c->r + 0.6f) && (len > 0.001f)) {
            p.x = q.x + d.x / len * (c->r + 0.6f);
            p.y = q.y + d.y / len * (c->r + 0.6f);
            p.z = q.z + d.z / len * (c->r + 0.6f);
        }
    }
    return p;
}

// One piece of a cloak: its 12 roots along the neckline from `a0` to `a1` (degrees), pushed straight out from him
static void BossGanon_Player_SetPiece(EnGanonMant* piece, Vec3f* center, Vec3f* fwd, Vec3f* right, f32 halfWidth,
                                      f32 a0, f32 a1, f32 grow, GanonMantCapsule* caps, EnGanonMant* like,
                                      f32 jointLength) {
    s32 i;

    for (i = 0; i < GANON_MANT_NUM_STRANDS; i++) {
        f32 alpha = DEG_TO_RAD(a0 + (a1 - a0) * i / (GANON_MANT_NUM_STRANDS - 1.0f));

        piece->overrideRoots[i] = BossGanon_Player_Neckline(center, fwd, right, halfWidth, alpha, grow, caps);
    }
    piece->rootOverride = true;
    piece->overrideBaseYaw = BossGanon_Player_OutwardYaw(&piece->overrideRoots[GANON_MANT_NUM_STRANDS / 2]);
    piece->capsules = caps;
    piece->capsuleCount = GDF_CLOAK_CAPSULES;
    if (piece != like) {
        piece->actor.world.pos = like->actor.world.pos;
        piece->backPush = like->backPush;
        piece->backSwayMagnitude = like->backSwayMagnitude;
        piece->sideSwayMagnitude = like->sideSwayMagnitude * 0.3f;
        piece->gravity = like->gravity;
        piece->minY = like->minY;
    }
    piece->minDist = 0.0f;
    piece->jointLength = jointLength;
}

// Each frame (from his draw): his body as capsules, and the cloak on it. The cloak is two layers of three pieces sewn
// edge to edge round the neckline: the cloak itself (right front, back, left front: closed at his throat, falling open
// down his front) and over it a shorter capelet on his shoulders. His cape is the cloak's first piece.
static GanonMantCapsule
    sGdfCapsulesOuter[GDF_CLOAK_CAPSULES]; // (the capelet's: a little larger, so it lies on the cloak)

static void BossGanon_Player_UpdateCloak(BossGanon* this) {
    // limb, from and to along its bone, radius (model units; his mesh's, plus the cloth's thickness)
    static const s16 sSpec[GDF_CLOAK_CAPSULES][4] = {
        { 1, 0, 3000, 1150 },                                                                   // torso
        { 18, 0, 1700, 1050 },                                                                  // pelvis
        { 2, 300, 2300, 1000 }, { 3, 0, 2040, 700 },  { 4, 0, 1803, 650 }, { 5, 0, 1100, 600 }, // left arm
        { 6, 300, 2300, 1000 }, { 7, 0, 2040, 700 },  { 8, 0, 1803, 650 }, { 9, 0, 1100, 600 }, // right arm
        { 19, 0, 2452, 950 },   { 22, 0, 2452, 950 },                                           // thighs
    };
    // The pieces round the neckline (degrees from his throat, round by his right)
    static const f32 sPieces[3][2] = { { 8.0f, 120.0f }, { 120.0f, 240.0f }, { 240.0f, 352.0f } };
    f32 yaw = BINANG_TO_RAD(this->actor.shape.rot.y);
    Vec3f fwd = { sinf(yaw), 0.0f, cosf(yaw) };
    Vec3f right = { -fwd.z, 0.0f, fwd.x };
    Vec3f rs;
    Vec3f ls;
    Vec3f center;
    f32 halfWidth;
    s32 cloak = BossGanon_Player_CloakOn();
    s32 i;

    if ((this != sPlayerGanondorf) || !sGdfLimbWorldValid || (sCape == NULL)) {
        return;
    }

    // His cape wider toward its hem, always, and billowing as he walks and runs: a ripple rolling down it, quicker and
    // fuller the faster he goes, swelling out behind him (set back to still as he stops)
    {
        f32 go = this->playerGrounded ? this->playerWalkWeight * CLAMP(sGdfWalkSpeed / 10.0f, 0.0f, 1.0f) : 0.0f;

        sCape->hemFlare = GDF_CAPE_HEM_FLARE;
        Math_ApproachF(&sCape->billow, GDF_CAPE_BILLOW * go, 0.2f, 0.3f);
        sCape->billowPhase += 0.18f + 0.22f * go;
        if (sCape->billowPhase > 2.0f * M_PI * 100.0f) {
            sCape->billowPhase -= 2.0f * M_PI * 100.0f;
        }
    }
    for (i = 0; i < GDF_CLOAK_CAPSULES; i++) {
        MtxF* m = &sGdfLimbWorld[sSpec[i][0]];
        f32 scale = sqrtf(SQ(m->xx) + SQ(m->yx) + SQ(m->zx));

        sGdfCapsules[i].a = BossGanon_Player_LimbPoint(sSpec[i][0], sSpec[i][1]);
        sGdfCapsules[i].b = BossGanon_Player_LimbPoint(sSpec[i][0], sSpec[i][2]);
        sGdfCapsules[i].r = (sSpec[i][3] + 150.0f) * scale;
        sGdfCapsulesOuter[i] = sGdfCapsules[i];
        sGdfCapsulesOuter[i].r += 2.5f;
    }

    // The neckline's middle: between his shoulder joints, a little above them
    rs = BossGanon_Player_LimbPoint(7, 0.0f);
    ls = BossGanon_Player_LimbPoint(3, 0.0f);
    center.x = (rs.x + ls.x) * 0.5f;
    center.y = (rs.y + ls.y) * 0.5f + 5.0f;
    center.z = (rs.z + ls.z) * 0.5f;
    halfWidth = sqrtf(SQ(rs.x - ls.x) + SQ(rs.z - ls.z)) * 0.5f;

    // The hood drapes to the neckline from his right shoulder round behind him to his left
    for (i = 0; i < GANON_MANT_NUM_STRANDS; i++) {
        sGdfCollar[i] = BossGanon_Player_Neckline(&center, &fwd, &right, halfWidth,
                                                  DEG_TO_RAD(70.0f + 220.0f * i / (GANON_MANT_NUM_STRANDS - 1.0f)),
                                                  0.0f, sGdfCapsules);
    }
    sGdfCollarValid = true;

    if (!cloak) {
        // The plain cape, as it always hung
        sCape->rootOverride = false;
        sCape->capsules = NULL;
        sCape->linkedCount = 0;
        sCape->jointLength = 0.0f;
        if (sGdfCapelet[0] != NULL) {
            sGdfCapelet[0]->actor.draw = NULL;
        }
        return;
    }

    // The cloak: his cape is its right front, the other two sewn on behind and round to his left front
    BossGanon_Player_SetPiece(sCape, &center, &fwd, &right, halfWidth, sPieces[0][0], sPieces[0][1], 0.0f, sGdfCapsules,
                              sCape, 0.0f);
    sCape->linkedCount = 0;
    if ((sGdfCloak[0] != NULL) && (sGdfCloak[1] != NULL)) {
        for (i = 0; i < GANON_MANT_MAX_LINKED; i++) {
            BossGanon_Player_SetPiece(sGdfCloak[i], &center, &fwd, &right, halfWidth, sPieces[i + 1][0],
                                      sPieces[i + 1][1], 0.0f, sGdfCapsules, sCape, 0.0f);
            sGdfCloak[i]->isLinked = true;
            sCape->linked[i] = sGdfCloak[i];
        }
        sCape->linkedCount = this->playerHidden ? 0 : GANON_MANT_MAX_LINKED;
    }

    // The capelet: the same three pieces on top, a little further out, and short (to his chest and shoulder blades)
    if ((sGdfCapelet[0] != NULL) && (sGdfCapelet[1] != NULL) && (sGdfCapelet[2] != NULL)) {
        for (i = 0; i < 3; i++) {
            BossGanon_Player_SetPiece(sGdfCapelet[i], &center, &fwd, &right, halfWidth, sPieces[i][0], sPieces[i][1],
                                      2.5f, sGdfCapsulesOuter, sCape, 3.4f);
        }
        sGdfCapelet[1]->isLinked = true;
        sGdfCapelet[2]->isLinked = true;
        sGdfCapelet[0]->linked[0] = sGdfCapelet[1];
        sGdfCapelet[0]->linked[1] = sGdfCapelet[2];
        sGdfCapelet[0]->linkedCount = GANON_MANT_MAX_LINKED;
        sGdfCapelet[0]->actor.draw = this->playerHidden ? NULL : sGdfCapeletDraw;
    }
}

// The shadow under his hood: a soft dark veil over his brow and eyes, feathered at every edge and gone by his nose (in
// his face's limb: x forward, y down, z across; it curves back round his face as his face does)
static void BossGanon_Player_DrawHoodShadow(PlayState* play) {
    // Play as Ganondorf (2S2H): not ported yet
}

typedef struct {
    BossGanon* dorf;
    EnGanonMant* cape;
    GanondorfEffect* effects;
} BossGanonContext;

static u8 sWindowShatterTex[2048] = { { 0 } };

void BossGanonEff_SpawnSparkle(PlayState* play, Vec3f* pos, Vec3f* velocity, Vec3f* accel, f32 scale, s16 arg6) {
    s16 i;
    GanondorfEffect* eff = sCurEffects;

    for (i = 0; i < 150; i++, eff++) {
        if (eff->type == GDF_EFF_NONE) {
            eff->type = GDF_EFF_SPARKLE;
            eff->pos = *pos;
            eff->velocity = *velocity;
            eff->accel = *accel;
            eff->scale = scale / 1000.0f;
            eff->unk_2E = (s16)Rand_ZeroFloat(100.0f) + 0xC8;
            eff->unk_30 = arg6;
            eff->timer = (s16)Rand_ZeroFloat(10.0f);
            eff->epoch++;
            break;
        }
    }
}

void BossGanonEff_SpawnLightRay(PlayState* play, Vec3f* pos, Vec3f* velocity, Vec3f* accel, f32 scale, f32 arg5,
                                s16 arg6) {
    s16 i;
    GanondorfEffect* eff = sCurEffects;

    for (i = 0; i < 150; i++, eff++) {
        if (eff->type == GDF_EFF_NONE) {
            eff->type = GDF_EFF_LIGHT_RAY;
            eff->pos = *pos;
            eff->velocity = *velocity;
            eff->accel = *accel;
            eff->scale = scale / 1000.0f;
            eff->unk_38 = 1.0f;
            eff->unk_40 = arg5;
            eff->unk_2E = (s16)Rand_ZeroFloat(100.0f) + 0xC8;
            eff->unk_30 = arg6;
            eff->timer = (s16)Rand_ZeroFloat(10.0f);
            eff->unk_48 = Math_Atan2F(eff->velocity.z, eff->velocity.x);
            eff->unk_44 = -Math_Atan2F(sqrtf(SQXZ(eff->velocity)), eff->velocity.y);
            eff->epoch++;
            break;
        }
    }
}

void BossGanonEff_SpawnLightning(PlayState* play, f32 scale, f32 arg2, f32 arg3) {
    s16 i;
    GanondorfEffect* eff = sCurEffects;

    for (i = 0; i < 150; i++, eff++) {
        if (eff->type == GDF_EFF_NONE) {
            eff->type = GDF_EFF_LIGHTNING;
            eff->velocity = sZeroVec;
            eff->accel = sZeroVec;
            eff->unk_2E = 0;
            eff->scale = scale;
            eff->unk_48 = arg2;
            eff->unk_3C = arg3;
            eff->timer = 0;
            eff->epoch++;
            break;
        }
    }
}

void BossGanonEff_SpawnDustDark(PlayState* play, Vec3f* pos, f32 scale, f32 arg3) {
    s16 i;
    GanondorfEffect* eff = sCurEffects;

    for (i = 0; i < 150; i++, eff++) {
        if (eff->type == GDF_EFF_NONE) {
            eff->type = GDF_EFF_IMPACT_DUST_DARK;
            eff->pos = *pos;
            eff->velocity = sZeroVec;
            eff->accel = sZeroVec;
            eff->scale = scale;
            eff->unk_40 = 1.0f;
            eff->unk_38 = arg3;
            eff->unk_30 = (s16)Rand_ZeroFloat(100.0f);
            eff->unk_2E = eff->timer = eff->alpha = 0;
            eff->epoch++;
            break;
        }
    }
}

void BossGanonEff_SpawnDustLight(PlayState* play, Vec3f* pos, f32 scale, f32 arg3, s16 bufIndex) {
    GanondorfEffect* effArr = sCurEffects;

    effArr[bufIndex].type = GDF_EFF_IMPACT_DUST_LIGHT;
    effArr[bufIndex].pos = *pos;
    effArr[bufIndex].velocity = sZeroVec;
    effArr[bufIndex].accel = sZeroVec;
    effArr[bufIndex].unk_40 = 1.0f;
    effArr[bufIndex].scale = scale;
    effArr[bufIndex].unk_38 = arg3;
    effArr[bufIndex].unk_30 = Rand_ZeroFloat(100.0f);
    effArr[bufIndex].unk_2E = effArr[bufIndex].timer = effArr[bufIndex].alpha = 0;
    effArr[bufIndex].epoch++;
}

void BossGanonEff_SpawnShockwave(PlayState* play, Vec3f* pos, f32 scale, f32 arg3) {
    s16 i;
    GanondorfEffect* eff = sCurEffects;

    for (i = 0; i < 150; i++, eff++) {
        if (eff->type == GDF_EFF_NONE) {
            eff->type = GDF_EFF_SHOCKWAVE;
            eff->pos = *pos;
            eff->velocity = sZeroVec;
            eff->accel = sZeroVec;
            eff->alpha = 255;
            eff->unk_40 = 0.6f;
            eff->scale = scale;
            eff->unk_38 = arg3;
            eff->unk_30 = (s16)Rand_ZeroFloat(100.0f);
            eff->unk_2E = eff->timer = 0;
            eff->epoch++;
            break;
        }
    }
}

void BossGanonEff_SpawnBlackDot(PlayState* play, Vec3f* pos, f32 scale) {
    s16 i;
    GanondorfEffect* eff = sCurEffects;

    for (i = 0; i < 150; i++, eff++) {
        if (eff->type == GDF_EFF_NONE) {
            eff->type = GDF_EFF_BLACK_DOT;
            eff->pos = *pos;
            eff->velocity = sZeroVec;
            eff->accel = sZeroVec;
            eff->unk_38 = 0.0f;
            eff->scale = scale / 1000.0f;
            eff->timer = 0;
            eff->alpha = 0;
            eff->unk_2E = 0;
            eff->epoch++;
            break;
        }
    }
}

// Play as Ganondorf (2S2H): his animations are read by name from oot.o2r, there are no animation objects to load
void BossGanon_SetAnimationObject(BossGanon* this, PlayState* play, s32 objectId) {
    this->animBankIndex = 0;
}

typedef struct {
    /* 0x00 */ Vec3s eye;
    /* 0x06 */ Vec3s at;
} CutsceneCameraPosition; // size = 0x12

static CutsceneCameraPosition sIntroCsCameraPositions[] = {
    { { 0, 40, 0 }, { 0, 50, 430 } },
    { { -20, 30, 400 }, { 10, 55, 440 } },
    { { 0, 60, 300 }, { 0, 273, -150 } },
    { { 0, 180, -260 }, { 0, 155, -300 } },
    { { -30, 60, 440 }, { 20, 25, 390 } },
    { { -50, 140, -360 }, { 50, 92, -390 } },
    { { -10, 264, -121 }, { 5, 266, -160 } },
    { { -13, 200, -310 }, { 0, 125, -410 } },
    { { 0, 40, -50 }, { 0, 35, 230 } },
    { { 0, 140, -250 }, { 0, 115, -570 } },
    { { -410, 150, -130 }, { 50, 155, -170 } },
    { { 0, 130, -230 }, { 0, 125, -2000 } },
    { { -2, 147, -293 }, { -200, 345, -2000 } },
};

// SoH: "Play as Ganondorf" title screen. Instead of the Hyrule Field title cutscene, the game opens in his
// chamber at the top of the tower: he plays Ganondorf's Theme on the organ forever, seen from behind, with the
// logo and "Press Start" on top. Link is there too, hidden and frozen at the door.
// The camera sits behind and above him and drifts slowly from side to side.
#define GDF_TITLE_CAM_EYE_Y 225.0f
#define GDF_TITLE_CAM_EYE_Z -95.0f
#define GDF_TITLE_CAM_AT_Y 150.0f
#define GDF_TITLE_CAM_AT_Z -470.0f

static f32 D_808E4D44[] = {
    1.0f, 3.0f, 0.0f, 7.0f, 13.0f, 4.0f, 6.0f, 11.0f, 5.0f, 2.0f, 8.0f, 14.0f, 10.0f, 12.0f, 9.0f,
};

s32 BossGanon_OverrideLimbDraw(PlayState* play, s32 limbIndex, Gfx** dList, Vec3f* pos, Vec3s* rot, void* thisx) {
    BossGanon* this = (BossGanon*)thisx;

    // SoH: "Play as Ganondorf", first person: no head (limbs 10-17: head, face, jaw and hair)
    if ((limbIndex >= 11) && (limbIndex <= 18) && BossGanon_Player_HeadHidden(this, play)) {
        *dList = NULL;
    }

    // SoH: "Play as Ganondorf", the upper half of his hair (limb 14) hangs from his torso's way, not his head's
    if ((limbIndex == 15) && GDF_IS_PLAYER(this) && sGdfHairTorsoValid) {
        BossGanon_Player_HairOverride(this, rot);
    }

    switch (limbIndex) {
        case 10:
            if (this->useOpenHand) {
                *dList = gGanondorfRightHandOpenDL;
            }
            break;

        case 20:
            rot->y += this->legRot.x + this->legRot.z;
            rot->z += this->legRot.y;
            break;

        case 21:
            if (this->legRot.y > 0.0f) {
                rot->z += this->legRot.y;
            }
            break;

        case 22:
            rot->y += this->legRot.x + this->legRot.z;
            rot->z += this->legRot.y;
            break;

        case 23:
            rot->y += this->legRot.x - this->legRot.z;
            rot->z += this->legRot.y;
            break;

        case 24:
            if (this->legRot.y > 0.0f) {
                rot->z += this->legRot.y;
            }
            break;

        case 25:
            rot->y += this->legRot.x - this->legRot.z;
            rot->z += this->legRot.y;
            break;

        default:
            break;
    }

    // SoH: Phantom Ganon skin, Ganondorf's own model isn't drawn
    if (sGdfPgCapture) {
        *dList = NULL;
    }

    return 0;
}

// SoH: "Play as Ganondorf", the lowest point of his model from his last draw, from a few points on the outside of his
// hands, shins and feet (measured on his model, in each limb's own space): toes, heels, soles, knees and knuckles. The
// ground offset keeps that point on the floor, so no animation (kneeling, the heavy landing, getting up) sinks him in.
static const Vec3f sGdfContactPoints[6][8] = {
    // left hand (limb 6)
    { { 584, -529, 345 },
      { 159, 303, -299 },
      { 1174, 315, 207 },
      { 977, -387, -322 },
      { 410, 166, 563 },
      { 162, -349, -243 },
      { 1142, -262, 373 },
      { 958, 124, -426 } },
    // right hand (limb 10)
    { { 584, -529, -345 },
      { 159, 303, 299 },
      { 1174, 315, -207 },
      { 977, -387, 322 },
      { 410, 166, -563 },
      { 162, -349, 243 },
      { 1142, -262, -373 },
      { 958, 124, 426 } },
    // left shin (limb 21): the knee end
    { { 1800, -737, 17 },
      { 575, 971, -45 },
      { 71, -645, -34 },
      { 2170, 968, -20 },
      { 2224, 123, 737 },
      { 2224, 123, -711 },
      { 392, 27, 671 },
      { 392, 27, -595 } },
    // left foot (limb 22): heel, sole and toes
    { { -391, -592, -565 },
      { 208, -592, 1277 },
      { 2324, -351, -313 },
      { -539, -586, 773 },
      { 373, -592, -565 },
      { 596, -586, 773 },
      { 2324, -351, 311 },
      { -324, -156, 1160 } },
    // right shin (limb 24)
    { { 1800, -737, -18 },
      { 575, 971, 44 },
      { 71, -645, 34 },
      { 2170, 968, 19 },
      { 2224, 123, -737 },
      { 2224, 123, 710 },
      { 392, 27, -671 },
      { 392, 27, 594 } },
    // right foot (limb 25)
    { { -372, -592, -565 },
      { 237, -592, 1277 },
      { 2324, -351, -312 },
      { -596, -586, 772 },
      { 391, -592, -565 },
      { -273, -156, 1161 },
      { 2324, -351, 312 },
      { 538, -586, 773 } },
};
static f32 sGdfStandOffset = 0.0f; // his ground offset standing still on his feet
static s32 sGdfStandOffsetValid = false;
static s16 sGdfFastGroundTimer = 0;   // frames left where the ground offset follows his pose at once
static f32 sGdfMeshLowY = 0.0f;       // world height of his lowest point, last draw
static f32 sGdfMeshLowYNext = 1.0e9f; // being gathered during this draw
static u8 sGdfMeshLowValid = false;
// Walking about on his own during a cutscene (BossGanon_Player_UpdateRoam)
static u8 sGdfRoaming = false;
static f32 sGdfRoamSpeed = 0.0f;
static s16 sGdfRoamYaw = 0;
static f32 sGdfRoamFloorY = 0.0f;
// The trident throw (BossGanon_Player_TridentSpin / TridentThrow)
#define GDF_TRIDENT_IN_HAND 0
#define GDF_TRIDENT_OVERHEAD 1
#define GDF_TRIDENT_OUT 2
#define GDF_TRIDENT_RETURNING 3
static u8 sGdfTridentState = GDF_TRIDENT_IN_HAND;
static Vec3f sGdfTridentPos;
static Vec3f sGdfTridentVel;
static s16 sGdfTridentSpin = 0;
// The hidden move: the trident spun for 5 s raises a ring of fireballs around him, scattered at the Links on the throw
#define GDF_FIRE_RING_COUNT 8
#define GDF_FIRE_RING_FRAMES 100
#define GDF_FIRE_RING_COST 20
static Actor* sGdfFireRing[GDF_FIRE_RING_COUNT];
static Actor* sGdfTridentTarget = NULL;
static s16 sGdfTridentTimer = 0;
static f32 sGdfTridentTravel = 0.0f;

// SoH: "Play as Ganondorf", his head (limb 18) from his last draw: Link's head, hat and focus while Link is hidden
static Vec3f sGdfPlayerHeadPos;
static u8 sGdfPlayerHeadValid = false;

// The throat grab: whoever he holds hangs from his right fist, placed as the hand is drawn (see the grab, below)
static void BossGanon_Player_GrabFollowHand(BossGanon* this);

static void BossGanon_Player_DrawKnife(BossGanon* this, PlayState* play, s32 limbIndex); // (his dagger, below)

void BossGanon_PostLimbDraw(PlayState* play, s32 limbIndex, Gfx** dList, Vec3s* rot, void* thisx) {
    static s8 bodyPartLimbMap[] = {
        -1, -1, 1, -1, 3, 4, 5, -1, 6, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, 2, 12, 13, 14, 9, 10, 11, -1, -1, -1, -1,
    };
    static Vec3f D_808E4DA0 = { -500.0f, 200.0f, -300.0f };
    static Vec3f D_808E4DAC = { -500.0f, 200.0f, 300.0f };
    static Vec3f D_808E4DB8 = { 0.0f, 0.0f, 0.0f };
    static Vec3f D_808E4DC4 = { 0.0f, 0.0f, 0.0f };
    static Vec3f D_808E4DD0 = { 0.0f, 0.0f, 0.0f };
    static Vec3f D_808E4DDC = { 1300.0f, 0.0f, 0.0f };
    static Vec3f D_808E4DE8 = { 600.0f, 420.0f, 100.0f };
    s8 bodyPart;
    BossGanon* this = (BossGanon*)thisx;

    bodyPart = bodyPartLimbMap[limbIndex];
    if (bodyPart >= 0) {
        Matrix_MultVec3f(&D_808E4DB8, &this->unk_2EC[bodyPart]);
    }

    if (GDF_IS_PLAYER(this)) {
        s32 set = -1;

        if (!sGdfPgCapture) {
            BossGanon_Player_DrawKnife(this, play, limbIndex);
        }
        switch (limbIndex) {
            case 6:
                set = 0;
                break;
            case 10:
                set = 1;
                break;
            case 21:
                set = 2;
                break;
            case 22:
                set = 3;
                break;
            case 24:
                set = 4;
                break;
            case 25:
                set = 5;
                break;
        }
        if (set >= 0) {
            Vec3f p;
            s32 k;

            for (k = 0; k < 8; k++) {
                Matrix_MultVec3f(&sGdfContactPoints[set][k], &p);
                sGdfMeshLowYNext = MIN(sGdfMeshLowYNext, p.y);
            }
        }
    }

    if ((limbIndex == 18) && GDF_IS_PLAYER(this)) {
        static Vec3f sHeadCenter = { 800.0f, 0.0f, 0.0f }; // a little way up the head from the neck joint

        Matrix_MultVec3f(&sHeadCenter, &sGdfPlayerHeadPos);
        sGdfPlayerHeadValid = true;
    }

    // SoH: Phantom Ganon skin, every limb's matrix for posing his skeleton
    if (sGdfPgCapture && (limbIndex >= 1) && (limbIndex <= 25)) {
        Matrix_Get(&sGdfLimbMtx[limbIndex - 1]);
    }

    if ((limbIndex == 2) && GDF_IS_PLAYER(this)) {
        Matrix_Get(&sGdfHairTorso);
        sGdfHairTorsoValid = true;
    }

    // SoH: "Play as Ganondorf", every limb where it's drawn (his cloak drapes over them; his hood sits on his head)
    if ((this == sPlayerGanondorf) && (limbIndex >= 1) && (limbIndex <= 25)) {
        Matrix_Get(&sGdfLimbWorld[limbIndex - 1]);
        sGdfLimbWorldValid = true;
    }

    // SoH: "Play as Ganondorf", his hands hold the sword and the sheath
    if (GDF_IS_PLAYER(this)) {
        if (limbIndex == 6) {
            Matrix_Get(&this->playerLeftHandMtx);
        } else if (limbIndex == 10) {
            Matrix_Get(&this->playerRightHandMtx);
            this->playerHandMtxValid = true;
            BossGanon_Player_GrabFollowHand(this);
        }
    }

    if (limbIndex == 2) {
        Matrix_MultVec3f(&D_808E4DB8, &this->unk_1FC);
    } else if (limbIndex == 19) {
        Matrix_MultVec3f(&D_808E4DB8, &this->actor.focus.pos);
    } else if (limbIndex == 11) {
        OPEN_DISPS(play->state.gfxCtx);

        Matrix_MultVec3f(&D_808E4DB8, &this->unk_208);
        if (!sGdfPgCapture && !BossGanon_Player_HeadHidden(this, play)) {
            // SoH: "Play as Ganondorf", he blinks: his glowing eyes squeezed shut toward their middle line (his head
            // limb's y runs down his face; the eyes span y -146 to 133 about -10), shut for two frames
            f32 open = GDF_IS_PLAYER(this) ? BossGanon_Player_EyesOpen() : 1.0f;

            if (open > 0.0f) {
                Matrix_Push();
                if (open < 1.0f) {
                    Matrix_Translate(0.0f, GDF_EYES_MID_Y, 0.0f, MTXMODE_APPLY);
                    Matrix_Scale(1.0f, open, 1.0f, MTXMODE_APPLY);
                    Matrix_Translate(0.0f, -GDF_EYES_MID_Y, 0.0f, MTXMODE_APPLY);
                }
                // (his eyes' own drawing state and texture again, right before them: whatever else was drawn in the
                // see-through pass since his draw set them up, between his limbs, left them garbled)
                Gfx_SetupDL_25Xlu(play->state.gfxCtx);
                gSPSegment(POLY_XLU_DISP++, 0x08, SEGMENTED_TO_VIRTUAL(gGanondorfNormalEyeTex));
                gSPMatrix(POLY_XLU_DISP++, MATRIX_NEWMTX(play->state.gfxCtx),
                          G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
                gSPDisplayList(POLY_XLU_DISP++, SEGMENTED_TO_VIRTUAL(gGanondorfEyesDL));
                Matrix_Pop();
            }
            // SoH: "Play as Ganondorf", under his hood the shadow over his brow and eyes
            if ((this == sPlayerGanondorf) && BossGanon_Player_HoodOn()) {
                BossGanon_Player_DrawHoodShadow(play);
            }
        }

        CLOSE_DISPS(play->state.gfxCtx);
    } else if (limbIndex == 6) {
        Matrix_MultVec3f(&D_808E4DC4, &this->unk_238);
    } else if (limbIndex == 10) {
        Matrix_MultVec3f(&D_808E4DD0, &this->unk_22C);

        if (this->unk_25C == 0) {
            Matrix_MultVec3f(&D_808E4DDC, &this->unk_260);
        }

        this->unk_25C = 0;

        if (this->triforceType == GDF_TRIFORCE_DORF) {
            Matrix_MultVec3f(&D_808E4DE8, &this->triforcePos);
        }
    } else if (limbIndex == 4) {
        Vec3f sp28 = D_808E4DA0;

        if (this->unk_198 == 1) {
            sp28.x += -300.0f;
            sp28.y += -300.0f;
            sp28.z += 700.0f;
        } else if (this->unk_198 == 2) {
            sp28.x += -300.0f;
            sp28.z += 700.0f;
        }

        Matrix_MultVec3f(&sp28, &this->unk_220);
    } else if (limbIndex == 8) {
        Vec3f sp1C = D_808E4DAC;

        if (this->unk_198 == 1) {
            sp1C.x += -300.0f;
            sp1C.y += -300.0f;
            sp1C.z += -700.0f;
        } else if (this->unk_198 == 2) {
            sp1C.x += -300.0f;
            sp1C.y += 100.0f;
            sp1C.z += -700.0f;
        }

        Matrix_MultVec3f(&sp1C, &this->unk_214);
    }
}

void BossGanon_InitRand(s32 seedInit0, s32 seedInit1, s32 seedInit2) {
    sSeed1 = seedInit0;
    sSeed2 = seedInit1;
    sSeed3 = seedInit2;
}

f32 BossGanon_RandZeroOne(void) {
    // Wichmann-Hill algorithm
    f32 randFloat;

    sSeed1 = (sSeed1 * 171) % 30269;
    sSeed2 = (sSeed2 * 172) % 30307;
    sSeed3 = (sSeed3 * 170) % 30323;

    randFloat = (sSeed1 / 30269.0f) + (sSeed2 / 30307.0f) + (sSeed3 / 30323.0f);

    while (randFloat >= 1.0f) {
        randFloat -= 1.0f;
    }

    return fabsf(randFloat);
}

void BossGanon_DrawShock(BossGanon* this, PlayState* play) {
    s32 pad;
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    s16 i;

    OPEN_DISPS(gfxCtx);

    if ((this->unk_2E8 != 0) || (this->unk_2E6 != 0)) {
        Gfx_SetupDL_25Xlu(play->state.gfxCtx);
        gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, 255);
        gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 0, 0);
        gSPDisplayList(POLY_XLU_DISP++, gGanondorfLightBallMaterialDL);

        if (this->unk_2E8 != 0) {
            Player* player = GET_PLAYER(play);

            for (i = 0; i < ARRAY_COUNT(player->bodyPartsPos); i++) {
                FrameInterpolation_RecordOpenChild("Ganondorf Shock 0", i);

                Matrix_Translate(player->bodyPartsPos[i].x, player->bodyPartsPos[i].y, player->bodyPartsPos[i].z,
                                 MTXMODE_NEW);
                Matrix_ReplaceRotation(&play->billboardMtxF);
                Matrix_Scale(this->unk_49C[i], this->unk_49C[i], this->unk_49C[i], MTXMODE_APPLY);
                Matrix_RotateZ(Rand_CenteredFloat(M_PI), MTXMODE_APPLY);
                gSPMatrix(POLY_XLU_DISP++, MATRIX_NEWMTX(gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
                gSPDisplayList(POLY_XLU_DISP++, gGanondorfSquareDL);

                FrameInterpolation_RecordCloseChild();
            }
        } else {
            for (i = 1; i < 15; i++) {
                FrameInterpolation_RecordOpenChild("Ganondorf Shock 1", i);

                Matrix_Translate(this->unk_2EC[i].x, this->unk_2EC[i].y, this->unk_2EC[i].z, MTXMODE_NEW);
                Matrix_ReplaceRotation(&play->billboardMtxF);
                Matrix_Scale(this->unk_49C[i], this->unk_49C[i], this->unk_49C[i], MTXMODE_APPLY);

                if (!this->shockGlow) {
                    Matrix_RotateZ(Rand_CenteredFloat(M_PI), MTXMODE_APPLY);
                }

                gSPMatrix(POLY_XLU_DISP++, MATRIX_NEWMTX(gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);

                if (this->shockGlow) {
                    gSPSegment(POLY_XLU_DISP++, 0x08,
                               Gfx_TwoTexScrollEx(play->state.gfxCtx, 0, 0, 0, 32, 64, 1, 0, (this->unk_1A2 + i) * -15,
                                                  32, 64, 0, 0, 0, -15));
                    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 200, 255, 170, 255);
                    gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 0, 128);
                    gSPDisplayList(POLY_XLU_DISP++, gGanondorfShockGlowDL);
                } else {
                    gSPDisplayList(POLY_XLU_DISP++, gGanondorfSquareDL);
                }

                FrameInterpolation_RecordCloseChild();
            }
        }
    }

    CLOSE_DISPS(gfxCtx);
}

void BossGanon_DrawHandLightBall(BossGanon* this, PlayState* play) {
    s32 pad;
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    s32 alpha;

    OPEN_DISPS(gfxCtx);

    if (this->handLightBallScale > 0.0f) {
        Gfx_SetupDL_25Xlu(play->state.gfxCtx);
        gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, 255);

        if ((this->unk_1A2 % 2) != 0) {
            gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 0, 0);
        } else {
            gDPSetEnvColor(POLY_XLU_DISP++, 100, 255, 0, 0);
        }

        gSPDisplayList(POLY_XLU_DISP++, gGanondorfLightBallMaterialDL);

        Matrix_Translate(this->unk_260.x, this->unk_260.y, this->unk_260.z, MTXMODE_NEW);
        Matrix_ReplaceRotation(&play->billboardMtxF);
        Matrix_Scale(this->handLightBallScale, this->handLightBallScale, this->handLightBallScale, MTXMODE_APPLY);
        Matrix_RotateZ(this->unk_258, 1);
        gSPMatrix(POLY_XLU_DISP++, MATRIX_NEWMTX(gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        gSPDisplayList(POLY_XLU_DISP++, gGanondorfSquareDL);

        alpha = ((this->unk_1A2 % 2) != 0) ? 100 : 80;
        gDPPipeSync(POLY_XLU_DISP++);
        gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 155, alpha);
        Matrix_Translate(this->unk_260.x, GDF_FLOOR_Y(this), this->unk_260.z, MTXMODE_NEW);
        Matrix_Scale(this->handLightBallScale * 0.75f, 1.0f, this->handLightBallScale * 0.75f, MTXMODE_APPLY);
        gSPMatrix(POLY_XLU_DISP++, MATRIX_NEWMTX(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        gSPDisplayList(POLY_XLU_DISP++, gGanondorfLightCoreDL);
    }

    CLOSE_DISPS(gfxCtx);
}

void BossGanon_DrawBigMagicCharge(BossGanon* this, PlayState* play) {
    s32 pad;
    f32 yRot;
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    s16 i;

    OPEN_DISPS(gfxCtx);

    if (this->unk_284 > 0.0f) {
        Gfx_SetupDL_25Xlu(play->state.gfxCtx);

        // light flecks
        gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 170, (s8)this->unk_290);
        gDPSetEnvColor(POLY_XLU_DISP++, 200, 255, 0, 128);
        gSPSegment(POLY_XLU_DISP++, 0x08,
                   Gfx_TwoTexScrollEx(play->state.gfxCtx, 0, this->unk_1A2 * -2, 0, 0x40, 0x40, 1, 0,
                                      this->unk_1A2 * 0xA, 0x40, 0x40, -2, 0, 0, 0xA));
        Matrix_Translate(this->unk_278.x, this->unk_278.y, this->unk_278.z, MTXMODE_NEW);
        Matrix_ReplaceRotation(&play->billboardMtxF);
        Matrix_Scale(this->unk_28C, this->unk_28C, this->unk_28C, MTXMODE_APPLY);
        gSPMatrix(POLY_XLU_DISP++, MATRIX_NEWMTX(gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        gSPDisplayList(POLY_XLU_DISP++, gGanondorfLightFlecksDL);

        // background circle texture
        Matrix_Translate(this->unk_278.x, this->unk_278.y, this->unk_278.z, MTXMODE_NEW);
        Matrix_ReplaceRotation(&play->billboardMtxF);
        Matrix_Scale(this->unk_284, this->unk_284, this->unk_284, MTXMODE_APPLY);
        gSPMatrix(POLY_XLU_DISP++, MATRIX_NEWMTX(gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 0, 100, (s8)this->unk_288);
        gSPSegment(POLY_XLU_DISP++, 0x09,
                   Gfx_TwoTexScrollEx(play->state.gfxCtx, 0, 0, 0, 0x20, 0x20, 1, 0, this->unk_1A2 * -4, 0x20, 0x20, 0,
                                      0, 0, -4));
        gSPDisplayList(POLY_XLU_DISP++, gGanondorfBigMagicBGCircleDL);

        // yellow background dot
        gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 150, 170, 0, (s8)this->unk_288);
        gSPSegment(POLY_XLU_DISP++, 0x0A,
                   Gfx_TwoTexScrollEx(play->state.gfxCtx, 0, 0, 0, 0x20, 0x20, 1, this->unk_1A2 * 2,
                                      this->unk_1A2 * -0x14, 0x40, 0x40, 0, 0, 2, -0x14));
        gSPDisplayList(POLY_XLU_DISP++, gGanondorfDotDL);

        // light ball material
        gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, 255);
        gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 100, 0);
        gSPDisplayList(POLY_XLU_DISP++, gGanondorfLightBallMaterialDL);

        // light ball geometry
        Matrix_Translate(this->unk_278.x, this->unk_278.y, this->unk_278.z, MTXMODE_NEW);
        Matrix_ReplaceRotation(&play->billboardMtxF);
        Matrix_Scale(this->unk_2D0, this->unk_2D0, this->unk_2D0, MTXMODE_APPLY);
        Matrix_RotateZ((this->unk_1A2 * 10.0f) / 1000.0f, MTXMODE_APPLY);
        gSPMatrix(POLY_XLU_DISP++, MATRIX_NEWMTX(gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        gSPDisplayList(POLY_XLU_DISP++, gGanondorfSquareDL);

        BossGanon_InitRand(this->unk_1AA + 1, 0x71AC, 0x263A);
        Matrix_Translate(this->unk_278.x, this->unk_278.y, this->unk_278.z, MTXMODE_NEW);
        Matrix_RotateY((this->unk_1A2 * 10.0f) / 1000.0f, MTXMODE_APPLY);
        gDPSetEnvColor(POLY_XLU_DISP++, 200, 255, 0, 0);

        yRot = BINANG_TO_RAD(this->actor.yawTowardsPlayer);

        for (i = 0; i < this->unk_1AC; i++) {
            FrameInterpolation_RecordOpenChild("Ganondorf Big Magic", i);

            f32 xzRot = (BossGanon_RandZeroOne() - 0.5f) * M_PI * 1.5f;

            gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, (s8)this->unk_294[i]);
            Matrix_Push();
            Matrix_RotateY(xzRot + yRot, MTXMODE_APPLY);
            Matrix_RotateX((BossGanon_RandZeroOne() - 0.5f) * M_PI, MTXMODE_APPLY);
            Matrix_RotateZ(xzRot, MTXMODE_APPLY);
            Matrix_Translate(0.0f, 0.0f, 50.0f, MTXMODE_APPLY);
            Matrix_Scale(4.0f, 4.0f, 1.0f, MTXMODE_APPLY);
            gSPMatrix(POLY_XLU_DISP++, MATRIX_NEWMTX(gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gSPDisplayList(POLY_XLU_DISP++, gGanondorfLightRayTriDL);

            Matrix_Pop();

            FrameInterpolation_RecordCloseChild();
        }
    }

    CLOSE_DISPS(gfxCtx);
}

void BossGanon_DrawTriforce(BossGanon* this, PlayState* play) {
    s32 pad;

    if (this->fwork[GDF_TRIFORCE_PRIM_A] > 0.0f) {
        OPEN_DISPS(play->state.gfxCtx);

        Matrix_Push();

        gDPPipeSync(POLY_XLU_DISP++);
        gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, (u8)this->fwork[GDF_TRIFORCE_PRIM_B],
                        (s8)this->fwork[GDF_TRIFORCE_PRIM_A]);
        gDPSetEnvColor(POLY_XLU_DISP++, 255, (u8)this->fwork[GDF_TRIFORCE_ENV_G], 0, 128);

        if (this->triforceType == GDF_TRIFORCE_PLAYER) {
            Player* player = GET_PLAYER(play);

            this->triforcePos = player->bodyPartsPos[12];

            this->triforcePos.x += -0.6f;
            this->triforcePos.y += 3.0f;
            this->triforcePos.z += -2.0f;
        }

        Matrix_Translate(this->triforcePos.x, this->triforcePos.y, this->triforcePos.z, MTXMODE_NEW);

        if (this->triforceType == GDF_TRIFORCE_PLAYER) {
            Matrix_RotateX(-1.4f, MTXMODE_APPLY);
            Matrix_RotateZ(4.0f, MTXMODE_APPLY);
        } else if (this->triforceType == GDF_TRIFORCE_ZELDA) {
            Matrix_RotateY(1.5f, 1);
            Matrix_RotateX(1.1f, 1);
            Matrix_RotateZ(-0.99999994f, MTXMODE_APPLY);
        } else {
            Matrix_ReplaceRotation(&play->billboardMtxF);
        }

        Matrix_Scale(this->fwork[GDF_TRIFORCE_SCALE], this->fwork[GDF_TRIFORCE_SCALE], 1.0f, MTXMODE_APPLY);
        gSPMatrix(POLY_XLU_DISP++, MATRIX_NEWMTX(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        gSPDisplayList(POLY_XLU_DISP++, SEGMENTED_TO_VIRTUAL(gGanondorfTriforceDL));

        Matrix_Pop();

        CLOSE_DISPS(play->state.gfxCtx);
    }
}

void BossGanon_DrawDarkVortex(BossGanon* this, PlayState* play) {
    s32 pad;

    if (this->fwork[GDF_VORTEX_ALPHA] > 0.0f) {
        OPEN_DISPS(play->state.gfxCtx);

        Matrix_Push();
        gDPPipeSync(POLY_XLU_DISP++);
        gSPSegment(POLY_XLU_DISP++, 0x08,
                   Gfx_TwoTexScrollEx(play->state.gfxCtx, 0, this->unk_1A2 * -8, 0, 0x20, 0x40, 1, this->unk_1A2 * -4,
                                      this->unk_1A2 * -8, 0x20, 0x20, -8, 0, -4, -8));
        gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 100, 0, 200, (s8)this->fwork[GDF_VORTEX_ALPHA]);
        gDPSetEnvColor(POLY_XLU_DISP++, 130, 0, 0, 128);

        if (this->csState != 21) {
            Matrix_Translate(0.0f, 105.0f, -400.0f, MTXMODE_NEW);
            Matrix_RotateX(M_PI / 2, MTXMODE_APPLY);
        } else {
            Matrix_Translate(-50.0f, 50.0f, -150.0f, MTXMODE_NEW);
            Matrix_RotateY(M_PI / 10, MTXMODE_APPLY);
            Matrix_RotateX(M_PI / 2, MTXMODE_APPLY);
        }

        Matrix_Scale(this->fwork[GDF_VORTEX_SCALE], this->fwork[GDF_VORTEX_SCALE], this->fwork[GDF_VORTEX_SCALE],
                     MTXMODE_APPLY);
        gSPMatrix(POLY_XLU_DISP++, MATRIX_NEWMTX(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        gSPDisplayList(POLY_XLU_DISP++, SEGMENTED_TO_VIRTUAL(gGanondorfVortexDL));

        Matrix_Pop();

        CLOSE_DISPS(play->state.gfxCtx);
    }
}

void BossGanon_GenShadowTexture(u8* tex, BossGanon* this, PlayState* play) {
    // Play as Ganondorf (2S2H): not ported yet
}

void BossGanon_DrawShadowTexture(void* tex, BossGanon* this, PlayState* play) {
    // Play as Ganondorf (2S2H): not ported yet
}

static void BossGanon_Draw_Impl(Actor* thisx, PlayState* play) {
    s32 i;
    BossGanon* this = (BossGanon*)thisx;
    void* shadowTex;

    // SoH: "Play as Ganondorf", hidden in first person and while Link is shown instead
    if (GDF_IS_PLAYER(this) && this->playerHidden) {
        BossGanon_Player_DrawCrawlFlame(this, play);
        BossGanon_DrawEffects(play);
        BossGanon_Player_DrawIntroProps(this, play);
        return;
    }

    if (GDF_IS_PLAYER(this)) {
        BossGanon_Player_DrawOrgan(this, play);
        BossGanon_Player_DrawDarkAura(this, play);
    }

    shadowTex = Graph_Alloc(play->state.gfxCtx, 64 * 64);

    OPEN_DISPS(play->state.gfxCtx);

    // Invalidate texture mask if it has changed
    if (this->windowShatterState != GDF_WINDOW_SHATTER_OFF) {
        gSPInvalidateTexCache(POLY_OPA_DISP++, sWindowShatterTex);
    }

    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);

    if ((this->unk_1A6 & 2) != 0) {
        POLY_OPA_DISP = Gfx_SetFog(POLY_OPA_DISP, 255, 50, 0, 0, 900, 1099);
    }

    gSPSegment(POLY_XLU_DISP++, 0x08, SEGMENTED_TO_VIRTUAL(gGanondorfNormalEyeTex));

    // SoH: "Play as Ganondorf", Phantom Ganon skin: Ganondorf's skeleton still runs (without drawing) so everything
    // that follows his hands, head and shoulders keeps working, and Phantom Ganon is drawn in the same pose
    if (GDF_IS_PLAYER(this) && BossGanon_Player_IsPhantom()) {
        MtxF base;

        Matrix_Get(&base);
        sGdfPgCapture = true;
        SkelAnime_DrawFlexOpa(play, this->skelAnime.skeleton, this->skelAnime.jointTable, this->skelAnime.dListCount,
                              BossGanon_OverrideLimbDraw, BossGanon_PostLimbDraw, &this->actor);
        sGdfPgCapture = false;
        BossGanon_Player_DrawPhantom(this, play, &base);
    } else {
        this->playerSpearValid = false;
        SkelAnime_DrawFlexOpa(play, this->skelAnime.skeleton, this->skelAnime.jointTable, this->skelAnime.dListCount,
                              BossGanon_OverrideLimbDraw, BossGanon_PostLimbDraw, &this->actor);
    }

    // SoH: "Play as Ganondorf", sword stance
    if (GDF_IS_PLAYER(this)) {
        BossGanon_Player_DrawSword(this, play);
        // His lowest point, for the ground offset
        sGdfMeshLowValid = sGdfMeshLowYNext < 1.0e8f;
        if (sGdfMeshLowValid) {
            sGdfMeshLowY = sGdfMeshLowYNext;
        }
        sGdfMeshLowYNext = 1.0e9f;
    }

    this->unk_2EC[0].x = this->unk_2EC[1].x;
    this->unk_2EC[0].y = this->unk_2EC[1].y + 30.0f;
    this->unk_2EC[0].z = this->unk_2EC[1].z;

    POLY_OPA_DISP = Play_SetFog(play, POLY_OPA_DISP);

    BossGanon_DrawEffects(play);

    sCape->actor.world.pos = this->actor.world.pos;

    sCape->rightForearmPos = this->unk_214;
    sCape->leftForearmPos = this->unk_220;
    // SoH: "Play as Ganondorf", walking and running: the cape hangs from two points on his arms, and the walk, run and
    // sprint swing his arms down and back, so the cape's top sank down his back (to his shoulder blades at a sprint).
    // Standing still, those points are remembered as they sit on his torso; on the move the cape hangs from those
    // places on his torso instead (by how much he's walking), as high as when he stands.
    if ((this == sPlayerGanondorf) && sGdfLimbWorldValid) {
        static Vec3f sLocalR;
        static Vec3f sLocalL;
        static u8 sLocalValid = false;
        MtxF inv;
        // (the walk keeps it well as it is: by how much he runs or sprints)
        f32 w = MIN(sGdfCapeRunLift, this->playerWalkWeight);

        if ((this->playerWalkWeight < 0.02f) && this->playerGrounded && (this->actionFunc == BossGanon_Player_Float)) {
            if (SkinMatrix_Invert(&sGdfLimbWorld[1], &inv) == 0) {
                SkinMatrix_Vec3fMtxFMultXYZ(&inv, &sCape->rightForearmPos, &sLocalR);
                SkinMatrix_Vec3fMtxFMultXYZ(&inv, &sCape->leftForearmPos, &sLocalL);
                sLocalValid = true;
            }
        } else if (sLocalValid && (w > 0.0f) && this->playerGrounded) {
            Vec3f r;
            Vec3f l;

            SkinMatrix_Vec3fMtxFMultXYZ(&sGdfLimbWorld[1], &sLocalR, &r);
            SkinMatrix_Vec3fMtxFMultXYZ(&sGdfLimbWorld[1], &sLocalL, &l);
            sCape->rightForearmPos.x += (r.x - sCape->rightForearmPos.x) * w;
            sCape->rightForearmPos.y += (r.y - sCape->rightForearmPos.y) * w;
            sCape->rightForearmPos.z += (r.z - sCape->rightForearmPos.z) * w;
            sCape->leftForearmPos.x += (l.x - sCape->leftForearmPos.x) * w;
            sCape->leftForearmPos.y += (l.y - sCape->leftForearmPos.y) * w;
            sCape->leftForearmPos.z += (l.z - sCape->leftForearmPos.z) * w;
        }
    }

    sCape->rightShoulderPos = this->unk_22C;
    sCape->leftShoulderPos = this->unk_238;
    if (GDF_IS_PLAYER(this)) {
        BossGanon_Player_UpdateCloak(this); // (his cloak and the collar his hood drapes to)
    }

    BossGanon_DrawShock(this, play);
    BossGanon_DrawHandLightBall(this, play);
    BossGanon_DrawBigMagicCharge(this, play);
    BossGanon_DrawTriforce(this, play);
    BossGanon_DrawDarkVortex(this, play);
    if (GDF_IS_PLAYER(this)) {
        BossGanon_Player_DrawIntroProps(this, play);
    }

    BossGanon_GenShadowTexture(shadowTex, this, play);
    BossGanon_DrawShadowTexture(shadowTex, this, play);

    CLOSE_DISPS(play->state.gfxCtx);
}

void BossGanon_LightBall_Draw(Actor* thisx, PlayState* play) {
    BossGanon* this = (BossGanon*)thisx;
    s16 i;
    f32 alpha;
    s32 pad;

    OPEN_DISPS(play->state.gfxCtx);

    Gfx_SetupDL_25Xlu(play->state.gfxCtx);

    alpha = ((this->unk_1A2 % 2) != 0) ? this->fwork[GDF_FWORK_1] * 0.4f : this->fwork[GDF_FWORK_1] * 0.35f;

    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 155, (s8)alpha);
    Matrix_Push();
    Matrix_Translate(this->actor.world.pos.x, this->actor.floorHeight, this->actor.world.pos.z, MTXMODE_NEW);
    Matrix_Scale(this->actor.scale.x * 0.75f, 1.0f, this->actor.scale.z * 0.75f, MTXMODE_APPLY);
    gSPMatrix(POLY_XLU_DISP++, MATRIX_NEWMTX(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    gSPDisplayList(POLY_XLU_DISP++, gGanondorfLightCoreDL);

    Matrix_Pop();
    gSPDisplayList(POLY_XLU_DISP++, gGanondorfLightBallMaterialDL);

    gDPPipeSync(POLY_XLU_DISP++);
    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, (s8)this->fwork[GDF_FWORK_1]);
    gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 0, 0);

    if (this->unk_1A8 == 1) {
        for (i = 0; i < 8; i++) {
            FrameInterpolation_RecordOpenChild("Ganondorf Light Ball 0", i);

            Matrix_Push();
            Matrix_RotateY(i * (M_PI / 8), MTXMODE_APPLY);
            Matrix_RotateZ(this->fwork[GDF_FWORK_0], MTXMODE_APPLY);
            gSPMatrix(POLY_XLU_DISP++, MATRIX_NEWMTX(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);

            gSPDisplayList(POLY_XLU_DISP++, gGanondorfSquareDL);
            Matrix_Pop();

            FrameInterpolation_RecordCloseChild();
        }
    } else if (this->unk_1A8 == 0) {
        Matrix_ReplaceRotation(&play->billboardMtxF);
        Matrix_RotateZ((this->actor.shape.rot.z / 32768.0f) * 3.1416f, MTXMODE_APPLY);
        gSPMatrix(POLY_XLU_DISP++, MATRIX_NEWMTX(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        gSPDisplayList(POLY_XLU_DISP++, gGanondorfSquareDL);
    }

    CLOSE_DISPS(play->state.gfxCtx);
}

static Gfx* sBigMagicLightStreakDLists[] = {
    gGanondorfLightStreak12DL, gGanondorfLightStreak11DL, gGanondorfLightStreak10DL, gGanondorfLightStreak9DL,
    gGanondorfLightStreak8DL,  gGanondorfLightStreak7DL,  gGanondorfLightStreak6DL,  gGanondorfLightStreak5DL,
    gGanondorfLightStreak4DL,  gGanondorfLightStreak3DL,  gGanondorfLightStreak2DL,  gGanondorfLightStreak1DL,
};

void func_808E324C(Actor* thisx, PlayState* play) {
    BossGanon* this = (BossGanon*)thisx;
    Mtx* mtx;
    s16 i;
    s32 temp;

    mtx = Graph_Alloc(play->state.gfxCtx, 12 * sizeof(Mtx));

    OPEN_DISPS(play->state.gfxCtx);

    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0x80, 255, 255, 255, (s8)this->fwork[GDF_FWORK_1]);
    gDPSetEnvColor(POLY_XLU_DISP++, 150, 255, 0, 128);
    gSPSegment(POLY_XLU_DISP++, 0x0D, mtx);

    for (i = 0; i < 12; i++) {
        temp = (s16)(((this->unk_1A6 - i) + 0xF) % 15);
        Matrix_Translate(this->unk_2EC[temp].x, this->unk_2EC[temp].y, this->unk_2EC[temp].z, MTXMODE_NEW);
        Matrix_RotateY(this->unk_3C4[temp].y, MTXMODE_APPLY);
        Matrix_RotateX(-this->unk_3C4[temp].x, MTXMODE_APPLY);
        Matrix_Scale(this->actor.scale.x, this->actor.scale.y, this->actor.scale.z, MTXMODE_APPLY);
        Matrix_RotateY(M_PI / 2, MTXMODE_APPLY);
        MATRIX_TOMTX(mtx);
        gSPMatrix(POLY_XLU_DISP++, mtx, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        gSPDisplayList(POLY_XLU_DISP++, sBigMagicLightStreakDLists[i]);
        mtx++;
    };

    Matrix_Translate(this->actor.world.pos.x, this->actor.world.pos.y, this->actor.world.pos.z, MTXMODE_NEW);
    Matrix_ReplaceRotation(&play->billboardMtxF);
    Matrix_Scale(10.0f, 10.0f, 10.0f, MTXMODE_APPLY);
    Matrix_RotateZ(Rand_CenteredFloat(M_PI), MTXMODE_APPLY);
    gSPMatrix(POLY_XLU_DISP++, MATRIX_NEWMTX(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    gSPDisplayList(POLY_XLU_DISP++, gGanondorfLightBallMaterialDL);

    gSPDisplayList(POLY_XLU_DISP++, gGanondorfSquareDL);

    CLOSE_DISPS(play->state.gfxCtx);
}

void BossGanon_UpdateEffects(PlayState* play) {
    Player* player = GET_PLAYER(play);
    GanondorfEffect* eff = sCurEffects;
    s16 i;
    s32 pad;
    f32 xDiff;
    f32 yDiff;
    f32 zDiff;
    f32 yRot;
    f32 xRot;
    Vec3f spA0;
    s16 bodyPart;
    f32 distToPlayer;
    s32 pad2;
    s32 pad3;

    spA0.x = 0.0f;
    spA0.y = 0.0f;

    for (i = 0; i < ARRAY_COUNT(sEffects); i++, eff++) {
        if (eff->type != GDF_EFF_NONE) {
            eff->pos.x += eff->velocity.x;
            eff->pos.y += eff->velocity.y;
            eff->pos.z += eff->velocity.z;

            eff->timer++;

            eff->velocity.x += eff->accel.x;
            eff->velocity.y += eff->accel.y;
            eff->velocity.z += eff->accel.z;

            if (eff->type == GDF_EFF_WINDOW_SHARD) {
                eff->unk_44 += 0.3f;
                eff->unk_48 += 0.5f;

                if (eff->pos.y < 0.0f) {
                    eff->type = GDF_EFF_NONE;
                }
            } else if (eff->type == GDF_EFF_SPARKLE) {
                eff->unk_3C += Rand_ZeroFloat(M_PI / 2) + M_PI / 2;
                eff->unk_2E -= eff->unk_30;

                if (eff->unk_2E <= 0) {
                    eff->unk_2E = 0;
                    eff->type = GDF_EFF_NONE;
                }

                eff->alpha = eff->unk_2E;

                if (eff->alpha > 255) {
                    eff->alpha = 255;
                }
            } else if (eff->type == GDF_EFF_BLACK_DOT) {
                xDiff = sGanondorf->unk_278.x - eff->pos.x;
                yDiff = sGanondorf->unk_278.y - eff->pos.y;
                zDiff = sGanondorf->unk_278.z - eff->pos.z;

                yRot = Math_FAtan2F(xDiff, zDiff);

                xRot = -Math_FAtan2F(yDiff, sqrtf(SQ(xDiff) + SQ(zDiff)));
                spA0.z = eff->unk_38;
                Matrix_RotateY(yRot, MTXMODE_NEW);
                Matrix_RotateX(xRot, MTXMODE_APPLY);
                Matrix_MultVec3f(&spA0, &eff->velocity);
                Math_ApproachF(&eff->unk_38, 10.0f, 1.0f, 0.5f);

                eff->alpha += 10;

                if (eff->alpha > 255) {
                    eff->alpha = 255;
                }

                if ((sqrtf(SQ(xDiff) + SQ(yDiff) + SQ(zDiff)) < 20.0f) || (eff->timer > 70)) {
                    eff->type = GDF_EFF_NONE;
                }
            } else if (eff->type == GDF_EFF_LIGHT_RAY) {
                eff->unk_3C += Rand_ZeroFloat(M_PI / 2) + M_PI / 2;
                eff->unk_2E -= eff->unk_30;

                if (eff->unk_2E <= 0) {
                    eff->unk_2E = 0;
                    eff->type = GDF_EFF_NONE;
                }

                eff->alpha = eff->unk_2E;

                if (eff->alpha > 255) {
                    eff->alpha = 255;
                }

                Math_ApproachF(&eff->unk_38, eff->unk_40, 1.0f, (eff->unk_40 / 15.0f) * 4.0f);
            } else if (eff->type == GDF_EFF_SHOCK) {
                if (eff->unk_2E == GDF_SHOCK_DORF_YELLOW) {
                    bodyPart = (s16)Rand_ZeroFloat(13.9f) + 1;

                    eff->pos.x = sGanondorf->unk_2EC[bodyPart].x + Rand_CenteredFloat(20.0f);
                    eff->pos.y = sGanondorf->unk_2EC[bodyPart].y + Rand_CenteredFloat(20.0f);
                    eff->pos.z = sGanondorf->unk_2EC[bodyPart].z + Rand_CenteredFloat(20.0f);
                } else {
                    bodyPart = (s16)Rand_ZeroFloat(17.9f);

                    eff->pos.x = player->bodyPartsPos[bodyPart].x + Rand_CenteredFloat(10.0f);
                    eff->pos.y = player->bodyPartsPos[bodyPart].y + Rand_CenteredFloat(15.0f);
                    eff->pos.z = player->bodyPartsPos[bodyPart].z + Rand_CenteredFloat(10.0f);
                }

                eff->unk_3C += (Rand_ZeroFloat(M_PI / 2) + M_PI / 2);

                if (eff->timer > 20) {
                    eff->type = GDF_EFF_NONE;
                }
            } else if (eff->type == GDF_EFF_LIGHTNING) {
                if (eff->unk_3C == 0.0f) {
                    eff->unk_44 = BINANG_TO_RAD(Camera_GetInputDirYaw(Play_GetCamera(play, CAM_ID_MAIN)));
                } else {
                    eff->unk_44 = M_PI / 2;
                }

                if (eff->timer > 12) {
                    eff->type = GDF_EFF_NONE;
                }
            } else if (eff->type == GDF_EFF_IMPACT_DUST_DARK) {
                eff->unk_30++; // unused

                if (eff->unk_2E == 0) {
                    eff->alpha += 26;

                    if (eff->alpha > 255) {
                        eff->alpha = 255;
                        eff->unk_2E = 1;
                    }
                } else if (eff->unk_2E == 1) {
                    eff->unk_2E = 2;
                } else if (eff->unk_2E == 2) {
                    eff->alpha -= 26;

                    if (eff->alpha < 0) {
                        eff->alpha = 0;
                        eff->type = GDF_EFF_NONE;
                    }
                }

                Math_ApproachF(&eff->scale, eff->unk_38, 1.0f, 0.01f);
                Math_ApproachF(&eff->unk_40, 4.0f, 1.0f, 0.15f);
            } else if (eff->type == GDF_EFF_IMPACT_DUST_LIGHT) {
                if (i == 0) {
                    Audio_PlaySfx(NA_SE_EN_GANON_WAVE_GND - SFX_FLAG);
                }

                eff->unk_30++; // unused

                if (eff->unk_2E == 0) {
                    eff->alpha += 100;

                    if (eff->alpha > 255) {
                        eff->alpha = 255;
                        eff->unk_2E = 1;
                    }
                } else if (eff->unk_2E == 1) {
                    if (eff->timer >= 20) {
                        eff->unk_2E = 2;
                    }
                } else if (eff->unk_2E == 2) {
                    eff->alpha -= 30;

                    if (eff->alpha < 0) {
                        eff->alpha = 0;
                        eff->type = GDF_EFF_NONE;
                    }
                }

                Math_ApproachF(&eff->scale, eff->unk_38, 1.0f, 0.1f);
                Math_ApproachF(&eff->unk_40, 1.0f, 1.0f, 0.15f);
            } else if (eff->type == GDF_EFF_SHOCKWAVE) {
                eff->unk_30++; // unused
                eff->alpha -= 30;

                if (eff->alpha < 0) {
                    eff->alpha = 0;
                    eff->type = GDF_EFF_NONE;
                }

                Math_ApproachF(&eff->scale, eff->unk_38, 1.0f, 0.13f);

                if ((eff->timer < 150) && (fabsf(player->actor.world.pos.y) < 5.0f) && !GDF_IS_PLAYER(sGanondorf)) {
                    distToPlayer =
                        sqrtf(SQ(eff->pos.x - player->actor.world.pos.x) + SQ(eff->pos.z - player->actor.world.pos.z));

                    if (((eff->scale * 150.0f) < distToPlayer) && (distToPlayer < (eff->scale * 300.0f))) {
                        eff->timer = 150;
                        func_800B8D50(play, &sGanondorf->actor, 7.0f, sGanondorf->actor.yawTowardsPlayer, 0.0f, 0x20);
                    }
                }
            }
        }
    }
}

static void* sLightningTextures[] = {
    gGanondorfLightning1Tex,  gGanondorfLightning1Tex, gGanondorfLightning2Tex,  gGanondorfLightning3Tex,
    gGanondorfLightning4Tex,  gGanondorfLightning5Tex, gGanondorfLightning6Tex,  gGanondorfLightning7Tex,
    gGanondorfLightning8Tex,  gGanondorfLightning9Tex, gGanondorfLightning10Tex, gGanondorfLightning11Tex,
    gGanondorfLightning12Tex,
};

static u8 sLightningPrimColors[] = {
    0,   0,   0,   255, 255, 255, 231, 250, 231, 208, 245, 208, 185, 240, 185, 162, 235, 162, 139, 230,
    139, 115, 225, 115, 92,  220, 92,  69,  215, 69,  46,  210, 46,  23,  205, 23,  0,   200, 0,
};

static u8 sLightningEnvColors[] = {
    0,   0,   0,   255, 255, 0,   240, 231, 23,  226, 208, 46,  212, 185, 69,  198, 162, 92,
    184, 139, 115, 170, 115, 139, 156, 92,  162, 142, 69,  185, 128, 46,  208, 114, 23,  231,
    100, 0,   255, 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
};

void BossGanon_DrawEffects(PlayState* play) {
    u8 flag = 0;
    s16 i;
    s32 pad;
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    GanondorfEffect* eff = sCurEffects;
    GanondorfEffect* effFirst = eff;

    OPEN_DISPS(gfxCtx);
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);

    for (i = 0; i < 200; i++, eff++) {
        if (eff->type == GDF_EFF_WINDOW_SHARD) {
            FrameInterpolation_RecordOpenChild(eff, eff->epoch);
            gDPPipeSync(POLY_OPA_DISP++);
            if (flag == 0) {
                gSPDisplayList(POLY_OPA_DISP++, gGanondorfWindowShardMaterialDL);
                flag++;
            }
            if ((eff->timer & 7) != 0) {
                gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, eff->color.r, eff->color.g, eff->color.b, 255);
            } else {
                gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 255, 255);
            }
            Matrix_Translate(eff->pos.x, eff->pos.y, eff->pos.z, MTXMODE_NEW);
            Matrix_Scale(eff->scale, eff->scale, eff->scale, MTXMODE_APPLY);
            Matrix_RotateY(eff->unk_48, MTXMODE_APPLY);
            Matrix_RotateX(eff->unk_44, MTXMODE_APPLY);
            gSPMatrix(POLY_OPA_DISP++, MATRIX_NEWMTX(gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gSPDisplayList(POLY_OPA_DISP++, gGanondorfWindowShardModelDL);
            FrameInterpolation_RecordCloseChild();
        }
    }

    eff = effFirst;
    flag = 0;

    for (i = 0; i < 150; i++, eff++) {
        if (eff->type == GDF_EFF_SPARKLE) {
            FrameInterpolation_RecordOpenChild(eff, eff->epoch);
            gDPPipeSync(POLY_XLU_DISP++);
            if (flag == 0) {
                gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 0, 0);
                gSPDisplayList(POLY_XLU_DISP++, gGanondorfLightBallMaterialDL);
                flag++;
            }
            gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, eff->alpha);
            Matrix_Translate(eff->pos.x, eff->pos.y, eff->pos.z, MTXMODE_NEW);
            Matrix_ReplaceRotation(&play->billboardMtxF);
            Matrix_Scale(eff->scale, eff->scale, 1.0f, MTXMODE_APPLY);
            Matrix_RotateZ(eff->unk_3C, MTXMODE_APPLY);
            gSPMatrix(POLY_XLU_DISP++, MATRIX_NEWMTX(gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gSPDisplayList(POLY_XLU_DISP++, gGanondorfSquareDL);
            FrameInterpolation_RecordCloseChild();
        }
    }

    eff = effFirst;
    flag = 0;

    for (i = 0; i < 150; i++, eff++) {
        if (eff->type == GDF_EFF_LIGHT_RAY) {
            FrameInterpolation_RecordOpenChild(eff, eff->epoch);
            gDPPipeSync(POLY_XLU_DISP++);
            if (flag == 0) {
                gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 0, 0);
                gSPDisplayList(POLY_XLU_DISP++, gGanondorfLightBallMaterialDL);
                flag++;
            }
            gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, eff->alpha);
            Matrix_Translate(eff->pos.x, eff->pos.y, eff->pos.z, MTXMODE_NEW);
            Matrix_RotateY(eff->unk_48, MTXMODE_APPLY);
            Matrix_RotateX(eff->unk_44, MTXMODE_APPLY);
            Matrix_RotateZ(eff->unk_3C, MTXMODE_APPLY);
            Matrix_Scale(eff->scale, eff->scale, eff->unk_38 * eff->scale, MTXMODE_APPLY);
            Matrix_RotateX(M_PI / 2, MTXMODE_APPLY);
            gSPMatrix(POLY_XLU_DISP++, MATRIX_NEWMTX(gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gSPDisplayList(POLY_XLU_DISP++, gGanondorfSquareDL);
            FrameInterpolation_RecordCloseChild();
        }
    }

    eff = effFirst;
    flag = 0;

    for (i = 0; i < 150; i++, eff++) {
        if (eff->type == GDF_EFF_SHOCK) {
            FrameInterpolation_RecordOpenChild(eff, eff->epoch);
            if (flag == 0) {
                gDPPipeSync(POLY_XLU_DISP++);
                if (eff->unk_2E == GDF_SHOCK_PLAYER_PURPLE) {
                    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 100, 0, 200, 255);
                    gDPSetEnvColor(POLY_XLU_DISP++, 130, 0, 0, 0);
                } else { // GDF_SHOCK_DORF_YELLOW or GDF_SHOCK_PLAYER_YELLOW
                    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, 255);
                    gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 0, 0);
                }
                flag++;
            }
            Matrix_Translate(eff->pos.x, eff->pos.y, eff->pos.z, MTXMODE_NEW);
            Matrix_Scale(eff->scale, eff->scale, 1.0f, MTXMODE_APPLY);
            Matrix_RotateX(eff->unk_3C * 1.3f, MTXMODE_APPLY);
            Matrix_RotateZ(eff->unk_3C, MTXMODE_APPLY);
            gSPMatrix(POLY_XLU_DISP++, MATRIX_NEWMTX(gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gSPDisplayList(POLY_XLU_DISP++, gGanondorfShockDL);
            FrameInterpolation_RecordCloseChild();
        }
    }

    eff = effFirst;

    for (i = 0; i < 150; i++, eff++) {
        if (eff->type == GDF_EFF_LIGHTNING) {
            FrameInterpolation_RecordOpenChild(eff, eff->epoch);
            gDPPipeSync(POLY_XLU_DISP++);
            gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, sLightningPrimColors[(eff->timer * 3) + 0],
                            sLightningPrimColors[(eff->timer * 3) + 1], sLightningPrimColors[(eff->timer * 3) + 2],
                            255);
            gDPSetEnvColor(POLY_XLU_DISP++, sLightningEnvColors[(eff->timer * 3) + 0],
                           sLightningEnvColors[(eff->timer * 3) + 1], sLightningEnvColors[(eff->timer * 3) + 2], 0);
            Matrix_Translate(sGanondorf->unk_260.x, sGanondorf->unk_260.y, sGanondorf->unk_260.z, MTXMODE_NEW);
            Matrix_RotateY(eff->unk_48, MTXMODE_APPLY);
            Matrix_RotateZ(eff->unk_3C, MTXMODE_APPLY);
            Matrix_Scale(eff->scale, eff->scale, eff->scale, MTXMODE_APPLY);
            Matrix_RotateY(eff->unk_44, MTXMODE_APPLY);
            gSPMatrix(POLY_XLU_DISP++, MATRIX_NEWMTX(gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gSPSegment(POLY_XLU_DISP++, 0x08, SEGMENTED_TO_VIRTUAL(sLightningTextures[eff->timer]));
            gSPDisplayList(POLY_XLU_DISP++, gGanondorfLightningDL);
            FrameInterpolation_RecordCloseChild();
        }
    }

    eff = effFirst;

    for (i = 0; i < 150; i++, eff++) {
        if (eff->type == GDF_EFF_IMPACT_DUST_DARK) {
            FrameInterpolation_RecordOpenChild(eff, eff->epoch);
            gDPPipeSync(POLY_XLU_DISP++);
            gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 0, 0, 0, eff->alpha);
            gDPSetEnvColor(POLY_XLU_DISP++, 100, 70, 0, 128);
            gSPSegment(POLY_XLU_DISP++, 0x08,
                       Gfx_TwoTexScrollEx(play->state.gfxCtx, 0, eff->timer * 4, 0, 32, 64, 1, eff->timer * 2,
                                          eff->timer * -20, 32, 32, 4, 0, 2, -20));
            Matrix_Translate(eff->pos.x, eff->pos.y, eff->pos.z, MTXMODE_NEW);
            Matrix_Scale(eff->scale, eff->unk_40 * eff->scale, eff->scale, MTXMODE_APPLY);
            gSPMatrix(POLY_XLU_DISP++, MATRIX_NEWMTX(gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gSPDisplayList(POLY_XLU_DISP++, gGanondorfImpactDarkDL);
            FrameInterpolation_RecordCloseChild();
        }
    }

    eff = effFirst;

    for (i = 0; i < 150; i++, eff++) {
        if (eff->type == GDF_EFF_IMPACT_DUST_LIGHT) {
            FrameInterpolation_RecordOpenChild(eff, eff->epoch);
            gDPPipeSync(POLY_XLU_DISP++);
            gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, eff->alpha);
            gDPSetEnvColor(POLY_XLU_DISP++, 200, 100, 0, 128);
            gSPSegment(POLY_XLU_DISP++, 0x08,
                       Gfx_TwoTexScrollEx(play->state.gfxCtx, 0, eff->timer * 4, 0, 32, 64, 1, eff->timer * 2,
                                          eff->timer * -20, 32, 32, 4, 0, 2, -20));
            Matrix_Translate(eff->pos.x, eff->pos.y, eff->pos.z, MTXMODE_NEW);
            Matrix_Scale(eff->scale, eff->unk_40 * eff->scale, eff->scale, MTXMODE_APPLY);
            gSPMatrix(POLY_XLU_DISP++, MATRIX_NEWMTX(gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gSPDisplayList(POLY_XLU_DISP++, gGanondorfImpactLightDL);
            FrameInterpolation_RecordCloseChild();
        }
    }

    eff = effFirst;

    for (i = 0; i < 150; i++, eff++) {
        if (eff->type == GDF_EFF_SHOCKWAVE) {
            FrameInterpolation_RecordOpenChild(eff, eff->epoch);
            gDPPipeSync(POLY_XLU_DISP++);
            gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 170, eff->alpha);
            gDPSetEnvColor(POLY_XLU_DISP++, 150, 255, 0, 128);
            gSPSegment(POLY_XLU_DISP++, 0x08,
                       Gfx_TwoTexScrollEx(play->state.gfxCtx, 0, (eff->timer * 100), 0, 64, 32, 1, (eff->timer * 100),
                                          0, 64, 32, 100, 0, 100, 0));
            Matrix_Translate(eff->pos.x, eff->pos.y, eff->pos.z, MTXMODE_NEW);
            Matrix_Scale((eff->scale * 200.0f) / 1500.0f, (eff->unk_40 * 200.0f) / 1500.0f,
                         (eff->scale * 200.0f) / 1500.0f, MTXMODE_APPLY);
            gSPMatrix(POLY_XLU_DISP++, MATRIX_NEWMTX(gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gSPDisplayList(POLY_XLU_DISP++, gGanondorfShockwaveDL);
            FrameInterpolation_RecordCloseChild();
        }
    }

    eff = effFirst;

    for (i = 0; i < 150; i++, eff++) {
        if (eff->type == GDF_EFF_BLACK_DOT) {
            FrameInterpolation_RecordOpenChild(eff, eff->epoch);
            gDPPipeSync(POLY_XLU_DISP++);
            gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 150, 170, 0, eff->alpha);
            gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 255, 128);
            gSPSegment(POLY_XLU_DISP++, 0x0A,
                       Gfx_TwoTexScrollEx(play->state.gfxCtx, 0, 0, 0, 32, 32, 1, eff->timer * 2, eff->timer * -20, 64,
                                          64, 0, 0, 2, -20));
            Matrix_Translate(eff->pos.x, eff->pos.y, eff->pos.z, MTXMODE_NEW);
            Matrix_ReplaceRotation(&play->billboardMtxF);
            Matrix_Scale(eff->scale, eff->scale, 1.0f, MTXMODE_APPLY);
            gSPMatrix(POLY_XLU_DISP++, MATRIX_NEWMTX(gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gSPDisplayList(POLY_XLU_DISP++, gGanondorfDotDL);
            FrameInterpolation_RecordCloseChild();
        }
    }

    CLOSE_DISPS(gfxCtx);
}

// ================================================================================================
// SoH: "Play as Ganondorf"
//
// Ganondorf from the Ganon's Tower fight, controlled by the player. Link is still there underneath,
// invisible, so walking, collision, doors, ladders, the camera and loading zones keep working. This
// actor follows him around and replaces his moveset with Ganondorf's boss attacks:
//
//   B            Charge a light ball and throw it. Keep holding B to charge Big Magic instead.
//   R            Block with the cape (Link can't be hurt while blocking).
//   R + B        Pound the floor (shockwave around Ganondorf).
//   C-Left       Light ball        (when C-button items are disabled)
//   C-Down       Pound the floor   (when C-button items are disabled)
//   C-Right      Big Magic         (when C-button items are disabled)
//
// Every attack carries all damage types (see DMG_GANONDORF_PLAYER), but only looks like Ganondorf's magic.
//
// The vanilla AI is tied to the arena (absolute coordinates, falling platforms, room lighting), so
// this mode has its own update function and action functions modelled on the vanilla ones. The
// model, animations, cape, effects and draw code are shared with the boss.
// ================================================================================================

#define CVAR_PLAY_AS_GDF(var) "gModes.PlayAsGanondorf." var

// ---- Killing NPCs (option, permanent) ----
// Every attack volume he puts out in a frame is noted here (cylinders from his pound, aura, slam and projectiles,
// segments from the blade). At the start of his next update, NPCs touching any of them die, bloodily, and are
// remembered in the save file as dead (GanondorfKills.cpp).
#define GDF_MAX_AT_VOLUMES 24
typedef struct {
    u8 isSegment;
    Vec3f a; // cylinder: bottom centre; segment: one end
    Vec3f b; // segment: other end
    f32 radius;
    f32 height; // cylinder
} GdfAtVolume;
static GdfAtVolume sGdfAtVolumes[GDF_MAX_AT_VOLUMES];
static s32 sGdfAtCount = 0;

static void BossGanon_Player_NoteCylinder(ColliderCylinder* cyl) {
    GdfAtVolume* v;

    if (!GanondorfKills_IsEnabled() || (sGdfAtCount >= GDF_MAX_AT_VOLUMES)) {
        return;
    }
    v = &sGdfAtVolumes[sGdfAtCount++];
    v->isSegment = false;
    v->a.x = cyl->dim.pos.x;
    v->a.y = cyl->dim.pos.y + cyl->dim.yShift;
    v->a.z = cyl->dim.pos.z;
    v->radius = cyl->dim.radius;
    v->height = cyl->dim.height;
}

static void BossGanon_Player_SetAT(BossGanon* this, PlayState* play) {
    BossGanon_Player_NoteCylinder(&this->collider);
    CollisionCheck_SetAT(play, &play->colChkCtx, &this->collider.base);
}

// The NPC (as a column from its feet to its head) touches the volume

// ---- The guards hunt him (after a murder in the market) ----
// A murder anywhere in Castle Town makes him wanted. While he's wanted, alarm bells ring through the market and
// guards come for him with their spears, a new one running in for every one he cuts down (three in the market, six
// on the castle grounds, where they swarm him). It lasts until he leaves the market by the drawbridge.
s32 Object_Spawn(ObjectContext* objectCtx, s16 objectId);
static u8 sGdfWanted = false;
static s16 sGdfLastScene = -1;
static s16 sGdfBellTimer = 0;

// Scene changes: leaving the market entrance for Hyrule Field (over the drawbridge) ends it
static void BossGanon_Player_ManhuntSceneChange(PlayState* play) {
    // Play as Ganondorf (2S2H): not ported yet
}

// A new guard, out of sight behind the camera, on the ground

static void BossGanon_Player_UpdateManhunt(BossGanon* this, PlayState* play) {
    // Play as Ganondorf (2S2H): not ported yet
}

static void BossGanon_Player_SlayNpcs(BossGanon* this, PlayState* play) {
    // Play as Ganondorf (2S2H): not ported yet
}

// Ganon (the beast form) notes his attacks here too, and runs the check while Ganondorf is hidden

s32 BossGanon_Player_IsPhantom(void);

// Phantom Ganon skin: his own voice and magic sounds in place of Ganondorf's
static u16 BossGanon_Player_Voice(u16 sfx) {
    // Looping sounds are passed as "id - SFX_FLAG"
    u8 looping = !(sfx & SFX_FLAG);
    u16 id = looping ? (sfx + SFX_FLAG) : sfx;
    u16 out;

    // Play as Ganondorf (2S2H): no Phantom Ganon skin yet, his own sounds
    (void)id;
    (void)out;
    return sfx;
}

void BossGanon_Player_Update(Actor* thisx, PlayState* play);
void BossGanon_PlayerLightBall_Update(Actor* thisx, PlayState* play);
void BossGanon_PlayerBigMagic_Update(Actor* thisx, PlayState* play);

void BossGanon_Player_SetupFloat(BossGanon* this, PlayState* play);
void BossGanon_Player_Float(BossGanon* this, PlayState* play);
void BossGanon_Player_SetupChargeLightBall(BossGanon* this, PlayState* play);
void BossGanon_Player_ChargeLightBall(BossGanon* this, PlayState* play);
void BossGanon_Player_SetupThrow(BossGanon* this, PlayState* play);
void BossGanon_Player_Throw(BossGanon* this, PlayState* play);
void BossGanon_Player_SetupChargeBigMagic(BossGanon* this, PlayState* play);
void BossGanon_Player_ChargeBigMagic(BossGanon* this, PlayState* play);
void BossGanon_Player_SetupPound(BossGanon* this, PlayState* play);
void BossGanon_Player_Pound(BossGanon* this, PlayState* play);
void BossGanon_Player_SetupBlock(BossGanon* this, PlayState* play);
void BossGanon_Player_Block(BossGanon* this, PlayState* play);
void BossGanon_Player_SetupHurt(BossGanon* this, PlayState* play);
void BossGanon_Player_Hurt(BossGanon* this, PlayState* play);
void BossGanon_Player_SetupDowned(BossGanon* this, PlayState* play);
void BossGanon_Player_Downed(BossGanon* this, PlayState* play);
static void BossGanon_Player_Pacified(BossGanon* this, PlayState* play);
static void BossGanon_Player_SetupPacified(BossGanon* this, PlayState* play);
void BossGanon_Player_SetupLaugh(BossGanon* this, PlayState* play);
static void BossGanon_Player_KeepFloatRoot(BossGanon* this);
static s32 BossGanon_Player_UsesSpear(void);
static s32 BossGanon_Player_HasTrident(void);
static s32 BossGanon_Player_ApplyTridentAttack(BossGanon* this);
// His dagger: drawn from the sheath on his leg into his right fist (the model only, for a stance to come)
static u8 sGdfDagger = false; // the dagger drawn (its stance is not in the game yet: the model is kept for it)
static s32 BossGanon_Player_DaggerOut(void);
static void BossGanon_Player_DrawDagger(BossGanon* this, PlayState* play);
static void BossGanon_Player_SetBlade(BossGanon* this, Vec3f* base, Vec3f* tip);
static void BossGanon_Player_ApplyClipRoot(BossGanon* this, const Vec3s* root, s32 count, f32 frame, f32 weight);
static void BossGanon_Player_ApplyClip(BossGanon* this, const Vec3s (*clip)[13], s32 count, f32 frame, s32 loop,
                                       f32 weight, f32 body);
static const Vec3s sGdfClipRun[21][13];
static const Vec3s sGdfClipJump[21][13];
static const Vec3s sGdfClipLadder[24][13];
static const Vec3s sGdfClipLadderTop[34][13];
void BossGanon_Player_SetupDance(BossGanon* this, PlayState* play);
static f32 sGdfJumpFrame = 0.0f; // where in the running jump he is
static void BossGanon_Player_ApplyTridentRun(BossGanon* this);
void BossGanon_Player_SetupTridentSpin(BossGanon* this, PlayState* play);
void BossGanon_Player_TridentSpin(BossGanon* this, PlayState* play);
void BossGanon_Player_TridentThrow(BossGanon* this, PlayState* play);
void BossGanon_Player_Grab(BossGanon* this, PlayState* play);
void BossGanon_Player_GrabThrow(BossGanon* this, PlayState* play);
s32 BossGanon_Player_TryGrab(BossGanon* this, PlayState* play);
static void BossGanon_Player_UpdateGrabbed(BossGanon* this, PlayState* play);
static void BossGanon_Player_UpdateRoam(BossGanon* this, PlayState* play);
void BossGanon_Player_Brood(BossGanon* this, PlayState* play);
void BossGanon_Player_Laugh(BossGanon* this, PlayState* play);
void BossGanon_Player_SetupOrgan(BossGanon* this, PlayState* play);
void BossGanon_Player_Organ(BossGanon* this, PlayState* play);
void BossGanon_Player_SetupDarkAura(BossGanon* this, PlayState* play);
void BossGanon_Player_Ride(BossGanon* this, PlayState* play);
void BossGanon_Player_Climb(BossGanon* this, PlayState* play);
void BossGanon_Player_LedgeClimb(BossGanon* this, PlayState* play);
void BossGanon_Player_SetupLedgeClimb(BossGanon* this, PlayState* play);
static void BossGanon_Player_PlaceLedgeClimb(BossGanon* this, PlayState* play);
static f32 BossGanon_Player_RunBump(f32 phase, f32 center, f32 width);
void BossGanon_Player_ItemGet(BossGanon* this, PlayState* play);
static void BossGanon_Player_TwistLean(BossGanon* this, f32 twist, f32 lean);
static s32 BossGanon_Player_GetSeat(PlayState* play, Vec3f* seat);
void BossGanon_Player_DarkAura(BossGanon* this, PlayState* play);
static u8 sPlayerDarkAuraMode = false;
static void BossGanon_Player_DeflectMagic(BossGanon* this, PlayState* play);
static void BossGanon_Player_DrawOrgan(BossGanon* this, PlayState* play);
static s32 BossGanon_Player_CanStartAttack(BossGanon* this, PlayState* play);
void BossGanon_Player_SetupSwordIdle(BossGanon* this, PlayState* play);
void BossGanon_Player_SwordIdle(BossGanon* this, PlayState* play);
void BossGanon_Player_SummonTrident(BossGanon* this, PlayState* play);
static void BossGanon_Player_SetupSummonTrident(BossGanon* this, PlayState* play);
void BossGanon_Player_UnsummonTrident(BossGanon* this, PlayState* play);
static void BossGanon_Player_SetupUnsummonTrident(BossGanon* this, PlayState* play);
static void BossGanon_Player_TridentBurst(BossGanon* this, PlayState* play, Vec3f* hand, s32 own);
static const Vec3s sGdfTridentArmPoses[2][3];
#define GDF_VANISH_BURST 100 // sGdfSummonBurst above this: the trident coming apart, not bursting in
static void BossGanon_Player_TridentVanish(BossGanon* this, PlayState* play, Vec3f* hand, s32 own);
static u8 sGdfSummonBurst = 0; // counts down after the trident bursts into his hand (sent to the others)
void BossGanon_Player_SwordAttack(BossGanon* this, PlayState* play);
void BossGanon_Player_SwordDash(BossGanon* this, PlayState* play);
void BossGanon_Player_Land(BossGanon* this, PlayState* play);
s32 BossGanon_Player_IsSwordGuarding(BossGanon* this);
static s32 BossGanon_Player_TryToggleSword(BossGanon* this, PlayState* play);
static void BossGanon_Player_DrawSword(BossGanon* this, PlayState* play);
static void BossGanon_Player_UpdateSwordHit(BossGanon* this, PlayState* play);
static void BossGanon_Player_UpdateAim(BossGanon* this, PlayState* play);
extern PlayState* gPlayState;

// Ganondorf's own attacks hurt enemies instead of Link
static ColliderCylinderInit sPlayerLightBallCylinderInit = {
    {
        COLTYPE_NONE,
        AT_ON | AT_TYPE_PLAYER,
        AC_NONE,
        OC1_NONE,
        OC2_NONE,
        COLSHAPE_CYLINDER,
    },
    {
        ELEMTYPE_UNK2,
        { DMG_GANONDORF_PLAYER, 0x00, 0x04 },
        { 0x00000000, 0x00, 0x00 },
        TOUCH_ON | TOUCH_SFX_NORMAL,
        BUMP_NONE,
        OCELEM_NONE,
    },
    { 20, 30, -15, { 0, 0, 0 } },
};

static ColliderCylinderInit sPlayerBigMagicCylinderInit = {
    {
        COLTYPE_NONE,
        AT_ON | AT_TYPE_PLAYER,
        AC_NONE,
        OC1_NONE,
        OC2_NONE,
        COLSHAPE_CYLINDER,
    },
    {
        ELEMTYPE_UNK2,
        { DMG_GANONDORF_PLAYER, 0x00, 0x02 },
        { 0x00000000, 0x00, 0x00 },
        TOUCH_ON | TOUCH_SFX_NORMAL,
        BUMP_NONE,
        OCELEM_NONE,
    },
    { 15, 20, -10, { 0, 0, 0 } },
};

static ColliderCylinderInit sPlayerPoundCylinderInit = {
    {
        COLTYPE_NONE,
        AT_ON | AT_TYPE_PLAYER,
        AC_NONE,
        OC1_NONE,
        OC2_NONE,
        COLSHAPE_CYLINDER,
    },
    {
        ELEMTYPE_UNK0,
        { DMG_GANONDORF_PLAYER, 0x00, 0x08 },
        { 0x00000000, 0x00, 0x00 },
        TOUCH_ON | TOUCH_SFX_NORMAL,
        BUMP_NONE,
        OCELEM_NONE,
    },
    { 60, 80, 0, { 0, 0, 0 } },
};

// The throat grab (D-Pad Down, see BossGanon_Player_TryGrab)
#define GDF_GRAB_NONE 0
#define GDF_GRAB_HELD 1
#define GDF_GRAB_FLYING 2
#define GDF_GRAB_DOWN 3
static Actor* sGdfGrabActor = NULL;
static u32 sGdfGrabBusyFrame = 0;
static u32 sGdfLastBFrame = 0xFFFF0000; // when Ganondorf last pressed B (the volley) // the last frame he held someone
                                        // (Invasion mode's grab cooldown)
static u8 sGdfGrabState = GDF_GRAB_NONE;
static u8 sGdfGrabIsEnemy = false;
static f32 sGdfGrabNeck = 0.0f;
static f32 sGdfGrabArmWeight = 0.0f;
static Vec3f sGdfGrabVel;
static s16 sGdfGrabTimer = 0;
static u8 sGdfGrabOrigCategory = ACTORCAT_NPC;
static u32 sGdfGrabNetClient = 0; // Invasion mode: the one held is another player's Link (his game moves him)
static s16 sGdfGrabFlyTimer = 0;  // frames since thrown (the ragdoll's flight)
static u8 sGdfGrabLanded = false; // the ragdoll has hit the ground (the slam's been done)

// Holding someone up by the throat (A mustn't talk to them)

s32 BossGanon_Player_IsBusy(Actor* thisx) {
    BossGanon* this = (BossGanon*)thisx;

    if ((thisx == NULL) || !GDF_IS_PLAYER(this)) {
        return false;
    }

    return (this->actionFunc == BossGanon_Player_ChargeLightBall) || (this->actionFunc == BossGanon_Player_Throw) ||
           (this->actionFunc == BossGanon_Player_ChargeBigMagic) || (this->actionFunc == BossGanon_Player_Pound) ||
           (this->actionFunc == BossGanon_Player_Block) || (this->actionFunc == BossGanon_Player_Laugh) ||
           (this->actionFunc == BossGanon_Player_Organ) || (this->actionFunc == BossGanon_Player_SwordAttack) ||
           (this->actionFunc == BossGanon_Player_SwordDash) || (this->actionFunc == BossGanon_Player_Land) ||
           (this->actionFunc == BossGanon_Player_TridentSpin) || (this->actionFunc == BossGanon_Player_GrabThrow) ||
           (this->actionFunc == BossGanon_Player_SummonTrident) ||
           (this->actionFunc == BossGanon_Player_UnsummonTrident) || (sGdfBreakTimer > 0);
    // Not the dark aura: he moves freely while holding it. Nor the trident's flight: he walks about while it's out
}

// Whether Ganondorf is free to start something (used for levitation)

static f32 BossGanon_Player_GetHoverHeight(void) {
    return CVarGetFloat(CVAR_PLAY_AS_GDF("HoverHeight"), 70.0f);
}

// Where Ganondorf's root sits above Link's feet: on the ground he stands and walks, levitating he floats
static f32 BossGanon_Player_GetHeightAboveLink(BossGanon* this) {
    if (this->playerGrounded) {
        return this->playerGroundOffset;
    }
    return BossGanon_Player_GetHoverHeight() + (Math_SinS(this->unk_1A2 * 1500) * 3.0f);
}

// ------------------------------------------------------------------------------------------------
// SoH: "Play as Ganondorf", walking on the ground
//
// His boss model has no walk, so the walk of young Ganondorf (object_gndd, the cutscene model) is retargeted onto it
// at runtime, from the game's own files. The two skeletons match limb for limb except that the boss model has an
// extra shoulder joint on each arm; the upper arms are corrected for it, so the arms swing the same way. Faster
// than his natural pace the cycle speeds up and the stride widens, into a run.
// ------------------------------------------------------------------------------------------------

// z_skelanime.c
void SkelAnime_GetFrameData(AnimationHeader* animation, s32 frame, s32 limbCount, Vec3s* frameTable);

#define GDF_LIMB_COUNT_PLAYER 25 // gGanondorfSkel
#define GDF_YOUNG_WALK_JOINTS 24 // 23 limbs + the root translation
#define GDF_WALK_CYCLE_FRAMES 30.0f
#define GDF_WALK_NATURAL_SPEED 2.3f // world units per frame at the animation's own pace
#define GDF_WALK_MAX_RATE 2.5f
#define GDF_WALK_MAX_STRIDE 1.6f
#define GDF_ANKLE_HEIGHT 9.9f   // his ankles above the floor when standing
#define GDF_SOLE_CLEARANCE 2.5f // his lowest point (soles, toes, knees, knuckles) above the floor
#define GDF_LEFT_FOOT_BODYPART 14
#define GDF_RIGHT_FOOT_BODYPART 11
#define GDF_LEFT_KNEE_BODYPART 13
#define GDF_RIGHT_KNEE_BODYPART 10

// Boss limb -> young Ganondorf limb (-1: keep the boss animation's own rotation). The legs come from the walk; the
// feet, face and torso keep his own (young Ganondorf's feet are rotated differently, and his torso is narrower).
static const s8 sGdfBossToYoungLimb[GDF_LIMB_COUNT_PLAYER] = {
    0,  1,  -1, -1, -1, -1, -1, -1, -1, -1, // root, torso (see below), arms (their own poses, see sGdfWalkArmPoses)
    -1, -1, -1, -1, -1, -1, -1, -1,         // neck, face and hair
    15, 16, 17, -1, 20, 21, -1,             // pelvis, legs (thigh, shin, foot)
};

// SoH: "Play as Ganondorf", his Gerudo dagger in its sheath, worn as his young self wears it (object_gndd, the
// cutscene model, from the player's ROM). Where it hangs is read from young Ganondorf's own skeleton at runtime: the
// limb it hangs from, its offset and its set, scaled from his younger build to the boss model's by the length of
// their shins. Drawn on the boss model's matching limb.
#define GDF_YOUNG_KNIFE_JOINTS 32
static s8 sGdfKnifeState = 0;     // 0 not looked up yet, 1 found, -1 not found
static s8 sGdfKnifeBossLimb = -1; // the boss limb (as the draw callbacks count them) it hangs from
static Vec3f sGdfKnifePos;        // its joint, in that limb's space
static Vec3s sGdfKnifeRot;        // its set
static f32 sGdfKnifeScale = 1.0f; // young model to boss model
static const char* sGdfKnifeDL = NULL;

// ---- The dagger, drawn ----
// Made when first needed from the game's own models (in the player's ROM; nothing of them is in our files):
//  - the sheath: the young Ganondorf's knife with its hilt's triangles (the Gerudo cloth and the jewelled guard) taken
//    out; the hilt: those triangles alone
//  - the blade: the Kokiri Sword's (its first, mirror-bright part, without its hilt), its vertices copied and bent into
//    a curved blade along the sheath's own curve, so it would slide home into it: the sheath curves on an arc (fitted
//    to it: radius 1634, leaving the mouth along the hilt's line); the blade runs 90% of the way down it
// Knife space: x across the blade's thickness, the curve in y and z.
#define GDF_DAGGER_GFX 400
#define GDF_BLADE_VTX 24
static Gfx sGdfSheathGfx[GDF_DAGGER_GFX];
static Gfx sGdfHiltGfx[GDF_DAGGER_GFX];
static Gfx sGdfBladeGfx[GDF_DAGGER_GFX];
static Vtx sGdfBladeVtx[GDF_BLADE_VTX];
static s8 sGdfDaggerBuilt = 0; // 0 not yet, 1 built, -1 couldn't be

#define GDF_KSWORD_A0 (-5.66f)     // the Kokiri blade's foot, along it ((x + y) / sqrt 2)
#define GDF_KSWORD_LEN 45.3f       // its foot to its point
#define GDF_DAGGER_MOUTH_Y (-3.0f) // the sheath's mouth (knife space)
#define GDF_DAGGER_MOUTH_Z (-52.0f)
#define GDF_DAGGER_MID_X 107.0f    // the middle of its thickness
#define GDF_DAGGER_T0_Y (-0.9353f) // the blade's way out of the mouth (down the hilt's line)
#define GDF_DAGGER_T0_Z (-0.3539f)
#define GDF_DAGGER_N_Y 0.3539f // the way the sheath curves from there
#define GDF_DAGGER_N_Z (-0.9353f)
#define GDF_DAGGER_RADIUS 1634.0f
#define GDF_DAGGER_S_PER_A 30.06f // knife units of curve per unit of Kokiri blade (90% of the sheath's 1513)
#define GDF_DAGGER_WIDTH 22.0f    // across the blade (its half width 7.8 to 172, inside the sheath)
#define GDF_DAGGER_THICK 10.0f
#define GDF_DAGGER_HILT_Y 520.0f // the middle of the hilt, where his fist closes
#define GDF_DAGGER_HILT_Z 150.0f
#define GDF_DAGGER_HILT_X 106.0f

// A point along the curved blade: `a` along the Kokiri blade from its foot, `w` across it, `t` through it

static void BossGanon_Player_DrawKnife(BossGanon* this, PlayState* play, s32 limbIndex) {
    // Play as Ganondorf (2S2H): not ported yet
}

static void BossGanon_Player_RotToMtx(Vec3s* rot, f32 m[3][3]) {
    f32 sx = sinf((f32)BINANG_TO_RAD(rot->x));
    f32 cx = cosf((f32)BINANG_TO_RAD(rot->x));
    f32 sy = sinf((f32)BINANG_TO_RAD(rot->y));
    f32 cy = cosf((f32)BINANG_TO_RAD(rot->y));
    f32 sz = sinf((f32)BINANG_TO_RAD(rot->z));
    f32 cz = cosf((f32)BINANG_TO_RAD(rot->z));

    // Rz * Ry * Rx, the order Matrix_TranslateRotateZYX applies them in
    m[0][0] = cz * cy;
    m[0][1] = (cz * sy * sx) - (sz * cx);
    m[0][2] = (cz * sy * cx) + (sz * sx);
    m[1][0] = sz * cy;
    m[1][1] = (sz * sy * sx) + (cz * cx);
    m[1][2] = (sz * sy * cx) - (cz * sx);
    m[2][0] = -sy;
    m[2][1] = cy * sx;
    m[2][2] = cy * cx;
}

static void BossGanon_Player_MtxToRot(f32 m[3][3], Vec3s* rot) {
    f32 sy = CLAMP(-m[2][0], -1.0f, 1.0f);

    rot->x = RADF_TO_BINANG(atan2f(m[2][1], m[2][2]));
    rot->y = RADF_TO_BINANG(asinf(sy));
    rot->z = RADF_TO_BINANG(atan2f(m[1][0], m[0][0]));
}

static s16 BossGanon_Player_ScaleAngle(s16 base, s16 target, f32 scale) {
    s16 diff = target - base;

    return (s16)(base + (s16)(diff * scale));
}

// Blends the walk over the current (standing) pose in jointTable
static void BossGanon_Player_BlendArm(BossGanon* this, s32 firstLimb, const Vec3s* pose, f32 weight);
static void BossGanon_Player_SlerpRot(Vec3s* a, Vec3s* b, f32 t, Vec3s* out);

// His arms while walking: solved like the sword poses (fists close to his body, forward and back), because young
// Ganondorf's arms hang differently from his torso and swing far out on the boss model
// clang-format off
static const Vec3s sGdfWalkArmPoses[4][3] = {
    { { 15506, 758, -14779 }, { -685, -4771, -6823 }, { 2315, 1027, -1733 } },    // right forward
    { { 19634, -5749, -14194 }, { -1134, -4536, -6861 }, { 2247, 1229, -2127 } }, // right back
    { { -15893, 251, -13672 }, { 25, 2864, -5541 }, { -790, 20, -1990 } },        // left forward
    { { -18870, 6804, -16061 }, { 524, 1983, -4121 }, { -845, 196, -2362 } },     // left back
};
// clang-format on

// How far through the swing the right arm is: back at frame 0, forward at frame 15 (the left arm is opposite)
static f32 BossGanon_Player_WalkArmPhase(BossGanon* this) {
    return 0.5f - (0.5f * Math_CosS((s16)(this->playerWalkFrame * (0x10000 / GDF_WALK_CYCLE_FRAMES))));
}

// Young Ganondorf's pelvis sits turned about 20 degrees on the boss skeleton through the whole walk cycle, which
// swung his legs out to one side (his lower body looked skewed) and crossed his feet. Turned back by this (in actor
// space), his legs run straight on two tracks.
#define GDF_WALK_PELVIS_YAW -3641
#define GDF_WALK_FOOT_SPLAY 1820 // 10 degrees outward
#define GDF_CLIMB_WALL_GAP 10.0f // climbing: further from the wall than Link

// out = (transposeA ? a^T : a) * b
static void BossGanon_Player_Mul3(f32 a[3][3], f32 b[3][3], f32 out[3][3], s32 transposeA) {
    f32 r[3][3];
    s32 i;
    s32 j;
    s32 k;

    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            r[i][j] = 0.0f;
            for (k = 0; k < 3; k++) {
                r[i][j] += (transposeA ? a[k][i] : a[i][k]) * b[k][j];
            }
        }
    }
    memcpy(out, r, sizeof(r));
}

static void BossGanon_Player_ApplyWalk(BossGanon* this) {
    static Vec3s sYoungA[GDF_YOUNG_WALK_JOINTS];
    static Vec3s sYoungB[GDF_YOUNG_WALK_JOINTS];
    static Vec3s sYoung[GDF_YOUNG_WALK_JOINTS];
    static Vec3s sYoungRest;
    static Vec3s sYoungTorsoMean;
    static u8 sYoungRestRead = false;
    Vec3s walk[GDF_LIMB_COUNT_PLAYER + 1];
    Vec3s arm[3];
    Vec3s* joints = this->skelAnime.jointTable;
    f32 speed = this->playerWalkWeight;
    s32 frameA = (s32)this->playerWalkFrame;
    s32 frameB = (frameA + 1) % (s32)GDF_WALK_CYCLE_FRAMES;
    f32 frac = this->playerWalkFrame - frameA;
    f32 stride;
    f32 phase;
    s32 b;
    s32 i;

    if (this->playerWalkWeight <= 0.0f) {
        return;
    }

    if (!sYoungRestRead) {
        f32 sum[3] = { 0.0f, 0.0f, 0.0f };

        for (i = 0; i < (s32)GDF_WALK_CYCLE_FRAMES; i++) {
            SkelAnime_GetFrameData((AnimationHeader*)gYoungGanondorfWalkAnim, i, GDF_YOUNG_WALK_JOINTS, sYoungA);
            if (i == 0) {
                sYoungRest = sYoungA[0];
            }
            sum[0] += sYoungA[2].x;
            sum[1] += sYoungA[2].y;
            sum[2] += sYoungA[2].z;
        }
        sYoungTorsoMean.x = (s16)(sum[0] / GDF_WALK_CYCLE_FRAMES);
        sYoungTorsoMean.y = (s16)(sum[1] / GDF_WALK_CYCLE_FRAMES);
        sYoungTorsoMean.z = (s16)(sum[2] / GDF_WALK_CYCLE_FRAMES);
        sYoungRestRead = true;
    }

    SkelAnime_GetFrameData((AnimationHeader*)gYoungGanondorfWalkAnim, frameA, GDF_YOUNG_WALK_JOINTS, sYoungA);
    SkelAnime_GetFrameData((AnimationHeader*)gYoungGanondorfWalkAnim, frameB, GDF_YOUNG_WALK_JOINTS, sYoungB);
    SkelAnime_InterpFrameTable(GDF_YOUNG_WALK_JOINTS, sYoung, sYoungA, sYoungB, frac);

    stride = CLAMP(this->playerWalkStride, 1.0f, GDF_WALK_MAX_STRIDE + (0.7f * this->playerSprintWeight));

    for (b = 0; b < GDF_LIMB_COUNT_PLAYER; b++) {
        s8 y = sGdfBossToYoungLimb[b];

        walk[1 + b] = (y >= 0) ? sYoung[1 + y] : joints[1 + b];
    }

    // His own upright torso, with a little of the walk's sway
    walk[2].x = (s16)(joints[2].x + (s16)(sYoung[2].x - sYoungTorsoMean.x) * 0.35f);
    walk[2].y = (s16)(joints[2].y + (s16)(sYoung[2].y - sYoungTorsoMean.y) * 0.35f);
    walk[2].z = (s16)(joints[2].z + (s16)(sYoung[2].z - sYoungTorsoMean.z) * 0.35f);

    // Wider stride when running
    for (b = 1; b <= GDF_LIMB_COUNT_PLAYER; b++) {
        walk[b].x = BossGanon_Player_ScaleAngle(joints[b].x, walk[b].x, stride);
        walk[b].y = BossGanon_Player_ScaleAngle(joints[b].y, walk[b].y, stride);
        walk[b].z = BossGanon_Player_ScaleAngle(joints[b].z, walk[b].z, stride);
    }

    // Straighten his pelvis: pelvis = root^T * turn * root * pelvis
    {
        Vec3s turnRot = { 0, GDF_WALK_PELVIS_YAW, 0 };
        f32 root[3][3];
        f32 turn[3][3];
        f32 m[3][3];

        BossGanon_Player_RotToMtx(&walk[1], root);
        BossGanon_Player_RotToMtx(&turnRot, turn);
        BossGanon_Player_RotToMtx(&walk[1 + 18], m);
        BossGanon_Player_Mul3(root, m, m, false);
        BossGanon_Player_Mul3(turn, m, m, false);
        BossGanon_Player_Mul3(root, m, m, true);
        BossGanon_Player_MtxToRot(m, &walk[1 + 18]);
    }

    // Point his feet where he walks. His feet keep the boss animation's own rotation (young Ganondorf's are built
    // differently), which left them turned out (about 30 and 20 degrees) on the walk. Turn each foot about the
    // vertical so its toes (the foot's local +z, 11 degrees out in his standing pose) face forward, splayed a little
    // outward, keeping the foot's pitch from the step. (v37 took local +x for the toes and turned them sideways.)
    {
        static const s8 sLegChains[2][5] = { { 0, 18, 19, 20, 21 }, { 0, 18, 22, 23, 24 } };
        s32 leg;
        s32 k;

        for (leg = 0; leg < 2; leg++) {
            f32 parent[3][3];
            f32 m[3][3];
            f32 turn[3][3];
            Vec3s turnRot = { 0, 0, 0 };
            s16 target = (leg == 0) ? GDF_WALK_FOOT_SPLAY : -GDF_WALK_FOOT_SPLAY;
            s16 toeYaw;

            BossGanon_Player_RotToMtx(&walk[1 + sLegChains[leg][0]], parent);
            for (k = 1; k < 4; k++) {
                BossGanon_Player_RotToMtx(&walk[1 + sLegChains[leg][k]], m);
                BossGanon_Player_Mul3(parent, m, parent, false);
            }
            BossGanon_Player_RotToMtx(&walk[1 + sLegChains[leg][4]], m);
            BossGanon_Player_Mul3(parent, m, m, false); // the foot in actor space
            toeYaw = Math_Atan2S(m[2][2], m[0][2]);     // binang of atan2(x, z) of the toe axis
            turnRot.y = (s16)(target - toeYaw);
            BossGanon_Player_RotToMtx(&turnRot, turn);
            BossGanon_Player_Mul3(turn, m, m, false);
            BossGanon_Player_Mul3(parent, m, m, true); // back into the shin's space
            BossGanon_Player_MtxToRot(m, &walk[1 + sLegChains[leg][4]]);
        }
    }

    // His own standing height, with the walk's bob
    walk[0] = joints[0];
    walk[0].y += (s16)((sYoung[0].y - sYoungRest.y) * stride);

    SkelAnime_InterpFrameTable(GDF_LIMB_COUNT_PLAYER + 1, joints, joints, walk, speed);

    // Arms swing close to his body, opposite to the legs
    phase = BossGanon_Player_WalkArmPhase(this);
    for (i = 0; i < 3; i++) {
        Vec3s fwd = sGdfWalkArmPoses[0][i];
        Vec3s back = sGdfWalkArmPoses[1][i];

        BossGanon_Player_SlerpRot(&back, &fwd, phase, &arm[i]);
    }
    BossGanon_Player_BlendArm(this, 7, arm, speed);
    for (i = 0; i < 3; i++) {
        Vec3s fwd = sGdfWalkArmPoses[2][i];
        Vec3s back = sGdfWalkArmPoses[3][i];

        BossGanon_Player_SlerpRot(&fwd, &back, phase, &arm[i]);
    }
    BossGanon_Player_BlendArm(this, 3, arm, speed);
}

// Cutscenes and conversations: Link is held by the scene, but Ganondorf can walk about on his own with the stick
// (camera-relative, like Link). He stays wherever he walked to until the scene is over, then he's back with Link. Not
// through doors, chests, items, ladders, ledges or on horseback, where his own animations follow Link.
#define GDF_ROAM_MAX_SPEED 5.0f
#define GDF_ROAM_RADIUS 30.0f

void BossGanon_Player_Dance(BossGanon* this, PlayState* play);
void BossGanon_Player_CsKneel(BossGanon* this, PlayState* play);

static s32 BossGanon_Player_ShouldBrood(PlayState* play);

static void BossGanon_Player_UpdateRoam(BossGanon* this, PlayState* play) {
    // Play as Ganondorf (2S2H): not ported yet
}

// Standing and walking, while not levitating: the walk cycle follows Link's speed
// ---- The Mixamo walks ----
// Walking (his walk at walking pace), Walking Backwards (moving backwards while Z-targeting, on the ground) and Strut
// Walking (the menu's alternative walk, in place of Walking: now the "Dr. Livesey" strut, from the player's own file),
// retargeted offline like the other Mixamo clips. They go
// over the old walk (the young Ganondorf walk, sped up), which still takes over at a full run (the stick run).
// clang-format off
static const Vec3s sGdfClipWalk[45][13] = {
    { { 2922, -26797, 13168 }, { -3684, -24668, -16671 }, { 998, -29345, 6153 }, { 62, -29283, -6518 }, { 8081, -28794, -2365 }, { 4308, -29626, 1938 }, { -9116, -28534, 1642 }, { -2880, -27386, 11905 }, { -16759, 3536, -16544 }, { 15228, -566, 16504 }, { 16663, -14356, 16157 }, { 21530, -2798, 20704 }, { -25494, 4055, 15285 } },
    { { 2567, -26514, 13799 }, { -3464, -27318, -11904 }, { 1131, -29732, 3837 }, { -133, -29270, -6576 }, { 7905, -28633, -4204 }, { 4447, -29667, 285 }, { -9714, -28242, 2831 }, { -2354, -26698, 13479 }, { -16685, 3611, -16497 }, { 15505, -576, 16456 }, { 16919, -14383, 15754 }, { 22888, -4793, 18793 }, { -24499, 4326, 16766 } },
    { { 2090, -26502, 13902 }, { -2961, -29197, -6226 }, { 1155, -29927, 1747 }, { -224, -29207, -6849 }, { 7582, -28397, -6013 }, { 4436, -29640, -1335 }, { -10398, -27852, 4022 }, { -1617, -25837, 15161 }, { -16602, 3693, -16452 }, { 15783, -590, 16414 }, { 17166, -14369, 15390 }, { 24015, -6436, 16788 }, { -23269, 4846, 18305 } },
    { { 1478, -26803, 13394 }, { -2141, -29922, 255 }, { 1016, -29981, 281 }, { -102, -29008, -7649 }, { 7075, -28116, -7708 }, { 4282, -29546, -2946 }, { -11174, -27343, 5243 }, { -763, -24804, 16857 }, { -16518, 3771, -16452 }, { 16056, -604, 16380 }, { 17395, -14318, 15061 }, { 24992, -7560, 14774 }, { -21766, 5555, 19884 } },
    { { 1041, -26554, 13922 }, { -1210, -29494, 5350 }, { 641, -29987, -597 }, { 199, -28601, -9053 }, { 6393, -27815, -9244 }, { 3976, -29390, -4519 }, { -11970, -26707, 6593 }, { -152, -23806, 18255 }, { -16387, 3830, -16556 }, { 16316, -606, 16331 }, { 17603, -14229, 14764 }, { 25891, -8321, 12665 }, { -20149, 6638, 21212 } },
    { { 580, -26002, 14952 }, { -60, -28550, 9214 }, { 170, -29973, -1261 }, { 642, -28010, -10724 }, { 5620, -27496, -10602 }, { 3572, -29175, -6007 }, { -12697, -25952, 8080 }, { 270, -22883, 19398 }, { -16185, 3871, -16708 }, { 16554, -587, 16264 }, { 17760, -14106, 14534 }, { 26636, -8984, 10479 }, { -18572, 8207, 22085 } },
    { { 157, -25135, 16377 }, { 1004, -27585, 11749 }, { -172, -29915, -2256 }, { 936, -27403, -12174 }, { 4959, -27186, -11675 }, { 3282, -28901, -7347 }, { -13218, -25161, 9603 }, { 653, -22056, 20325 }, { -16001, 3896, -16799 }, { 16767, -548, 16197 }, { 17866, -13958, 14388 }, { 27159, -9520, 8471 }, { -17152, 10032, 22476 } },
    { { -96, -24147, 17802 }, { 1529, -27019, 12948 }, { -400, -29801, -3430 }, { 1082, -26755, -13528 }, { 4407, -26968, -12382 }, { 3116, -28590, -8538 }, { -13543, -24422, 10962 }, { 943, -21396, 21008 }, { -15851, 3925, -16830 }, { 16977, -518, 16143 }, { 17997, -13815, 14236 }, { 27453, -9959, 6869 }, { -15885, 11754, 22572 } },
    { { -146, -23685, 18412 }, { 1442, -26824, 13357 }, { -507, -29625, -4704 }, { 1063, -26132, -14697 }, { 3950, -26885, -12712 }, { 3035, -28273, -9563 }, { -13693, -23850, 11988 }, { 965, -21007, 21396 }, { -15735, 3953, -16812 }, { 17195, -518, 16102 }, { 18185, -13690, 14026 }, { 27563, -10345, 5765 }, { -14856, 13263, 22436 } },
    { { -107, -23861, 18183 }, { 959, -27080, 12874 }, { -448, -29373, -6085 }, { 805, -25500, -15783 }, { 3625, -26953, -12666 }, { 2994, -28049, -10213 }, { -13737, -23556, 12506 }, { 618, -20968, 21446 }, { -15695, 3913, -16755 }, { 17413, -505, 16066 }, { 18377, -13563, 13800 }, { 27575, -10504, 5413 }, { -14207, 14505, 22085 } },
    { { -183, -24318, 17568 }, { 483, -27528, 11916 }, { -242, -29012, -7632 }, { 366, -24871, -16772 }, { 3428, -27118, -12364 }, { 2947, -28002, -10355 }, { -13773, -23486, 12598 }, { 50, -21194, 21233 }, { -15723, 3783, -16658 }, { 17608, -430, 16029 }, { 18496, -13428, 13643 }, { 27564, -10287, 5867 }, { -13870, 15448, 21656 } },
    { { -392, -24871, 16772 }, { 119, -27999, 10772 }, { 131, -28440, -9547 }, { -262, -24481, -17339 }, { 3502, -27392, -11722 }, { 3042, -28197, -9780 }, { -13717, -23642, 12365 }, { -674, -21642, 20765 }, { -15794, 3675, -16598 }, { 17739, -282, 15997 }, { 18499, -13324, 13615 }, { 27472, -9800, 7017 }, { -13874, 15976, 21267 } },
    { { -601, -25270, 16158 }, { -261, -28465, 9470 }, { 337, -27910, -10996 }, { -770, -23955, -18043 }, { 3903, -27697, -10847 }, { 3377, -28516, -8685 }, { -13519, -24059, 11764 }, { -1528, -22362, 19940 }, { -15875, 3581, -16552 }, { 17776, -111, 15985 }, { 18380, -13291, 13732 }, { 27274, -9423, 8205 }, { -14181, 15964, 21073 } },
    { { -464, -24820, 16845 }, { -813, -29236, 6677 }, { -210, -28072, -10579 }, { -633, -22381, -19967 }, { 4576, -27950, -9893 }, { 3922, -28816, -7365 }, { -13203, -24694, 10766 }, { -2368, -23292, 18759 }, { -15953, 3495, -16519 }, { 17737, 30, 15990 }, { 18196, -13312, 13946 }, { 27109, -8964, 9204 }, { -14716, 15496, 21055 } },
    { { -96, -24071, 17905 }, { -1524, -29836, 2740 }, { -1305, -28738, -8510 }, { -33, -19780, -22555 }, { 5212, -28172, -8898 }, { 4344, -29097, -5873 }, { -13010, -25362, 9354 }, { -3260, -24337, 17236 }, { -16033, 3417, -16530 }, { 17656, 120, 15980 }, { 18026, -13340, 14168 }, { 27007, -8222, 10149 }, { -15296, 14744, 21181 } },
    { { 189, -23975, 18033 }, { -2100, -29926, -196 }, { -2400, -29364, -5658 }, { 584, -16923, -24764 }, { 5854, -28381, -7764 }, { 4663, -29349, -4106 }, { -12813, -25987, 7778 }, { -3835, -25279, 15692 }, { -16125, 3346, -16550 }, { 17551, 177, 15977 }, { 17938, -13333, 14297 }, { 26976, -7053, 11071 }, { -15894, 13805, 21373 } },
    { { 321, -24221, 17698 }, { -2502, -29790, -2507 }, { -3239, -29727, -2409 }, { 1195, -14444, -26267 }, { 6531, -28565, -6432 }, { 4915, -29526, -2013 }, { -12438, -26556, 6329 }, { -4334, -26143, 14062 }, { -16222, 3281, -16562 }, { 17392, 205, 16004 }, { 17914, -13295, 14326 }, { 26942, -5721, 11891 }, { -16496, 12667, 21620 } },
    { { 307, -24677, 17058 }, { -2705, -29565, -4314 }, { -3650, -29763, 925 }, { 1629, -13089, -26945 }, { 7123, -28728, -4895 }, { 5005, -29577, 368 }, { -11922, -27046, 5138 }, { -4914, -26882, 12378 }, { -16317, 3223, -16541 }, { 17122, 198, 16065 }, { 17856, -13257, 14349 }, { 26787, -4515, 12730 }, { -17096, 11448, 21833 } },
    { { 189, -25341, 16056 }, { -2732, -29332, -5671 }, { -3784, -29472, 4135 }, { 1953, -12952, -26989 }, { 7514, -28863, -3237 }, { 4817, -29471, 2870 }, { -11385, -27459, 4047 }, { -5617, -27485, 10630 }, { -16413, 3172, -16505 }, { 16759, 173, 16135 }, { 17722, -13256, 14433 }, { 26417, -3361, 13815 }, { -17833, 10297, 21817 } },
    { { -31, -26168, 14671 }, { -2575, -29150, -6606 }, { -3833, -28900, 7078 }, { 2231, -14066, -26404 }, { 7701, -28954, -1542 }, { 4259, -29198, 5415 }, { -10892, -27806, 2863 }, { -6359, -27954, 8839 }, { -16514, 3128, -16471 }, { 16345, 146, 16194 }, { 17545, -13326, 14566 }, { 25731, -2100, 15280 }, { -18987, 9179, 21336 } },
    { { -291, -27014, 13043 }, { -2290, -29010, -7292 }, { -3856, -28102, 9767 }, { 2490, -15902, -25317 }, { 7762, -28978, 96 }, { 3324, -28736, 7950 }, { -10460, -28077, 1500 }, { -6927, -28328, 7037 }, { -16618, 3092, -16441 }, { 15940, 136, 16234 }, { 17377, -13463, 14727 }, { 24716, -698, 16989 }, { -20575, 7923, 20345 } },
    { { -502, -27779, 11316 }, { -1980, -28865, -7929 }, { -3730, -27143, 12221 }, { 2625, -18034, -23830 }, { 7804, -28920, 1652 }, { 2127, -28057, 10405 }, { -10009, -28281, 67 }, { -7221, -28636, 5273 }, { -16724, 3062, -16412 }, { 15569, 148, 16262 }, { 17203, -13626, 14936 }, { 23412, 777, 18742 }, { -22385, 6376, 18928 } },
    { { -655, -28472, 9429 }, { -1681, -28724, -8490 }, { -3453, -26092, 14398 }, { 2568, -20435, -21814 }, { 7874, -28773, 3175 }, { 764, -27182, 12672 }, { -9538, -28412, -1326 }, { -7315, -28873, 3579 }, { -16832, 3041, -16379 }, { 15231, 176, 16293 }, { 16989, -13784, 15233 }, { 21826, 2033, 20482 }, { -24182, 4457, 17186 } },
    { { -863, -29072, 7355 }, { -1347, -28610, -8925 }, { -3200, -24969, 16318 }, { 2420, -22939, -19181 }, { 7963, -28549, 4640 }, { -941, -26068, 14817 }, { -9047, -28472, -2740 }, { -7257, -29046, 1920 }, { -16942, 3044, -16349 }, { 14901, 182, 16334 }, { 16755, -13934, 15575 }, { 20082, 3073, 22074 }, { -25668, 2313, 15356 } },
    { { -1106, -29521, 5226 }, { -999, -28498, -9322 }, { -2995, -23832, 17974 }, { 2200, -25348, -15895 }, { 8111, -28260, 5965 }, { -3142, -24656, 16799 }, { -8538, -28443, -4254 }, { -7032, -29162, 363 }, { -17051, 3073, -16330 }, { 14570, 152, 16382 }, { 16511, -14058, 15933 }, { 18331, 4306, 23354 }, { -26712, 147, 13655 } },
    { { -1264, -29803, 3188 }, { -726, -28348, -9790 }, { -2707, -22825, 19279 }, { 1734, -27463, -11949 }, { 8379, -27913, 7119 }, { -5577, -23069, 18350 }, { -7987, -28326, -5819 }, { -6653, -29236, -1005 }, { -17156, 3114, -16326 }, { 14251, 94, 16430 }, { 16243, -14123, 16305 }, { 16719, 6193, 24127 }, { -27369, -1801, 12154 } },
    { { -1283, -29949, 1193 }, { -578, -28169, -10303 }, { -2183, -22118, 20150 }, { 906, -29055, -7417 }, { 8739, -27495, 8224 }, { -7979, -21413, 19438 }, { -7341, -28162, -7281 }, { -6159, -29278, -2209 }, { -17255, 3192, -16334 }, { 13958, 21, 16477 }, { 15962, -14132, 16678 }, { 15169, 8560, 24426 }, { -27835, -3488, 10633 } },
    { { -1195, -29965, -837 }, { -548, -27983, -10801 }, { -1387, -21988, 20362 }, { -268, -29910, -2304 }, { 9103, -27002, 9383 }, { -10155, -19819, 20102 }, { -6664, -27968, -8566 }, { -5645, -29282, -3272 }, { -17366, 3285, -16316 }, { 13693, -52, 16525 }, { 15719, -14086, 16997 }, { 13750, 11122, 24233 }, { -28200, -4919, 8977 } },
    { { -1011, -29867, -2635 }, { -614, -27685, -11540 }, { -573, -22449, 19892 }, { -1336, -29871, 2443 }, { 9516, -26406, 10592 }, { -12099, -18328, 20437 }, { -5944, -27774, -9657 }, { -5095, -29263, -4212 }, { -17532, 3397, -16220 }, { 13454, -114, 16579 }, { 15570, -14000, 17197 }, { 12778, 13764, 23394 }, { -28445, -6211, 7232 } },
    { { -904, -29676, -4306 }, { -644, -27291, -12442 }, { -77, -23320, 18872 }, { -1874, -29171, 6747 }, { 9841, -25767, 11799 }, { -13952, -16888, 20498 }, { -5360, -27566, -10554 }, { -4694, -29197, -5051 }, { -17670, 3509, -16176 }, { 13242, -159, 16624 }, { 15490, -13908, 17299 }, { 12546, 16245, 21879 }, { -28606, -7174, 5497 } },
    { { -1082, -29298, -6362 }, { -359, -27082, -12901 }, { 264, -24790, 16893 }, { -2025, -27870, 10916 }, { 10128, -25133, 12876 }, { -15403, -15747, 20366 }, { -4910, -27380, -11235 }, { -4420, -29104, -5782 }, { -17756, 3631, -16162 }, { 13086, -210, 16671 }, { 15452, -13849, 17341 }, { 12952, 18263, 19968 }, { -28699, -7810, 3919 } },
    { { -1458, -28716, -8559 }, { 184, -26931, -13217 }, { 424, -25819, 15271 }, { -1842, -27071, 12798 }, { 10379, -24583, 13709 }, { -16287, -15068, 20191 }, { -4552, -27239, -11717 }, { -4202, -29011, -6381 }, { -17800, 3718, -16157 }, { 12989, -266, 16715 }, { 15432, -13811, 17371 }, { 13476, 19640, 18239 }, { -28697, -8349, 2597 } },
    { { -1628, -28272, -9903 }, { 583, -26259, -14496 }, { 512, -25303, 16109 }, { -1469, -27784, 11221 }, { 10615, -24211, 14182 }, { -16560, -14829, 20146 }, { -4304, -27189, -11927 }, { -4039, -28940, -6794 }, { -17793, 3705, -16205 }, { 12941, -291, 16740 }, { 15426, -13748, 17406 }, { 13800, 20412, 17115 }, { -28595, -8923, 1637 } },
    { { -1854, -27795, -11136 }, { 1081, -25487, -15788 }, { 645, -25025, 16533 }, { -1084, -28452, 9452 }, { 10853, -24136, 14131 }, { -16233, -14984, 20297 }, { -4233, -27271, -11763 }, { -3977, -28917, -6926 }, { -17723, 3589, -16327 }, { 12910, -262, 16752 }, { 15447, -13628, 17414 }, { 13974, 20666, 16663 }, { -28445, -9455, 1205 } },
    { { -2019, -27351, -12160 }, { 1455, -24740, -16906 }, { 516, -24261, 17640 }, { -576, -29279, 6510 }, { 10949, -24381, 13627 }, { -15514, -15381, 20561 }, { -4514, -27459, -11209 }, { -4213, -28951, -6640 }, { -17627, 3502, -16433 }, { 12878, -203, 16757 }, { 15535, -13500, 17338 }, { 14514, 20539, 16355 }, { -28364, -9672, 1385 } },
    { { -1346, -27580, -11728 }, { 889, -23177, -19027 }, { 257, -23775, 18295 }, { 74, -29775, 3668 }, { 11011, -24896, 12607 }, { -14308, -16280, 20742 }, { -4997, -27713, -10346 }, { -4592, -29062, -5857 }, { -17539, 3468, -16485 }, { 12841, -141, 16767 }, { 15711, -13437, 17165 }, { 15445, 20177, 15948 }, { -28256, -9820, 2271 } },
    { { 331, -28547, -9217 }, { -628, -20122, -22242 }, { -19, -23638, 18472 }, { 732, -29964, 1270 }, { 11077, -25523, 11220 }, { -12619, -17748, 20634 }, { -5518, -27988, -9287 }, { -4944, -29228, -4612 }, { -17459, 3452, -16540 }, { 12820, -89, 16767 }, { 15883, -13452, 17014 }, { 16435, 19721, 15522 }, { -28054, -9953, 3734 } },
    { { 1689, -29219, -6589 }, { -1719, -17225, -24502 }, { -265, -23890, 18144 }, { 1353, -29953, -988 }, { 10909, -26184, 9766 }, { -10680, -19566, 20078 }, { -6185, -28220, -8086 }, { -5387, -29360, -2995 }, { -17372, 3455, -16545 }, { 12846, -54, 16740 }, { 15960, -13506, 16988 }, { 17456, 19067, 15222 }, { -27938, -9473, 5453 } },
    { { 2545, -29693, -3445 }, { -2353, -14735, -26026 }, { -399, -24321, 17560 }, { 1835, -29808, -2846 }, { 10517, -26799, 8438 }, { -8453, -21714, 18895 }, { -6865, -28380, -6888 }, { -5790, -29412, -1178 }, { -17284, 3471, -16524 }, { 12989, -76, 16688 }, { 15921, -13565, 17093 }, { 18337, 18045, 15431 }, { -28062, -8062, 6894 } },
    { { 2981, -29851, -113 }, { -2722, -13095, -26854 }, { -411, -24888, 16746 }, { 2114, -29618, -4279 }, { 10014, -27343, 7216 }, { -5941, -23932, 17088 }, { -7460, -28484, -5747 }, { -6058, -29374, 680 }, { -17204, 3492, -16521 }, { 13254, -154, 16628 }, { 15867, -13628, 17197 }, { 18846, 16429, 16581 }, { -28351, -6040, 7731 } },
    { { 3179, -29671, 3083 }, { -3058, -12727, -26994 }, { -299, -25564, 15697 }, { 2124, -29448, -5322 }, { 9495, -27813, 6022 }, { -3161, -25962, 14695 }, { -7923, -28569, -4585 }, { -6106, -29258, 2583 }, { -17130, 3515, -16540 }, { 13615, -260, 16580 }, { 15868, -13714, 17221 }, { 18943, 13918, 18640 }, { -28664, -3801, 7996 } },
    { { 3316, -29224, 5914 }, { -3434, -13776, -26428 }, { -42, -26367, 14310 }, { 1837, -29326, -6050 }, { 9047, -28215, 4699 }, { -861, -27396, 12195 }, { -8278, -28646, -3298 }, { -5888, -29062, 4555 }, { -17056, 3536, -16555 }, { 14020, -351, 16551 }, { 15938, -13831, 17152 }, { 18996, 10551, 20684 }, { -28800, -1778, 8208 } },
    { { 3417, -28605, 8371 }, { -3837, -15775, -25227 }, { 295, -27213, 12624 }, { 1365, -29245, -6549 }, { 8679, -28535, 3228 }, { 1011, -28392, 9636 }, { -8520, -28700, -1920 }, { -5384, -28779, 6541 }, { -16980, 3554, -16554 }, { 14414, -417, 16537 }, { 16046, -13967, 17024 }, { 19258, 6905, 21942 }, { -28618, -185, 8997 } },
    { { 3386, -27933, 10407 }, { -4080, -18515, -23250 }, { 615, -28029, 10678 }, { 844, -29214, -6770 }, { 8422, -28752, 1552 }, { 2530, -29055, 7029 }, { -8684, -28710, -576 }, { -4632, -28403, 8474 }, { -16905, 3562, -16549 }, { 14758, -468, 16530 }, { 16190, -14112, 16828 }, { 19814, 3336, 22277 }, { -27967, 1116, 10797 } },
    { { 3201, -27303, 12012 }, { -4019, -21608, -20419 }, { 864, -28758, 8500 }, { 388, -29233, -6731 }, { 8237, -28845, -335 }, { 3612, -29447, 4449 }, { -8860, -28655, 618 }, { -3769, -27931, 10280 }, { -16832, 3557, -16547 }, { 15031, -517, 16519 }, { 16394, -14247, 16537 }, { 20608, 8, 21801 }, { -26856, 2495, 13136 } },
};
// clang-format on
// clang-format off
static const Vec3s sGdfClipWalkBack[36][13] = {
    { { 3396, -27770, 10830 }, { 717, -25511, -15770 }, { -497, -28788, 8427 }, { -714, -28010, -10721 }, { 7170, -24617, -15575 }, { 1623, -29773, 3306 }, { -6839, -29030, -3237 }, { 616, -25859, 15197 }, { -16723, 1863, -16411 }, { 18107, -909, 16341 }, { 16663, -14356, 16157 }, { 27326, 4596, 11497 }, { -25690, 8606, 12882 } },
    { { 3386, -28669, 8164 }, { 809, -23218, -18981 }, { -373, -28396, 9671 }, { -720, -28180, -10266 }, { 8417, -25604, -13175 }, { 2484, -29374, 5569 }, { -7247, -28196, -7243 }, { -6, -26541, 13986 }, { -16979, 1727, -16435 }, { 17994, -675, 16424 }, { 17229, -14227, 15625 }, { 27330, 5891, 10879 }, { -26805, 6470, 11817 } },
    { { 3547, -29324, 5248 }, { 726, -21003, -21409 }, { -285, -27861, 11121 }, { -712, -28288, -9964 }, { 9588, -26335, -10702 }, { 3026, -28641, 8398 }, { -7652, -26763, -11188 }, { -77, -27124, 12818 }, { -17226, 1591, -16534 }, { 17723, -455, 16526 }, { 17259, -14092, 15682 }, { 27039, 7107, 10881 }, { -27758, 4730, 10350 } },
    { { 3471, -29720, 2156 }, { 950, -19231, -23006 }, { -277, -27234, 12579 }, { -663, -28408, -9619 }, { 10590, -26772, -8433 }, { 3310, -27478, 11577 }, { -8259, -24981, -14413 }, { 447, -27793, 11285 }, { -17445, 1455, -16660 }, { 17337, -241, 16588 }, { 17110, -13927, 15933 }, { 26400, 8325, 11565 }, { -28536, 3384, 8616 } },
    { { 3245, -29804, -1083 }, { 1439, -18363, -23679 }, { -394, -26607, 13854 }, { -550, -28606, -9021 }, { 11484, -26936, -6524 }, { 3377, -25891, 14774 }, { -8388, -23254, -16997 }, { 1160, -28479, 9361 }, { -17632, 1321, -16750 }, { 16901, -57, 16630 }, { 17114, -13820, 16028 }, { 25271, 8676, 13643 }, { -29144, 2412, 6692 } },
    { { 2968, -29587, -3979 }, { 2006, -18478, -23549 }, { -580, -25978, 14993 }, { -448, -28876, -8124 }, { 12159, -26897, -5357 }, { 3112, -23640, 18207 }, { -7894, -21845, -18986 }, { 2166, -29028, 7258 }, { -17816, 1201, -16790 }, { 16369, 91, 16641 }, { 16820, -13718, 16393 }, { 23800, 9646, 15509 }, { -29579, 1723, 4702 } },
    { { 2634, -29335, -5701 }, { 2414, -19213, -22914 }, { -676, -25293, 16119 }, { -555, -29204, -6844 }, { 12682, -26849, -4277 }, { 2506, -20813, 21460 }, { -7044, -20678, -20562 }, { 3635, -29391, 4794 }, { -18040, 1139, -16797 }, { 15929, 128, 16638 }, { 16511, -13574, 16749 }, { 22438, 10968, 16621 }, { -29855, 1310, 2640 } },
    { { 2285, -29337, -5844 }, { 2254, -20365, -21913 }, { -651, -24637, 17105 }, { -929, -29588, -4870 }, { 13110, -26880, -2361 }, { 1072, -17636, 24245 }, { -5950, -19802, -21737 }, { 4803, -29545, 2012 }, { -18295, 1176, -16787 }, { 15667, 0, 16610 }, { 16354, -13424, 16842 }, { 21668, 12078, 16871 }, { -29966, 1353, 427 } },
    { { 1986, -29523, -4946 }, { 1074, -21637, -20753 }, { -626, -24159, 17775 }, { -1428, -29897, -2027 }, { 13199, -26937, 459 }, { -1327, -14479, 26241 }, { -4833, -19275, -22475 }, { 5266, -29533, -218 }, { -18525, 1303, -16756 }, { 15530, -226, 16578 }, { 16288, -13247, 16828 }, { 21515, 12449, 16797 }, { -29913, 1685, -1538 } },
    { { 1880, -29748, -3396 }, { 222, -22461, -19886 }, { -675, -23944, 18061 }, { -1844, -29909, 1429 }, { 12789, -26961, 3092 }, { -3646, -12115, 27202 }, { -4097, -18860, -22967 }, { 5508, -29453, -1486 }, { -18699, 1460, -16645 }, { 15497, -526, 16620 }, { 16241, -13114, 16897 }, { 22173, 12596, 15801 }, { -29844, 1857, -2432 } },
    { { 1805, -29871, -2108 }, { -393, -23388, -18784 }, { -794, -23530, 18593 }, { -2089, -29675, 3878 }, { 11722, -27250, 4476 }, { -5587, -10568, 27515 }, { -4136, -18664, -23120 }, { 5024, -29531, -1645 }, { -18794, 1585, -16365 }, { 15485, -815, 16615 }, { 16051, -12973, 17095 }, { 23775, 12441, 13416 }, { -29927, 1652, -1286 } },
    { { 1900, -29940, 21 }, { -835, -23863, -18162 }, { -986, -22842, 19424 }, { -2249, -29445, 5286 }, { 10814, -27535, 4990 }, { -6580, -9626, 27641 }, { -4919, -18835, -22827 }, { 4327, -29658, -1303 }, { -18791, 1696, -16063 }, { 15520, -1079, 16634 }, { 15971, -12888, 17233 }, { 25630, 11473, 10558 }, { -29898, 1265, 2122 } },
    { { 1967, -29785, 2997 }, { -1218, -24198, -17691 }, { -1547, -22731, 19517 }, { -2610, -29100, 6808 }, { 9972, -27881, 4815 }, { -6780, -9577, 27610 }, { -6124, -19732, -21752 }, { 3058, -29841, -399 }, { -18678, 1828, -15937 }, { 15459, -1248, 16587 }, { 15816, -12909, 17405 }, { 27352, 9944, 7278 }, { -29243, 976, 6624 } },
    { { 1145, -29583, 4850 }, { -799, -24951, -16638 }, { -2231, -23401, 18639 }, { -2420, -28915, 7620 }, { 9295, -28245, 3978 }, { -5923, -10988, 27280 }, { -7232, -21220, -19935 }, { 1404, -29960, 646 }, { -18352, 1889, -16059 }, { 15411, -1369, 16527 }, { 15736, -13009, 17452 }, { 28584, 8240, 3883 }, { -28044, 571, 10640 } },
    { { 288, -29469, 5611 }, { -304, -25972, -15013 }, { -2907, -24725, 16740 }, { -1499, -29166, 6864 }, { 9038, -28496, 2508 }, { -3649, -14267, 26137 }, { -7964, -23255, -17198 }, { -266, -29979, 1093 }, { -17799, 1820, -16392 }, { 15553, -1438, 16485 }, { 16096, -13147, 17044 }, { 29228, 6726, 680 }, { -26726, -165, 13628 } },
    { { -244, -29427, 5828 }, { 3, -26937, -13205 }, { -3491, -25780, 14940 }, { -523, -29749, 3836 }, { 9021, -28611, -171 }, { -374, -18500, 23613 }, { -8380, -25350, -13680 }, { -2017, -29886, 1660 }, { -17271, 1743, -16670 }, { 15644, -1369, 16391 }, { 16011, -13315, 17039 }, { 29319, 5789, -2623 }, { -25298, -861, 16101 } },
    { { -563, -29358, 6148 }, { 245, -27635, -11671 }, { -3887, -26166, 14151 }, { 337, -29973, -1227 }, { 8493, -28599, -3154 }, { 1793, -22343, 19940 }, { -8622, -26732, -10538 }, { -3037, -29713, 2817 }, { -16889, 1699, -16815 }, { 15765, -1256, 16326 }, { 15816, -13540, 17236 }, { 29079, 5261, -5169 }, { -23618, -1051, 18468 } },
    { { -766, -29195, 6862 }, { 451, -28049, -10633 }, { -3971, -26733, 13021 }, { 695, -29305, -6383 }, { 7608, -28449, -5723 }, { 2399, -25214, 16078 }, { -9097, -27383, -8212 }, { -3262, -29426, 4842 }, { -16569, 1636, -16937 }, { 15988, -1088, 16226 }, { 15606, -13754, 17497 }, { 28654, 5761, -6765 }, { -21821, -385, 20584 } },
    { { -908, -28923, 7913 }, { 641, -28291, -9961 }, { -3884, -27511, 11316 }, { 766, -27916, -10959 }, { 6392, -28228, -7894 }, { 2915, -27357, 11962 }, { -9769, -27623, -6444 }, { -2823, -28922, 7453 }, { -16288, 1564, -17029 }, { 16236, -924, 16134 }, { 15367, -13834, 17806 }, { 27939, 6928, -8452 }, { -20005, 1044, 22332 } },
    { { -948, -28529, 9230 }, { 760, -28439, -9522 }, { -3937, -28307, 9121 }, { 936, -26114, -14737 }, { 5242, -27973, -9488 }, { 2684, -28908, 7556 }, { -10462, -27688, -4893 }, { -2131, -28119, 10237 }, { -16068, 1505, -17046 }, { 16528, -744, 16031 }, { 15164, -13820, 18056 }, { 27621, 7758, -8769 }, { -18097, 2680, 23777 } },
    { { -1027, -28020, 10670 }, { 914, -28547, -9178 }, { -4043, -29010, 6488 }, { 1143, -24220, -17666 }, { 4263, -27574, -11024 }, { 2066, -29747, 3293 }, { -11024, -27687, -3449 }, { -1188, -27043, 12933 }, { -15909, 1445, -16985 }, { 16883, -584, 15934 }, { 14974, -13760, 18229 }, { 28049, 7480, -7569 }, { -16324, 4409, 24781 } },
    { { -1177, -27453, 12039 }, { 1117, -28677, -8740 }, { -3854, -29543, 3515 }, { 989, -22563, -19747 }, { 3429, -27024, -12568 }, { 1261, -29971, -369 }, { -11338, -27690, -2164 }, { 162, -25722, 15438 }, { -15793, 1384, -16876 }, { 17331, -421, 15862 }, { 15037, -13681, 18073 }, { 28630, 6988, -5610 }, { -14939, 5965, 25323 } },
    { { -1372, -26841, 13329 }, { 1368, -28843, -8138 }, { -3270, -29816, 581 }, { 317, -21313, -21111 }, { 2600, -26532, -13759 }, { 308, -29898, -2457 }, { -11481, -27697, -1026 }, { 1914, -24176, 17660 }, { -15663, 1342, -16746 }, { 17860, -257, 15800 }, { 15112, -13636, 17820 }, { 29254, 5867, -3124 }, { -13780, 6926, 25732 } },
    { { -1631, -26126, 14654 }, { 1707, -29032, -7366 }, { -2764, -29820, -1774 }, { -289, -20465, -21934 }, { 1767, -25768, -15261 }, { -767, -29777, -3572 }, { -11577, -27676, 59 }, { 4002, -22513, 19420 }, { -15473, 1350, -16615 }, { 18451, -154, 15752 }, { 15163, -13683, 17585 }, { 29641, 4623, -126 }, { -12404, 7294, 26324 } },
    { { -1972, -25347, 15925 }, { 2186, -29258, -6260 }, { -2452, -29716, -3307 }, { -609, -20120, -22244 }, { 1112, -24855, -16763 }, { -1723, -29653, -4211 }, { -11584, -27645, 1255 }, { 6126, -20911, 20620 }, { -15243, 1416, -16519 }, { 19039, -128, 15716 }, { 15318, -13784, 17283 }, { 29675, 3390, 2812 }, { -10733, 7673, 26943 } },
    { { -2179, -24651, 16958 }, { 2609, -29521, -4660 }, { -2221, -29629, -4147 }, { -862, -20615, -21778 }, { 267, -23633, -18478 }, { -2933, -29481, -4717 }, { -11465, -27602, 2581 }, { 7915, -19514, 21367 }, { -15034, 1512, -16508 }, { 19477, -209, 15685 }, { 15349, -13889, 17170 }, { 29524, 2435, 4731 }, { -9057, 8425, 27331 } },
    { { -2172, -24052, 17798 }, { 2873, -29753, -2548 }, { -2192, -29622, -4210 }, { -971, -21820, -20566 }, { -362, -22135, -20246 }, { -4009, -29303, -5026 }, { -11358, -27483, 3962 }, { 9398, -18168, 21945 }, { -14896, 1653, -16625 }, { 19851, -361, 15679 }, { 15677, -13930, 16831 }, { 29499, 1654, 5200 }, { -7637, 9381, 27453 } },
    { { -1927, -23483, 18569 }, { 2906, -29859, -26 }, { -2202, -29708, -3551 }, { -868, -22880, -19384 }, { -737, -20881, -21528 }, { -4768, -29163, -5174 }, { -11211, -27342, 5171 }, { 10527, -16891, 22447 }, { -14801, 1864, -16875 }, { 20062, -653, 15702 }, { 15996, -13946, 16592 }, { 29582, 1120, 4864 }, { -7086, 10168, 27320 } },
    { { -1470, -23008, 19195 }, { 2839, -29761, 2500 }, { -2231, -29850, -1999 }, { -815, -23707, -18366 }, { -747, -19942, -22400 }, { -5145, -29121, -5048 }, { -10990, -27202, 6265 }, { 11021, -15972, 22879 }, { -14697, 2092, -17214 }, { 20148, -991, 15708 }, { 16132, -13859, 16586 }, { 29574, 890, 4960 }, { -7763, 11055, 26787 } },
    { { -1114, -22320, 20015 }, { 3112, -29570, 3989 }, { -2223, -29916, 331 }, { -552, -24187, -17739 }, { -340, -19397, -22884 }, { -5092, -29228, -4450 }, { -10531, -27061, 7535 }, { 10737, -15530, 23314 }, { -14619, 2301, -17540 }, { 20041, -1267, 15716 }, { 16045, -13603, 16774 }, { 29451, 985, 5628 }, { -9445, 12497, 25586 } },
    { { -720, -22160, 20209 }, { 3189, -29376, 5187 }, { -2245, -29761, 3042 }, { -240, -24587, -17187 }, { 405, -19173, -23070 }, { -4676, -29433, -3437 }, { -9946, -27117, 8108 }, { 10141, -15657, 23495 }, { -14656, 2465, -17704 }, { 19946, -1464, 15810 }, { 16533, -13381, 16456 }, { 29391, 1494, 5826 }, { -11734, 13370, 24157 } },
    { { -194, -22800, 19496 }, { 2853, -29306, 5746 }, { -2058, -29526, 4895 }, { -203, -25320, -16089 }, { 1252, -19443, -22812 }, { -3958, -29590, -2957 }, { -9371, -27455, 7644 }, { 8836, -16529, 23425 }, { -14862, 2501, -17648 }, { 19676, -1621, 15859 }, { 16592, -13277, 16514 }, { 29172, 2006, 6705 }, { -14457, 13504, 22553 } },
    { { 970, -23452, 18683 }, { 2069, -29698, 3709 }, { -1723, -29351, 5961 }, { -281, -26090, -14807 }, { 2256, -20430, -21852 }, { -2833, -29743, -2703 }, { -8511, -28134, 6005 }, { 7072, -17945, 22978 }, { -15188, 2425, -17462 }, { 19339, -1591, 15972 }, { 16328, -13357, 16913 }, { 28850, 2012, 7976 }, { -17423, 12650, 20891 } },
    { { 2036, -24342, 17417 }, { 1206, -29967, -732 }, { -1396, -29226, 6624 }, { -341, -26735, -13605 }, { 3420, -21388, -20757 }, { -1705, -29905, -1667 }, { -7685, -28757, 3737 }, { 5174, -20055, 21703 }, { -15567, 2305, -17215 }, { 19035, -1437, 16067 }, { 16470, -13699, 16831 }, { 28375, 2131, 9504 }, { -20260, 11526, 18886 } },
    { { 2776, -25532, 15506 }, { 677, -29391, -5976 }, { -1079, -29130, 7092 }, { -454, -27297, -12437 }, { 4729, -22348, -19447 }, { -670, -29990, 351 }, { -6989, -29134, 1540 }, { 3373, -22453, 19608 }, { -15971, 2162, -16928 }, { 18770, -1255, 16151 }, { 16970, -14053, 16216 }, { 27891, 2610, 10737 }, { -22638, 10600, 16587 } },
    { { 3089, -26644, 13436 }, { 602, -27818, -11216 }, { -776, -29013, 7594 }, { -593, -27739, -11409 }, { 5954, -23547, -17609 }, { 520, -29949, 1663 }, { -6627, -29252, -631 }, { 1712, -24481, 17255 }, { -16366, 2010, -16651 }, { 18454, -1074, 16241 }, { 16966, -14242, 16034 }, { 27563, 3453, 11329 }, { -24420, 9709, 14472 } },
};
// clang-format on
// clang-format off
static const Vec3s sGdfClipStrut[35][13] = {
    { { 11, -26228, 14563 }, { 2, -25429, -15918 }, { -368, -26861, 13356 }, { 533, 4221, -29697 }, { 4454, -28901, -6701 }, { 2050, -29589, 4505 }, { -2974, -28566, -8668 }, { -6488, -29204, 2245 }, { 13592, 2161, 15796 }, { 13638, 1049, 16102 }, { 10086, -11565, 22191 }, { 29999, -33, -207 }, { -25863, 2486, -14997 } },
    { { -16, -28218, 10186 }, { 1, -22998, -19264 }, { -321, -25334, 16066 }, { 509, 284, -29994 }, { 4242, -28901, -6838 }, { 2190, -29589, 4439 }, { -3245, -28566, -8571 }, { -6414, -29204, 2448 }, { 13927, 2179, 15865 }, { 13968, 1057, 16135 }, { 10730, -11731, 21611 }, { 29978, -33, -1149 }, { -26321, 2486, -14177 } },
    { { -50, -29560, 5119 }, { -5, -21559, -20862 }, { -225, -21506, 20916 }, { 459, -9511, -28449 }, { 3652, -28901, -7170 }, { 2555, -29589, 4239 }, { -3953, -28566, -8268 }, { -6186, -29204, 2978 }, { 14823, 2215, 16053 }, { 14849, 1074, 16225 }, { 12660, -12087, 19855 }, { 29776, -33, -3662 }, { -27419, 2486, -11917 } },
    { { -18, -29998, 301 }, { -3, -21330, -21095 }, { -97, -18258, 23804 }, { 329, -19899, -22448 }, { 2758, -28901, -7559 }, { 3049, -29589, 3899 }, { -4925, -28566, -7729 }, { -5780, -29204, 3704 }, { 16117, 2238, 16327 }, { 16121, 1086, 16357 }, { 15858, -12331, 16906 }, { 29114, -33, -7238 }, { -28660, 2486, -8512 } },
    { { 121, -29833, -3163 }, { 42, -21822, -20587 }, { 34, -17718, 24209 }, { 98, -26572, -13925 }, { 1661, -28901, -7873 }, { 3570, -29589, 3428 }, { -5968, -28566, -6954 }, { -5198, -29204, 4484 }, { 17632, 2223, 16649 }, { 17609, 1079, 16511 }, { 19676, -12152, 13379 }, { 27797, -33, -11283 }, { -29575, 2486, -4373 } },
    { { 369, -29745, -3886 }, { 139, -22052, -20340 }, { -13, -18921, 23281 }, { 1, -29191, -6920 }, { 507, -28901, -8031 }, { 4028, -29589, 2877 }, { -6910, -28566, -6020 }, { -4496, -29204, 5188 }, { 19176, 2161, 16972 }, { 19128, 1049, 16666 }, { 22938, -11515, 10418 }, { 25877, -33, -15179 }, { -29897, 2486, -56 } },
    { { 709, -29559, -5076 }, { 250, -20709, -21705 }, { -343, -19751, 22579 }, { 172, -29799, -3462 }, { -573, -29055, -7447 }, { 4138, -29615, 2420 }, { -7230, -28743, -4642 }, { -3517, -29253, 5646 }, { 20602, 2063, 17259 }, { 20526, 1002, 16804 }, { 25201, -10643, 8438 }, { 23654, -280, -18450 }, { -29653, 2227, 3967 } },
    { { 1139, -29169, -6918 }, { 299, -17212, -24569 }, { -936, -19726, 22583 }, { 575, -29913, -2214 }, { -1303, -29378, -5938 }, { 3857, -29667, 2238 }, { -6588, -29120, -2934 }, { -2273, -29362, 5719 }, { 21854, 1945, 17499 }, { 21743, 945, 16920 }, { 26703, -9737, 7190 }, { 21418, -873, -20988 }, { -29007, 1608, 7485 } },
    { { 1605, -28404, -9519 }, { 264, -12893, -27087 }, { -1710, -19167, 23015 }, { 1190, -29881, -2394 }, { -1560, -29675, -4121 }, { 3445, -29712, 2302 }, { -5357, -29482, -1452 }, { -1092, -29474, 5484 }, { 22861, 1830, 17680 }, { 22716, 890, 17008 }, { 27673, -8945, 6433 }, { 19446, -1582, -22789 }, { -28175, 867, 10267 } },
    { { 2022, -27439, -11958 }, { 145, -8645, -28727 }, { -2382, -18589, 23426 }, { 1756, -29797, -3005 }, { -1499, -29848, -2614 }, { 3098, -29737, 2476 }, { -4133, -29710, -491 }, { -235, -29554, 5151 }, { 23537, 1743, 17795 }, { 23367, 848, 17064 }, { 28233, -8389, 6019 }, { 18043, -2177, -23868 }, { -27455, 244, 12091 } },
    { { 2311, -27044, -12777 }, { -66, -4599, -29645 }, { -2610, -18504, 23469 }, { 1931, -29768, -3184 }, { -1416, -29900, -1990 }, { 2953, -29743, 2574 }, { -3585, -29785, -152 }, { 93, -29583, 4984 }, { 23785, 1710, 17836 }, { 23605, 831, 17084 }, { 28423, -8181, 5883 }, { 17515, -2427, -24235 }, { -27157, -18, 12747 } },
    { { 2373, -27384, -12021 }, { -204, -1429, -29965 }, { -1964, -19497, 22716 }, { 1223, -29620, -4600 }, { -474, -29861, -2848 }, { 3886, -29691, 1829 }, { -4639, -29635, 494 }, { -1049, -29418, 5784 }, { 23531, 1744, 17794 }, { 23395, 848, 17063 }, { 28228, -8394, 6023 }, { 17845, -1221, -24085 }, { -27480, 1242, 11971 } },
    { { 2216, -27963, -10637 }, { -172, 633, -29993 }, { -1126, -21578, 20811 }, { 437, -28835, -8269 }, { 556, -29758, -3761 }, { 4795, -29586, 1304 }, { -5836, -29417, 749 }, { -2472, -29198, 6432 }, { 22876, 1828, 17683 }, { 22788, 889, 17009 }, { 27686, -8933, 6423 }, { 19032, -17, -23190 }, { -28140, 2499, 10093 } },
    { { 1928, -28640, -8718 }, { -72, 2216, -29918 }, { -598, -24208, 17709 }, { 97, -27233, -12584 }, { 706, -29726, -3982 }, { 4667, -29586, 1705 }, { -6041, -29386, -30 }, { -3005, -29198, 6199 }, { 21987, 1931, 17523 }, { 21905, 938, 16932 }, { 26841, -9636, 7080 }, { 20926, -17, -21497 }, { -28890, 2498, 7689 } },
    { { 1586, -29249, -6479 }, { -8, 3922, -29742 }, { -296, -26589, 13891 }, { 5, -24967, -16632 }, { 886, -29680, -4279 }, { 4491, -29586, 2121 }, { -6187, -29340, -956 }, { -3552, -29199, 5901 }, { 21029, 2026, 17342 }, { 20956, 984, 16844 }, { 25756, -10347, 7968 }, { 22792, -18, -19507 }, { -29466, 2498, 5051 } },
    { { 1261, -29696, -4066 }, { 23, 5680, -29457 }, { -123, -28407, 9644 }, { -10, -22639, -19685 }, { 1072, -29619, -4644 }, { 4295, -29586, 2491 }, { -6248, -29279, -1927 }, { -4031, -29199, 5582 }, { 20140, 2099, 17168 }, { 20078, 1019, 16760 }, { 24539, -10949, 9007 }, { 24352, -19, -17521 }, { -29785, 2497, 2575 } },
    { { 983, -29957, -1270 }, { 81, 6949, -29184 }, { -22, -29649, 4579 }, { -7, -21050, -21375 }, { 1271, -29543, -5058 }, { 4092, -29587, 2809 }, { -6222, -29204, -2904 }, { -4438, -29200, 5261 }, { 19337, 2152, 17005 }, { 19286, 1045, 16682 }, { 23227, -11428, 10159 }, { 25611, -21, -15624 }, { -29894, 2496, 330 } },
    { { 745, -29912, 2170 }, { 158, 7575, -29028 }, { 30, -29996, -470 }, { 2, -20604, -21805 }, { 1496, -29454, -5497 }, { 3887, -29587, 3084 }, { -6114, -29115, -3864 }, { -4785, -29200, 4944 }, { 18607, 2190, 16854 }, { 18568, 1062, 16609 }, { 21836, -11794, 11409 }, { 26626, -22, -13822 }, { -29847, 2495, -1707 } },
    { { 531, -29275, 6532 }, { 258, 7175, -29128 }, { 47, -29675, -4403 }, { 10, -21015, -21410 }, { 1754, -29355, -5934 }, { 3682, -29587, 3322 }, { -5928, -29017, -4785 }, { -5082, -29201, 4633 }, { 17937, 2215, 16714 }, { 17909, 1075, 16542 }, { 20387, -12058, 12727 }, { 27446, -24, -12113 }, { -29683, 2493, -3566 } },
    { { 317, -27469, 12057 }, { 404, 5000, -29578 }, { 42, -29447, -5735 }, { 11, -21307, -21119 }, { 2045, -29250, -6343 }, { 3478, -29588, 3531 }, { -5673, -28913, -5645 }, { -5340, -29202, 4329 }, { 17315, 2230, 16582 }, { 17298, 1082, 16479 }, { 18911, -12230, 14083 }, { 28109, -26, -10484 }, { -29427, 2492, -5278 } },
    { { 54, -23656, 18450 }, { 576, -1757, -29943 }, { 21, -29007, -7653 }, { 5, -21124, -21303 }, { 2372, -29147, -6697 }, { 3273, -29588, 3719 }, { -5354, -28810, -6429 }, { -5568, -29202, 4026 }, { 16722, 2238, 16456 }, { 16716, 1085, 16419 }, { 17419, -12319, 15462 }, { 28650, -28, -8897 }, { -29091, 2490, -6894 } },
    { { -141, -19204, 23047 }, { 591, -12131, -27432 }, { -17, -28261, -10065 }, { -10, -20060, -22307 }, { 2757, -29051, -6958 }, { 3051, -29588, 3901 }, { -4954, -28715, -7134 }, { -5787, -29203, 3700 }, { 16110, 2238, 16326 }, { 16114, 1086, 16356 }, { 15839, -12330, 16924 }, { 29116, -30, -7230 }, { -28650, 2489, -8544 } },
    { { -173, -16991, 24724 }, { 392, -21031, -21390 }, { -82, -27418, -12176 }, { -37, -18185, -23860 }, { 3224, -28973, -7084 }, { 2791, -29589, 4089 }, { -4445, -28638, -7755 }, { -6011, -29204, 3319 }, { 15421, 2230, 16179 }, { 15437, 1082, 16286 }, { 14094, -12243, 18537 }, { 29523, -31, -5331 }, { -28042, 2487, -10367 } },
    { { -105, -17355, 24470 }, { 141, -26602, -13868 }, { -197, -27025, -13024 }, { -68, -15134, -25903 }, { 3790, -28920, -7019 }, { 2467, -29589, 4292 }, { -3795, -28585, -8276 }, { -6247, -29204, 2848 }, { 14600, 2207, 16006 }, { 14630, 1071, 16203 }, { 12154, -12012, 20318 }, { 29845, -32, -3041 }, { -27164, 2486, -12488 } },
    { { 16, -18921, 23281 }, { -5, -29191, -6919 }, { -418, -27526, -11922 }, { -77, -10671, -28038 }, { 4454, -28901, -6701 }, { 2050, -29589, 4505 }, { -2974, -28566, -8668 }, { -6488, -29204, 2245 }, { 13592, 2161, 15796 }, { 13638, 1049, 16102 }, { 10086, -11565, 22191 }, { 29999, -33, -207 }, { -25863, 2486, -14997 } },
    { { 277, -19861, 22482 }, { -138, -29872, -2766 }, { -776, -28351, -9779 }, { 37, -5264, -29535 }, { 4575, -29129, -5531 }, { 1155, -29627, 4575 }, { -1933, -28828, -8074 }, { -6334, -29277, 1647 }, { 12387, 2081, 15553 }, { 12437, 1010, 15985 }, { 8092, -10855, 23946 }, { 29816, -405, 3294 }, { -24039, 2097, -17825 } },
    { { 793, -19606, 22693 }, { -481, -29948, -1704 }, { -1258, -28863, -8084 }, { 331, 114, -29998 }, { 3638, -29538, -3776 }, { -148, -29692, 4286 }, { -1110, -29313, -6285 }, { -5713, -29420, 1341 }, { 11188, 1974, 15321 }, { 11221, 959, 15873 }, { 6546, -10016, 25249 }, { 29179, -1227, 6861 }, { -21938, 1238, -20426 } },
    { { 1431, -18818, 23320 }, { -977, -29901, -2231 }, { -1766, -29139, -6915 }, { 719, 4590, -29638 }, { 2268, -29818, -2402 }, { -1262, -29733, 3791 }, { -801, -29668, -4380 }, { -5074, -29538, 1325 }, { 10267, 1873, 15152 }, { 10277, 910, 15791 }, { 5588, -9309, 26016 }, { 28356, -2051, 9577 }, { -20119, 375, -22251 } },
    { { 1742, -18436, 23603 }, { -1229, -29857, -2656 }, { -2053, -29321, -6004 }, { 978, 7376, -29063 }, { 1556, -29900, -1883 }, { -1722, -29743, 3519 }, { -781, -29785, -3502 }, { -4790, -29583, 1380 }, { 9898, 1829, 15087 }, { 9897, 889, 15760 }, { 5249, -9015, 26277 }, { 27942, -2427, 10646 }, { -19342, -18, -22932 } },
    { { 1594, -18729, 23381 }, { -1078, -29853, -2766 }, { -2049, -29507, -5012 }, { 1029, 8732, -28683 }, { 2602, -29858, -1304 }, { -620, -29691, 4250 }, { -1807, -29633, -4316 }, { -5839, -29418, 679 }, { 9994, 1841, 15103 }, { 10036, 895, 15768 }, { 5335, -9092, 26211 }, { 28234, -1221, 10066 }, { -19449, 1242, -22808 } },
    { { 1265, -19321, 22915 }, { -764, -29794, -3425 }, { -1906, -29706, -3731 }, { 1004, 9539, -28425 }, { 3748, -29749, -974 }, { 584, -29586, 4935 }, { -2824, -29408, -5213 }, { -6889, -29198, 114 }, { 10266, 1873, 15152 }, { 10351, 910, 15791 }, { 5588, -9309, 26016 }, { 28626, -17, 8976 }, { -19890, 2499, -22319 } },
    { { 889, -20170, 22190 }, { -440, -29622, -4729 }, { -1656, -29889, -1976 }, { 920, 9875, -28313 }, { 3923, -29705, -1494 }, { 782, -29586, 4906 }, { -2813, -29364, -5461 }, { -6877, -29198, 395 }, { 10694, 1922, 15229 }, { 10775, 934, 15829 }, { 6012, -9643, 25681 }, { 28964, -18, 7815 }, { -20786, 2498, -21487 } },
    { { 560, -21265, 21154 }, { -203, -29235, -6727 }, { -1334, -29967, 432 }, { 800, 9747, -28361 }, { 4084, -29641, -2176 }, { 1040, -29586, 4856 }, { -2848, -29301, -5775 }, { -6844, -29199, 762 }, { 11257, 1981, 15334 }, { 11333, 962, 15879 }, { 6626, -10067, 25184 }, { 29337, -19, 6271 }, { -21909, 2497, -20341 } },
    { { 306, -22631, 19691 }, { -69, -28496, -9380 }, { -981, -29758, 3675 }, { 672, 9029, -28601 }, { 4178, -29557, -2990 }, { 1347, -29586, 4778 }, { -2960, -29217, -6132 }, { -6778, -29200, 1201 }, { 11937, 2044, 15464 }, { 12006, 992, 15942 }, { 7466, -10553, 24481 }, { 29678, -20, 4386 }, { -23179, 2496, -18881 } },
    { { 126, -24294, 17600 }, { -11, -27253, -12542 }, { -641, -28922, 7944 }, { 567, 7404, -29066 }, { 4158, -29454, -3894 }, { 1691, -29587, 4664 }, { -3178, -29115, -6497 }, { -6667, -29200, 1699 }, { 12719, 2106, 15619 }, { 12778, 1022, 16017 }, { 8592, -11067, 23512 }, { 29919, -22, 2201 }, { -24515, 2495, -17111 } },
};
// clang-format on
// The sprint (holding A): Mixamo's "Running" (21 frames to the cycle, its right foot down at frame 17, its left at
// 6 to 7). The legs and arms as the other clips (bone directions); the pelvis, chest and head turned as the runner's
// own are in the world (each built from the runner's hips, shoulders, neck and head, so his chest leans 11-15 degrees
// into the run and his shoulders turn about 20 degrees either way with his arms), not as turns from Ganondorf's idle
// stance (which twisted his whole torso 30 degrees off to one side). Checked against the source frame by frame: the
// arms clear his torso and hips by more than an arm's thickness all through the cycle, and his feet keep their own
// angle on his shins (the clip's foot turns tilted his toes up through every step).
// clang-format off
static const Vec3s sGdfClipSprint[21][13] = {
    { { 4280, -21690, 20279 }, { -2449, -4157, -29609 }, { 1650, -29858, -2399 }, { 1806, -22172, -20128 }, { 16706, -6893, -23945 }, { 9683, -28263, 2725 }, { -16583, -23762, 7771 }, { 6140, 3164, 29194 }, { -15391, 4818, -16359 }, { 18006, -2290, 16438 }, { 16663, -14356, 16157 }, { 28316, 9403, 3131 }, { -29615, 3438, -3332 } },
    { { 3025, -17758, 23990 }, { 921, -18401, -23676 }, { 2131, -27838, -10979 }, { 1386, -22803, -19445 }, { 14883, -2470, -25930 }, { 10180, -28184, -1416 }, { -19561, -18371, 13412 }, { 12509, 7497, 26217 }, { -15069, 4991, -15984 }, { 18901, -2278, 16265 }, { 16034, -14666, 16719 }, { 28620, 8867, 1505 }, { -27825, 11189, 746 } },
    { { 2091, -17392, 24354 }, { 2899, -26523, -13715 }, { 2906, -26678, -13410 }, { 1316, -18849, -23302 }, { 12824, -1292, -27090 }, { 10461, -27303, -6717 }, { -21417, -14155, 15523 }, { 16279, 2931, 25028 }, { -15176, 5166, -15823 }, { 19543, -2316, 16129 }, { 15595, -14773, 17039 }, { 28872, 8143, 350 }, { -23464, 18133, 4545 } },
    { { 1885, -19083, 23071 }, { 2484, -29771, -2741 }, { 2388, -26564, -13735 }, { 1373, -11127, -27826 }, { 10102, -2873, -28102 }, { 9796, -26300, -10599 }, { -21969, -13961, 14915 }, { 16782, -2646, 24726 }, { -15433, 5183, -16062 }, { 20035, -2278, 16067 }, { 16102, -14661, 16541 }, { 28621, 8783, -1926 }, { -21668, 19462, 7191 } },
    { { 1849, -21227, 21119 }, { 559, -29504, 5406 }, { 2263, -27396, -12013 }, { 782, -1074, -29971 }, { 7488, -4216, -28743 }, { 9255, -27123, -8870 }, { -21667, -16355, 12769 }, { 16209, -5969, 24528 }, { -15657, 4961, -16214 }, { 20235, -2128, 16111 }, { 16468, -14554, 16320 }, { 27491, 10670, -5514 }, { -22471, 17660, 9119 } },
    { { 1375, -22913, 19316 }, { -1425, -28837, 8150 }, { 1547, -28629, -8833 }, { 344, 8299, -28827 }, { 6820, -7208, -28311 }, { 8720, -28575, -2722 }, { -21763, -19157, 7704 }, { 15069, -7521, 24827 }, { -16059, 4567, -16251 }, { 20002, -1994, 16259 }, { 16412, -14472, 16519 }, { 25827, 10332, -11234 }, { -23229, 16615, 9184 } },
    { { 370, -23872, 18166 }, { -2902, -29696, 3117 }, { -600, -29655, -4500 }, { 1496, 15291, -25767 }, { 8447, -13122, -25621 }, { 7596, -28250, 6653 }, { -21197, -21212, 850 }, { 11045, -10904, 25673 }, { -16533, 4289, -16119 }, { 19302, -1905, 16331 }, { 16045, -14254, 16829 }, { 25694, 8979, -12616 }, { -23020, 17073, 8865 } },
    { { 82, -24789, 16897 }, { -3954, -29249, -5369 }, { -2004, -29879, 1793 }, { 2652, 18180, -23716 }, { 12036, -19354, -19508 }, { 4905, -23542, 17936 }, { -18337, -22510, -7555 }, { 4204, -16236, 24874 }, { -16859, 4323, -15817 }, { 18276, -2029, 16376 }, { 15061, -13981, 17753 }, { 28615, 6727, -5994 }, { -22935, 17234, 8774 } },
    { { 880, -26442, 14144 }, { -4742, -26886, -12435 }, { -2831, -26878, 13022 }, { 2804, 14176, -26290 }, { 15813, -23652, -9515 }, { -802, -14362, 26326 }, { -15323, -19971, -16321 }, { -4003, -21556, 20477 }, { -17114, 4573, -15497 }, { 17090, -2377, 16420 }, { 14057, -13970, 18798 }, { 29025, 5988, 4660 }, { -23047, 16354, 10070 } },
    { { 2499, -28844, 7859 }, { -5946, -23755, -17330 }, { -1735, -21689, 20654 }, { 587, 5085, -29560 }, { 17073, -24515, 2739 }, { -4835, -4908, 29198 }, { -14182, -14062, -22386 }, { -8667, -25065, 14023 }, { -17379, 4868, -15542 }, { 15867, -2560, 16359 }, { 13345, -14348, 19339 }, { 27753, 7764, 8335 }, { -24196, 15673, 8300 } },
    { { 2645, -29869, -913 }, { -5846, -21710, -19862 }, { -568, -18523, 23592 }, { -855, -5890, -29404 }, { 17276, -20117, 14031 }, { -7089, 3694, 28915 }, { -13809, -5628, -26031 }, { -10686, -27061, 7313 }, { -17727, 5142, -16130 }, { 14559, -2607, 16188 }, { 13380, -14765, 18910 }, { 28025, 9117, 5611 }, { -22315, 18564, 7578 } },
    { { 1664, -28117, -10328 }, { -5414, -21514, -20195 }, { 180, -15923, 25425 }, { -2272, -16715, -24808 }, { 16448, -14028, 20801 }, { -11146, 9805, 26070 }, { -12511, 2332, -27167 }, { -11410, -27743, 339 }, { -18081, 5396, -16695 }, { 13419, -2458, 16174 }, { 12667, -14957, 19455 }, { 26434, 10128, 9934 }, { -22839, 18435, 6207 } },
    { { 225, -26046, -14885 }, { -4998, -19190, -22511 }, { 470, -15252, 25829 }, { -3520, -25109, -16035 }, { 16873, -10356, 22540 }, { -15839, 10475, 23225 }, { -9887, 6741, -27510 }, { -11873, -26908, -5915 }, { -18257, 5666, -16960 }, { 12768, -2471, 16252 }, { 11344, -15067, 20845 }, { 25286, 11261, 11567 }, { -24041, 17759, 2573 } },
    { { -460, -25018, -16550 }, { -5377, -12741, -26622 }, { 713, -17555, 24317 }, { -3695, -29248, -5557 }, { 18247, -9863, 21675 }, { -18804, 6415, 22478 }, { -6619, 4826, -28860 }, { -12916, -25433, -9292 }, { -18070, 5731, -16867 }, { 12612, -2446, 16412 }, { 10671, -14953, 21485 }, { 24669, 11982, 12160 }, { -24049, 17933, -286 } },
    { { 9, -26124, -14749 }, { -5365, -1941, -29453 }, { 711, -20684, 21717 }, { -1222, -29791, 3317 }, { 19420, -12366, 19234 }, { -18509, 1387, 23569 }, { -4420, 1777, -29619 }, { -13236, -25347, -9073 }, { -17741, 5568, -16614 }, { 12623, -2290, 16571 }, { 10785, -14707, 21240 }, { 23879, 12146, 13502 }, { -23792, 18202, -1623 } },
    { { 508, -27347, -12325 }, { -5859, 8205, -28255 }, { 1832, -23427, 18650 }, { 305, -28160, 10341 }, { 21676, -15133, 14183 }, { -14952, -6181, 25264 }, { -4157, -726, -29702 }, { -13834, -26288, -4190 }, { -17704, 5320, -16570 }, { 12829, -2167, 16654 }, { 11594, -14321, 20371 }, { 22955, 11772, 15313 }, { -24248, 17624, -1195 } },
    { { 923, -28175, -10261 }, { -5216, 15622, -25075 }, { 2026, -24697, 16910 }, { 564, -27715, 11470 }, { 23212, -17192, 8103 }, { -9126, -14125, 24843 }, { -6366, -5899, -28717 }, { -13868, -26522, 2062 }, { -17719, 5148, -16492 }, { 13365, -2068, 16698 }, { 12465, -14116, 19489 }, { 23668, 8982, 16098 }, { -22807, 19422, -1612 } },
    { { 2249, -29010, -7304 }, { -5805, 20492, -21128 }, { 1930, -26214, 14461 }, { 1341, -29222, 6656 }, { 23911, -18077, 1206 }, { -2260, -20194, 22070 }, { -9664, -11658, -25898 }, { -12387, -25943, 8576 }, { -17445, 5092, -16359 }, { 14188, -2158, 16675 }, { 13465, -14215, 18538 }, { 25841, 6667, 13704 }, { -22274, 19636, -4276 } },
    { { 5189, -29354, -3380 }, { -8002, 22365, -18325 }, { 2066, -25653, 15416 }, { 1361, -29291, -6338 }, { 23651, -17034, -7106 }, { 2885, -23555, 18354 }, { -13072, -17764, -20336 }, { -9723, -23342, 16144 }, { -16864, 5176, -16334 }, { 15041, -2296, 16637 }, { 14913, -14234, 17192 }, { 27300, 6779, 10430 }, { -25106, 14857, -6997 } },
    { { 6211, -28867, 5301 }, { -7675, 19261, -21682 }, { 2106, -27054, 12792 }, { 1194, -26565, -13887 }, { 21078, -15618, -14553 }, { 6885, -26225, 12840 }, { -14613, -23617, -11345 }, { -6408, -17214, 23720 }, { -16212, 5215, -16429 }, { 15897, -2405, 16613 }, { 16317, -14117, 15975 }, { 27659, 8471, 7952 }, { -28510, 5761, -7349 } },
    { { 5798, -25766, 14229 }, { -5473, 10708, -27484 }, { 1524, -29174, 6822 }, { 1817, -23518, -18536 }, { 18308, -12145, -20428 }, { 8640, -27832, 7122 }, { -15197, -25844, -1073 }, { -1187, -7759, 28955 }, { -15840, 4908, -16557 }, { 16950, -2331, 16499 }, { 17043, -14078, 15512 }, { 28241, 8893, 4835 }, { -29172, 1459, -6846 } },
};
// clang-format on
// Ground covered by one cycle of it on his legs: his stance foot travels 9.18 a frame back under him (both feet alike)
// over 21 frames. His cadence follows his speed from it, so his feet don't slide.
#define GDF_SPRINT_STRIDE 192.7f

// Ground covered by one cycle of each clip on his legs (his stance foot's travel under him, measured from the clips),
// as the speed at which a cycle takes the walk's 30 frames: his cadence follows from it, so his feet don't slide
#define GDF_MWALK_NATURAL_WALK 3.12f
#define GDF_MWALK_NATURAL_BACK 2.38f
#define GDF_MWALK_NATURAL_STRUT 2.67f
#define GDF_TRIDENT_RUN_NATURAL 5.29f

#define GDF_MWALK_FULL_SPEED 3.5f // up to this speed, the Mixamo walk alone
#define GDF_MWALK_GONE_SPEED 5.0f // from this one, the stick run alone

// Walking backwards (Z-targeting), strutting, or the plain walk
static s32 BossGanon_Player_MixamoWalkBack(PlayState* play, f32 speed) {
    Player* player = GET_PLAYER(play);
    s16 heading = player->actor.world.rot.y - player->actor.shape.rot.y;
    s32 targeting =
        (player->focusActor != NULL) || (player->stateFlags1 & (PLAYER_STATE1_Z_TARGETING | PLAYER_STATE1_PARALLEL));

    return !sGdfRoaming && targeting && (speed > 0.5f) && ((player->speedXZ < -0.5f) || (ABS(heading) > 0x5000));
}

// The speed his cadence is set by: the clip he's walking in (blended into the stick run's as it takes over)
static f32 sGdfWalkSpeed = 0.0f; // how fast he's walking (this frame's, from UpdateWalk)

// With the trident: how much of his walk is the plain walk (the rest the trident run), by speed, as the plain walk
// gives way to the stick run
static f32 BossGanon_Player_TridentWalkAmount(f32 speed) {
    // Play as Ganondorf (2S2H): not ported yet
    return 0;
}

// In his trident stance (drawn, with a trident chosen): its own walk and run. Out of it (a trident only chosen as his
// weapon), the plain walk and the sprint's run.
static u8 sPlayerSwordState; // (defined below)
static s32 BossGanon_Player_InTridentStance(void) {
    // Play as Ganondorf (2S2H): not ported yet
    return 0;
}

static f32 BossGanon_Player_WalkNaturalSpeed(BossGanon* this, PlayState* play, f32 speed) {
    s32 back = BossGanon_Player_MixamoWalkBack(play, speed);
    f32 clip =
        back ? GDF_MWALK_NATURAL_BACK
             : (CVarGetInteger(CVAR_PLAY_AS_GDF("StrutWalk"), 0) ? GDF_MWALK_NATURAL_STRUT : GDF_MWALK_NATURAL_WALK);
    f32 pace =
        back ? 1.0f
             : 1.0f - CLAMP((speed - GDF_MWALK_FULL_SPEED) / (GDF_MWALK_GONE_SPEED - GDF_MWALK_FULL_SPEED), 0.0f, 1.0f);

    if (BossGanon_Player_InTridentStance() && this->playerGrounded) {
        // The trident walk's cadence into the trident run's
        return GDF_TRIDENT_RUN_NATURAL +
               (GDF_MWALK_NATURAL_WALK - GDF_TRIDENT_RUN_NATURAL) * BossGanon_Player_TridentWalkAmount(speed);
    }
    return GDF_WALK_NATURAL_SPEED + (clip - GDF_WALK_NATURAL_SPEED) * pace;
}

static void BossGanon_Player_ApplyMixamoWalk(BossGanon* this, PlayState* play, f32 speed) {
    s32 back = BossGanon_Player_MixamoWalkBack(play, speed);
    s32 strut = CVarGetInteger(CVAR_PLAY_AS_GDF("StrutWalk"), 0);
    f32 phase = (this->playerWalkFrame - 1.0f) / GDF_WALK_CYCLE_FRAMES; // (his right heel lands at walk frame 1)
    f32 pace = 1.0f - CLAMP((speed - GDF_MWALK_FULL_SPEED) / (GDF_MWALK_GONE_SPEED - GDF_MWALK_FULL_SPEED), 0.0f, 1.0f);
    f32 w;

    phase -= floorf(phase);
    // Running (the stick pushed far): the sprint's own run (Mixamo's "Running", as when holding A), taking over from
    // the walk as he speeds up (its right heel comes down at its frame 17). With the trident, its own run does this.
    if (!back && (pace < 1.0f) && !BossGanon_Player_InTridentStance()) {
        // (in full under the walk, which blends over it: no stick run shows through)
        f32 rw = this->playerWalkWeight * (1.0f - this->playerSprintWeight);

        if (rw > 0.0f) {
            BossGanon_Player_ApplyClip(this, sGdfClipSprint, ARRAY_COUNT(sGdfClipSprint),
                                       17.0f + phase * ARRAY_COUNT(sGdfClipSprint), true, rw, 1.0f);
        }
    }
    w = this->playerWalkWeight * (1.0f - this->playerSprintWeight) * (back ? 1.0f : pace);
    if (w <= 0.0f) {
        return;
    }
    // Each clip from where its right heel lands
    if (back) {
        BossGanon_Player_ApplyClip(this, sGdfClipWalkBack, ARRAY_COUNT(sGdfClipWalkBack),
                                   25.0f + phase * ARRAY_COUNT(sGdfClipWalkBack), true, w, 1.0f);
    } else if (strut) {
        // (the "Dr. Livesey" strut: its right heel lands at its frame 6; and its song while he struts at walking pace)
        BossGanon_Player_ApplyClip(this, sGdfClipStrut, ARRAY_COUNT(sGdfClipStrut),
                                   6.0f + phase * ARRAY_COUNT(sGdfClipStrut), true, w, 1.0f);
        if ((w > 0.5f) && this->playerGrounded) {
            GanondorfStrut_Strutting();
        }
    } else {
        BossGanon_Player_ApplyClip(this, sGdfClipWalk, ARRAY_COUNT(sGdfClipWalk),
                                   31.0f + phase * ARRAY_COUNT(sGdfClipWalk), true, w, 1.0f);
    }
}

// His feet on the clips (walk, run): the clips swing his legs by their bones' directions, and his feet ride along on
// his shins, so a shin's twist turned a foot in (the right one toward his left). Each foot turned about the vertical
// so its toes (the foot's local +z) point ahead, splayed a little outward, as the old walk did; its pitch kept.
static void BossGanon_Player_FeetAhead(BossGanon* this, f32 weight) {
    static const s8 sLegChains[2][5] = { { 0, 18, 19, 20, 21 }, { 0, 18, 22, 23, 24 } };
    Vec3s* joints = this->skelAnime.jointTable;
    s32 leg;
    s32 k;

    if (weight <= 0.0f) {
        return;
    }
    for (leg = 0; leg < 2; leg++) {
        f32 parent[3][3];
        f32 m[3][3];
        f32 turn[3][3];
        Vec3s turnRot = { 0, 0, 0 };
        s16 target = (leg == 0) ? GDF_WALK_FOOT_SPLAY : -GDF_WALK_FOOT_SPLAY;
        s16 toeYaw;

        BossGanon_Player_RotToMtx(&joints[1 + sLegChains[leg][0]], parent);
        for (k = 1; k < 4; k++) {
            BossGanon_Player_RotToMtx(&joints[1 + sLegChains[leg][k]], m);
            BossGanon_Player_Mul3(parent, m, parent, false);
        }
        BossGanon_Player_RotToMtx(&joints[1 + sLegChains[leg][4]], m);
        BossGanon_Player_Mul3(parent, m, m, false); // the foot in actor space
        toeYaw = Math_Atan2S(m[2][2], m[0][2]);
        turnRot.y = (s16)((s16)(target - toeYaw) * weight);
        BossGanon_Player_RotToMtx(&turnRot, turn);
        BossGanon_Player_Mul3(turn, m, m, false);
        BossGanon_Player_Mul3(parent, m, m, true); // back into the shin's space
        BossGanon_Player_MtxToRot(m, &joints[1 + sLegChains[leg][4]]);
    }
}

static void BossGanon_Player_UpdateWalk(BossGanon* this, PlayState* play) {
    Player* player = GET_PLAYER(play);
    // Walking about on his own in a cutscene: his own speed, not Link's
    f32 speed = sGdfRoaming ? sGdfRoamSpeed : fabsf(player->speedXZ);
    f32 rate;
    u8 inCs =
        !sGdfRoaming && ((player->stateFlags1 & PLAYER_STATE1_IN_CUTSCENE) || (play->csCtx.state != CS_STATE_IDLE));
    u8 moving;

    // In cutscenes the scene often moves Link itself (up the steps to the Master Sword, say) with no speed of his
    // own: he walks as far as Link went (a jump of more than a stride is a cut, not a step)
    if (inCs) {
        f32 moved = Math_Vec3f_DistXZ(&player->actor.world.pos, &player->actor.prevPos);

        if (moved < 15.0f) {
            speed = MAX(speed, moved);
        }
    }
    moving = (speed > 0.5f) && (sGdfRoaming || inCs || (player->actor.bgCheckFlags & BGCHECKFLAG_GROUND));
    sGdfWalkSpeed = speed;

    Math_ApproachF(&this->playerWalkWeight, moving ? 1.0f : 0.0f, 1.0f, 0.15f);
    // Sprinting (holding A): a faster cycle, longer strides and a forward lean
    Math_ApproachF(&this->playerSprintWeight, (moving && PlayAsGanondorf_IsSprinting()) ? 1.0f : 0.0f, 1.0f, 0.1f);

    rate = CLAMP(speed / BossGanon_Player_WalkNaturalSpeed(this, play, speed), 0.5f,
                 GDF_WALK_MAX_RATE + this->playerSprintWeight);
    // Sprinting, or running on the stick (the same run): as many strides as his speed covers (the run's own stride:
    // no sliding feet)
    {
        f32 runAmt = MAX(this->playerSprintWeight,
                         (BossGanon_Player_InTridentStance() || BossGanon_Player_MixamoWalkBack(play, speed))
                             ? 0.0f
                             : 1.0f - BossGanon_Player_TridentWalkAmount(speed));

        if (runAmt > 0.0f) {
            f32 sprintRate = MAX(speed * GDF_WALK_CYCLE_FRAMES / GDF_SPRINT_STRIDE, 0.5f);

            rate += (sprintRate - rate) * runAmt;
        }
    }
    this->playerWalkStride = speed / (GDF_WALK_NATURAL_SPEED * GDF_WALK_MAX_RATE);
    if (moving || (this->playerWalkWeight > 0.0f)) {
        f32 prev = this->playerWalkFrame;

        this->playerWalkFrame += rate;
        while (this->playerWalkFrame >= GDF_WALK_CYCLE_FRAMES) {
            this->playerWalkFrame -= GDF_WALK_CYCLE_FRAMES;
        }

        // His footsteps, on the frames his heels come down (right foot at 1, left at 16), with the floor's sound
        if (moving && !this->playerHidden) {
            static const f32 sContacts[] = { 1.0f, 16.0f };
            s32 i;

            for (i = 0; i < ARRAY_COUNT(sContacts); i++) {
                f32 c = sContacts[i];
                u8 crossed = (prev < this->playerWalkFrame) ? ((prev < c) && (this->playerWalkFrame >= c))
                                                            : ((prev < c) || (this->playerWalkFrame >= c));

                if (crossed) {
                    u16 sfx = NA_SE_PL_WALK_GROUND + player->floorSfxOffset;

                    Actor_PlaySfx(&player->actor, sfx);
                    player->stateFlags2 |= PLAYER_STATE2_FOOTSTEP;
                }
            }
        }
    }

    BossGanon_Player_ApplyWalk(this);
    BossGanon_Player_ApplyMixamoWalk(this, play, speed);
    // Running (the stick pushed far): his torso leans into it, more the faster he goes (up to 11 degrees). Upright,
    // with the run's long stride reaching out ahead, he looked like he was leaning back. (The sprint adds its own.)
    {
        f32 run = CLAMP((this->playerWalkStride - 1.0f) / (GDF_WALK_MAX_STRIDE - 1.0f), 0.0f, 1.0f);

        run *= this->playerWalkWeight * (1.0f - this->playerSprintWeight);
        // (the run clip leans on its own: only the trident's walk-run and backing off need this)
        if (!BossGanon_Player_InTridentStance() && !BossGanon_Player_MixamoWalkBack(play, speed)) {
            run *= BossGanon_Player_TridentWalkAmount(speed); // (fading as the run clip takes over)
        }
        if (run > 0.0f) {
            BossGanon_Player_TwistLean(this, 0.0f, 0x0800 * run);
        }
    }
    if (this->playerSprintWeight > 0.0f) {
        f32 runWeight = this->playerSprintWeight * this->playerWalkWeight;
        // Shoulders turn against the hips with each stride (the right shoulder comes forward with the right arm)
        f32 twist = -0x0500 * Math_CosS((s16)(this->playerWalkFrame * (0x10000 / GDF_WALK_CYCLE_FRAMES)));

        f32 phase = (this->playerWalkFrame - 1.0f) / GDF_WALK_CYCLE_FRAMES; // (the right heel at walk frame 1)

        (void)twist;
        phase -= floorf(phase);
        // Mixamo's "Running", all of it (its right foot comes down at its frame 17, his at walk frame 1): legs, both
        // arms swinging, and the runner's lean and turn of the shoulders
        BossGanon_Player_ApplyClip(this, sGdfClipSprint, ARRAY_COUNT(sGdfClipSprint),
                                   17.0f + phase * ARRAY_COUNT(sGdfClipSprint), true, runWeight, 1.0f);
    }
    BossGanon_Player_FeetAhead(this, this->playerWalkWeight);
    sGdfCapeRunLift = this->playerWalkWeight *
                      MAX(this->playerSprintWeight,
                          (BossGanon_Player_InTridentStance() || BossGanon_Player_MixamoWalkBack(play, speed))
                              ? 0.0f
                              : 1.0f - BossGanon_Player_TridentWalkAmount(speed));
}

// Keeps his lowest ankle or knee at the floor (from the last draw), so any animation stands on the ground. Knees
// count too: kneeling (the heavy landing) puts a knee lower than either foot, and his knee pad is about as thick as
// his sole.
// His lowest knee or ankle above his origin, from this frame's pose (not the last draw, a frame behind: after a fall
// that put him a whole fall's step too low on landing, and while the landing's pose changed fast it was always off)
static f32 BossGanon_Player_PoseLowest(BossGanon* this) {
    static const s8 sChains[2][5] = { { 0, 18, 19, 20, 21 }, { 0, 18, 22, 23, 24 } };
    Vec3s* joints = this->skelAnime.jointTable;
    f32 lowest = FLT_MAX;
    s32 leg;
    s32 k;

    for (leg = 0; leg < 2; leg++) {
        f32 rot[3][3];
        f32 local[3][3];
        f32 pos[3] = { joints[0].x, joints[0].y, joints[0].z };

        BossGanon_Player_RotToMtx(&joints[1 + 0], rot);
        for (k = 1; k < 5; k++) {
            s32 limb = sChains[leg][k];
            StandardLimb* l = SEGMENTED_TO_VIRTUAL(this->skelAnime.skeleton[limb]);
            f32 jp[3] = { l->jointPos.x, l->jointPos.y, l->jointPos.z };
            s32 r;

            for (r = 0; r < 3; r++) {
                pos[r] += rot[r][0] * jp[0] + rot[r][1] * jp[1] + rot[r][2] * jp[2];
            }
            if (k >= 3) { // the knee (the shin's origin) and the ankle (the foot's)
                lowest = MIN(lowest, pos[1]);
            }
            {
                // As drawn: the leg bends the limb draw adds (OverrideLimbDraw's legRot, the vanilla boss's floating
                // legs; still bent after some landings, which left him floating above the floor)
                Vec3s r = joints[1 + limb];
                s32 side = (limb >= 22) ? -1 : 1;
                s32 seg = (limb - 19) % 3; // thigh, shin, foot

                if ((limb >= 19) && (seg != 1)) {
                    r.y += (s16)(this->legRot.x + side * this->legRot.z);
                    r.z += (s16)this->legRot.y;
                } else if ((limb >= 19) && (this->legRot.y > 0.0f)) {
                    r.z += (s16)this->legRot.y;
                }
                BossGanon_Player_RotToMtx(&r, local);
            }
            BossGanon_Player_Mul3(rot, local, rot, false);
        }
    }
    return (lowest + this->actor.shape.yOffset) * this->actor.scale.y;
}

static void BossGanon_Player_UpdateGroundOffset(BossGanon* this, PlayState* play) {
    Player* player = GET_PLAYER(play);
    f32 ankleAboveRoot = BossGanon_Player_PoseLowest(this);
    f32 target = GDF_ANKLE_HEIGHT - ankleAboveRoot + (this->playerFloorY - player->actor.world.pos.y);
    s32 fast = (this->actionFunc == BossGanon_Player_Land) || (sGdfFastGroundTimer > 0);

    // Better: the lowest point of the model itself (toes, heels, knees, knuckles), kept just above the floor. The
    // joints alone let tucked toes and a planted hand go through the floor when kneeling.
    if (sGdfMeshLowValid && !fast) { // (the mesh is from the last draw: not while the pose changes fast)
        f32 meshAboveRoot = sGdfMeshLowY - this->actor.world.pos.y;

        if ((meshAboveRoot > -300.0f) && (meshAboveRoot < 100.0f)) {
            target = MAX(target, GDF_SOLE_CLEARANCE - meshAboveRoot + (this->playerFloorY - player->actor.world.pos.y));
        }
    }

    if (this->playerHidden || (ankleAboveRoot < -300.0f) || (ankleAboveRoot > 100.0f)) {
        return; // not drawn (stale), or not a sensible pose
    }

    // (on the ground, as high as a pose needs: the landing's root sits far lower than his idle's, and capped at 60 he
    // sank into the floor until he stood)
    target = CLAMP(target, -20.0f, BossGanon_Player_GetHoverHeight() + (this->playerGrounded ? 200.0f : 60.0f));
    // The landing drops him onto one knee in two frames: follow it at once, or he sinks into the floor first. Same
    // while he blends from getting up into his standing idle afterwards: the get-up ends with his root far lower than
    // the idle's, and a slow follow let the blend lift him off the floor for a moment.
    if (sGdfFastGroundTimer > 0) {
        sGdfFastGroundTimer--;
    }
    if ((this->actionFunc == BossGanon_Player_Land) && (this->unk_1C2 < 2)) {
        Math_ApproachF(&this->playerGroundOffset, target, 1.0f, 40.0f);
    } else if ((this->actionFunc == BossGanon_Player_Land) || (sGdfFastGroundTimer > 0)) {
        // Getting up (and blending into his idle after): follow the pose at once both ways, so his feet never sink
        // and he never rises slowly out of the floor afterwards (this frame's pose, so it no longer overshoots)
        Math_ApproachF(&this->playerGroundOffset, target, 1.0f, 60.0f);
    } else {
        Math_ApproachF(&this->playerGroundOffset, target, 0.5f, 6.0f);
        // Settled on his feet, standing still in his normal idle: remember the height, the ceiling for the next
        // get-up (not while running or in a stance: bent knees there put it too low, and he sank into the ground on
        // landing)
        if (this->playerGrounded && (this->actionFunc == BossGanon_Player_Float) && (this->playerWalkWeight < 0.05f) &&
            (fabsf(target - this->playerGroundOffset) < 1.5f)) {
            sGdfStandOffset = this->playerGroundOffset + 1.0f;
            sGdfStandOffsetValid = true;
        }
    }
}

// Rotation that points from `from` to `to`, using the same convention as the vanilla light ball
static void BossGanon_Player_AimRot(Vec3f* from, Vec3f* to, s16* rotX, s16* rotY) {
    f32 dx = to->x - from->x;
    f32 dy = to->y - from->y;
    f32 dz = to->z - from->z;

    *rotY = Math_Atan2S(dz, dx);
    *rotX = Math_Atan2S(sqrtf(SQ(dx) + SQ(dz)), dy);
}

// Closest targetable enemy in front of `from`
// Puzzle targets that are meant to be shot: eye switches, crystal switches, sun switches and unlit torches
static s32 BossGanon_Player_IsPuzzleTarget(Actor* actor) {
    // Play as Ganondorf (2S2H): sun switches (Majora's Mask's eye and crystal switches are other actors, still to do)
    return actor->id == ACTOR_OBJ_LIGHTSWITCH;
}

// What Ganondorf is facing: the target that best combines being in front of him and being close.
// With puzzleTargets, eye switches, crystal switches, sun switches and unlit torches count too.
static Actor* BossGanon_Player_FindTarget(PlayState* play, Vec3f* from, s16 facingYaw, f32 maxDist, s16 maxAngle,
                                          u8 puzzleTargets) {
    static u8 sTargetCategories[] = { ACTORCAT_ENEMY, ACTORCAT_BOSS, ACTORCAT_SWITCH, ACTORCAT_PROP };
    Actor* best = NULL;
    f32 bestScore = 0.0f;
    s32 i;

    for (i = 0; i < ARRAY_COUNT(sTargetCategories); i++) {
        u8 isEnemyCategory = (sTargetCategories[i] == ACTORCAT_ENEMY) || (sTargetCategories[i] == ACTORCAT_BOSS);
        Actor* actor;

        if (!isEnemyCategory && !puzzleTargets) {
            continue;
        }

        for (actor = play->actorCtx.actorLists[sTargetCategories[i]].first; actor != NULL; actor = actor->next) {
            f32 dist;
            f32 score;
            s16 yawDiff;

            if ((actor->update == NULL) || (actor->id == ACTOR_PLAYER_GDF) || (actor->id == ACTOR_EN_GANON_MANT)) {
                continue;
            }
            if (isEnemyCategory) {
                if (!(actor->flags & ACTOR_FLAG_ATTENTION_ENABLED)) {
                    continue;
                }
            } else if (!BossGanon_Player_IsPuzzleTarget(actor)) {
                continue;
            }

            dist = Math_Vec3f_DistXYZ(from, &actor->focus.pos);
            if (dist >= maxDist) {
                continue;
            }

            yawDiff = Math_Vec3f_Yaw(from, &actor->focus.pos) - facingYaw;
            if (ABS(yawDiff) > maxAngle) {
                continue;
            }

            // Angle counts most, so the thing he's turned toward wins over something closer off to the side
            score = (f32)ABS(yawDiff) + (dist * 12.0f);
            if ((best == NULL) || (score < bestScore)) {
                best = actor;
                bestScore = score;
            }
        }
    }

    // Invasion mode: the Links (their dummies here) are his first targets, ahead of any monster
    if (GdfInvasion_CanHurtLinks()) {
        Actor* actor;

        for (actor = play->actorCtx.actorLists[ACTORCAT_NPC].first; actor != NULL; actor = actor->next) {
            u32 clientId = GdfInvasion_DummyClientId(actor);
            Vec3f at;
            f32 dist;
            f32 score;
            s16 yawDiff;

            if ((actor->update == NULL) || (clientId == 0) || !GdfInvasion_CanHurtLink(clientId)) {
                continue;
            }
            at = actor->world.pos;
            at.y += 40.0f;
            if (Math_Vec3f_DistXYZ(&actor->focus.pos, &at) > 60.0f) {
                actor->focus.pos = at; // (aimed at by the light balls and the trident)
            }
            dist = Math_Vec3f_DistXYZ(from, &at);
            yawDiff = Math_Vec3f_Yaw(from, &at) - facingYaw;
            if ((dist >= maxDist * 1.3f) || (ABS(yawDiff) > maxAngle + 0x800)) {
                continue;
            }
            score = ((f32)ABS(yawDiff) + (dist * 12.0f)) * 0.5f;
            if ((best == NULL) || (score < bestScore) || (GdfInvasion_DummyClientId(best) == 0)) {
                best = actor;
                bestScore = score;
            }
        }
    }

    return best;
}

// Aim at Link's lock-on target, else the enemy or puzzle target he's facing, else straight ahead.
// yawTowardsPlayer is reused as the aim direction because the vanilla effect code reads it.
static void BossGanon_Player_UpdateAim(BossGanon* this, PlayState* play) {
    Player* player = GET_PLAYER(play);
    Actor* target = player->focusActor;

    if ((target == NULL) || (target->update == NULL)) {
        target = BossGanon_Player_FindTarget(play, &this->unk_1FC, player->actor.shape.rot.y, 700.0f, 0x2000, true);
    }

    if (target != NULL) {
        this->playerAimPos = target->focus.pos;
    } else {
        this->playerAimPos.x = player->actor.world.pos.x + (Math_SinS(player->actor.shape.rot.y) * 300.0f);
        this->playerAimPos.y = player->actor.world.pos.y + 25.0f;
        this->playerAimPos.z = player->actor.world.pos.z + (Math_CosS(player->actor.shape.rot.y) * 300.0f);
    }

    this->actor.yawTowardsPlayer = Math_Vec3f_Yaw(&this->actor.world.pos, &this->playerAimPos);
}

static void BossGanon_Player_SnapToLink(BossGanon* this, PlayState* play) {
    Player* player = GET_PLAYER(play);

    this->actor.world.pos = player->actor.world.pos;
    this->actor.world.pos.y += BossGanon_Player_GetHeightAboveLink(this);
    this->actor.prevPos = this->actor.world.pos;
    this->actor.shape.rot.y = this->actor.world.rot.y = player->actor.shape.rot.y;
}

static void BossGanon_Player_Follow(BossGanon* this, PlayState* play, u8 controlY) {
    Player* player = GET_PLAYER(play);
    Vec3f target;
    f32 dx;
    f32 dy;
    f32 dz;

    // Up onto a ledge: his own path, timed by Link's climb
    if (this->actionFunc == BossGanon_Player_LedgeClimb) {
        BossGanon_Player_PlaceLedgeClimb(this, play);
        return;
    }

    // Climbing: a little further from the wall than Link, he's much bigger
    if (this->actionFunc == BossGanon_Player_Climb) {
        target = player->actor.world.pos;
        target.x -= Math_SinS(player->actor.shape.rot.y) * GDF_CLIMB_WALL_GAP;
        target.z -= Math_CosS(player->actor.shape.rot.y) * GDF_CLIMB_WALL_GAP;
        target.y += this->playerGroundOffset;
        this->actor.world.pos.x += (target.x - this->actor.world.pos.x) * 0.5f;
        this->actor.world.pos.y += (target.y - this->actor.world.pos.y) * 0.5f;
        this->actor.world.pos.z += (target.z - this->actor.world.pos.z) * 0.5f;
        return;
    }

    // Riding: his pelvis on the saddle
    if ((this->actionFunc == BossGanon_Player_Ride) && BossGanon_Player_GetSeat(play, &target)) {
        target.y += CVarGetFloat(CVAR_PLAY_AS_GDF("HorseSeatHeight"), 0.0f) - 72.56f;
        this->actor.world.pos = target;
        return;
    }

    target.x = player->actor.world.pos.x;
    target.y = player->actor.world.pos.y + BossGanon_Player_GetHeightAboveLink(this);
    target.z = player->actor.world.pos.z;
    BossGanon_Player_SwdKeepBack(play, &target);

    dx = target.x - this->actor.world.pos.x;
    dy = target.y - this->actor.world.pos.y;
    dz = target.z - this->actor.world.pos.z;

    // Warps, voiding out, ...
    if ((SQ(dx) + SQ(dy) + SQ(dz)) > SQ(250.0f)) {
        BossGanon_Player_SnapToLink(this, play);
        return;
    }

    this->actor.world.pos.x += dx * 0.5f;
    this->actor.world.pos.z += dz * 0.5f;

    if (controlY) {
        // Keeps up with Link's jumps
        this->actor.world.pos.y += dy * ((this->playerAirWeight > 0.0f) ? 0.7f : 0.25f);
    }
}

// Also used by Ganon (Boss_Ganon2): moments when Link has to be shown instead of whichever form is out
s32 BossGanon_Player_ShouldHideAnyForm(PlayState* play) {
    Player* player = GET_PLAYER(play);

    // The Gerudo intro: shots without him, and the boy in his vision
    if (false) {
        return true;
    }

    // Play as Ganondorf (2S2H): Link's other forms (Deku, Goron, Zora, Fierce Deity) are their own characters
    if (player->transformation != PLAYER_FORM_HUMAN) {
        return true;
    }
    // First person
    if (player->stateFlags1 & PLAYER_STATE1_FIRST_PERSON) {
        return true;
    }
    // Riding Epona (his horse isn't ported yet)
    if (player->stateFlags1 & PLAYER_STATE1_ON_HORSE) {
        return true;
    }
    // Link isn't drawn either
    if (player->stateFlags2 & PLAYER_STATE2_DISABLE_DRAW) {
        return true;
    }
    // Crawlspaces are too small for Ganondorf, show Link instead
    if (player->stateFlags2 & PLAYER_STATE2_CRAWLING) {
        return true;
    }
    // Story cutscenes animate Link, show him instead
    if (CVarGetInteger(CVAR_PLAY_AS_GDF("ShowLinkInCutscenes"), 1) && (play->csCtx.state != CS_STATE_IDLE)) {
        return true;
    }

    return false;
}

static s32 BossGanon_Player_ShouldHide(BossGanon* this, PlayState* play) {
    s32 hide;

    sGdfFpBodyQuery = (this == sPlayerGanondorf);
    hide = BossGanon_Player_ShouldHideAnyForm(play);
    sGdfFpBodyQuery = false;
    // Ganon is out instead of him
    return hide || PlayAsGanondorf_IsGanonOut(play);
}

static s32 BossGanon_Player_CanStartAttack(BossGanon* this, PlayState* play) {
    // Invasion mode: not while his Dark Arts menu is open or he's been pacified
    return !this->playerHidden && BossGanon_Player_LinkCanAttack(play) && GdfInvasion_CanAct();
}

// Also used by Ganon: Link is in a state where an attack can start
s32 BossGanon_Player_LinkCanAttack(PlayState* play) {
    Player* player = GET_PLAYER(play);

    if (Player_InCsMode(play) || (play->csCtx.state != CS_STATE_IDLE)) {
        return false;
    }
    if ((Message_GetState(&play->msgCtx) != TEXT_STATE_NONE) || (player->textboxBtnCooldownTimer != 0)) {
        return false;
    }
    if (player->stateFlags1 &
        (PLAYER_STATE1_INPUT_DISABLED | PLAYER_STATE1_TALKING | PLAYER_STATE1_DEAD | PLAYER_STATE1_GETTING_ITEM |
         PLAYER_STATE1_IN_ITEM_CS | PLAYER_STATE1_IN_CUTSCENE | PLAYER_STATE1_FIRST_PERSON |
         PLAYER_STATE1_CLIMBING_LADDER | PLAYER_STATE1_HANGING_OFF_LEDGE | PLAYER_STATE1_CLIMBING_LEDGE |
         PLAYER_STATE1_CARRYING_ACTOR)) {
        return false;
    }
    if (player->stateFlags2 & PLAYER_STATE2_CRAWLING) {
        return false;
    }

    return true;
}

static void BossGanon_Player_StartAttack(BossGanon* this, u16 button) {
    this->playerBtnHeld = button;
    this->playerHeldTimer = 0;
}

static void BossGanon_Player_ReadButtons(BossGanon* this, PlayState* play) {
    Input* input = &play->state.input[0];
    u8 useCButtons = !PlayAsGanondorf_KeepItems();
    u8 bigMagicEnabled = CVarGetInteger(CVAR_PLAY_AS_GDF("BigMagic"), 1);

    if (!BossGanon_Player_CanStartAttack(this, play)) {
        return;
    }
    // Invasion mode: C-buttons with a Dark Art on them cast it
    if (GdfInvasion_HandleCButtons(play)) {
        return;
    }
    // The ocarina is out: the D-pad controls the song player
    if (GanondorfSongs_IsOcarinaOut()) {
        return;
    }

    if (BossGanon_Player_TryToggleSword(this, play)) {
        return;
    }

    if (CHECK_BTN_ALL(input->press.button, BTN_B)) {
        if (sPlayerDarkAuraMode) {
            BossGanon_Player_SetupDarkAura(this, play);
        } else if (CHECK_BTN_ALL(input->cur.button, BTN_R)) {
            BossGanon_Player_SetupPound(this, play);
        } else {
            BossGanon_Player_StartAttack(this, BTN_B);
            BossGanon_Player_SetupChargeLightBall(this, play);
        }
    } else if (useCButtons && CHECK_BTN_ALL(input->press.button, BTN_CLEFT)) {
        BossGanon_Player_StartAttack(this, BTN_CLEFT);
        BossGanon_Player_SetupChargeLightBall(this, play);
    } else if (useCButtons && CHECK_BTN_ALL(input->press.button, BTN_CDOWN)) {
        BossGanon_Player_SetupPound(this, play);
    } else if (useCButtons && bigMagicEnabled && CHECK_BTN_ALL(input->press.button, BTN_CRIGHT)) {
        BossGanon_Player_SetupChargeBigMagic(this, play);
    } else if (CHECK_BTN_ALL(input->press.button, BTN_DUP)) {
        BossGanon_Player_SetupLaugh(this, play); // his taunt: the Triforce raised (the dance is D-Pad Down's)
    } else if (CHECK_BTN_ALL(input->press.button, BTN_DDOWN) && BossGanon_Player_TryGrab(this, play)) {
        // by the throat
    } else if (CHECK_BTN_ALL(input->press.button, BTN_DDOWN) && this->playerGrounded &&
               (this->actionFunc == BossGanon_Player_Float)) {
        BossGanon_Player_SetupDance(this, play); // nobody to grab: the dance
    } else if (PlayAsGanondorf_OrganButtonPressed() && (GET_PLAYER(play)->actor.bgCheckFlags & BGCHECKFLAG_GROUND)) {
        // The organ rises out of solid ground, so not while levitating
        BossGanon_Player_SetupOrgan(this, play);
    } else if (CHECK_BTN_ALL(input->cur.button, BTN_R)) {
        BossGanon_Player_SetupBlock(this, play);
    }
}

// ------------------------------------------------------------------------------------------------
// SoH: "Play as Ganondorf", sword stance (D-Pad Left)
//
// D-Pad Left: the sword appears in its sheath, the sheath held at his left hip. The first B draws the blade with a
// cut on the way out; after that B, B, B is a three-hit combo, stick + R a dash (B out of it: a lunging thrust),
// R alone a guard, and the C-buttons sword skills (spin slash, ground slam, dark energy slash). D-Pad Left puts the
// blade back in the sheath, and D-Pad Left again puts the sword away.
//
// The models come from gdf_sword.bin (GanondorfSword.cpp). The arm poses were solved offline with inverse kinematics
// on his skeleton (fist position + blade direction) and are blended with quaternions on top of whatever his body is
// doing (standing, walking, floating). The left arm holds the sheath still while the right arm swings with the walk.
// ------------------------------------------------------------------------------------------------

#define GDF_SWORD_OFF 0
#define GDF_SWORD_SHEATHED 1
#define GDF_SWORD_DRAWN 2

typedef enum {
    GDF_POSE_REST,
    GDF_POSE_GRAB,
    GDF_POSE_DRAW_CUT,
    GDF_POSE_H1_WIND,
    GDF_POSE_H1_HIT,
    GDF_POSE_H1_FOLLOW,
    GDF_POSE_H2_HIT,
    GDF_POSE_H2_FOLLOW,
    GDF_POSE_H3_RAISE,
    GDF_POSE_H3_SLAM,
    GDF_POSE_THRUST,
    GDF_POSE_GUARD,
    GDF_POSE_SHEATH_HOLD, // left arm
    GDF_POSE_SWING_FWD,   // walking with the blade: fist forward
    GDF_POSE_SWING_BACK,  // ...and back
    GDF_POSE_MAX,
} GdfSwordPose;

// Upper arm, forearm and hand rotations (the shoulder keeps the animation's)
// clang-format off
static const Vec3s sGdfSwordPoses[GDF_POSE_MAX][3] = {
    { { 32010, 29971, 23028 }, { 1209, 4825, -8607 }, { 4289, -13661, -1643 } },   // rest
    { { -24427, -31715, 8174 }, { 2889, 4607, -6547 }, { 16078, -601, 4283 } },    // grab
    { { -28730, 29878, 24638 }, { 1853, 5778, -9964 }, { 13918, -710, -7349 } },   // draw_cut
    { { -24122, -27454, 32439 }, { 2425, 6842, -11812 }, { 14006, -239, -3192 } }, // h1_wind
    { { 24675, 28341, 14931 }, { -1745, 5958, -2439 }, { -13920, -554, 8581 } },   // h1_hit
    { { -29516, -23484, 10321 }, { 3176, 5670, -12897 }, { 17519, 2549, -4039 } }, // h1_follow
    { { -30391, 30387, 16607 }, { 1421, 5733, -9965 }, { 8524, -2000, -1621 } },   // h2_hit
    { { -25361, -31449, 24174 }, { 2409, 6432, -11162 }, { 13317, 370, -1941 } },  // h2_follow
    { { -16998, -32175, -23642 }, { 2219, 8469, -11946 }, { 11727, 1137, 2273 } }, // h3_raise
    { { 30798, 24401, 18203 }, { 793, 4581, -7324 }, { 3763, -17056, -153 } },     // h3_slam
    { { -29810, 25473, 11710 }, { 2110, 5219, -9589 }, { 14782, 17209, -12905 } }, // thrust
    { { 31176, 25553, 18697 }, { -744, 6249, -13057 }, { -4786, 428, 6294 } },     // guard
    { { 351, 2785, -8876 }, { 1613, -5640, -11036 }, { 978, 19, -3269 } },         // sheath_hold
    { { 31232, 30617, 22845 }, { 971, 4901, -8784 }, { 4469, -12067, -2338 } },    // swing_fwd
    { { 26886, -27634, 26633 }, { -270, 4689, -6994 }, { 1227, -6930, -2906 } },   // swing_back
};
// clang-format on

#define GDF_LEFT_ARM_LIMB 3 // upper arm; forearm and hand follow
#define GDF_RIGHT_ARM_LIMB 7

// Sword in his hand: fist centre, then the sword frame turned so the blade comes out along the hand's -z
#define GDF_GRIP_X 650.0f
#define GDF_GRIP_Y 0.0f
#define GDF_GRIP_Z -100.0f
#define GDF_TRIDENT_GRIP_Z -500.0f         // the trident model's grip, in the sword frame
#define GDF_TRIDENT_TAIL_Z -2700.0f        // about where its butt end is
#define GDF_TRIDENT_ATTACK_GRIP_Z -1900.0f // where he holds it to attack: near the butt end
#define GDF_TRIDENT_CARRY_GRIP_Z 1300.0f   // the middle of the shaft, where he carries it
#define GDF_SWORD_SCALE 1.4f               // a greatsword: about 62 units of blade on a 100 unit tall Ganondorf
#define GDF_SWORD_GRIP_Z -500.0f           // where he holds the sword (sword frame)
#define GDF_SHEATH_GRIP_Z 350.0f           // where he holds the sheath, just under its mouth
#define GDF_BLADE_BASE_Z -125.0f
#define GDF_BLADE_TIP_Z 4530.0f

typedef enum {
    GDF_SWORD_ATK_NONE,
    GDF_SWORD_ATK_DRAW,
    GDF_SWORD_ATK_SHEATHE,
    GDF_SWORD_ATK_COMBO1,
    GDF_SWORD_ATK_COMBO2,
    GDF_SWORD_ATK_COMBO3,
    GDF_SWORD_ATK_THRUST,
    GDF_SWORD_ATK_SPIN,
    GDF_SWORD_ATK_SLAM,
    GDF_SWORD_ATK_WAVE,
} GdfSwordAttackId;

#define GDF_KEY_FROM -1 // the arm as it was when the attack started
#define GDF_KEY_END -2

typedef struct {
    s8 frame;
    s8 pose;
} GdfSwordKey;

typedef struct {
    GdfSwordKey keys[6];
    s8 length;      // frames until back to the stance
    s8 activeStart; // blade hits on these frames
    s8 activeEnd;
    s8 comboFrom;   // B from this frame queues the next hit
    s8 next;        // the next hit of the combo
    s8 impactFrame; // ground slam / dark wave
    u16 swingSfx;
} GdfSwordAttack;

static const GdfSwordAttack sGdfSwordAttacks[] = {
    /* NONE */ { { { 0, GDF_KEY_END } }, 0, -1, -1, -1, 0, -1, 0 },
    // Draws the blade from the sheath at his hip and cuts outward in one motion
    /* DRAW */
    { { { 0, GDF_KEY_FROM },
        { 5, GDF_POSE_GRAB },
        { 7, GDF_POSE_GRAB },
        { 10, GDF_POSE_DRAW_CUT },
        { 17, GDF_POSE_DRAW_CUT },
        { 25, GDF_POSE_REST } },
      25,
      7,
      12,
      -1,
      0,
      -1,
      NA_SE_IT_SWORD_SWING_HARD },
    // Back into the sheath, with a click
    /* SHEATHE */
    { { { 0, GDF_KEY_FROM },
        { 6, GDF_POSE_DRAW_CUT },
        { 12, GDF_POSE_GRAB },
        { 15, GDF_POSE_GRAB },
        { 20, GDF_KEY_END } },
      20,
      -1,
      -1,
      -1,
      0,
      -1,
      0 },
    /* COMBO1: horizontal slash from his right */
    { { { 0, GDF_KEY_FROM },
        { 3, GDF_POSE_H1_WIND },
        { 6, GDF_POSE_H1_HIT },
        { 9, GDF_POSE_H1_FOLLOW },
        { 18, GDF_POSE_REST } },
      18,
      3,
      10,
      5,
      GDF_SWORD_ATK_COMBO2,
      -1,
      NA_SE_IT_SWORD_SWING_HARD },
    /* COMBO2: backhand */
    { { { 0, GDF_KEY_FROM }, { 3, GDF_POSE_H2_HIT }, { 6, GDF_POSE_H2_FOLLOW }, { 15, GDF_POSE_REST } },
      15,
      1,
      7,
      3,
      GDF_SWORD_ATK_COMBO3,
      -1,
      NA_SE_IT_SWORD_SWING_HARD },
    /* COMBO3: overhead slam */
    { { { 0, GDF_KEY_FROM },
        { 6, GDF_POSE_H3_RAISE },
        { 10, GDF_POSE_H3_SLAM },
        { 20, GDF_POSE_H3_SLAM },
        { 30, GDF_POSE_REST } },
      30,
      7,
      11,
      -1,
      0,
      10,
      NA_SE_IT_SWORD_SWING_HARD },
    /* THRUST: out of a dash */
    { { { 0, GDF_KEY_FROM }, { 3, GDF_POSE_THRUST }, { 11, GDF_POSE_THRUST }, { 18, GDF_POSE_REST } },
      18,
      2,
      10,
      -1,
      0,
      -1,
      NA_SE_IT_SWORD_SWING_HARD },
    /* SPIN: a full turn with the blade out */
    { { { 0, GDF_KEY_FROM }, { 3, GDF_POSE_H1_HIT }, { 16, GDF_POSE_H1_HIT }, { 24, GDF_POSE_REST } },
      24,
      3,
      16,
      -1,
      0,
      -1,
      NA_SE_IT_ROLLING_CUT },
    /* SLAM: overhead into the ground, with a shockwave */
    { { { 0, GDF_KEY_FROM },
        { 8, GDF_POSE_H3_RAISE },
        { 12, GDF_POSE_H3_SLAM },
        { 24, GDF_POSE_H3_SLAM },
        { 32, GDF_POSE_REST } },
      32,
      9,
      13,
      -1,
      0,
      12,
      NA_SE_IT_SWORD_SWING_HARD },
    /* WAVE: a slash that sends dark energy flying */
    { { { 0, GDF_KEY_FROM },
        { 5, GDF_POSE_H1_WIND },
        { 9, GDF_POSE_H1_HIT },
        { 12, GDF_POSE_H1_FOLLOW },
        { 20, GDF_POSE_REST } },
      20,
      7,
      11,
      -1,
      0,
      9,
      NA_SE_IT_SWORD_SWING_HARD },
};

// His whole body moves with the sword: the upper body twists into the swing (positive: turned to his left) and
// leans (positive: forward), the hips turn a little the other way, and his legs step into a lunge (a frame of the
// walk with the right foot forward) on the strikes.
typedef struct {
    s8 frame;
    s16 twist;
    s16 lean;
    u8 lunge; // percent
} GdfSwordBodyKey;

#define GDF_LUNGE_FRAME 0.0f // walk frame with the right foot forward and both feet down

static const GdfSwordBodyKey sGdfSwordBody[][6] = {
    /* NONE */ { { -1 } },
    /* DRAW: turn to the sheath at his left hip, then unwind into the cut */
    { { 0, 0, 0, 0 },
      { 5, 0x1800, 0x0A00, 20 },
      { 7, 0x1C00, 0x0A00, 30 },
      { 10, -0x2000, 0x0600, 90 },
      { 17, -0x1400, 0x0300, 70 },
      { 25, 0, 0, 0 } },
    /* SHEATHE: lift, then turn to the hip and look down at it */
    { { 0, 0, 0, 0 },
      { 6, -0x0C00, -0x0300, 0 },
      { 12, 0x1800, 0x0C00, 20 },
      { 15, 0x1800, 0x0C00, 20 },
      { 20, 0, 0, 0 },
      { -1 } },
    /* COMBO1: wind up turned right, unwind to the left */
    { { 0, 0, 0, 0 },
      { 3, -0x2400, 0x0200, 30 },
      { 6, 0x1400, 0x0A00, 100 },
      { 9, 0x2800, 0x0800, 100 },
      { 18, 0, 0, 0 },
      { -1 } },
    /* COMBO2: backhand, unwinding to the right */
    { { 0, 0x2800, 0x0800, 100 }, { 3, -0x0800, 0x0A00, 100 }, { 6, -0x2800, 0x0800, 100 }, { 15, 0, 0, 0 }, { -1 } },
    /* COMBO3: rear back, then bend into the slam */
    { { 0, -0x2800, 0x0800, 100 },
      { 6, -0x0800, -0x0C00, 40 },
      { 10, 0, 0x1C00, 100 },
      { 20, 0, 0x1800, 100 },
      { 30, 0, 0, 0 },
      { -1 } },
    /* THRUST: right shoulder forward, leaning into it */
    { { 0, 0, 0, 0 }, { 3, -0x1400, 0x1000, 100 }, { 11, -0x1000, 0x0C00, 100 }, { 18, 0, 0, 0 }, { -1 } },
    /* SPIN: crouched forward while he turns */
    { { 0, 0, 0, 0 }, { 3, 0, 0x0800, 60 }, { 16, 0, 0x0800, 60 }, { 24, 0, 0, 0 }, { -1 } },
    /* SLAM */
    { { 0, 0, 0, 0 },
      { 8, -0x0800, -0x1000, 40 },
      { 12, 0, 0x2000, 100 },
      { 24, 0, 0x1C00, 100 },
      { 32, 0, 0, 0 },
      { -1 } },
    /* WAVE: like the first slash, flung wider */
    { { 0, 0, 0, 0 },
      { 5, -0x2800, 0x0200, 40 },
      { 9, 0x1800, 0x0C00, 100 },
      { 12, 0x2C00, 0x0800, 100 },
      { 20, 0, 0, 0 },
      { -1 } },
};

// Persists across areas (the player Ganondorf is spawned again in every scene)
static u8 sPlayerSwordState = GDF_SWORD_OFF;

// His sword or trident is out: the C-buttons are its moves, not Link's items

static ColliderQuadInit sPlayerSwordQuadInit = {
    {
        COLTYPE_NONE,
        AT_ON | AT_TYPE_PLAYER,
        AC_NONE,
        OC1_NONE,
        OC2_NONE,
        COLSHAPE_QUAD,
    },
    {
        ELEMTYPE_UNK2,
        { DMG_GANONDORF_PLAYER, 0x00, 0x08 },
        { 0x00000000, 0x00, 0x00 },
        TOUCH_ON | TOUCH_SFX_NORMAL,
        BUMP_NONE,
        OCELEM_NONE,
    },
    { { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } } },
};

// ---- Quaternion blending of joint rotations ----------------------------------------------------

typedef struct {
    f32 w, x, y, z;
} GdfQuat;

static GdfQuat BossGanon_Player_RotToQuat(Vec3s* rot) {
    f32 m[3][3];
    GdfQuat q;
    f32 trace;
    f32 s;

    BossGanon_Player_RotToMtx(rot, m);
    trace = m[0][0] + m[1][1] + m[2][2];
    if (trace > 0.0f) {
        s = sqrtf(trace + 1.0f) * 2.0f;
        q.w = 0.25f * s;
        q.x = (m[2][1] - m[1][2]) / s;
        q.y = (m[0][2] - m[2][0]) / s;
        q.z = (m[1][0] - m[0][1]) / s;
    } else if ((m[0][0] > m[1][1]) && (m[0][0] > m[2][2])) {
        s = sqrtf(1.0f + m[0][0] - m[1][1] - m[2][2]) * 2.0f;
        q.w = (m[2][1] - m[1][2]) / s;
        q.x = 0.25f * s;
        q.y = (m[0][1] + m[1][0]) / s;
        q.z = (m[0][2] + m[2][0]) / s;
    } else if (m[1][1] > m[2][2]) {
        s = sqrtf(1.0f + m[1][1] - m[0][0] - m[2][2]) * 2.0f;
        q.w = (m[0][2] - m[2][0]) / s;
        q.x = (m[0][1] + m[1][0]) / s;
        q.y = 0.25f * s;
        q.z = (m[1][2] + m[2][1]) / s;
    } else {
        s = sqrtf(1.0f + m[2][2] - m[0][0] - m[1][1]) * 2.0f;
        q.w = (m[1][0] - m[0][1]) / s;
        q.x = (m[0][2] + m[2][0]) / s;
        q.y = (m[1][2] + m[2][1]) / s;
        q.z = 0.25f * s;
    }
    return q;
}

static void BossGanon_Player_QuatToRot(GdfQuat q, Vec3s* rot) {
    f32 m[3][3];

    m[0][0] = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
    m[0][1] = 2.0f * (q.x * q.y - q.z * q.w);
    m[0][2] = 2.0f * (q.x * q.z + q.y * q.w);
    m[1][0] = 2.0f * (q.x * q.y + q.z * q.w);
    m[1][1] = 1.0f - 2.0f * (q.x * q.x + q.z * q.z);
    m[1][2] = 2.0f * (q.y * q.z - q.x * q.w);
    m[2][0] = 2.0f * (q.x * q.z - q.y * q.w);
    m[2][1] = 2.0f * (q.y * q.z + q.x * q.w);
    m[2][2] = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
    BossGanon_Player_MtxToRot(m, rot);
}

// Rotation part way from `a` to `b`
static void BossGanon_Player_SlerpRot(Vec3s* a, Vec3s* b, f32 t, Vec3s* out) {
    GdfQuat qa;
    GdfQuat qb;
    GdfQuat q;
    f32 dot;
    f32 len;

    if (t <= 0.0f) {
        *out = *a;
        return;
    }
    if (t >= 1.0f) {
        *out = *b;
        return;
    }

    qa = BossGanon_Player_RotToQuat(a);
    qb = BossGanon_Player_RotToQuat(b);
    dot = qa.w * qb.w + qa.x * qb.x + qa.y * qb.y + qa.z * qb.z;
    if (dot < 0.0f) {
        qb.w = -qb.w;
        qb.x = -qb.x;
        qb.y = -qb.y;
        qb.z = -qb.z;
        dot = -dot;
    }

    if (dot > 0.9995f) {
        q.w = qa.w + (qb.w - qa.w) * t;
        q.x = qa.x + (qb.x - qa.x) * t;
        q.y = qa.y + (qb.y - qa.y) * t;
        q.z = qa.z + (qb.z - qa.z) * t;
    } else {
        f32 theta = acosf(dot);
        f32 sinTheta = sinf(theta);
        f32 wa = sinf((1.0f - t) * theta) / sinTheta;
        f32 wb = sinf(t * theta) / sinTheta;

        q.w = qa.w * wa + qb.w * wb;
        q.x = qa.x * wa + qb.x * wb;
        q.y = qa.y * wa + qb.y * wb;
        q.z = qa.z * wa + qb.z * wb;
    }
    len = sqrtf(q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z);
    q.w /= len;
    q.x /= len;
    q.y /= len;
    q.z /= len;
    BossGanon_Player_QuatToRot(q, out);
}

// Moves an arm's three joints (upper arm, forearm, hand) part way towards `pose`
static void BossGanon_Player_BlendArm(BossGanon* this, s32 firstLimb, const Vec3s* pose, f32 weight) {
    Vec3s* joints = this->skelAnime.jointTable;
    s32 i;

    if (weight <= 0.0f) {
        return;
    }
    for (i = 0; i < 3; i++) {
        Vec3s target = pose[i];
        BossGanon_Player_SlerpRot(&joints[1 + firstLimb + i], &target, weight, &joints[1 + firstLimb + i]);
    }
}

// The right arm at `frame` of the current attack: blends between its keyframes

// Left arm holds the sheath still. The right arm swings with the walk; with the blade out it rests the blade lowered
// when standing, and swings between two poses that keep the blade pointing forward and down when walking.

// ---- Whole-body motion -------------------------------------------------------------------------

static void BossGanon_Player_MtxMul(f32 a[3][3], f32 b[3][3], f32 out[3][3]) {
    f32 r[3][3];
    s32 i;
    s32 j;

    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            r[i][j] = (a[i][0] * b[0][j]) + (a[i][1] * b[1][j]) + (a[i][2] * b[2][j]);
        }
    }
    memcpy(out, r, sizeof(r));
}

// Turns a joint (whose parent is the root) by `turn` in his own space (actor space): R' = Rroot^T * turn * Rroot * R
static void BossGanon_Player_TurnJoint(BossGanon* this, s32 limb, f32 turn[3][3]) {
    Vec3s* joints = this->skelAnime.jointTable;
    f32 root[3][3];
    f32 rootT[3][3];
    f32 joint[3][3];
    f32 m[3][3];
    s32 i;
    s32 j;

    BossGanon_Player_RotToMtx(&joints[1], root);
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            rootT[i][j] = root[j][i];
        }
    }
    BossGanon_Player_RotToMtx(&joints[1 + limb], joint);
    BossGanon_Player_MtxMul(root, joint, m);
    BossGanon_Player_MtxMul(turn, m, m);
    BossGanon_Player_MtxMul(rootT, m, m);
    BossGanon_Player_MtxToRot(m, &joints[1 + limb]);
}

// On the ground, his legs stand (from his standing idle) while his upper body plays the action. The block, light
// ball and magic animations are made for floating: their legs fold up or hang down, which sank him into the floor.
// The pelvis is set so his legs keep their standing direction whatever the animation does with his root.
static void BossGanon_Player_StandLegs(BossGanon* this) {
    static Vec3s sStand[GDF_LIMB_COUNT_PLAYER + 1];
    static u8 sStandRead = false;
    Vec3s* joints = this->skelAnime.jointTable;
    f32 weight = this->playerStandLegsWeight;
    f32 root[3][3];
    f32 standRoot[3][3];
    f32 standPelvis[3][3];
    f32 m[3][3];
    Vec3s pelvis;
    s32 i;
    s32 j;

    if (weight <= 0.0f) {
        return;
    }
    if (!sStandRead) {
        SkelAnime_GetFrameData((AnimationHeader*)gGanondorfStandIdleAnim, 0, GDF_LIMB_COUNT_PLAYER + 1, sStand);
        sStandRead = true;
    }

    // pelvis = root^T * standRoot * standPelvis
    BossGanon_Player_RotToMtx(&joints[1], root);
    BossGanon_Player_RotToMtx(&sStand[1], standRoot);
    BossGanon_Player_RotToMtx(&sStand[1 + 18], standPelvis);
    BossGanon_Player_MtxMul(standRoot, standPelvis, m);
    for (i = 0; i < 3; i++) {
        for (j = i + 1; j < 3; j++) {
            f32 t = root[i][j];

            root[i][j] = root[j][i];
            root[j][i] = t;
        }
    }
    BossGanon_Player_MtxMul(root, m, m);
    BossGanon_Player_MtxToRot(m, &pelvis);

    BossGanon_Player_SlerpRot(&joints[1 + 18], &pelvis, weight, &joints[1 + 18]);
    for (i = 19; i <= 24; i++) {
        BossGanon_Player_SlerpRot(&joints[1 + i], &sStand[1 + i], weight, &joints[1 + i]);
    }
    // His standing height, so crouching poses bend at the hips instead of pushing his legs into the floor. Fully,
    // not blended: while the animations morph in and out of the floating ones (whose root sits 72 units lower), a
    // blended height dipped him into the ground at the start and end of every throw.
    joints[0].y = sStand[0].y;
}

static void BossGanon_Player_TwistLean(BossGanon* this, f32 twist, f32 lean) {
    Vec3s yaw = { 0, 0, 0 };
    Vec3s pitch = { 0, 0, 0 };
    f32 my[3][3];
    f32 mx[3][3];
    f32 turn[3][3];

    if ((twist == 0.0f) && (lean == 0.0f)) {
        return;
    }
    // Upper body
    yaw.y = (s16)twist;
    pitch.x = (s16)lean;
    BossGanon_Player_RotToMtx(&yaw, my);
    BossGanon_Player_RotToMtx(&pitch, mx);
    BossGanon_Player_MtxMul(my, mx, turn);
    BossGanon_Player_TurnJoint(this, 1, turn);
    // Hips turn a little the other way
    yaw.y = (s16)(twist * -0.35f);
    BossGanon_Player_RotToMtx(&yaw, my);
    BossGanon_Player_TurnJoint(this, 18, my);
}

// Body pose of the current attack at `frame`: twist, lean and the lunge

// ---- Jumping ------------------------------------------------------------------------------------
//
// Like Link's jumps (Link still does the jumping): a leap with both knees pulled up, but with Ganondorf's flair: arms
// spread wide (with the blade out, raised overhead), and after a big drop a heavy landing on one knee from his boss
// fight that shakes the ground, then he rises.

// clang-format off
static const Vec3s sGdfJumpArmPoses[2][3] = {
    { { -30128, -32436, 27559 }, { 310, 6473, -7851 }, { 2331, 1984, -1421 } }, // right, spread
    { { -2258, 4986, -2650 }, { 1116, -5525, -6703 }, { -724, -575, -1517 } },  // left, spread
};
// clang-format on

#define GDF_JUMP_LEFT_KNEE_FRAME 5 // walk frames with that knee highest
#define GDF_JUMP_RIGHT_KNEE_FRAME 20
#define GDF_BIG_LANDING_HEIGHT 100.0f

static s32 BossGanon_Player_IsJumping(PlayState* play) {
    Player* player = GET_PLAYER(play);

    return !(player->actor.bgCheckFlags & BGCHECKFLAG_GROUND) &&
           !(player->stateFlags1 & (PLAYER_STATE1_CLIMBING_LADDER | PLAYER_STATE1_HANGING_OFF_LEDGE |
                                    PLAYER_STATE1_CLIMBING_LEDGE | PLAYER_STATE1_IN_WATER | PLAYER_STATE1_ON_HORSE)) &&
           !(player->stateFlags2 & PLAYER_STATE2_CRAWLING);
}

static void BossGanon_Player_ApplyJump(BossGanon* this, PlayState* play) {
    f32 weight;

    Math_ApproachF(&this->playerAirWeight, (this->playerGrounded && BossGanon_Player_IsJumping(play)) ? 1.0f : 0.0f,
                   1.0f, 0.2f);
    weight = this->playerAirWeight;
    if (weight <= 0.0f) {
        sGdfJumpFrame = 0.0f;
        return;
    }

    // The running jump: take-off and the leap up (its first 13 frames, played at its own speed), then held tucked
    // while he flies; falling, his legs reach down for the ground
    if (BossGanon_Player_IsJumping(play)) {
        static u32 sLastJumpFrame = 0;
        Player* player = GET_PLAYER(play);

        if (play->gameplayFrames - sLastJumpFrame > 1) {
            sGdfJumpFrame = 0.0f; // a new jump: from the take-off
        }
        sLastJumpFrame = play->gameplayFrames;
        if (sGdfJumpFrame < 13.0f) {
            sGdfJumpFrame = MIN(sGdfJumpFrame + 1.5f, 13.0f);
        } else if (player->actor.velocity.y < -4.0f) {
            sGdfJumpFrame = MIN(sGdfJumpFrame + 0.5f, 17.0f);
        }
    }
    BossGanon_Player_ApplyClip(this, sGdfClipJump, ARRAY_COUNT(sGdfClipJump), sGdfJumpFrame, false, weight, 1.0f);
}

// The heavy landing: Mixamo's "Hard Landing" (from its feet touching down, its frame 4, to frame 50, as he rises; the
// bone directions, the pelvis and chest as the source's own, the head as its head bone's turn, the backs of his hands
// from its knuckles). Its root keeps both feet where they landed (the height: his ground offset keeps them down) until
// he gets up; from there it hands back to his idle stance.
// clang-format off
static const Vec3s sGdfClipLand[47][13] = {
    { { -96, -5068, 29569 }, { 4112, -23677, -17958 }, { -24293, -4238, 17084 }, { 14943, -24610, -8429 }, { 23054, -19144, 1409 }, { 13259, -14606, 22603 }, { -25144, -11236, -11897 }, { -25170, -13121, 9710 }, { -11227, 5001, -13936 }, { 11402, -2180, 16961 }, { 12810, -3880, 15284 }, { -350, -29947, 1747 }, { -1053, -29980, -279 } },
    { { 1569, -1564, 29918 }, { 1090, -25459, -15832 }, { -23640, -3020, 18222 }, { 13869, -25300, -8219 }, { 20126, -21608, 5295 }, { 5984, -14741, 25434 }, { -24995, -15564, -5746 }, { -18086, -15144, 18536 }, { -11226, 7455, -13469 }, { 11935, -5071, 17378 }, { 13815, -4646, 14704 }, { -1247, -29966, 683 }, { -1096, -29979, -290 } },
    { { 4860, -956, 29588 }, { -2575, -26189, -14405 }, { -22982, -2958, 19055 }, { 13532, -25276, -8832 }, { 17123, -23674, 6807 }, { -219, -15806, 25497 }, { -21172, -21250, -432 }, { -11986, -18259, 20566 }, { -11163, 9914, -12638 }, { 12777, -7795, 17989 }, { 15259, -5455, 14313 }, { -1449, -29961, 510 }, { 718, -29968, -1185 } },
    { { 8436, -1001, 28772 }, { -5872, -25761, -14208 }, { -23033, -2696, 19032 }, { 14017, -24622, -9863 }, { 8428, -24348, 15368 }, { 1936, -20949, 21387 }, { -14776, -25140, 7047 }, { -8878, -20944, 19558 }, { -10521, 11829, -11217 }, { 13259, -9797, 18774 }, { 16546, -5788, 14365 }, { -1133, -29978, 85 }, { 2727, -29773, -2472 } },
    { { 10852, 231, 27967 }, { -7876, -24809, -14916 }, { -23260, -1242, 18905 }, { 14604, -23498, -11602 }, { 5210, -24715, 16187 }, { -58, -23355, 18829 }, { -9289, -25533, 12720 }, { -6766, -23375, 17546 }, { -9696, 12893, -9831 }, { 13399, -10894, 19564 }, { 17027, -6452, 15354 }, { -812, -29985, 506 }, { 3813, -29561, -3411 } },
    { { 11340, 759, 27764 }, { -7971, -23918, -16260 }, { -22129, -463, 20251 }, { 13810, -22250, -14635 }, { 10031, -24169, 14671 }, { -6525, -23730, 17156 }, { -12432, -26484, 6637 }, { 2841, -20973, 21262 }, { -9398, 13649, -9238 }, { 13627, -11670, 19845 }, { 17270, -8705, 15778 }, { 569, -29937, 1860 }, { -2648, -29831, 1755 } },
    { { 10721, -39, 28019 }, { -6612, -23177, -17864 }, { -19460, -1807, 22761 }, { 12016, -20386, -18440 }, { 13739, -23222, 13115 }, { -11606, -22922, 15489 }, { -18813, -22598, 5947 }, { 10113, -21284, 18567 }, { -9805, 14316, -9592 }, { 14070, -12329, 19278 }, { 18589, -11693, 14662 }, { 2157, -29831, 2335 }, { -2648, -29831, 1755 } },
    { { 10787, -1275, 27964 }, { -5354, -22573, -19020 }, { -16196, -4505, 24848 }, { 10150, -18129, -21640 }, { 15552, -22572, 12191 }, { -14218, -21628, 15168 }, { -20232, -21121, 6674 }, { 11858, -23355, 14627 }, { -11979, 14703, -11727 }, { 15017, -12652, 17954 }, { -31215, -14754, -845 }, { 2154, -29831, 2334 }, { -2649, -29831, 1754 } },
    { { 12472, -2193, 27196 }, { -5239, -22181, -19508 }, { -13230, -7112, 25969 }, { 8678, -15656, -24075 }, { 14885, -23265, 11713 }, { -13701, -22363, 14566 }, { -19298, -22395, 5105 }, { 10166, -25035, 13034 }, { -15773, 14592, -15294 }, { 16306, -12536, 16504 }, { -21260, -11131, -10822 }, { 2153, -29831, 2334 }, { -2648, -29831, 1754 } },
    { { 15021, -2810, 25816 }, { -5867, -22057, -19470 }, { -10706, -9699, 26293 }, { 7274, -12988, -26046 }, { 12623, -24759, 11298 }, { -11463, -24103, 13699 }, { -17172, -24512, 2066 }, { 6392, -26397, 12742 }, { -18827, 14090, -17966 }, { 17583, -12200, 15358 }, { -22025, -10011, -10404 }, { 2154, -29831, 2334 }, { -2650, -29831, 1758 } },
    { { 17351, -3480, 24225 }, { -6566, -22159, -19127 }, { -8175, -12174, 26172 }, { 5579, -10306, -27616 }, { 9424, -26515, 10398 }, { -8081, -25738, 13126 }, { -14735, -26131, -166 }, { 2500, -27565, 11573 }, { -20847, 13574, -19533 }, { 18750, -11923, 14483 }, { -23348, -9949, -9298 }, { 2154, -29831, 2334 }, { -2651, -29831, 1760 } },
    { { 19080, -4377, 22733 }, { -7091, -22378, -18680 }, { -5541, -14163, 25859 }, { 3713, -7948, -28689 }, { 7586, -27594, 9000 }, { -5783, -26592, 12625 }, { -13063, -27006, 205 }, { 989, -28795, 8357 }, { -22559, 13303, -20773 }, { 19566, -11872, 13672 }, { -24045, -9888, -8922 }, { 2154, -29831, 2334 }, { -2648, -29831, 1754 } },
    { { 20271, -5274, 21478 }, { -7286, -22580, -18360 }, { -3246, -15576, 25433 }, { 2078, -6069, -29306 }, { 5403, -28421, 7942 }, { -3953, -27735, 10730 }, { -10904, -27946, 351 }, { -633, -29513, 5345 }, { -24213, 13241, -21995 }, { 20282, -11986, 12867 }, { -24659, -10012, -8707 }, { 2154, -29831, 2333 }, { -2649, -29831, 1754 } },
    { { 21114, -5834, 20498 }, { -7202, -22652, -18303 }, { -1666, -16580, 24946 }, { 882, -4517, -29645 }, { 6129, -28559, 6841 }, { -4598, -27971, 9821 }, { -9588, -28425, -250 }, { -1576, -29698, 3942 }, { -25574, 13231, -23028 }, { 20796, -12102, 12172 }, { -25820, -10368, -8002 }, { 2154, -29831, 2334 }, { -2648, -29831, 1754 } },
    { { 21743, -6034, 19770 }, { -7094, -22596, -18414 }, { -827, -17308, 24490 }, { 148, -3171, -29832 }, { 7570, -28338, 6295 }, { -5850, -27888, 9382 }, { -9670, -28374, -1186 }, { -1065, -29765, 3596 }, { -26406, 13205, -23658 }, { 21152, -12145, 11738 }, { -27542, -10689, -6644 }, { 2154, -29831, 2334 }, { -2648, -29831, 1754 } },
    { { 22149, -6067, 19303 }, { -7050, -22444, -18616 }, { -608, -17788, 24150 }, { -97, -2051, -29930 }, { 8752, -28018, 6197 }, { -6804, -27723, 9227 }, { -11074, -27759, -2605 }, { 756, -29731, 3937 }, { -26729, 13170, -23888 }, { 21536, -12163, 11525 }, { -29062, -10719, -5360 }, { 2154, -29831, 2334 }, { -2648, -29831, 1754 } },
    { { 22277, -6022, 19170 }, { -6935, -22193, -18958 }, { -905, -18412, 23668 }, { 111, -878, -29987 }, { 9606, -27739, 6185 }, { -7508, -27496, 9359 }, { -12939, -26764, -4033 }, { 2787, -29566, 4255 }, { -26708, 13136, -23840 }, { 22043, -12193, 11396 }, { -29699, -10466, -4862 }, { 2154, -29831, 2334 }, { -2648, -29831, 1754 } },
    { { 22145, -5861, 19371 }, { -6689, -21887, -19396 }, { -1544, -18760, 23360 }, { 634, -35, -29993 }, { 9789, -27668, 6218 }, { -7774, -27337, 9606 }, { -14824, -25577, -5107 }, { 4461, -29325, 4490 }, { -26513, 13094, -23637 }, { 22656, -12219, 11282 }, { -29327, -9957, -5154 }, { 2154, -29831, 2334 }, { -2648, -29831, 1754 } },
    { { 21833, -5588, 19801 }, { -6413, -21622, -19783 }, { -2363, -18716, 23327 }, { 1318, 358, -29969 }, { 9317, -27800, 6352 }, { -7570, -27299, 9871 }, { -16485, -24407, -5703 }, { 5612, -29083, 4764 }, { -26217, 13036, -23342 }, { 23243, -12220, 11212 }, { -28255, -9245, -5834 }, { 2154, -29831, 2334 }, { -2648, -29831, 1754 } },
    { { 21403, -5259, 20353 }, { -6190, -21453, -20036 }, { -3251, -18284, 23561 }, { 2096, 238, -29926 }, { 8640, -27935, 6706 }, { -7140, -27302, 10181 }, { -17747, -23444, -5953 }, { 6278, -28862, 5252 }, { -25817, 12979, -22983 }, { 23639, -12199, 11246 }, { -26907, -8537, -6532 }, { 2154, -29831, 2334 }, { -2648, -29831, 1754 } },
    { { 20889, -4934, 20960 }, { -6021, -21380, -20166 }, { -4143, -17448, 24050 }, { 2919, -475, -29854 }, { 8333, -27863, 7364 }, { -6965, -27194, 10581 }, { -18530, -22807, -6040 }, { 6572, -28611, 6180 }, { -25336, 12951, -22596 }, { 23734, -12175, 11400 }, { -25583, -8081, -7130 }, { 2154, -29831, 2334 }, { -2648, -29831, 1754 } },
    { { 20330, -4619, 21572 }, { -5917, -21452, -20119 }, { -4927, -16169, 24785 }, { 3656, -1846, -29719 }, { 8157, -27644, 8323 }, { -6836, -27046, 11036 }, { -19084, -22348, -6031 }, { 6727, -28202, 7706 }, { -24863, 12963, -22241 }, { 23645, -12173, 11641 }, { -24478, -7935, -7524 }, { 2151, -29832, 2333 }, { -2648, -29831, 1754 } },
    { { 19780, -4288, 22144 }, { -5922, -21742, -19805 }, { -5465, -14420, 25733 }, { 4137, -3883, -29459 }, { 7656, -27530, 9137 }, { -6090, -26873, 11864 }, { -19500, -22091, -5635 }, { 6808, -27624, 9516 }, { -24451, 13005, -21930 }, { 23444, -12193, 11941 }, { -23774, -8089, -7640 }, { 2036, -29842, 2308 }, { -2648, -29831, 1754 } },
    { { 19211, -3975, 22696 }, { -6028, -22223, -19231 }, { -5781, -12771, 26523 }, { 4284, -6068, -29066 }, { 6083, -27691, 9808 }, { -3821, -26877, 12769 }, { -19811, -22069, -4526 }, { 6776, -27030, 11110 }, { -23995, 13049, -21548 }, { 23186, -12203, 12329 }, { -23517, -8505, -7441 }, { 2364, -29831, 2122 }, { -2648, -29831, 1754 } },
    { { 18521, -3743, 23301 }, { -6155, -22818, -18478 }, { -5987, -11381, 27104 }, { 4078, -8291, -28542 }, { 7380, -27431, 9647 }, { -4133, -25838, 14673 }, { -19934, -22241, -2827 }, { 6409, -26565, 12379 }, { -23335, 13071, -20929 }, { 22910, -12183, 12871 }, { -23602, -9126, -6976 }, { 2656, -29810, -2073 }, { -2648, -29831, 1754 } },
    { { 17655, -3677, 23975 }, { -6254, -23455, -17628 }, { -6249, -10120, 27542 }, { 3654, -10585, -27832 }, { 7821, -27234, 9857 }, { -3484, -24976, 16250 }, { -19849, -22484, -692 }, { 5659, -26324, 13229 }, { -22459, 13094, -20036 }, { 22658, -12157, 13596 }, { -23880, -9888, -6303 }, { 2929, -28925, -7400 }, { -2648, -29831, 1754 } },
    { { 16687, -3890, 24625 }, { -6365, -24049, -16768 }, { -6711, -9058, 27801 }, { 3193, -12843, -26923 }, { 8820, -27172, 9159 }, { -3499, -23630, 18149 }, { -19325, -22909, 1313 }, { 4265, -26251, 13882 }, { -21471, 13144, -18979 }, { 22478, -12156, 14444 }, { -24319, -10679, -5419 }, { 3504, -26689, -13244 }, { -2648, -29831, 1754 } },
    { { 15780, -4376, 25136 }, { -6586, -24550, -15934 }, { -7477, -8335, 27832 }, { 2938, -14852, -25899 }, { 10312, -27080, 7767 }, { -3896, -21859, 20174 }, { -17272, -24502, 1151 }, { 391, -25616, 15611 }, { -20456, 13156, -17922 }, { 22367, -12130, 15275 }, { -24933, -11349, -4390 }, { 4338, -22503, -19360 }, { -2668, -29830, 1751 } },
    { { 15035, -4982, 25478 }, { -6924, -24971, -15117 }, { -8475, -8049, 27629 }, { 2941, -16509, -24876 }, { 12314, -26667, 6105 }, { -4648, -20145, 21739 }, { -13860, -26600, 563 }, { -5289, -24327, 16740 }, { -19509, 13020, -17064 }, { 22207, -11980, 15945 }, { -25641, -11864, -3430 }, { 5598, -18462, -22975 }, { -2426, -29896, -574 } },
    { { 14325, -5702, 25735 }, { -7167, -25322, -14402 }, { -9478, -8279, 27233 }, { 3072, -17855, -23911 }, { 14713, -25790, 4293 }, { -5398, -18438, 23041 }, { -14488, -26157, 2433 }, { -5203, -24007, 17221 }, { -18670, 12742, -16480 }, { 21896, -11690, 16427 }, { -26323, -12339, -2734 }, { 5603, -16423, -24472 }, { -600, -28959, -7812 } },
    { { 13488, -6632, 25963 }, { -7111, -25631, -13874 }, { -10259, -9030, 26706 }, { 3183, -18958, -23031 }, { 17792, -24094, 1700 }, { -6037, -17138, 23872 }, { -15451, -25336, 4398 }, { -5130, -23468, 17970 }, { -17916, 12401, -16060 }, { 21489, -11341, 16763 }, { -26959, -12858, -2275 }, { 4268, -16928, -24397 }, { 5880, -24526, -16244 } },
    { { 12582, -7735, 26112 }, { -6812, -25918, -13485 }, { -10819, -10108, 26092 }, { 3206, -19937, -22186 }, { 20755, -21649, -753 }, { -6479, -16437, 24246 }, { -15515, -25339, 4148 }, { -5496, -22918, 18562 }, { -17260, 12015, -15723 }, { 21029, -10981, 16970 }, { -27730, -13382, -1772 }, { 3787, -18093, -23628 }, { 7070, -21325, -19881 } },
    { { 11753, -8953, 26110 }, { -6459, -26178, -13153 }, { -11321, -11352, 25357 }, { 3343, -20847, -21313 }, { 23229, -18717, -3179 }, { -6476, -16778, 24012 }, { -16010, -25207, 2880 }, { -5336, -22187, 19475 }, { -16712, 11580, -15481 }, { 20521, -10593, 17079 }, { -29053, -13832, -752 }, { 3931, -19429, -22518 }, { 8038, -18344, -22336 } },
    { { 11040, -10271, 25935 }, { -6140, -26405, -12848 }, { -11887, -12544, 24522 }, { 3720, -21611, -20473 }, { 24910, -15916, -5118 }, { -5730, -18266, 23098 }, { -17826, -24117, 793 }, { -3761, -22137, 19895 }, { -16239, 11129, -15301 }, { 20030, -10167, 17155 }, { -31241, -14136, 1131 }, { 4114, -20345, -21660 }, { 9458, -16072, -23500 } },
    { { 10364, -11702, 25606 }, { -5820, -26600, -12592 }, { -12461, -13757, 23569 }, { 4125, -22335, -19600 }, { 25773, -14211, -5814 }, { -4288, -20590, 21393 }, { -20259, -22062, -1689 }, { -1982, -23123, 19011 }, { -15793, 10665, -15130 }, { 19596, -9696, 17252 }, { 31281, -14242, 3849 }, { 4192, -20243, -21740 }, { 10283, -14433, -24206 } },
    { { 9589, -13224, 25163 }, { -5374, -26777, -12414 }, { -12972, -15025, 22494 }, { 4329, -23078, -18672 }, { 26297, -13299, -5623 }, { -2857, -22627, 19490 }, { -22208, -19858, -3526 }, { -1036, -24715, 16974 }, { -15340, 10143, -14967 }, { 19170, -9174, 17362 }, { 28014, -14116, 6809 }, { 3714, -19132, -22807 }, { 9816, -13414, -24974 } },
    { { 8581, -14764, 24665 }, { -4633, -26968, -12298 }, { -13404, -16399, 21246 }, { 4428, -23878, -17613 }, { 26609, -12857, -5164 }, { -1596, -24036, 17881 }, { -23163, -18454, -4787 }, { -1123, -26428, 14153 }, { -14816, 9552, -14808 }, { 18628, -8618, 17480 }, { 25218, -13803, 9262 }, { 1967, -17782, -24082 }, { 9598, -12513, -25521 } },
    { { 7234, -16155, 24222 }, { -3533, -27185, -12186 }, { -13975, -17890, 19612 }, { 4622, -24815, -16213 }, { 26657, -12911, -4762 }, { -327, -24927, 16689 }, { -23421, -17641, -6344 }, { -1570, -27571, 11721 }, { -14112, 8960, -14582 }, { 17897, -8080, 17642 }, { 23101, -13436, 11008 }, { -871, -16744, -24877 }, { 11188, -11843, -25191 } },
    { { 5584, -17201, 23936 }, { -2117, -27409, -12011 }, { -14818, -19443, 17390 }, { 4986, -25870, -14348 }, { 26415, -13505, -4454 }, { 938, -25371, 15982 }, { -23271, -17022, -8290 }, { -1950, -28244, 9922 }, { -13200, 8409, -14222 }, { 17079, -7565, 17866 }, { 21473, -13087, 12278 }, { -3709, -16091, -25047 }, { 14461, -12054, -23358 } },
    { { 3791, -18343, 23435 }, { -489, -27628, -11682 }, { -15790, -20985, 14502 }, { 5562, -26781, -12323 }, { 25743, -14741, -4472 }, { 1967, -25351, 15921 }, { -22761, -16574, -10356 }, { -2682, -28593, 8676 }, { -12228, 7890, -13790 }, { 16363, -7052, 18070 }, { 20235, -12711, 13214 }, { -6097, -15894, -24702 }, { 17753, -12741, -20555 } },
    { { 1841, -18757, 23341 }, { 1221, -27687, -11487 }, { -16675, -22059, 11634 }, { 5909, -27413, -10658 }, { 24495, -16558, -5082 }, { 2752, -24735, 16751 }, { -22117, -16202, -12179 }, { -3681, -28691, 7953 }, { -11395, 7409, -13459 }, { 15801, -6534, 18138 }, { 19342, -12221, 13846 }, { -8397, -16126, -23863 }, { 19945, -12971, -18275 } },
    { { -1392, -19409, 22834 }, { 2861, -28083, -10156 }, { -16856, -22613, 10223 }, { 5971, -27163, -11247 }, { 22604, -18729, -6187 }, { 3725, -23469, 18312 }, { -21221, -15868, -14067 }, { -5079, -28477, 7953 }, { -10854, 6921, -13291 }, { 15356, -5969, 18052 }, { 18713, -11549, 14192 }, { -10916, -17099, -22101 }, { 21558, -12576, -16646 } },
    { { -4423, -20494, 21458 }, { 3831, -28924, -6981 }, { -15731, -23737, 9439 }, { 6292, -25776, -14001 }, { 20364, -20830, -7172 }, { 4946, -21714, 20101 }, { -19553, -15660, -16506 }, { -7369, -27736, 8742 }, { -10642, 6341, -13329 }, { 15067, -5286, 17835 }, { 18298, -10697, 14261 }, { -13571, -19188, -18645 }, { 23481, -11989, -14315 } },
    { { -6346, -21442, 20000 }, { 4168, -29365, -4509 }, { -13729, -25726, 7048 }, { 7113, -24235, -16189 }, { 18102, -22673, -7632 }, { 6295, -19853, 21592 }, { -16918, -15819, -19066 }, { -10717, -26447, 9258 }, { -10601, 5708, -13592 }, { 14943, -4532, 17509 }, { 18030, -9818, 14146 }, { -15963, -21387, -13703 }, { 25800, -11331, -10294 } },
    { { -7255, -22102, 18944 }, { 4188, -29580, -2741 }, { -11676, -27338, 4037 }, { 8236, -23282, -17033 }, { 15960, -24279, -7470 }, { 7697, -18522, 22309 }, { -13939, -16869, -20521 }, { -14425, -25016, 8131 }, { -10533, 5119, -13905 }, { 14842, -3857, 17158 }, { 17793, -9079, 14002 }, { -17687, -22546, -8881 }, { 27529, -10700, -5260 } },
    { { -7555, -21954, 18999 }, { 5100, -29391, -3183 }, { -10501, -28020, 2142 }, { 9006, -23182, -16777 }, { 14350, -25422, -6914 }, { 8547, -18227, 22243 }, { -12009, -18407, -20419 }, { -17008, -24109, 5429 }, { -10283, 4593, -14131 }, { 14632, -3336, 16880 }, { 17529, -8518, 13923 }, { -18719, -22774, -5565 }, { 28039, -10616, -1040 } },
    { { -7850, -22042, 18776 }, { 5954, -29195, -3494 }, { -10315, -28164, 621 }, { 9400, -23712, -15792 }, { 13475, -25975, -6614 }, { 8619, -18901, 21644 }, { -11725, -19470, -19582 }, { -17616, -24172, 2325 }, { -9868, 4093, -14244 }, { 14287, -2928, 16728 }, { 17251, -8113, 13926 }, { -19198, -22672, -4174 }, { 27967, -10752, 1502 } },
};
// clang-format on
// clang-format off
static const Vec3s sGdfClipLandRoot[47] = {
    { 0, 0, 0 },       { 78, 0, 112 },    { 91, 0, 288 },    { 68, 0, 501 },    { 46, 0, 711 },    { 56, 0, 895 },
    { 51, 0, 1048 },   { 10, 0, 1170 },   { -51, 0, 1281 },  { -102, 0, 1393 }, { -136, 0, 1499 }, { -158, 0, 1591 },
    { -177, 0, 1673 }, { -195, 0, 1748 }, { -208, 0, 1810 }, { -216, 0, 1856 }, { -222, 0, 1897 }, { -228, 0, 1919 },
    { -229, 0, 1915 }, { -228, 0, 1889 }, { -225, 0, 1844 }, { -220, 0, 1781 }, { -210, 0, 1694 }, { -191, 0, 1597 },
    { -163, 0, 1491 }, { -128, 0, 1376 }, { -87, 0, 1260 },  { -39, 0, 1155 },  { 15, 0, 1059 },   { 69, 0, 975 },
    { 111, 0, 905 },   { 140, 0, 845 },   { 158, 0, 793 },   { 167, 0, 752 },   { 172, 0, 719 },   { 180, 0, 691 },
    { 184, 0, 660 },   { 182, 0, 625 },   { 170, 0, 585 },   { 141, 0, 569 },   { 141, 0, 569 },   { 141, 0, 569 },
    { 141, 0, 569 },   { 141, 0, 569 },   { 141, 0, 569 },   { 141, 0, 569 },   { 141, 0, 569 },
};
// clang-format on
#define GDF_LAND_RATE 1.25f      // source frames a game frame (its own pace is 1.5: a little heavier)
#define GDF_LAND_FADE_FROM 36.0f // (its frame 40: he's half up) to the end, back into his stance

void BossGanon_Player_SetupLand(BossGanon* this, PlayState* play, f32 height) {
    // Play as Ganondorf (2S2H): not ported yet
}

// Down into a deep crouch, a moment, then he rises
void BossGanon_Player_Land(BossGanon* this, PlayState* play) {
    // Play as Ganondorf (2S2H): not ported yet
}

// ---- Stance and attacks ------------------------------------------------------------------------

void BossGanon_Player_SetupSwordIdle(BossGanon* this, PlayState* play) {
    // Play as Ganondorf (2S2H): not ported yet
}

// Stick direction in the world (camera-relative), or -1 when the stick is neutral

// D-Pad Left with the sword out or away, from his normal moveset
static s32 BossGanon_Player_TryToggleSword(BossGanon* this, PlayState* play) {
    // Play as Ganondorf (2S2H): not ported yet
    return 0;
}

void BossGanon_Player_SwordIdle(BossGanon* this, PlayState* play) {
    // Play as Ganondorf (2S2H): not ported yet
}

// The ground slam: a quake, dust and a shockwave that hits everything around the impact

void BossGanon_Player_SwordAttack(BossGanon* this, PlayState* play) {
    // Play as Ganondorf (2S2H): not ported yet
}

#define GDF_DASH_FRAMES 9

// A quick dash; B during it turns into a lunging thrust
void BossGanon_Player_SwordDash(BossGanon* this, PlayState* play) {
    // Play as Ganondorf (2S2H): not ported yet
}

// Used by PlayAsGanondorf.cpp to move Link along with the dash and the thrust

// ---- Drawing and hits --------------------------------------------------------------------------

// ---- Phantom Ganon skin ----
// Phantom Ganon's skeleton has the same build as Ganondorf's (bones along +x, arms 3-12, legs 17-24), so each of his
// limbs simply takes the world rotation of the matching Ganondorf limb. Only the hands are modelled differently
// (his point along +y, with the spear through the fist along +x), so they get a fixed turn that lines his spear up
// with where Ganondorf's sword blade would point. His legs are shorter: he's drawn bigger to stand as tall.
#define GDF_PG_SCALE (5132.0f / 4133.0f)
#define GDF_SPEAR_SCALE 0.75f
#define GDF_SPEAR_BASE_Z 1500.0f
#define GDF_SPEAR_TIP_Z 8520.0f

// Ganondorf limb for each Phantom Ganon limb, and Phantom Ganon's limb parents
static const s8 sGdfPgSource[25] = { 0, 1,  1,  3,  3,  4,  5,  7,  7,  8,  9,  9, 9,
                                     1, 10, 18, 18, 19, 19, 20, 21, 22, 22, 23, 24 };
static const s8 sGdfPgParent[25] = { -1, 0, 1, 2,  3,  4,  5,  2,  7,  8,  9,  10, 11,
                                     2,  2, 0, 15, 16, 17, 18, 19, 16, 21, 22, 23 };
// Row-major turns from his hand space into Ganondorf's (right hand: spear tip to the blade's -z; left: mirrored)
static const f32 sGdfPgHandR[3][3] = { { 0, 1, 0 }, { 0, 0, -1 }, { -1, 0, 0 } };
static const f32 sGdfPgHandL[3][3] = { { 0, 1, 0 }, { 0, 0, 1 }, { 1, 0, 0 } };
// His feet point along their local +x (sole toward +y), Ganondorf's along +z (sole toward -y): turn them so his toes
// face where Ganondorf's do. Without it both feet pointed to his left.
static const f32 sGdfPgFoot[3][3] = { { 0, 0, 1 }, { 0, -1, 0 }, { 1, 0, 0 } };
// The spear in his hand, as his own animations always hold it
static const Vec3s sGdfPgSpearRot = { 16286, -16384, 16286 };

s32 BossGanon_Player_IsPhantom(void) {
    // Play as Ganondorf (2S2H): not ported yet
    return 0;
}

// A trident in his hands (the weapon menu): the spear's moveset (the combo ends in a stab) and no sheath

typedef f32 GdfMat3[3][3];

static void BossGanon_Pg_FromMtxF(MtxF* mf, GdfMat3 out, f32* scale) {
    // Play as Ganondorf (2S2H): not ported yet
}

static void BossGanon_Pg_Mul(const GdfMat3 a, const GdfMat3 b, GdfMat3 out, s32 transposeA) {
    // Play as Ganondorf (2S2H): not ported yet
}

// R = Rz * Ry * Rx (the skeleton's rotation order)
static void BossGanon_Pg_FromRot(Vec3s* rot, GdfMat3 out) {
    // Play as Ganondorf (2S2H): not ported yet
}

static void BossGanon_Pg_ToRot(GdfMat3 m, Vec3s* rot) {
    // Play as Ganondorf (2S2H): not ported yet
}

// His hair: the running clips pitch his head, and the hair (hanging from it) swung in through the back of his armour's
// collar. Its upper half is turned to hang as it does in his idle stance, seen from his torso (down, and back off his
// shoulders), streaming further back the faster he runs and swaying a little with his strides; the player's lags
// behind a little, like hair. The lower half follows it as it always did.
static void BossGanon_Player_HairOverride(BossGanon* this, Vec3s* rot) {
    static Vec3f sLag = { -0.83f, -0.55f, -0.12f };
    MtxF parentMtx;
    GdfMat3 P;
    GdfMat3 T;
    GdfMat3 R0;
    GdfMat3 Q;
    GdfMat3 Rn;
    f32 want[3];
    f32 w[3];
    f32 l[3];
    f32 c[3];
    f32 v[3];
    f32 len;
    f32 cc;
    f32 k;
    f32 run = 0.0f;
    s32 i;
    s32 j;

    if (this == sPlayerGanondorf) {
        run = CLAMP(sGdfWalkSpeed / 8.0f, 0.0f, 1.0f) * this->playerWalkWeight;
    }
    want[0] = -0.83f;
    want[1] = -0.55f - 0.35f * run;
    want[2] = -0.12f + 0.08f * run * Math_SinS((s16)(this->playerWalkFrame * (0x10000 / 30.0f)));
    if (this == sPlayerGanondorf) {
        sLag.x += (want[0] - sLag.x) * 0.25f;
        sLag.y += (want[1] - sLag.y) * 0.25f;
        sLag.z += (want[2] - sLag.z) * 0.25f;
        want[0] = sLag.x;
        want[1] = sLag.y;
        want[2] = sLag.z;
    }
    len = sqrtf(SQ(want[0]) + SQ(want[1]) + SQ(want[2]));
    for (i = 0; i < 3; i++) {
        want[i] /= len;
    }

    // That way in the world (from his torso), then in his head's own space
    Matrix_Get(&parentMtx);
    BossGanon_Pg_FromMtxF(&parentMtx, P, NULL);
    BossGanon_Pg_FromMtxF(&sGdfHairTorso, T, NULL);
    for (i = 0; i < 3; i++) {
        w[i] = T[i][0] * want[0] + T[i][1] * want[1] + T[i][2] * want[2];
    }
    for (i = 0; i < 3; i++) {
        l[i] = P[0][i] * w[0] + P[1][i] * w[1] + P[2][i] * w[2];
    }

    // The least turn that takes the hair's way (its local x) there, keeping its own twist
    BossGanon_Pg_FromRot(rot, R0);
    for (i = 0; i < 3; i++) {
        c[i] = R0[i][0];
    }
    v[0] = c[1] * l[2] - c[2] * l[1];
    v[1] = c[2] * l[0] - c[0] * l[2];
    v[2] = c[0] * l[1] - c[1] * l[0];
    cc = c[0] * l[0] + c[1] * l[1] + c[2] * l[2];
    if (cc < -0.95f) {
        return; // (turned right round: something's off, leave it)
    }
    k = 1.0f / (1.0f + cc);
    {
        f32 V[3][3] = { { 0.0f, -v[2], v[1] }, { v[2], 0.0f, -v[0] }, { -v[1], v[0], 0.0f } };

        for (i = 0; i < 3; i++) {
            for (j = 0; j < 3; j++) {
                Q[i][j] = ((i == j) ? 1.0f : 0.0f) + V[i][j] +
                          k * (V[i][0] * V[0][j] + V[i][1] * V[1][j] + V[i][2] * V[2][j]);
            }
        }
    }
    BossGanon_Pg_Mul(Q, R0, Rn, false);
    BossGanon_Pg_ToRot(Rn, rot);
}

static void BossGanon_Player_DrawPhantom(BossGanon* this, PlayState* play, MtxF* base) {
    // Play as Ganondorf (2S2H): not ported yet
}

// Door kick: while Link opens a door, Ganondorf raises his right leg straight out in front (the whole leg turns about
// the hip) and leans back, timed so the foot lands on the frame the door flies off (z_en_door.c)
#define GDF_KICK_LIFT_DEG 80.0f

// Raises his right leg straight out by `deg` (scaled by the weight `w`, 0 to 1) and leans back

// ---- No door holds him ----
// Locked doors (small key or boss key), chained doors, barred doors waiting on a switch or a cleared room, the
// Gerudo cells, gates, grates and breakable walls: A in front of one and he kicks it in. A door he unlocks or unbars
// (and then kicks open as he goes through, like any door); a cell, gate, grate or wall he smashes to pieces.
#define GDF_BREAK_KICK_FRAMES 22
#define GDF_BREAK_KICK_IMPACT 8
#define GDF_BREAK_RANGE 95.0f

// The Door of Time's own opening scene (z_en_okarina_tag_cutscene_data.c), as the Song of Time plays it
extern CutsceneData D_80ABFB40[];

// Shut to him: locked, chained or barred (and so not opened the normal way)

// A in front of a shut door (from the input filter, before Link reads A): the kick starts

static void BossGanon_Player_ApplyBreakKick(BossGanon* this, PlayState* play) {
    // Play as Ganondorf (2S2H): not ported yet
}

// Chests, opened his way. Small ones (also a small chest opened slowly, for something new) he boots open: Mixamo's
// "Kick Soccerball", his left foot meeting the lid as it bursts open (z_en_box.c, EnBox_GdfKickFrame). Big ones and
// boss key chests he bends down to, grips and heaves the lid up over his head: "Opening A Lid", lifting it as it
// cracks open and pushing it up and over as it swings open. Both at the clips' own speed (a chest's animation runs
// 1.5 frames a game frame, as the clips' 30 a second do). His root follows each clip's hips (sGdfClip*Root, its
// planted feet's offset from the root, the other way), so his feet stay where they stand while his knees bend.
// Rows as the other clips; the lid's are its source's frames 20 to 200.
// clang-format off
static const Vec3s sGdfClipKick[22][13] = {
    { { 3263, -29317, 5468 }, { 1129, -28086, -10483 }, { 311, -28901, 8041 }, { 4796, -28208, -9015 }, { 18351, -19551, -13453 }, { 16949, -24657, 2178 }, { -14325, -25176, -7809 }, { -11339, -27564, 3412 }, { -16318, 4705, -15497 }, { 16428, -2364, 16931 }, { 16663, -14356, 16157 }, { 25737, 13590, 7274 }, { -26456, 10868, 9054 } },
    { { 3434, -28524, 8637 }, { 1195, -27321, -12335 }, { 159, -28738, 8609 }, { 5084, -27999, -9497 }, { 18320, -19314, -13833 }, { 17021, -24653, 1597 }, { -14292, -25293, -7484 }, { -10970, -27604, 4202 }, { -16155, 4368, -15512 }, { 16420, -2205, 16969 }, { 16626, -14203, 16358 }, { 25850, 13183, 7614 }, { -26421, 10470, 9609 } },
    { { 3600, -26799, 12994 }, { 957, -26529, -13975 }, { 163, -28567, 9159 }, { 5306, -27735, -10130 }, { 18375, -19396, -13645 }, { 17177, -24552, 1466 }, { -14261, -25476, -6899 }, { -10529, -27608, 5190 }, { -16155, 4007, -15582 }, { 16527, -2013, 16974 }, { 16679, -13990, 16363 }, { 25888, 12899, 7965 }, { -26238, 10362, 10208 } },
    { { 3860, -24116, 17422 }, { 366, -26700, -13673 }, { 327, -28354, 9795 }, { 5390, -27292, -11228 }, { 18262, -19567, -13551 }, { 17402, -24406, 1240 }, { -14291, -25671, -6065 }, { -10241, -27477, 6335 }, { -16364, 3614, -15681 }, { 16708, -1765, 16935 }, { 16756, -13694, 16206 }, { 25924, 12898, 7846 }, { -25972, 10813, 10418 } },
    { { 4254, -21457, 20530 }, { 348, -27448, -12103 }, { 410, -28073, 10570 }, { 5388, -26602, -12780 }, { 17896, -19830, -13656 }, { 17487, -24366, 728 }, { -14301, -25896, -4986 }, { -10175, -27197, 7535 }, { -16567, 3198, -15660 }, { 16885, -1462, 16893 }, { 16879, -13284, 15954 }, { 26059, 13179, 6874 }, { -25749, 11661, 10050 } },
    { { 4219, -19073, 22769 }, { 521, -28898, -8041 }, { 379, -27762, 11362 }, { 5390, -25862, -14217 }, { 17415, -20233, -13687 }, { 17581, -24308, 16 }, { -14244, -26117, -3876 }, { -10196, -26909, 8483 }, { -16663, 2789, -15574 }, { 17020, -1220, 16860 }, { 16917, -12928, 15779 }, { 26137, 13801, 5135 }, { -25573, 12355, 9663 } },
    { { 3980, -17073, 24345 }, { 720, -29977, -934 }, { 292, -27468, 12059 }, { 5498, -25328, -15109 }, { 16893, -20514, -13922 }, { 17893, -24057, -1041 }, { -13998, -26420, -2459 }, { -10035, -26679, 9355 }, { -16652, 2519, -15650 }, { 17097, -1165, 16835 }, { 16843, -12850, 15808 }, { 26295, 14101, 3121 }, { -25449, 12562, 9724 } },
    { { 3818, -15833, 25194 }, { 1404, -29149, 6954 }, { 395, -27296, 12441 }, { 5441, -24989, -15682 }, { 16377, -20817, -14087 }, { 17979, -23905, -2302 }, { -13894, -26553, -1372 }, { -9903, -26534, 9893 }, { -16751, 2375, -15708 }, { 17142, -1166, 16799 }, { 16693, -12851, 15883 }, { 26372, 14084, 2478 }, { -25245, 12506, 10309 } },
    { { 3711, -14865, 25793 }, { 2553, -25424, 15720 }, { 722, -27219, 12594 }, { 5080, -24754, -16168 }, { 16032, -21241, -13849 }, { 17818, -23950, -2990 }, { -13836, -26615, -450 }, { -9471, -26534, 10308 }, { -17028, 2213, -15729 }, { 17368, -1155, 16768 }, { 17086, -12879, 15442 }, { 26606, 13674, 2265 }, { -24775, 12219, 11701 } },
    { { 3695, -13813, 26373 }, { 3454, -19976, 22114 }, { 936, -27245, 12523 }, { 4814, -24674, -16372 }, { 16068, -21471, -13447 }, { 17601, -24163, -2521 }, { -13781, -26646, 297 }, { -9045, -26589, 10547 }, { -17215, 2253, -15776 }, { 17539, -1172, 16738 }, { 17310, -13045, 15128 }, { 26624, 13587, 2566 }, { -24261, 11904, 13026 } },
    { { 3796, -12897, 26819 }, { 3822, -17673, 23939 }, { 1054, -27320, 12349 }, { 4632, -24607, -16524 }, { 16002, -21597, -13323 }, { 17354, -24385, -2052 }, { -13776, -26638, 815 }, { -8762, -26682, 10549 }, { -17338, 2388, -15850 }, { 17673, -1218, 16699 }, { 17519, -13302, 14814 }, { 26454, 13975, 2211 }, { -23769, 11558, 14193 } },
    { { 3915, -11996, 27217 }, { 3562, -16548, 24769 }, { 1110, -27440, 12076 }, { 4507, -24618, -16542 }, { 15647, -21739, -13512 }, { 17064, -24566, -2305 }, { -13964, -26531, 1043 }, { -8736, -26773, 10339 }, { -17415, 2575, -15912 }, { 17815, -1348, 16620 }, { 17909, -13616, 14279 }, { 26506, 13970, 1506 }, { -23281, 11228, 15229 } },
    { { 3996, -11111, 27578 }, { 2999, -17133, 24443 }, { 1129, -27607, 11687 }, { 4414, -24709, -16432 }, { 15277, -21877, -13712 }, { 16835, -24680, -2737 }, { -14313, -26347, 989 }, { -8857, -26811, 10135 }, { -17456, 2757, -15934 }, { 17977, -1503, 16526 }, { 18554, -13907, 13485 }, { 26801, 13451, 887 }, { -22917, 11011, 15924 } },
    { { 4139, -10893, 27644 }, { 2160, -20891, 21422 }, { 1093, -27840, 11124 }, { 4346, -24915, -16136 }, { 15171, -21962, -13694 }, { 16797, -24685, -2917 }, { -14722, -26133, 594 }, { -9014, -26810, 9998 }, { -17451, 2901, -15890 }, { 18109, -1623, 16472 }, { 19273, -14144, 12665 }, { 27076, 12902, 656 }, { -22856, 10976, 16035 } },
    { { 4301, -11043, 27560 }, { 1345, -24561, 17174 }, { 986, -28139, 10354 }, { 4350, -25298, -15527 }, { 15461, -21865, -13524 }, { 16831, -24724, -2330 }, { -15149, -25892, -371 }, { -9278, -26826, 9708 }, { -17366, 3031, -15773 }, { 18125, -1710, 16485 }, { 19815, -14372, 12077 }, { 27144, 12694, 1436 }, { -23223, 11132, 15388 } },
    { { 4485, -12232, 27023 }, { 698, -27311, 12394 }, { 822, -28376, 9701 }, { 4450, -25710, -14806 }, { 15754, -21664, -13508 }, { 16855, -24755, -1763 }, { -15597, -25565, -1776 }, { -9816, -26841, 9122 }, { -17205, 3216, -15630 }, { 18017, -1776, 16516 }, { 20296, -14601, 11582 }, { 27103, 12595, 2605 }, { -23800, 11443, 14234 } },
    { { 4574, -14461, 25884 }, { 251, -29146, 7103 }, { 548, -28590, 9072 }, { 4649, -26216, -13825 }, { 16109, -21399, -13511 }, { 16878, -24774, -1188 }, { -15794, -25289, -3317 }, { -10418, -26897, 8247 }, { -16917, 3469, -15483 }, { 17796, -1835, 16579 }, { 20486, -14789, 11441 }, { 26971, 12621, 3643 }, { -24455, 11673, 12871 } },
    { { 4494, -17489, 23957 }, { 141, -29984, 984 }, { 268, -28735, 8616 }, { 4820, -26721, -12757 }, { 16532, -21092, -13485 }, { 16959, -24739, -620 }, { -15617, -25168, -4761 }, { -10901, -27054, 7017 }, { -16601, 3753, -15392 }, { 17497, -1903, 16670 }, { 20223, -14940, 11865 }, { 26921, 12494, 4375 }, { -25084, 11633, 11638 } },
    { { 4374, -21173, 20798 }, { 183, -29591, -4932 }, { 109, -28812, 8359 }, { 4917, -27139, -11802 }, { 17018, -20762, -13391 }, { 16982, -24731, 125 }, { -15270, -25133, -5927 }, { -11241, -27255, 5549 }, { -16376, 3958, -15368 }, { 17212, -1974, 16741 }, { 19718, -15019, 12566 }, { 26807, 12415, 5219 }, { -25544, 11455, 10783 } },
    { { 3962, -25658, 15032 }, { 453, -28152, -10356 }, { 90, -28834, 8282 }, { 4929, -27466, -11013 }, { 17519, -20434, -13249 }, { 16971, -24724, 857 }, { -14887, -25201, -6577 }, { -11328, -27410, 4512 }, { -16299, 4156, -15427 }, { 16965, -2062, 16800 }, { 18668, -14889, 13810 }, { 26544, 12566, 6125 }, { -25866, 11233, 10236 } },
    { { 3395, -28744, 7889 }, { 945, -27193, -12635 }, { 148, -28862, 8185 }, { 4896, -27821, -10102 }, { 17957, -19986, -13346 }, { 16930, -24712, 1652 }, { -14601, -25221, -7122 }, { -11321, -27486, 4042 }, { -16271, 4400, -15483 }, { 16713, -2185, 16861 }, { 17535, -14611, 15124 }, { 26160, 12963, 6898 }, { -26171, 11057, 9633 } },
    { { 3263, -29317, 5468 }, { 1129, -28086, -10483 }, { 310, -28901, 8041 }, { 4802, -28208, -9014 }, { 18351, -19551, -13453 }, { 16949, -24657, 2178 }, { -14325, -25176, -7809 }, { -11339, -27564, 3412 }, { -16318, 4705, -15497 }, { 16428, -2364, 16931 }, { 16663, -14356, 16157 }, { 25737, 13590, 7274 }, { -26456, 10868, 9054 } },
};
// clang-format on
// clang-format off
static const Vec3s sGdfClipKickRoot[22] = {
    { 0, 0, 0 },        { -11, -21, -35 },  { -23, -47, -68 },  { -35, -90, -84 },  { -47, -156, -75 },
    { -58, -230, -71 }, { -51, -300, -82 }, { -48, -343, -87 }, { -44, -365, -96 }, { -34, -371, -79 },
    { -19, -376, -41 }, { -5, -370, 3 },    { 4, -352, 46 },    { 7, -315, 85 },    { 2, -254, 116 },
    { -10, -197, 139 }, { -20, -139, 153 }, { -19, -93, 151 },  { -15, -61, 127 },  { -5, -43, 92 },
    { 2, -24, 51 },     { 0, 0, 0 },
};
// clang-format on
// clang-format off
static const Vec3s sGdfClipLid[181][13] = {
    { { 2001, -29078, 7104 }, { -227, -29264, -6602 }, { -2201, -29241, 6333 }, { -547, -29373, -6078 }, { 6918, -27002, 11093 }, { 1119, -26999, 13030 }, { -9673, -26509, 10183 }, { -453, -25746, 15392 }, { -16401, 2838, -16638 }, { 15993, 1205, 16315 }, { -15746, -11927, -17210 }, { 3456, 13457, 26589 }, { -11890, 14277, 23554 } },
    { { 2017, -29069, 7137 }, { -238, -29268, -6583 }, { -2200, -29233, 6372 }, { -545, -29376, -6065 }, { 6926, -27042, 10989 }, { 1091, -27047, 12933 }, { -9783, -26515, 10062 }, { -554, -25819, 15267 }, { -16401, 2844, -16638 }, { 15995, 1165, 16309 }, { -15742, -11712, -17220 }, { 3432, 13789, 26421 }, { -11642, 14414, 23594 } },
    { { 2027, -29058, 7178 }, { -243, -29273, -6562 }, { -2199, -29223, 6417 }, { -544, -29379, -6047 }, { 6934, -27085, 10876 }, { 1064, -27098, 12828 }, { -9916, -26522, 9912 }, { -677, -25908, 15109 }, { -16400, 2853, -16638 }, { 15994, 1112, 16303 }, { -15739, -11529, -17223 }, { 3306, 14304, 26162 }, { -11300, 14607, 23642 } },
    { { 2025, -29045, 7230 }, { -237, -29278, -6540 }, { -2195, -29212, 6468 }, { -547, -29384, -6024 }, { 6937, -27130, 10764 }, { 1034, -27151, 12718 }, { -10047, -26534, 9748 }, { -806, -26005, 14936 }, { -16400, 2860, -16639 }, { 15994, 1043, 16296 }, { -15735, -11359, -17225 }, { 3107, 15049, 25766 }, { -10809, 14904, 23686 } },
    { { 2009, -29031, 7292 }, { -219, -29284, -6510 }, { -2186, -29199, 6529 }, { -556, -29390, -5992 }, { 6957, -27170, 10649 }, { 966, -27201, 12615 }, { -10190, -26544, 9569 }, { -967, -26111, 14740 }, { -16399, 2864, -16640 }, { 15990, 955, 16288 }, { -15723, -11196, -17235 }, { 2918, 15982, 25220 }, { -10101, 15349, 23714 } },
    { { 1985, -29012, 7374 }, { -195, -29293, -6472 }, { -2176, -29180, 6619 }, { -568, -29396, -5962 }, { 7051, -27200, 10508 }, { 805, -27245, 12533 }, { -10376, -26547, 9358 }, { -1161, -26234, 14506 }, { -16398, 2868, -16636 }, { 15976, 843, 16280 }, { -15705, -11031, -17252 }, { 2793, 16986, 24570 }, { -9152, 15947, 23705 } },
    { { 1969, -28978, 7511 }, { -181, -29301, -6436 }, { -2168, -29144, 6778 }, { -579, -29399, -5947 }, { 7252, -27219, 10323 }, { 534, -27283, 12464 }, { -10619, -26544, 9089 }, { -1338, -26382, 14220 }, { -16399, 2874, -16630 }, { 15957, 698, 16273 }, { -15693, -10849, -17266 }, { 2677, 17927, 23905 }, { -8022, 16636, 23641 } },
    { { 1974, -28921, 7724 }, { -185, -29309, -6398 }, { -2158, -29089, 7011 }, { -591, -29400, -5940 }, { 7525, -27225, 10107 }, { 200, -27321, 12391 }, { -10897, -26542, 8762 }, { -1470, -26552, 13886 }, { -16409, 2888, -16629 }, { 15941, 508, 16270 }, { -15697, -10639, -17268 }, { 2445, 18751, 23290 }, { -6852, 17339, 23503 } },
    { { 2011, -28832, 8041 }, { -213, -29313, -6379 }, { -2152, -29010, 7332 }, { -592, -29399, -5947 }, { 7753, -27224, 9937 }, { -69, -27365, 12295 }, { -11173, -26549, 8386 }, { -1604, -26734, 13517 }, { -16433, 2911, -16640 }, { 15931, 267, 16274 }, { -15712, -10401, -17260 }, { 2079, 19530, 22677 }, { -5761, 18031, 23275 } },
    { { 2076, -28703, 8474 }, { -260, -29309, -6397 }, { -2167, -28902, 7744 }, { -567, -29392, -5982 }, { 7793, -27239, 9865 }, { -127, -27437, 12133 }, { -11432, -26572, 7951 }, { -1787, -26916, 13127 }, { -16472, 2941, -16669 }, { 15919, -23, 16285 }, { -15727, -10149, -17249 }, { 1765, 20328, 21992 }, { -4764, 18703, 22968 } },
    { { 2146, -28532, 9019 }, { -308, -29294, -6464 }, { -2207, -28759, 8249 }, { -513, -29377, -6060 }, { 7612, -27306, 9822 }, { 53, -27565, 11839 }, { -11710, -26606, 7416 }, { -1997, -27097, 12718 }, { -16522, 2978, -16710 }, { 15894, -359, 16299 }, { -15732, -9887, -17238 }, { 1641, 21098, 21265 }, { -3769, 19332, 22629 } },
    { { 2204, -28316, 9662 }, { -346, -29267, -6583 }, { -2263, -28574, 8856 }, { -446, -29350, -6195 }, { 7337, -27438, 9661 }, { 328, -27753, 11387 }, { -12081, -26619, 6745 }, { -2159, -27283, 12287 }, { -16575, 3027, -16755 }, { 15855, -738, 16311 }, { -15723, -9599, -17232 }, { 1627, 21708, 20643 }, { -2668, 19877, 22311 } },
    { { 2244, -28060, 10372 }, { -381, -29232, -6734 }, { -2326, -28341, 9560 }, { -382, -29312, -6375 }, { 7167, -27596, 9332 }, { 506, -27964, 10851 }, { -12583, -26573, 5962 }, { -2222, -27479, 11830 }, { -16611, 3100, -16786 }, { 15806, -1158, 16317 }, { -15703, -9262, -17235 }, { 1506, 22092, 20241 }, { -1449, 20308, 22033 } },
    { { 2286, -27776, 11103 }, { -435, -29195, -6889 }, { -2411, -28064, 10325 }, { -311, -29271, -6564 }, { 7189, -27720, 8941 }, { 554, -28150, 10358 }, { -13124, -26476, 5175 }, { -2248, -27685, 11335 }, { -16616, 3207, -16794 }, { 15753, -1607, 16322 }, { -15682, -8874, -17242 }, { 1254, 22335, 19990 }, { -265, 20650, 21760 } },
    { { 2347, -27472, 11822 }, { -524, -29163, -7018 }, { -2536, -27758, 11094 }, { -212, -29236, -6725 }, { 7377, -27784, 8580 }, { 541, -28290, 9970 }, { -13555, -26382, 4502 }, { -2342, -27890, 10800 }, { -16601, 3350, -16787 }, { 15700, -2074, 16338 }, { -15677, -8458, -17235 }, { 994, 22541, 19771 }, { 672, 20992, 21422 } },
    { { 2431, -27154, 12520 }, { -640, -29137, -7114 }, { -2694, -27438, 11828 }, { -84, -29208, -6847 }, { 7631, -27813, 8258 }, { 503, -28402, 9646 }, { -13791, -26336, 4031 }, { -2572, -28075, 10255 }, { -16595, 3518, -16786 }, { 15656, -2545, 16370 }, { -15699, -8039, -17202 }, { 770, 22791, 19493 }, { 1258, 21487, 20897 } },
    { { 2521, -26816, 13211 }, { -762, -29111, -7208 }, { -2846, -27110, 12529 }, { 38, -29178, -6975 }, { 7874, -27837, 7943 }, { 412, -28509, 9331 }, { -13808, -26360, 3806 }, { -2952, -28228, 9719 }, { -16624, 3694, -16804 }, { 15630, -3005, 16420 }, { -15749, -7618, -17145 }, { 464, 23109, 19124 }, { 1542, 22231, 20085 } },
    { { 2616, -26451, 13910 }, { -883, -29071, -7356 }, { -2955, -26762, 13232 }, { 120, -29127, -7185 }, { 8095, -27864, 7622 }, { 257, -28618, 8998 }, { -13640, -26456, 3750 }, { -3452, -28354, 9172 }, { -16687, 3870, -16837 }, { 15616, -3440, 16475 }, { -15808, -7174, -17082 }, { -47, 23453, 18706 }, { 1684, 23107, 19059 } },
    { { 2735, -26058, 14611 }, { -1020, -29006, -7591 }, { -3022, -26384, 13954 }, { 167, -29045, -7508 }, { 8308, -27887, 7302 }, { 87, -28724, 8657 }, { -13447, -26563, 3688 }, { -3962, -28455, 8639 }, { -16758, 4040, -16868 }, { 15599, -3842, 16525 }, { -15843, -6704, -17039 }, { -699, 23740, 18328 }, { 1892, 23882, 18057 } },
    { { 2896, -25657, 15275 }, { -1188, -28922, -7882 }, { -3093, -25988, 14664 }, { 227, -28942, -7893 }, { 8526, -27897, 7007 }, { -53, -28817, 8343 }, { -13417, -26601, 3519 }, { -4352, -28524, 8214 }, { -16813, 4193, -16882 }, { 15566, -4206, 16566 }, { -15838, -6238, -17023 }, { -1347, 23920, 18056 }, { 2276, 24443, 17243 } },
    { { 3074, -25273, 15870 }, { -1369, -28835, -8167 }, { -3206, -25598, 15313 }, { 331, -28839, -8257 }, { 8766, -27886, 6746 }, { -150, -28893, 8073 }, { -13578, -26548, 3296 }, { -4583, -28560, 7958 }, { -16844, 4316, -16879 }, { 15522, -4531, 16602 }, { -15808, -5827, -17023 }, { -1910, 24026, 17863 }, { 2732, 24857, 16574 } },
    { { 3227, -24918, 16392 }, { -1524, -28756, -8414 }, { -3341, -25233, 15879 }, { 458, -28748, -8564 }, { 9033, -27863, 6485 }, { -233, -28964, 7812 }, { -13781, -26467, 3097 }, { -4667, -28588, 7805 }, { -16863, 4412, -16870 }, { 15488, -4832, 16639 }, { -15782, -5492, -17027 }, { -2340, 24130, 17671 }, { 3038, 25259, 15898 } },
    { { 3337, -24598, 16847 }, { -1635, -28690, -8616 }, { -3435, -24904, 16370 }, { 545, -28669, -8821 }, { 9325, -27834, 6190 }, { -321, -29042, 7516 }, { -13887, -26432, 2919 }, { -4576, -28646, 7647 }, { -16885, 4503, -16862 }, { 15478, -5129, 16677 }, { -15780, -5211, -17028 }, { -2579, 24293, 17412 }, { 3031, 25727, 15130 } },
    { { 3415, -24326, 17222 }, { -1716, -28638, -8771 }, { -3462, -24620, 16789 }, { 566, -28602, -9035 }, { 9625, -27795, 5899 }, { -432, -29119, 7206 }, { -13905, -26444, 2713 }, { -4305, -28742, 7442 }, { -16902, 4598, -16848 }, { 15481, -5419, 16721 }, { -15792, -4959, -17024 }, { -2547, 24541, 17066 }, { 2753, 26205, 14344 } },
    { { 3474, -24115, 17505 }, { -1787, -28604, -8866 }, { -3471, -24387, 17124 }, { 560, -28553, -9186 }, { 9923, -27735, 5683 }, { -564, -29181, 6940 }, { -13899, -26472, 2463 }, { -3917, -28856, 7210 }, { -16907, 4697, -16820 }, { 15482, -5692, 16771 }, { -15803, -4721, -17008 }, { -2258, 24853, 16651 }, { 2362, 26616, 13639 } },
    { { 3525, -23954, 17714 }, { -1860, -28588, -8902 }, { -3523, -24201, 17375 }, { 587, -28527, -9267 }, { 10211, -27658, 5550 }, { -722, -29226, 6731 }, { -13859, -26517, 2182 }, { -3461, -28975, 6961 }, { -16910, 4805, -16793 }, { 15478, -5944, 16820 }, { -15806, -4497, -16971 }, { -1839, 25176, 16211 }, { 1961, 26960, 13011 } },
    { { 3578, -23814, 17891 }, { -1933, -28587, -8892 }, { -3621, -24044, 17571 }, { 659, -28515, -9298 }, { 10487, -27579, 5426 }, { -877, -29269, 6524 }, { -13712, -26616, 1891 }, { -2965, -29096, 6682 }, { -16937, 4935, -16785 }, { 15481, -6185, 16854 }, { -15799, -4297, -16915 }, { -1459, 25469, 15785 }, { 1500, 27278, 12396 } },
    { { 3642, -23659, 18082 }, { -2004, -28588, -8871 }, { -3706, -23887, 17767 }, { 731, -28505, -9325 }, { 10739, -27517, 5240 }, { -981, -29324, 6257 }, { -13534, -26728, 1567 }, { -2445, -29207, 6402 }, { -16987, 5099, -16803 }, { 15488, -6429, 16873 }, { -15781, -4130, -16861 }, { -1224, 25729, 15379 }, { 997, 27556, 11819 } },
    { { 3722, -23468, 18315 }, { -2076, -28579, -8885 }, { -3741, -23701, 18007 }, { 761, -28482, -9391 }, { 10994, -27468, 4964 }, { -1050, -29390, 5925 }, { -13651, -26692, 1093 }, { -1803, -29294, 6216 }, { -17041, 5300, -16833 }, { 15486, -6684, 16886 }, { -15750, -3995, -16830 }, { -1125, 25963, 14988 }, { 788, 27747, 11378 } },
    { { 3817, -23231, 18594 }, { -2164, -28547, -8965 }, { -3743, -23472, 18304 }, { 757, -28441, -9515 }, { 11388, -27372, 4592 }, { -1247, -29453, 5567 }, { -14372, -26331, 327 }, { -869, -29335, 6220 }, { -17078, 5525, -16861 }, { 15466, -6952, 16901 }, { -15715, -3878, -16826 }, { -999, 26189, 14599 }, { 1281, 27818, 11158 } },
    { { 3917, -22955, 18914 }, { -2271, -28491, -9115 }, { -3769, -23197, 18646 }, { 765, -28381, -9693 }, { 12119, -27133, 4117 }, { -1802, -29486, 5229 }, { -15552, -25644, -718 }, { 298, -29316, 6364 }, { -17094, 5751, -16880 }, { 15443, -7229, 16919 }, { -15682, -3760, -16836 }, { -662, 26440, 14160 }, { 2407, 27732, 11186 } },
    { { 4009, -22648, 19261 }, { -2390, -28416, -9317 }, { -3854, -22885, 19011 }, { 820, -28305, -9907 }, { 13191, -26709, 3552 }, { -2721, -29466, 4932 }, { -16681, -24867, -1841 }, { 1336, -29265, 6463 }, { -17098, 5960, -16889 }, { 15438, -7513, 16937 }, { -15644, -3626, -16849 }, { -132, 26746, 13589 }, { 3343, 27586, 11307 } },
    { { 4079, -22325, 19620 }, { -2504, -28330, -9547 }, { -3992, -22547, 19383 }, { 913, -28222, -10134 }, { 14432, -26136, 2934 }, { -3821, -29388, 4661 }, { -17464, -24222, -2879 }, { 2030, -29235, 6417 }, { -17092, 6144, -16885 }, { 15455, -7803, 16951 }, { -15589, -3463, -16871 }, { 439, 27107, 12846 }, { 3419, 27600, 11249 } },
    { { 4125, -21999, 19975 }, { -2602, -28239, -9787 }, { -4134, -22200, 19750 }, { 1006, -28138, -10355 }, { 15678, -25474, 2294 }, { -4903, -29268, 4401 }, { -17973, -23715, -3820 }, { 2474, -29229, 6287 }, { -17066, 6312, -16866 }, { 15477, -8102, 16964 }, { -15512, -3262, -16913 }, { 964, 27475, 12009 }, { 2661, 27824, 10896 } },
    { { 4166, -21678, 20315 }, { -2692, -28146, -10029 }, { -4238, -21855, 20109 }, { 1062, -28056, -10570 }, { 16877, -24751, 1593 }, { -5862, -29125, 4170 }, { -18408, -23210, -4734 }, { 2863, -29215, 6190 }, { -17021, 6472, -16838 }, { 15483, -8408, 16978 }, { -15426, -3032, -16974 }, { 1532, 27784, 11211 }, { 1464, 28146, 10280 } },
    { { 4239, -21357, 20637 }, { -2799, -28053, -10256 }, { -4290, -21524, 20453 }, { 1082, -27978, -10774 }, { 18030, -23966, 736 }, { -6670, -28974, 4004 }, { -18905, -22595, -5665 }, { 3294, -29169, 6189 }, { -16973, 6623, -16820 }, { 15477, -8713, 16992 }, { -15360, -2804, -17050 }, { 2276, 27985, 10567 }, { 108, 28451, 9514 } },
    { { 4365, -21042, 20933 }, { -2945, -27966, -10453 }, { -4312, -21214, 20769 }, { 1089, -27906, -10958 }, { 19113, -23121, -305 }, { -7317, -28832, 3900 }, { -19474, -21848, -6590 }, { 3761, -29090, 6293 }, { -16934, 6762, -16822 }, { 15468, -9012, 17008 }, { -15325, -2609, -17134 }, { 3193, 28065, 10108 }, { -1347, 28672, 8723 } },
    { { 4530, -20750, 21187 }, { -3129, -27885, -10613 }, { -4334, -20929, 21052 }, { 1097, -27842, -11118 }, { 20086, -22239, -1413 }, { -7773, -28722, 3829 }, { -20071, -21012, -7460 }, { 4253, -28976, 6503 }, { -16897, 6882, -16826 }, { 15450, -9293, 17047 }, { -15308, -2460, -17215 }, { 4111, 28059, 9787 }, { -2893, 28773, 7984 } },
    { { 4702, -20495, 21397 }, { -3328, -27811, -10745 }, { -4357, -20667, 21304 }, { 1100, -27783, -11264 }, { 20911, -21376, -2408 }, { -8029, -28656, 3789 }, { -20637, -20151, -8247 }, { 4745, -28829, 6811 }, { -16857, 6978, -16813 }, { 15413, -9536, 17121 }, { -15293, -2352, -17281 }, { 4865, 28027, 9530 }, { -4453, 28756, 7299 } },
    { { 4858, -20274, 21572 }, { -3511, -27744, -10860 }, { -4363, -20426, 21535 }, { 1088, -27724, -11410 }, { 21569, -20605, -3192 }, { -8127, -28624, 3823 }, { -21113, -19340, -8956 }, { 5180, -28665, 7175 }, { -16822, 7049, -16782 }, { 15370, -9720, 17207 }, { -15274, -2270, -17327 }, { 5424, 28008, 9280 }, { -5926, 28648, 6645 } },
    { { 4981, -20081, 21724 }, { -3645, -27688, -10957 }, { -4346, -20211, 21740 }, { 1069, -27666, -11551 }, { 22048, -19985, -3802 }, { -8138, -28606, 3931 }, { -21466, -18634, -9591 }, { 5526, -28503, 7553 }, { -16807, 7096, -16752 }, { 15339, -9841, 17277 }, { -15253, -2194, -17358 }, { 5871, 27981, 9089 }, { -7234, 28475, 6070 } },
    { { 5048, -19924, 21853 }, { -3709, -27650, -11032 }, { -4332, -20037, 21903 }, { 1069, -27618, -11665 }, { 22346, -19553, -4284 }, { -8121, -28596, 4039 }, { -21686, -18094, -10114 }, { 5789, -28359, 7889 }, { -16811, 7122, -16730 }, { 15325, -9914, 17327 }, { -15234, -2106, -17379 }, { 6247, 27913, 9047 }, { -8300, 28259, 5704 } },
    { { 5051, -19820, 21947 }, { -3699, -27632, -11082 }, { -4338, -19923, 22006 }, { 1092, -27588, -11734 }, { 22474, -19328, -4618 }, { -8097, -28600, 4062 }, { -21778, -17782, -10465 }, { 5976, -28260, 8103 }, { -16820, 7129, -16716 }, { 15314, -9955, 17367 }, { -15221, -2007, -17388 }, { 6490, 27813, 9181 }, { -9057, 28034, 5661 } },
    { { 5003, -19785, 21989 }, { -3634, -27631, -11104 }, { -4352, -19883, 22039 }, { 1121, -27579, -11753 }, { 22448, -19327, -4748 }, { -8043, -28627, 3977 }, { -21755, -17731, -10598 }, { 6074, -28230, 8134 }, { -16827, 7124, -16707 }, { 15295, -9968, 17404 }, { -15216, -1931, -17386 }, { 6513, 27726, 9425 }, { -9508, 27829, 5929 } },
    { { 4935, -19827, 21966 }, { -3550, -27648, -11090 }, { -4346, -19924, 22003 }, { 1126, -27590, -11727 }, { 22303, -19513, -4672 }, { -7927, -28679, 3833 }, { -21656, -17900, -10516 }, { 6104, -28245, 8060 }, { -16827, 7121, -16699 }, { 15264, -9945, 17437 }, { -15216, -1931, -17373 }, { 6330, 27683, 9674 }, { -9832, 27635, 6296 } },
    { { 4864, -19932, 21888 }, { -3460, -27679, -11042 }, { -4303, -20032, 21914 }, { 1095, -27616, -11669 }, { 22099, -19799, -4430 }, { -7757, -28744, 3691 }, { -21511, -18220, -10261 }, { 6130, -28250, 8021 }, { -16829, 7129, -16699 }, { 15229, -9882, 17461 }, { -15220, -2028, -17353 }, { 6040, 27679, 9868 }, { -10150, 27455, 6573 } },
    { { 4794, -20080, 21767 }, { -3377, -27720, -10965 }, { -4233, -20188, 21784 }, { 1036, -27651, -11590 }, { 21894, -20092, -4121 }, { -7578, -28804, 3591 }, { -21378, -18573, -9900 }, { 6216, -28208, 8103 }, { -16838, 7145, -16708 }, { 15200, -9778, 17482 }, { -15222, -2205, -17321 }, { 5812, 27684, 9991 }, { -10505, 27292, 6692 } },
    { { 4733, -20256, 21617 }, { -3309, -27764, -10871 }, { -4169, -20372, 21624 }, { 979, -27690, -11501 }, { 21761, -20286, -3862 }, { -7432, -28843, 3581 }, { -21296, -18859, -9531 }, { 6349, -28130, 8269 }, { -16855, 7149, -16721 }, { 15186, -9650, 17503 }, { -15214, -2399, -17281 }, { 5694, 27687, 10051 }, { -10862, 27150, 6699 } },
    { { 4686, -20444, 21450 }, { -3263, -27807, -10775 }, { -4128, -20566, 21447 }, { 939, -27728, -11415 }, { 21752, -20327, -3698 }, { -7349, -28847, 3719 }, { -21222, -19101, -9207 }, { 6480, -28059, 8408 }, { -16879, 7127, -16737 }, { 15191, -9510, 17508 }, { -15200, -2570, -17241 }, { 5607, 27695, 10078 }, { -11109, 27049, 6704 } },
    { { 4647, -20639, 21270 }, { -3232, -27846, -10684 }, { -4110, -20767, 21257 }, { 914, -27762, -11332 }, { 21873, -20213, -3611 }, { -7347, -28809, 4007 }, { -21127, -19325, -8954 }, { 6568, -28019, 8472 }, { -16904, 7080, -16752 }, { 15209, -9363, 17492 }, { -15182, -2733, -17211 }, { 5523, 27700, 10109 }, { -11162, 27012, 6763 } },
    { { 4597, -20848, 21077 }, { -3204, -27881, -10600 }, { -4136, -20975, 21046 }, { 921, -27799, -11242 }, { 22104, -19970, -3549 }, { -7444, -28732, 4365 }, { -21073, -19453, -8803 }, { 6602, -28003, 8499 }, { -16917, 7017, -16759 }, { 15227, -9208, 17475 }, { -15162, -2920, -17188 }, { 5509, 27685, 10158 }, { -10980, 27062, 6861 } },
    { { 4520, -21075, 20867 }, { -3167, -27914, -10526 }, { -4219, -21195, 20808 }, { 969, -27841, -11133 }, { 22403, -19653, -3448 }, { -7628, -28629, 4709 }, { -21148, -19406, -8728 }, { 6629, -27972, 8580 }, { -16912, 6943, -16755 }, { 15237, -9040, 17480 }, { -15139, -3130, -17163 }, { 5592, 27659, 10183 }, { -10616, 27183, 6957 } },
    { { 4415, -21323, 20636 }, { -3115, -27945, -10459 }, { -4342, -21430, 20540 }, { 1044, -27889, -11005 }, { 22730, -19307, -3251 }, { -7877, -28504, 5049 }, { -21336, -19242, -8633 }, { 6773, -27868, 8802 }, { -16885, 6863, -16739 }, { 15238, -8854, 17504 }, { -15111, -3344, -17135 }, { 5688, 27654, 10145 }, { -10228, 27317, 7011 } },
    { { 4287, -21591, 20383 }, { -3047, -27977, -10391 }, { -4459, -21683, 20247 }, { 1104, -27944, -10858 }, { 23054, -18970, -2940 }, { -8142, -28354, 5453 }, { -21534, -19113, -8424 }, { 7115, -27660, 9181 }, { -16841, 6783, -16717 }, { 15230, -8652, 17530 }, { -15071, -3552, -17120 }, { 5670, 27694, 10045 }, { -9982, 27415, 6984 } },
    { { 4148, -21874, 20108 }, { -2974, -28014, -10313 }, { -4542, -21951, 19938 }, { 1121, -28007, -10695 }, { 23356, -18656, -2539 }, { -8391, -28182, 5946 }, { -21734, -19020, -8116 }, { 7600, -27359, 9682 }, { -16783, 6707, -16689 }, { 15218, -8439, 17545 }, { -15025, -3766, -17121 }, { 5495, 27788, 9882 }, { -9880, 27483, 6862 } },
    { { 4006, -22165, 19815 }, { -2904, -28054, -10225 }, { -4602, -22224, 19619 }, { 1107, -28073, -10522 }, { 23626, -18366, -2116 }, { -8605, -28000, 6477 }, { -22038, -18807, -7786 }, { 8091, -27001, 10270 }, { -16714, 6632, -16653 }, { 15208, -8223, 17555 }, { -14992, -4004, -17116 }, { 5244, 27909, 9673 }, { -9800, 27561, 6660 } },
    { { 3861, -22464, 19506 }, { -2834, -28095, -10131 }, { -4661, -22498, 19291 }, { 1088, -28141, -10340 }, { 23871, -18088, -1731 }, { -8783, -27816, 7008 }, { -22460, -18445, -7439 }, { 8523, -26611, 10919 }, { -16635, 6544, -16607 }, { 15201, -8013, 17568 }, { -14983, -4269, -17091 }, { 5023, 28015, 9484 }, { -9650, 27672, 6415 } },
    { { 3714, -22766, 19181 }, { -2760, -28139, -10029 }, { -4710, -22770, 18956 }, { 1064, -28207, -10159 }, { 24103, -17807, -1394 }, { -8938, -27619, 7568 }, { -22873, -18110, -6989 }, { 8935, -26177, 11616 }, { -16555, 6431, -16551 }, { 15187, -7805, 17586 }, { -14978, -4543, -17067 }, { 4868, 28085, 9357 }, { -9447, 27806, 6129 } },
    { { 3587, -23062, 18848 }, { -2691, -28186, -9916 }, { -4720, -23035, 18631 }, { 1016, -28268, -9995 }, { 24335, -17513, -1059 }, { -9092, -27389, 8195 }, { -23169, -17941, -6429 }, { 9349, -25699, 12335 }, { -16487, 6298, -16494 }, { 15163, -7596, 17601 }, { -14946, -4799, -17087 }, { 4727, 28150, 9232 }, { -9213, 27955, 5799 } },
    { { 3506, -23340, 18519 }, { -2640, -28234, -9791 }, { -4663, -23288, 18329 }, { 934, -28320, -9853 }, { 24565, -17208, -663 }, { -9249, -27125, 8872 }, { -23356, -17886, -5880 }, { 9708, -25200, 13065 }, { -16437, 6166, -16445 }, { 15143, -7386, 17609 }, { -14879, -5021, -17161 }, { 4494, 28267, 8989 }, { -8962, 28106, 5454 } },
    { { 3465, -23600, 18194 }, { -2602, -28284, -9655 }, { -4537, -23538, 18038 }, { 813, -28370, -9721 }, { 24775, -16916, -199 }, { -9407, -26843, 9538 }, { -23502, -17831, -5451 }, { 9965, -24711, 13786 }, { -16403, 6045, -16416 }, { 15144, -7185, 17604 }, { -14810, -5207, -17254 }, { 4142, 28445, 8588 }, { -8711, 28246, 5127 } },
    { { 3422, -23863, 17856 }, { -2545, -28336, -9517 }, { -4371, -23810, 17719 }, { 668, -28424, -9573 }, { 24932, -16683, 322 }, { -9570, -26550, 10174 }, { -23642, -17755, -5081 }, { 10140, -24219, 14512 }, { -16370, 5914, -16410 }, { 15170, -6992, 17583 }, { -14772, -5378, -17326 }, { 3795, 28647, 8058 }, { -8465, 28382, 4774 } },
    { { 3350, -24161, 17465 }, { -2449, -28390, -9381 }, { -4220, -24124, 17327 }, { 542, -28488, -9389 }, { 25023, -16521, 957 }, { -9718, -26220, 10865 }, { -23757, -17734, -4596 }, { 10273, -23649, 15336 }, { -16319, 5726, -16407 }, { 15196, -6770, 17555 }, { -14755, -5589, -17375 }, { 3616, 28847, 7401 }, { -8195, 28547, 4228 } },
    { { 3252, -24497, 17009 }, { -2329, -28446, -9242 }, { -4129, -24458, 16874 }, { 467, -28555, -9186 }, { 25077, -16375, 1739 }, { -9821, -25836, 11665 }, { -23828, -17798, -3931 }, { 10403, -22946, 16287 }, { -16246, 5470, -16382 }, { 15201, -6495, 17540 }, { -14732, -5868, -17411 }, { 3599, 29030, 6655 }, { -7909, 28734, 3432 } },
    { { 3150, -24805, 16577 }, { -2216, -28498, -9107 }, { -4075, -24748, 16459 }, { 418, -28612, -9009 }, { 25122, -16201, 2532 }, { -9887, -25461, 12408 }, { -23883, -17864, -3243 }, { 10549, -22215, 17182 }, { -16185, 5246, -16343 }, { 15194, -6241, 17544 }, { -14708, -6149, -17427 }, { 3582, 29173, 6009 }, { -7646, 28888, 2645 } },
    { { 3066, -25017, 16271 }, { -2124, -28544, -8986 }, { -4005, -24951, 16168 }, { 352, -28652, -8884 }, { 25158, -16024, 3215 }, { -9935, -25163, 12966 }, { -23953, -17866, -2661 }, { 10692, -21589, 17877 }, { -16165, 5153, -16320 }, { 15194, -6071, 17551 }, { -14701, -6369, -17416 }, { 3466, 29279, 5545 }, { -7415, 28995, 2069 } },
    { { 2998, -25151, 16075 }, { -2046, -28577, -8897 }, { -3907, -25090, 15976 }, { 262, -28676, -8812 }, { 25173, -15858, 3853 }, { -9953, -24880, 13487 }, { -24010, -17867, -2078 }, { 10784, -21038, 18468 }, { -16180, 5156, -16322 }, { 15210, -5940, 17545 }, { -14698, -6576, -17399 }, { 3258, 29393, 5045 }, { -7168, 29091, 1522 } },
    { { 2928, -25267, 15906 }, { -1965, -28600, -8842 }, { -3813, -25210, 15808 }, { 176, -28687, -8778 }, { 25167, -15681, 4552 }, { -9918, -24502, 14188 }, { -23998, -17954, -1329 }, { 10790, -20459, 19105 }, { -16202, 5152, -16326 }, { 15235, -5767, 17524 }, { -14670, -6848, -17409 }, { 2944, 29544, 4297 }, { -6828, 29203, 749 } },
    { { 2837, -25396, 15716 }, { -1868, -28611, -8826 }, { -3739, -25334, 15628 }, { 103, -28689, -8770 }, { 25155, -15461, 5308 }, { -9830, -24010, 15062 }, { -23910, -18114, -439 }, { 10715, -19822, 19806 }, { -16203, 5083, -16314 }, { 15252, -5549, 17498 }, { -14621, -7173, -17453 }, { 2481, 29702, 3409 }, { -6368, 29316, -213 } },
    { { 2726, -25532, 15514 }, { -1758, -28612, -8848 }, { -3670, -25453, 15449 }, { 23, -28683, -8792 }, { 25150, -15191, 6057 }, { -9717, -23469, 15962 }, { -23797, -18262, 438 }, { 10601, -19146, 20520 }, { -16179, 4969, -16283 }, { 15257, -5333, 17479 }, { -14581, -7501, -17493 }, { 1910, 29825, 2608 }, { -5834, 29404, -1161 } },
    { { 2614, -25660, 15321 }, { -1657, -28602, -8899 }, { -3593, -25567, 15278 }, { -75, -28667, -8841 }, { 25142, -14895, 6782 }, { -9617, -22921, 16797 }, { -23724, -18321, 1237 }, { 10492, -18437, 21213 }, { -16154, 4845, -16255 }, { 15265, -5137, 17469 }, { -14576, -7820, -17496 }, { 1372, 29904, 1965 }, { -5308, 29458, -2008 } },
    { { 2532, -25778, 15136 }, { -1593, -28587, -8958 }, { -3531, -25682, 15099 }, { -165, -28648, -8902 }, { 25114, -14606, 7479 }, { -9539, -22375, 17560 }, { -23699, -18284, 2006 }, { 10398, -17719, 21861 }, { -16146, 4729, -16247 }, { 15284, -4955, 17459 }, { -14587, -8137, -17480 }, { 978, 29951, 1406 }, { -4862, 29472, -2790 } },
    { { 2497, -25886, 14956 }, { -1577, -28574, -9004 }, { -3497, -25792, 14918 }, { -221, -28631, -8955 }, { 25072, -14338, 8114 }, { -9464, -21848, 18251 }, { -23677, -18213, 2780 }, { 10309, -17049, 22429 }, { -16154, 4640, -16251 }, { 15306, -4792, 17448 }, { -14590, -8435, -17481 }, { 723, 29979, 849 }, { -4528, 29445, -3537 } },
    { { 2497, -25979, 14795 }, { -1592, -28560, -9044 }, { -3471, -25888, 14756 }, { -260, -28617, -9000 }, { 25023, -14093, 8673 }, { -9386, -21354, 18866 }, { -23628, -18144, 3534 }, { 10184, -16454, 22925 }, { -16159, 4589, -16257 }, { 15321, -4660, 17429 }, { -14589, -8686, -17511 }, { 550, 29994, 226 }, { -4244, 29390, -4267 } },
    { { 2508, -26049, 14669 }, { -1605, -28542, -9099 }, { -3425, -25970, 14622 }, { -302, -28604, -9041 }, { 24968, -13870, 9177 }, { -9325, -20885, 19413 }, { -23578, -18062, 4223 }, { 9983, -15894, 23403 }, { -16151, 4570, -16270 }, { 15329, -4557, 17397 }, { -14593, -8888, -17559 }, { 444, 29992, -558 }, { -3908, 29319, -5010 } },
    { { 2513, -26097, 14581 }, { -1603, -28518, -9173 }, { -3382, -26037, 14513 }, { -337, -28591, -9079 }, { 24911, -13643, 9660 }, { -9289, -20430, 19908 }, { -23548, -17948, 4832 }, { 9719, -15336, 23882 }, { -16137, 4572, -16291 }, { 15342, -4481, 17353 }, { -14595, -9054, -17610 }, { 402, 29960, -1487 }, { -3517, 29231, -5761 } },
    { { 2510, -26124, 14534 }, { -1601, -28491, -9259 }, { -3379, -26078, 14440 }, { -337, -28576, -9127 }, { 24871, -13386, 10112 }, { -9268, -19973, 20376 }, { -23530, -17826, 5346 }, { 9436, -14793, 24334 }, { -16125, 4588, -16311 }, { 15359, -4436, 17307 }, { -14582, -9195, -17660 }, { 456, 29898, -2433 }, { -3175, 29116, -6495 } },
    { { 2528, -26123, 14533 }, { -1635, -28459, -9348 }, { -3426, -26083, 14421 }, { -301, -28554, -9196 }, { 24859, -13115, 10491 }, { -9256, -19482, 20851 }, { -23493, -17749, 5750 }, { 9160, -14292, 24735 }, { -16123, 4617, -16324 }, { 15376, -4427, 17263 }, { -14570, -9306, -17706 }, { 644, 29813, -3279 }, { -2977, 28968, -7210 } },
    { { 2601, -26090, 14579 }, { -1732, -28424, -9437 }, { -3490, -26049, 14466 }, { -254, -28523, -9292 }, { 24857, -12903, 10754 }, { -9248, -18943, 21346 }, { -23397, -17800, 5978 }, { 8858, -13862, 25088 }, { -16129, 4652, -16329 }, { 15383, -4456, 17217 }, { -14594, -9367, -17732 }, { 948, 29713, -4028 }, { -2955, 28790, -7900 } },
    { { 2716, -26034, 14657 }, { -1867, -28383, -9536 }, { -3514, -25983, 14578 }, { -244, -28478, -9431 }, { 24840, -12785, 10932 }, { -9218, -18398, 21830 }, { -23200, -18066, 5947 }, { 8477, -13503, 25413 }, { -16137, 4678, -16318 }, { 15368, -4515, 17185 }, { -14652, -9359, -17723 }, { 1268, 29592, -4763 }, { -3108, 28600, -8506 } },
    { { 2840, -25970, 14748 }, { -2001, -28335, -9650 }, { -3463, -25894, 14747 }, { -304, -28411, -9628 }, { 24815, -12703, 11083 }, { -9106, -17923, 22268 }, { -22852, -18608, 5615 }, { 8006, -13195, 25726 }, { -16141, 4686, -16280 }, { 15331, -4589, 17197 }, { -14710, -9282, -17669 }, { 1488, 29449, -5527 }, { -3493, 28424, -8938 } },
    { { 2967, -25886, 14870 }, { -2130, -28281, -9780 }, { -3367, -25783, 14962 }, { -404, -28324, -9880 }, { 24817, -12610, 11184 }, { -8868, -17544, 22662 }, { -22369, -19325, 5117 }, { 7528, -12935, 26000 }, { -16166, 4693, -16233 }, { 15303, -4675, 17244 }, { -14767, -9169, -17571 }, { 1578, 29290, -6293 }, { -4096, 28279, -9139 } },
    { { 3119, -25739, 15092 }, { -2276, -28212, -9946 }, { -3273, -25635, 15235 }, { -493, -28220, -10169 }, { 24858, -12564, 11145 }, { -8548, -17201, 23044 }, { -21869, -19999, 4670 }, { 7136, -12758, 26198 }, { -16237, 4734, -16229 }, { 15325, -4781, 17265 }, { -14862, -9061, -17454 }, { 1615, 29122, -7022 }, { -4768, 28179, -9120 } },
    { { 3299, -25496, 15461 }, { -2457, -28112, -10181 }, { -3200, -25437, 15580 }, { -551, -28104, -10481 }, { 24901, -12638, 10964 }, { -8242, -16848, 23414 }, { -21483, -20469, 4413 }, { 6829, -12694, 26310 }, { -16336, 4829, -16291 }, { 15390, -4910, 17226 }, { -14989, -8977, -17364 }, { 1752, 28958, -7638 }, { -5319, 28133, -8957 } },
    { { 3481, -25175, 15941 }, { -2688, -27974, -10499 }, { -3137, -25175, 16012 }, { -559, -27972, -10829 }, { 24928, -12775, 10743 }, { -8002, -16524, 23726 }, { -21242, -20740, 4312 }, { 6559, -12712, 26370 }, { -16412, 4951, -16379 }, { 15443, -5048, 17170 }, { -15085, -8898, -17323 }, { 2063, 28835, -8016 }, { -5729, 28106, -8790 } },
    { { 3645, -24862, 16389 }, { -2899, -27825, -10835 }, { -3115, -24889, 16457 }, { -556, -27835, -11176 }, { 24978, -12863, 10519 }, { -7816, -16286, 23951 }, { -21129, -20861, 4285 }, { 6346, -12708, 26424 }, { -16441, 5058, -16436 }, { 15448, -5175, 17158 }, { -15116, -8795, -17306 }, { 2459, 28786, -8082 }, { -6157, 28012, -8800 } },
    { { 3779, -24565, 16801 }, { -3030, -27676, -11175 }, { -3115, -24609, 16872 }, { -523, -27692, -11527 }, { 25096, -12895, 10194 }, { -7676, -16076, 24138 }, { -21156, -20825, 4331 }, { 6302, -12567, 26502 }, { -16442, 5131, -16457 }, { 15421, -5289, 17181 }, { -15129, -8662, -17284 }, { 2792, 28792, -7950 }, { -6860, 27762, -9066 } },
    { { 3835, -24277, 17203 }, { -3097, -27523, -11528 }, { -3126, -24330, 17271 }, { -485, -27543, -11880 }, { 25254, -12956, 9714 }, { -7600, -15742, 24381 }, { -21262, -20677, 4514 }, { 6528, -12232, 26604 }, { -16445, 5183, -16471 }, { 15394, -5395, 17198 }, { -15171, -8530, -17258 }, { 3045, 28799, -7834 }, { -8082, 27302, -9448 } },
    { { 3858, -23994, 17590 }, { -3125, -27376, -11865 }, { -3117, -24060, 17646 }, { -462, -27394, -12222 }, { 25382, -13070, 9216 }, { -7604, -15197, 24723 }, { -21290, -20579, 4822 }, { 6970, -11742, 26712 }, { -16464, 5225, -16492 }, { 15377, -5493, 17192 }, { -15234, -8429, -17227 }, { 3307, 28770, -7831 }, { -9709, 26660, -9746 } },
    { { 3914, -23748, 17908 }, { -3166, -27253, -12134 }, { -3078, -23831, 17962 }, { -450, -27255, -12528 }, { 25427, -13182, 8927 }, { -7695, -14508, 25106 }, { -21149, -20633, 5196 }, { 7449, -11181, 26823 }, { -16505, 5256, -16514 }, { 15367, -5574, 17181 }, { -15295, -8361, -17175 }, { 3601, 28720, -7887 }, { -11187, 25977, -10002 } },
    { { 4021, -23577, 18110 }, { -3242, -27164, -12312 }, { -3032, -23672, 18178 }, { -428, -27141, -12774 }, { 25403, -13217, 8945 }, { -7850, -13844, 25431 }, { -20942, -20736, 5608 }, { 7810, -10626, 26946 }, { -16556, 5277, -16529 }, { 15360, -5627, 17176 }, { -15341, -8307, -17097 }, { 3851, 28677, -7925 }, { -11973, 25460, -10413 } },
    { { 4145, -23508, 18172 }, { -3327, -27110, -12409 }, { -2998, -23608, 18267 }, { -393, -27062, -12941 }, { 25345, -13124, 9239 }, { -8017, -13332, 25651 }, { -20794, -20745, 6105 }, { 8041, -10116, 27074 }, { -16598, 5285, -16533 }, { 15349, -5644, 17181 }, { -15369, -8263, -17018 }, { 3958, 28660, -7934 }, { -11959, 25244, -10943 } },
    { { 4226, -23564, 18079 }, { -3370, -27099, -12421 }, { -2959, -23662, 18204 }, { -367, -27033, -13004 }, { 25274, -12905, 9731 }, { -8139, -12977, 25794 }, { -20714, -20634, 6719 }, { 8228, -9679, 27177 }, { -16621, 5283, -16528 }, { 15336, -5626, 17194 }, { -15379, -8251, -16961 }, { 3877, 28664, -7959 }, { -11477, 25313, -11294 } },
    { { 4220, -23757, 17827 }, { -3328, -27144, -12334 }, { -2904, -23850, 17965 }, { -367, -27063, -12940 }, { 25160, -12581, 10427 }, { -8212, -12725, 25897 }, { -20663, -20429, 7464 }, { 8418, -9349, 27235 }, { -16635, 5279, -16523 }, { 15331, -5581, 17210 }, { -15382, -8293, -16931 }, { 3583, 28668, -8081 }, { -10826, 25600, -11288 } },
    { { 4118, -24072, 17424 }, { -3189, -27248, -12141 }, { -2849, -24164, 17550 }, { -376, -27158, -12739 }, { 24952, -12192, 11347 }, { -8277, -12501, 25985 }, { -20617, -20130, 8349 }, { 8580, -9122, 27261 }, { -16647, 5286, -16527 }, { 15339, -5526, 17217 }, { -15387, -8385, -16917 }, { 3092, 28649, -8346 }, { -10114, 25988, -11061 } },
    { { 3969, -24470, 16895 }, { -2999, -27395, -11854 }, { -2815, -24562, 16994 }, { -374, -27307, -12416 }, { 24622, -11751, 12477 }, { -8362, -12233, 26085 }, { -20550, -19730, 9403 }, { 8658, -8936, 27298 }, { -16650, 5307, -16538 }, { 15350, -5470, 17209 }, { -15416, -8513, -16900 }, { 2428, 28603, -8715 }, { -9346, 26344, -10893 } },
    { { 3841, -24910, 16271 }, { -2815, -27573, -11480 }, { -2791, -25002, 16343 }, { -357, -27492, -12002 }, { 24180, -11264, 13727 }, { -8457, -11868, 26223 }, { -20420, -19238, 10628 }, { 8652, -8722, 27369 }, { -16646, 5341, -16551 }, { 15360, -5421, 17188 }, { -15504, -8660, -16851 }, { 1655, 28549, -9068 }, { -8538, 26592, -10951 } },
    { { 3755, -25339, 15615 }, { -2590, -27772, -11046 }, { -2751, -25455, 15635 }, { -339, -27693, -11532 }, { 23656, -10738, 15003 }, { -8542, -11419, 26394 }, { -20214, -18662, 11964 }, { 8586, -8481, 27466 }, { -16646, 5388, -16564 }, { 15369, -5385, 17157 }, { -15646, -8808, -16768 }, { 884, 28502, -9320 }, { -7672, 26758, -11186 } },
    { { 3596, -25757, 14954 }, { -2307, -27968, -10606 }, { -2680, -25894, 14911 }, { -350, -27890, -11046 }, { 23069, -10189, 16248 }, { -8603, -10964, 26567 }, { -19962, -18021, 13295 }, { 8450, -8268, 27572 }, { -16650, 5447, -16578 }, { 15369, -5366, 17124 }, { -15774, -8934, -16691 }, { 234, 28464, -9473 }, { -6699, 26912, -11439 } },
    { { 3356, -26153, 14310 }, { -1962, -28149, -10186 }, { -2596, -26282, 14231 }, { -430, -28073, -10570 }, { 22439, -9647, 17420 }, { -8615, -10564, 26724 }, { -19683, -17355, 14539 }, { 8244, -8108, 27682 }, { -16648, 5511, -16588 }, { 15352, -5363, 17095 }, { -15833, -9035, -16654 }, { -293, 28419, -9607 }, { -5677, 27070, -11617 } },
    { { 3088, -26529, 13664 }, { -1598, -28321, -9765 }, { -2462, -26642, 13569 }, { -555, -28239, -10112 }, { 21781, -9139, 18495 }, { -8558, -10222, 26875 }, { -19346, -16706, 15705 }, { 8041, -7977, 27780 }, { -16645, 5570, -16590 }, { 15337, -5372, 17071 }, { -15848, -9122, -16640 }, { -831, 28342, -9801 }, { -4792, 27183, -11752 } },
    { { 2821, -26868, 13045 }, { -1274, -28476, -9355 }, { -2300, -26981, 12912 }, { -734, -28393, -9658 }, { 21079, -8656, 19513 }, { -8444, -9895, 27033 }, { -18926, -16064, 16845 }, { 7881, -7830, 27867 }, { -16643, 5626, -16584 }, { 15336, -5392, 17052 }, { -15859, -9205, -16634 }, { -1432, 28221, -10076 }, { -4126, 27224, -11910 } },
    { { 2576, -27165, 12467 }, { -1000, -28609, -8975 }, { -2138, -27290, 12275 }, { -942, -28533, -9219 }, { 20293, -8166, 20531 }, { -8300, -9544, 27203 }, { -18439, -15371, 17993 }, { 7719, -7651, 27962 }, { -16636, 5689, -16574 }, { 15345, -5419, 17037 }, { -15862, -9284, -16650 }, { -2010, 28079, -10369 }, { -3545, 27225, -12092 } },
    { { 2371, -27420, 11938 }, { -774, -28719, -8638 }, { -1985, -27552, 11702 }, { -1128, -28647, -8837 }, { 19445, -7649, 21526 }, { -8136, -9164, 27383 }, { -17918, -14587, 19136 }, { 7526, -7442, 28071 }, { -16629, 5759, -16568 }, { 15362, -5446, 17023 }, { -15852, -9357, -16687 }, { -2504, 27962, -10577 }, { -2836, 27224, -12280 } },
    { { 2213, -27631, 11474 }, { -597, -28807, -8356 }, { -1843, -27764, 11214 }, { -1272, -28733, -8530 }, { 18613, -7126, 22422 }, { -7935, -8776, 27568 }, { -17390, -13731, 20225 }, { 7322, -7195, 28189 }, { -16629, 5832, -16570 }, { 15389, -5470, 17009 }, { -15834, -9422, -16727 }, { -2951, 27889, -10653 }, { -1950, 27218, -12466 } },
    { { 2085, -27804, 11073 }, { -452, -28876, -8123 }, { -1714, -27940, 10789 }, { -1391, -28804, -8269 }, { 17850, -6635, 23181 }, { -7699, -8402, 27751 }, { -16872, -12875, 21203 }, { 7118, -6910, 28312 }, { -16632, 5901, -16577 }, { 15421, -5499, 16995 }, { -15823, -9478, -16754 }, { -3432, 27824, -10680 }, { -1103, 27203, -12601 } },
    { { 1949, -27948, 10729 }, { -306, -28935, -7916 }, { -1589, -28090, 10412 }, { -1497, -28868, -8023 }, { 17139, -6188, 23832 }, { -7450, -8041, 27926 }, { -16354, -12097, 22050 }, { 6890, -6621, 28437 }, { -16633, 5964, -16585 }, { 15447, -5544, 16985 }, { -15833, -9523, -16756 }, { -3963, 27721, -10764 }, { -576, 27212, -12617 } },
    { { 1725, -28027, 10560 }, { -200, -28965, -7810 }, { -1442, -28197, 10141 }, { -1561, -28918, -7831 }, { 16424, -5771, 24432 }, { -7207, -7672, 28093 }, { -15795, -11408, 22812 }, { 6626, -6384, 28554 }, { -16628, 6023, -16592 }, { 15462, -5615, 16976 }, { -15873, -9557, -16730 }, { -4430, 27600, -10892 }, { -382, 27250, -12541 } },
    { { 1452, -28093, 10424 }, { -67, -29000, -7682 }, { -1276, -28267, 9968 }, { -1565, -28961, -7669 }, { 15685, -5400, 24997 }, { -6958, -7285, 28258 }, { -15165, -10757, 23544 }, { 6361, -6205, 28654 }, { -16617, 6086, -16599 }, { 15466, -5720, 16969 }, { -15928, -9572, -16690 }, { -4685, 27537, -10943 }, { -273, 27291, -12455 } },
    { { 1167, -28151, 10302 }, { 56, -29048, -7496 }, { -1126, -28313, 9855 }, { -1505, -29012, -7486 }, { 14941, -5086, 25512 }, { -6686, -6903, 28419 }, { -14483, -10088, 24258 }, { 6118, -6009, 28748 }, { -16598, 6168, -16603 }, { 15466, -5861, 16967 }, { -15979, -9564, -16655 }, { -4752, 27552, -10877 }, { -51, 27312, -12411 } },
    { { 912, -28203, 10187 }, { 134, -29112, -7244 }, { -1094, -28309, 9869 }, { -1532, -29066, -7267 }, { 14191, -4768, 25998 }, { -6409, -6556, 28565 }, { -13785, -9372, 24943 }, { 5866, -5733, 28857 }, { -16576, 6275, -16600 }, { 15462, -6032, 16975 }, { -16015, -9543, -16625 }, { -4896, 27542, -10837 }, { 259, 27324, -12384 } },
    { { 735, -28237, 10106 }, { 195, -29182, -6955 }, { -1030, -28304, 9891 }, { -1621, -29125, -7009 }, { 13379, -4355, 26496 }, { -6137, -6252, 28692 }, { -13074, -8594, 25597 }, { 5616, -5423, 28966 }, { -16559, 6404, -16597 }, { 15469, -6226, 16985 }, { -16043, -9515, -16598 }, { -5382, 27397, -10975 }, { 619, 27317, -12386 } },
    { { 640, -28245, 10091 }, { 266, -29244, -6687 }, { -911, -28301, 9911 }, { -1794, -29178, -6739 }, { 12482, -3860, 27005 }, { -5840, -5974, 28813 }, { -12322, -7766, 26227 }, { 5419, -5178, 29049 }, { -16544, 6542, -16596 }, { 15501, -6430, 16990 }, { -16062, -9468, -16575 }, { -6110, 27145, -11217 }, { 1084, 27241, -12521 } },
    { { 560, -28248, 10086 }, { 291, -29297, -6448 }, { -807, -28291, 9947 }, { -1978, -29221, -6498 }, { 11587, -3384, 27464 }, { -5521, -5699, 28932 }, { -11520, -6929, 26819 }, { 5231, -5015, 29112 }, { -16525, 6673, -16592 }, { 15566, -6632, 16992 }, { -16066, -9381, -16557 }, { -6803, 26929, -11338 }, { 1632, 27036, -12899 } },
    { { 504, -28245, 10097 }, { 280, -29342, -6244 }, { -792, -28265, 10022 }, { -2087, -29256, -6303 }, { 10810, -2988, 27825 }, { -5244, -5410, 29038 }, { -10665, -6123, 27364 }, { 4954, -4834, 29191 }, { -16503, 6795, -16581 }, { 15651, -6823, 16999 }, { -16053, -9251, -16547 }, { -7320, 26847, -11210 }, { 2058, 26713, -13498 } },
    { { 485, -28232, 10134 }, { 248, -29378, -6073 }, { -855, -28220, 10143 }, { -2113, -29284, -6162 }, { 10138, -2631, 28112 }, { -4978, -5068, 29147 }, { -9768, -5355, 27855 }, { 4561, -4555, 29299 }, { -16491, 6915, -16574 }, { 15741, -6998, 17009 }, { -16023, -9088, -16547 }, { -7663, 26888, -10878 }, { 2153, 26351, -14177 } },
    { { 499, -28212, 10190 }, { 209, -29405, -5943 }, { -916, -28168, 10282 }, { -2111, -29303, -6073 }, { 9403, -2263, 28398 }, { -4588, -4630, 29283 }, { -8883, -4635, 28277 }, { 4121, -4184, 29420 }, { -16498, 7032, -16577 }, { 15832, -7151, 17016 }, { -15983, -8917, -16562 }, { -7886, 27023, -10371 }, { 1923, 26048, -14758 } },
    { { 528, -28199, 10224 }, { 163, -29424, -5850 }, { -934, -28130, 10385 }, { -2122, -29312, -6023 }, { 8523, -1920, 28700 }, { -4019, -4134, 29441 }, { -8095, -4022, 28606 }, { 3704, -3808, 29526 }, { -16515, 7142, -16588 }, { 15921, -7274, 17018 }, { -15941, -8751, -16590 }, { -8112, 27250, -9572 }, { 1575, 25860, -15125 } },
    { { 537, -28212, 10187 }, { 123, -29438, -5777 }, { -949, -28116, 10420 }, { -2133, -29319, -5987 }, { 7691, -1667, 28949 }, { -3474, -3711, 29566 }, { -7449, -3589, 28838 }, { 3377, -3528, 29600 }, { -16519, 7240, -16591 }, { 15990, -7374, 17022 }, { -15907, -8591, -16624 }, { -8498, 27473, -8546 }, { 1363, 25772, -15295 } },
    { { 518, -28254, 10073 }, { 94, -29451, -5711 }, { -1006, -28125, 10392 }, { -2115, -29327, -5954 }, { 7201, -1513, 29084 }, { -3220, -3460, 29625 }, { -6967, -3359, 28986 }, { 3208, -3396, 29634 }, { -16500, 7323, -16576 }, { 16021, -7457, 17034 }, { -15879, -8441, -16656 }, { -9027, 27569, -7647 }, { 1344, 25747, -15339 } },
    { { 506, -28303, 9935 }, { 62, -29461, -5663 }, { -1095, -28142, 10337 }, { -2071, -29334, -5934 }, { 7111, -1424, 29110 }, { -3300, -3362, 29628 }, { -6630, -3271, 29075 }, { 3209, -3373, 29637 }, { -16475, 7387, -16554 }, { 16015, -7523, 17050 }, { -15847, -8311, -16690 }, { -9456, 27550, -7180 }, { 1369, 25753, -15326 } },
    { { 524, -28333, 9846 }, { 19, -29462, -5657 }, { -1179, -28156, 10289 }, { -2018, -29337, -5939 }, { 7213, -1384, 29087 }, { -3523, -3333, 29605 }, { -6395, -3256, 29129 }, { 3330, -3408, 29619 }, { -16459, 7437, -16543 }, { 15985, -7570, 17060 }, { -15809, -8206, -16731 }, { -9572, 27530, -7104 }, { 1328, 25793, -15262 } },
    { { 556, -28331, 9850 }, { -26, -29452, -5709 }, { -1238, -28156, 10282 }, { -1967, -29333, -5973 }, { 7264, -1373, 29075 }, { -3688, -3298, 29589 }, { -6185, -3270, 29173 }, { 3477, -3468, 29595 }, { -16457, 7472, -16552 }, { 15944, -7598, 17057 }, { -15779, -8127, -16770 }, { -9400, 27569, -7181 }, { 1254, 25872, -15135 } },
    { { 580, -28302, 9934 }, { -59, -29435, -5795 }, { -1272, -28138, 10328 }, { -1919, -29326, -6027 }, { 7215, -1364, 29087 }, { -3772, -3251, 29584 }, { -5942, -3314, 29218 }, { 3592, -3557, 29571 }, { -16464, 7495, -16575 }, { 15898, -7611, 17044 }, { -15767, -8077, -16796 }, { -9156, 27629, -7268 }, { 1201, 25977, -14959 } },
    { { 597, -28266, 10035 }, { -80, -29423, -5855 }, { -1292, -28111, 10398 }, { -1879, -29319, -6070 }, { 7124, -1350, 29111 }, { -3823, -3224, 29580 }, { -5663, -3390, 29265 }, { 3673, -3678, 29546 }, { -16478, 7508, -16597 }, { 15850, -7615, 17032 }, { -15769, -8053, -16808 }, { -9009, 27654, -7355 }, { 1140, 26079, -14784 } },
    { { 611, -28242, 10099 }, { -98, -29423, -5855 }, { -1313, -28089, 10454 }, { -1858, -29319, -6077 }, { 6991, -1352, 29143 }, { -3829, -3226, 29579 }, { -5392, -3465, 29307 }, { 3766, -3797, 29519 }, { -16491, 7511, -16601 }, { 15805, -7613, 17029 }, { -15764, -8044, -16815 }, { -8974, 27642, -7443 }, { 990, 26158, -14655 } },
    { { 613, -28236, 10118 }, { -116, -29432, -5807 }, { -1346, -28074, 10489 }, { -1860, -29326, -6046 }, { 6742, -1383, 29200 }, { -3723, -3236, 29592 }, { -5173, -3515, 29341 }, { 3904, -3886, 29490 }, { -16486, 7499, -16580 }, { 15760, -7601, 17041 }, { -15746, -8040, -16820 }, { -9025, 27611, -7496 }, { 860, 26218, -14556 } },
    { { 595, -28234, 10123 }, { -130, -29444, -5748 }, { -1388, -28059, 10526 }, { -1871, -29336, -5992 }, { 6363, -1424, 29283 }, { -3524, -3226, 29617 }, { -5022, -3548, 29363 }, { 4070, -3947, 29459 }, { -16453, 7471, -16542 }, { 15715, -7575, 17059 }, { -15723, -8042, -16819 }, { -9161, 27566, -7495 }, { 983, 26277, -14441 } },
    { { 562, -28226, 10147 }, { -141, -29453, -5702 }, { -1434, -28033, 10586 }, { -1872, -29347, -5936 }, { 5944, -1434, 29370 }, { -3361, -3190, 29640 }, { -4936, -3554, 29377 }, { 4245, -3971, 29432 }, { -16404, 7429, -16503 }, { 15672, -7529, 17071 }, { -15702, -8066, -16809 }, { -9387, 27500, -7459 }, { 1371, 26344, -14287 } },
    { { 527, -28209, 10196 }, { -155, -29460, -5664 }, { -1481, -28003, 10660 }, { -1854, -29359, -5882 }, { 5558, -1408, 29447 }, { -3338, -3161, 29646 }, { -4890, -3518, 29389 }, { 4433, -3962, 29405 }, { -16362, 7379, -16479 }, { 15638, -7465, 17071 }, { -15683, -8116, -16798 }, { -9665, 27420, -7397 }, { 1859, 26423, -14084 } },
    { { 499, -28187, 10259 }, { -171, -29469, -5615 }, { -1518, -27972, 10736 }, { -1831, -29372, -5823 }, { 5237, -1363, 29508 }, { -3476, -3173, 29628 }, { -4864, -3451, 29401 }, { 4636, -3939, 29377 }, { -16341, 7328, -16475 }, { 15611, -7384, 17058 }, { -15657, -8189, -16802 }, { -9982, 27332, -7303 }, { 2376, 26522, -13818 } },
    { { 485, -28167, 10315 }, { -187, -29484, -5538 }, { -1539, -27946, 10801 }, { -1813, -29389, -5746 }, { 4982, -1313, 29554 }, { -3714, -3231, 29593 }, { -4852, -3376, 29412 }, { 4864, -3908, 29344 }, { -16337, 7271, -16485 }, { 15585, -7286, 17039 }, { -15620, -8291, -16829 }, { -10382, 27214, -7184 }, { 2901, 26648, -13470 } },
    { { 494, -28152, 10354 }, { -211, -29503, -5433 }, { -1556, -27930, 10838 }, { -1802, -29410, -5641 }, { 4787, -1261, 29589 }, { -3974, -3312, 29551 }, { -4861, -3309, 29418 }, { 5117, -3854, 29308 }, { -16341, 7199, -16497 }, { 15557, -7168, 17019 }, { -15580, -8419, -16865 }, { -10888, 27048, -7061 }, { 3346, 26804, -13051 } },
    { { 527, -28139, 10389 }, { -254, -29524, -5315 }, { -1573, -27929, 10840 }, { -1797, -29435, -5508 }, { 4645, -1239, 29612 }, { -4226, -3418, 29504 }, { -4902, -3277, 29415 }, { 5388, -3772, 29270 }, { -16346, 7114, -16508 }, { 15530, -7035, 17004 }, { -15550, -8565, -16892 }, { -11423, 26855, -6949 }, { 3602, 26983, -12608 } },
    { { 573, -28120, 10437 }, { -306, -29545, -5196 }, { -1573, -27936, 10821 }, { -1803, -29463, -5356 }, { 4537, -1270, 29628 }, { -4470, -3553, 29452 }, { -4978, -3300, 29400 }, { 5655, -3676, 29232 }, { -16354, 7027, -16520 }, { 15505, -6896, 16986 }, { -15528, -8716, -16913 }, { -11824, 26704, -6861 }, { 3704, 27147, -12220 } },
    { { 619, -28095, 10501 }, { -348, -29564, -5085 }, { -1541, -27944, 10806 }, { -1818, -29488, -5208 }, { 4453, -1349, 29637 }, { -4716, -3706, 29394 }, { -5074, -3370, 29375 }, { 5899, -3579, 29196 }, { -16369, 6935, -16541 }, { 15478, -6752, 16962 }, { -15494, -8876, -16953 }, { -11945, 26661, -6820 }, { 3770, 27248, -11972 } },
    { { 680, -28057, 10599 }, { -361, -29578, -5003 }, { -1496, -27944, 10812 }, { -1839, -29508, -5087 }, { 4403, -1464, 29639 }, { -4954, -3877, 29333 }, { -5157, -3459, 29350 }, { 6119, -3494, 29161 }, { -16392, 6835, -16568 }, { 15448, -6603, 16935 }, { -15435, -9056, -17028 }, { -11730, 26745, -6863 }, { 3894, 27264, -11897 } },
    { { 742, -28015, 10706 }, { -368, -29587, -4945 }, { -1478, -27939, 10827 }, { -1851, -29524, -4991 }, { 4405, -1631, 29630 }, { -5160, -4081, 29270 }, { -5212, -3539, 29331 }, { 6321, -3441, 29124 }, { -16416, 6728, -16591 }, { 15422, -6453, 16914 }, { -15364, -9252, -17126 }, { -11250, 26908, -7027 }, { 4077, 27218, -11940 } },
    { { 784, -27982, 10788 }, { -385, -29595, -4898 }, { -1508, -27940, 10821 }, { -1837, -29539, -4905 }, { 4484, -1856, 29605 }, { -5312, -4326, 29207 }, { -5268, -3590, 29315 }, { 6479, -3412, 29093 }, { -16433, 6614, -16607 }, { 15417, -6301, 16900 }, { -15310, -9448, -17211 }, { -10628, 27078, -7336 }, { 4252, 27148, -12037 } },
    { { 804, -27966, 10830 }, { -397, -29603, -4851 }, { -1570, -27955, 10774 }, { -1800, -29557, -4812 }, { 4639, -2101, 29565 }, { -5415, -4590, 29148 }, { -5385, -3608, 29291 }, { 6565, -3361, 29079 }, { -16438, 6491, -16611 }, { 15441, -6142, 16893 }, { -15284, -9635, -17259 }, { -10028, 27194, -7741 }, { 4343, 27085, -12146 } },
    { { 811, -27964, 10834 }, { -390, -29610, -4808 }, { -1638, -27984, 10686 }, { -1762, -29576, -4708 }, { 4847, -2329, 29514 }, { -5493, -4855, 29091 }, { -5577, -3635, 29252 }, { 6566, -3265, 29090 }, { -16427, 6359, -16607 }, { 15472, -5971, 16890 }, { -15261, -9813, -17284 }, { -9538, 27252, -8145 }, { 4324, 27042, -12249 } },
    { { 819, -27972, 10812 }, { -370, -29615, -4774 }, { -1699, -28025, 10569 }, { -1738, -29594, -4602 }, { 5080, -2549, 29457 }, { -5556, -5120, 29033 }, { -5789, -3725, 29200 }, { 6525, -3149, 29112 }, { -16405, 6220, -16598 }, { 15482, -5788, 16889 }, { -15212, -9984, -17317 }, { -9159, 27287, -8457 }, { 4213, 27017, -12343 } },
    { { 841, -27988, 10770 }, { -353, -29620, -4747 }, { -1748, -28075, 10427 }, { -1722, -29609, -4507 }, { 5352, -2803, 29385 }, { -5590, -5398, 28976 }, { -5972, -3882, 29142 }, { 6510, -3054, 29126 }, { -16388, 6074, -16592 }, { 15463, -5594, 16889 }, { -15144, -10155, -17362 }, { -8850, 27323, -8667 }, { 4040, 27000, -12437 } },
    { { 876, -28013, 10702 }, { -346, -29623, -4727 }, { -1789, -28136, 10255 }, { -1709, -29621, -4435 }, { 5704, -3128, 29286 }, { -5567, -5702, 28922 }, { -6146, -4095, 29077 }, { 6516, -3020, 29128 }, { -16386, 5914, -16585 }, { 15439, -5380, 16891 }, { -15094, -10344, -17395 }, { -8587, 27359, -8819 }, { 3828, 26988, -12530 } },
    { { 912, -28046, 10611 }, { -343, -29624, -4723 }, { -1852, -28194, 10083 }, { -1709, -29628, -4389 }, { 6131, -3524, 29155 }, { -5495, -6046, 28866 }, { -6353, -4360, 28994 }, { 6487, -3063, 29130 }, { -16391, 5740, -16577 }, { 15436, -5146, 16895 }, { -15082, -10575, -17398 }, { -8340, 27391, -8955 }, { 3593, 26979, -12619 } },
    { { 939, -28080, 10518 }, { -336, -29621, -4744 }, { -1956, -28237, 9942 }, { -1727, -29630, -4368 }, { 6578, -3953, 29002 }, { -5399, -6451, 28796 }, { -6589, -4684, 28890 }, { 6411, -3181, 29134 }, { -16393, 5560, -16565 }, { 15458, -4895, 16897 }, { -15094, -10860, -17379 }, { -8102, 27423, -9074 }, { 3364, 26959, -12723 } },
    { { 950, -28116, 10422 }, { -314, -29615, -4780 }, { -2056, -28279, 9801 }, { -1732, -29632, -4353 }, { 6989, -4381, 28844 }, { -5279, -6913, 28711 }, { -6829, -5059, 28771 }, { 6313, -3350, 29136 }, { -16383, 5380, -16553 }, { 15492, -4634, 16894 }, { -15096, -11187, -17365 }, { -7884, 27467, -9133 }, { 3174, 26914, -12866 } },
    { { 959, -28159, 10303 }, { -276, -29610, -4814 }, { -2151, -28328, 9637 }, { -1735, -29637, -4319 }, { 7384, -4807, 28677 }, { -5101, -7384, 28626 }, { -7118, -5479, 28624 }, { 6171, -3539, 29144 }, { -16362, 5201, -16545 }, { 15516, -4363, 16885 }, { -15052, -11525, -17394 }, { -7689, 27538, -9085 }, { 3052, 26861, -13007 } },
    { { 991, -28211, 10157 }, { -238, -29605, -4846 }, { -2230, -28375, 9482 }, { -1718, -29640, -4302 }, { 7843, -5263, 28474 }, { -4860, -7814, 28554 }, { -7520, -5974, 28421 }, { 5901, -3716, 29178 }, { -16331, 5024, -16541 }, { 15523, -4082, 16871 }, { -14945, -11848, -17492 }, { -7447, 27668, -8889 }, { 2906, 26868, -13026 } },
    { { 1031, -28234, 10089 }, { -257, -29586, -4958 }, { -2289, -28401, 9387 }, { -1654, -29637, -4352 }, { 8453, -5764, 28202 }, { -4632, -8205, 28482 }, { -8009, -6576, 28153 }, { 5469, -3883, 29241 }, { -16293, 4851, -16543 }, { 15523, -3791, 16852 }, { -14783, -12160, -17652 }, { -7072, 27885, -8511 }, { 2580, 27006, -12807 } },
    { { 1084, -28261, 10006 }, { -288, -29568, -5063 }, { -2335, -28432, 9283 }, { -1583, -29633, -4403 }, { 9217, -6296, 27846 }, { -4506, -8626, 28378 }, { -8493, -7253, 27844 }, { 4927, -4099, 29307 }, { -16258, 4686, -16545 }, { 15521, -3492, 16834 }, { -14600, -12486, -17831 }, { -6605, 28166, -7939 }, { 2019, 27272, -12335 } },
    { { 1140, -28301, 9887 }, { -326, -29556, -5133 }, { -2367, -28468, 9162 }, { -1513, -29630, -4449 }, { 10067, -6853, 27417 }, { -4477, -9166, 28213 }, { -8898, -7948, 27525 }, { 4350, -4455, 29347 }, { -16232, 4528, -16542 }, { 15516, -3187, 16817 }, { -14440, -12844, -17979 }, { -6236, 28449, -7194 }, { 1355, 27597, -11687 } },
    { { 1199, -28357, 9719 }, { -360, -29552, -5154 }, { -2380, -28515, 9012 }, { -1451, -29628, -4483 }, { 10853, -7488, 26947 }, { -4399, -9856, 27991 }, { -9204, -8651, 27211 }, { 3746, -4987, 29344 }, { -16225, 4380, -16534 }, { 15503, -2881, 16801 }, { -14339, -13214, -18064 }, { -6059, 28694, -6317 }, { 691, 27908, -10985 } },
    { { 1256, -28421, 9521 }, { -384, -29553, -5148 }, { -2372, -28569, 8841 }, { -1402, -29625, -4514 }, { 11467, -8234, 26471 }, { -4167, -10636, 27740 }, { -9435, -9367, 26893 }, { 3096, -5645, 29301 }, { -16235, 4240, -16527 }, { 15489, -2579, 16778 }, { -14304, -13556, -18097 }, { -5994, 28900, -5373 }, { 50, 28160, -10344 } },
    { { 1308, -28483, 9327 }, { -400, -29553, -5144 }, { -2346, -28624, 8669 }, { -1376, -29620, -4554 }, { 11933, -9051, 25994 }, { -3835, -11404, 27482 }, { -9645, -10101, 26551 }, { 2410, -6376, 29215 }, { -16255, 4103, -16525 }, { 15488, -2282, 16747 }, { -14310, -13846, -18116 }, { -5928, 29073, -4430 }, { -518, 28370, -9741 } },
    { { 1362, -28538, 9152 }, { -421, -29551, -5155 }, { -2318, -28675, 8506 }, { -1360, -29614, -4598 }, { 12394, -9860, 25479 }, { -3551, -12115, 27214 }, { -9905, -10895, 26138 }, { 1772, -7184, 29073 }, { -16273, 3969, -16526 }, { 15502, -1994, 16715 }, { -14315, -14093, -18154 }, { -5839, 29215, -3523 }, { -865, 28613, -8976 } },
    { { 1428, -28586, 8989 }, { -461, -29548, -5170 }, { -2305, -28722, 8349 }, { -1335, -29610, -4633 }, { 12940, -10629, 24891 }, { -3389, -12782, 26928 }, { -10277, -11779, 25605 }, { 1300, -8049, 28871 }, { -16281, 3845, -16528 }, { 15530, -1723, 16695 }, { -14304, -14328, -18207 }, { -5762, 29320, -2673 }, { -915, 28923, -7913 } },
    { { 1498, -28631, 8832 }, { -508, -29544, -5186 }, { -2305, -28767, 8195 }, { -1300, -29608, -4657 }, { 13537, -11373, 24236 }, { -3309, -13435, 26618 }, { -10757, -12679, 24970 }, { 1022, -8886, 28636 }, { -16282, 3734, -16530 }, { 15566, -1469, 16688 }, { -14304, -14571, -18232 }, { -5747, 29386, -1850 }, { -825, 29238, -6669 } },
    { { 1553, -28674, 8681 }, { -541, -29540, -5207 }, { -2307, -28807, 8052 }, { -1262, -29605, -4686 }, { 14085, -12088, 23569 }, { -3254, -14072, 26295 }, { -11234, -13484, 24331 }, { 831, -9635, 28398 }, { -16280, 3636, -16529 }, { 15609, -1227, 16688 }, { -14343, -14816, -18206 }, { -5774, 29422, -1013 }, { -824, 29484, -5480 } },
    { { 1588, -28713, 8547 }, { -553, -29533, -5246 }, { -2305, -28843, 7921 }, { -1226, -29600, -4728 }, { 14521, -12753, 22945 }, { -3200, -14673, 25971 }, { -11582, -14160, 23777 }, { 578, -10334, 28158 }, { -16276, 3548, -16528 }, { 15654, -994, 16683 }, { -14363, -15046, -18195 }, { -5761, 29442, -80 }, { -988, 29657, -4416 } },
    { { 1613, -28744, 8437 }, { -554, -29520, -5315 }, { -2311, -28875, 7802 }, { -1180, -29593, -4782 }, { 14840, -13380, 22377 }, { -3127, -15238, 25652 }, { -11806, -14764, 23295 }, { 242, -11032, 27897 }, { -16269, 3466, -16533 }, { 15691, -767, 16666 }, { -14214, -15257, -18354 }, { -5610, 29454, 1007 }, { -1237, 29784, -3371 } },
    { { 1637, -28769, 8345 }, { -550, -29503, -5410 }, { -2326, -28905, 7689 }, { -1123, -29585, -4844 }, { 15047, -14007, 21849 }, { -2995, -15788, 25333 }, { -11979, -15347, 22825 }, { -90, -11729, 27612 }, { -16257, 3388, -16541 }, { 15712, -546, 16640 }, { -13798, -15456, -18782 }, { -5318, 29440, 2235 }, { -1490, 29877, -2273 } },
    { { 1665, -28795, 8250 }, { -544, -29485, -5507 }, { -2345, -28932, 7577 }, { -1064, -29576, -4914 }, { 15144, -14670, 21341 }, { -2749, -16351, 25002 }, { -12149, -15940, 22323 }, { -329, -12422, 27305 }, { -16248, 3316, -16549 }, { 15720, -334, 16611 }, { -13169, -15647, -19426 }, { -5006, 29367, 3540 }, { -1723, 29930, -1109 } },
    { { 1695, -28823, 8147 }, { -537, -29470, -5589 }, { -2356, -28961, 7465 }, { -1014, -29565, -4989 }, { 15146, -15365, 20845 }, { -2414, -16933, 24646 }, { -12312, -16547, 21786 }, { -463, -13118, 26976 }, { -16250, 3256, -16555 }, { 15726, -137, 16584 }, { -12396, -15822, -20220 }, { -4802, 29215, 4838 }, { -1950, 29936, 79 } },
    { { 1724, -28848, 8051 }, { -530, -29455, -5665 }, { -2355, -28988, 7357 }, { -976, -29554, -5063 }, { 15063, -16080, 20360 }, { -2036, -17530, 24260 }, { -12475, -17182, 21194 }, { -550, -13848, 26607 }, { -16262, 3206, -16565 }, { 15742, 35, 16558 }, { -11235, -15976, -21415 }, { -4751, 28986, 6104 }, { -2125, 29897, 1286 } },
    { { 1751, -28870, 7964 }, { -523, -29440, -5743 }, { -2345, -29015, 7254 }, { -947, -29541, -5143 }, { 14919, -16792, 19885 }, { -1669, -18129, 23844 }, { -12633, -17837, 20548 }, { -651, -14612, 26193 }, { -16280, 3154, -16577 }, { 15771, 187, 16533 }, { -8698, -16110, -23996 }, { -4797, 28687, 7352 }, { -2225, 29812, 2512 } },
    { { 1781, -28895, 7867 }, { -521, -29427, -5811 }, { -2334, -29040, 7156 }, { -921, -29525, -5237 }, { 14724, -17491, 19423 }, { -1323, -18727, 23400 }, { -12766, -18485, 19883 }, { -759, -15381, 25746 }, { -16306, 3094, -16583 }, { 15810, 323, 16514 }, { -1965, -16210, -30767 }, { -4859, 28317, 8634 }, { -2300, 29675, 3752 } },
    { { 1810, -28921, 7764 }, { -521, -29414, -5876 }, { -2325, -29064, 7061 }, { -898, -29506, -5347 }, { 14475, -18180, 18973 }, { -1015, -19324, 22925 }, { -12856, -19078, 19255 }, { -867, -16115, 25289 }, { -16341, 3034, -16586 }, { 15847, 447, 16500 }, { 8684, -16179, 24096 }, { -4825, 27864, 10015 }, { -2458, 29480, 4987 } },
    { { 1832, -28941, 7684 }, { -522, -29397, -5961 }, { -2322, -29085, 6975 }, { -877, -29486, -5460 }, { 14175, -18863, 18527 }, { -759, -19917, 22422 }, { -12912, -19602, 18682 }, { -998, -16799, 24835 }, { -16371, 2988, -16593 }, { 15870, 558, 16482 }, { 13786, -16032, 18979 }, { -4599, 27318, 11514 }, { -2762, 29221, 6203 } },
    { { 1838, -28958, 7619 }, { -514, -29378, -6057 }, { -2322, -29105, 6892 }, { -859, -29469, -5553 }, { 13875, -19522, 18066 }, { -541, -20493, 21903 }, { -12936, -20074, 18158 }, { -1160, -17431, 24389 }, { -16384, 2961, -16602 }, { 15875, 654, 16460 }, { 15736, -15848, 17030 }, { -4187, 26683, 13058 }, { -3232, 28897, 7384 } },
    { { 1831, -28977, 7547 }, { -498, -29361, -6141 }, { -2322, -29124, 6811 }, { -842, -29455, -5628 }, { 13620, -20151, 17563 }, { -307, -21052, 21371 }, { -12933, -20536, 17635 }, { -1320, -18034, 23938 }, { -16381, 2947, -16604 }, { 15872, 736, 16441 }, { 16432, -15648, 16350 }, { -3727, 25993, 14507 }, { -3787, 28510, 8534 } },
    { { 1815, -29002, 7457 }, { -474, -29348, -6205 }, { -2323, -29143, 6730 }, { -827, -29444, -5689 }, { 13409, -20755, 17013 }, { -17, -21595, 20824 }, { -12909, -21001, 17098 }, { -1453, -18620, 23477 }, { -16369, 2938, -16597 }, { 15872, 804, 16430 }, { 16539, -15442, 16273 }, { -3333, 25320, 15741 }, { -4323, 28076, 9646 } },
    { { 1801, -29027, 7363 }, { -448, -29335, -6263 }, { -2327, -29162, 6645 }, { -810, -29434, -5741 }, { 13191, -21351, 16435 }, { 325, -22126, 20256 }, { -12880, -21454, 16548 }, { -1567, -19186, 23010 }, { -16353, 2928, -16590 }, { 15877, 859, 16423 }, { 16419, -15246, 16416 }, { -2969, 24716, 16742 }, { -4766, 27621, 10695 } },
    { { 1794, -29047, 7284 }, { -425, -29321, -6332 }, { -2332, -29181, 6562 }, { -791, -29426, -5787 }, { 12928, -21944, 15854 }, { 673, -22638, 19675 }, { -12857, -21883, 15994 }, { -1678, -19721, 22545 }, { -16334, 2918, -16588 }, { 15878, 903, 16418 }, { 16289, -15073, 16551 }, { -2471, 24179, 17586 }, { -5139, 27159, 11661 } },
    { { 1787, -29063, 7223 }, { -400, -29305, -6407 }, { -2334, -29196, 6491 }, { -774, -29417, -5832 }, { 12614, -22519, 15290 }, { 996, -23111, 19102 }, { -12828, -22283, 15457 }, { -1785, -20216, 22094 }, { -16315, 2908, -16590 }, { 15872, 935, 16414 }, { 16248, -14923, 16582 }, { -1719, 23687, 18330 }, { -5511, 26690, 12541 } },
    { { 1770, -29075, 7176 }, { -370, -29290, -6477 }, { -2337, -29208, 6435 }, { -758, -29409, -5879 }, { 12261, -23062, 14759 }, { 1289, -23543, 18549 }, { -12777, -22656, 14949 }, { -1870, -20678, 21655 }, { -16302, 2899, -16591 }, { 15863, 960, 16411 }, { 16302, -14791, 16519 }, { -801, 23198, 19005 }, { -5906, 26204, 13359 } },
    { { 1749, -29086, 7137 }, { -341, -29278, -6534 }, { -2342, -29218, 6388 }, { -744, -29400, -5922 }, { 11858, -23568, 14281 }, { 1577, -23942, 18008 }, { -12706, -23001, 14475 }, { -1922, -21121, 21219 }, { -16297, 2895, -16593 }, { 15859, 977, 16406 }, { 16392, -14679, 16418 }, { 131, 22703, 19610 }, { -6267, 25714, 14125 } },
    { { 1736, -29097, 7094 }, { -315, -29269, -6577 }, { -2345, -29229, 6339 }, { -730, -29394, -5955 }, { 11390, -24037, 13874 }, { 1887, -24323, 17459 }, { -12619, -23324, 14026 }, { -1941, -21558, 20772 }, { -16296, 2896, -16595 }, { 15861, 988, 16399 }, { 16460, -14597, 16336 }, { 991, 22197, 20157 }, { -6561, 25226, 14852 } },
    { { 1734, -29110, 7041 }, { -295, -29262, -6606 }, { -2343, -29241, 6285 }, { -716, -29389, -5980 }, { 10868, -24463, 13544 }, { 2207, -24680, 16912 }, { -12520, -23631, 13595 }, { -1932, -21982, 20323 }, { -16291, 2898, -16594 }, { 15869, 997, 16392 }, { 16475, -14534, 16306 }, { 1770, 21688, 20652 }, { -6882, 24713, 15554 } },
    { { 1740, -29123, 6986 }, { -284, -29257, -6630 }, { -2337, -29252, 6233 }, { -706, -29386, -5999 }, { 10345, -24835, 13275 }, { 2482, -25000, 16397 }, { -12419, -23917, 13181 }, { -1903, -22380, 19888 }, { -16283, 2900, -16592 }, { 15883, 1005, 16385 }, { 16428, -14474, 16350 }, { 2441, 21176, 21110 }, { -7349, 24122, 16252 } },
    { { 1751, -29133, 6942 }, { -276, -29252, -6653 }, { -2328, -29261, 6195 }, { -699, -29382, -6016 }, { 9917, -25149, 13006 }, { 2646, -25283, 15930 }, { -12314, -24184, 12787 }, { -1863, -22747, 19470 }, { -16272, 2904, -16590 }, { 15898, 1008, 16376 }, { 16334, -14404, 16460 }, { 2961, 20636, 21573 }, { -7955, 23436, 16955 } },
};
// clang-format on
// clang-format off
static const Vec3s sGdfClipLidRoot[181] = {
    { 0, 0, 0 },         { 0, -1, -4 },       { -1, -1, -8 },      { -1, -2, -13 },     { -1, -3, -20 },
    { -1, -4, -30 },     { -2, -6, -43 },     { -2, -11, -62 },    { -1, -18, -86 },    { 1, -29, -114 },
    { 4, -44, -145 },    { 8, -65, -179 },    { 11, -90, -213 },   { 13, -120, -245 },  { 15, -152, -274 },
    { 17, -186, -302 },  { 21, -222, -326 },  { 27, -262, -345 },  { 31, -307, -357 },  { 34, -355, -364 },
    { 33, -400, -370 },  { 32, -441, -378 },  { 31, -479, -385 },  { 30, -511, -389 },  { 28, -537, -392 },
    { 26, -559, -392 },  { 27, -578, -390 },  { 29, -600, -386 },  { 31, -628, -378 },  { 34, -663, -365 },
    { 37, -704, -351 },  { 40, -749, -337 },  { 43, -795, -326 },  { 44, -842, -316 },  { 45, -887, -306 },
    { 46, -932, -297 },  { 48, -974, -289 },  { 51, -1013, -284 }, { 52, -1047, -280 }, { 52, -1077, -277 },
    { 49, -1102, -276 }, { 47, -1121, -277 }, { 44, -1133, -279 }, { 41, -1136, -280 }, { 39, -1131, -280 },
    { 37, -1121, -277 }, { 37, -1106, -272 }, { 37, -1088, -267 }, { 39, -1067, -264 }, { 41, -1044, -261 },
    { 43, -1019, -259 }, { 46, -992, -257 },  { 48, -963, -254 },  { 51, -932, -250 },  { 53, -899, -246 },
    { 56, -866, -241 },  { 58, -832, -237 },  { 58, -795, -235 },  { 57, -759, -236 },  { 55, -724, -237 },
    { 52, -691, -238 },  { 50, -656, -239 },  { 49, -614, -243 },  { 45, -566, -251 },  { 40, -524, -257 },
    { 37, -498, -255 },  { 36, -485, -246 },  { 35, -473, -237 },  { 34, -459, -231 },  { 30, -442, -227 },
    { 29, -427, -223 },  { 30, -414, -217 },  { 32, -402, -208 },  { 34, -393, -198 },  { 35, -387, -185 },
    { 38, -384, -172 },  { 40, -384, -159 },  { 43, -388, -148 },  { 45, -395, -139 },  { 46, -405, -133 },
    { 43, -417, -128 },  { 38, -432, -124 },  { 37, -453, -119 },  { 44, -486, -112 },  { 55, -528, -105 },
    { 63, -571, -98 },   { 64, -611, -92 },   { 66, -651, -87 },   { 67, -689, -83 },   { 66, -721, -79 },
    { 64, -745, -75 },   { 60, -757, -68 },   { 55, -754, -59 },   { 50, -735, -46 },   { 47, -701, -29 },
    { 44, -656, -10 },   { 41, -606, 11 },    { 33, -555, 32 },    { 26, -506, 54 },    { 22, -461, 76 },
    { 17, -419, 97 },    { 14, -381, 117 },   { 13, -348, 138 },   { 13, -321, 159 },   { 12, -300, 180 },
    { 12, -284, 200 },   { 11, -270, 216 },   { 13, -263, 228 },   { 13, -258, 235 },   { 11, -255, 237 },
    { 17, -255, 232 },   { 23, -256, 225 },   { 26, -260, 216 },   { 32, -266, 208 },   { 38, -272, 200 },
    { 43, -280, 193 },   { 46, -290, 188 },   { 49, -299, 188 },   { 52, -305, 194 },   { 54, -309, 204 },
    { 56, -310, 216 },   { 57, -312, 227 },   { 59, -316, 234 },   { 61, -320, 238 },   { 62, -325, 237 },
    { 62, -327, 232 },   { 62, -326, 225 },   { 62, -324, 216 },   { 63, -321, 203 },   { 64, -318, 189 },
    { 66, -315, 173 },   { 68, -311, 155 },   { 70, -305, 135 },   { 73, -297, 113 },   { 75, -289, 90 },
    { 76, -281, 66 },    { 76, -274, 42 },    { 76, -267, 19 },    { 78, -259, -3 },    { 79, -250, -23 },
    { 79, -238, -41 },   { 79, -226, -57 },   { 78, -213, -72 },   { 77, -199, -85 },   { 77, -184, -97 },
    { 80, -170, -108 },  { 81, -157, -119 },  { 83, -143, -129 },  { 82, -129, -137 },  { 82, -120, -144 },
    { 80, -110, -150 },  { 78, -100, -154 },  { 74, -89, -157 },   { 69, -78, -158 },   { 65, -68, -159 },
    { 62, -59, -159 },   { 60, -50, -159 },   { 58, -43, -159 },   { 55, -36, -157 },   { 53, -30, -154 },
    { 51, -26, -149 },   { 49, -22, -144 },   { 47, -19, -137 },   { 44, -16, -129 },   { 42, -13, -120 },
    { 40, -11, -112 },   { 38, -9, -105 },    { 35, -6, -97 },     { 34, -5, -87 },     { 34, -4, -76 },
    { 33, -4, -64 },     { 31, -3, -53 },     { 29, -2, -42 },     { 28, -1, -31 },     { 27, 0, -22 },
    { 26, 0, -14 },      { 25, 0, -6 },       { 24, 0, 1 },        { 23, 0, 8 },        { 21, 1, 15 },
    { 19, 1, 20 },
};
// clang-format on

#define GDF_CHEST_KICK_CONTACT 9.0f // the kick's frame the foot meets the lid (its fastest, shin high)
#define GDF_CHEST_LID_FADE 165.0f   // the lid's frame his arms are down again (blended back to his stance by 175)

// The lid: the chest's frames to the clip's. Bent down and gripping as the lid cracks open (21 to 40), lifted and held
// as the light pours out, heaved up and over as it swings open (102 to 110), then on at the clip's speed
static const f32 sGdfChestLidKeys[][2] = { { 0, 4 }, { 24, 43 }, { 40, 70 }, { 92, 80 }, { 112, 121 }, { 116, 125 } };

static void BossGanon_Player_ApplyChestKick(BossGanon* this, PlayState* play) {
    // Play as Ganondorf (2S2H): not ported yet
}

// ---- The Master Sword ----
// Pulling it as a child, he kicks it out of the pedestal instead: the Mixamo kick, his foot meeting the hilt, and it
// shoots over the pedestal and buries its blade in the wall beyond, spinning end over end and arriving point first.
// Seven years on, the adult Temple of Time still has it there. Putting it back (to return to the past), he draws it out
// of the wall with his magic and calls it into his raised fists, then drives it down into the pedestal.
// The pedestal actor (Bg_Toki_Swd) stays where it is (adult Link puts the sword back through it, by its position); the
// sword out of the stone is drawn here, with the pedestal's own model. The wall is found by a line test straight ahead
// of the pedestal, the same each time, so the sword is where it went in.
#define GDF_SWD_SCALE 0.025f
#define GDF_SWD_LIFT 20.0f  // the model's origin above the pedestal actor (its yOffset, 800)
#define GDF_SWD_TIP 19.35f  // the blade's tip below the origin (774 model units)
#define GDF_SWD_GRIP 11.25f // the middle of the grip above it (450)
#define GDF_SWD_SINK 8.0f   // how deep the blade goes into the wall
#define GDF_SWD_FLIGHT 14   // frames from the kick to the wall
#define GDF_SWD_GRAVITY 0.6f
#define GDF_SWD_KICK_CONTACT 13 // frames from the pull to his foot meeting the hilt (the kick's frame 20)
#define GDF_SWD_PULL_FRAMES 6   // drawn out of the wall, shaking
#define GDF_SWD_CALL_FRAMES 16  // flying to his hands
#define GDF_SWD_THRUST_FRAMES 6 // down into the stone

typedef enum {
    GDF_SWD_NONE,   // in the stone (the pedestal draws it), or gone
    GDF_SWD_KICK,   // pulled: in the stone until his foot lands, then flying to the wall
    GDF_SWD_LODGED, // in the wall
    GDF_SWD_RETURN, // adult Link is putting it back: in the wall until he's at the pedestal
    GDF_SWD_SUMMON, // out of the wall and into his hands
    GDF_SWD_HELD,   // over his head
    GDF_SWD_THRUST, // down into the pedestal
} GdfSwdState;

static Actor* sGdfSwd = NULL;
static u8 sGdfSwdState = GDF_SWD_NONE;
static u8 sGdfSwdWasDrawn = false; // in the stone (as a child), so its disappearing is the pull
static u8 sGdfSwdReturn = false;   // adult Link has taken hold of the pedestal to put it back (Bg_Toki_Swd)
static u8 sGdfSwdPlaced = false;   // the game has put it back, waiting on his thrust (Bg_Toki_Swd)
static s16 sGdfSwdTimer = 0;
static s16 sGdfSwdStill = 0;   // frames Link hasn't moved (in place at the pedestal)
static f32 sGdfSwdArms = 0.0f; // his fists raised for it (the item pose)
static Vec3f sGdfSwdPos;       // the model's origin
static Vec3s sGdfSwdRot;
static Vec3f sGdfSwdFrom;
static Vec3s sGdfSwdFromRot;
// The kick that sends it flying: Mixamo's "Great Sword Kick" (a step in and a high front kick with the left foot, then
// back), its arms turned out from the sword's two-handed grip (25 degrees about his front-back axis at each shoulder)
// so his fists are apart, as unarmed. His hips stay put (no root table): the step is in his legs.
// clang-format off
static const Vec3s sGdfClipSwdKick[53][13] = {
    { { 286, -26271, 14484 }, { 3685, -29682, -2318 }, { -7836, -28774, 3262 }, { 1947, -27400, -12061 }, { 12757, -27103, -1638 }, { -4570, -27094, 12044 }, { -11930, -27522, 472 }, { 8036, -20350, 20525 }, { -13323, 3978, -15462 }, { 15426, -3660, 17080 }, { 16663, -14356, 16157 }, { 22587, -4221, 19288 }, { -24411, -934, 17413 } },
    { { 82, -25287, 16142 }, { 3676, -29440, -4447 }, { -8007, -28721, 3318 }, { 1933, -27474, -11892 }, { 12337, -27340, 565 }, { -7358, -26991, 10833 }, { -12993, -27034, 610 }, { 5143, -19803, 21941 }, { -12491, 4045, -15792 }, { 14694, -3258, 16857 }, { 18369, -14645, 14402 }, { 23736, -5144, 17611 }, { -24761, 1497, 16872 } },
    { { -455, -23754, 18318 }, { 3794, -28968, -6817 }, { -8679, -28497, 3551 }, { 2326, -27302, -12214 }, { 11746, -27077, 5372 }, { -11603, -25517, 10689 }, { -13495, -26547, 3627 }, { 2303, -19770, 22446 }, { -11649, 4395, -15985 }, { 13179, -2917, 16795 }, { 18857, -14701, 13830 }, { 23854, -4981, 17498 }, { -25107, 3925, 15945 } },
    { { -1454, -22667, 19598 }, { 4564, -28474, -8269 }, { -9362, -28232, 3911 }, { 2766, -26927, -12934 }, { 9194, -26300, 11127 }, { -16036, -23268, 10072 }, { -13873, -25576, 7307 }, { -1385, -20008, 22311 }, { -10870, 4625, -15866 }, { 11223, -2655, 16850 }, { 18365, -14567, 14173 }, { 22702, -4446, 19101 }, { -25502, 6640, 14337 } },
    { { -2820, -21868, 20343 }, { 6100, -28311, -7828 }, { -9434, -28183, 4091 }, { 2754, -26499, -13792 }, { 3756, -24829, 16414 }, { -19352, -21401, 8213 }, { -15491, -23714, 9882 }, { -5233, -19535, 22158 }, { -10304, 4725, -15790 }, { 9237, -2426, 17008 }, { 17539, -14265, 14844 }, { 19852, -3670, 22191 }, { -25816, 9893, 11648 } },
    { { -4193, -21135, 20874 }, { 7954, -28319, -5897 }, { -8704, -28410, 4133 }, { 2083, -26041, -14750 }, { -3548, -22939, 19005 }, { -20531, -20546, 7505 }, { -18401, -21184, 10614 }, { -7329, -16879, 23694 }, { -9727, 5043, -15580 }, { 7900, -2186, 17289 }, { 16623, -13851, 15717 }, { 15212, -2313, 25754 }, { -25562, 13518, 7992 } },
    { { -5598, -21247, 20426 }, { 8781, -28682, -499 }, { -7322, -28925, 3117 }, { 870, -25635, -15560 }, { -7319, -20459, 20685 }, { -22169, -18870, 7243 }, { -19874, -19214, 11654 }, { -8946, -13607, 25196 }, { -8976, 5153, -15051 }, { 7171, -1754, 17559 }, { 15611, -13459, 16773 }, { 10042, -27, 28269 }, { -24893, 16306, 3802 } },
    { { -7365, -22433, 18507 }, { 7549, -28942, 2314 }, { -6635, -29253, -513 }, { 355, -25804, -15298 }, { -8336, -17825, 22645 }, { -23126, -17177, 8376 }, { -19864, -17880, 13628 }, { -10426, -11934, 25473 }, { -8131, 4382, -14767 }, { 6590, -876, 17649 }, { 14828, -13240, 17542 }, { 6244, 3117, 29177 }, { -24565, 17215, -430 } },
    { { -9155, -24315, 14999 }, { 5736, -29406, 1540 }, { -8151, -28655, -3532 }, { 3339, -26123, -14369 }, { -7046, -15767, 24531 }, { -22962, -16820, 9478 }, { -19542, -17902, 14059 }, { -11324, -12098, 25008 }, { -6814, 3228, -14682 }, { 5962, 206, 17584 }, { 14896, -13208, 17451 }, { 3837, 6444, 29047 }, { -24984, 16016, -4390 } },
    { { -11057, -25797, 10594 }, { 5622, -29456, 840 }, { -12985, -26790, -3700 }, { 9110, -26238, -11339 }, { -5341, -15385, 25194 }, { -21306, -18507, 10175 }, { -19938, -19669, 10751 }, { -10137, -12056, 25532 }, { -4832, 2476, -14297 }, { 5444, 1014, 17510 }, { 14424, -13175, 18211 }, { 2043, 9336, 28437 }, { -25865, 12924, -7999 } },
    { { -13245, -26180, 6259 }, { 8426, -28746, 1640 }, { -16655, -24655, -3841 }, { 13721, -25055, -9164 }, { -1299, -15806, 25465 }, { -20631, -19731, 9224 }, { -19785, -21658, 6282 }, { -8263, -11385, 26497 }, { -2526, 1677, -13976 }, { 5123, 1602, 17478 }, { 12509, -13004, 20749 }, { 586, 11862, 27549 }, { -26644, 8158, -11116 } },
    { { -14282, -26283, 2289 }, { 10266, -28035, 2940 }, { -18484, -23359, -3563 }, { 18026, -23160, -6219 }, { 3630, -15722, 25291 }, { -21397, -18643, 9725 }, { -18890, -22880, 4434 }, { -6716, -10171, 27412 }, { -857, 658, -13765 }, { 4982, 2128, 17453 }, { 9248, -12407, 24765 }, { -458, 14667, 26166 }, { -26889, 1978, -13156 } },
    { { -14145, -26453, -402 }, { 9804, -28166, 3254 }, { -18222, -23625, -3135 }, { 20219, -22158, -428 }, { 7638, -14375, 25200 }, { -22931, -15552, 11502 }, { -18077, -23270, 5632 }, { -5664, -8303, 28266 }, { -489, -168, -13705 }, { 5044, 2884, 17531 }, { 6858, -11537, 27826 }, { -1152, 18250, 23782 }, { -26401, -4916, -13373 } },
    { { -13278, -26863, -1435 }, { 7905, -28876, 1921 }, { -15104, -25822, -2260 }, { 19509, -22175, 5262 }, { 9427, -11841, 25902 }, { -24249, -13027, 11929 }, { -17896, -22884, 7488 }, { -5116, -5561, 29033 }, { -528, -1126, -13589 }, { 5536, 3827, 17829 }, { 5366, -10738, 29776 }, { -2091, 22003, 20285 }, { -25323, -11205, -11541 } },
    { { -12551, -27218, -1277 }, { 6397, -29197, 2569 }, { -11914, -27520, -849 }, { 17531, -23344, 6908 }, { 8172, -7825, 27785 }, { -24470, -13643, 10728 }, { -18536, -21998, 8516 }, { -5115, -1816, 29505 }, { -624, -2396, -13668 }, { 6469, 4925, 18456 }, { 4599, -10078, 30748 }, { -3974, 24727, 16516 }, { -23988, -15817, -8627 } },
    { { -13465, -26805, 423 }, { 5880, -29147, 3986 }, { -11412, -27744, -199 }, { 16321, -23663, 8583 }, { 3579, -3023, 29632 }, { -23137, -16774, 9127 }, { -19885, -20874, 8300 }, { -6094, 2177, 29294 }, { -982, -4113, -13451 }, { 7366, 6115, 19362 }, { 4544, -9447, 30815 }, { -6668, 26085, 13233 }, { -22575, -18733, -6278 } },
    { { -17879, -23978, 2329 }, { 8926, -28159, 5231 }, { -10615, -28057, -331 }, { 15574, -23711, 9759 }, { -3732, -123, 29767 }, { -20888, -20134, 7635 }, { -21614, -19745, 6554 }, { -8472, 5209, 28304 }, { -1800, -5752, -12605 }, { 7690, 7043, 20217 }, { 4753, -8590, 30652 }, { -9831, 26266, 10652 }, { -21173, -20529, -5499 } },
    { { -21240, -20748, 4290 }, { 11899, -26138, 8672 }, { -10092, -28252, -17 }, { 14716, -24967, 7750 }, { -11253, 198, 27809 }, { -18717, -22780, 5543 }, { -22875, -19100, 3452 }, { -12116, 7003, 26536 }, { -2687, -6798, -11800 }, { 7514, 8025, 21261 }, { 4803, -7500, 30809 }, { -13085, 25602, 8564 }, { -19618, -21813, -6272 } },
    { { -23401, -17342, 7186 }, { 14780, -22007, 14045 }, { -10696, -28026, -380 }, { 14192, -25817, 5664 }, { -16967, -2223, 24641 }, { -16395, -25027, 2204 }, { -22377, -19970, -700 }, { -16560, 7428, 23887 }, { -4428, -7757, -10408 }, { 6880, 8979, 22205 }, { 4623, -6437, 31180 }, { -16396, 24188, 6790 }, { -17657, -22951, -7842 } },
    { { -23859, -13897, 11731 }, { 18799, -16746, 16315 }, { -9975, -28269, -1178 }, { 12805, -26724, 4672 }, { -20055, -6409, 21371 }, { -13439, -26797, -1145 }, { -19336, -22389, -4982 }, { -20407, 7512, 20667 }, { -5817, -8834, -8919 }, { 6267, 9856, 23058 }, { 4487, -5538, 31476 }, { -20864, 20823, 5578 }, { -15167, -24143, -9333 } },
    { { -20656, -11653, 18373 }, { 23191, -9663, 16395 }, { -8299, -28761, -1988 }, { 10660, -27642, 4718 }, { -20540, -9763, 19565 }, { -9287, -28305, -3549 }, { -13577, -25503, -8080 }, { -22888, 7627, 17832 }, { -6549, -9816, -7530 }, { 6021, 10672, 24081 }, { 4600, -4745, 31670 }, { -25525, 15142, 4383 }, { -11909, -25504, -10379 } },
    { { -14954, -8748, 24492 }, { 23694, -1204, 18361 }, { -7208, -29010, -2543 }, { 9352, -27939, 5653 }, { -19848, -10430, 19931 }, { -4018, -29452, -4057 }, { -5530, -28233, -8504 }, { -24460, 6797, 15985 }, { -7203, -10506, -5776 }, { 5588, 11297, 24856 }, { 4883, -3921, 32036 }, { -28826, 7708, 3110 }, { -7321, -26910, -11057 } },
    { { -9391, -4334, 28161 }, { 18255, 5670, 23121 }, { -7907, -28813, -2699 }, { 9560, -27523, 7149 }, { -19120, -9078, 21260 }, { 1321, -29899, -2076 }, { 3353, -29292, -5546 }, { -25592, 3582, 15238 }, { -7320, -10845, -4578 }, { 4729, 11524, 25451 }, { 5090, -3012, -32702 }, { -29952, -122, 1687 }, { -1550, -27762, -11263 } },
    { { -3340, 1026, 29796 }, { 11322, 8932, 26307 }, { -9417, -28347, -2783 }, { 10561, -26862, 8178 }, { -18373, -7140, 22615 }, { 4849, -29554, 1751 }, { 10924, -27935, -582 }, { -25598, -2315, 15471 }, { -7418, -10890, -4047 }, { 3549, 11511, 25835 }, { 5128, -2185, -31606 }, { -29420, -5868, 30 }, { 4545, -27516, -11055 } },
    { { -21, 4634, 29640 }, { 7955, 9523, 27314 }, { -10578, -27932, -2814 }, { 11435, -26422, 8433 }, { -17554, -5438, 23713 }, { 5572, -28974, 5426 }, { 15081, -25518, 4624 }, { -23803, -7793, 16513 }, { -7931, -10324, -4126 }, { 2849, 11399, 26323 }, { 5133, -1835, -30650 }, { -28765, -8337, -1758 }, { 9246, -26572, -10413 } },
    { { -3646, 4767, 29394 }, { 13643, 8986, 25162 }, { -11521, -27515, -3194 }, { 11982, -26241, 8235 }, { -16908, -4094, 24441 }, { 4578, -28534, 8054 }, { 15928, -23961, 8494 }, { -21468, -10675, 18033 }, { -9006, -9849, -4063 }, { 2854, 11103, 26702 }, { 5311, -2278, -30139 }, { -28807, -7690, -3317 }, { 11456, -26062, -9462 } },
    { { -8243, 2854, 28704 }, { 20201, 6039, 21341 }, { -11476, -27528, -3239 }, { 11312, -26737, 7560 }, { -16300, -3578, 24930 }, { 2953, -28112, 10049 }, { 14235, -24248, 10460 }, { -19886, -10700, 19750 }, { -8758, -10153, -5056 }, { 3883, 10761, 27436 }, { 5609, -3346, -30069 }, { -29264, -4991, -4323 }, { 10933, -26663, -8340 } },
    { { -13079, 49, 26999 }, { 24879, 1395, 16706 }, { -11115, -27671, -3286 }, { 10100, -27439, 6715 }, { -15802, -4018, 25182 }, { 818, -27586, 11762 }, { 10381, -25932, 10943 }, { -18912, -9109, 21433 }, { -8586, -9338, -4985 }, { 5138, 10411, 28015 }, { 5724, -4588, -30285 }, { -29668, -1505, -4189 }, { 8675, -27732, -7461 } },
    { { -17042, -2546, 24558 }, { 27024, -4938, 12054 }, { -11494, -27426, -3966 }, { 9495, -27771, 6214 }, { -15256, -5191, 25304 }, { -1746, -27251, 12422 }, { 5279, -27481, 10814 }, { -17494, -7607, 23154 }, { -8088, -9170, -5007 }, { 5998, 9902, 28130 }, { 5535, -5700, -30690 }, { -29877, 1146, -2465 }, { 5797, -28537, -7215 } },
    { { -19394, -5546, 22206 }, { 26435, -11157, 8758 }, { -12824, -26725, -4616 }, { 10105, -27582, 6093 }, { -14552, -6341, 25456 }, { -3964, -26922, 12629 }, { 1023, -28123, 10394 }, { -15556, -7599, 24500 }, { -8407, -8861, -4534 }, { 6272, 9147, 27641 }, { 5271, -6655, -31371 }, { -29857, 2836, 731 }, { 2954, -28822, -7784 } },
    { { -20680, -9771, 19413 }, { 24084, -15740, 8498 }, { -14417, -26012, -3937 }, { 11679, -26971, 6012 }, { -13312, -7223, 25896 }, { -5395, -26843, 12261 }, { -1423, -28494, 9279 }, { -13668, -8965, 25156 }, { -8433, -7817, -5125 }, { 6166, 8099, 26719 }, { 5159, -7602, -32338 }, { -29363, 3655, 4945 }, { 158, -28645, -8911 } },
    { { -20475, -14678, 16289 }, { 20391, -20058, 9050 }, { -15257, -25742, -2139 }, { 13105, -26330, 5917 }, { -10932, -8061, 26749 }, { -6574, -27243, 10705 }, { -2531, -29000, 7253 }, { -12299, -11026, 25043 }, { -8073, -6711, -6616 }, { 5982, 6945, 25739 }, { 5227, -8664, 32142 }, { -28194, 4017, 9431 }, { -2851, -28146, -9983 } },
    { { -18950, -18704, 13824 }, { 15522, -24603, 7333 }, { -15343, -25775, -487 }, { 13940, -25789, 6372 }, { -7531, -9551, 27424 }, { -7707, -27802, 8223 }, { -3116, -29486, 4569 }, { -11695, -13369, 24177 }, { -7421, -5642, -8103 }, { 5817, 5822, 24773 }, { 5401, -9759, 31224 }, { -26372, 4610, 13537 }, { -6256, -27441, -10386 } },
    { { -17012, -21552, 12088 }, { 11399, -27375, 4546 }, { -14922, -26025, 221 }, { 14020, -25344, 7818 }, { -3860, -12095, 27181 }, { -9032, -28085, 5446 }, { -4016, -29657, 2078 }, { -11636, -16072, 22501 }, { -6579, -4257, -8527 }, { 5607, 4696, 23896 }, { 5622, -10643, 30501 }, { -24181, 5806, 16779 }, { -9931, -26555, -9808 } },
    { { -15327, -23569, 10467 }, { 9692, -28354, 1450 }, { -13747, -26663, 336 }, { 14132, -24733, 9411 }, { -931, -15233, 25828 }, { -10792, -27817, 3126 }, { -5650, -29447, 967 }, { -11861, -19241, 19726 }, { -5225, -2537, -9272 }, { 5183, 3516, 23094 }, { 5917, -11144, 29799 }, { -21864, 7512, 19119 }, { -13405, -25377, -8736 } },
    { { -15043, -24332, 9037 }, { 9067, -28511, -2217 }, { -14175, -26430, -716 }, { 15344, -23568, 10444 }, { 641, -17816, 24128 }, { -12900, -27044, 1487 }, { -7756, -28917, 1914 }, { -12025, -22066, 16386 }, { -4278, -701, -10188 }, { 4619, 2259, 22423 }, { 6355, -11300, 28877 }, { -19388, 9116, 21000 }, { -16551, -23687, -8063 } },
    { { -16113, -23748, 8741 }, { 10316, -27866, -4129 }, { -16808, -24659, -3076 }, { 17396, -22565, 9391 }, { 1588, -19483, 22757 }, { -15135, -25876, 1167 }, { -9675, -28095, 4129 }, { -12141, -23802, 13640 }, { -3748, 793, -10843 }, { 4233, 1006, 21645 }, { 6956, -11360, 27635 }, { -16066, 10261, 23165 }, { -19726, -21010, -8334 } },
    { { -17517, -22399, 9564 }, { 12581, -27011, -3483 }, { -18978, -22716, -4880 }, { 19547, -21494, 7478 }, { 2643, -20770, 21486 }, { -16716, -24844, 1830 }, { -10481, -27612, 5265 }, { -12451, -23567, 13768 }, { -3693, 1712, -11056 }, { 4366, -63, 21022 }, { 7644, -11594, 26211 }, { -11312, 10693, 25646 }, { -23271, -17166, -7987 } },
    { { -17833, -21547, 10848 }, { 13302, -26746, -2773 }, { -21388, -20114, -6163 }, { 19022, -22670, 4922 }, { 3931, -22092, 19913 }, { -17246, -24453, 2153 }, { -10743, -27704, 4134 }, { -12251, -21707, 16695 }, { -3846, 2231, -11295 }, { 4986, -949, 20570 }, { 8426, -12120, 24743 }, { -4602, 11036, 27514 }, { -26637, -12880, -4958 } },
    { { -17094, -21126, 12708 }, { 13100, -26849, -2747 }, { -22049, -19199, -6726 }, { 18165, -23870, 485 }, { 4876, -23741, 17680 }, { -16706, -24807, 2351 }, { -11776, -27443, 2861 }, { -10837, -20344, 19202 }, { -4097, 2697, -11782 }, { 5826, -1600, 20107 }, { 9606, -12821, 23016 }, { 3807, 11058, 27627 }, { -28565, -9163, 327 } },
    { { -15669, -20823, 14863 }, { 11994, -27358, -2767 }, { -21610, -19611, -6958 }, { 15098, -25482, -4768 }, { 4906, -25573, 14899 }, { -15326, -25611, 3037 }, { -13072, -26871, 2658 }, { -8829, -19955, 20588 }, { -4569, 3091, -12183 }, { 6642, -2106, 19703 }, { 11488, -13383, 20732 }, { 11532, 9256, 26102 }, { -28840, -6628, 4934 } },
    { { -13698, -20436, 17167 }, { 10142, -28125, -2476 }, { -19923, -21298, -7035 }, { 10474, -26823, -8416 }, { 4787, -26920, 12344 }, { -13878, -26170, 4751 }, { -14053, -26298, 3301 }, { -6537, -20324, 21076 }, { -5189, 3612, -12485 }, { 7361, -2531, 19335 }, { 13547, -13584, 18411 }, { 15916, 6760, 24515 }, { -28794, -4998, 6777 } },
    { { -11078, -20131, 19288 }, { 7813, -28934, -1333 }, { -18409, -22842, -6271 }, { 6914, -26626, -11969 }, { 5369, -27563, 10558 }, { -12751, -26307, 6736 }, { -14396, -26011, 4025 }, { -4225, -21201, 20800 }, { -6002, 4104, -12893 }, { 8087, -2919, 18880 }, { 14813, -13521, 17032 }, { 17849, 5124, 23562 }, { -28764, -3827, 7615 } },
    { { -7815, -20293, 20667 }, { 5393, -29491, 1107 }, { -15914, -25075, -4239 }, { 3771, -25326, -15633 }, { 6817, -27620, 9522 }, { -12275, -26223, 7853 }, { -14017, -26176, 4281 }, { -2347, -21966, 20298 }, { -7052, 4434, -13534 }, { 9002, -3258, 18254 }, { 15198, -13370, 16650 }, { 19441, 3562, 22569 }, { -28557, -2821, 8749 } },
    { { -4578, -21328, 20595 }, { 3109, -29375, 5236 }, { -13107, -26771, -3396 }, { 1534, -25185, -16229 }, { 8456, -27322, 9056 }, { -12224, -26172, 8100 }, { -13276, -26584, 4131 }, { -710, -22185, 20182 }, { -8252, 4637, -14071 }, { 10128, -3533, 17682 }, { 15435, -13204, 16420 }, { 21080, 1897, 21261 }, { -28123, -1662, 10312 } },
    { { -1837, -23860, 18092 }, { 1174, -27665, 11543 }, { -11980, -27455, -1641 }, { 1610, -25472, -15767 }, { 9276, -27099, 8922 }, { -11663, -26464, 7975 }, { -12523, -26953, 4088 }, { 1117, -21998, 20368 }, { -9766, 4900, -14487 }, { 11287, -3824, 17344 }, { 15727, -13068, 16098 }, { 22386, 464, 19966 }, { -27322, -483, 12381 } },
    { { -1947, -23216, 18900 }, { 3290, -28170, 9778 }, { -11317, -27780, -425 }, { 2039, -25898, -15005 }, { 9526, -27105, 8637 }, { -10278, -26974, 8172 }, { -11915, -27211, 4195 }, { 2886, -21734, 20477 }, { -11180, 5215, -14972 }, { 12261, -3965, 17182 }, { 15864, -13047, 15927 }, { 23177, -636, 19038 }, { -26444, 556, 14157 } },
    { { -2214, -22709, 19479 }, { 5561, -28872, 5959 }, { -10643, -28044, 499 }, { 2259, -26348, -14166 }, { 9773, -27222, 7966 }, { -8626, -27447, 8502 }, { -11458, -27409, 4180 }, { 4183, -21647, 20345 }, { -11899, 4952, -15174 }, { 12973, -4013, 17061 }, { 16009, -13167, 15776 }, { 23392, -1605, 18714 }, { -25855, 1170, 15171 } },
    { { -1301, -23748, 18285 }, { 5318, -29523, -357 }, { -9891, -28284, 1476 }, { 2244, -26775, -13344 }, { 10205, -27408, 6684 }, { -7214, -27760, 8795 }, { -11251, -27558, 3737 }, { 4923, -21758, 20059 }, { -12121, 4556, -15359 }, { 13496, -4025, 16929 }, { 16220, -13369, 15643 }, { 23243, -2515, 18800 }, { -25617, 1222, 15565 } },
    { { -178, -25269, 16169 }, { 4313, -29333, -4579 }, { -9073, -28508, 2230 }, { 2108, -27131, -12627 }, { 10797, -27633, 4452 }, { -5952, -27827, 9498 }, { -11290, -27654, 2793 }, { 5510, -21836, 19820 }, { -12368, 4411, -15514 }, { 13977, -3973, 16872 }, { 16570, -13596, 15474 }, { 22977, -3289, 19006 }, { -25523, 865, 15743 } },
    { { 159, -25787, 15329 }, { 3805, -29475, -4092 }, { -8418, -28663, 2751 }, { 1994, -27329, -12212 }, { 11620, -27595, 1863 }, { -4897, -27695, 10440 }, { -11378, -27706, 1709 }, { 6403, -21688, 19714 }, { -12737, 4284, -15526 }, { 14490, -3888, 16914 }, { 17062, -13836, 15230 }, { 22767, -3870, 19149 }, { -25311, 275, 16102 } },
    { { 173, -26121, 14754 }, { 3839, -29615, -2869 }, { -7956, -28760, 3090 }, { 1942, -27403, -12054 }, { 12509, -27266, -303 }, { -4341, -27437, 11330 }, { -11484, -27700, 897 }, { 7464, -21208, 19862 }, { -13148, 4126, -15523 }, { 15009, -3788, 16987 }, { 17319, -14092, 15234 }, { 22658, -4209, 19206 }, { -24914, -373, 16708 } },
    { { 286, -26271, 14484 }, { 3685, -29682, -2318 }, { -7836, -28774, 3262 }, { 1947, -27400, -12061 }, { 12757, -27103, -1638 }, { -4571, -27094, 12044 }, { -11930, -27522, 472 }, { 8036, -20350, 20525 }, { -13323, 3978, -15462 }, { 15426, -3660, 17080 }, { 16663, -14356, 16157 }, { 22587, -4221, 19288 }, { -24411, -934, 17413 } },
};
// clang-format on

static Vec3f sGdfSwdWall; // lodged
static Vec3f sGdfSwdHit;  // where its blade went into the wall, and the wall's way out there (the crack and smoke)
static Vec3f sGdfSwdHitNormal;
static u8 sGdfSwdCracked = false;
// The shot of it flying into the wall: frames since the kick began (-1: none)
static s16 sGdfSwdShow = -1;
#define GDF_SWD_SHOW_FRAMES (GDF_SWD_KICK_CONTACT + GDF_SWD_FLIGHT + 34)
#define GDF_SWD_STAND_DIST 48.0f // he kicks from this far from the stone (Link stands close; the kick reaches out)
static f32 sGdfSwdKickClip = -1.0f;
static Vec3s sGdfSwdWallRot;
static f32 sGdfSwdFlightVy; // the kick's flight: its first upward speed

void BgTokiSwd_Draw(Actor* thisx, PlayState* play);
static const Vec3s sGdfItemPoses[2][3];

// From the pedestal (Bg_Toki_Swd): adult Link takes hold of it to put the sword back

// ... and puts it back. True: he still has it out of the stone, so the pedestal doesn't draw it yet (his thrust does)

// Where the kick sends it: the wall straight ahead of the pedestal (as Link faces it), some way up. Its flight is a
// throw over GDF_SWD_FLIGHT frames, and it goes in along the way it arrives

static void BossGanon_Player_MasterSwordKick(BossGanon* this, PlayState* play) {
    // Play as Ganondorf (2S2H): not ported yet
}

// The sword out of the stone: kicked, in the wall, called back and held
// The crack round the hole the blade made: a little decal on the wall's face

// The pull: he stands back from the stone, so his kick reaches the hilt (Link steps right up to it)
static void BossGanon_Player_SwdKeepBack(PlayState* play, Vec3f* target) {
    // Play as Ganondorf (2S2H): not ported yet
}

// The cutscene's camera while the sword flies (PlayAsGanondorf_AdjustCutsceneView): from the side of its flight, the
// pedestal, him and the wall all in view, until a moment after it's in the wall. Eased in and out of the cutscene's
// own shot.

// Drawn by the pedestal's own actor (z_bg_toki_swd.c), not from his draw: it stays in the wall while he's hidden (in
// first person, say)

static void BossGanon_Player_ApplyDoorKick(BossGanon* this, PlayState* play) {
    // Play as Ganondorf (2S2H): not ported yet
}

// Sprinting, from motion capture: a jogging man's run cycle from the CMU Graphics Lab Motion Capture Database
// (mocap.cs.cmu.edu, clips 16_35 and 16_36; the database was created with funding from NSF EIA-0196217; free for all
// uses), averaged over its two runs and retargeted to him. For
// each limb only its swing forward and back is taken (the angle off straight down, in his body's side view): his own
// spread (broad arms, wide stance) and twist stay his. The heel kicks up a little less than the runner's, for his
// weight. Phase 0 is the right heel coming down; tenths of a degree, forward positive.
#define GDF_MOCAP_RUN_SAMPLES 32

// Left thigh, left shin, right thigh, right shin, left upper arm, left forearm, right upper arm, right forearm
// clang-format off
static const s16 sGdfMocapRun[GDF_MOCAP_RUN_SAMPLES][8] = {
    { -158, -493, 292, -24, -199, 889, -414, 749 },   { -135, -539, 272, -23, -222, 843, -406, 766 },
    { -99, -573, 246, -33, -235, 818, -382, 791 },    { -53, -597, 217, -57, -248, 794, -355, 817 },
    { -1, -610, 188, -95, -260, 773, -332, 844 },     { 53, -611, 159, -142, -271, 757, -310, 873 },
    { 107, -599, 127, -191, -283, 742, -285, 904 },   { 158, -571, 91, -234, -297, 726, -256, 938 },
    { 205, -525, 48, -266, -315, 712, -226, 973 },    { 245, -463, -3, -291, -335, 705, -205, 1009 },
    { 274, -392, -54, -314, -352, 704, -193, 1042 },  { 292, -314, -99, -339, -367, 707, -184, 1071 },
    { 301, -236, -133, -367, -381, 709, -177, 1090 }, { 301, -165, -157, -395, -392, 710, -172, 1097 },
    { 296, -108, -170, -422, -399, 713, -173, 1091 }, { 285, -63, -173, -450, -402, 720, -178, 1078 },
    { 274, -33, -162, -483, -400, 730, -183, 1059 },  { 265, -22, -138, -517, -391, 743, -190, 1035 },
    { 256, -30, -101, -549, -378, 760, -198, 1006 },  { 243, -53, -51, -576, -359, 781, -209, 971 },
    { 227, -89, 5, -594, -338, 802, -226, 931 },      { 208, -136, 63, -599, -316, 825, -246, 892 },
    { 181, -187, 120, -591, -291, 850, -267, 852 },   { 141, -234, 175, -565, -262, 877, -288, 811 },
    { 95, -273, 226, -520, -232, 902, -310, 774 },    { 46, -304, 268, -456, -208, 926, -333, 750 },
    { -7, -326, 297, -378, -191, 949, -353, 740 },    { -60, -343, 313, -296, -180, 967, -369, 736 },
    { -107, -361, 318, -216, -172, 980, -382, 736 },  { -141, -382, 318, -144, -166, 985, -390, 737 },
    { -161, -407, 313, -84, -162, 982, -395, 740 },   { -167, -443, 305, -43, -171, 951, -404, 743 },
};
// clang-format on

// His stride is centred under his hips: the runner's legs reach on average 11 degrees ahead of straight down (under a
// torso leaning forward as far), which on him - his torso leans by its own amount - read as leaning back. Legs swing
// this much further back, arms this much further forward.
#define GDF_MOCAP_LEG_SHIFT_DEG -10.0f
#define GDF_MOCAP_ARM_SHIFT_DEG 8.0f

// Turns `limb` (a limb's rotation in his model space; the bone runs along its x) so the bone swings to `pitch`, keeping
// how far out to the side it already leans; by `weight` of the way
static void BossGanon_Player_SwingBoneToward(GdfMat3 limb, const f32 want[3], f32 weight);

// Turns `limb` (its bone along its x) so the bone points along `want` (a unit vector in his model space), by `weight`
static void BossGanon_Player_SwingBoneToward(GdfMat3 limb, const f32 want[3], f32 weight) {
    f32 cur[3] = { limb[0][0], limb[1][0], limb[2][0] };
    f32 axis[3] = { cur[1] * want[2] - cur[2] * want[1], cur[2] * want[0] - cur[0] * want[2],
                    cur[0] * want[1] - cur[1] * want[0] };
    f32 len = sqrtf(SQ(axis[0]) + SQ(axis[1]) + SQ(axis[2]));
    f32 dot = CLAMP(cur[0] * want[0] + cur[1] * want[1] + cur[2] * want[2], -1.0f, 1.0f);
    f32 angle = atan2f(len, dot) * weight;
    f32 c;
    f32 sn;
    f32 v;
    GdfMat3 rot;
    GdfMat3 out;

    if (len < 1e-5f) {
        return;
    }
    axis[0] /= len;
    axis[1] /= len;
    axis[2] /= len;
    c = cosf(angle);
    sn = sinf(angle);
    v = 1.0f - c;
    rot[0][0] = c + axis[0] * axis[0] * v;
    rot[0][1] = axis[0] * axis[1] * v - axis[2] * sn;
    rot[0][2] = axis[0] * axis[2] * v + axis[1] * sn;
    rot[1][0] = axis[1] * axis[0] * v + axis[2] * sn;
    rot[1][1] = c + axis[1] * axis[1] * v;
    rot[1][2] = axis[1] * axis[2] * v - axis[0] * sn;
    rot[2][0] = axis[2] * axis[0] * v - axis[1] * sn;
    rot[2][1] = axis[2] * axis[1] * v + axis[0] * sn;
    rot[2][2] = c + axis[2] * axis[2] * v;
    BossGanon_Pg_Mul(rot, limb, out, false);
    memcpy(limb, out, sizeof(GdfMat3));
}

// A two-bone chain (thigh and shin, or upper arm and forearm) under `parent`, swung to the run cycle's angles

// ---- Summoning the trident ----
// D-Pad Left into the trident stance: Ganondorf thrusts his right arm up over his head, and the trident explodes into
// existence in his fist with a crack of thunder - dark fire, sparks and light flying off it - then he brings it down
// into his stance. Drawn from the start (no drawing it from his side first).
#define GDF_SUMMON_FRAMES 26
#define GDF_SUMMON_BURST_FRAME 9

// The burst (on his screen, `own`, with his own flair on top: his shock and rays of light; the others' screens get the
// fire, sparks, sound and shake)

void BossGanon_Player_SummonTrident(BossGanon* this, PlayState* play) {
    // Play as Ganondorf (2S2H): not ported yet
}

// D-Pad Left out of the trident stance (outside Invasion games): the summon in reverse. He raises the trident over his
// head and it comes apart into dark fire and sparks drawn in to his fist, then his empty hand comes down.

// The trident coming apart (on his screen, `own`, with his shock over it too)

void BossGanon_Player_UnsummonTrident(BossGanon* this, PlayState* play) {
    // Play as Ganondorf (2S2H): not ported yet
}

// ---- The trident's moves, from motion capture ----
// Three of Mixamo's sword-and-shield clips (Adobe, free to use in games), retargeted offline to his skeleton: its slash
// (his first two hits: the second is the slash played back, a rising backhand), its running spin attack (his spin) and
// its run (how he runs with the trident), and its stab. Per frame: where his thighs, shins, upper arms and forearms
// point (x 30000, in his model space, facing +z) - left thigh, left shin, right thigh, right shin, left upper arm, left
// forearm, right upper arm, right forearm - then his pelvis, chest and head as rotations in his model space, and which
// way the backs of his left and right hands face (x 30000; none for his right hand, which keeps his grip on the
// trident). His shoulders keep their breadth.
// clang-format off
static const Vec3s sGdfClipSlash[46][13] = {
    { { -991, -19568, 22718 }, { 4794, -29574, 1544 }, { -14664, -24194, -9981 }, { 8339, -24992, -14347 }, { 8749, -24256, 15333 }, { -28466, -2950, 9000 }, { -2404, -23224, -18838 }, { -25961, -13301, 7008 }, { -5884, 2902, -12436 }, { 5701, -1206, 17726 }, { 16663, -14356, 16157 }, { 5549, 1325, 29453 }, { 0, 0, 0 } },
    { { -1302, -19768, 22529 }, { 4807, -29542, 2040 }, { -15095, -24032, -9728 }, { 8362, -25061, -14214 }, { 7630, -22354, 18495 }, { -26148, -5318, 13711 }, { -4785, -22718, -18999 }, { -26747, -8943, 10229 }, { -5813, 2733, -12177 }, { 5835, -876, 17961 }, { 16258, -13878, 16591 }, { 8816, 6899, 27833 }, { 0, 0, 0 } },
    { { -1724, -19913, 22372 }, { 4805, -29541, 2065 }, { -15365, -23848, -9757 }, { 8110, -25122, -14252 }, { 6729, -19424, 21850 }, { -21299, -6829, 19993 }, { -7854, -21775, -19083 }, { -26609, -4081, 13240 }, { -5766, 2569, -11914 }, { 5807, -602, 18179 }, { 15427, -13499, 17338 }, { 11533, 10682, 25552 }, { 0, 0, 0 } },
    { { -2291, -19969, 22271 }, { 5105, -29501, 1906 }, { -15543, -23649, -9956 }, { 7682, -25194, -14362 }, { 6061, -16637, 24217 }, { -14942, -7735, 24837 }, { -10356, -19702, -20114 }, { -25559, -1730, 15612 }, { -5714, 2401, -11656 }, { 5609, -466, 18487 }, { 14674, -13334, 18070 }, { 14256, 12352, 23328 }, { 0, 0, 0 } },
    { { -3930, -18716, 23114 }, { 7120, -29092, 1718 }, { -15603, -23435, -10360 }, { 7105, -25287, -14494 }, { 5385, -14270, 25833 }, { -8355, -8688, 27472 }, { -11877, -17048, -21640 }, { -24409, -1523, 17375 }, { -5635, 2212, -11412 }, { 5260, -373, 18794 }, { 13963, -13217, 18675 }, { 16781, 13071, 21156 }, { 0, 0, 0 } },
    { { -5179, -17851, 23548 }, { 8924, -28592, 1684 }, { -15538, -23178, -11016 }, { 6453, -25374, -14646 }, { 4709, -12343, 26935 }, { -2229, -9977, 28204 }, { -13205, -14032, -22994 }, { -23692, -910, 18381 }, { -5485, 2008, -11207 }, { 4770, -264, 19040 }, { 13108, -13061, 19308 }, { 18247, 14477, 18906 }, { 0, 0, 0 } },
    { { -5533, -17223, 23932 }, { 10538, -28013, 2052 }, { -15179, -22852, -12139 }, { 5770, -25396, -14891 }, { 7044, -11516, 26791 }, { 840, -10427, 28117 }, { -13065, -10920, -24699 }, { -24556, 2776, 17009 }, { -5188, 1774, -11063 }, { 4096, -97, 19133 }, { 11972, -12797, 20007 }, { 20473, 14503, 16447 }, { 0, 0, 0 } },
    { { -4239, -16997, 24354 }, { 11558, -27613, 1984 }, { -14436, -22731, -13225 }, { 4872, -25208, -15519 }, { 9079, -10806, 26472 }, { 4680, -11007, 27512 }, { -12391, -7060, -26393 }, { -24776, 8537, 14603 }, { -4942, 1644, -10769 }, { 3271, 80, 19225 }, { 10985, -12465, 20498 }, { 22737, 14343, 13315 }, { 0, 0, 0 } },
    { { -879, -16339, 25145 }, { 11075, -27832, 1646 }, { -13610, -22643, -14215 }, { 3976, -24878, -16287 }, { 10422, -10316, 26172 }, { 9013, -11580, 26166 }, { -10834, -1764, -27920 }, { -23075, 14034, 13060 }, { -4684, 1659, -10490 }, { 2401, 110, 19282 }, { 10061, -12248, 20906 }, { 23659, 15204, 10446 }, { 0, 0, 0 } },
    { { 1291, -16828, 24802 }, { 9964, -28125, 3117 }, { -13332, -22328, -14957 }, { 3691, -24269, -17246 }, { 12786, -10006, 25227 }, { 12603, -11842, 24514 }, { -7563, 3955, -28761 }, { -20597, 17506, 13011 }, { -4673, 1845, -10282 }, { 1798, 27, 19298 }, { 9817, -12126, 20671 }, { 23703, 16962, 7104 }, { 0, 0, 0 } },
    { { 1836, -18766, 23334 }, { 8491, -28093, 6221 }, { -13047, -22123, -15503 }, { 3140, -23535, -18337 }, { 16384, -9865, 23114 }, { 15182, -12342, 22741 }, { -4418, 7279, -28766 }, { -17664, 20562, 12853 }, { -4974, 2245, -10222 }, { 1778, -173, 19199 }, { 10469, -12050, 19555 }, { 23178, 18612, 4049 }, { 0, 0, 0 } },
    { { 2153, -18814, 23268 }, { 7162, -28632, 5378 }, { -12856, -22200, -15553 }, { 2431, -22876, -19255 }, { 21607, -9629, 18451 }, { 12202, -14279, 23393 }, { -3048, 7412, -28910 }, { -16107, 23125, 10286 }, { -5468, 2785, -10485 }, { 2765, -557, 19026 }, { 11283, -11860, 18610 }, { 23371, 18048, 5300 }, { 0, 0, 0 } },
    { { 3026, -17925, 23865 }, { 5694, -29319, 2824 }, { -12132, -22719, -15384 }, { 1433, -22155, -20176 }, { 25224, -10560, 12339 }, { 8339, -15951, 24000 }, { -4693, 6100, -28996 }, { -14668, 24916, 8005 }, { -6250, 3502, -11062 }, { 4690, -1058, 18670 }, { 13547, -11752, 16793 }, { 24590, 15739, 6900 }, { 0, 0, 0 } },
    { { 3956, -17341, 24159 }, { 4281, -29689, 457 }, { -11198, -23103, -15520 }, { 111, -21616, -20802 }, { 26430, -12942, 5827 }, { 5269, -16888, 24228 }, { -13454, 2303, -26715 }, { -9319, 28061, 5072 }, { -7469, 4380, -11961 }, { 7356, -1650, 18182 }, { 16429, -11804, 14740 }, { 25962, 12885, 7743 }, { 0, 0, 0 } },
    { { 4959, -16482, 24571 }, { 2810, -29796, -2073 }, { -9986, -23067, -16377 }, { -1146, -20621, -21760 }, { 25088, -16449, -145 }, { 4509, -17038, 24277 }, { -23531, 75, -18608 }, { -3517, 29762, -1368 }, { -9665, 5184, -13262 }, { 10646, -2223, 17617 }, { 19728, -11475, 12278 }, { 27078, 10628, 7335 }, { 0, 0, 0 } },
    { { 5910, -15404, 25056 }, { 1452, -29656, -4292 }, { -10234, -23031, -16273 }, { -826, -18386, -23691 }, { 22899, -18659, -5242 }, { 3335, -14616, 25985 }, { -29452, -2852, -4944 }, { -5973, 29004, -4809 }, { -12223, 5587, -14630 }, { 14122, -2695, 16934 }, { 21992, -11254, 10602 }, { 27634, 9277, 7091 }, { 0, 0, 0 } },
    { { 6513, -14371, 25516 }, { 560, -29429, -5797 }, { -10455, -23339, -15684 }, { -600, -15940, -25408 }, { 19173, -20522, -10547 }, { 7681, -12167, 26324 }, { -28988, -3601, 6834 }, { -12671, 27151, -1512 }, { -14387, 5636, -15387 }, { 17410, -3111, 16383 }, { 23060, -11524, 9829 }, { 28277, 9009, 4384 }, { 0, 0, 0 } },
    { { 7074, -13487, 25847 }, { -114, -29186, -6939 }, { -10081, -23677, -15421 }, { -1010, -13691, -26675 }, { 14085, -21810, -15031 }, { 14546, -11082, 23782 }, { -22916, -7399, 17892 }, { -16237, 21046, 13908 }, { -16269, 5599, -15859 }, { 20604, -3394, 15787 }, { 23892, -11801, 9031 }, { 28791, 8431, 30 }, { 0, 0, 0 } },
    { { 7780, -12991, 25898 }, { -671, -28963, -7791 }, { -9225, -23867, -15661 }, { -1784, -12338, -27287 }, { 8530, -21830, -18727 }, { 19625, -13576, 18180 }, { -7936, -12045, 26305 }, { 1317, 3066, 29814 }, { -17777, 5779, -16317 }, { 23793, -3646, 15149 }, { 25057, -11943, 7849 }, { 28672, 7192, -5116 }, { 0, 0, 0 } },
    { { 8595, -12832, 25719 }, { -1165, -28776, -8400 }, { -8341, -23902, -16098 }, { -2494, -11781, -27477 }, { 2022, -20839, -21486 }, { 21271, -18523, 10220 }, { 11330, -18816, 20435 }, { 17450, -11159, 21702 }, { -18976, 6112, -16864 }, { 26678, -3847, 14470 }, { 26757, -11828, 6126 }, { 27328, 5307, -11181 }, { 0, 0, 0 } },
    { { 9271, -12870, 25464 }, { -1434, -28642, -8809 }, { -7713, -23964, -16317 }, { -2964, -11511, -27545 }, { -4444, -19239, -22586 }, { 18194, -23754, 2178 }, { 17377, -23584, 6470 }, { 23345, -16588, 8937 }, { -19923, 6527, -17417 }, { 28787, -4039, 13772 }, { 28599, -11428, 4061 }, { 24750, 3649, -16556 }, { 0, 0, 0 } },
    { { 9634, -13050, 25236 }, { -1368, -28565, -9065 }, { -7350, -23720, -16833 }, { -3209, -11845, -27375 }, { -9618, -16926, -22826 }, { 12686, -26967, -3444 }, { 14938, -26008, -641 }, { 25866, -15169, 931 }, { -20597, 7001, -17928 }, { 30028, -4261, 13082 }, { 29978, -10719, 2236 }, { 22282, 2817, -19890 }, { 0, 0, 0 } },
    { { 9857, -13319, 25009 }, { -1139, -28535, -9191 }, { -7227, -23847, -16706 }, { -3176, -11988, -27317 }, { -13187, -14033, -23004 }, { 8401, -27971, -6857 }, { 13118, -26898, -2102 }, { 25871, -13463, -7031 }, { -21111, 7531, -18441 }, { 30845, -4515, 12391 }, { 30635, -10315, 977 }, { 20672, 2792, -21561 }, { 0, 0, 0 } },
    { { 10204, -13595, 24720 }, { -1033, -28528, -9223 }, { -7165, -24055, -16432 }, { -3077, -12081, -27287 }, { -15010, -11809, -23135 }, { 6759, -27755, -9164 }, { 13640, -26597, -2562 }, { 21886, -13444, -15500 }, { -21629, 8055, -18997 }, { 31751, -4738, 11692 }, { 31051, -9949, -63 }, { 19812, 3152, -22306 }, { 0, 0, 0 } },
    { { 10653, -13856, 24383 }, { -1052, -28544, -9173 }, { -7111, -24197, -16246 }, { -2955, -12245, -27227 }, { -15999, -10312, -23188 }, { 5449, -27473, -10749 }, { 13658, -26619, -2214 }, { 15620, -13440, -21804 }, { -22146, 8547, -19584 }, { 32590, -4920, 11006 }, { 31275, -9597, -903 }, { 19850, 2710, -22330 }, { 0, 0, 0 } },
    { { 11042, -14072, 24084 }, { -1094, -28576, -9066 }, { -7082, -24193, -16265 }, { -2806, -12561, -27099 }, { -16125, -9649, -23386 }, { 4314, -26955, -12443 }, { 13647, -26712, -504 }, { 10191, -13317, -24876 }, { -22542, 8970, -20119 }, { -32487, -5064, 10399 }, { 31257, -9319, -1464 }, { 19374, 2000, -22817 }, { 0, 0, 0 } },
    { { 11058, -14129, 24044 }, { -1100, -28633, -8885 }, { -7458, -24234, -16033 }, { -2487, -12804, -27016 }, { -14697, -10666, -23879 }, { 3958, -26664, -13167 }, { 12930, -26862, 3355 }, { 7572, -14510, -25142 }, { -22298, 9184, -20247 }, { 32496, -5195, 10112 }, { 30805, -9297, -1427 }, { 19685, 1531, -22587 }, { 0, 0, 0 } },
    { { 11047, -14133, 24046 }, { -1169, -28700, -8657 }, { -7831, -24361, -15660 }, { -2228, -12982, -26954 }, { -11869, -12991, -24297 }, { 4323, -26782, -12807 }, { 10963, -27101, 6733 }, { 7944, -15801, -24233 }, { -21850, 9342, -20235 }, { 31643, -5333, 9918 }, { 30287, -9309, -1288 }, { 20767, 1208, -21616 }, { 0, 0, 0 } },
    { { 10949, -14100, 24111 }, { -1235, -28768, -8420 }, { -8190, -24551, -15171 }, { -2038, -13080, -26922 }, { -9752, -16255, -23252 }, { 3932, -27754, -10690 }, { 8079, -27162, 9848 }, { 10217, -17342, -22246 }, { -21194, 9458, -20073 }, { 30483, -5493, 9821 }, { 29901, -9273, -1190 }, { 21638, 739, -20767 }, { 0, 0, 0 } },
    { { 10416, -13991, 24408 }, { -1084, -28821, -8256 }, { -8877, -24891, -14199 }, { -1676, -13022, -26974 }, { -7647, -19560, -21423 }, { 3808, -29279, -5312 }, { 4149, -26670, 13095 }, { 13820, -19671, -17946 }, { -20014, 9465, -19533 }, { 28582, -5706, 9999 }, { 29970, -9095, -1200 }, { 23470, -1803, -18598 }, { 0, 0, 0 } },
    { { 9652, -13840, 24805 }, { -768, -28854, -8176 }, { -9468, -25160, -13317 }, { -1413, -12980, -27010 }, { -5171, -21896, -19845 }, { 3987, -29527, 3503 }, { -585, -25687, 15487 }, { 17192, -21659, -11633 }, { -18585, 9424, -18784 }, { 26373, -5970, 10371 }, { 30347, -8895, -1301 }, { 25292, -5736, -15081 }, { 0, 0, 0 } },
    { { 8149, -13492, 25526 }, { 97, -28823, -8319 }, { -10182, -25495, -12097 }, { -1174, -12649, -27178 }, { 607, -24632, -17114 }, { 3380, -26022, 14541 }, { -7143, -23550, 17157 }, { 19067, -23055, -2216 }, { -16284, 9095, -17433 }, { 23261, -6218, 11269 }, { 30439, -9406, -1061 }, { 28055, -8351, -6569 }, { 0, 0, 0 } },
    { { 6396, -13040, 26250 }, { 1637, -28671, -8676 }, { -10969, -25710, -10895 }, { -601, -12593, -27222 }, { 5732, -26941, -11887 }, { -930, -18046, 23947 }, { -14559, -20415, 16469 }, { 17457, -23238, 7432 }, { -13694, 8363, -16276 }, { 19950, -6250, 12091 }, { 30192, -10262, -924 }, { 27129, -12139, 4082 }, { 0, 0, 0 } },
    { { 4458, -12936, 26698 }, { 3589, -28487, -8694 }, { -12457, -25670, -9266 }, { 929, -13010, -27016 }, { 10359, -27235, -7136 }, { -6468, -10669, 27283 }, { -21351, -16635, 12938 }, { 13175, -21666, 16031 }, { -11252, 7432, -15323 }, { 16641, -6121, 12986 }, { 29506, -11592, -554 }, { 24114, -10644, 14325 }, { 0, 0, 0 } },
    { { 3313, -13581, 26544 }, { 5088, -28595, -7510 }, { -13962, -25238, -8253 }, { 2979, -14382, -26159 }, { 13571, -26698, -1747 }, { -11139, -6038, 27193 }, { -25488, -13679, 7952 }, { 8475, -18285, 22222 }, { -9547, 6408, -14623 }, { 14008, -5627, 13899 }, { 28143, -12666, 796 }, { 23169, -5637, 18204 }, { 0, 0, 0 } },
    { { 2938, -14279, 26220 }, { 5938, -28951, -5155 }, { -15037, -24640, -8172 }, { 4679, -16206, -24808 }, { 14944, -25879, 2636 }, { -14764, -4759, 25678 }, { -27146, -12572, 2244 }, { 4574, -14599, 25806 }, { -8441, 5742, -13978 }, { 12130, -5159, 14672 }, { 26871, -13356, 2344 }, { 22476, -2368, 19729 }, { 0, 0, 0 } },
    { { 3175, -14954, 25813 }, { 6090, -29291, -2222 }, { -14976, -23786, -10486 }, { 5232, -18668, -22894 }, { 15627, -24848, 6194 }, { -16965, -4511, 24328 }, { -27638, -11398, -2492 }, { 344, -12245, 27385 }, { -7820, 5284, -13444 }, { 10850, -4690, 15259 }, { 26042, -13706, 3593 }, { 21168, -56, 21258 }, { 0, 0, 0 } },
    { { 2809, -16084, 25168 }, { 6848, -29185, 1151 }, { -13739, -23311, -12955 }, { 4566, -20941, -20991 }, { 15669, -23718, 9588 }, { -18848, -3975, 22999 }, { -27322, -10039, -7259 }, { -4337, -10474, 27776 }, { -7331, 4838, -12977 }, { 9699, -4175, 15734 }, { 25070, -14083, 4935 }, { 19451, 2030, 22750 }, { 0, 0, 0 } },
    { { 2686, -17353, 24324 }, { 7155, -28735, 4806 }, { -11527, -23461, -14721 }, { 3111, -22598, -19485 }, { 15255, -22746, 12243 }, { -20753, -3298, 21411 }, { -26000, -9138, -11852 }, { -8987, -9221, 27096 }, { -6943, 4415, -12589 }, { 8741, -3667, 16133 }, { 24015, -14464, 6377 }, { 17390, 3954, 24124 }, { 0, 0, 0 } },
    { { 2511, -18346, 23604 }, { 7550, -27638, 8894 }, { -9762, -23961, -15185 }, { 2141, -23551, -18460 }, { 14637, -22248, 13812 }, { -23366, -2865, 18596 }, { -23899, -9272, -15584 }, { -12871, -8475, 25739 }, { -6636, 4044, -12309 }, { 8107, -3236, 16500 }, { 23655, -14770, 7248 }, { 14355, 4544, 25948 }, { 0, 0, 0 } },
    { { 2058, -18598, 23450 }, { 6878, -27960, 8423 }, { -10812, -24161, -14119 }, { 3581, -24001, -17639 }, { 13364, -22708, 14345 }, { -25839, -2490, 15037 }, { -21197, -10625, -18379 }, { -16242, -7788, 23990 }, { -6322, 3685, -12165 }, { 7642, -2885, 16849 }, { 23575, -15107, 7891 }, { 10881, 3807, 27697 }, { 0, 0, 0 } },
    { { 1092, -18090, 23908 }, { 5232, -29326, 3555 }, { -12220, -23893, -13409 }, { 5150, -24482, -16557 }, { 12800, -23583, 13415 }, { -27642, -2277, 11433 }, { -17594, -12974, -20546 }, { -19829, -8087, 21010 }, { -6156, 3465, -12180 }, { 7258, -2553, 17077 }, { 21696, -15210, 10099 }, { 7249, 3708, 28874 }, { 0, 0, 0 } },
    { { 131, -18188, 23858 }, { 4464, -29650, 980 }, { -13092, -23870, -12603 }, { 6195, -24709, -15846 }, { 12759, -24242, 12228 }, { -28715, -2323, 8370 }, { -13555, -15611, -21738 }, { -22746, -9003, 17365 }, { -6059, 3301, -12230 }, { 6898, -2218, 17262 }, { 19652, -15112, 12420 }, { 4135, 4179, 29418 }, { 0, 0, 0 } },
    { { -666, -18824, 23350 }, { 4420, -29672, -215 }, { -13716, -23952, -11755 }, { 7001, -24808, -15348 }, { 12646, -24538, 11746 }, { -29069, -2809, 6863 }, { -9454, -18331, -21785 }, { -24693, -10339, 13541 }, { -5995, 3175, -12303 }, { 6514, -1883, 17421 }, { 18173, -14885, 14146 }, { 2483, 3779, 29657 }, { 0, 0, 0 } },
    { { -931, -19192, 23039 }, { 4440, -29669, -144 }, { -14208, -24086, -10863 }, { 7684, -24875, -14906 }, { 11105, -24937, 12444 }, { -29136, -2671, 6629 }, { -5659, -20929, -20736 }, { -25681, -11850, 10003 }, { -5946, 3049, -12376 }, { 6107, -1547, 17572 }, { 17263, -14610, 15299 }, { 2684, 2616, 29765 }, { 0, 0, 0 } },
    { { -991, -19568, 22718 }, { 4794, -29574, 1544 }, { -14664, -24194, -9981 }, { 8339, -24992, -14347 }, { 8749, -24256, 15333 }, { -28466, -2950, 9000 }, { -2404, -23224, -18838 }, { -25961, -13302, 7008 }, { -5884, 2902, -12436 }, { 5701, -1206, 17726 }, { 16691, -14354, 16131 }, { 5549, 1325, 29453 }, { 0, 0, 0 } },
};
// clang-format on

// clang-format off
static const Vec3s sGdfClipSpin[40][13] = {
    { { 1553, -22943, 19266 }, { 1485, -8365, -28772 }, { -2442, -29892, 710 }, { -475, -21377, -21043 }, { 9985, -19833, -20173 }, { 9056, -27990, 5878 }, { -12330, -26709, -5882 }, { 5100, -12483, 26799 }, { -13771, 6559, -14411 }, { 17867, -6789, 17345 }, { 16663, -14356, 16157 }, { 29403, 5087, 3097 }, { 0, 0, 0 } },
    { { -9845, -20357, 19715 }, { 11104, -14425, -23846 }, { -6731, -29001, -3689 }, { 3086, -22309, -19819 }, { 19875, -14374, -17274 }, { -2663, -21677, 20568 }, { -11215, -27692, -2712 }, { 2965, -12202, 27245 }, { -11152, 5371, -13045 }, { 14922, -4717, 18919 }, { 21358, -14460, 12707 }, { 24335, 5840, 16545 }, { 0, 0, 0 } },
    { { -16628, -18220, 17075 }, { 17642, -19314, -14688 }, { -9171, -27133, -8929 }, { 3031, -23982, -17767 }, { 27099, -12078, -4445 }, { -17073, -16821, 18044 }, { -11434, -27603, 2705 }, { 1289, -10407, 28107 }, { -8417, 3363, -11657 }, { 12791, -2365, 19679 }, { 23021, -13500, 12126 }, { 12750, 6206, 26437 }, { 0, 0, 0 } },
    { { -19352, -19572, 11934 }, { 21720, -20549, -2451 }, { -9578, -25375, -12821 }, { -195, -24924, -16697 }, { 22613, -14933, 12872 }, { -28095, -10151, 2763 }, { -14360, -25399, 6977 }, { -2432, -10004, 28178 }, { -5228, 985, -11051 }, { 10597, -219, 19757 }, { 17779, -12579, 17156 }, { -3792, 6234, 29099 }, { 0, 0, 0 } },
    { { -16444, -23462, 8896 }, { 21064, -19587, 8523 }, { -13009, -23461, -13430 }, { 2474, -25879, -14972 }, { 8261, -14072, 25174 }, { -27109, -9833, -8272 }, { -18643, -21662, 9123 }, { -9886, -10790, 26189 }, { -1867, -471, -11381 }, { 7381, 1138, 19133 }, { 12994, -10883, 20871 }, { -15054, 7753, 24764 }, { 0, 0, 0 } },
    { { -10303, -26804, 8683 }, { 16111, -19387, 16267 }, { -14956, -20881, -15502 }, { 2788, -28279, -9618 }, { -2189, -10560, 27995 }, { -22962, -12899, -14366 }, { -23570, -18057, 4288 }, { -16750, -10387, 22617 }, { 1626, -1232, -11597 }, { 3347, 1640, 18601 }, { 9796, -8933, 23120 }, { -22318, 8837, 17995 }, { 0, 0, 0 } },
    { { -4134, -27780, 10543 }, { 9245, -20906, 19428 }, { -14766, -18319, -18611 }, { 4568, -29650, -190 }, { -11895, -8426, 26220 }, { -17294, -11280, -21764 }, { -26162, -14140, -3954 }, { -21624, -9278, 18609 }, { 5058, -2599, -11076 }, { -558, 1655, 18464 }, { 6998, -6692, 24803 }, { -27950, 8702, 6565 }, { 0, 0, 0 } },
    { { -3986, -28318, 9067 }, { 6644, -22487, 18714 }, { -10819, -19270, -20288 }, { 6703, -26476, 12413 }, { -17756, -9434, 22265 }, { -7718, -8360, -27759 }, { -24107, -10981, -14081 }, { -25637, -9130, 12625 }, { 7548, -3299, -11978 }, { -3749, 1583, 18625 }, { 3613, -3063, 26611 }, { -28300, 8948, -4362 }, { 0, 0, 0 } },
    { { -7585, -28873, 2967 }, { 8753, -22382, 17956 }, { -2638, -23209, -18825 }, { 5146, -19635, 22090 }, { -21961, -13139, 15654 }, { 3573, -4698, -29414 }, { -18829, -8083, -21912 }, { -28187, -9758, 3205 }, { 9757, -3802, -12799 }, { -6922, 1738, 18466 }, { -160, 1910, 26772 }, { -24985, 10234, -13078 }, { 0, 0, 0 } },
    { { -9256, -28471, -1936 }, { 9873, -22900, 16677 }, { 5815, -26846, -12062 }, { 2000, -14583, 26141 }, { -24394, -16672, 5195 }, { 15031, 39, -25963 }, { -6378, -5583, -28778 }, { -27353, -9605, -7718 }, { 12323, -3766, -13269 }, { -10327, 1696, 18269 }, { -5410, 6599, 24165 }, { -18577, 11375, -20628 }, { 0, 0, 0 } },
    { { -8015, -27616, -8552 }, { 7023, -23696, 17005 }, { 11814, -27252, -4214 }, { -1424, -14446, 26254 }, { -20163, -19730, -10207 }, { 24140, 8294, -15763 }, { 13253, -3296, -26711 }, { -18653, -7279, -22340 }, { 15092, -3958, -13634 }, { -13568, 1619, 18187 }, { -13674, 8627, 17674 }, { -9021, 11844, -26045 }, { 0, 0, 0 } },
    { { -5382, -26211, -13566 }, { 2293, -25086, 16293 }, { 16276, -25042, 2830 }, { -4024, -18014, 23650 }, { -9658, -20093, -20075 }, { 28104, 7107, -7722 }, { 23132, -2258, -18969 }, { -7592, -4175, -28722 }, { 18747, -4380, -14632 }, { -16940, 1343, 18016 }, { -22405, 6681, 11210 }, { -111, 13726, -26675 }, { 0, 0, 0 } },
    { { -4211, -25082, -15911 }, { -1589, -26369, 14218 }, { 18530, -22158, 8101 }, { -3220, -22615, 19447 }, { -1343, -18021, -23947 }, { 29438, 1509, -5578 }, { 27020, -2806, -12730 }, { 3931, -2695, -29619 }, { 23150, -4384, -16309 }, { -20700, 746, 17888 }, { -27959, 2047, 8810 }, { 6355, 16932, -23936 }, { 0, 0, 0 } },
    { { -320, -25273, -16160 }, { -9972, -26221, 10630 }, { 17633, -20741, 12605 }, { 1204, -25745, 15355 }, { -138, -19507, -22791 }, { 28915, -1719, -7810 }, { 28797, -2433, -8052 }, { 16668, -3382, -24713 }, { 27824, -3156, -17482 }, { -25068, 519, 18234 }, { -32459, -3487, 8918 }, { 7117, 18045, -22885 }, { 0, 0, 0 } },
    { { 1675, -26669, -13638 }, { -19128, -23053, 1635 }, { 16723, -19851, 15043 }, { 4298, -27080, 12175 }, { -500, -24038, -17943 }, { 23636, -10281, -15351 }, { 29773, -1155, -3494 }, { 24288, -844, -17589 }, { 32690, -757, -17863 }, { -28842, 224, 18422 }, { 27543, -8705, 10750 }, { 6903, 15648, -24647 }, { 0, 0, 0 } },
    { { -2485, -27741, -11146 }, { -21182, -18595, -10274 }, { 14770, -19778, 17048 }, { 7259, -27166, 10455 }, { 2622, -24295, -17403 }, { 9230, -22224, -17914 }, { 27725, 1887, 11303 }, { 28510, 3257, -8750 }, { -28391, 1409, -17334 }, { 32652, -186, 18402 }, { 19955, -10943, 15517 }, { 6474, 13943, -25762 }, { 0, 0, 0 } },
    { { -5029, -26733, -12650 }, { -19791, -13948, -17714 }, { 13196, -19193, 18907 }, { 8376, -27645, 8100 }, { 2734, -24571, -16994 }, { 3229, -23727, -18072 }, { 18126, -234, 23904 }, { 28237, 4615, 9022 }, { -24974, 2488, -16761 }, { 28560, -733, 18201 }, { 15811, -11541, 17977 }, { 9789, 13064, -25170 }, { 0, 0, 0 } },
    { { -4502, -26570, -13182 }, { -17761, -6857, -23185 }, { 10688, -18977, 20631 }, { 9516, -28322, 2700 }, { 4544, -24261, -17052 }, { 4606, -24216, -17099 }, { 9245, -2408, 28438 }, { 14355, -1005, 26323 }, { -22708, 3057, -16562 }, { 25533, -995, 18042 }, { 13133, -12548, 20021 }, { 15297, 10901, -23392 }, { 0, 0, 0 } },
    { { -3158, -26716, -13277 }, { -14867, -810, -26044 }, { 8385, -19817, 20904 }, { 10802, -27814, -3115 }, { 4482, -19807, -22082 }, { 11264, -26313, -8985 }, { -10320, -6880, 27316 }, { -4090, -6350, 29034 }, { -21103, 3376, -17001 }, { 22676, -1707, 17541 }, { 8667, -12567, 24643 }, { 20482, 8930, -20019 }, { 0, 0, 0 } },
    { { -485, -27539, -11890 }, { -11678, 2669, -27505 }, { 9355, -20578, 19724 }, { 7668, -26886, -10878 }, { 8836, -16743, -23272 }, { 13071, -26537, -4996 }, { -25636, -8675, 12944 }, { -22679, -12367, 15255 }, { -20499, 3572, -17638 }, { 20696, -2568, 16862 }, { 7349, -12520, 25916 }, { 23264, 8338, -17008 }, { 0, 0, 0 } },
    { { 1726, -29041, -7324 }, { -8686, 2041, -28642 }, { 9541, -22878, 16898 }, { 4409, -24875, -16180 }, { 12186, -16736, -21712 }, { 14114, -25831, -5793 }, { -27532, -4830, -10892 }, { -27641, -11551, -1603 }, { -20179, 4077, -17515 }, { 18677, -3237, 16852 }, { 6913, -12796, 25781 }, { 23118, 9363, -16670 }, { 0, 0, 0 } },
    { { 2954, -29843, -818 }, { -6030, -1129, -29366 }, { 7687, -25144, 14447 }, { 3272, -22881, -19125 }, { 15155, -15363, -20840 }, { 13738, -26192, -5022 }, { -19935, -1604, -22361 }, { -24706, -9245, -14287 }, { -18735, 4741, -16680 }, { 16547, -3843, 17145 }, { 5784, -13033, 26081 }, { 22633, 10140, -16880 }, { 0, 0, 0 } },
    { { 3220, -28945, 7199 }, { -3201, -7879, -28769 }, { 2744, -27123, 12523 }, { 5788, -21116, -20509 }, { 20358, -13968, -17043 }, { 13998, -26326, -3315 }, { -11145, -487, -27849 }, { -19955, -8184, -20853 }, { -15945, 5134, -14930 }, { 14288, -3977, 17914 }, { 5465, -13036, 26223 }, { 24129, 9096, -15331 }, { 0, 0, 0 } },
    { { 2575, -25481, 15624 }, { -573, -16789, -24856 }, { -3224, -28082, 10051 }, { 9168, -20158, -20239 }, { 24017, -12499, -12921 }, { 14025, -26477, -1514 }, { -4377, -396, -29676 }, { -14360, -7592, -25222 }, { -13087, 4999, -13107 }, { 12165, -3803, 18761 }, { 5556, -13275, 26122 }, { 26116, 6327, -13340 }, { 0, 0, 0 } },
    { { -370, -21159, 21264 }, { 2734, -24612, -16934 }, { -8271, -28025, 6795 }, { 11192, -20015, -19343 }, { 26544, -11198, -8368 }, { 14406, -26116, 3228 }, { 887, -837, -29975 }, { -11048, -11038, -25615 }, { -10500, 4527, -11489 }, { 10334, -3215, 19562 }, { 5736, -13414, 26021 }, { 28110, 3539, -9865 }, { 0, 0, 0 } },
    { { -5403, -18851, 22703 }, { 5626, -28217, -8493 }, { -12269, -27243, 2694 }, { 12801, -20282, -18022 }, { 27667, -10813, -4200 }, { 13983, -24742, 9608 }, { 2767, -2653, -29754 }, { -12732, -15801, -22096 }, { -8314, 3679, -10378 }, { 8737, -2414, 20102 }, { 5689, -13395, 26274 }, { 28893, 4311, -6828 }, { 0, 0, 0 } },
    { { -9296, -17981, 22142 }, { 7485, -29047, 514 }, { -15268, -25746, -2004 }, { 14736, -21263, -15190 }, { 27996, -10777, 325 }, { 13620, -22145, 14970 }, { 2311, -6221, -29257 }, { -16753, -18306, -16859 }, { -6349, 2548, -9986 }, { 6971, -1536, 20416 }, { 5209, -13062, 27137 }, { 28788, 7878, -3034 }, { 0, 0, 0 } },
    { { -9776, -17805, 22077 }, { 6599, -28642, 6006 }, { -17408, -23655, -6115 }, { 16362, -22443, -11339 }, { 27149, -11419, 5703 }, { 11947, -18708, 20181 }, { -29, -11078, -27880 }, { -21115, -17416, -12282 }, { -4582, 1442, -9900 }, { 5300, -724, 20395 }, { 4831, -12595, 27680 }, { 28018, 10597, 1637 }, { 0, 0, 0 } },
    { { -8833, -17339, 22833 }, { 5347, -28596, 7326 }, { -18456, -21546, -9753 }, { 16491, -23556, -8554 }, { 25606, -11826, 10221 }, { 5146, -13459, 26313 }, { -5116, -14966, -25492 }, { -23517, -15750, -9944 }, { -2962, 539, -9927 }, { 3878, -229, 20221 }, { 5025, -12076, 27463 }, { 26032, 13073, 7170 }, { 0, 0, 0 } },
    { { -8331, -16663, 23514 }, { 4031, -29093, 6109 }, { -18574, -20362, -11848 }, { 15773, -24356, -7616 }, { 23494, -11976, 14303 }, { -4324, -7142, 28815 }, { -10523, -17799, -21736 }, { -23975, -15637, -8982 }, { -2002, 168, -10041 }, { 2782, -420, 20098 }, { 4900, -11724, 27482 }, { 23159, 14370, 12537 }, { 0, 0, 0 } },
    { { -8628, -16900, 23237 }, { 3658, -29307, 5263 }, { -17997, -20469, -12535 }, { 14822, -24813, -8037 }, { 23605, -12365, 13781 }, { -14218, -2601, 26289 }, { -12622, -19918, -18546 }, { -25613, -14995, -4372 }, { -2016, 482, -10252 }, { 2270, -814, 20075 }, { 4952, -11502, 27320 }, { 20370, 13284, 17567 }, { 0, 0, 0 } },
    { { -8881, -17345, 22810 }, { 3410, -29444, 4630 }, { -16820, -20964, -13326 }, { 13793, -25273, -8429 }, { 22133, -14441, 14198 }, { -23560, -786, 18555 }, { -13071, -20447, -17638 }, { -27418, -12024, 1909 }, { -2368, 1056, -10583 }, { 2329, -1208, 19890 }, { 5554, -11326, 26467 }, { 14508, 9522, 24471 }, { 0, 0, 0 } },
    { { -8722, -18280, 22130 }, { 3081, -29492, 4555 }, { -15439, -22000, -13327 }, { 12887, -25424, -9357 }, { 19120, -16693, 15993 }, { -28403, 1099, 9594 }, { -13684, -19663, -18058 }, { -27942, -7235, 8179 }, { -2817, 1527, -10743 }, { 3006, -1539, 19522 }, { 6977, -11131, 24769 }, { 6209, 6741, 28566 }, { 0, 0, 0 } },
    { { -8637, -19227, 21348 }, { 3097, -29461, 4738 }, { -14199, -22786, -13386 }, { 12111, -25554, -10015 }, { 15397, -18478, 17930 }, { -29694, 2526, 3448 }, { -12071, -19296, -19544 }, { -27106, -3752, 12295 }, { -3120, 1721, -10755 }, { 3708, -1726, 19222 }, { 8093, -10860, 23733 }, { 70, 5786, 29437 }, { 0, 0, 0 } },
    { { -8493, -19599, 21065 }, { 3344, -29471, 4504 }, { -13367, -23269, -13412 }, { 11744, -25519, -10530 }, { 12214, -19596, 19152 }, { -29854, 2930, 409 }, { -9046, -19619, -20815 }, { -26429, -3859, 13661 }, { -3177, 1764, -10921 }, { 3917, -1669, 18974 }, { 7911, -10570, 24108 }, { -3377, 4996, 29388 }, { 0, 0, 0 } },
    { { -8241, -19767, 21008 }, { 3551, -29503, 4118 }, { -12719, -23774, -13154 }, { 11553, -25307, -11228 }, { 10146, -21017, 18850 }, { -29887, 2593, 137 }, { -5547, -20120, -21551 }, { -26089, -4860, 13991 }, { -3165, 1777, -11127 }, { 3945, -1533, 18744 }, { 7374, -10291, 24816 }, { -4652, 4015, 29364 }, { 0, 0, 0 } },
    { { -7919, -19776, 21123 }, { 3669, -29540, 3731 }, { -12302, -24210, -12749 }, { 11574, -24987, -11903 }, { 8504, -22280, 18201 }, { -29946, 1722, 519 }, { -1674, -20395, -21937 }, { -26287, -6491, 12918 }, { -3163, 1777, -11312 }, { 3818, -1365, 18540 }, { 6691, -10115, 25523 }, { -4817, 3199, 29438 }, { 0, 0, 0 } },
    { { -7589, -19693, 21321 }, { 3751, -29573, 3369 }, { -12025, -24372, -12704 }, { 11706, -24824, -12112 }, { 7113, -23169, 17680 }, { -29985, 385, 859 }, { 1317, -21126, -21259 }, { -27117, -8791, 9349 }, { -3165, 1769, -11471 }, { 3490, -1174, 18371 }, { 6123, -10051, 26194 }, { -4336, 2515, 29578 }, { 0, 0, 0 } },
    { { -7298, -19612, 21497 }, { 3877, -29593, 3041 }, { -11740, -24329, -13048 }, { 11836, -24855, -11921 }, { 5730, -23795, 17349 }, { -29959, -1228, 977 }, { 3101, -22240, -19894 }, { -27496, -11099, 4560 }, { -3168, 1760, -11634 }, { 3107, -982, 18201 }, { 5777, -10097, 26781 }, { -3515, 1899, 29733 }, { 0, 0, 0 } },
    { { -7152, -19568, 21586 }, { 4191, -29574, 2793 }, { -11396, -24252, -13490 }, { 11949, -24934, -11640 }, { 4209, -24250, 17153 }, { -29845, -2913, 891 }, { 2801, -23235, -18770 }, { -26902, -13273, -332 }, { -3171, 1753, -11831 }, { 2835, -794, 17999 }, { 5356, -10180, 27374 }, { -2696, 1346, 29848 }, { 0, 0, 0 } },
};
// clang-format on

// clang-format off
static const Vec3s sGdfClipRun[21][13] = {
    { { 1658, -23256, 18879 }, { 1515, -7298, -29059 }, { -2385, -29903, 307 }, { -498, -21680, -20730 }, { 9985, -19833, -20173 }, { 9056, -27990, 5878 }, { -12330, -26709, -5882 }, { 5100, -12483, 26799 }, { -13716, 6522, -14235 }, { 17842, -6782, 17388 }, { 16663, -14356, 16157 }, { 29403, 5087, 3097 }, { 0, 0, 0 } },
    { { 637, -19647, 22663 }, { 1865, -12662, -27133 }, { -1812, -28479, -9256 }, { -1376, -22943, -19280 }, { 6125, -14575, -25496 }, { 10223, -28190, -896 }, { -13362, -26809, -1656 }, { 5762, -12492, 26660 }, { -13496, 6355, -13826 }, { 18915, -6515, 17089 }, { 16871, -14595, 16405 }, { 29067, 7003, -2462 }, { 0, 0, 0 } },
    { { 468, -15602, 25619 }, { 1637, -18296, -23718 }, { -816, -25085, -16434 }, { -2318, -24662, -16924 }, { 2513, -9253, -28427 }, { 9648, -27865, -5520 }, { -14730, -26028, 2360 }, { 6706, -13814, 25772 }, { -13584, 6506, -13703 }, { 19513, -6251, 16843 }, { 17131, -14878, 16243 }, { 27872, 8700, -6888 }, { 0, 0, 0 } },
    { { 567, -13699, 26684 }, { 1277, -24577, -17156 }, { -926, -23229, -18962 }, { -2257, -22781, -19389 }, { 634, -5670, -29452 }, { 8391, -27778, -7615 }, { -15808, -24989, 5066 }, { 7033, -15473, 24721 }, { -13952, 6829, -14078 }, { 19849, -6093, 16622 }, { 16695, -15066, 16666 }, { 26952, 9621, -9001 }, { 0, 0, 0 } },
    { { 312, -15826, 25484 }, { 1246, -29640, -4462 }, { -2625, -25248, -15990 }, { -427, -15972, -25391 }, { 1000, -5439, -29486 }, { 6760, -28430, -6787 }, { -16628, -24381, 5392 }, { 6651, -16580, 24101 }, { -14416, 6944, -14560 }, { 19955, -6002, 16444 }, { 15670, -15107, 17648 }, { 27207, 9070, -8804 }, { 0, 0, 0 } },
    { { 264, -22885, 19397 }, { 903, -28840, 8213 }, { -2510, -26581, -13681 }, { -210, -6456, -29296 }, { 3261, -8797, -28495 }, { 4740, -29482, -2892 }, { -17305, -24181, 3978 }, { 5720, -16699, 24258 }, { -14919, 6710, -14961 }, { 19675, -5922, 16293 }, { 15404, -15012, 17813 }, { 28347, 6996, -6892 }, { 0, 0, 0 } },
    { { 19, -25113, 16412 }, { 714, -29609, 4778 }, { -3630, -27796, -10688 }, { 897, 3193, -29816 }, { 6547, -14713, -25311 }, { 2098, -29787, 2889 }, { -17684, -24133, 2199 }, { 4384, -16134, 24909 }, { -15400, 6295, -15198 }, { 18892, -5891, 16170 }, { 16180, -14802, 16880 }, { 29472, 4411, -3460 }, { 0, 0, 0 } },
    { { 640, -24543, 17240 }, { -300, -29825, -3218 }, { -4610, -28969, -6288 }, { 2168, 11237, -27731 }, { 9521, -21335, -18820 }, { -504, -28625, 8965 }, { -17965, -24017, 682 }, { 2910, -15715, 25388 }, { -15882, 5978, -15299 }, { 17683, -5967, 16083 }, { 16707, -14472, 16192 }, { 29828, 3087, 887 }, { 0, 0, 0 } },
    { { 518, -25465, 15852 }, { -496, -28237, -10120 }, { -6759, -29205, 1176 }, { 3717, 13965, -26290 }, { 10364, -26468, -9593 }, { -2690, -25816, 15043 }, { -18321, -23746, -677 }, { 1366, -15873, 25420 }, { -16485, 5968, -15523 }, { 16268, -6110, 16007 }, { 16266, -14107, 16505 }, { 29606, 3018, 3792 }, { 0, 0, 0 } },
    { { 926, -27485, 11989 }, { -941, -26146, -14680 }, { -6933, -27310, 10302 }, { 3202, 9964, -28115 }, { 8526, -28760, -424 }, { -5320, -21201, 20548 }, { -18470, -23551, -2050 }, { -512, -16472, 25068 }, { -17202, 6286, -16089 }, { 14848, -6232, 15936 }, { 15335, -13845, 17336 }, { 29617, 1654, 4482 }, { 0, 0, 0 } },
    { { 1748, -29496, 5191 }, { -1510, -24900, -16664 }, { -6333, -22556, 18739 }, { 2108, -1346, -29896 }, { 6138, -28701, 6209 }, { -8432, -15220, 24439 }, { -18091, -23678, -3476 }, { -2839, -17290, 24352 }, { -17963, 6767, -17012 }, { 13492, -6302, 15913 }, { 14547, -13781, 18017 }, { 29495, -1225, 5342 }, { 0, 0, 0 } },
    { { 1566, -29843, -2640 }, { -1184, -24650, -17058 }, { -4869, -18824, 22846 }, { 292, -13780, -26646 }, { 5511, -27545, 10531 }, { -11269, -8548, 26456 }, { -17258, -24044, -4902 }, { -5254, -18141, 23308 }, { -18536, 7246, -17931 }, { 12165, -6309, 16011 }, { 13973, -13907, 18473 }, { 28841, -2689, 7807 }, { 0, 0, 0 } },
    { { 1011, -28652, -8833 }, { -687, -24717, -16989 }, { -3787, -17734, 23899 }, { -703, -22776, -19513 }, { 6561, -25915, 13614 }, { -13601, -2494, 26623 }, { -16484, -24214, -6476 }, { -7106, -18891, 22196 }, { -18659, 7664, -18370 }, { 10987, -6250, 16206 }, { 13331, -14096, 19066 }, { 27896, -1495, 10936 }, { 0, 0, 0 } },
    { { 858, -27820, -11195 }, { -1428, -20622, -21742 }, { -3293, -18303, 23540 }, { -1178, -27392, -12179 }, { 7962, -24459, 15438 }, { -15347, 1025, 25757 }, { -16172, -23836, -8385 }, { -7901, -19475, 21408 }, { -18272, 7917, -18118 }, { 10200, -6166, 16399 }, { 12807, -14229, 19621 }, { 26858, 661, 13350 }, { 0, 0, 0 } },
    { { 312, -26652, -13768 }, { -1892, -14413, -26243 }, { -3061, -19774, 22352 }, { -1195, -29665, -4306 }, { 9034, -23927, 15680 }, { -16451, 1022, 25067 }, { -16257, -23180, -9919 }, { -7565, -19842, 21191 }, { -17591, 7809, -17467 }, { 9917, -6052, 16528 }, { 13102, -14308, 19353 }, { 25950, 2020, 14918 }, { 0, 0, 0 } },
    { { 352, -26843, -13391 }, { -2350, -4460, -29573 }, { -2778, -22279, 19898 }, { -1367, -29800, 3174 }, { 10056, -24369, 14318 }, { -16785, -2301, 24758 }, { -16383, -23074, -9959 }, { -6394, -19900, 21520 }, { -16958, 7319, -16820 }, { 10124, -5855, 16615 }, { 14399, -14309, 18015 }, { 25274, 3029, 15876 }, { 0, 0, 0 } },
    { { 1157, -27668, -11538 }, { -2691, 6530, -29157 }, { -2117, -25698, 15335 }, { -1777, -28483, 9249 }, { 11437, -25348, 11256 }, { -15752, -8077, 24220 }, { -16029, -23893, -8497 }, { -4597, -19517, 22315 }, { -16579, 6688, -16400 }, { 10788, -5613, 16725 }, { 15620, -14172, 16711 }, { 24810, 4667, 16207 }, { 0, 0, 0 } },
    { { 2173, -28832, -8001 }, { -2331, 14970, -25894 }, { -2325, -24614, 16993 }, { -1043, -29978, -487 }, { 12777, -26361, 6469 }, { -12853, -14665, 22797 }, { -14944, -25108, -6801 }, { -2156, -18523, 23500 }, { -16401, 6234, -16239 }, { 11841, -5488, 16899 }, { 15730, -14016, 16566 }, { 24838, 6201, 15641 }, { 0, 0, 0 } },
    { { 3557, -29696, -2340 }, { -1552, 18026, -23930 }, { -1818, -24727, 16890 }, { -966, -28115, -10423 }, { 13363, -26856, 424 }, { -8395, -20478, 20253 }, { -13540, -26096, -5973 }, { 594, -16972, 24730 }, { -16203, 6113, -16152 }, { 13151, -5617, 17109 }, { 15164, -13986, 17223 }, { 25868, 6604, 13683 }, { 0, 0, 0 } },
    { { 4258, -29105, 5896 }, { -638, 14829, -26071 }, { -1581, -26791, 13406 }, { -790, -25003, -16560 }, { 13087, -26255, -6279 }, { -2961, -24782, 16645 }, { -12535, -26623, -5839 }, { 2950, -15288, 25643 }, { -15722, 6228, -15874 }, { 14635, -5940, 17263 }, { 15012, -14055, 17559 }, { 27453, 6053, 10474 }, { 0, 0, 0 } },
    { { 2971, -26679, 13394 }, { 1407, 5468, -29464 }, { -2199, -29068, 7088 }, { -52, -22738, -19570 }, { 12096, -24016, -13301 }, { 3103, -27331, 11975 }, { -12217, -26778, -5802 }, { 4477, -13782, 26268 }, { -14850, 6395, -15209 }, { 16247, -6330, 17334 }, { 15723, -14156, 17020 }, { 28735, 5348, 6759 }, { 0, 0, 0 } },
};
// clang-format on

// Its stab (the combo's third hit, and the thrust out of a dash), up to where the thrust is held
// clang-format off
static const Vec3s sGdfClipStab[32][13] = {
    { { -1824, -24192, 17647 }, { 10373, -27912, -3653 }, { -12797, -27118, -930 }, { 7715, -25031, -14626 }, { 12883, -20733, 17441 }, { -23395, 4836, 18146 }, { -7900, -14485, -25055 }, { -4032, -17570, 23980 }, { -7293, 1511, -14543 }, { 6896, 522, 15687 }, { 16663, -14356, 16157 }, { 12989, 17845, 20318 }, { 0, 0, 0 } },
    { { -1919, -24008, 17886 }, { 10480, -27883, -3568 }, { -13202, -26931, -645 }, { 8161, -24982, -14467 }, { 12210, -19909, 18829 }, { -23507, 5588, 17782 }, { -7660, -14619, -25052 }, { -5139, -17802, 23594 }, { -7012, 1482, -14547 }, { 6632, 411, 15620 }, { 16071, -14477, 16650 }, { 12114, 18807, 19989 }, { 0, 0, 0 } },
    { { -2314, -23709, 18235 }, { 10678, -27797, -3648 }, { -13712, -26679, -448 }, { 8683, -24881, -14337 }, { 11671, -18652, 20394 }, { -23518, 5873, 17675 }, { -7406, -14793, -25026 }, { -6340, -18054, 23105 }, { -6843, 1477, -14559 }, { 6324, 298, 15563 }, { 15335, -14607, 17293 }, { 10503, 19705, 20035 }, { 0, 0, 0 } },
    { { -2949, -23176, 18819 }, { 10862, -27682, -3963 }, { -14308, -26367, -247 }, { 9328, -24626, -14372 }, { 11381, -16623, 22229 }, { -22846, 4865, 18825 }, { -7222, -15008, -24952 }, { -7751, -18351, 22431 }, { -6915, 1547, -14619 }, { 5962, 198, 15518 }, { 14608, -14781, 17868 }, { 8525, 19710, 20948 }, { 0, 0, 0 } },
    { { -3497, -22272, 19792 }, { 10845, -27580, -4659 }, { -14898, -26039, 87 }, { 10109, -24132, -14678 }, { 11709, -13781, 23937 }, { -20434, 2447, 21828 }, { -6865, -15028, -25041 }, { -9130, -18742, 21573 }, { -7217, 1747, -14755 }, { 5540, 89, 15485 }, { 14150, -14974, 18043 }, { 6697, 16353, 24243 }, { 0, 0, 0 } },
    { { -3535, -21069, 21062 }, { 10500, -27519, -5699 }, { -15364, -25763, 469 }, { 10930, -23443, -15197 }, { 13904, -10508, 24418 }, { -16080, 124, 25327 }, { -6211, -14664, -25424 }, { -10516, -19185, 20527 }, { -7574, 2061, -14950 }, { 5180, -93, 15419 }, { 14160, -15163, 17577 }, { 5386, 7188, 28624 }, { 0, 0, 0 } },
    { { -2947, -20638, 21573 }, { 10073, -27557, -6257 }, { -15090, -25925, 430 }, { 11034, -23110, -15626 }, { 18886, -9632, 21226 }, { -14613, -1370, 26164 }, { -4777, -14423, -25868 }, { -10486, -18692, 20992 }, { -7883, 2352, -15056 }, { 5532, -214, 15316 }, { 15814, -15174, 15604 }, { 2481, 2607, 29783 }, { 0, 0, 0 } },
    { { -2358, -20756, 21532 }, { 9671, -27700, -6260 }, { -14127, -26465, 103 }, { 10685, -23028, -15986 }, { 24055, -10136, 14785 }, { -15308, -2882, 25639 }, { -2729, -14715, -26001 }, { -9881, -16715, 22869 }, { -8522, 2703, -15204 }, { 6598, -249, 15099 }, { 18777, -15024, 12456 }, { -1277, 5393, 29484 }, { 0, 0, 0 } },
    { { -1846, -21448, 20894 }, { 9280, -27991, -5513 }, { -12157, -27422, -483 }, { 9703, -23216, -16337 }, { 26283, -13016, 6309 }, { -16562, -4275, 24646 }, { -1716, -15651, -25536 }, { -7364, -13577, 25718 }, { -9815, 3146, -15506 }, { 8501, -176, 14841 }, { 21505, -14613, 9943 }, { -3463, 9226, 28335 }, { 0, 0, 0 } },
    { { -1302, -22507, 19792 }, { 8861, -28355, -4177 }, { -9406, -28450, -1454 }, { 8040, -23641, -16627 }, { 24354, -17367, -2289 }, { -16765, -5167, 24336 }, { -5033, -16827, -24321 }, { -1199, -10507, 28074 }, { -11617, 3587, -15924 }, { 11270, -141, 14668 }, { 23421, -14131, 8773 }, { -3076, 7438, 28900 }, { 0, 0, 0 } },
    { { -648, -23482, 18659 }, { 8346, -28668, -2918 }, { -6706, -29109, -2778 }, { 5973, -24058, -16898 }, { 20164, -20299, -9020 }, { -14499, -5833, 25608 }, { -15000, -17561, -19148 }, { 8524, -8084, 27604 }, { -13433, 3945, -16321 }, { 14778, -260, 14648 }, { 25639, -13779, 7569 }, { 679, 737, 29983 }, { 0, 0, 0 } },
    { { 93, -23858, 18187 }, { 7625, -28907, -2498 }, { -4768, -29363, -3884 }, { 3973, -24030, -17514 }, { 16502, -21158, -13416 }, { -9873, -6813, 27497 }, { -25266, -15413, -4905 }, { 16049, -7177, 24309 }, { -14821, 4227, -16683 }, { 18408, -541, 14813 }, { 28762, -13507, 5219 }, { 8064, -3224, 28715 }, { 0, 0, 0 } },
    { { 952, -23540, 18573 }, { 6656, -29093, -3053 }, { -4019, -29334, -4833 }, { 2036, -23545, -18480 }, { 12880, -20828, -17329 }, { -4007, -7637, 28734 }, { -25232, -11707, 11237 }, { 17173, -6808, 23638 }, { -15623, 4449, -16987 }, { 21427, -917, 15009 }, { 31687, -13118, 2417 }, { 16739, -2757, 24743 }, { 0, 0, 0 } },
    { { 1892, -22846, 19351 }, { 5609, -29187, -4082 }, { -4164, -29160, -5688 }, { 780, -22955, -19299 }, { 8635, -19123, -21442 }, { 1892, -8790, 28621 }, { -17876, -8837, 22413 }, { 13414, -7256, 25834 }, { -16049, 4631, -17223 }, { 23584, -1203, 15096 }, { -32700, -12878, 974 }, { 22563, -542, 19764 }, { 0, 0, 0 } },
    { { 2698, -22223, 19971 }, { 4761, -29180, -5084 }, { -4563, -28894, -6658 }, { 252, -22512, -19828 }, { 4485, -15796, -25107 }, { 6836, -11224, 26968 }, { -8360, -7001, 27948 }, { 5943, -7487, 28436 }, { -16532, 4771, -17388 }, { 25125, -1385, 15147 }, { 32209, -13169, 1509 }, { 25391, 1280, 15926 }, { 0, 0, 0 } },
    { { 3075, -21869, 20305 }, { 4359, -29091, -5891 }, { -4760, -28637, -7569 }, { 51, -22051, -20341 }, { 378, -11391, -27751 }, { 11159, -14929, 23507 }, { -1663, -6255, 29294 }, { 2178, -6600, 29184 }, { -17365, 4877, -17560 }, { 26636, -1679, 15186 }, { 31314, -13490, 2816 }, { 27445, 1555, 12015 }, { 0, 0, 0 } },
    { { 2895, -21644, 20571 }, { 4538, -28961, -6379 }, { -4711, -28636, -7601 }, { -94, -21225, -21201 }, { -2938, -7943, -28780 }, { 13246, -17694, 20285 }, { 1041, -5838, 29408 }, { 1120, -5846, 29404 }, { -18345, 4807, -17835 }, { 27327, -1785, 15169 }, { 29148, -13944, 5419 }, { 28713, 1041, 8630 }, { 0, 0, 0 } },
    { { 2454, -21468, 20812 }, { 5017, -28804, -6722 }, { -4451, -28873, -6820 }, { -345, -20063, -22301 }, { -5442, -6512, -28775 }, { 14235, -18987, 18354 }, { 1829, -5829, 29371 }, { 1831, -5829, 29371 }, { -19114, 4608, -18067 }, { 27737, -1794, 15135 }, { 28608, -14080, 6272 }, { 29439, 636, 5739 }, { 0, 0, 0 } },
    { { 2396, -21278, 21012 }, { 5122, -28711, -7032 }, { -4176, -29130, -5829 }, { -592, -18967, -23236 }, { -6527, -6530, -28544 }, { 14748, -19190, 17727 }, { 2291, -6098, 29284 }, { 2308, -6099, 29283 }, { -19523, 4470, -18146 }, { 28079, -1808, 15101 }, { 29893, -14006, 5125 }, { 29665, 768, 4405 }, { 0, 0, 0 } },
    { { 2416, -21124, 21164 }, { 5131, -28640, -7308 }, { -4118, -29275, -5103 }, { -605, -18214, -23830 }, { -6192, -7025, -28501 }, { 14731, -18944, 18003 }, { 2578, -6329, 29211 }, { 2588, -6329, 29210 }, { -19693, 4482, -18133 }, { 28206, -1828, 15055 }, { 30688, -13904, 4315 }, { 29587, 1035, 4851 }, { 0, 0, 0 } },
    { { 2542, -20915, 21357 }, { 5015, -28578, -7625 }, { -4295, -29309, -4749 }, { -411, -17746, -24185 }, { -5300, -7731, -28498 }, { 14450, -18471, 18708 }, { 2875, -6439, 29159 }, { 2876, -6439, 29159 }, { -19745, 4569, -18119 }, { 28090, -1836, 14996 }, { 30483, -13901, 4435 }, { 29390, 1105, 5916 }, { 0, 0, 0 } },
    { { 2651, -20607, 21641 }, { 4909, -28481, -8047 }, { -4516, -29301, -4590 }, { -274, -17348, -24474 }, { -4634, -8577, -28372 }, { 14252, -17931, 19374 }, { 3340, -6497, 29097 }, { 3350, -6498, 29096 }, { -19695, 4632, -18119 }, { 27870, -1832, 14928 }, { 29851, -13957, 4969 }, { 29254, 1055, 6563 }, { 0, 0, 0 } },
    { { 2589, -20267, 21967 }, { 4960, -28326, -8547 }, { -4658, -29307, -4406 }, { -261, -16914, -24776 }, { -4213, -9432, -28165 }, { 14042, -17547, 19873 }, { 3954, -6571, 29003 }, { 3971, -6571, 29001 }, { -19506, 4655, -18090 }, { 27636, -1827, 14850 }, { 28990, -14071, 5738 }, { 29204, 858, 6812 }, { 0, 0, 0 } },
    { { 2356, -19973, 22260 }, { 5180, -28130, -9049 }, { -4749, -29339, -4085 }, { -147, -16488, -25062 }, { -3583, -10005, -28055 }, { 13583, -17457, 20267 }, { 4522, -6695, 28892 }, { 4523, -6695, 28891 }, { -19173, 4671, -18013 }, { 27341, -1836, 14757 }, { 27316, -14282, 7303 }, { 29069, -28, 7418 }, { 0, 0, 0 } },
    { { 2157, -19719, 22506 }, { 5413, -27937, -9499 }, { -4867, -29376, -3660 }, { 263, -16172, -25267 }, { -2263, -10051, -28176 }, { 12820, -17555, 20676 }, { 4827, -6844, 28807 }, { 4841, -6844, 28805 }, { -18746, 4701, -17942 }, { 26964, -1885, 14636 }, { 24228, -14471, 10260 }, { 28523, -2117, 9052 }, { 0, 0, 0 } },
    { { 2186, -19450, 22736 }, { 5460, -27790, -9894 }, { -5105, -29377, -3309 }, { 917, -16071, -25316 }, { 35, -9771, -28364 }, { 11994, -17385, 21305 }, { 5026, -6955, 28747 }, { 5031, -6955, 28746 }, { -18277, 4742, -17934 }, { 26638, -1993, 14507 }, { 21169, -14405, 13214 }, { 27348, -4387, 11525 }, { 0, 0, 0 } },
    { { 2383, -19232, 22901 }, { 5262, -27713, -10214 }, { -5561, -29295, -3295 }, { 1603, -16341, -25108 }, { 3468, -9981, -28078 }, { 11173, -16305, 22568 }, { 5637, -7087, 28601 }, { 5655, -7088, 28597 }, { -17725, 4772, -17929 }, { 26457, -2149, 14436 }, { 20388, -14134, 13945 }, { 25950, -4502, 14364 }, { 0, 0, 0 } },
    { { 2369, -19286, 22857 }, { 5057, -27690, -10377 }, { -6214, -29105, -3777 }, { 2097, -17176, -24507 }, { 8164, -11953, -26277 }, { 9309, -13626, 25053 }, { 6942, -7290, 28261 }, { 6955, -7289, 28258 }, { -16944, 4754, -17778 }, { 26141, -2267, 14503 }, { 20954, -13713, 13267 }, { 24385, -1097, 17440 }, { 0, 0, 0 } },
    { { 1839, -19761, 22497 }, { 5170, -27678, -10353 }, { -6990, -28810, -4598 }, { 2329, -18611, -23413 }, { 13199, -16295, -21453 }, { 4365, -8846, 28332 }, { 7654, -7472, 28028 }, { 8694, -7247, 27783 }, { -15701, 4659, -17379 }, { 25024, -2207, 14721 }, { 20271, -13247, 13569 }, { 21808, 4257, 20157 }, { 0, 0, 0 } },
    { { 723, -20604, 21794 }, { 5881, -27564, -10278 }, { -7814, -28503, -5152 }, { 2386, -20237, -22018 }, { 15376, -20914, -15040 }, { -3425, -4191, 29508 }, { 4647, -8475, 28400 }, { 13208, -6443, 26154 }, { -13769, 4503, -16713 }, { 23035, -2050, 14939 }, { 18286, -13215, 14989 }, { 18607, 6528, 22609 }, { 0, 0, 0 } },
    { { -736, -21723, 20677 }, { 7247, -27286, -10147 }, { -8787, -28222, -5131 }, { 2453, -21680, -20590 }, { 13827, -23909, -11711 }, { -11224, -2794, 27681 }, { -2068, -9597, 28348 }, { 17062, -7547, 23493 }, { -11256, 4186, -15937 }, { 20676, -1884, 14954 }, { 14707, -13889, 18202 }, { 16186, 4243, 24900 }, { 0, 0, 0 } },
    { { -2187, -23086, 19032 }, { 9220, -26860, -9670 }, { -10029, -27856, -4843 }, { 2797, -22949, -19118 }, { 11862, -24556, -12502 }, { -18029, -5342, 23375 }, { -11801, -9672, 25830 }, { 14455, -11154, 23804 }, { -8508, 3542, -15329 }, { 17593, -1687, 14647 }, { 3764, -13657, 28907 }, { 13640, -2083, 26639 }, { 0, 0, 0 } },
};
// clang-format on

// Jumping off an edge: Mixamo's running jump, from the take-off to the landing (retargeted the same way)
// clang-format off
static const Vec3s sGdfClipJump[21][13] = {
    { { -7754, -18489, 22317 }, { 5706, 4150, -29158 }, { -5307, -29308, 3587 }, { 9112, -17631, -22497 }, { 7783, -14040, -25344 }, { -4499, -27636, -10771 }, { -973, -26586, 13865 }, { 6353, 1342, 29289 }, { -15288, 5169, -17770 }, { 15428, -1617, 16683 }, { 16663, -14356, 16157 }, { 25981, -7858, 12776 }, { -22371, 1835, 19904 } },
    { { -12040, -1179, 27453 }, { 14529, -17153, -19866 }, { -4609, -28257, -8960 }, { 6716, -21494, -19822 }, { 1777, -17076, -24602 }, { -597, -26359, -14313 }, { -5021, -21612, 20192 }, { 16767, 3865, 24575 }, { -12548, 4202, -15038 }, { 17753, -1038, 16671 }, { 18665, -14270, 12233 }, { 27075, -8386, 9830 }, { -6303, 19349, 22042 } },
    { { -14004, -442, 26527 }, { 19954, -21907, -4682 }, { -3117, -28024, -10245 }, { 5887, -22950, -18403 }, { -2018, -17197, -24498 }, { 842, -23927, -18078 }, { -5224, -13917, 26058 }, { 24022, -449, 17966 }, { -11839, 3147, -12956 }, { 20650, -869, 16501 }, { 23598, -14458, 5765 }, { 27843, -9420, 6003 }, { 10299, 23210, 15976 } },
    { { -12289, -1777, 27310 }, { 19289, -22467, 4816 }, { -596, -28471, -9436 }, { 6240, -23131, -18056 }, { -843, -12938, -27054 }, { 8039, -20097, -20772 }, { -7438, -5595, 28520 }, { 20112, -4805, 21735 }, { -12316, 2770, -11922 }, { 22357, -1226, 16443 }, { 32253, -14802, -2421 }, { 29661, 391, 4479 }, { 10734, 24679, 13256 } },
    { { -9904, -1074, 28298 }, { 15524, -23140, 11116 }, { 3237, -28317, -9364 }, { 7000, -22491, -18578 }, { 780, -8373, -28797 }, { 14802, -13496, -22333 }, { -12508, 1714, 27214 }, { 8198, -3461, 28650 }, { -13130, 2663, -11977 }, { 22820, -1656, 16460 }, { -26380, -14357, -7907 }, { 25827, 14423, 4996 }, { 5379, 27885, 9669 } },
    { { -7508, 2730, 28917 }, { 9930, -23752, 15402 }, { 5285, -28015, -9338 }, { 9256, -19117, -21187 }, { 2833, -5958, -29266 }, { 18054, -8224, -22504 }, { -15317, 2387, 25685 }, { -1919, -1563, 29898 }, { -13942, 2517, -12841 }, { 22563, -1903, 16447 }, { -22673, -14221, -10558 }, { 15277, 24865, 6952 }, { -1419, 28753, 8440 } },
    { { -4218, 5215, 29241 }, { 4436, -23962, 17498 }, { 5465, -28537, -7468 }, { 11834, -13489, -24042 }, { 6170, -5328, -28871 }, { 20171, -5837, -21425 }, { -16761, -1258, 24849 }, { -8366, -3052, 28648 }, { -14687, 2192, -13726 }, { 22435, -1910, 16203 }, { -19862, -13890, -12986 }, { 4847, 28240, 8890 }, { -8014, 27740, 8142 } },
    { { -2122, 5248, 29461 }, { 1060, -23668, 18404 }, { 4477, -29346, -4335 }, { 13302, -7955, -25686 }, { 10240, -6032, -27546 }, { 22246, -5634, -19323 }, { -18811, -6304, 22504 }, { -11825, -7378, 26566 }, { -15186, 2098, -14209 }, { 22566, -1796, 15965 }, { -18189, -13446, -14646 }, { -628, 28523, 9277 }, { -12182, 26234, 7961 } },
    { { -1775, 3818, 29703 }, { -448, -23202, 19012 }, { 1568, -29959, 166 }, { 14232, -3546, -26170 }, { 13805, -6718, -25774 }, { 24322, -6182, -16438 }, { -20661, -11103, 18705 }, { -14633, -12218, 23165 }, { -15254, 2144, -14347 }, { 22608, -1787, 16052 }, { -17138, -13149, -15569 }, { -1378, 28618, 8896 }, { -14492, 24806, 8641 } },
    { { -2350, 1683, 29860 }, { -562, -23406, 18757 }, { -2061, -29467, 5237 }, { 14278, -1409, -26347 }, { 16767, -6232, -24084 }, { 25883, -6465, -13722 }, { -21811, -15074, 14037 }, { -16593, -16581, 18701 }, { -14993, 1967, -14326 }, { 22303, -1965, 16341 }, { -16177, -12927, -16218 }, { -994, 28430, 9526 }, { -15869, 23202, 10481 } },
    { { -2605, -825, 29875 }, { 81, -24012, 17984 }, { -4915, -27624, 10619 }, { 13120, -1569, -26933 }, { 19686, -4437, -22198 }, { 27306, -5755, -11011 }, { -22110, -18080, 9179 }, { -17668, -19916, 13827 }, { -14810, 1528, -14425 }, { 21609, -2225, 16632 }, { -15096, -12713, -16936 }, { -2816, 27980, 10449 }, { -17189, 21320, 12247 } },
    { { -1852, -3646, 29720 }, { 401, -24749, 16951 }, { -6343, -24702, 15799 }, { 11045, -3823, -27629 }, { 22936, -1337, -19291 }, { 28731, -3628, -7835 }, { -21733, -20285, 4023 }, { -18007, -22390, 8627 }, { -15069, 1093, -14807 }, { 20684, -2457, 16830 }, { -14126, -12477, -17584 }, { -7160, 27187, 10469 }, { -18896, 19254, 13123 } },
    { { -192, -6773, 29225 }, { 95, -25477, 15839 }, { -6529, -21243, 20152 }, { 8575, -7684, -27702 }, { 25951, 2905, -14768 }, { 29711, -210, -4145 }, { -21053, -21291, -1862 }, { -17624, -24052, 3297 }, { -15800, 832, -15451 }, { 19768, -2621, 16921 }, { -13598, -12325, -17901 }, { -11915, 25766, 9702 }, { -20876, 16935, 13318 } },
    { { 1861, -10290, 28119 }, { -535, -26092, 14796 }, { -5995, -17767, 23418 }, { 5985, -12431, -26639 }, { 27789, 7364, -8576 }, { 29743, 3914, -229 }, { -20093, -20528, -8655 }, { -16636, -24892, -1912 }, { -16810, 721, -16258 }, { 19017, -2678, 16917 }, { -13461, -12341, -17957 }, { -15490, 23924, 9366 }, { -22965, 14300, 12967 } },
    { { 3669, -13952, 26304 }, { -1209, -26655, 13714 }, { -5047, -14641, 25694 }, { 3266, -17454, -24180 }, { 28193, 10116, -1686 }, { 28858, 7127, 4055 }, { -17535, -18719, -15561 }, { -14887, -24988, -7347 }, { -17812, 673, -17059 }, { 18351, -2578, 16842 }, { -13576, -12431, -17956 }, { -17766, 22539, 8739 }, { -25227, 11976, 10964 } },
    { { 4878, -17306, 24014 }, { -1991, -27535, 11740 }, { -3635, -12196, 27167 }, { 509, -22249, -20117 }, { 27750, 10702, 3927 }, { 27209, 6954, 10550 }, { -12212, -17629, -20979 }, { -11617, -24533, -12773 }, { -18580, 683, -17685 }, { 17454, -2328, 16751 }, { -13884, -12411, -17971 }, { -18008, 23101, 6486 }, { -27260, 11015, 5962 } },
    { { 6115, -18752, 22605 }, { -3414, -28697, 8051 }, { -1953, -10289, 28113 }, { -1579, -26236, -14462 }, { 27500, 9428, 7409 }, { 21892, 2736, 20328 }, { -5302, -17696, -23637 }, { -6962, -23903, -16738 }, { -19037, 729, -18132 }, { 16173, -1901, 16662 }, { -14223, -12388, -18071 }, { -13739, 26257, 4669 }, { -27475, 11746, -2676 } },
    { { 6172, -20649, 20869 }, { -4449, -29355, 4303 }, { -336, -8423, 28791 }, { -2346, -29030, -7195 }, { 27410, 5053, 11099 }, { 9718, -1303, 28352 }, { 1099, -17908, -24043 }, { -2655, -23580, -18355 }, { -19366, 699, -18326 }, { 14691, -1372, 16495 }, { -14138, -12733, -18492 }, { -9109, 27639, 7289 }, { -23310, 13776, -12918 } },
    { { 4061, -24937, 16176 }, { -3867, -29634, 2622 }, { 810, -6852, 29196 }, { -1563, -29934, 1234 }, { 25217, -729, 16235 }, { -5564, -1969, 29414 }, { 6150, -18968, -22414 }, { -370, -24543, -17249 }, { -19743, 436, -18211 }, { 13246, -876, 16135 }, { -12994, -13718, -19587 }, { -11046, 27166, 6324 }, { -14210, 16156, -20906 } },
    { { 4086, -25610, 15080 }, { -7962, -28312, -5918 }, { 1265, -6247, 29315 }, { 329, -28593, 9072 }, { 19671, -5223, 22040 }, { -18435, 306, 23665 }, { 8799, -20438, -20121 }, { -2337, -25984, -14811 }, { -20100, 46, -17794 }, { 11990, -431, 15549 }, { -9860, -15044, -22466 }, { -11155, 27842, -601 }, { -3874, 16668, -24640 } },
    { { 3946, -26023, 14395 }, { -7840, -23982, -16230 }, { 1171, -6845, 29185 }, { 2368, -26843, 13186 }, { 12056, -8780, 26030 }, { -25806, 2904, 15021 }, { 9565, -22485, -17405 }, { -5885, -27659, -10018 }, { -20150, -83, -17463 }, { 11065, -42, 15213 }, { 728, -16032, -32734 }, { -8248, 28389, -5101 }, { 1801, 12801, -27072 } },
};
// clang-format on

// Ladders and vine walls: Mixamo's Climbing Ladder (one cycle, a hand and a foot each), and the end of its Climbing
// To Top, from the last rung over the top to standing (retargeted the same way)
// clang-format off
static const Vec3s sGdfClipLadder[24][13] = {
    { { 3434, -18973, 22983 }, { -10578, -27983, 2254 }, { -13527, -11231, 24308 }, { 13018, -23731, -12937 }, { 13546, 6896, 25864 }, { -14125, 18064, 19343 }, { -9928, -17178, 22503 }, { 4570, 20677, 21250 }, { -15920, 337, -15838 }, { 17487, 1863, 16915 }, { 16663, -14356, 16157 }, { 6378, 26207, -13135 }, { 2717, -28269, 9668 } },
    { { 3761, -19251, 22700 }, { -9827, -28303, 1543 }, { -16472, -7411, 23953 }, { 14439, -24033, -10675 }, { 13008, 4106, 26720 }, { -11512, 19650, 19528 }, { -10049, -16161, 23192 }, { 5536, 20185, 21492 }, { -15661, 80, -15714 }, { 17225, 2101, 16976 }, { 17671, -14285, 15155 }, { 6378, 26207, -13135 }, { 3130, -28335, 9344 } },
    { { 3421, -20283, 21838 }, { -8593, -28704, 1499 }, { -18588, -4723, 23069 }, { 14931, -24927, -7465 }, { 12434, 840, 27289 }, { -9289, 20663, 19666 }, { -10366, -16049, 23130 }, { 7469, 19749, 21311 }, { -15426, -210, -15616 }, { 16689, 2393, 16989 }, { 18267, -14281, 14460 }, { 6378, 26207, -13135 }, { 5753, -28487, 7442 } },
    { { 2414, -22036, 20213 }, { -7087, -29029, 2664 }, { -19641, -4352, 22255 }, { 14677, -25829, -4177 }, { 11925, -2884, 27377 }, { -8030, 20969, 19895 }, { -10816, -16931, 22279 }, { 9403, 19877, 20408 }, { -15046, -391, -15559 }, { 16083, 2607, 16901 }, { 19172, -14296, 13509 }, { 6378, 26207, -13135 }, { 9188, -28046, 5384 } },
    { { 168, -22789, 19509 }, { -4108, -29514, 3473 }, { -19477, -5909, 22040 }, { 13996, -26478, -1734 }, { 11303, -7108, 26865 }, { -7047, 21192, 20030 }, { -11436, -17570, 21459 }, { 11094, 21097, 18216 }, { -14620, -402, -15508 }, { 15548, 2677, 16700 }, { 20405, -14382, 12445 }, { 6378, 26207, -13135 }, { 11762, -27137, 5021 } },
    { { -554, -22670, 19640 }, { -1642, -29691, 3970 }, { -17479, -8205, 22960 }, { 12476, -27279, -467 }, { 10427, -11355, 25736 }, { -5558, 21736, 19916 }, { -11626, -15317, 23026 }, { 12647, 23959, 12885 }, { -14467, -218, -15477 }, { 15212, 2551, 16435 }, { 21633, -14494, 11557 }, { 6378, 26207, -13135 }, { 13483, -25552, 8081 } },
    { { 1822, -22795, 19418 }, { -1568, -29578, 4766 }, { -14479, -10884, 23915 }, { 10506, -28100, 121 }, { 9494, -14932, 24226 }, { -3787, 21926, 20122 }, { -11632, -10673, 25510 }, { 14208, 25460, 7067 }, { -14870, 106, -15517 }, { 15157, 2254, 16210 }, { 22661, -14515, 10840 }, { 6378, 26207, -13135 }, { 15523, -22691, 12006 } },
    { { 4761, -21479, 20396 }, { -2362, -29728, 3266 }, { -11623, -12836, 24498 }, { 8742, -28697, 188 }, { 8890, -16633, 23330 }, { -2444, 21856, 20405 }, { -12713, -7457, 26130 }, { 15744, 24856, 5858 }, { -15747, 339, -15714 }, { 15248, 1960, 16033 }, { 23197, -14418, 10488 }, { 6378, 26207, -13135 }, { 16556, -20425, 14447 } },
    { { 7645, -18577, 22281 }, { -4324, -29681, -569 }, { -9895, -13807, 24728 }, { 7791, -28970, -219 }, { 8816, -16266, 23616 }, { -1984, 21656, 20666 }, { -14055, -3390, 26286 }, { 16807, 23830, 7048 }, { -16669, 260, -15974 }, { 15417, 1891, 15880 }, { 23187, -14304, 10639 }, { 6378, 26207, -13135 }, { 16462, -18568, 16860 } },
    { { 10595, -15855, 23160 }, { -7515, -28782, -3888 }, { -8789, -14669, 24649 }, { 7316, -29089, -569 }, { 8956, -15095, 24330 }, { -2034, 21020, 21308 }, { -15257, 2156, 25741 }, { 18629, 21684, 9097 }, { -17289, -24, -16179 }, { 15757, 2022, 15704 }, { 22871, -14209, 11097 }, { 6378, 26207, -13135 }, { 15140, -17456, 19133 } },
    { { 13799, -12938, 23285 }, { -11402, -27075, -6079 }, { -7815, -15580, 24417 }, { 7051, -29147, -873 }, { 8892, -14377, 24784 }, { -1867, 20574, 21754 }, { -15267, 7507, 24710 }, { 20083, 18600, 12277 }, { -17644, -220, -16330 }, { 15976, 2172, 15478 }, { 22374, -14083, 11663 }, { 6378, 26207, -13135 }, { 12853, -17654, 20571 } },
    { { 16736, -10107, 22754 }, { -14944, -25107, -6804 }, { -6344, -16191, 24446 }, { 6365, -29280, -1472 }, { 8691, -14701, 24665 }, { -1143, 20854, 21536 }, { -14020, 9900, 24606 }, { 19117, 17322, 15313 }, { -17977, -185, -16353 }, { 15833, 2212, 15290 }, { 21642, -13949, 12316 }, { 7408, 26282, -12424 }, { 10300, -19295, 20534 } },
    { { 18957, -7327, 22067 }, { -17644, -23436, -6280 }, { -5607, -17015, 24064 }, { 6217, -29304, -1615 }, { 8899, -14798, 24532 }, { -1545, 20772, 21590 }, { -13836, 7808, 25448 }, { 16955, 17969, 17018 }, { -18358, -54, -16321 }, { 15750, 2189, 15191 }, { 20780, -13853, 13027 }, { 9586, 25985, -11529 }, { 6698, -23891, 16863 } },
    { { 20157, -5720, 21470 }, { -19336, -22474, -4586 }, { -4346, -18213, 23439 }, { 5221, -29504, -1502 }, { 9617, -14242, 24590 }, { -3552, 20356, 21749 }, { -14622, 2967, 26027 }, { 14957, 18866, 17899 }, { -18753, -3, -16342 }, { 16248, 2210, 15161 }, { 20178, -13783, 13576 }, { 9628, 25391, -12751 }, { 3284, -27770, 10866 } },
    { { 20420, -5698, 21227 }, { -20182, -22104, -2027 }, { -2766, -20062, 22133 }, { 3610, -29777, -525 }, { 10339, -12814, 25078 }, { -6612, 20032, 21331 }, { -14998, -1977, 25907 }, { 13398, 19649, 18287 }, { -19169, -5, -16458 }, { 17179, 2258, 15187 }, { 20100, -13767, 13714 }, { 4424, 24121, -17281 }, { 2717, -28269, 9668 } },
    { { 19802, -6900, 21454 }, { -20249, -22120, 817 }, { -1331, -21907, 20453 }, { 1830, -29918, 1257 }, { 11017, -10182, 25980 }, { -9835, 20223, 19857 }, { -14850, -6995, 25110 }, { 11966, 20426, 18428 }, { -19645, 81, -16642 }, { 18147, 2220, 15294 }, { 20493, -13920, 13318 }, { -1372, 18402, -23654 }, { 2717, -28269, 9668 } },
    { { 17926, -8669, 22439 }, { -19165, -22926, 2667 }, { 209, -23227, 18985 }, { -454, -29851, 2950 }, { 11914, -6846, 26668 }, { -12687, 20446, 17917 }, { -14059, -11962, 23649 }, { 10262, 21570, 18150 }, { -20092, 287, -16861 }, { 18826, 2051, 15493 }, { 21359, -14231, 12256 }, { -311, 10535, -28088 }, { 2717, -28269, 9668 } },
    { { 15043, -10716, 23640 }, { -17010, -24474, 3418 }, { 1307, -23839, 18166 }, { -2456, -29728, 3197 }, { 13047, -3174, 26827 }, { -15139, 20586, 15717 }, { -12757, -15842, 22052 }, { 8234, 22745, 17744 }, { -20245, 497, -17061 }, { 18996, 1786, 15722 }, { 22578, -14438, 10721 }, { 4557, 6387, -28956 }, { 2717, -28269, 9668 } },
    { { 11947, -13171, 24162 }, { -14646, -25930, 3624 }, { -113, -22966, 19302 }, { -1893, -29938, 346 }, { 14307, 586, 26362 }, { -17157, 20450, 13691 }, { -11356, -17922, 21210 }, { 6415, 23427, 17607 }, { -19890, 605, -17110 }, { 18743, 1543, 15966 }, { 23347, -14378, 9691 }, { 10068, 8767, -26866 }, { 2717, -28269, 9668 } },
    { { 9301, -15445, 23978 }, { -12988, -26833, 3359 }, { -2107, -21065, 21256 }, { 21, -29706, -4189 }, { 14641, 4188, 25848 }, { -17407, 21014, 12466 }, { -10321, -18408, 21322 }, { 5374, 23440, 17936 }, { -19074, 582, -16952 }, { 18367, 1465, 16212 }, { 23087, -14222, 9849 }, { 12939, 11568, -24469 }, { 2717, -28269, 9668 } },
    { { 7463, -16864, 23662 }, { -12280, -27207, 2999 }, { -4617, -18543, 23127 }, { 3152, -28638, -8364 }, { 13719, 8026, 25444 }, { -16415, 21359, 13204 }, { -9823, -17791, 22068 }, { 4950, 22795, 18865 }, { -18074, 475, -16654 }, { 17985, 1579, 16405 }, { 21670, -14169, 11311 }, { 13354, 14409, -22672 }, { 2717, -28269, 9668 } },
    { { 6195, -17590, 23499 }, { -11966, -27380, 2676 }, { -7670, -16066, 24146 }, { 6890, -26877, -11408 }, { 12516, 10597, 25121 }, { -15419, 20451, 15620 }, { -9632, -16950, 22802 }, { 4675, 21865, 20002 }, { -17175, 375, -16304 }, { 17643, 1767, 16563 }, { 19220, -14174, 13828 }, { 11535, 19021, -20129 }, { 2717, -28269, 9668 } },
    { { 4891, -18208, 23335 }, { -11366, -27657, 2426 }, { -10841, -13501, 24499 }, { 10362, -25037, -12877 }, { 12447, 10009, 25395 }, { -14844, 18815, 18046 }, { -9666, -16684, 22983 }, { 4534, 21089, 20849 }, { -16484, 332, -15995 }, { 17482, 1877, 16738 }, { 17093, -14177, 15889 }, { 8703, 23796, -16062 }, { 2717, -28269, 9668 } },
    { { 3434, -18973, 22983 }, { -10578, -27983, 2254 }, { -13527, -11231, 24308 }, { 13018, -23731, -12937 }, { 13453, 6934, 25903 }, { -14084, 18043, 19392 }, { -9943, -17131, 22531 }, { 4528, 20650, 21285 }, { -15920, 337, -15838 }, { 17487, 1863, 16915 }, { 16663, -14356, 16157 }, { 6378, 26207, -13135 }, { 2717, -28269, 9668 } },
};
// clang-format on

// clang-format off
static const Vec3s sGdfClipLadderTop[34][13] = {
    { { 1159, -24878, 16726 }, { -3257, -16231, -25019 }, { -1974, -24178, 17651 }, { 3213, -29779, 1698 }, { 12760, -27135, 933 }, { -28071, -8211, 6679 }, { -7945, -3479, -28719 }, { -5546, -16436, 24477 }, { -15034, 8068, -14447 }, { 14382, -7416, 17802 }, { -19234, -10040, -10020 }, { -17375, 15178, 19177 }, { 26763, 7220, 11472 } },
    { { 645, -23048, 19193 }, { -2025, -9501, -28384 }, { -1048, -25031, 16502 }, { 3440, -29752, 1733 }, { 13359, -26176, 6028 }, { -28286, -7042, 7094 }, { -5335, -803, -29511 }, { -5680, -19620, 21973 }, { -15470, 7851, -14505 }, { 14520, -7338, 17796 }, { -19813, -9883, -9423 }, { -15870, 18049, 17956 }, { 26299, 8559, 11623 } },
    { { 1920, -19564, 22662 }, { -2355, -4494, -29568 }, { -405, -25390, 15975 }, { 3868, -29715, 1432 }, { 12118, -26421, 7421 }, { -28541, -4437, 8107 }, { -3444, 2090, -29728 }, { -4970, -22438, 19283 }, { -15821, 7775, -14412 }, { 14624, -7322, 17858 }, { -20516, -9390, -8708 }, { -11275, 22192, 16745 }, { 26206, 8814, 11642 } },
    { { 3994, -12114, 27153 }, { -3325, -5930, -29220 }, { 78, -25333, 16070 }, { 4355, -29667, 941 }, { 9908, -27342, 7367 }, { -28507, -1454, 9233 }, { -3082, 4653, -29476 }, { -3382, -24357, 17184 }, { -15993, 8187, -14255 }, { 14596, -7536, 17923 }, { -21035, -8774, -8142 }, { -7217, 26095, 12922 }, { 26281, 8966, 11355 } },
    { { 6021, 311, 29388 }, { -4302, -16500, -24683 }, { 566, -25271, 16157 }, { 4687, -29627, 500 }, { 7865, -27951, 7544 }, { -27995, 762, 10756 }, { -3947, 6275, -29070 }, { -1506, -25138, 16304 }, { -15990, 8939, -14057 }, { 14437, -7908, 17919 }, { -21150, -8177, -7827 }, { -3950, 28862, 7167 }, { 26286, 9292, 11077 } },
    { { 6292, 7466, 28367 }, { -3740, -25733, -14960 }, { 1292, -25567, 15641 }, { 4752, -29610, 833 }, { 6778, -28066, 8147 }, { -27290, 1444, 12375 }, { -4945, 7393, -28651 }, { 223, -25329, 16075 }, { -16075, 9722, -13932 }, { 14269, -8285, 17845 }, { -20918, -7614, -7753 }, { -1750, 29590, 4623 }, { 26244, 9461, 11033 } },
    { { 5580, 7353, 28545 }, { -2905, -28766, -8006 }, { 2007, -25977, 14871 }, { 4751, -29597, 1203 }, { 6237, -28079, 8526 }, { -26808, 818, 13440 }, { -5566, 9353, -27956 }, { 1547, -25966, 14945 }, { -16245, 10220, -14068 }, { 14207, -8509, 17707 }, { -20598, -7062, -7874 }, { -757, 29623, 4683 }, { 26488, 9059, 10785 } },
    { { 5343, 5845, 28936 }, { -2386, -29023, -7209 }, { 2651, -26943, 12925 }, { 4553, -29599, 1774 }, { 4888, -28502, 7984 }, { -26848, -98, 13385 }, { -5777, 12708, -26554 }, { 2250, -27650, 11421 }, { -16269, 10344, -14254 }, { 14217, -8563, 17547 }, { -20313, -6478, -8201 }, { 400, 29041, 7514 }, { 27146, 8063, 9904 } },
    { { 5040, 2435, 29473 }, { -2256, -28029, -10453 }, { 3328, -28502, 8749 }, { 4403, -29655, 1080 }, { 2339, -29181, 6558 }, { -26945, -1259, 13129 }, { -6872, 13851, -25709 }, { 3331, -29231, 5868 }, { -16040, 10437, -14259 }, { 14180, -8678, 17321 }, { -19886, -5806, -8851 }, { 2327, 27148, 12553 }, { 27945, 6762, 8564 } },
    { { 4070, -3953, 29459 }, { -1951, -26278, -14341 }, { 3856, -29421, 4418 }, { 4861, -29369, -3721 }, { -958, -29667, 4353 }, { -26571, -3467, 13489 }, { -8337, 10141, -26975 }, { 3309, -29791, 1251 }, { -15726, 10489, -14243 }, { 14117, -8877, 16927 }, { -19165, -5044, -9826 }, { 4672, 23304, 18305 }, { 28606, 5429, 7226 } },
    { { 2884, -9613, 28271 }, { -1629, -25298, -16042 }, { 4435, -29657, -900 }, { 5225, -28577, -7487 }, { -3501, -29517, 4063 }, { -25288, -5630, 15127 }, { -8912, 5048, -28198 }, { 1230, -29917, -1865 }, { -15479, 10489, -14317 }, { 14092, -9102, 16441 }, { -18224, -4281, -10942 }, { 7073, 18383, 22628 }, { 29118, 4303, 5799 } },
    { { 2077, -13375, 26773 }, { -1581, -25046, -16438 }, { 4470, -29662, -403 }, { 5491, -25969, -13980 }, { -6042, -28951, 5034 }, { -23972, -7681, 16320 }, { -10088, 357, -28251 }, { -813, -29639, -4567 }, { -15500, 10494, -14496 }, { 14298, -9264, 15990 }, { -17310, -3659, -11952 }, { 8565, 13646, 25307 }, { 29541, 3230, 4113 } },
    { { 1277, -16667, 24911 }, { -1331, -25118, -16349 }, { 4027, -29666, 1923 }, { 5150, -22654, -18980 }, { -8073, -27869, 7626 }, { -22293, -9375, 17752 }, { -11436, -4189, -27417 }, { -2361, -28905, -7675 }, { -15880, 10519, -14984 }, { 14889, -9278, 15497 }, { -16579, -3350, -12807 }, { 9350, 10433, 26528 }, { 29506, 1534, 5200 } },
    { { 971, -19853, 22470 }, { -1502, -25463, -15792 }, { 3246, -28924, 7272 }, { 3775, -19055, -22862 }, { -8014, -26769, 10918 }, { -20134, -9490, 20113 }, { -11245, -7610, -26751 }, { -3123, -27756, -10948 }, { -16382, 10325, -15625 }, { 15584, -9061, 15113 }, { -15925, -3369, -13574 }, { 9871, 9829, 26570 }, { 28904, -1568, 7879 } },
    { { 1227, -22920, 19317 }, { -1996, -25786, -15202 }, { 2797, -26317, 14128 }, { 1718, -15889, -25388 }, { -7023, -26474, 12240 }, { -19345, -8763, 21189 }, { -9596, -9428, -26815 }, { -2820, -26310, -14135 }, { -16953, 9713, -16238 }, { 16112, -8542, 14935 }, { -15386, -3511, -14175 }, { 10150, 11339, 25853 }, { 27513, -5960, 10369 } },
    { { 1765, -25708, 15362 }, { -2236, -26051, -14709 }, { 2472, -22467, 19726 }, { 388, -15623, -25608 }, { -6384, -27278, 10731 }, { -21217, -7091, 19989 }, { -7693, -9586, -27367 }, { -1783, -24948, -16565 }, { -17561, 8802, -16667 }, { 16368, -7766, 14842 }, { -15392, -3782, -14498 }, { 11009, 13706, 24310 }, { 25428, -11012, 11496 } },
    { { 2739, -27867, 10767 }, { -2459, -26445, -13950 }, { 2131, -19249, 22912 }, { 19, -19011, -23208 }, { -4103, -28341, 8943 }, { -23322, -4991, 18199 }, { -6743, -8652, -27923 }, { -1059, -23890, -18115 }, { -18343, 7801, -17295 }, { 16437, -6885, 14584 }, { -16246, -4448, -14599 }, { 13167, 14665, 22618 }, { 23234, -15656, 10727 } },
    { { 3309, -29298, 5538 }, { -2154, -27089, -12711 }, { 1309, -18067, 23914 }, { 574, -23448, -18704 }, { -2296, -28840, 7938 }, { -25127, -4928, 15632 }, { -5513, -7263, -28581 }, { 148, -23761, -18314 }, { -18882, 6937, -17771 }, { 16246, -6098, 14401 }, { -17572, -5619, -14544 }, { 15425, 11718, 22908 }, { 21716, -18887, 8469 } },
    { { 3285, -29784, 1460 }, { -1616, -27573, -11710 }, { -581, -18467, 23635 }, { 2207, -26933, -13029 }, { 2646, -28936, 7465 }, { -24645, -7293, 15473 }, { -3987, -6753, -28957 }, { 1628, -24488, -17254 }, { -19104, 6292, -17902 }, { 15862, -5439, 14484 }, { -18707, -7105, -14283 }, { 17033, 7962, 23377 }, { 21377, -20237, 5785 } },
    { { 3087, -29787, -1793 }, { -803, -27685, -11528 }, { -2261, -20259, 22011 }, { 3197, -29247, -5866 }, { 4481, -28953, 6453 }, { -22715, -12594, 15014 }, { -2850, -6928, -29049 }, { 2309, -25642, -15400 }, { -19291, 5923, -17877 }, { 15472, -4918, 14743 }, { -19595, -8720, -13714 }, { 17832, 5268, 23543 }, { 22187, -19935, 3215 } },
    { { 3377, -29673, -2844 }, { -633, -27104, -12844 }, { -1932, -23133, 19004 }, { 2194, -29892, 1276 }, { 4684, -29238, 4817 }, { -19152, -17941, 14537 }, { -1918, -6984, -29113 }, { 2971, -27041, -12646 }, { -19489, 5789, -17802 }, { 15283, -4535, 15076 }, { -20802, -10255, -12587 }, { 18632, 6579, 22574 }, { 24745, -16881, 1651 } },
    { { 4345, -29673, -787 }, { -1771, -25225, -16142 }, { -506, -26416, 14211 }, { 1110, -29185, 6855 }, { 5748, -29234, 3515 }, { -12850, -22642, 14908 }, { -2614, -7665, -28886 }, { 2429, -28958, -7454 }, { -19555, 5572, -17685 }, { 15542, -4146, 15403 }, { -22948, -11313, -10415 }, { 19293, 10174, 20598 }, { 26920, -13239, 113 } },
    { { 5752, -29271, 3182 }, { -4111, -21986, -19993 }, { -138, -27402, 12211 }, { 1204, -29020, 7509 }, { 7425, -28975, 2309 }, { -4115, -26351, 13736 }, { -2914, -8834, -28521 }, { 625, -29939, -1803 }, { -19614, 5167, -17621 }, { 16384, -3709, 15638 }, { -25446, -11487, -7584 }, { 19414, 14356, 17804 }, { 28103, -10379, -1576 } },
    { { 7485, -27820, 8367 }, { -8029, -19170, -21635 }, { -1391, -25882, 15105 }, { 2173, -29751, 3184 }, { 9645, -28387, 1082 }, { 4699, -28017, 9642 }, { -3881, -10362, -27885 }, { -2166, -29815, 2530 }, { -19755, 4756, -17574 }, { 17462, -3322, 15789 }, { -26414, -11028, -6018 }, { 18810, 17690, 15273 }, { 28282, -8810, -4744 } },
    { { 7777, -25515, 13729 }, { -9124, -19226, -21145 }, { -2938, -23583, 18309 }, { 3485, -29737, -1892 }, { 11025, -27888, 857 }, { 10835, -27217, 6466 }, { -3989, -13632, -26425 }, { -2472, -29770, 2763 }, { -19787, 4534, -17512 }, { 18370, -3080, 15909 }, { -25621, -10446, -6226 }, { 18282, 20175, 12599 }, { 26932, -9583, -9100 } },
    { { 7341, -22850, 18000 }, { -6915, -21917, -19283 }, { -3581, -23099, 18805 }, { 4147, -29340, -4684 }, { 13356, -26840, -1112 }, { 14686, -25801, 4317 }, { -6382, -17645, -23408 }, { -2122, -29865, 1898 }, { -19513, 4444, -17357 }, { 18969, -2922, 16073 }, { -24167, -9880, -7228 }, { 18844, 20104, 11863 }, { 23760, -14864, -10700 } },
    { { 7286, -21125, 20016 }, { -2956, -24994, -16327 }, { -4322, -23587, 18027 }, { 4805, -28974, -6116 }, { 15567, -25351, -3870 }, { 17402, -24422, 857 }, { -8910, -20906, -19584 }, { -1001, -29800, 3309 }, { -19005, 4351, -17070 }, { 19223, -2757, 16285 }, { -22943, -9455, -8127 }, { 21896, 18918, 7914 }, { 19871, -21379, -6935 } },
    { { 7431, -21154, 19932 }, { 1489, -27513, -11866 }, { -4940, -24167, 17075 }, { 5059, -28667, -7251 }, { 16994, -23467, -7778 }, { 19145, -23035, -1692 }, { -11593, -24492, -12874 }, { -521, -28849, 8215 }, { -18399, 4207, -16719 }, { 19049, -2547, 16504 }, { -22179, -9420, -8707 }, { 23450, 18629, 1748 }, { 17965, -24026, -135 } },
    { { 7761, -22797, 17890 }, { 5751, -28670, -6706 }, { -4957, -24772, 16180 }, { 4353, -28491, -8326 }, { 17796, -22197, -9520 }, { 19752, -22580, -8 }, { -12037, -27204, -3881 }, { -2089, -27216, 12446 }, { -17722, 4031, -16387 }, { 18528, -2296, 16702 }, { -21684, -9830, -9107 }, { 23001, 19012, -3082 }, { 18388, -23187, 4921 } },
    { { 8554, -24816, 14525 }, { 8577, -28672, -2081 }, { -4897, -25397, 15198 }, { 3131, -28393, -9166 }, { 18127, -22031, -9278 }, { 19474, -22524, 3665 }, { -10524, -27914, 3173 }, { -4822, -26404, 13401 }, { -17070, 3860, -16131 }, { 17950, -2062, 16853 }, { -21311, -10492, -9394 }, { 22963, 18899, -3940 }, { 20058, -21610, 5539 } },
    { { 9207, -26321, 11065 }, { 9414, -28464, 1077 }, { -5196, -26030, 13979 }, { 1955, -28370, -9556 }, { 18137, -22135, -9005 }, { 18199, -22825, 6917 }, { -9792, -27762, 5779 }, { -5710, -25462, 14802 }, { -16625, 3694, -15974 }, { 17556, -1921, 16933 }, { -21084, -11097, -9542 }, { 23049, 19014, -2684 }, { 21498, -20534, 4028 } },
    { { 8931, -26951, 9688 }, { 8719, -28666, 1492 }, { -5792, -26576, 12655 }, { 883, -28426, -9550 }, { 17887, -22343, -8992 }, { 15696, -23472, 10134 }, { -10741, -27671, 4350 }, { -5524, -23217, 18178 }, { -16437, 3516, -15942 }, { 17372, -1894, 16939 }, { -21035, -11485, -9568 }, { 22987, 19275, -267 }, { 21114, -20862, 4359 } },
    { { 8216, -27059, 10017 }, { 7469, -29055, 93 }, { -6417, -26974, 11456 }, { -165, -28508, -9341 }, { 17402, -22791, -8817 }, { 12267, -24038, 13103 }, { -11974, -27507, 131 }, { -4684, -20144, 21732 }, { -16404, 3400, -15968 }, { 17269, -1935, 16925 }, { -21203, -11745, -9459 }, { 22601, 19573, 2468 }, { 20465, -20904, 6649 } },
    { { 7754, -27078, 10327 }, { 6394, -29294, -991 }, { -6894, -27206, 10598 }, { -1135, -28522, -9231 }, { 16794, -23306, -8650 }, { 8974, -24090, 15465 }, { -12194, -27072, -4294 }, { -3819, -17798, 23846 }, { -16412, 3423, -15963 }, { 17200, -1993, 16937 }, { -21639, -11989, -9177 }, { 22034, 19897, 4313 }, { 19984, -20531, 8893 } },
};
// clang-format on

// ---- The dance emote (the Ganondorf menu) ----
// The Cucui Ganon dance, from MMadness' "Suavemente" motion kit for MikuMikuDance, retargeted offline to his skeleton
// like the Mixamo clips: the kit's intro and then its four dances, one after another, each blended into the next; after
// the fourth, the four again, until the song starts over. sGdfClipDanceHips: how far his hips sway (x, z in his model
// space) so his feet stay put. The song (the player's own .wav, see GanondorfDance.cpp) sets the time, so he keeps
// with it.
#define GDF_DANCE_RATE 15.0f // rows a second

void BossGanon_Player_Dance(BossGanon* this, PlayState* play);
void BossGanon_Player_SetupFloat(BossGanon* this, PlayState* play);
static f32 sGdfDanceClock = 0.0f; // without the song: his own count, in seconds

void BossGanon_Player_SetupDance(BossGanon* this, PlayState* play) {
    // Play as Ganondorf (2S2H): not ported yet
}

void BossGanon_Player_Dance(BossGanon* this, PlayState* play) {
    // Play as Ganondorf (2S2H): not ported yet
}

typedef struct {
    s8 t;    // the attack's frame (20 a second)
    s8 clip; // the clip's (30 a second)
} GdfClipKey;

// Applies a clip's frame (between frames: blended) to his pose, by `weight`
static void BossGanon_Player_BlendRot(GdfMat3 cur, GdfMat3 target, f32 weight, GdfMat3 out) {
    GdfMat3 delta;
    f32 axis[3];
    f32 len;
    f32 angle;
    f32 c;
    f32 sn;
    f32 v;
    GdfMat3 rot;

    BossGanon_Pg_Mul(cur, target, delta, true); // cur^T * target: the turn left, in cur's own frame
    axis[0] = delta[2][1] - delta[1][2];
    axis[1] = delta[0][2] - delta[2][0];
    axis[2] = delta[1][0] - delta[0][1];
    len = sqrtf(SQ(axis[0]) + SQ(axis[1]) + SQ(axis[2]));
    angle = atan2f(len * 0.5f, CLAMP((delta[0][0] + delta[1][1] + delta[2][2] - 1.0f) * 0.5f, -1.0f, 1.0f));
    if ((len < 1e-6f) || (weight >= 1.0f)) {
        memcpy(out, (weight >= 1.0f) ? target : cur, sizeof(GdfMat3));
        return;
    }
    axis[0] /= len;
    axis[1] /= len;
    axis[2] /= len;
    angle *= weight;
    c = cosf(angle);
    sn = sinf(angle);
    v = 1.0f - c;
    rot[0][0] = c + axis[0] * axis[0] * v;
    rot[0][1] = axis[0] * axis[1] * v - axis[2] * sn;
    rot[0][2] = axis[0] * axis[2] * v + axis[1] * sn;
    rot[1][0] = axis[1] * axis[0] * v + axis[2] * sn;
    rot[1][1] = c + axis[1] * axis[1] * v;
    rot[1][2] = axis[1] * axis[2] * v - axis[0] * sn;
    rot[2][0] = axis[2] * axis[0] * v - axis[1] * sn;
    rot[2][1] = axis[2] * axis[1] * v + axis[0] * sn;
    rot[2][2] = c + axis[2] * axis[2] * v;
    BossGanon_Pg_Mul(cur, rot, out, false);
}

static void BossGanon_Player_ClipDir(const Vec3s (*clip)[13], s32 a, s32 b, f32 w, s32 i, f32 out[3]) {
    f32 len;

    out[0] = clip[a][i].x + (clip[b][i].x - clip[a][i].x) * w;
    out[1] = clip[a][i].y + (clip[b][i].y - clip[a][i].y) * w;
    out[2] = clip[a][i].z + (clip[b][i].z - clip[a][i].z) * w;
    len = sqrtf(SQ(out[0]) + SQ(out[1]) + SQ(out[2]));
    if (len < 1.0f) {
        out[0] = 0.0f;
        out[1] = -1.0f;
        out[2] = 0.0f;
        return;
    }
    out[0] /= len;
    out[1] /= len;
    out[2] /= len;
}

static void BossGanon_Player_ClipRot(const Vec3s (*clip)[13], s32 a, s32 b, f32 w, s32 i, GdfMat3 out) {
    GdfMat3 ra;
    GdfMat3 rb;

    BossGanon_Pg_FromRot((Vec3s*)&clip[a][8 + i], ra);
    BossGanon_Pg_FromRot((Vec3s*)&clip[b][8 + i], rb);
    BossGanon_Player_BlendRot(ra, rb, w, out);
}

// An arm ApplyClip leaves as it is (1: his right, holding the trident as he walks with it), or -1
static s8 sGdfClipKeepArm = -1;

// The back of each hand at rest, in its forearm's own frame (left, right): his fists hang with their backs to his sides
static const f32 sGdfHandBack[2][3] = { { 0.0f, 0.9556f, 0.29468f }, { 0.0f, 0.91925f, -0.39368f } };

// Rolls `limb` about its own bone (its x) so its `local` axis faces as near `want` (model space) as it can, by `weight`
static void BossGanon_Player_RollBoneToward(GdfMat3 limb, const f32 local[3], const f32 want[3], f32 weight) {
    f32 axis[3] = { limb[0][0], limb[1][0], limb[2][0] };
    f32 have[3];
    f32 to[3];
    f32 d;
    f32 cr[3];
    f32 angle;
    f32 c;
    f32 sn;
    f32 v;
    s32 i;
    GdfMat3 rot;
    GdfMat3 out;

    for (i = 0; i < 3; i++) {
        have[i] = limb[i][0] * local[0] + limb[i][1] * local[1] + limb[i][2] * local[2];
    }
    // Both across the bone
    d = have[0] * axis[0] + have[1] * axis[1] + have[2] * axis[2];
    for (i = 0; i < 3; i++) {
        have[i] -= axis[i] * d;
    }
    d = want[0] * axis[0] + want[1] * axis[1] + want[2] * axis[2];
    for (i = 0; i < 3; i++) {
        to[i] = want[i] - axis[i] * d;
    }
    if ((SQ(have[0]) + SQ(have[1]) + SQ(have[2]) < 1e-6f) || (SQ(to[0]) + SQ(to[1]) + SQ(to[2]) < 1e-6f)) {
        return;
    }
    cr[0] = have[1] * to[2] - have[2] * to[1];
    cr[1] = have[2] * to[0] - have[0] * to[2];
    cr[2] = have[0] * to[1] - have[1] * to[0];
    angle = atan2f(cr[0] * axis[0] + cr[1] * axis[1] + cr[2] * axis[2],
                   have[0] * to[0] + have[1] * to[1] + have[2] * to[2]) *
            weight;
    c = cosf(angle);
    sn = sinf(angle);
    v = 1.0f - c;
    rot[0][0] = c + axis[0] * axis[0] * v;
    rot[0][1] = axis[0] * axis[1] * v - axis[2] * sn;
    rot[0][2] = axis[0] * axis[2] * v + axis[1] * sn;
    rot[1][0] = axis[1] * axis[0] * v + axis[2] * sn;
    rot[1][1] = c + axis[1] * axis[1] * v;
    rot[1][2] = axis[1] * axis[2] * v - axis[0] * sn;
    rot[2][0] = axis[2] * axis[0] * v - axis[1] * sn;
    rot[2][1] = axis[2] * axis[1] * v + axis[0] * sn;
    rot[2][2] = c + axis[2] * axis[2] * v;
    BossGanon_Pg_Mul(rot, limb, out, false);
    memcpy(limb, out, sizeof(GdfMat3));
}

// `body`: how much of its pelvis, chest and head turns to take. (Every clip's are the source's own posture in the
// world, built from its hips, shoulders, neck and head, so all of them take it in full. Until 8 Nov they were turns
// from his idle stance instead, which leans back and twists 30 degrees, so the clips leaned him back and twisted him
// off to one side, and took them only in part.) The head is the exception: it's the source's head bone's own turn from
// rest, put on his standing head, so he looks where the source looks (built from its neck and head instead, it tipped
// his face up by 30 to 75 degrees). His feet keep how they sit on his shins (the clip's are set for a different foot:
// angled wrong on his).
static void BossGanon_Player_ApplyClip(BossGanon* this, const Vec3s (*clip)[13], s32 count, f32 frame, s32 loop,
                                       f32 weight, f32 body) {
    static const s8 sLegs[2][4] = { { 19, 20, 21, 0 }, { 22, 23, 24, 2 } }; // thigh, shin, foot, first direction
    static const s8 sArms[2][4] = { { 2, 3, 4, 4 }, { 6, 7, 8, 6 } };       // shoulder, upper arm, forearm, direction
    Vec3s* joints = this->skelAnime.jointTable;
    s32 a;
    s32 b;
    f32 w;
    s32 side;
    GdfMat3 root;
    GdfMat3 cur;
    GdfMat3 target;
    GdfMat3 pelvis;
    GdfMat3 chest;
    GdfMat3 tmp;
    GdfMat3 local;
    f32 dir[3];

    if (weight <= 0.0f) {
        return;
    }
    weight = MIN(weight, 1.0f);
    if (loop) {
        frame = fmodf(frame, (f32)count);
        if (frame < 0.0f) {
            frame += count;
        }
    } else {
        frame = CLAMP(frame, 0.0f, count - 1.0f);
    }
    a = (s32)frame;
    w = frame - a;
    b = loop ? ((a + 1) % count) : MIN(a + 1, count - 1);

    BossGanon_Pg_FromRot(&joints[1 + 0], root);

    // Pelvis and chest (children of his root)
    BossGanon_Pg_FromRot(&joints[1 + 18], tmp);
    BossGanon_Pg_Mul(root, tmp, cur, false);
    BossGanon_Player_ClipRot(clip, a, b, w, 0, target);
    BossGanon_Player_BlendRot(cur, target, weight * body, pelvis);
    BossGanon_Pg_Mul(root, pelvis, local, true);
    BossGanon_Pg_ToRot(local, &joints[1 + 18]);

    BossGanon_Pg_FromRot(&joints[1 + 1], tmp);
    BossGanon_Pg_Mul(root, tmp, cur, false);
    BossGanon_Player_ClipRot(clip, a, b, w, 1, target);
    BossGanon_Player_BlendRot(cur, target, weight * body, chest);
    BossGanon_Pg_Mul(root, chest, local, true);
    BossGanon_Pg_ToRot(local, &joints[1 + 1]);

    // Head (the limb his face hangs from)
    BossGanon_Pg_FromRot(&joints[1 + 10], tmp);
    BossGanon_Pg_Mul(chest, tmp, cur, false);
    BossGanon_Player_ClipRot(clip, a, b, w, 2, target);
    BossGanon_Player_BlendRot(cur, target, weight * body, tmp);
    BossGanon_Pg_Mul(chest, tmp, local, true);
    BossGanon_Pg_ToRot(local, &joints[1 + 10]);

    for (side = 0; side < 2; side++) {
        GdfMat3 thigh;
        GdfMat3 shin;

        BossGanon_Pg_FromRot(&joints[1 + sLegs[side][0]], tmp);
        BossGanon_Pg_Mul(pelvis, tmp, thigh, false);
        BossGanon_Player_ClipDir(clip, a, b, w, sLegs[side][3], dir);
        BossGanon_Player_SwingBoneToward(thigh, dir, weight);
        BossGanon_Pg_FromRot(&joints[1 + sLegs[side][1]], tmp);
        BossGanon_Pg_Mul(thigh, tmp, shin, false);
        BossGanon_Player_ClipDir(clip, a, b, w, sLegs[side][3] + 1, dir);
        BossGanon_Player_SwingBoneToward(shin, dir, weight);

        BossGanon_Pg_Mul(pelvis, thigh, local, true);
        BossGanon_Pg_ToRot(local, &joints[1 + sLegs[side][0]]);
        BossGanon_Pg_Mul(thigh, shin, local, true);
        BossGanon_Pg_ToRot(local, &joints[1 + sLegs[side][1]]);
    }

    for (side = 0; side < 2; side++) {
        GdfMat3 shoulder;
        GdfMat3 upper;
        GdfMat3 fore;

        if (side == sGdfClipKeepArm) {
            continue;
        }

        BossGanon_Pg_FromRot(&joints[1 + sArms[side][0]], tmp);
        BossGanon_Pg_Mul(chest, tmp, shoulder, false);
        BossGanon_Pg_FromRot(&joints[1 + sArms[side][1]], tmp);
        BossGanon_Pg_Mul(shoulder, tmp, upper, false);
        BossGanon_Player_ClipDir(clip, a, b, w, sArms[side][3], dir);
        BossGanon_Player_SwingBoneToward(upper, dir, weight);
        BossGanon_Pg_FromRot(&joints[1 + sArms[side][2]], tmp);
        BossGanon_Pg_Mul(upper, tmp, fore, false);
        BossGanon_Player_ClipDir(clip, a, b, w, sArms[side][3] + 1, dir);
        BossGanon_Player_SwingBoneToward(fore, dir, weight);
        // Its roll: the back of his hand faces the way the source's does. (Turned the shortest way to its direction
        // alone, the forearm kept whatever roll the pose under the clip had, and walking his palms came round to the
        // front.) A row without one (his right hand holding the trident) keeps the grip's roll.
        if ((clip[a][11 + side].x != 0) || (clip[a][11 + side].y != 0) || (clip[a][11 + side].z != 0)) {
            BossGanon_Player_ClipDir(clip, a, b, w, 11 + side, dir);
            BossGanon_Player_RollBoneToward(fore, sGdfHandBack[side], dir, weight);
        }

        BossGanon_Pg_Mul(shoulder, upper, local, true);
        BossGanon_Pg_ToRot(local, &joints[1 + sArms[side][1]]);
        BossGanon_Pg_Mul(upper, fore, local, true);
        BossGanon_Pg_ToRot(local, &joints[1 + sArms[side][2]]);
    }
}

// The attack's frame to the clip's, through the keys

// The trident's first hit: the slash, wound up from his right and cut across to his left (striking at frame 6)
static const GdfClipKey sGdfTridentSlash1[] = { { 0, 0 }, { 3, 10 }, { 6, 20 }, { 10, 30 }, { 18, 45 } };
// The second: the slash backwards, a rising backhand from his left up to his right (striking at frame 3)
static const GdfClipKey sGdfTridentSlash2[] = { { 0, 30 }, { 1, 27 }, { 3, 20 }, { 7, 12 }, { 15, 0 } };
// The spin: a full turn on the run (turning from frame 3 to 16)
// The third: the stab, drawn back and thrust out at frame 3, held to frame 11 (then blended back to his stance)
static const GdfClipKey sGdfTridentStab[] = { { 0, 6 }, { 2, 11 }, { 3, 15 }, { 11, 27 }, { 18, 31 } };
static const GdfClipKey sGdfTridentSpinClip[] = { { 0, 0 }, { 3, 2 }, { 16, 18 }, { 24, 39 } };

// With the trident in hand: his combo's slashes and his spin follow the clips (blending in and out of them)

// Walking with the trident: the plain walk (its legs, torso and free left arm; his right arm keeps the trident as his
// stance holds it), giving way to the clip's run as he speeds up, both in step with his walk (the walk's right heel
// comes down at its frame 31, the run's at 17)

// The dagger in his right fist: its hilt (his fist closed round the middle of it) and the curved blade going on from
// the hilt's line, the hilt's line laid along his grip as the sword's is. (Knife space to the sword frame: x stays
// across, the blade's way out of the hilt becomes the sword's +z.)
#define GDF_DAGGER_ROLL_DEG 0.0f // turned about the blade (0: its curve's plane across his fist)

// The opening in Link's house: instead of lying in Link's bed, Ganondorf leans against the wall beside it, arms crossed
// and head bowed, brooding, for as long as the scene's cutscene runs. The arms were posed offline with IK on his
// skeleton (each forearm across his chest, the fist tucked by the other arm's biceps).
#define GDF_BROOD_LEAN_DEG 9.0f // leaning back against the wall
#define GDF_BROOD_NOD_DEG 20.0f // head bowed
#define GDF_BROOD_SPOT_X 74.0f  // beside the bed (the bed is against the back wall, x -35 to 35)
#define GDF_BROOD_SPOT_Z 104.0f

// clang-format off
static const Vec3s sGdfBroodArms[8] = {
    { -7405, -16897, -9782 }, { 216, -3305, -14623 },  { 877, -5668, -15566 }, { -914, -765, -2736 }, // left 2-5
    { 8017, 21399, -8564 },   { 30700, 25088, 19765 }, { 589, 4570, -15445 },  { 2258, -798, -1675 }, // right 6-9
};
// clang-format on

void BossGanon_Player_Brood(BossGanon* this, PlayState* play);

static s32 BossGanon_Player_ShouldBrood(PlayState* play) {
    // Play as Ganondorf (2S2H): not ported yet
    return 0;
}

static void BossGanon_Player_SetupBrood(BossGanon* this, PlayState* play) {
    // Play as Ganondorf (2S2H): not ported yet
}

void BossGanon_Player_Brood(BossGanon* this, PlayState* play) {
    // Play as Ganondorf (2S2H): not ported yet
}

// Where he stands, facing into the room, his feet on the floor
static void BossGanon_Player_PlaceBrood(BossGanon* this, PlayState* play) {
    // Play as Ganondorf (2S2H): not ported yet
}

// Blade (or spear) positions for the hitbox and the trail

// From the hand matrix (captured in the limb callback) to the sword's own frame

static void BossGanon_Player_DrawSword(BossGanon* this, PlayState* play) {
    // Play as Ganondorf (2S2H): not ported yet
}

// The blade's hitbox: the area it swept since the last frame
static void BossGanon_Player_UpdateSwordHit(BossGanon* this, PlayState* play) {
    // Play as Ganondorf (2S2H): not ported yet
}

void BossGanon_Player_Init(BossGanon* this, PlayState* play) {
    Actor* thisx = &this->actor;
    Player* player = GET_PLAYER(play);
    s16 i;

    for (i = 0; i < ARRAY_COUNT(sPlayerEffects); i++) {
        sPlayerEffects[i].type = GDF_EFF_NONE;
    }
    sCurEffects = sPlayerEffects;

    sGanondorf = this;

    thisx->gravity = 0.0f;
    // Not an enemy: no lock-on, no Navi, no battle music
    thisx->flags &= ~(ACTOR_FLAG_ATTENTION_ENABLED | ACTOR_FLAG_HOSTILE);
    thisx->flags |= ACTOR_FLAG_UPDATE_DURING_OCARINA;
    thisx->update = BossGanon_Player_Update;
    ActorShape_Init(&thisx->shape, 0, NULL, 0);
    Actor_SetScale(thisx, 0.01f);
    SkelAnime_InitFlex(play, &this->skelAnime, &gGanondorfSkel, NULL, NULL, NULL, 0);
    Collider_InitCylinder(play, &this->collider);
    Collider_SetCylinder(play, &this->collider, thisx, &sPlayerPoundCylinderInit);

    // Follow Link through every room, and keep updating while he talks or gets items
    thisx->room = -1;
    thisx->parent = &player->actor;

    // Its own vertex buffers, so it can be drawn together with the real Ganondorf's cape
    sCape = (EnGanonMant*)Actor_SpawnAsChild(&play->actorCtx, thisx, play, ACTOR_EN_GANON_MANT, 0.0f, 0.0f, 0.0f, 0, 0,
                                             0, PLAYER_GANONDORF_CAPE_PARAMS);
    if (sCape == NULL) {
        Actor_Kill(thisx);
        return;
    }
    sCape->actor.room = -1;
    sPlayerCapeDraw = sCape->actor.draw;
    // The capelet's three pieces (the first draws the other two, while the Cloak option is on)
    for (s32 k = 0; k < 3; k++) {
        sGdfCapelet[k] = (EnGanonMant*)Actor_SpawnAsChild(&play->actorCtx, thisx, play, ACTOR_EN_GANON_MANT, 0.0f, 0.0f,
                                                          0.0f, 0, 0, 0, PLAYER_GANONDORF_CAPELET_PARAMS + k);
        if (sGdfCapelet[k] != NULL) {
            sGdfCapelet[k]->actor.room = -1;
            sGdfCapelet[k]->isLinked = (k > 0);
        }
    }
    if (sGdfCapelet[0] != NULL) {
        sGdfCapeletDraw = sGdfCapelet[0]->actor.draw;
        sGdfCapelet[0]->actor.draw = NULL;
    }
    // His cloak's other two pieces (sewn onto the cape, which draws them, while the Cloak option is on)
    for (s32 k = 0; k < GANON_MANT_MAX_LINKED; k++) {
        sGdfCloak[k] = (EnGanonMant*)Actor_SpawnAsChild(
            &play->actorCtx, thisx, play, ACTOR_EN_GANON_MANT, 0.0f, 0.0f, 0.0f, 0, 0, 0,
            (k == 0) ? PLAYER_GANONDORF_CLOAK_PARAMS : PLAYER_GANONDORF_CLOAK2_PARAMS);
        if (sGdfCloak[k] != NULL) {
            sGdfCloak[k]->actor.room = -1;
            sGdfCloak[k]->isLinked = true;
        }
    }

    // Ganondorf goes in front of his cape in the update and draw order, like in the vanilla init.
    // MISC also isn't affected by things that freeze or look for bosses: his cape goes there too (first, so it stays
    // behind him in the list), or it stopped moving whenever the game freezes bosses (chests, some conversations).
    Actor_ChangeCategory(play, &play->actorCtx, &sCape->actor, ACTORCAT_MISC);
    Actor_ChangeCategory(play, &play->actorCtx, thisx, ACTORCAT_MISC);

    this->playerFloorY = player->actor.floorHeight;
    Collider_InitQuad(play, &this->playerSwordQuad);
    Collider_SetQuad(play, &this->playerSwordQuad, thisx, &sPlayerSwordQuadInit);
    sPlayerSwordState = GDF_SWORD_OFF; // Play as Ganondorf (2S2H): his sword and trident aren't ported yet
    this->playerPrevInvincibilityTimer = player->invincibilityTimer;
    this->playerOrganSink = GDF_ORGAN_SINK_DEPTH;
    this->triforceType = GDF_TRIFORCE_DORF;
    BossGanon_Player_ManhuntSceneChange(play);
    // A new area: standing on his feet at once (his ground offset started at 0, his idle's root far above that, and
    // he rose out of the floor over the first frames: sunk to his shoulders by the doorway as he came in)
    this->playerGrounded = CVarGetInteger(CVAR_PLAY_AS_GDF("WalkOnGround"), 1) && !PlayAsGanondorf_IsLevitating();
    this->playerGroundOffset = sGdfStandOffsetValid ? sGdfStandOffset : 45.0f;
    sGdfFastGroundTimer = 16; // (then from his pose, at once)
    BossGanon_Player_SnapToLink(this, play);
    BossGanon_Player_UpdateAim(this, play);
    this->playerAimYaw = this->actor.yawTowardsPlayer;
    BossGanon_Player_SetupFloat(this, play);
}

void BossGanon_PlayerProjectile_Init(BossGanon* this, PlayState* play) {
    Actor* thisx = &this->actor;
    s16 i;

    thisx->flags &= ~(ACTOR_FLAG_ATTENTION_ENABLED | ACTOR_FLAG_HOSTILE);
    thisx->room = -1;
    this->fwork[GDF_FWORK_1] = 255.0f;
    Collider_InitCylinder(play, &this->collider);

    if (thisx->params == GDF_PARAMS_PLAYER_LIGHT_BALL) {
        thisx->update = BossGanon_PlayerLightBall_Update;
        thisx->draw = BossGanon_LightBall_Draw;
        thisx->speed = 14.0f;
        Actor_SetScale(thisx, 6.0f);
        this->timers[2] = 90; // lifetime
        Collider_SetCylinder(play, &this->collider, thisx, &sPlayerLightBallCylinderInit);
    } else {
        s16 index = thisx->params - GDF_PARAMS_PLAYER_BIG_MAGIC;

        thisx->update = BossGanon_PlayerBigMagic_Update;
        thisx->draw = func_808E324C;
        Actor_SetScale(thisx, 0.01f);
        this->unk_1C2 = 10;
        // Same phase offsets as the vanilla balls (params 0x104 - 0x108)
        this->unk_1A2 = 520 + (-(0x104 + index) * 2);

        for (i = 0; i < 15; i++) {
            this->unk_2EC[i] = thisx->world.pos;
            this->unk_3C4[i].x = BINANG_TO_RAD(thisx->world.rot.x);
            this->unk_3C4[i].y = BINANG_TO_RAD(thisx->world.rot.y);
        }

        this->timers[1] = 3;
        this->timers[2] = 75; // lifetime
        Collider_SetCylinder(play, &this->collider, thisx, &sPlayerBigMagicCylinderInit);
    }
}

// Phantom Ganon always has his spear out; turning the skin off puts it away again
static u8 sGdfPgSpearForced = false;

void BossGanon_Player_SetupFloat(BossGanon* this, PlayState* play) {
    if (sPlayerSwordState == GDF_SWORD_OFF) {
        sGdfDagger = false;
    }
    if (BossGanon_Player_IsPhantom()) {
        sPlayerSwordState = GDF_SWORD_DRAWN;
        sGdfPgSpearForced = true;
    } else if (sGdfPgSpearForced) {
        sPlayerSwordState = GDF_SWORD_OFF;
        sGdfPgSpearForced = false;
    }

    // With the sword out, his idle is the sword stance
    if (sPlayerSwordState != GDF_SWORD_OFF) {
        BossGanon_Player_SetupSwordIdle(this, play);
        return;
    }
    if (this->playerGrounded) {
        BossGanon_SetAnimationObject(this, play, OBJECT_GANON_ANIME2);
        Animation_MorphToLoop(&this->skelAnime, &gGanondorfStandIdleAnim, -10.0f);
    } else {
        BossGanon_SetAnimationObject(this, play, OBJECT_GANON_ANIME1);
        Animation_MorphToLoop(&this->skelAnime, &gGanondorfFloatAnim, -10.0f);
    }
    this->playerStandAnim = this->playerGrounded; // which of the two is playing
    this->actionFunc = BossGanon_Player_Float;
    this->unk_1C2 = 0;
    this->playerBtnHeld = 0;
}

void BossGanon_Player_Float(BossGanon* this, PlayState* play) {
    Player* player = GET_PLAYER(play);

    // Landed, or started levitating: stand or float
    if (this->playerStandAnim != this->playerGrounded) {
        BossGanon_Player_SetupFloat(this, play);
    }

    if (this->playerGrounded) {
        // Walking and running his cape spreads wide behind him as it does while he levitates: half way at a slow walk,
        // all the way at a run (as he gets going and settles, with his walk)
        f32 spread = this->playerWalkWeight * CLAMP(0.5f + sGdfWalkSpeed / 12.0f, 0.0f, 1.0f);

        sCape->backPush = -2.0f - 1.0f * spread;
        sCape->backSwayMagnitude = 0.25f;
        sCape->sideSwayMagnitude = -1.0f - 2.0f * spread;
        sCape->minDist = 20.0f * spread;

        SkelAnime_Update(&this->skelAnime);
        BossGanon_Player_UpdateWalk(this, play);
        BossGanon_Player_ApplyJump(this, play);
        Math_ApproachZeroF(&this->handLightBallScale, 1.0f, 1.0f);
        if (GanondorfDance_TakeRequest()) {
            BossGanon_Player_SetupDance(this, play); // the menu's dance emote
            return;
        }
        BossGanon_Player_ReadButtons(this, play);
        return;
    }
    this->playerWalkWeight = 0.0f;

    this->legSwayEnabled = true;

    sCape->backPush = -3.0f;
    sCape->backSwayMagnitude = 0.25f;
    sCape->sideSwayMagnitude = -3.0f;
    sCape->minDist = 20.0f;

    SkelAnime_Update(&this->skelAnime);
    Math_ApproachZeroF(&this->handLightBallScale, 1.0f, 1.0f);

    // The swish of flying. Levitation moves Link itself and keeps his speedXZ at 0, so this goes by how far he
    // actually moved this frame (up and down too)
    if (Math_Vec3f_DistXYZ(&player->actor.world.pos, &player->actor.prevPos) > 1.0f) {
        Audio_PlaySfx_AtPos(&this->actor.projectedPos, NA_SE_EN_FANTOM_FLOAT - SFX_FLAG);
    }

    BossGanon_Player_ReadButtons(this, play);
}

// B's magic: a quick charge, and once the ball has left his hand B (or R, or anything else) cuts the follow-through
// short. Held, it still becomes Big Magic after 20 frames (the charge waits for it).
#define GDF_LIGHT_BALL_CHARGE 16
#define GDF_THROW_CANCEL_FRAME 14.0f // the ball leaves at 11

void BossGanon_Player_SetupChargeLightBall(BossGanon* this, PlayState* play) {
    // Invasion mode: his attacks cost magic
    if (!GdfInvasion_PayAttack(5)) {
        return;
    }
    BossGanon_SetAnimationObject(this, play, OBJECT_GANON_ANIME1);
    this->fwork[GDF_FWORK_1] = Animation_GetLastFrame(&gGanondorfChargeLightBallAnim);
    Animation_MorphToPlayOnce(&this->skelAnime, &gGanondorfChargeLightBallAnim, -3.0f);
    this->actionFunc = BossGanon_Player_ChargeLightBall;
    this->timers[0] = GDF_LIGHT_BALL_CHARGE; // (25 until 8 Nov)
    this->unk_1C2 = 0;
    BossGanon_Player_UpdateAim(this, play);
}

void BossGanon_Player_ChargeLightBall(BossGanon* this, PlayState* play) {
    SkelAnime_Update(&this->skelAnime);

    this->legSwayEnabled = true;
    sCape->backPush = -3.0f;
    sCape->backSwayMagnitude = 1.25f;
    sCape->sideSwayMagnitude = -2.0f;
    sCape->minDist = 10.0f;

    BossGanon_Player_UpdateAim(this, play);

    if (this->timers[0] == GDF_LIGHT_BALL_CHARGE - 4) {
        this->unk_26C = 10;
        this->unk_270 = Rand_ZeroFloat(M_PI);
        Audio_PlayActorSound2(&this->actor, BossGanon_Player_Voice(NA_SE_EN_GANON_SPARK));
    }

    if (this->timers[0] < 10) {
        this->unk_258 += (Rand_ZeroFloat(M_PI / 2) + (M_PI / 2));
        Math_ApproachF(&this->handLightBallScale, 10.0f, 0.5f, 1.25f);

        if (this->timers[0] == 0) {
            // Still holding the button: charge Big Magic instead (waiting, held, until it has been 20 frames)
            if ((this->playerBtnHeld != 0) && CVarGetInteger(CVAR_PLAY_AS_GDF("BigMagic"), 1)) {
                if (this->playerHeldTimer >= 20) {
                    this->handLightBallScale = 0.0f;
                    BossGanon_Player_SetupChargeBigMagic(this, play);
                }
            } else {
                BossGanon_Player_SetupThrow(this, play);
            }
        }
    }
}

void BossGanon_Player_SetupThrow(BossGanon* this, PlayState* play) {
    BossGanon_SetAnimationObject(this, play, OBJECT_GANON_ANIME1);
    this->fwork[GDF_FWORK_1] = Animation_GetLastFrame(&gGanondorfThrowAnim);
    Animation_MorphToPlayOnce(&this->skelAnime, &gGanondorfThrowAnim, 0.0f);
    this->actionFunc = BossGanon_Player_Throw;
    this->unk_1C2 = 0;
}

void BossGanon_Player_Throw(BossGanon* this, PlayState* play) {
    s16 rotX;
    s16 rotY;

    SkelAnime_Update(&this->skelAnime);
    Math_ApproachZeroF(&this->handLightBallScale, 1.0f, 0.2f);

    if (this->skelAnime.curFrame < 11.0f) {
        BossGanon_Player_UpdateAim(this, play);
    }

    if (this->skelAnime.curFrame <= 12.0f) {
        this->lensFlareTimer = 2;
        this->lensFlareMode = 2;
        gCustomLensFlare1Pos = this->unk_260;
    }

    if (Animation_OnFrame(&this->skelAnime, 12.0f)) {
        this->handLightBallScale = 0.0f;
    }

    if (Animation_OnFrame(&this->skelAnime, 11.0f)) {
        this->unk_25C = 1;
        Audio_PlayActorSound2(&this->actor, BossGanon_Player_Voice(NA_SE_EN_GANON_THROW));
        Audio_PlayActorSound2(&this->actor, BossGanon_Player_Voice(NA_SE_EN_GANON_THROW_MASIC));
        BossGanon_Player_AimRot(&this->unk_260, &this->playerAimPos, &rotX, &rotY);
        Actor_SpawnAsChild(&play->actorCtx, &this->actor, play, ACTOR_PLAYER_GDF, this->unk_260.x, this->unk_260.y,
                           this->unk_260.z, rotX, rotY, 0, GDF_PARAMS_PLAYER_LIGHT_BALL);
    }

    if (Animation_OnFrame(&this->skelAnime, this->fwork[GDF_FWORK_1])) {
        BossGanon_Player_SetupFloat(this, play);
        return;
    }
    // The ball is away: the next press (another ball, the cape, ...) cuts the follow-through short
    if (this->skelAnime.curFrame >= GDF_THROW_CANCEL_FRAME) {
        BossGanon_Player_ReadButtons(this, play);
    }
}

void BossGanon_Player_SetupChargeBigMagic(BossGanon* this, PlayState* play) {
    // Invasion mode: his attacks cost magic
    if (!GdfInvasion_PayAttack(15)) {
        return;
    }
    BossGanon_SetAnimationObject(this, play, OBJECT_GANON_ANIME1);
    this->unk_1C2 = 0;
    this->timers[0] = 8;
    this->unk_1AA = Rand_ZeroFloat(20000.0f);
    this->unk_1AC = 0;
    this->actionFunc = BossGanon_Player_ChargeBigMagic;
    BossGanon_Player_UpdateAim(this, play);
}

// Vanilla BossGanon_ChargeBigMagic without the arena movement, and with a shorter charge
void BossGanon_Player_ChargeBigMagic(BossGanon* this, PlayState* play) {
    Vec3f sp80;
    Vec3f sp74;
    Vec3f sp68;
    s16 i;

    SkelAnime_Update(&this->skelAnime);

    if (this->unk_1C2 < 3) {
        BossGanon_Player_UpdateAim(this, play);
    }

    switch (this->unk_1C2) {
        case 0:
            if (this->timers[0] == 0) {
                this->fwork[GDF_FWORK_1] = Animation_GetLastFrame(&gGanondorfBigMagicChargeStartAnim);
                Animation_MorphToPlayOnce(&this->skelAnime, &gGanondorfBigMagicChargeStartAnim, 0.0f);
                this->unk_1C2 = 1;
            }
            break;

        case 1:
            if (Animation_OnFrame(&this->skelAnime, this->fwork[GDF_FWORK_1])) {
                this->fwork[GDF_FWORK_1] = Animation_GetLastFrame(&gGanondorfBigMagicChargeHoldAnim);
                Animation_MorphToLoop(&this->skelAnime, &gGanondorfBigMagicChargeHoldAnim, 0.0f);
                this->unk_1C2 = 2;
                // Vanilla charges for 100 frames. The thresholds below are the vanilla ones, so starting lower
                // only skips the first part of the charge.
                this->timers[0] = 60;
            }
            break;

        case 2:
            Audio_PlayActorSound2(&this->actor, BossGanon_Player_Voice(NA_SE_EN_GANON_CHARGE_MASIC - SFX_FLAG));
            this->unk_278.x = this->unk_2EC[0].x;
            this->unk_278.y = this->unk_2EC[0].y + 50.0f + 30.0f;
            this->unk_278.z = this->unk_2EC[0].z;

            Math_ApproachF(&this->unk_284, 0.25f, 0.1f, 0.006f);
            Math_ApproachF(&this->unk_288, 255.0f, 1.0f, 255.0f);
            Math_ApproachF(&this->unk_28C, 0.25f, 0.1f, 0.006f);

            if ((this->timers[0] > 20) && (this->timers[0] < 60)) {
                Math_ApproachF(&this->unk_290, 255.0f, 1.0f, 15.0f);
            }

            if (this->timers[0] == 0) {
                this->fwork[GDF_FWORK_1] = Animation_GetLastFrame(&gGanondorfBigMagicWindupAnim);
                Animation_MorphToPlayOnce(&this->skelAnime, &gGanondorfBigMagicWindupAnim, 0.0f);
                this->unk_1C2 = 3;
                this->timers[0] = 6;
                this->timers[1] = 15;
                Audio_PlayActorSound2(&this->actor, BossGanon_Player_Voice(NA_SE_EN_GANON_DARKWAVE));
                break;
            }

            if ((this->timers[0] >= 7) && (this->timers[0] < 26)) {
                if (this->unk_1AC < ARRAY_COUNT(this->unk_294)) {
                    this->unk_1AC++;
                }

                for (i = 0; i < this->unk_1AC; i++) {
                    Math_ApproachF(&this->unk_294[i], 200.0f, 1.0f, 40.0f);
                }
            }

            if (this->timers[0] <= 30) {
                Math_ApproachF(&this->unk_284, 0.4f, 0.5f, 0.017f);
                this->unk_28C = this->unk_284;
                Math_ApproachF(&this->unk_2D0, 45.0f, 0.1f, 10.0f);
                this->lensFlareTimer = 1;
                this->lensFlareMode = 2;
                gCustomLensFlare1Pos = this->unk_278;
            }

            if ((this->timers[0] <= 47) && (this->timers[0] >= 42)) {
                this->unk_274 = 48 - this->timers[0];
            }

            if (this->timers[0] > 30) {
                sp74.x = 0.0f;
                sp74.y = Rand_ZeroFloat(10.0f) + 150.0f;
                sp74.z = 0.0f;

                Matrix_RotateY(BINANG_TO_RAD(this->actor.yawTowardsPlayer), MTXMODE_NEW);
                Matrix_RotateZ(Rand_ZeroFloat(65536.0f), MTXMODE_APPLY);
                Matrix_MultVec3f(&sp74, &sp68);

                sp80.x = this->unk_278.x + sp68.x;
                sp80.y = this->unk_278.y + sp68.y;
                sp80.z = this->unk_278.z + sp68.z;

                BossGanonEff_SpawnBlackDot(play, &sp80, 20.0f);
            }
            break;

        case 3:
            for (i = 0; i < ARRAY_COUNT(this->unk_294); i++) {
                Math_ApproachF(&this->unk_294[i], 0.0f, 1.0f, 40.0f);
            }

            if (this->timers[0] == 1) {
                sCape->attachLeftArmTimer = 15.0f;
                Audio_PlayActorSound2(&this->actor, NA_SE_EV_GANON_MANTLE);
            }

            if (this->timers[0] == 0) {
                Math_ApproachZeroF(&this->unk_284, 1.0f, 0.08f);
                this->unk_28C = this->unk_284;
                Math_ApproachZeroF(&this->unk_2D0, 1.0f, 10.0f);
                Math_ApproachF(&this->unk_278.x, this->unk_1FC.x, 0.5f, 30.0f);
                Math_ApproachF(&this->unk_278.y, this->unk_1FC.y, 0.5f, 30.0f);
                Math_ApproachF(&this->unk_278.z, this->unk_1FC.z, 0.5f, 30.0f);
            }

            if (this->timers[1] == 0) {
                this->fwork[GDF_FWORK_1] = Animation_GetLastFrame(&gGanondorfBigMagicThrowAnim);
                Animation_MorphToLoop(&this->skelAnime, &gGanondorfBigMagicThrowAnim, 0.0f);
                this->unk_1C2 = 4;
                this->unk_288 = 0.0f;
                this->unk_290 = 0.0f;
                this->unk_284 = 0.0f;
                this->unk_28C = 0.0f;
            }
            break;

        case 4:
            if (Animation_OnFrame(&this->skelAnime, 5.0f)) {
                for (i = 0; i < GDF_PLAYER_BIG_MAGIC_COUNT; i++) {
                    Actor_SpawnAsChild(&play->actorCtx, &this->actor, play, ACTOR_PLAYER_GDF, this->unk_1FC.x,
                                       this->unk_1FC.y, this->unk_1FC.z, 0, this->actor.yawTowardsPlayer, 0,
                                       GDF_PARAMS_PLAYER_BIG_MAGIC + i);
                }

                Audio_PlayActorSound2(&this->actor, BossGanon_Player_Voice(NA_SE_EN_GANON_BIGMASIC));
                Audio_PlayActorSound2(&this->actor, BossGanon_Player_Voice(NA_SE_EN_GANON_THROW_BIG));
            }

            if (Animation_OnFrame(&this->skelAnime, 3.0f)) {
                sCape->attachShouldersTimer = 26.0f;
                Audio_PlayActorSound2(&this->actor, NA_SE_EV_GANON_MANTLE);
            }

            if (Animation_OnFrame(&this->skelAnime, this->fwork[GDF_FWORK_1])) {
                this->fwork[GDF_FWORK_1] = Animation_GetLastFrame(&gGanondorfBigMagicThrowEndAnim);
                Animation_MorphToLoop(&this->skelAnime, &gGanondorfBigMagicThrowEndAnim, 0.0f);
                this->unk_1C2 = 5;
            }
            break;

        case 5:
            if (Animation_OnFrame(&this->skelAnime, this->fwork[GDF_FWORK_1])) {
                BossGanon_Player_SetupFloat(this, play);
            }
            break;
    }
}

void BossGanon_Player_SetupPound(BossGanon* this, PlayState* play) {
    // Invasion mode: his attacks cost magic
    if (!GdfInvasion_PayAttack(8)) {
        return;
    }
    BossGanon_SetAnimationObject(this, play, OBJECT_GANON_ANIME1);
    this->unk_1C2 = 0;
    this->timers[0] = 26;
    this->actionFunc = BossGanon_Player_Pound;
    this->actor.velocity.y = 0.0f;
    this->handLightBallScale = 0.0f;
    this->playerBtnHeld = 0;
}

// Vanilla BossGanon_PoundFloor, relative to the floor under Link
void BossGanon_Player_Pound(BossGanon* this, PlayState* play) {
    Player* player = GET_PLAYER(play);
    f32 floorY = this->playerFloorY;
    f32 hover = BossGanon_Player_GetHoverHeight();
    f32 heightTarget;
    // On the ground he hops up and slams down with his legs tucked as when he levitates (the pound's own floating
    // legs), his lowest knee or foot coming down to the floor (the ground offset follows his pose, at once here); the
    // floating heights are for levitating, where he rises above his hover height and slams down to just over the floor
    u8 grounded = this->playerGrounded;
    f32 baseY = grounded ? (player->actor.world.pos.y + this->playerGroundOffset) : (player->actor.world.pos.y + hover);
    f32 impactY = grounded ? baseY : (floorY + 60.0f);

    if (grounded) {
        sGdfFastGroundTimer = MAX(sGdfFastGroundTimer, 2);
    }
    s16 i;
    Vec3f sp6C;
    Vec3f sp60;
    Vec3f sp54;

    SkelAnime_Update(&this->skelAnime);

    switch (this->unk_1C2) {
        case 0:
            if (this->timers[0] == 5) {
                Audio_PlayActorSound2(&this->actor, BossGanon_Player_Voice(NA_SE_EN_GANON_HIT_GND));
            }

            if (this->timers[0] < 14) {
                heightTarget = baseY + 90.0f;
                this->unk_258 += (Rand_ZeroFloat(M_PI / 2) + (M_PI / 2));
                Math_ApproachF(&this->handLightBallScale, 7.0f, 0.5f, 1.0f);
            } else {
                heightTarget = baseY + 40.0f;
            }

            Math_ApproachF(&this->actor.world.pos.y, heightTarget, 0.1f, this->actor.velocity.y);
            Math_ApproachF(&this->actor.velocity.y, 20.0f, 1.0f, 1.0f);

            if (this->timers[0] == 14) {
                this->fwork[GDF_FWORK_1] = Animation_GetLastFrame(&gGanondorfPoundAnim);
                Animation_MorphToPlayOnce(&this->skelAnime, &gGanondorfPoundAnim, 0.0f);
                this->actor.velocity.y = 0.0f;
            }

            if (this->timers[0] == 0) {
                this->unk_1C2 = 1;
                this->actor.velocity.y = 0.0f;
            }
            break;

        case 1:
            sCape->gravity = -1.0f;
            Math_ApproachF(&this->actor.velocity.y, -50.0f, 1.0f, 10.0f);
            this->actor.world.pos.y += this->actor.velocity.y;

            if (this->actor.world.pos.y < impactY) {
                this->actor.world.pos.y = impactY;
                this->unk_1C2 = 2;
                this->timers[0] = 10;
                Actor_RequestQuakeAndRumble(&this->actor, play, 0xA, 0x14);
                this->unk_19C = 35;
                Audio_PlayActorSound2(&this->actor, BossGanon_Player_Voice(NA_SE_EN_GANON_HIT_GND_IMP));
                this->handLightBallScale = 0.0f;
                sp60 = this->unk_260;
                sp60.y = floorY;

                for (i = 0; i < 80; i++) {
                    sp6C.x = Rand_CenteredFloat(25.0f);
                    sp6C.y = Rand_ZeroFloat(17.0f);
                    sp6C.z = Rand_CenteredFloat(25.0f);
                    BossGanonEff_SpawnLightRay(play, &sp60, &sp6C, &sZeroVec, Rand_ZeroFloat(300.0f) + 500.0f, 13.0f,
                                               0x1E);
                }
            }
            break;

        case 2:
            if (this->timers[0] == 0) {
                this->fwork[GDF_FWORK_1] = Animation_GetLastFrame(&gGanondorfPoundEndAnim);
                Animation_MorphToPlayOnce(&this->skelAnime, &gGanondorfPoundEndAnim, 0.0f);
                this->unk_1C2 = 3;
                this->unk_19F = 1;
                this->actor.velocity.y = 0.0f;
            }
            break;

        case 3:
        case 4:
            if (grounded) {
                this->actor.world.pos.y = baseY; // he landed on his feet
            } else {
                Math_ApproachF(&this->actor.world.pos.y, baseY, 0.1f, this->actor.velocity.y);
            }
            Math_ApproachF(&this->actor.velocity.y, 20.0f, 1.0f, 1.0f);

            if (Animation_OnFrame(&this->skelAnime, this->fwork[GDF_FWORK_1])) {
                if (this->unk_1C2 == 3) {
                    this->fwork[GDF_FWORK_1] = Animation_GetLastFrame(&gGanondorfGetUp3Anim);
                    Animation_MorphToPlayOnce(&this->skelAnime, &gGanondorfGetUp3Anim, 0.0f);
                    SkelAnime_Update(&this->skelAnime);
                    sCape->attachShouldersTimer = 18.0f;
                    Audio_PlayActorSound2(&this->actor, NA_SE_EV_GANON_MANTLE);
                    this->unk_1C2 = 4;
                } else {
                    BossGanon_Player_SetupFloat(this, play);
                    if (grounded) {
                        sGdfFastGroundTimer = 16; // (onto his feet through the blend into his idle)
                    }
                }
            }
            break;
    }

    if ((this->unk_19C == 35) || (this->unk_19C == 30) || (this->unk_19C == 25)) {
        sp54 = this->actor.world.pos;
        sp54.y = floorY;
        BossGanonEff_SpawnDustLight(play, &sp54, 0, 3.0f, this->unk_19C - 25);
    }

    if (this->unk_19C == 35) {
        sp54 = this->actor.world.pos;
        sp54.y = floorY;
        BossGanonEff_SpawnShockwave(play, &sp54, 0, 3.0f);
    }

    // The shockwave hurts enemies around Ganondorf in two pulses as it spreads
    if ((this->unk_19C == 35) || (this->unk_19C == 27)) {
        this->collider.dim.radius = (this->unk_19C == 35) ? 130 : 280;
        this->collider.dim.height = 80;
        this->collider.dim.yShift = 0;
        this->collider.dim.pos.x = this->actor.world.pos.x;
        this->collider.dim.pos.y = floorY - 20.0f;
        this->collider.dim.pos.z = this->actor.world.pos.z;
        BossGanon_Player_SetAT(this, play);
    }
}

// The cape swats the real Ganondorf's light balls and Big Magic back at him (the tennis from his fight)
static void BossGanon_Player_DeflectMagic(BossGanon* this, PlayState* play) {
    // Play as Ganondorf (2S2H): not ported yet
}

// The cape is up (the start of the block, before it's released)

// While the cape is up it works as Link's shield, for everything that checks for one: a large hard shield
// hitbox in front of Ganondorf (projectiles and attacks bounce off it) and the shield's position and angle
// (where reflected nuts, rocks, Twinrova's magic and mirror light go). Called right after Link's update, because
// Link's own update resets his shield collider.

// R's cape guard: up at least 6 frames (10 until 8 Nov), and 4 frames into lowering it R raises it again (or B
// throws, ...) instead of waiting out the whole 20 frame release
#define GDF_BLOCK_MIN_FRAMES 6
#define GDF_BLOCK_RELEASE_CANCEL_FRAME 4.0f

void BossGanon_Player_SetupBlock(BossGanon* this, PlayState* play) {
    Player* player = GET_PLAYER(play);

    BossGanon_SetAnimationObject(this, play, OBJECT_GANON_ANIME1);
    this->fwork[GDF_FWORK_1] = Animation_GetLastFrame(&gGanondorfBlockAnim);
    Animation_MorphToPlayOnce(&this->skelAnime, &gGanondorfBlockAnim, 0.0f);
    this->actionFunc = BossGanon_Player_Block;
    this->unk_1C2 = 0;
    sCape->attachLeftArmTimer = this->timers[0] = GDF_BLOCK_MIN_FRAMES;
    Audio_PlayActorSound2(&this->actor, NA_SE_EV_GANON_MANTLE);
    this->handLightBallScale = 0.0f;
    this->playerShieldPitch = 0;

    // Turn to face the closest enemy (a Deku Scrub, say) so the cape sends its shots straight back
    BossGanon_Player_UpdateAim(this, play);
    player->actor.shape.rot.y = player->yaw = player->actor.world.rot.y = this->actor.yawTowardsPlayer;
}

void BossGanon_Player_Block(BossGanon* this, PlayState* play) {
    Player* player = GET_PLAYER(play);
    Input* input = &play->state.input[0];

    SkelAnime_Update(&this->skelAnime);
    sCape->backPush = -9.0f;
    sCape->backSwayMagnitude = 0.25f;
    sCape->sideSwayMagnitude = -2.0f;
    sCape->minDist = 13.0f;

    if (this->unk_1C2 == 0) {
        BossGanon_Player_DeflectMagic(this, play);

        // Ganondorf's cape guard: Link can't be hurt or knocked back while it's up (intangibility)
        if ((player->invincibilityTimer >= 0) && (player->invincibilityTimer < 2)) {
            player->invincibilityTimer = 2;
        }

        if (CHECK_BTN_ALL(input->cur.button, BTN_R) && BossGanon_Player_CanStartAttack(this, play)) {
            // Aim the cape like Link aims his shield: stick left/right turns, up/down tilts
            s16 targetPitch = (s16)(input->rel.stick_y * 200);

            player->actor.shape.rot.y -= (s16)(input->rel.stick_x * 40);
            player->yaw = player->actor.world.rot.y = player->actor.shape.rot.y;
            this->actor.yawTowardsPlayer = player->actor.shape.rot.y;
            Math_ScaledStepToS(&this->playerShieldPitch, targetPitch, 0x800);

            if (this->timers[0] < 2) {
                this->timers[0] = 2;
            }
            sCape->attachLeftArmTimer = 10.0f;

            if (CHECK_BTN_ALL(input->press.button, BTN_B)) {
                BossGanon_Player_SetupPound(this, play);
                return;
            }
        }

        if (this->timers[0] == 0) {
            this->unk_1C2 = 1;
            Animation_MorphToPlayOnce(&this->skelAnime, &gGanondorfBlockReleaseAnim, 0.0f);
            this->fwork[GDF_FWORK_1] = Animation_GetLastFrame(&gGanondorfBlockReleaseAnim);
            SkelAnime_Update(&this->skelAnime);
            sCape->attachShouldersTimer = 15.0f;
            Audio_PlayActorSound2(&this->actor, NA_SE_EV_GANON_MANTLE);
            Audio_PlayActorSound2(&this->actor, BossGanon_Player_Voice(NA_SE_EN_GANON_AT_RETURN)); // "Hah!"
        }
    } else {
        sCape->sideSwayMagnitude = -13.0f;

        if (Animation_OnFrame(&this->skelAnime, this->fwork[GDF_FWORK_1])) {
            BossGanon_Player_SetupFloat(this, play);
        } else if (this->skelAnime.curFrame >= GDF_BLOCK_RELEASE_CANCEL_FRAME) {
            BossGanon_Player_ReadButtons(this, play);
        }
    }
}

static void BossGanon_Player_CancelMagic(BossGanon* this) {
    this->handLightBallScale = 0.0f;
    this->unk_284 = 0.0f;
    this->unk_288 = 0.0f;
    this->unk_28C = 0.0f;
    this->unk_290 = 0.0f;
    this->unk_2D0 = 0.0f;
    this->unk_1AC = 0;
    this->unk_274 = 0;
    this->unk_26C = 0;
    this->lensFlareTimer = 0;
    this->playerBtnHeld = 0;
    sCape->attachRightArmTimer = sCape->attachLeftArmTimer = 0.0f;
}

void BossGanon_Player_SetupHurt(BossGanon* this, PlayState* play) {
    s16 i;

    BossGanon_SetAnimationObject(this, play, OBJECT_GANON_ANIME1);
    BossGanon_Player_CancelMagic(this);
    this->fwork[GDF_FWORK_1] = Animation_GetLastFrame(&gGanondorfDamageAnim);
    Animation_MorphToPlayOnce(&this->skelAnime, &gGanondorfDamageAnim, 0.0f);
    this->actionFunc = BossGanon_Player_Hurt;
    this->unk_1A6 = 15; // red flash

    // A short burst of the yellow shock he gets when hit by his own light ball
    for (i = 1; i < 15; i++) {
        this->unk_4E4[i] = D_808E4C58[i];
    }
    this->unk_2E6 = 30;
    this->unk_2E8 = 0;
    this->unk_508 = 3.0f;

    Audio_PlayActorSound2(&this->actor, BossGanon_Player_Voice(NA_SE_EN_GANON_DAMAGE1));
}

void BossGanon_Player_Hurt(BossGanon* this, PlayState* play) {
    SkelAnime_Update(&this->skelAnime);

    if (Animation_OnFrame(&this->skelAnime, this->fwork[GDF_FWORK_1])) {
        BossGanon_Player_SetupFloat(this, play);
    }
}

// ---- Kneeling for young Link's cutscenes ----
// The camera is framed for a child there, and standing he was cut off at the chest. He goes down on one knee (his
// defeat's kneel) and turns his head to whoever is talking; he rises when it's over.
static void BossGanon_Pg_RotY(f32 deg, GdfMat3 out);
static u8 sGdfCsPoseCur; // (his cutscene pose, set further down)
static f32 sGdfKneelYaw = 0.0f;
static f32 sGdfKneelPitch = 0.0f;
static s16 sGdfKneelStill = 0; // frames Link has stood still (counted in his update)
#define GDF_KNEEL_STILL_FRAMES 10

// Boss rooms: their cutscenes (the boss's entrance, its defeat, the blue warp) are framed for whoever is fighting, not
// for a child at a conversation, so he stays on his feet

// The villain Link's Light Arrow in the drawbridge escape (z_en_viewer.c): struck, he drops to his knees for a while
static s16 sGdfCsStagger = 0;

static s32 BossGanon_Player_ShouldKneel(BossGanon* this, PlayState* play) {
    // Play as Ganondorf (2S2H): not ported yet
    return 0;
}

// His head turned by (yaw, pitch) degrees in his body's frame (as the cutscene poses turn it)

void BossGanon_Player_CsKneel(BossGanon* this, PlayState* play);

static void BossGanon_Player_SetupCsKneel(BossGanon* this, PlayState* play) {
    // Play as Ganondorf (2S2H): not ported yet
}

void BossGanon_Player_CsKneel(BossGanon* this, PlayState* play) {
    // Play as Ganondorf (2S2H): not ported yet
}

void BossGanon_Player_SetupDowned(BossGanon* this, PlayState* play) {
    // Play as Ganondorf (2S2H): not ported yet
}

static s32 BossGanon_Player_IsLinkDead(PlayState* play) {
    Player* player = GET_PLAYER(play);

    return (gSaveContext.save.saveInfo.playerData.health <= 0) || (player->stateFlags1 & PLAYER_STATE1_DEAD);
}

// Falls to the floor when Link dies, gets back up if a fairy revives him
void BossGanon_Player_Downed(BossGanon* this, PlayState* play) {
    // Play as Ganondorf (2S2H): not ported yet
}

static void BossGanon_Player_Update_Impl(Actor* thisx, PlayState* play) {
    BossGanon* this = (BossGanon*)thisx;
    Player* player = GET_PLAYER(play);
    Input* input = &play->state.input[0];
    f32 legRotX;
    f32 legRotY;
    f32 legRotZ;
    f32 sin;
    f32 cos;
    f32 targetLensFlareScale;
    f32 xOffset;
    f32 zOffset;
    Vec3f spE8;
    s16 i;
    u8 controlY;

    if (sCape == NULL) {
        Actor_Kill(thisx);
        return;
    }

    BossGanon_Player_SlayNpcs(this, play);
    BossGanon_Player_UpdateManhunt(this, play);

    // The engine sets yawTowardsPlayer to the direction of Link every frame, but Link stands right under him, so that
    // direction is meaningless and he'd turn to it mid-attack. Restore the aim from the last update.
    this->actor.yawTowardsPlayer = this->playerAimYaw;

    sCape->gravity = -3.0f;
    this->shockGlow = false;
    this->unk_1A2++;
    this->unk_1A4++;

    // The floor under Link stands in for the arena floor
    if (player->actor.floorHeight > (BGCHECK_Y_MIN + 1.0f)) {
        this->playerFloorY = player->actor.floorHeight;
    } else {
        this->playerFloorY = player->actor.world.pos.y;
    }
    // Walking about on his own in a cutscene: the floor under him instead
    BossGanon_Player_UpdateRoam(this, play);
    if (sGdfRoaming) {
        this->playerFloorY = sGdfRoamFloorY;
    }
    // The cape's hem rests on the floor under Link. Raising that floor gradually keeps the hem from jittering on
    // slopes and stairs; lowering it is instant, so the cape never gets pushed up when he drops off a ledge.
    {
        f32 capeFloor = this->playerFloorY + 2.0f;

        if ((capeFloor < sCape->minY) || (capeFloor > sCape->minY + 60.0f)) {
            sCape->minY = capeFloor;
        } else {
            Math_ApproachF(&sCape->minY, capeFloor, 0.3f, 4.0f);
        }
    }

    // On the ground he stands and walks; he only floats while levitating
    this->playerGrounded = CVarGetInteger(CVAR_PLAY_AS_GDF("WalkOnGround"), 1) && !PlayAsGanondorf_IsLevitating();

    // Jumps: remember the highest point, and land heavily after a big drop
    if (this->playerGrounded && BossGanon_Player_IsJumping(play)) {
        if (!this->playerWasAirborne) {
            this->playerWasAirborne = true;
            this->playerJumpTopY = player->actor.world.pos.y;
        }
        this->playerJumpTopY = MAX(this->playerJumpTopY, player->actor.world.pos.y);
    } else if (this->playerWasAirborne) {
        f32 drop = this->playerJumpTopY - player->actor.world.pos.y;

        this->playerWasAirborne = false;
        if (this->playerGrounded && (player->actor.bgCheckFlags & BGCHECKFLAG_GROUND) && !this->playerHidden &&
            (drop > GDF_BIG_LANDING_HEIGHT) &&
            ((this->actionFunc == BossGanon_Player_Float) || (this->actionFunc == BossGanon_Player_SwordIdle))) {
            BossGanon_Player_SetupLand(this, play, drop);
        }
    }

    // Stepping or pulling himself up onto a ledge
    if ((player->stateFlags1 & PLAYER_STATE1_CLIMBING_LEDGE) &&
        !(player->stateFlags1 & PLAYER_STATE1_CLIMBING_LADDER) && (this->actionFunc != BossGanon_Player_LedgeClimb) &&
        (this->actionFunc != BossGanon_Player_Downed) && (this->actionFunc != BossGanon_Player_Ride) &&
        this->playerGrounded && !this->playerHidden) {
        BossGanon_Player_SetupLedgeClimb(this, play);
    }

    // Climbing a ladder or a vine wall (Play as Ganondorf (2S2H): not ported yet, he follows in his own pose)
    if (false && (player->stateFlags1 & PLAYER_STATE1_CLIMBING_LADDER) &&
        (this->actionFunc != BossGanon_Player_Climb) && (this->actionFunc != BossGanon_Player_Downed) &&
        !this->playerHidden) {
        BossGanon_Player_ReleaseOrganCam(this, play);
        BossGanon_SetAnimationObject(this, play, OBJECT_GANON_ANIME2);
        Animation_MorphToLoop(&this->skelAnime, &gGanondorfStandIdleAnim, -5.0f);
        this->actionFunc = BossGanon_Player_Climb;
        this->playerWalkWeight = 0.0f;
        this->playerStandLegsWeight = 0.0f;
    }

    // Holding up something he found (a chest, a heart piece, ...), like Link does (2S2H: not ported yet)
    if (false &&
        (((player->stateFlags1 & PLAYER_STATE1_GETTING_ITEM) && (player->unk_862 != 0)) || (sGdfHoldUpTimer > 0)) &&
        (this->actionFunc != BossGanon_Player_ItemGet) && (this->actionFunc != BossGanon_Player_Downed) &&
        (this->actionFunc != BossGanon_Player_Ride) && !this->playerHidden) {
        BossGanon_Player_ReleaseOrganCam(this, play);
        BossGanon_SetAnimationObject(this, play, OBJECT_GANON_ANIME2);
        Animation_MorphToLoop(&this->skelAnime, &gGanondorfStandIdleAnim, -5.0f);
        this->actionFunc = BossGanon_Player_ItemGet;
        this->playerItemWeight = 0.0f;
        Audio_PlayActorSound2(&this->actor, BossGanon_Player_Voice(NA_SE_EN_GANON_LAUGH));
    }

    // On his horse (or the Gerudo intro's) (2S2H: not ported yet)
    if (false && ((player->stateFlags1 & PLAYER_STATE1_ON_HORSE) || false) &&
        (this->actionFunc != BossGanon_Player_Ride) && (this->actionFunc != BossGanon_Player_Downed)) {
        Vec3f seat;

        if (BossGanon_Player_GetSeat(play, &seat)) {
            BossGanon_Player_ReleaseOrganCam(this, play);
            BossGanon_SetAnimationObject(this, play, OBJECT_GANON_ANIME2);
            Animation_MorphToLoop(&this->skelAnime, &gGanondorfStandIdleAnim, -5.0f);
            this->actionFunc = BossGanon_Player_Ride;
            this->playerRideFrame = 0.0f;
            this->playerWalkWeight = 0.0f;
            this->playerStandLegsWeight = 0.0f;
        }
    }

    // The organ is his ocarina: when Link plays it, Ganondorf plays the organ
    if ((player->stateFlags2 & PLAYER_STATE2_OCARINA_PLAYING) && PlayAsGanondorf_UseOrganOcarina() &&
        (this->actionFunc != BossGanon_Player_Organ) && (this->actionFunc != BossGanon_Player_Downed)) {
        BossGanon_Player_StopOrganMusic(this, play);
        BossGanon_Player_SetupOrgan(this, play);
        this->playerOrganOcarina = true;
        // Straight into playing: the first notes come right away
        BossGanon_SetAnimationObject(this, play, OBJECT_GANON_ANIME2);
        Animation_MorphToLoop(&this->skelAnime, &gGanondorfPlayOrganAnim, -5.0f);
    } else if (this->actionFunc != BossGanon_Player_Organ) {
        this->playerOrganOcarina = false;
    }

    this->playerHidden = BossGanon_Player_ShouldHide(this, play) || GdfInvasion_IsVanished();
    if (sCape->actor.update != NULL) {
        sCape->actor.draw = this->playerHidden ? NULL : sPlayerCapeDraw;
    }
    if (this->playerHidden && (sCape != NULL)) {
        sCape->linkedCount = 0; // (sewn on again from his draw)
        if (sGdfCapelet[0] != NULL) {
            sGdfCapelet[0]->actor.draw = NULL;
        }
    }

    // Track how long the attack button is held (to charge Big Magic)
    if (this->playerBtnHeld != 0) {
        if (CHECK_BTN_ALL(input->cur.button, this->playerBtnHeld)) {
            this->playerHeldTimer++;
        } else {
            this->playerBtnHeld = 0;
        }
    }

    if (sGdfGrabState != GDF_GRAB_NONE) {
        sGdfGrabBusyFrame = play->gameplayFrames;
    }
    if (CHECK_BTN_ALL(play->state.input[0].press.button, BTN_B)) {
        sGdfLastBFrame = play->gameplayFrames; // (the volley)
    }
    // Invasion mode: pacified
    if (GdfInvasion_IsPacified()) {
        if (this->actionFunc != BossGanon_Player_Pacified) {
            BossGanon_Player_SetupPacified(this, play);
        }
    } else if (BossGanon_Player_IsLinkDead(play)) {
        if (this->actionFunc != BossGanon_Player_Downed) {
            BossGanon_Player_SetupDowned(this, play);
        }
    } else if ((player->invincibilityTimer > 0) && (this->playerPrevInvincibilityTimer <= 0) &&
               (this->actionFunc != BossGanon_Player_Downed)) {
        BossGanon_Player_SetupHurt(this, play);
    }

    this->actionFunc(this, play);
    BossGanon_Player_UpdateGrabbed(this, play);

    {
        // The dark aura handles his legs itself (it walks)
        u8 standLegs =
            (this->actionFunc != BossGanon_Player_DarkAura) && (this->actionFunc != BossGanon_Player_Climb) &&
            this->playerGrounded && !this->playerHidden &&
            ((this->actionFunc == BossGanon_Player_Block) || (this->actionFunc == BossGanon_Player_ChargeLightBall) ||
             (this->actionFunc == BossGanon_Player_Throw) || (this->actionFunc == BossGanon_Player_ChargeBigMagic) ||
             (this->actionFunc == BossGanon_Player_Laugh) || (this->actionFunc == BossGanon_Player_Hurt));
        // (the ground pound keeps its own floating legs, tucked as when he levitates, on the ground too)

        // In fast (the morph into the action already smooths it); out over ~13 frames, longer than the 10-frame
        // morph back to his standing idle, so the floating root height never shows
        Math_ApproachF(&this->playerStandLegsWeight, standLegs ? 1.0f : 0.0f, 1.0f, standLegs ? 0.5f : 0.08f);
        if ((this->actionFunc == BossGanon_Player_DarkAura) || (this->actionFunc == BossGanon_Player_Climb)) {
            this->playerStandLegsWeight = 0.0f;
        } else if (this->playerGrounded) {
            BossGanon_Player_StandLegs(this);
        } else {
            this->playerStandLegsWeight = 0.0f;
        }
    }

    // The Gerudo intro's direction: his arm pose, head and hand
    BossGanon_Player_ApplyIntroCs(this, play);

    // His taunt (D-pad Up) works in cutscenes and conversations too, where his other moves don't
    if (CHECK_BTN_ALL(play->state.input[0].press.button, BTN_DUP) && !this->playerHidden &&
        !GanondorfSongs_IsOcarinaOut() &&
        ((player->stateFlags1 & (PLAYER_STATE1_IN_CUTSCENE | PLAYER_STATE1_TALKING)) ||
         (play->csCtx.state != CS_STATE_IDLE)) &&
        ((this->actionFunc == BossGanon_Player_Float) || (this->actionFunc == BossGanon_Player_SwordIdle) ||
         (this->actionFunc == BossGanon_Player_Brood))) {
        BossGanon_Player_SetupLaugh(this, play);
    }
    // So does the dance: D-Pad Down, or the menu's button (on the ground)
    if (!this->playerHidden && !GanondorfSongs_IsOcarinaOut() && this->playerGrounded &&
        ((player->stateFlags1 & (PLAYER_STATE1_IN_CUTSCENE | PLAYER_STATE1_TALKING)) ||
         (play->csCtx.state != CS_STATE_IDLE)) &&
        ((this->actionFunc == BossGanon_Player_Float) || (this->actionFunc == BossGanon_Player_Brood) ||
         (this->actionFunc == BossGanon_Player_Laugh) || (this->actionFunc == BossGanon_Player_CsKneel)) &&
        (CHECK_BTN_ALL(play->state.input[0].press.button, BTN_DDOWN) || GanondorfDance_TakeRequest())) {
        BossGanon_Player_SetupDance(this, play);
    }
    // Young Link's conversations and cutscenes: down on one knee, so the child's camera sees all of him
    sGdfKneelStill = (Math_Vec3f_DistXZ(&player->actor.world.pos, &player->actor.prevPos) < 0.5f)
                         ? MIN(sGdfKneelStill + 1, 1000)
                         : 0;
    if (sGdfCsStagger > 0) {
        sGdfCsStagger--;
        if ((this->actionFunc != BossGanon_Player_CsKneel) && !this->playerHidden && this->playerGrounded) {
            BossGanon_Player_SetupCsKneel(this, play); // struck: down at once
        }
    }
    if ((this->actionFunc == BossGanon_Player_Float) && BossGanon_Player_ShouldKneel(this, play)) {
        BossGanon_Player_SetupCsKneel(this, play);
    }

    // The opening in Link's house: brooding against the wall while the cutscene runs (a taunt can interrupt it)
    if (BossGanon_Player_ShouldBrood(play) && !this->playerHidden && !sGdfRoaming) {
        if ((this->actionFunc != BossGanon_Player_Brood) && (this->actionFunc != BossGanon_Player_Laugh) &&
            (this->actionFunc != BossGanon_Player_Dance)) {
            BossGanon_Player_SetupBrood(this, play);
            BossGanon_Player_Brood(this, play);
        }
    } else if (this->actionFunc == BossGanon_Player_Brood) {
        BossGanon_Player_SetupFloat(this, play);
    }

    // Anything that interrupts the throw (getting hurt, cutscenes, ...) puts the trident back in his hand
    if ((sGdfTridentState != GDF_TRIDENT_IN_HAND) && (this->actionFunc != BossGanon_Player_TridentSpin) &&
        (this->actionFunc != BossGanon_Player_TridentThrow)) {
        sGdfTridentState = GDF_TRIDENT_IN_HAND;
    }

    // He kicks doors open
    BossGanon_Player_ApplyDoorKick(this, play);
    BossGanon_Player_ApplyChestKick(this, play);
    BossGanon_Player_MasterSwordKick(this, play);
    BossGanon_Player_UpdateBlink();
    BossGanon_Player_ApplyBreakKick(this, play);
    BossGanon_Player_ApplyDoorDuck(this, play);
    BossGanon_Player_UpdateBodyguards(this, play);

    // Into and out of a crawlspace: a puff of dark smoke as he turns into the flame and back
    {
        u8 flame = PlayAsGanondorf_IsCrawlFlame(play, player);

        if (flame != this->playerCrawlFlame) {
            Vec3f zero = { 0.0f, 0.0f, 0.0f };
            Vec3f pos = player->actor.world.pos;

            pos.y += 20.0f;
            {
                Color_RGBA8 prim = { 200, 120, 255, 255 };
                Color_RGBA8 env = { 60, 0, 120, 255 };

                EffectSsDeadDb_Spawn(play, &pos, &zero, &zero, &prim, &env, 150, 0, 9);
            }
            Audio_PlayActorSound2(&this->actor, BossGanon_Player_Voice(NA_SE_EN_GANON_SPARK));
            this->playerCrawlFlame = flame;
        }
    }

    // Link isn't drawn, so his body part positions (used by enemies for grabs, effects and aiming) would go stale.
    // Use Ganondorf's from his last draw instead.
    if (!this->playerHidden) {
        // Ganondorf's body parts (unk_2EC): 0 chest (torso + 30), 1 torso, 2 pelvis, 3-5 one arm, 6-8 the other,
        // 9-11 one leg, 12-14 the other. His head is captured on its own (sGdfPlayerHeadPos).
        static const u8 sLinkToDorfBodyPart[] = {
            2,          // waist: his pelvis
            12, 13, 14, // right leg
            9,  10, 11, // left leg
            0,  0,      // head, hat (replaced by his head below)
            0,          // collar
            3,  4,  5,  // left arm
            6,  7,  8,  // right arm
            2,  1,      // sheath, torso
        };

        for (i = 0; (i < ARRAY_COUNT(sLinkToDorfBodyPart)) && (i < ARRAY_COUNT(player->bodyPartsPos)); i++) {
            player->bodyPartsPos[i] = this->unk_2EC[sLinkToDorfBodyPart[i]];
        }
        // His head, not his hips: Navi/the Poe hovers by the hat, enemies aim at the focus, NPCs look at it
        if (sGdfPlayerHeadValid) {
            player->bodyPartsPos[PLAYER_BODYPART_HEAD] = sGdfPlayerHeadPos;
            player->bodyPartsPos[PLAYER_BODYPART_HAT] = sGdfPlayerHeadPos;
            player->bodyPartsPos[PLAYER_BODYPART_HAT].y += 15.0f;
            player->actor.focus.pos = sGdfPlayerHeadPos;
        }
    }

    // After the action, so the block's own intangibility doesn't count as getting hurt
    this->playerPrevInvincibilityTimer = player->invincibilityTimer;

    if (this->playerGrounded && (this->actionFunc != BossGanon_Player_Climb) &&
        (this->actionFunc != BossGanon_Player_LedgeClimb)) {
        BossGanon_Player_UpdateGroundOffset(this, play);
    }
    BossGanon_Player_UpdateSwordHit(this, play);

    controlY = (this->actionFunc != BossGanon_Player_Pound) && (this->actionFunc != BossGanon_Player_Downed) &&
               (this->actionFunc != BossGanon_Player_Organ);

    // The organ sinks back into the ground once he stops playing, and the area music comes back
    if (this->actionFunc != BossGanon_Player_Organ) {
        Math_ApproachZeroF(&this->actor.shape.yOffset, 0.3f, 700.0f);
        BossGanon_Player_StopOrganMusic(this, play);
        BossGanon_Player_ReleaseOrganCam(this, play);
        if (this->playerOrganSink < GDF_ORGAN_SINK_DEPTH) {
            // Silently (no more grinding sound, it kept looping)
            Math_ApproachF(&this->playerOrganSink, GDF_ORGAN_SINK_DEPTH, 1.0f, 25.0f);
        }
    }

    // Triforce of Power on his hand, from the laugh
    if ((this->actionFunc != BossGanon_Player_Laugh) && (this->actionFunc != BossGanon_Player_DarkAura)) {
        Math_ApproachZeroF(&this->fwork[GDF_TRIFORCE_PRIM_A], 1.0f, 25.0f);
    }
    // The aura fades once he lets go
    if (this->actionFunc != BossGanon_Player_DarkAura) {
        Math_ApproachZeroF(&this->playerAuraAlpha, 1.0f, 20.0f);
        Math_ApproachZeroF(&this->playerAuraScale, 1.0f, 0.02f);
    }
    if (sGdfRoaming) {
        // Where he walked to, standing on the floor there (the ground offset is relative to Link's height)
        this->actor.world.pos.y = player->actor.world.pos.y + this->playerGroundOffset;
    } else {
        BossGanon_Player_Follow(this, play, controlY);
    }

    if (sGdfRoaming) {
        // Walking about on his own in a cutscene: he faces where he walks, all the way round. (Turned toward Link's
        // facing first, as below, and then toward his own way, the two pulls met halfway: he could only face within
        // a half circle around Link's facing.)
        Math_ApproachS(&this->actor.shape.rot.y, sGdfRoamYaw, 3, 0x1800);
        this->actor.yawTowardsPlayer = this->actor.shape.rot.y;
    } else if ((this->actionFunc == BossGanon_Player_SwordAttack) && (this->playerSwordAttack == GDF_SWORD_ATK_SPIN) &&
               (this->playerSpinYaw != 0)) {
        this->actor.shape.rot.y = this->playerDashYaw + this->playerSpinYaw;
    } else if (BossGanon_Player_IsBusy(thisx)) {
        Math_ApproachS(&this->actor.shape.rot.y, this->actor.yawTowardsPlayer, 3, 0x1000);
    } else if (this->actionFunc != BossGanon_Player_Downed) {
        s16 linkYaw = player->actor.shape.rot.y;

        // In cutscenes Link's animation can turn him itself (its root's yaw: the opening's drawbridge, say), so he's
        // drawn facing his body's yaw plus that turn: Ganondorf faces the way Link is drawn
        if ((player->stateFlags1 & PLAYER_STATE1_IN_CUTSCENE) || (play->csCtx.state != CS_STATE_IDLE)) {
            linkYaw += player->skelAnime.jointTable[1].y;
        }
        Math_ApproachS(&this->actor.shape.rot.y, linkYaw, 2, 0x2000);
        // Between attacks his aim is wherever Link faces, so an attack that doesn't aim itself goes straight ahead
        this->actor.yawTowardsPlayer = linkYaw;
    }
    this->actor.world.rot.y = this->actor.shape.rot.y;
    this->playerAimYaw = this->actor.yawTowardsPlayer;
    if ((this->actionFunc == BossGanon_Player_Brood) ||
        ((this->actionFunc == BossGanon_Player_Laugh) && BossGanon_Player_ShouldBrood(play) && !sGdfRoaming)) {
        BossGanon_Player_PlaceBrood(this, play);
    }

    // Used for the leg sway
    this->actor.velocity.x = this->actor.world.pos.x - this->actor.prevPos.x;
    this->actor.velocity.z = this->actor.world.pos.z - this->actor.prevPos.z;
    if (controlY) {
        this->actor.velocity.y = this->actor.world.pos.y - this->actor.prevPos.y;
    }

    for (i = 0; i < ARRAY_COUNT(this->timers); i++) {
        if (this->timers[i] != 0) {
            this->timers[i]--;
        }
    }

    if (this->unk_1A6 != 0) {
        this->unk_1A6--;
    }

    if (this->unk_2D4 != 0) {
        this->unk_2D4--;
    }

    if (this->unk_2E8 != 0) {
        this->unk_2E8--;
    }

    if (this->unk_2E6 != 0) {
        this->unk_2E6--;
    }

    if (this->unk_19C != 0) {
        this->unk_19C--;
    }

    // Leg sway, from BossGanon_Update. Only while floating: on the ground it bent his standing legs into the floor
    if (this->legSwayEnabled && !this->playerGrounded) {
        f32 velX = CLAMP(this->actor.velocity.x, -6.0f, 6.0f);
        f32 velZ = CLAMP(this->actor.velocity.z, -6.0f, 6.0f);

        sin = Math_SinS(-this->actor.shape.rot.y);
        cos = Math_CosS(-this->actor.shape.rot.y);

        legRotX = ((velZ * sin) + (cos * velX)) * 300.0f;
        legRotY = ((-sin * velX) + (cos * velZ)) * 300.0f;
        legRotZ = (Math_SinS(this->unk_1A2 * 2268) * -500.0f) - 500.0f;
    } else {
        legRotY = legRotX = legRotZ = 0.0f;
    }

    this->legSwayEnabled = false;

    Math_SmoothStepToF(&this->legRot.x, legRotX, 1.0f, 600.0f, 0.0f);
    Math_SmoothStepToF(&this->legRot.y, legRotY, 1.0f, 600.0f, 0.0f);
    Math_SmoothStepToF(&this->legRot.z, legRotZ, 1.0f, 100.0f, 0.0f);

    // Electric shock on Ganondorf's body
    if (this->unk_2E6 != 0) {
        for (i = 1; i < ARRAY_COUNT(this->unk_49C); i++) {
            if (this->unk_4E4[i] != 0) {
                this->unk_4E4[i]--;
                Math_ApproachF(&this->unk_49C[i], this->unk_508, 1.0f, 2.0f);
            } else {
                Math_ApproachZeroF(&this->unk_49C[i], 1.0f, 0.2f);
            }
        }
    }

    // Dust when landing
    if (this->unk_19F != 0) {
        this->unk_19F = 0;
        spE8 = this->actor.world.pos;
        spE8.y = this->playerFloorY;
        BossGanonEff_SpawnDustDark(play, &spE8, 0.2, 0.7f);
        BossGanonEff_SpawnDustDark(play, &spE8, 0.3f, 0.8f);
    }

    // Lightning while charging a light ball
    if (this->unk_26C != 0) {
        this->unk_26C--;

        if (this->unk_26C == 0) {
            BossGanonEff_SpawnLightning(play, 1.0f, 0.0f, 0.0f);
        }

        BossGanonEff_SpawnLightning(play, 1.0f, D_808E4D44[this->unk_26C] * (M_PI / 5) + this->unk_270,
                                    Rand_CenteredFloat(M_PI / 5) + (M_PI / 2));
    }

    BossGanon_UpdateEffects(play);

    // Lens flare, from BossGanon_Update
    if (this->lensFlareTimer != 0) {
        this->lensFlareTimer--;

        if (this->lensFlareMode == 1) {
            targetLensFlareScale = 40.0f;
        } else if (this->lensFlareMode == 4) {
            targetLensFlareScale = 25.0f;
        } else {
            targetLensFlareScale = 10.0f;
        }

        Math_ApproachF(&this->lensFlareScale, targetLensFlareScale, 0.3f, 10.0f);
    } else {
        Math_ApproachZeroF(&this->lensFlareScale, 1.0f, 5.0f);

        if (this->lensFlareScale == 0.0f) {
            this->lensFlareMode = 0;
        }
    }

    if ((this->lensFlareMode != 0) && !this->playerHidden) {
        gCustomLensFlare1On = true;
        sPlayerLensFlareOn = true;

        if (this->lensFlareMode == 1) {
            gCustomLensFlare1Pos = this->actor.world.pos;
        }

        D_801F4E44 = this->lensFlareScale;
        D_801F4E48 = 10.0f;
        D_801F4E4C = 0;
    } else if (sPlayerLensFlareOn) {
        gCustomLensFlare1On = false;
        sPlayerLensFlareOn = false;
    }

    // Light balls that fly into the Big Magic charge
    if (this->unk_274 != 0) {
        i = this->unk_274 - 1;

        this->unk_278.x = this->unk_2EC[0].x;
        this->unk_278.y = this->unk_2EC[0].y + 50.0f + 30.0f;
        this->unk_278.z = this->unk_2EC[0].z;

        xOffset = (sinf(i * 1.2566371f) * 600.0f);
        zOffset = (cosf(i * 1.2566371f) * 600.0f);

        Actor_SpawnAsChild(&play->actorCtx, &this->actor, play, ACTOR_PLAYER_GDF, this->unk_1FC.x + xOffset,
                           this->unk_1FC.y, this->unk_1FC.z + zOffset, 0, (s16)(i * 13107.2f) + 0x6000, 0, 0xFA + i);
        this->unk_274 = 0;
    }
}

// Ganondorf's light ball, thrown by the player. Flies straight and bursts on enemies, walls and floors.

// Points the ball at a spot (NULL: the nearest Link) at its speed
static void BossGanon_PlayerBall_Aim(BossGanon* this, PlayState* play, Vec3f* at) {
    Vec3f target;
    f32 dx;
    f32 dy;
    f32 dz;

    if (at == NULL) {
        Actor* best = NULL;
        f32 bestDist = 3000.0f;
        Actor* actor;

        for (actor = play->actorCtx.actorLists[ACTORCAT_NPC].first; actor != NULL; actor = actor->next) {
            f32 d;
            if ((actor->update == NULL) || (GdfInvasion_DummyClientId(actor) == 0)) {
                continue;
            }
            d = Math_Vec3f_DistXYZ(&this->actor.world.pos, &actor->world.pos);
            if (d < bestDist) {
                bestDist = d;
                best = actor;
            }
        }
        if (best == NULL) {
            return;
        }
        target = best->world.pos;
        target.y += 40.0f;
    } else {
        target = *at;
    }
    dx = target.x - this->actor.world.pos.x;
    dy = target.y - this->actor.world.pos.y;
    dz = target.z - this->actor.world.pos.z;
    this->actor.world.rot.y = Math_Atan2S(dz, dx);
    this->actor.world.rot.x = RADF_TO_BINANG(atan2f(dy, sqrtf(SQ(dx) + SQ(dz))));
}

static s32 BossGanon_PlayerBall_NearLink(BossGanon* this, PlayState* play) {
    Actor* actor;

    if (!GdfInvasion_CanHurtLinks()) {
        return false;
    }
    for (actor = play->actorCtx.actorLists[ACTORCAT_NPC].first; actor != NULL; actor = actor->next) {
        if ((actor->update != NULL) && (GdfInvasion_DummyClientId(actor) != 0) &&
            (Math_Vec3f_DistXYZ(&this->actor.world.pos, &actor->world.pos) < 180.0f)) {
            return true;
        }
    }
    return false;
}

// lightOnly: 1 his light balls, 0 those and his Big Magic

// A Link sent it back: at Ganondorf it goes

// A Link missed it (his game has hurt him): it bursts

static void BossGanon_PlayerLightBall_Update_Impl(Actor* thisx, PlayState* play) {
    BossGanon* this = (BossGanon*)thisx;
    u8 burst = false;
    u8 hitSomething = false;
    s16 i;
    Vec3f vel;
    Vec3f accel;
    Vec3f pos;

    this->unk_1A2++;

    if (this->unk_1A8 != 0) {
        // Burst, from BossGanon_LightBall_Update
        this->actor.shape.rot.y += 0x1000;
        Math_ApproachZeroF(&this->fwork[GDF_FWORK_1], 1.0f, 30.0f);
        Math_ApproachF(&this->actor.scale.x, 20.0f, 0.5f, 100.0f);
        this->fwork[GDF_FWORK_0] += ((M_PI / 2) + Rand_ZeroFloat(M_PI / 4));
        Actor_SetScale(&this->actor, this->actor.scale.x);

        if (this->fwork[GDF_FWORK_1] == 0.0f) {
            Actor_Kill(&this->actor);
        }
        return;
    }

    Audio_PlayActorSound2(&this->actor, BossGanon_Player_Voice(NA_SE_EN_GANON_FIRE - SFX_FLAG));
    Actor_SetScale(&this->actor, ((this->unk_1A2 % 2) != 0) ? 6.0f : 5.25f);
    if (this->actor.home.rot.z == 1) {
        BossGanon_FireballFlames(play, &this->actor.world.pos, this->unk_1A2); // the fireball ring's
    }
    this->actor.shape.rot.z += (s16)(Rand_ZeroOne() * 20000.0f) + 0x4000;

    for (i = 0; i < ARRAY_COUNT(this->timers); i++) {
        if (this->timers[i] != 0) {
            this->timers[i]--;
        }
    }

    // The fireball ring: circling him while he spins the trident
    if (this->unk_1A6 == 2) {
        BossGanon* dorf = (BossGanon*)this->actor.parent;

        if ((dorf == NULL) || (dorf->actor.update == NULL) || (dorf->actionFunc != BossGanon_Player_TridentSpin)) {
            this->unk_1A6 = 0; // he stopped: it bursts
            this->timers[2] = 0;
        } else {
            s16 angle = (s16)(this->unk_1AC * (0x10000 / GDF_FIRE_RING_COUNT)) + sGdfTridentSpin / 2;

            this->actor.world.pos.x = dorf->actor.world.pos.x + Math_SinS(angle) * 120.0f;
            this->actor.world.pos.y =
                dorf->actor.world.pos.y + 80.0f + Math_SinS(angle * 2 + this->unk_1A2 * 0x400) * 10.0f;
            this->actor.world.pos.z = dorf->actor.world.pos.z + Math_CosS(angle) * 120.0f;
            this->timers[2] = 90;
            Collider_UpdateCylinder(&this->actor, &this->collider);
            if (!BossGanon_PlayerBall_NearLink(this, play)) {
                BossGanon_Player_SetAT(this, play);
            }
            return;
        }
    }

    Actor_UpdateVelocityWithoutGravity(&this->actor);
    Actor_UpdatePos(&this->actor);
    Actor_UpdateBgCheckInfo(play, &this->actor, 10.0f, 12.0f, 12.0f, 1 | 2 | 4);

    if (this->collider.base.atFlags & AT_HIT) {
        this->collider.base.atFlags &= ~AT_HIT;
        hitSomething = true;
        burst = true;
    }
    if (this->actor.bgCheckFlags & (BGCHECKFLAG_GROUND | BGCHECKFLAG_WALL | BGCHECKFLAG_CEILING)) {
        burst = true;
    }
    if (this->timers[2] == 0) {
        burst = true;
    }

    // Invasion mode, the volley: sent back by a Link, the ball flies at Ganondorf; B just as it reaches him sends it
    // back at a Link (a little faster each time), missing gets him hit
    if ((this->unk_1A6 != 0) && (sPlayerGanondorf != NULL) && (sPlayerGanondorf->actor.update != NULL)) {
        Vec3f him = sPlayerGanondorf->actor.world.pos;
        f32 dist;

        him.y += 70.0f;
        dist = Math_Vec3f_DistXYZ(&this->actor.world.pos, &him);
        if ((dist < 110.0f) && ((u32)(play->gameplayFrames - sGdfLastBFrame) < 8)) {
            BossGanon_PlayerBall_Aim(this, play, NULL);
            this->unk_1A6 = 0;
            this->unk_1A4++;
            this->actor.speed = MIN(this->actor.speed * 1.1f, 40.0f);
            this->timers[2] = 90;
            Audio_PlayActorSound2(&sPlayerGanondorf->actor, NA_SE_IT_SWORD_REFLECT_MG);
        } else if (dist < 50.0f) {
            GdfInvasion_BallHitGanondorf();
            burst = true;
            hitSomething = true;
        } else {
            BossGanon_PlayerBall_Aim(this, play, &him);
            this->timers[2] = MAX(this->timers[2], 30);
        }
    }

    // Also on the frame it bursts against a wall or floor: eye switches and the like sit right on the surface,
    // and the wall check can stop the ball before its hitbox reaches them. (In Invasion mode, near a Link it doesn't
    // hit here: his own game decides whether he sent it back)
    if (!hitSomething && (this->unk_1A6 == 0) && !BossGanon_PlayerBall_NearLink(this, play)) {
        Collider_UpdateCylinder(&this->actor, &this->collider);
        BossGanon_Player_SetAT(this, play);
    }

    if (!burst) {

        for (i = 0; i < 2; i++) {
            accel.x = accel.z = 0.0f;
            accel.y = 0.2f;
            vel.x = vel.y = vel.z = 0.0f;
            pos.x = Rand_CenteredFloat(30.0f) + this->actor.world.pos.x;
            pos.y = Rand_CenteredFloat(30.0f) + this->actor.world.pos.y;
            pos.z = Rand_CenteredFloat(30.0f) + this->actor.world.pos.z;
            BossGanonEff_SpawnSparkle(play, &pos, &vel, &accel, Rand_ZeroFloat(500.0f) + 700.0f, 0x1E);
        }
    } else {
        SoundSource_PlaySfxAtFixedWorldPos(play, &this->actor.world.pos, 80, NA_SE_EN_GANON_HIT_THUNDER);

        for (i = 0; i < 70; i++) {
            vel.x = Rand_CenteredFloat(30.0f);
            vel.y = Rand_CenteredFloat(30.0f);
            vel.z = Rand_CenteredFloat(30.0f);
            BossGanonEff_SpawnLightRay(play, &this->actor.world.pos, &vel, &sZeroVec, Rand_ZeroFloat(200.0f) + 500.0f,
                                       15.0f, 0x1E);
        }

        this->actor.speed = 0.0f;
        this->unk_1A8 = 1;
    }
}

// Ganondorf's Big Magic, thrown by the player. Five balls that spread out and home in on enemies.
static void BossGanon_PlayerBigMagic_Update_Impl(Actor* thisx, PlayState* play) {
    BossGanon* this = (BossGanon*)thisx;
    BossGanon* dorf = (BossGanon*)this->actor.parent;
    u8 numEffects = 0;
    s16 index = this->actor.params - GDF_PARAMS_PLAYER_BIG_MAGIC;
    s16 i;
    s16 yawTarget;
    s16 pitchTarget;
    f32 dist;
    f32 wobble;
    Actor* target;
    Vec3f targetPos;
    Vec3f sp60;

    this->unk_1A2++;
    Actor_SetScale(&this->actor, 0.01f);

    for (i = 0; i < ARRAY_COUNT(this->timers); i++) {
        if (this->timers[i] != 0) {
            this->timers[i]--;
        }
    }

    Actor_UpdateVelocityWithoutGravity(&this->actor);
    Actor_UpdatePos(&this->actor);

    this->unk_1A6++;

    if (this->unk_1A6 >= 15) {
        this->unk_1A6 = 0;
    }

    this->unk_2EC[this->unk_1A6] = this->actor.world.pos;
    this->unk_3C4[this->unk_1A6].x = BINANG_TO_RAD(this->actor.world.rot.x);
    this->unk_3C4[this->unk_1A6].y = BINANG_TO_RAD(this->actor.world.rot.y);

    switch (this->unk_1C2) {
        case 1:
            Math_ApproachZeroF(&this->fwork[GDF_FWORK_1], 1.0f, 40.0f);

            if (this->fwork[GDF_FWORK_1] == 0.0f) {
                Actor_Kill(&this->actor);
            }
            break;

        case 10:
            this->unk_1C2 = 11;
            this->timers[0] = 14;
            this->actor.speed = 20.0f;
            this->fwork[GDF_FWORK_1] = 255.0f;
            this->csCamMaxStepScale = 0.0f;
            this->unk_1F0 = (dorf != NULL) ? dorf->playerAimPos : this->actor.world.pos;
            // Fan out like the vanilla balls
            this->actor.shape.rot.y = this->actor.world.rot.y + (index * 0x2000) - 0x4000;
            this->actor.shape.rot.x = 0;
            // fallthrough
        case 11:
            // Spread out for a moment, then home in
            if (this->timers[0] < 8) {
                target = BossGanon_Player_FindTarget(play, &this->actor.world.pos, this->actor.shape.rot.y, 600.0f,
                                                     0x6000, false);
                targetPos = (target != NULL) ? target->focus.pos : this->unk_1F0;

                yawTarget = Math_Vec3f_Yaw(&this->actor.world.pos, &targetPos);
                dist = sqrtf(SQ(targetPos.x - this->actor.world.pos.x) + SQ(targetPos.z - this->actor.world.pos.z));
                pitchTarget = Math_Atan2S(dist, targetPos.y - this->actor.world.pos.y);

                Math_ApproachS(&this->actor.shape.rot.y, yawTarget, 1, this->csCamMaxStepScale);
                Math_ApproachS(&this->actor.shape.rot.x, pitchTarget, 1, this->csCamMaxStepScale);
                Math_ApproachF(&this->csCamMaxStepScale, 2048.0f, 1.0f, 128.0f);

                wobble = CLAMP(dist * 10.0f, 0.0f, 6144.0f);
            } else {
                wobble = 6144.0f;
            }

            this->actor.world.rot.x = (Math_CosS(this->unk_1A2 * 0x3400) * wobble * 0.1f) + this->actor.shape.rot.x;
            this->actor.world.rot.y = (Math_SinS(this->unk_1A2 * 0x1A00) * wobble * 0.3f) + this->actor.shape.rot.y;

            Actor_UpdateBgCheckInfo(play, &this->actor, 10.0f, 10.0f, 10.0f, 1 | 2 | 4);

            if (this->collider.base.atFlags & AT_HIT) {
                this->collider.base.atFlags &= ~AT_HIT;
                numEffects = 40;
            } else if ((this->actor.bgCheckFlags & (BGCHECKFLAG_GROUND | BGCHECKFLAG_WALL | BGCHECKFLAG_CEILING)) ||
                       (this->timers[2] == 0)) {
                numEffects = 10;
            }

            if ((numEffects != 40) && !BossGanon_PlayerBall_NearLink(this, play)) {
                // Still hits what it touches on the frame it bursts against a wall (eye switches). (In Invasion mode,
                // near a Link his own game decides: only a spin attack stops it)
                Collider_UpdateCylinder(&this->actor, &this->collider);
                BossGanon_Player_SetAT(this, play);
            }
            if (numEffects != 0) {
                this->unk_1C2 = 1;
                this->actor.speed = 0.0f;
            }
            break;
    }

    if (numEffects) {
        SoundSource_PlaySfxAtFixedWorldPos(play, &this->actor.world.pos, 80, NA_SE_EN_FANTOM_THUNDER);

        for (i = 0; i < numEffects; i++) {
            sp60.x = Rand_CenteredFloat(30.0f);
            sp60.y = Rand_CenteredFloat(30.0f);
            sp60.z = Rand_CenteredFloat(30.0f);

            BossGanonEff_SpawnLightRay(play, &this->actor.world.pos, &sp60, &sZeroVec, Rand_ZeroFloat(200.0f) + 500.0f,
                                       15.0f, 0x1E);
        }
    }
}

// ------------------------------------------------------------------------------------------------
// SoH: "Play as Ganondorf", D-pad Up: laugh while showing the Triforce of Power (from his intro)
// ------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------
// SoH: "Play as Ganondorf", insta-kill dark aura (D-Pad Left mode)
//
// The purple vortex and dark shocks he hits Link with when Link enters his throne room, around himself. D-Pad Left
// turns the mode on; then hold B: the aura surrounds him (move with the stick, on the ground or in the air) and every
// enemy caught in it dies. Its health drops to 1 and the aura hits it like any of his attacks, so it dies its own
// way (death animation, drops); one the aura can't hurt is destroyed after a moment. Bosses only take the hits.
// ------------------------------------------------------------------------------------------------

#define GDF_AURA_MAX_SCALE 0.24f // the vortex model is 600 wide and 3000 tall: radius 600 * scale
#define GDF_AURA_HEIGHT_SCALE 0.45f
#define GDF_AURA_FORCE_KILL_FRAMES 25
#define GDF_AURA_MAX_TRACKED 32

typedef struct {
    Actor* actor;
    s16 frames;
} GdfAuraVictim;

static GdfAuraVictim sGdfAuraVictims[GDF_AURA_MAX_TRACKED];

// ---- The dark aura against bosses ----
// Some bosses can only be hurt or opened up with a weapon he doesn't have (hookshot for Morpha, arrows for Phantom
// Ganon in the paintings, boomerang for Barinade, the mirror shield for Twinrova). So no boss can leave him stuck,
// the dark aura stands in for whatever the boss needs. The boss's own code asks here, then fakes the hit with
// BossGanon_Player_AuraFakeHit so its damage logic runs as usual. Bosses get a wider reach than enemies: many fly or
// keep their distance.
static ColliderInfo sGdfAuraHitInfo;

// Marks the collider as hit by the aura, carrying dmgFlags (elem: the element that was hit, may be NULL)

// The raised-hand animations (taunt, dark aura) are standing ones: their root sits a standing height above the
// floating animation's, so levitating he'd rise by that much. In the air, keep the floating root (and its dangling
// legs).
static void BossGanon_Player_KeepFloatRoot(BossGanon* this) {
    Vec3s floatPose[GDF_LIMB_COUNT_PLAYER + 1];
    Vec3s* joints = this->skelAnime.jointTable;
    s32 i;

    if (this->playerGrounded) {
        return;
    }
    SkelAnime_GetFrameData((AnimationHeader*)gGanondorfFloatAnim,
                           (s32)(this->unk_1A2 % (s32)Animation_GetLastFrame(&gGanondorfFloatAnim)),
                           GDF_LIMB_COUNT_PLAYER + 1, floatPose);
    joints[0] = floatPose[0];
    for (i = 18; i < GDF_LIMB_COUNT_PLAYER; i++) {
        joints[1 + i] = floatPose[1 + i]; // pelvis and legs
    }
}

void BossGanon_Player_SetupDarkAura(BossGanon* this, PlayState* play) {
    // Play as Ganondorf (2S2H): not ported yet
}

// Is this actor still in the enemy list (a tracked one may have died and been freed since)

void BossGanon_Player_DarkAura(BossGanon* this, PlayState* play) {
    // Play as Ganondorf (2S2H): not ported yet
}

// The purple vortex from his throne room, standing around him
static void BossGanon_Player_DrawDarkAura(BossGanon* this, PlayState* play) {
    // Play as Ganondorf (2S2H): not ported yet
}

// Crawlspaces: a flickering purple flame where Link is (gameplay_keep's torch flame, tinted)
static void BossGanon_Player_DrawCrawlFlame(BossGanon* this, PlayState* play) {
    // Play as Ganondorf (2S2H): not ported yet
}

// ------------------------------------------------------------------------------------------------
// SoH: "Play as Ganondorf", riding his horse (Epona, drawn as his black horse unless that's turned off)
//
// Seated on the saddle: young Ganondorf's horseback ride cycle on his legs (it straddles the horse), his own upright
// torso, and both fists forward on the reins (solved offline with inverse kinematics on his skeleton).
// ------------------------------------------------------------------------------------------------

#define GDF_RIDE_CYCLE_FRAMES 24.0f

// clang-format off
static const Vec3s sGdfReinsPoses[2][3] = {
    { { -2360, 1441, -10464 }, { 937, -6137, -15592 }, { -1175, -1047, -4413 } }, // left arm
    { { 32515, 31667, 18489 }, { 149, 6346, -13011 }, { 2096, 1076, -3143 } },    // right arm
};
// clang-format on

static s32 BossGanon_Player_GetSeat(PlayState* play, Vec3f* seat) {
    // Play as Ganondorf (2S2H): not ported yet
    return 0;
}

void BossGanon_Player_Ride(BossGanon* this, PlayState* play) {
    // Play as Ganondorf (2S2H): not ported yet
}

// ------------------------------------------------------------------------------------------------
// SoH: "Play as Ganondorf", climbing ladders and vine walls, and holding up found items
// ------------------------------------------------------------------------------------------------

// Arm poses solved offline with inverse kinematics on his skeleton: fists on the ladder high above his head and at
// chest height, and both fists raised together over his head holding an item
// clang-format off
static const Vec3s sGdfClimbPoses[4][3] = {
    { { -6994, -3513, 7468 }, { 2852, -12664, -4315 }, { 4392, -9851, 1201 } },     // left arm, high
    { { -3521, 3968, -14824 }, { -2707, -7560, -24466 }, { -7238, -8633, -1420 } }, // left arm, low
    { { -28322, 19201, 21828 }, { 834, 7723, -14672 }, { 5723, -5859, -6160 } },    // right arm, high
    { { 32551, 25869, 21196 }, { -1118, 6437, -14874 }, { -1091, -2672, -4555 } },  // right arm, low
};
// clang-format on
// clang-format off
static const Vec3s sGdfItemPoses[2][3] = {
    { { 11691, 27520, -16011 }, { 3178, -1419, -16117 }, { 7921, -1511, -7597 } }, // left arm
    { { -28634, 15814, 19375 }, { 1338, 7492, -14082 }, { 8790, -4294, -8015 } },  // right arm
};
// clang-format on

void BossGanon_Player_Organ(BossGanon* this, PlayState* play) {
    // Play as Ganondorf (2S2H): not ported yet
}

// Play as Ganondorf (2S2H): not ported yet
static void BossGanon_Player_ReleaseOrganCam(BossGanon* this, PlayState* play) {
}
static void BossGanon_Player_ApplyDoorDuck(BossGanon* this, PlayState* play) {
}
static void BossGanon_Player_UpdateBodyguards(BossGanon* this, PlayState* play) {
}
static void BossGanon_Player_ApplyIntroCs(BossGanon* this, PlayState* play) {
}
static void BossGanon_Player_GrabFollowHand(BossGanon* this) {
}
static void BossGanon_Player_UpdateGrabbed(BossGanon* this, PlayState* play) {
}
static void BossGanon_Player_PlaceLedgeClimb(BossGanon* this, PlayState* play) {
}

// The fireball ring's balls burn: flames licking up and trailing off them
static void BossGanon_FireballFlames(PlayState* play, Vec3f* pos, s16 frame) {
    Vec3f vel = { 0.0f, 1.5f, 0.0f };
    Vec3f accel = { 0.0f, 0.4f, 0.0f };
    Color_RGBA8 prim = { 255, 220, 60, 255 };
    Color_RGBA8 env = { 255, 40, 0, 255 };
    Vec3f p;
    s32 i;

    for (i = 0; i < 2; i++) {
        p.x = pos->x + Rand_CenteredFloat(22.0f);
        p.y = pos->y + Rand_CenteredFloat(22.0f);
        p.z = pos->z + Rand_CenteredFloat(22.0f);
        EffectSsDeadDb_Spawn(play, &p, &vel, &accel, &prim, &env, 35 + (s16)Rand_ZeroFloat(25.0f), -2, 8);
    }
}

static void BossGanon_Player_SetupPacified(BossGanon* this, PlayState* play) {
}

static void BossGanon_Player_Pacified(BossGanon* this, PlayState* play) {
}

// ================================================================================================
// Play as Ganondorf (2S2H): entry points. There is no real Ganondorf in Majora's Mask, so his state is always the
// player-controlled one's.
// ================================================================================================

static void BossGanon_UsePlayerContext(void) {
    if (sPlayerGanondorf != NULL) {
        sGanondorf = sPlayerGanondorf;
    }
    sCape = sPlayerCape;
    sCurEffects = sPlayerEffects;
}

void BossGanon_Init(Actor* thisx, PlayState* play) {
    BossGanon* this = (BossGanon*)thisx;

    // His files are read by name from oot.o2r (PlayAsGanondorf.cpp mounts them): nothing to draw without them
    if (!PlayAsGanondorf_AssetsReady()) {
        Actor_Kill(thisx);
        return;
    }
    BossGanon_UsePlayerContext();
    if (thisx->params == GDF_PARAMS_PLAYER) {
        BossGanon_Player_Init(this, play);
        if (thisx->update != NULL) {
            sPlayerGanondorf = sGanondorf;
            sPlayerCape = sCape;
        }
    } else if ((thisx->params >= GDF_PARAMS_PLAYER_LIGHT_BALL) &&
               (thisx->params < GDF_PARAMS_PLAYER_BIG_MAGIC + GDF_PLAYER_BIG_MAGIC_COUNT)) {
        BossGanon_PlayerProjectile_Init(this, play);
    } else {
        Actor_Kill(thisx);
    }
}

void BossGanon_Destroy(Actor* thisx, PlayState* play) {
    BossGanon* this = (BossGanon*)thisx;

    if (thisx->params == GDF_PARAMS_PLAYER) {
        if (sPlayerGanondorf == this) {
            sPlayerGanondorf = NULL;
            sGanondorf = NULL;
        }
        if (sPlayerCape != NULL) {
            Actor_Kill(&sPlayerCape->actor);
            sPlayerCape = NULL;
            sCape = NULL;
        }
        if (sPlayerLensFlareOn) {
            gCustomLensFlare1On = false;
            sPlayerLensFlareOn = false;
        }
        SkelAnime_Free(&this->skelAnime, play);
        Collider_DestroyCylinder(play, &this->collider);
    }
}

void BossGanon_Update(Actor* thisx, PlayState* play) {
    // (each kind of his actors sets its own update in its init)
}

void BossGanon_Draw(Actor* thisx, PlayState* play) {
    BossGanon_UsePlayerContext();
    BossGanon_Draw_Impl(thisx, play);
}

void BossGanon_Player_Update(Actor* thisx, PlayState* play) {
    BossGanon_UsePlayerContext();
    BossGanon_Player_Update_Impl(thisx, play);
}

void BossGanon_PlayerLightBall_Update(Actor* thisx, PlayState* play) {
    BossGanon_UsePlayerContext();
    BossGanon_PlayerLightBall_Update_Impl(thisx, play);
}

void BossGanon_PlayerBigMagic_Update(Actor* thisx, PlayState* play) {
    BossGanon_UsePlayerContext();
    BossGanon_PlayerBigMagic_Update_Impl(thisx, play);
}

// z_collision_check.c: his attacks carry every damage type and go through every defense, like Majora's
s32 BossGanon_IsPlayerAttack(Actor* actor) {
    return (actor != NULL) && (actor->id == ACTOR_PLAYER_GDF);
}

// PlayAsGanondorf.cpp: Link is in a state where he can start something (levitating, say)
s32 PlayerGdf_CanAct(Actor* thisx) {
    BossGanon* this = (BossGanon*)thisx;

    return (thisx != NULL) && !this->playerHidden && BossGanon_Player_LinkCanAttack(gPlayState) &&
           !BossGanon_Player_IsBusy(thisx);
}

s32 PlayerGdf_IsHidden(Actor* thisx) {
    return (thisx != NULL) && ((BossGanon*)thisx)->playerHidden;
}

ActorProfile Player_Gdf_Profile = {
    /**/ ACTOR_PLAYER_GDF,
    /**/ ACTORCAT_MISC,
    /**/ FLAGS,
    /**/ GAMEPLAY_KEEP, // his files are read by name from oot.o2r, no object to load
    /**/ sizeof(BossGanon),
    /**/ BossGanon_Init,
    /**/ BossGanon_Destroy,
    /**/ BossGanon_Update,
    /**/ BossGanon_Draw,
};
