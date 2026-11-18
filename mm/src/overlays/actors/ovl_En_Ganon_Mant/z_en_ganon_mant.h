#ifndef Z_EN_GANON_MANT_H
#define Z_EN_GANON_MANT_H

#include <libultraship/libultra.h>
#include "global.h"

struct EnGanonMant;

#define GANON_MANT_NUM_JOINTS 12
#define GANON_MANT_NUM_STRANDS 12

// SoH: params value used for the cape worn by Link
#define LINK_GANON_CAPE_PARAMS 0x4C4B
// SoH: Link's cape vertices are stored at this many units per world unit for extra precision
#define LINK_GANON_CAPE_VTX_SCALE 16.0f
// SoH: params value used for the cape of the player-controlled Ganondorf ("Play as Ganondorf"). It has its own
// vertex buffers, so it can be on screen together with the real Ganondorf's cape.
#define PLAYER_GANONDORF_CAPE_PARAMS 0x4744
// SoH: the player-controlled Ganondorf's cloak: a second cloth that falls over his left shoulder and down his front
// (z_boss_ganon.c sets its roots, its push and the shapes of his body it drapes over every frame)
#define PLAYER_GANONDORF_CLOAK_PARAMS 0x4746
#define PLAYER_GANONDORF_CLOAK2_PARAMS 0x4747
// SoH: the cloak's capelet over his shoulders: three more pieces, the first drawing the other two
#define PLAYER_GANONDORF_CAPELET_PARAMS 0x4748
#define PLAYER_GANONDORF_CAPELET2_PARAMS 0x4749
#define PLAYER_GANONDORF_CAPELET3_PARAMS 0x474A
#define GANON_MANT_MAX_LINKED 2

// SoH: a part of his body the cloth can't pass through: a capsule from a to b, of radius r (world units)
typedef struct {
    Vec3f a;
    Vec3f b;
    f32 r;
} GanonMantCapsule;

typedef struct {
    /* 0x000 */ Vec3f root;                             // root position along the collar
    /* 0x00C */ Vec3f joints[GANON_MANT_NUM_JOINTS];    // "joints" for deforming the cloak, stemming from root and
                                                        // propagating down the cloak
    /* 0x090 */ Vec3f rotations[GANON_MANT_NUM_JOINTS]; // normal vector rotations, x and y only
    /* 0x120 */ Vec3f
        velocities[GANON_MANT_NUM_JOINTS];      // velocities calculated as differences in position from last update
    /* 0x1B0 */ u8 torn[GANON_MANT_NUM_JOINTS]; // guess: whether the joint is torn?
} MantStrand;                                   // size = 0x1C8

typedef struct EnGanonMant {
    /* 0x0000 */ Actor actor;
    /* 0x014C */ MantStrand strands[GANON_MANT_NUM_STRANDS];
    /* 0x16AC */ f32 minY;     // minimum Y value possible for joints, for emulating collision with a floor
    /* 0x16B0 */ f32 backPush; // larger values push the cloak further outwards, negative is away from the actor
    /* 0x16B4 */ f32 backSwayMagnitude; // magnitude of backwards/forwards swaying
    /* 0x16B8 */ f32 sideSwayMagnitude; // magnitude of sideways swaying
    /* 0x16BC */ f32
        attachRightArmTimer; // timer for the duration of which the cloak is attached to right forearm and left shoulder
    /* 0x16C0 */ f32
        attachLeftArmTimer; // timer for the duration of which the cloak is attached to left forearm and right shoulder
    /* 0x16C4 */ f32 attachShouldersTimer; // timer for the duration of which the cloak is attached to both shoulders
    /* 0x16C8 */ f32 gravity;              // strand gravity
    /* 0x16CC */ f32 baseYaw;
    /* 0x16D0 */ f32 minDist; // closest distance the cloak can get to the attached actor without being pushed away
    /* 0x16D4 */ Vec3f rightForearmPos;
    /* 0x16E0 */ Vec3f leftForearmPos;
    /* 0x16EC */ Vec3f rightShoulderPos;
    /* 0x16F8 */ Vec3f leftShoulderPos;
    /* 0x1704 */ u8 tearTimer; // tear the cloak for x many frames
    /* 0x1705 */ u8 updateHasRun;
    /* 0x1706 */ u8 frameTimer;
    // SoH: fields used when the cape is worn by Link (see soh/Enhancements/Graphics/LinkGanonCape.cpp)
    u8 isLinkCape;     // true when spawned with LINK_GANON_CAPE_PARAMS
    u8 needsReset;     // snap all joints to hang straight down on the next physics step
    f32 jointLength;   // strand segment length override, 0 = vanilla
    f32 velocityClamp; // per-axis velocity clamp override, 0 = vanilla (5.0)
    Vec3f anchor;      // origin the vertex buffer is stored relative to (Link mode only)
    u8 ownVertices;    // SoH: uses its own vertex buffers instead of the shared gMant1Vtx/gMant2Vtx
    // SoH: the player-controlled Ganondorf's cloak (both cloths): roots and push direction set from outside, and his
    // body as capsules the cloth is kept out of
    u8 rootOverride;
    Vec3f overrideRoots[GANON_MANT_NUM_STRANDS];
    f32 overrideBaseYaw;
    const GanonMantCapsule* capsules;
    s16 capsuleCount;
    // SoH: cloths sewn on along this one's last strand, one after another (each one's first strand is the one before's
    // last), simulated and drawn by this one, so they're one piece of fabric
    struct EnGanonMant* linked[GANON_MANT_MAX_LINKED];
    s16 linkedCount;
    u8 isLinked; // sewn onto another (drawn by it)
    // SoH: the player-controlled Ganondorf's cape: its hem flared out wider than its top (outward push growing down the
    // cape), and a billow: a ripple travelling down it and out behind him, at billowPhase (radians, advanced from
    // outside), as he walks and runs. Both 0 on every other cape.
    f32 hemFlare;
    f32 billow;
    f32 billowPhase;
} EnGanonMant;

// SoH: the player-controlled Ganondorf's hood, drawn with his cape (z_boss_ganon.c / GanondorfHood.cpp); null if off
// (the hood, GanondorfHood.cpp, is not ported yet)

#endif
