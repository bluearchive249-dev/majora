#ifndef Z_BOSS_GANON_H
#define Z_BOSS_GANON_H

#include "global.h"
#include "gdf_compat.h"

struct BossGanon;

typedef void (*BossGanonActionFunc)(struct BossGanon*, PlayState*);

// SoH: "Play as Ganondorf". These params spawn Ganondorf and his projectiles in player-controlled mode.
// Vanilla uses 0/1 for Ganondorf himself and 0x64-0x190 for his projectiles.
#define GDF_PARAMS_PLAYER 0x10
#define GDF_PARAMS_PUPPET 0x11      // another player's Ganondorf (Invasion mode), posed from the network
#define GDF_PARAMS_PUPPET_BALL 0x12 // his light balls and Big Magic on the other players' screens (just for show)
#define GDF_PARAMS_PLAYER_LIGHT_BALL 0x1F4
#define GDF_PARAMS_PLAYER_BIG_MAGIC 0x1F5 // 0x1F5 - 0x1F9, one per ball
#define GDF_PLAYER_BIG_MAGIC_COUNT 5

// Ganondorf's attacks carry every damage type in the game, so they break rocks and walls, hit switches and hurt
// anything Link could hurt with any weapon. Only "blocked by a shield" is left out: enemies treat that one as a
// harmless shield push. Which damage-table entry is used is decided in CollisionCheck_ApplyDamage.
// Every damage type except the shield and the ones that only stun or freeze (Deku Nut, Boomerang, Hookshot, ice).
// Many enemies check the hit's types directly and take the stun/freeze branch first (Gohma larvae, for one), so
// with those included his attacks would only ever paralyze them.
#define DMG_GANONDORF_PLAYER                                                                                           \
    ((u32) ~(DMG_NORMAL_SHIELD | DMG_DEKU_NUT | DMG_ZORA_BOOMERANG | DMG_HOOKSHOT | DMG_ICE_ARROW | DMG_ZORA_BARRIER | \
             DMG_DEKU_BUBBLE))

typedef enum {
    /*  0 */ GDF_FWORK_0,
    /*  1 */ GDF_FWORK_1,
    /*  2 */ GDF_CENTER_POS,
    /*  3 */ GDF_TRIFORCE_PRIM_B,
    /*  4 */ GDF_TRIFORCE_PRIM_A,
    /*  5 */ GDF_TRIFORCE_ENV_G,
    /*  6 */ GDF_TRIFORCE_SCALE,
    /*  7 */ GDF_VORTEX_ALPHA,
    /*  8 */ GDF_VORTEX_SCALE,
    /*  9 */ GDF_FWORK_UNUSED_9,
    /* 10 */ GDF_FWORK_MAX
} GanondorfFwork;

typedef enum {
    /* 0 */ GDF_WINDOW_SHATTER_OFF,
    /* 1 */ GDF_WINDOW_SHATTER_PARTIAL,
    /* 2 */ GDF_WINDOW_SHATTER_FULL
} WindowShatterState;

typedef enum {
    /* 0 */ GDF_SHOCK_DORF_YELLOW,
    /* 1 */ GDF_SHOCK_PLAYER_YELLOW,
    /* 2 */ GDF_SHOCK_PLAYER_PURPLE
} GanondorfShockType;

typedef enum {
    /* 0 */ GDF_EFF_NONE,
    /* 1 */ GDF_EFF_SPARKLE,
    /* 2 */ GDF_EFF_LIGHT_RAY,
    /* 3 */ GDF_EFF_SHOCK,
    /* 4 */ GDF_EFF_LIGHTNING,
    /* 5 */ GDF_EFF_IMPACT_DUST_DARK,
    /* 6 */ GDF_EFF_IMPACT_DUST_LIGHT,
    /* 7 */ GDF_EFF_SHOCKWAVE,
    /* 8 */ GDF_EFF_BLACK_DOT,
    /* 9 */ GDF_EFF_WINDOW_SHARD
} GanondorfEffectType;

typedef enum {
    /* 0 */ GDF_TRIFORCE_PLAYER,
    /* 1 */ GDF_TRIFORCE_ZELDA,
    /* 2 */ GDF_TRIFORCE_DORF
} GanondorfTriforceType;

typedef struct {
    /* 0x00 */ u8 type;
    /* 0x01 */ u8 timer;
    /* 0x04 */ Vec3f pos;
    /* 0x10 */ Vec3f velocity;
    /* 0x1C */ Vec3f accel;
    /* 0x28 */ Color_RGB8 color;
    /* 0x2C */ s16 alpha;
    /* 0x2E */ s16 unk_2E;
    /* 0x30 */ s16 unk_30;
    /* 0x34 */ f32 scale;
    /* 0x38 */ f32 unk_38; // scale target mostly, but used for other things
    /* 0x3C */ f32 unk_3C; // mostly z rot
    /* 0x40 */ f32 unk_40;
    /* 0x44 */ f32 unk_44; // mostly x rot
    /* 0x48 */ f32 unk_48; // mostly y rot
    u32 epoch;
} GanondorfEffect; // size = 0x4C

typedef struct BossGanon {
    /* 0x0000 */ Actor actor;
    /* 0x014C */ s32 animBankIndex;
    /* 0x0150 */ SkelAnime skelAnime;
    /* 0x0194 */ BossGanonActionFunc actionFunc;
    /* 0x0198 */ u8 unk_198;
    /* 0x0199 */ u8 legSwayEnabled;
    /* 0x019A */ u8 unk_19A;
    /* 0x019C */ s16 unk_19C; // timer
    /* 0x019E */ u8 unk_19E;
    /* 0x019F */ u8 unk_19F;
    /* 0x01A0 */ s8 envLightMode;
    /* 0x01A2 */ s16 unk_1A2;
    /* 0x01A4 */ s16 unk_1A4;
    /* 0x01A6 */ s16 unk_1A6;
    /* 0x01A8 */ s16 unk_1A8;
    /* 0x01AA */ s16 unk_1AA;
    /* 0x01AC */ s16 unk_1AC;
    /* 0x01AE */ s16 triforceType;
    /* 0x01B0 */ char unk_1B0[0x6];
    /* 0x01B6 */ s16 timers[5];
    /* 0x01C0 */ u8 startVolley;
    /* 0x01C2 */ s16 unk_1C2;
    /* 0x01C4 */ s16 screenFlashTimer;
    /* 0x01C8 */ f32 fwork[GDF_FWORK_MAX];
    /* 0x01F0 */ Vec3f unk_1F0;
    /* 0x01FC */ Vec3f unk_1FC;
    /* 0x0208 */ Vec3f unk_208;
    /* 0x0214 */ Vec3f unk_214;
    /* 0x0220 */ Vec3f unk_220;
    /* 0x022C */ Vec3f unk_22C;
    /* 0x0238 */ Vec3f unk_238;
    /* 0x0244 */ char unk_244[0x10];
    /* 0x0254 */ f32 handLightBallScale;
    /* 0x0258 */ f32 unk_258;
    /* 0x025C */ u8 unk_25C;
    /* 0x0260 */ Vec3f unk_260; // hand position?
    /* 0x026C */ s16 unk_26C;   // timer?
    /* 0x0270 */ f32 unk_270;
    /* 0x0274 */ u8 unk_274;
    /* 0x0278 */ Vec3f unk_278;
    /* 0x0284 */ f32 unk_284; // scale for something
    /* 0x0288 */ f32 unk_288;
    /* 0x028C */ f32 unk_28C; // scale for something
    /* 0x0290 */ f32 unk_290;
    /* 0x0294 */ f32 unk_294[15];
    /* 0x02D0 */ f32 unk_2D0; // scale for something
    /* 0x02D4 */ s16 unk_2D4; // timer
    /* 0x02D8 */ Vec3f triforcePos;
    /* 0x02E4 */ u8 shockGlow;
    /* 0x02E6 */ s16 unk_2E6;       // timer
    /* 0x02E8 */ s16 unk_2E8;       // timer
    /* 0x02EC */ Vec3f unk_2EC[18]; // body parts pos for ganondorf, used for other things as well
    /* 0x03C4 */ Vec3f unk_3C4[18];
    /* 0x049C */ f32 unk_49C[18];
    /* 0x04E4 */ s16 unk_4E4[18];
    /* 0x0508 */ f32 unk_508;
    /* 0x050C */ Vec3f legRot;
    /* 0x0518 */ char unk_518[0xF8];
    /* 0x0610 */ ColliderCylinder collider;
    /* 0x065C */ char unk_65C[0x10];
    /* 0x066C */ u8 lensFlareMode;
    /* 0x066E */ s16 lensFlareTimer;
    /* 0x0670 */ f32 lensFlareScale;
    /* 0x0674 */ u32 csTimer;
    /* 0x0678 */ s16 csState;
    /* 0x067A */ s16 csCamIndex;
    /* 0x067C */ char unk_67C[0x4];
    /* 0x0680 */ Vec3f csCamEye;
    /* 0x068C */ Vec3f csCamAt;
    /* 0x0698 */ char unk_698[0xC];
    /* 0x06A4 */ Vec3f csCamEyeMaxStep;
    /* 0x06B0 */ Vec3f csCamAtMaxStep;
    /* 0x06BC */ Vec3f csCamTargetEye;
    /* 0x06C8 */ char unk_6C8[0xC];
    /* 0x06D4 */ Vec3f csCamTargetAt;
    /* 0x06E0 */ char unk_6E0[0xC];
    /* 0x06EC */ f32 csCamMaxStepScale;
    /* 0x06F0 */ f32 csCamMovementScale;
    /* 0x06F4 */ f32 csCamFov;
    /* 0x06F8 */ char unk_6F8[0xC];
    /* 0x0704 */ f32 unk_704;
    /* 0x0708 */ char unk_708[0x4];
    /* 0x070C */ f32 unk_70C;
    /* 0x070C */ f32 unk_710;
    /* 0x0714 */ f32 whiteFillAlpha;
    /* 0x0718 */ s16 organAlpha;
    /* 0x071A */ u8 useOpenHand;
    /* 0x071B */ u8 windowShatterState;
    // SoH: player-controlled Ganondorf
    u8 playerHidden;     // not drawn this frame (first person, cutscene showing Link, ...)
    u16 playerBtnHeld;   // button that started the current attack, while it is still held
    s16 playerHeldTimer; // how long it has been held
    s16 playerPrevInvincibilityTimer;
    Vec3f playerAimPos;     // where attacks are thrown at
    f32 playerFloorY;       // floor below Link, replaces the arena floor (y = 0)
    Vec3f playerOrganPos;   // where the organ stands (its room origin)
    f32 playerSprintWeight; // holding A on the ground: sprinting
    f32 playerClimbPhase;   // ladder and vine climbing cycle
    f32 playerItemWeight;   // holding a found item up
    f32 playerRideFrame;    // riding his horse: the young Ganondorf horseback ride cycle
    u8 playerCrawlFlame;    // travelling through a crawlspace as a purple flame
    s16 playerOrganCam;     // close-up camera on the keys while he plays (0: none)
    s16 playerOrganYaw;     // which way the organ faces
    f32 playerOrganSink;    // how far below the floor the organ is, 0 = fully up
    u8 playerOrganMusic;    // Ganondorf's Theme is playing and the area music has to come back afterwards
    u16 playerPrevBgm;      // the area music to bring back
    s16 playerShieldPitch;  // tilt of the cape shield, from the stick while blocking
    u8 playerGrounded;      // standing/walking on the ground instead of floating (not levitating)
    f32 playerGroundOffset; // keeps his feet on the floor whatever the animation
    f32 playerWalkFrame;    // phase of the walk cycle
    f32 playerWalkWeight;   // 0 standing .. 1 walking
    f32 playerWalkStride;   // wider than the walk animation when running
    f32 playerAuraScale;    // insta-kill dark aura (D-Pad Left mode): size of the vortex around him
    f32 playerAuraAlpha;
    s16 playerAuraTimer;
    f32 playerStandLegsWeight; // grounded: his legs stand while his upper body plays a floating action
    u8 playerStandAnim;        // the standing (not floating) idle is playing
    // Sword stance (D-Pad Left)
    u8 playerSwordAttack; // which attack is playing (GDF_SWORD_ATK_*)
    u8 playerSwordQueued; // B pressed during a swing: the next hit of the combo
    u8 playerSwordGuard;  // R held with the sword out
    u8 playerHandMtxValid;
    u8 playerBladeValid; // blade positions from the last two draws
    s16 playerSwordTimer;
    s16 playerDashTimer;
    s16 playerDashYaw;
    s16 playerSpinYaw; // extra body turn for the spin slash
    f32 playerDashSpeed;
    f32 playerSheathArmWeight; // left arm holding the sheath at his hip
    f32 playerSwordArmWeight;  // right arm in a sword pose
    f32 playerGuardWeight;
    Vec3s playerArmFrom[3]; // right arm when the attack started, to blend from
    MtxF playerLeftHandMtx;
    MtxF playerRightHandMtx;
    Vec3f playerBladeBase;
    Vec3f playerBladeTip;
    Vec3f playerBladeBasePrev;
    Vec3f playerBladeTipPrev;
    ColliderQuad playerSwordQuad;
    s32 playerSwordBlure;
    u8 playerWasAirborne;  // Link was jumping or falling
    f32 playerJumpTopY;    // highest point of the jump, for the landing
    f32 playerAirWeight;   // 0 on the ground .. 1 in the air (jump pose)
    u8 playerOrganOcarina; // the organ is played as his ocarina (Link's songs), not for Ganondorf's Theme
    // Phantom Ganon skin: his skeleton, posed from Ganondorf's every frame, and the spear's hit points
    SkelAnime playerPgSkel;
    Vec3s playerPgJoints[26];
    Vec3s playerPgMorph[26];
    u8 playerPgReady;
    s16 playerAimYaw; // the attack direction (the engine overwrites yawTowardsPlayer every frame)
    u8 playerSpearValid;
    Vec3f playerSpearBase;
    Vec3f playerSpearTip;
} BossGanon;

// SoH: "Play as Ganondorf"
s32 BossGanon_Player_IsBusy(Actor* thisx);
s32 BossGanon_Player_IsHolding(void);
s32 BossGanon_Player_InStance(void);                              // his sword or trident is out
s32 BossGanon_Player_TryBreakKick(Actor* thisx, PlayState* play); // A at a shut door: he kicks it in
s32 BossGanon_Player_IsPhantom(void);
// The guards hunt him (he murdered someone in the market and hasn't left by the drawbridge)
s32 BossGanon_Player_IsWanted(void);
// Killing NPCs (option): attack volumes from Ganon's form, and the check itself
void BossGanon_Player_NoteAttackCylinder(ColliderCylinder* cyl);
void BossGanon_Player_NoteAttackSphere(Vec3f* center, f32 radius);
void BossGanon_Player_KillNpcsNow(PlayState* play);
s32 BossGanon_Player_GetItemHoldPos(Actor* thisx, Vec3f* pos);
void BossGanon_Player_HoldUp(s16 frames);      // hold something up over his head for a while, as for an item
s32 BossGanon_Player_GetHeldUpPos(Vec3f* pos); // where that is (false when he isn't)
s32 BossGanon_Player_AuraReaches(PlayState* play, Actor* boss, Vec3f* pos);
void BossGanon_Player_AuraFakeHit(Collider* col, ColliderInfo* elem, u32 dmgFlags);
s32 BossGanon_Player_SwallowLightBall(PlayState* play, Vec3f* pos, f32 radius);
s32 BossGanon_Player_IsHidden(Actor* thisx);
s32 BossGanon_IsPlayerAttack(Actor* actor);
s32 BossGanon_Player_CanAct(Actor* thisx);
s32 BossGanon_Player_IsCapeUp(Actor* thisx);
void BossGanon_Player_UpdateCapeShield(Actor* thisx, PlayState* play);
s32 BossGanon_Player_GetDash(Actor* thisx, s16* yaw, f32* speed);
// Shared with Ganon (Boss_Ganon2)
s32 BossGanon_Player_ShouldHideAnyForm(PlayState* play);
s32 BossGanon_Player_LinkCanAttack(PlayState* play);

// For PlayAsGanondorf.cpp and z_collision_check.c
s32 PlayerGdf_IsHidden(Actor* thisx);
s32 PlayerGdf_CanAct(Actor* thisx);
s32 BossGanon_IsPlayerAttack(Actor* actor);

// PlayAsGanondorf.cpp
s32 PlayAsGanondorf_AssetsReady(void);
void PlayAsGanondorf_RetryAssets(void);

#endif
