/*
 * File: z_en_ganon_mant.c
 * Overlay: ovl_En_Ganon_Mant
 * Description: Ganondorf's Cape
 */

#include "z_en_ganon_mant.h"
#include <string.h>
#include "overlays/actors/ovl_Player_Gdf/z_player_gdf.h"
#include <libultraship/bridge/resourcebridge.h>
#include <libultraship/bridge/consolevariablebridge.h>

// BenPort.cpp
Vtx* ResourceMgr_LoadVtxByName(char* path);
void Gfx_RegisterBlendedTexture(const char* name, u8* mask, u8* replacement);

#define FLAGS (ACTOR_FLAG_UPDATE_CULLING_DISABLED | ACTOR_FLAG_DRAW_CULLING_DISABLED)

void EnGanonMant_Init(Actor* thisx, PlayState* play);
void EnGanonMant_Destroy(Actor* thisx, PlayState* play);
void EnGanonMant_Update(Actor* thisx, PlayState* play);
void EnGanonMant_Draw(Actor* thisx, PlayState* play);

ActorProfile En_Ganon_Mant_Profile = {
    /**/ ACTOR_EN_GANON_MANT,
    /**/ ACTORCAT_BOSS,
    /**/ FLAGS,
    /**/ GAMEPLAY_KEEP,
    /**/ sizeof(EnGanonMant),
    /**/ EnGanonMant_Init,
    /**/ EnGanonMant_Destroy,
    /**/ EnGanonMant_Update,
    /**/ EnGanonMant_Draw,
};

static s16 sTearSizesMedium[] = {
    0, 0, 0, 1, 1, 1, 1, 1, 0, 0,
};

static s16 sTearSizesLarge[] = {
    0, 0, 0, 0, 1, 1, 2, 2, 2, 1, 1, 0, 0, 0, 0,
};

static s16 sTearSizesSmall[] = {
    0, 0, 0, 0, 0, 0, 0,
};

typedef struct {
    s16* tearAreaSizes;
    s16 count;
} TearShape; // size = 0x8

/**
 * The arrays pointed to by this table describe how many pixels should
 * be removed from the cloak texture in a single pass
 */
static TearShape sTearShapes[] = {
    { sTearSizesMedium, ARRAY_COUNT(sTearSizesMedium) },
    { sTearSizesMedium, ARRAY_COUNT(sTearSizesMedium) },
    { sTearSizesLarge, ARRAY_COUNT(sTearSizesLarge) },
    { sTearSizesSmall, ARRAY_COUNT(sTearSizesSmall) },
};

// How much each joint is affected by backwards/forwards swaying motion
static f32 sBackSwayCoefficients[GANON_MANT_NUM_JOINTS] = {
    0.0f, 1.0f, 0.5f, 0.25f, 0.1f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
};

static f32 D_80A24DB4[] = {
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
};

// How much each joint is affected by sideways swaying motion, tends to 0
static f32 sSideSwayCoefficients[GANON_MANT_NUM_JOINTS] = {
    0.0f, 1.0f, 0.9f, 0.8f, 0.7f, 0.6f, 0.5f, 0.4f, 0.3f, 0.2f, 0.1f, 0.0f,
};

static f32 D_80A24E00[] = {
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
};

static f32 sDistMultipliers[GANON_MANT_NUM_JOINTS] = {
    0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.1f, 1.2f, 1.3f, 1.4f, 1.5f, 1.6f, 1.7f,
};

static f32 D_80A24E48[] = {
    1.8f,
};

#define MAP_STRAND_TO_VTX(n)                                                                               \
    (n) + GANON_MANT_NUM_JOINTS * 0, (n) + GANON_MANT_NUM_JOINTS * 1, (n) + GANON_MANT_NUM_JOINTS * 2,     \
        (n) + GANON_MANT_NUM_JOINTS * 3, (n) + GANON_MANT_NUM_JOINTS * 4, (n) + GANON_MANT_NUM_JOINTS * 5, \
        (n) + GANON_MANT_NUM_JOINTS * 6, (n) + GANON_MANT_NUM_JOINTS * 7, (n) + GANON_MANT_NUM_JOINTS * 8, \
        (n) + GANON_MANT_NUM_JOINTS * 9, (n) + GANON_MANT_NUM_JOINTS * 10, (n) + GANON_MANT_NUM_JOINTS * 11

static u16 sVerticesMap[GANON_MANT_NUM_STRANDS * GANON_MANT_NUM_JOINTS] = {
    MAP_STRAND_TO_VTX(11), MAP_STRAND_TO_VTX(10), MAP_STRAND_TO_VTX(9), MAP_STRAND_TO_VTX(8),
    MAP_STRAND_TO_VTX(7),  MAP_STRAND_TO_VTX(6),  MAP_STRAND_TO_VTX(5), MAP_STRAND_TO_VTX(4),
    MAP_STRAND_TO_VTX(3),  MAP_STRAND_TO_VTX(2),  MAP_STRAND_TO_VTX(1), MAP_STRAND_TO_VTX(0),
};

#define MANT_TEX_WIDTH 32
#define MANT_TEX_HEIGHT 64

static u8 sMaskTex[MANT_TEX_WIDTH * MANT_TEX_HEIGHT] = { { 0 } };

#include "oot/overlays/ovl_En_Ganon_Mant/ovl_En_Ganon_Mant.h"

// SoH: Phantom Ganon skin for the player Ganondorf ("Play as Ganondorf"). His cape turns black and gray, with a
// ragged hem (some strands end early) and holes worn through it.
static u8 sPhantomMantTex[MANT_TEX_WIDTH * MANT_TEX_HEIGHT * 2];
static const u8 sPhantomHem[GANON_MANT_NUM_STRANDS] = { 12, 9, 11, 8, 12, 10, 7, 11, 9, 12, 8, 10 };

static s32 EnGanonMant_IsPhantom(EnGanonMant* this) {
    return this->ownVertices && CVarGetInteger("gModes.PlayAsGanondorf.PhantomGanon", 0);
}

// The cape texture in grays, with holes (alpha 0) worn into the middle of the cape and wherever it was torn
static s32 EnGanonMant_BuildPhantomTex(void) {
    u8* src = (u8*)ResourceGetDataByName(gMantTex);
    s32 i;

    if (src == NULL) {
        return false;
    }
    for (i = 0; i < MANT_TEX_WIDTH * MANT_TEX_HEIGHT; i++) {
        s32 tx = i % MANT_TEX_WIDTH;
        s32 ty = i / MANT_TEX_WIDTH;
        u16 c = (src[2 * i] << 8) | src[2 * i + 1];
        s32 r = (c >> 11) & 0x1F;
        s32 g = (c >> 6) & 0x1F;
        s32 b = (c >> 1) & 0x1F;
        s32 gray = (r * 77 + g * 150 + b * 29) >> 8;
        u32 hash = (u32)(tx * 73856093) ^ (u32)(ty * 19349663);
        u8 hole;

        gray = 2 + (gray * 15) / 31;
        // The texture repeats mirrored along the cape: tx near 0 is both the top and the hem, so the holes stay in
        // the middle of the cape
        hash ^= hash >> 13;
        hash *= 0x5BD1E995;
        hash ^= hash >> 15;
        hole = (tx >= 12) && ((hash % 100) < 9);
        if (sMaskTex[i]) {
            hole = true;
        }
        c = (gray << 11) | (gray << 6) | (gray << 1) | (hole ? 0 : 1);
        sPhantomMantTex[2 * i] = c >> 8;
        sPhantomMantTex[2 * i + 1] = c & 0xFF;
    }
    return true;
}

// SoH: double-buffered vertices for the cape that owns them (only the player-controlled Ganondorf's cape).
// Filled from the shared buffers once so texture coordinates and colors match.
#define GANON_MANT_NUM_VTX 144
// (A set each: his cape, and his cloak's two other pieces)
static Vtx sOwnVertices[6][2][GANON_MANT_NUM_VTX];
static u8 sOwnVerticesReady[6] = { false, false, false, false, false, false };

static Vtx* EnGanonMant_GetOwnVertices(EnGanonMant* this) {
    s32 set = ((this->actor.params >= PLAYER_GANONDORF_CLOAK_PARAMS) &&
               (this->actor.params <= PLAYER_GANONDORF_CAPELET3_PARAMS))
                  ? (this->actor.params - PLAYER_GANONDORF_CLOAK_PARAMS + 1)
                  : 0;

    if (!sOwnVerticesReady[set]) {
        Vtx* src1 = ResourceMgr_LoadVtxByName(SEGMENTED_TO_VIRTUAL(gMant1Vtx));
        Vtx* src2 = ResourceMgr_LoadVtxByName(SEGMENTED_TO_VIRTUAL(gMant2Vtx));

        if ((src1 == NULL) || (src2 == NULL)) {
            return NULL;
        }
        memcpy(sOwnVertices[set][0], src1, sizeof(sOwnVertices[set][0]));
        memcpy(sOwnVertices[set][1], src2, sizeof(sOwnVertices[set][1]));
        sOwnVerticesReady[set] = true;
    }

    return sOwnVertices[set][(this->frameTimer % 2 != 0) ? 0 : 1];
}

void EnGanonMant_Init(Actor* thisx, PlayState* play) {
    EnGanonMant* this = (EnGanonMant*)thisx;

    this->actor.flags &= ~ACTOR_FLAG_ATTENTION_ENABLED;

    if (this->actor.params == LINK_GANON_CAPE_PARAMS) {
        this->isLinkCape = true;
        this->needsReset = true;
    }

    if ((this->actor.params == PLAYER_GANONDORF_CAPE_PARAMS) ||
        ((this->actor.params >= PLAYER_GANONDORF_CLOAK_PARAMS) &&
         (this->actor.params <= PLAYER_GANONDORF_CAPELET3_PARAMS))) {
        this->ownVertices = true;
    }

    for (int i = 0; i < ARRAY_COUNT(sMaskTex); i++) {
        sMaskTex[i] = 0;
    }

    Gfx_RegisterBlendedTexture(gMantTex, sMaskTex, NULL);
}

void EnGanonMant_Destroy(Actor* thisx, PlayState* play) {
}

/**
 * Randomly zeros portions of the cloak texture
 */
void EnGanonMant_Tear(EnGanonMant* this) {
    s32 pad;
    s16 i;
    s16 areaX;
    s16 areaY;
    s16 texIdx;
    f32 tx = Rand_ZeroFloat(MANT_TEX_WIDTH);
    f32 ty = Rand_ZeroFloat(MANT_TEX_HEIGHT);
    f32 tearAngle = Rand_ZeroFloat(2 * M_PI);
    f32 tearDirX = sinf(tearAngle);
    f32 tearDirY = cosf(tearAngle);
    TearShape* shape = &sTearShapes[(s16)Rand_ZeroFloat(ARRAY_COUNT(sTearShapes) - 0.01f)];
    s16 count = shape->count;
    s16* tearAreaSizes = shape->tearAreaSizes;

    for (i = 0; i < count; i++) {
        if ((0 <= tx && tx < MANT_TEX_WIDTH) && (0 <= ty && ty < MANT_TEX_HEIGHT)) {
            for (areaX = 0; areaX <= tearAreaSizes[i]; areaX++) {
                texIdx = 0;
                for (areaY = 0; areaY <= tearAreaSizes[i]; areaY++) {
                    texIdx = (s16)((s16)tx + ((s16)ty * MANT_TEX_WIDTH)) + ((s16)areaX + ((s16)areaY * MANT_TEX_WIDTH));
                    if (texIdx < MANT_TEX_WIDTH * MANT_TEX_HEIGHT) {
                        sMaskTex[texIdx] = 1;
                    }
                }
            }
        }
        tx += tearDirX;
        ty += tearDirY;
    }

    for (i = 0; i < 4; i++) {
        this->strands[(s16)Rand_ZeroFloat(GANON_MANT_NUM_STRANDS - 0.1f)]
            .torn[(s16)Rand_ZeroFloat(GANON_MANT_NUM_JOINTS - 0.1f)] = true;
    }
}

/**
 * Updates the dynamic strands that control the shape and motion of the cloak
 */
void EnGanonMant_UpdateStrand(PlayState* play, EnGanonMant* this, Vec3f* root, Vec3f* pos, Vec3f* nextPos, Vec3f* rot,
                              Vec3f* vel, s16 strandNum) {
    f32 xDiff;
    f32 zDiff;
    f32 gravity;
    s32 pad[4];
    f32 yaw;
    s16 i;
    f32 x;
    f32 y;
    f32 z;
    f32 jointLength;
    Vec3f delta;
    Vec3f posStep;
    Vec3f backSwayOffset;
    Vec3f sideSwayOffset;

    delta.y = 0;
    if (this->actor.params == 0x23) {
        // Pushes all the strands away from the actor
        delta.x = 0.0f;
        delta.z = -30.0f;
        Matrix_RotateY(BINANG_TO_RAD(this->actor.shape.rot.y), MTXMODE_NEW);
        Matrix_MultVec3f(&delta, &posStep);
        for (i = 0; i < GANON_MANT_NUM_JOINTS; i++) {
            (pos + i)->x += posStep.x;
            (pos + i)->z += posStep.z;
        }
        // Set length
        jointLength = 6.5f;
    } else if (this->jointLength > 0.0f) {
        // SoH: Link's cape uses a shorter, configurable length
        jointLength = this->jointLength;
    } else {
        jointLength = 9.5f;
    }

    for (i = 0; i < GANON_MANT_NUM_JOINTS; i++, pos++, vel++, rot++, nextPos++) {
        if (i == 0) {
            // Constraint: first position is always root
            pos->x = root->x;
            pos->y = root->y;
            pos->z = root->z;
        } else {
            // Decelerate
            Math_ApproachZeroF(&vel->x, 1.0f, 0.1f);
            Math_ApproachZeroF(&vel->y, 1.0f, 0.1f);
            Math_ApproachZeroF(&vel->z, 1.0f, 0.1f);

            // Push the cloak away from attached actor, plus oscillations
            delta.x = 0;
            delta.z = (this->backPush + (sinf((strandNum * (2 * M_PI)) / 2.1f) * this->backSwayMagnitude)) *
                      sBackSwayCoefficients[i];
            Matrix_RotateY(this->baseYaw, MTXMODE_NEW);
            Matrix_MultVec3f(&delta, &backSwayOffset);

            // Push the cloak out to either side, in a swaying manner
            delta.x = cosf((strandNum * M_PI) / (GANON_MANT_NUM_STRANDS - 1.0f)) * this->sideSwayMagnitude *
                      sSideSwayCoefficients[i];
            delta.z = 0;
            Matrix_MultVec3f(&delta, &sideSwayOffset);

            // SoH: the player-controlled Ganondorf's cape: its hem flared wide (its edge strands pushed out to either
            // side, more the lower the joint), and its billow (a wave rolling down it from the collar, swelling out
            // behind him and fluttering its sides)
            if ((this->hemFlare != 0.0f) || (this->billow != 0.0f)) {
                f32 down = i / (GANON_MANT_NUM_JOINTS - 1.0f);
                f32 across = cosf((strandNum * M_PI) / (GANON_MANT_NUM_STRANDS - 1.0f));
                f32 wave = sinf(this->billowPhase - i * 0.55f + strandNum * 0.35f);
                Vec3f extra;

                delta.x = -across * (this->hemFlare * down * down +
                                     this->billow * 0.6f * sinf(this->billowPhase * 1.3f - i * 0.45f) * down);
                delta.z = -this->billow * (0.55f + 0.45f * wave) * down;
                Matrix_MultVec3f(&delta, &extra);
                sideSwayOffset.x += extra.x;
                sideSwayOffset.z += extra.z;
            }

            // Calculate position difference
            gravity = this->gravity;
            x = ((pos->x + vel->x) - (pos - 1)->x) + (backSwayOffset.x + sideSwayOffset.x);
            y = ((pos->y + vel->y) - (pos - 1)->y) + gravity;
            z = ((pos->z + vel->z) - (pos - 1)->z) + (backSwayOffset.z + sideSwayOffset.z);

            // Calculate rotations in the direction of the position difference
            yaw = Math_Atan2F(z, x);
            x = -Math_Atan2F(sqrtf(SQ(x) + SQ(z)), y);
            (rot - 1)->x = x;

            // Calculate real position difference of correct length in the correct direction
            delta.x = 0;
            delta.z = jointLength;
            Matrix_RotateY(yaw, MTXMODE_NEW);
            Matrix_RotateX(x, MTXMODE_APPLY);
            Matrix_MultVec3f(&delta, &posStep);

            // Save position
            x = pos->x;
            y = pos->y;
            z = pos->z;

            // Calculate next position
            pos->x = (pos - 1)->x + posStep.x;
            pos->y = (pos - 1)->y + posStep.y;
            pos->z = (pos - 1)->z + posStep.z;

            // Pushes the cloak away from the actor if it is too close
            xDiff = pos->x - this->actor.world.pos.x;
            zDiff = pos->z - this->actor.world.pos.z;
            if (sqrtf(SQ(xDiff) + SQ(zDiff)) < (sDistMultipliers[i] * this->minDist)) {
                yaw = Math_Atan2F(zDiff, xDiff);
                delta.z = this->minDist * sDistMultipliers[i];
                Matrix_RotateY(yaw, MTXMODE_NEW);
                Matrix_MultVec3f(&delta, &posStep);
                pos->x = this->actor.world.pos.x + posStep.x;
                pos->z = this->actor.world.pos.z + posStep.z;
            }

            // SoH: his body (the player-controlled Ganondorf's cloak): out of every capsule, so his torso and arms
            // carry the cloth instead of passing through it
            if (this->capsules != NULL) {
                s16 c;

                for (c = 0; c < this->capsuleCount; c++) {
                    const GanonMantCapsule* cap = &this->capsules[c];
                    f32 abx = cap->b.x - cap->a.x;
                    f32 aby = cap->b.y - cap->a.y;
                    f32 abz = cap->b.z - cap->a.z;
                    f32 len2 = SQ(abx) + SQ(aby) + SQ(abz);
                    f32 t = 0.0f;
                    f32 dx;
                    f32 dy;
                    f32 dz;
                    f32 d;

                    if (len2 > 0.0001f) {
                        t = ((pos->x - cap->a.x) * abx + (pos->y - cap->a.y) * aby + (pos->z - cap->a.z) * abz) / len2;
                        t = CLAMP(t, 0.0f, 1.0f);
                    }
                    dx = pos->x - (cap->a.x + abx * t);
                    dy = pos->y - (cap->a.y + aby * t);
                    dz = pos->z - (cap->a.z + abz * t);
                    d = sqrtf(SQ(dx) + SQ(dy) + SQ(dz));
                    if (d < cap->r) {
                        if (d < 0.001f) {
                            // (right on the bone: out the way the strand hangs from its root)
                            dx = pos->x - root->x;
                            dy = 0.0f;
                            dz = pos->z - root->z;
                            d = sqrtf(SQ(dx) + SQ(dz));
                            if (d < 0.001f) {
                                continue;
                            }
                        }
                        pos->x = (cap->a.x + abx * t) + dx / d * cap->r;
                        pos->y = (cap->a.y + aby * t) + dy / d * cap->r;
                        pos->z = (cap->a.z + abz * t) + dz / d * cap->r;
                    }
                }
            }

            // Enforces minY constraint
            if (pos->y < this->minY) {
                pos->y = this->minY;
            }

            // Calculate next velocity
            vel->x = (pos->x - x) * 80.0f / 100.0f;
            vel->y = (pos->y - y) * 80.0f / 100.0f;
            vel->z = (pos->z - z) * 80.0f / 100.0f;

            if (this->actor.params != 0x23) {
                f32 velClamp = (this->velocityClamp > 0.0f) ? this->velocityClamp : 5.0f;

                // Clamp elements of vel into [-5.0, 5.0]
                if (vel->x > velClamp) {
                    vel->x = velClamp;
                } else if (vel->x < -velClamp) {
                    vel->x = -velClamp;
                }
                if (vel->y > velClamp) {
                    vel->y = velClamp;
                } else if (vel->y < -velClamp) {
                    vel->y = -velClamp;
                }
                if (vel->z > velClamp) {
                    vel->z = velClamp;
                } else if (vel->z < -velClamp) {
                    vel->z = -velClamp;
                }
            }

            // update angle
            xDiff = pos->x - nextPos->x;
            zDiff = pos->z - nextPos->z;
            (rot - 1)->y = Math_Atan2F(zDiff, xDiff);
        }
    }
    rot[11].y = rot[10].y;
    rot[11].x = rot[10].x;
}

/**
 * Update the cloak vertices using the current state of the strands
 */
void EnGanonMant_UpdateVertices(EnGanonMant* this) {
    s16 i;
    Vtx* vtx;
    Vtx* vertices;
    MantStrand* strand;
    s16 j;
    s16 k;
    Vec3f up;
    Vec3f normal;

    if (this->frameTimer % 2 != 0) {
        vertices = SEGMENTED_TO_VIRTUAL(gMant1Vtx);
    } else {
        vertices = SEGMENTED_TO_VIRTUAL(gMant2Vtx);
    }

    vertices = ResourceMgr_LoadVtxByName(vertices);

    // SoH: the player-controlled Ganondorf's cape writes to its own copy, so two capes can be drawn at once
    if (this->ownVertices) {
        Vtx* own = EnGanonMant_GetOwnVertices(this);

        if (own != NULL) {
            vertices = own;
        }
    }

    up.x = 0.0f;
    up.y = 30.0f;
    up.z = 0.0f;

    strand = &this->strands[0];
    for (i = 0; i < GANON_MANT_NUM_STRANDS; i++, strand++) {
        for (j = 0, k = 0; j < GANON_MANT_NUM_JOINTS; j++, k += GANON_MANT_NUM_JOINTS) {
            s16 jp = j;

            // SoH: Phantom Ganon's ragged hem, this strand ends early
            if (EnGanonMant_IsPhantom(this) && (j >= sPhantomHem[i])) {
                jp = sPhantomHem[i] - 1;
            }
            vtx = &vertices[sVerticesMap[i + k]];
            // SoH: the player-controlled Ganondorf's cape is stored relative to an anchor too, so the 60 fps
            // interpolation moves it smoothly together with him instead of in 20 fps steps
            if (this->isLinkCape || this->ownVertices) {
                // SoH: store relative to the anchor at higher precision; the small cape would
                // visibly snap to whole units otherwise
                vtx->n.ob[0] =
                    CLAMP((strand->joints[jp].x - this->anchor.x) * LINK_GANON_CAPE_VTX_SCALE, -32000, 32000);
                vtx->n.ob[1] =
                    CLAMP((strand->joints[jp].y - this->anchor.y) * LINK_GANON_CAPE_VTX_SCALE, -32000, 32000);
                vtx->n.ob[2] =
                    CLAMP((strand->joints[jp].z - this->anchor.z) * LINK_GANON_CAPE_VTX_SCALE, -32000, 32000);
            } else {
                vtx->n.ob[0] = strand->joints[jp].x;
                vtx->n.ob[1] = strand->joints[jp].y;
                vtx->n.ob[2] = strand->joints[jp].z;
            }
            Matrix_RotateY(strand->rotations[jp].y, MTXMODE_NEW);
            Matrix_RotateX(strand->rotations[jp].x, MTXMODE_APPLY);
            Matrix_MultVec3f(&up, &normal);
            vtx->n.n[0] = normal.x;
            vtx->n.n[1] = normal.y;
            vtx->n.n[2] = normal.z;
        }
    }
}

void EnGanonMant_Update(Actor* thisx, PlayState* play) {
    EnGanonMant* this = (EnGanonMant*)thisx;
    BossGanon* ganon = (BossGanon*)this->actor.parent;

    this->updateHasRun = true;
    this->frameTimer++;

    if (this->attachLeftArmTimer == 0.0f) {
    } else {
        this->attachLeftArmTimer -= 1.0f;
    }
    if (this->attachRightArmTimer != 0.0f) {
        this->attachRightArmTimer -= 1.0f;
    }
    if (this->attachShouldersTimer != 0.0f) {
        this->attachShouldersTimer -= 1.0f;
    }

    this->actor.shape.rot.y = ganon->actor.shape.rot.y;

    if (this->tearTimer != 0) {
        this->tearTimer--;
        EnGanonMant_Tear(this);
    }
}

void EnGanonMant_DrawCloak(PlayState* play, EnGanonMant* this) {
    s32 pad;

    OPEN_DISPS(play->state.gfxCtx);

    // Invalidate cape texture mask as it may have been torn
    gSPInvalidateTexCache(POLY_OPA_DISP++, sMaskTex);

    if (this->isLinkCape || this->ownVertices) {
        Matrix_Translate(this->anchor.x, this->anchor.y, this->anchor.z, MTXMODE_NEW);
        Matrix_Scale(1.0f / LINK_GANON_CAPE_VTX_SCALE, 1.0f / LINK_GANON_CAPE_VTX_SCALE,
                     1.0f / LINK_GANON_CAPE_VTX_SCALE, MTXMODE_APPLY);
    } else {
        Matrix_Translate(0.0f, 0.0f, 0.0f, MTXMODE_NEW);
    }

    gSPMatrix(POLY_OPA_DISP++, MATRIX_NEWMTX(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);

    // set texture
    if (EnGanonMant_IsPhantom(this) && EnGanonMant_BuildPhantomTex()) {
        gSPInvalidateTexCache(POLY_OPA_DISP++, sPhantomMantTex);
        gDPPipeSync(POLY_OPA_DISP++);
        gSPTexture(POLY_OPA_DISP++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
        gDPLoadTextureBlock(POLY_OPA_DISP++, sPhantomMantTex, G_IM_FMT_RGBA, G_IM_SIZ_16b, MANT_TEX_WIDTH,
                            MANT_TEX_HEIGHT, 0, G_TX_MIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_CLAMP, 5, 6, G_TX_NOLOD,
                            G_TX_NOLOD);
    } else {
        gSPDisplayList(POLY_OPA_DISP++, gMantMaterialDL);
    }

    // set vertices, vertices are double buffered to prevent
    // modification of vertices as they are being drawn
    if (this->ownVertices && (EnGanonMant_GetOwnVertices(this) != NULL)) {
        gSPSegment(POLY_OPA_DISP++, 0x0C, EnGanonMant_GetOwnVertices(this));
    } else if (this->frameTimer % 2 != 0) {
        gSPSegmentLoadRes(POLY_OPA_DISP++, 0x0C, gMant1Vtx);
    } else {
        gSPSegmentLoadRes(POLY_OPA_DISP++, 0x0C, gMant2Vtx);
    }

    // draw cloak
    gSPDisplayList(POLY_OPA_DISP++, gMantDL);

    if (this->actor.params == PLAYER_GANONDORF_CAPE_PARAMS) {
        // SoH: the player-controlled Ganondorf's hood, the same way as Link's
        void* hoodVtx = NULL; // Play as Ganondorf (2S2H): the hood isn't ported yet

        if (hoodVtx != NULL) {
            gSPSegment(POLY_OPA_DISP++, 0x0C, hoodVtx);
            gSPDisplayList(POLY_OPA_DISP++, gMantDL);
        }
    }

    if (this->isLinkCape) {
        // SoH: Link's hood, same material and grid as the cloak, sharing its top edge
        void* hoodVtx = NULL; // (Link's cape: Ship of Harkinian only)

        if (hoodVtx != NULL) {
            gSPSegment(POLY_OPA_DISP++, 0x0C, hoodVtx);
            gSPDisplayList(POLY_OPA_DISP++, gMantDL);
        }
    }

    CLOSE_DISPS(play->state.gfxCtx);
}

// SoH: a cloth sewn on (the player-controlled Ganondorf's cloak): its roots and push from outside, and its first strand
// the one before's last, so the two are one piece of fabric
static void EnGanonMant_SimLinked(PlayState* play, EnGanonMant* this, EnGanonMant* prev, s32 sewn) {
    s16 strandIdx;

    this->frameTimer = prev->frameTimer; // (the same buffer as the one it's drawn with)
    this->baseYaw = this->overrideBaseYaw;
    this->anchor = this->overrideRoots[GANON_MANT_NUM_STRANDS / 2];
    for (strandIdx = 0; strandIdx < GANON_MANT_NUM_STRANDS; strandIdx++) {
        s16 nextStrandIdx = (strandIdx + 1 < GANON_MANT_NUM_STRANDS) ? (strandIdx + 1) : (strandIdx - 1);

        this->strands[strandIdx].root = this->overrideRoots[strandIdx];
        if ((strandIdx == 0) && sewn) {
            MantStrand* edge = &prev->strands[GANON_MANT_NUM_STRANDS - 1];

            memcpy(this->strands[0].joints, edge->joints, sizeof(edge->joints));
            memcpy(this->strands[0].rotations, edge->rotations, sizeof(edge->rotations));
            memcpy(this->strands[0].velocities, edge->velocities, sizeof(edge->velocities));
            this->strands[0].root = edge->root;
            continue;
        }
        Matrix_Push();
        EnGanonMant_UpdateStrand(play, this, &this->strands[strandIdx].root, this->strands[strandIdx].joints,
                                 this->strands[nextStrandIdx].joints, this->strands[strandIdx].rotations,
                                 this->strands[strandIdx].velocities, strandIdx);
        Matrix_Pop();
    }
    EnGanonMant_UpdateVertices(this);
}

void EnGanonMant_Draw(Actor* thisx, PlayState* play) {
    EnGanonMant* this = (EnGanonMant*)thisx;
    f32 xDiff;
    f32 pitch;
    Vec3f strandOffset;
    Vec3f strandDivPos;
    f32 zDiff;
    f32 diffHalfDist;
    f32 yDiff;
    f32 yaw;
    Vec3f* rightPos;
    Vec3f* leftPos;
    s16 strandIdx;
    Vec3f midpoint;
    s16 nextStrandIdx;

    // Play as Ganondorf (2S2H): his pieces aren't drawn while Link is shown instead (cutscenes can stop his update,
    // which would otherwise take the cape's draw away)
    if (this->ownVertices && BossGanon_Player_ShouldHideAnyForm(play)) {
        return;
    }

    if (this->isLinked) {
        return; // (sewn onto another cloth, which simulates and draws it)
    }

    if (this->isLinkCape) { // (Link's cape: Ship of Harkinian only)
        // SoH: Link isn't visible this frame (first person, hidden in a cutscene, ...)
        return;
    }

    if (this->updateHasRun) {
        // Only run this if update has run since last draw

        // Choose endpoints
        if (this->isLinkCape) {
            // SoH: Link's cape always hangs from his shoulders; gravity etc. are set in PrepareDraw
            rightPos = &this->rightShoulderPos;
            leftPos = &this->leftShoulderPos;
        } else if (this->attachRightArmTimer != 0.0f) {
            rightPos = &this->rightForearmPos;
            leftPos = &this->leftShoulderPos;
            this->gravity = -13.0f;
        } else if (this->attachLeftArmTimer != 0.0f) {
            rightPos = &this->rightShoulderPos;
            leftPos = &this->leftForearmPos;
            this->gravity = -13.0f;
        } else if (this->attachShouldersTimer != 0.0f) {
            rightPos = &this->rightShoulderPos;
            leftPos = &this->leftShoulderPos;
            this->gravity = -3.0f;
        } else {
            rightPos = &this->rightForearmPos;
            leftPos = &this->leftForearmPos;
        }

        xDiff = leftPos->x - rightPos->x;
        yDiff = leftPos->y - rightPos->y;
        zDiff = leftPos->z - rightPos->z;

        midpoint.x = rightPos->x + xDiff * 0.5f;
        midpoint.y = rightPos->y + yDiff * 0.5f;
        midpoint.z = rightPos->z + zDiff * 0.5f;

        // Calculte base orientation for chosen endpoints
        yaw = Math_Atan2F(zDiff, xDiff);
        pitch = -Math_Atan2F(sqrtf(SQ(xDiff) + SQ(zDiff)), yDiff);
        diffHalfDist = sqrtf(SQ(xDiff) + SQ(yDiff) + SQ(zDiff)) * 0.5f;

        Matrix_RotateY(yaw, MTXMODE_NEW);
        Matrix_RotateX(pitch, MTXMODE_APPLY);
        this->baseYaw = yaw - M_PI / 2.0f;
        // SoH: the player-controlled Ganondorf's cloak: its push from outside
        if (this->rootOverride) {
            this->baseYaw = this->overrideBaseYaw;
        }

        if (this->ownVertices) {
            this->anchor = midpoint;
        }

        if (this->isLinkCape) {
            this->anchor = midpoint;

            if (this->needsReset) {
                // SoH: first frame or Link teleported, let every strand hang straight down from its root
                for (strandIdx = 0; strandIdx < GANON_MANT_NUM_STRANDS; strandIdx++) {
                    s16 j;

                    Matrix_Push();
                    strandOffset.x = sinf((strandIdx * M_PI) / (GANON_MANT_NUM_STRANDS - 1)) * diffHalfDist;
                    strandOffset.y = 0;
                    strandOffset.z = -cosf((strandIdx * M_PI) / (GANON_MANT_NUM_STRANDS - 1)) * diffHalfDist;
                    Matrix_MultVec3f(&strandOffset, &strandDivPos);
                    Matrix_Pop();

                    for (j = 0; j < GANON_MANT_NUM_JOINTS; j++) {
                        this->strands[strandIdx].joints[j].x = midpoint.x + strandDivPos.x;
                        this->strands[strandIdx].joints[j].y = midpoint.y + strandDivPos.y - j * this->jointLength;
                        this->strands[strandIdx].joints[j].z = midpoint.z + strandDivPos.z;
                        this->strands[strandIdx].velocities[j].x = 0.0f;
                        this->strands[strandIdx].velocities[j].y = 0.0f;
                        this->strands[strandIdx].velocities[j].z = 0.0f;
                    }
                }
                this->needsReset = false;
            }
        }

        for (strandIdx = 0; strandIdx < GANON_MANT_NUM_STRANDS; strandIdx++) {
            Matrix_Push();

            // Calculate root positions for chosen endpoints
            strandOffset.x = sinf((strandIdx * M_PI) / (GANON_MANT_NUM_STRANDS - 1)) * diffHalfDist;
            strandOffset.y = 0;
            strandOffset.z = -cosf((strandIdx * M_PI) / (GANON_MANT_NUM_STRANDS - 1)) * diffHalfDist;
            Matrix_MultVec3f(&strandOffset, &strandDivPos);
            this->strands[strandIdx].root.x = midpoint.x + strandDivPos.x;
            this->strands[strandIdx].root.y = midpoint.y + strandDivPos.y;
            this->strands[strandIdx].root.z = midpoint.z + strandDivPos.z;
            // SoH: the player-controlled Ganondorf's cloak: its roots from outside (along his shoulders, or over his
            // left shoulder and down his front)
            if (this->rootOverride) {
                this->strands[strandIdx].root = this->overrideRoots[strandIdx];
            }

            nextStrandIdx = strandIdx + 1;
            if (nextStrandIdx >= GANON_MANT_NUM_STRANDS) {
                nextStrandIdx = strandIdx - 1;
            }

            // Update the strand joints
            EnGanonMant_UpdateStrand(play, this, &this->strands[strandIdx].root, this->strands[strandIdx].joints,
                                     this->strands[nextStrandIdx].joints, this->strands[strandIdx].rotations,
                                     this->strands[strandIdx].velocities, strandIdx);
            Matrix_Pop();
        }
        EnGanonMant_UpdateVertices(this);

        // SoH: the cloths sewn on, in order (each onto the one before; the first onto this one, while its roots are
        // its own and not following an arm)
        for (strandIdx = 0; strandIdx < this->linkedCount; strandIdx++) {
            EnGanonMant* prev = (strandIdx == 0) ? this : this->linked[strandIdx - 1];

            EnGanonMant_SimLinked(play, this->linked[strandIdx], prev, (strandIdx > 0) || this->rootOverride);
        }
        this->updateHasRun = false;
    }

    EnGanonMant_DrawCloak(play, this);
    for (strandIdx = 0; strandIdx < this->linkedCount; strandIdx++) {
        EnGanonMant_DrawCloak(play, this->linked[strandIdx]);
    }
}
