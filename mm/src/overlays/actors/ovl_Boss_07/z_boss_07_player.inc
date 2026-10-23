// clang-format off
/**
 * 2S2H "Play as Majora"
 *
 * Majora (Boss_07 with MAJORA_TYPE_PLAYER) follows Link and attacks with its own moves from the final battle.
 * Link is still there underneath: he stays invisible and keeps handling walking, collision, doors, ladders,
 * loading zones and the camera. See 2s2h/Enhancements/Modes/PlayAsMajora.cpp for the Link side (spawning,
 * hiding Link, filtering his buttons).
 *
 * This is its own actor with its own state, so the real Majora in the Moon fight is unaffected.
 *
 * Three forms, switched on the D-pad (the same button again goes back to the Mask). The new form grows in like in the
 * fight: Incarnation's limbs sprout from the mask, Wrath's body stretches out.
 *
 *  - Majora's Mask floats at Link's head height, its tentacles trailing on the floor.
 *    B / C-Left: spin attack. The mask flies spinning at the target, cuts through everything on the way and comes back.
 *    R + B / C-Right: fire beam from its eyes, held as long as the button is. Aims at the target, or the stick steers it.
 *  - Majora's Incarnation (D-Pad Right) runs with Link (faster than Link), leaving afterimages at full speed.
 *    B / C-Left: energy balls from both hands, at the target. Hold to keep firing.
 *    D-Pad Up: a dance, in turn: squatting dance, moonwalk (slides backwards, the stick picks the way), pirouette, taunt.
 *  - Majora's Wrath (D-Pad Left) walks with Link (a walk cycle built for her skeleton), its two whips trailing.
 *    Every whip segment hits while the whip moves fast. Optional bust (from her chest's eyes).
 *    B: quick whip, B again during it: the three-hit combo. Hold B: whip flurry. R + B: spin attack that carries Link.
 *    C-Left: double whip. C-Down: kick. C-Right: long whip. D-Pad Up: taunt.
 *
 * "The target" is Link's lock-on target, else the enemy he faces, else straight ahead.
 * Wrath's attacks are the vanilla `Boss07_Wrath_Attack`, aimed by pointing `yawTowardsPlayer` at the target.
 */
// clang-format on

#define MAJORA_PLAYER_CVAR(name) ("gModes.PlayAsMajora." name)

#include "z_boss_07_player_bust.inc"

// Each form's scale in the final battle; the menu sizes are fractions of these
#define MAJORA_PLAYER_MASK_BATTLE_SCALE 0.1f
#define MAJORA_PLAYER_INCARNATION_BATTLE_SCALE 0.015f
#define MAJORA_PLAYER_WRATH_BATTLE_SCALE 0.015f

#define MAJORA_PLAYER_FORM_GROW_FRAMES 20

void Boss07_Player_Update(Actor* thisx, PlayState* play2);
void Boss07_Player_Draw(Actor* thisx, PlayState* play2);
void Boss07_PlayerProjectile_Update(Actor* thisx, PlayState* play2);

void Boss07_Player_SetupFloat(Boss07* this, PlayState* play);
void Boss07_Player_Float(Boss07* this, PlayState* play);
void Boss07_Player_SetupSpinAttack(Boss07* this, PlayState* play);
void Boss07_Player_SpinAttack(Boss07* this, PlayState* play);
void Boss07_Player_SetupIncarnationMove(Boss07* this, PlayState* play);
void Boss07_Player_IncarnationMove(Boss07* this, PlayState* play);
void Boss07_Player_SetupIncarnationAttack(Boss07* this, PlayState* play);
void Boss07_Player_IncarnationAttack(Boss07* this, PlayState* play);
void Boss07_Player_SetupIncarnationDance(Boss07* this, PlayState* play);
void Boss07_Player_IncarnationDance(Boss07* this, PlayState* play);
void Boss07_Player_SetupWrathMove(Boss07* this, PlayState* play);
void Boss07_Player_WrathMove(Boss07* this, PlayState* play);
void Boss07_Player_SetupFireBeam(Boss07* this, PlayState* play);
void Boss07_Player_FireBeam(Boss07* this, PlayState* play);

typedef enum MajoraPlayerSpinSubAction {
    /* 0 */ MAJORA_PLAYER_SPIN_WIND_UP,
    /* 1 */ MAJORA_PLAYER_SPIN_OUT,
    /* 2 */ MAJORA_PLAYER_SPIN_RETURN,
    /* 3 */ MAJORA_PLAYER_SPIN_END
} MajoraPlayerSpinSubAction;

typedef enum MajoraPlayerDance {
    /* 0 */ MAJORA_PLAYER_DANCE_SQUATTING,
    /* 1 */ MAJORA_PLAYER_DANCE_MOONWALK,
    /* 2 */ MAJORA_PLAYER_DANCE_PIROUETTE,
    /* 3 */ MAJORA_PLAYER_DANCE_TAUNT,
    /* 4 */ MAJORA_PLAYER_DANCE_MAX
} MajoraPlayerDance;

#define MAJORA_PLAYER_SPIN_SPEED 20.0f
#define MAJORA_PLAYER_SPIN_RANGE 900.0f // turns back this far from Link
#define MAJORA_PLAYER_AIM_RANGE 700.0f
#define MAJORA_PLAYER_AIM_CONE 0x2800
#define MAJORA_PLAYER_BALL_SPEED 25.0f
#define MAJORA_PLAYER_BEAM_RANGE 1500.0f
#define MAJORA_PLAYER_BEAM_SPHERES 24
#define MAJORA_PLAYER_MOONWALK_SPEED 9.0f

// Wrath's walk, found from her skeleton (object_boss07): the thighs swing on their z axis (negative = forward), the
// knees bend on the lower-leg roots' z axis, the upper arms swing on their y axis. Per 0x1000 of thigh swing the foot
// moves 2282 model units.
#define MAJORA_PLAYER_WALK_FOOT_PER_BINANG (2282.0f / 0x1000)

static EffectTireMarkInit sPlayerBeamTireMarkInit = { 0, 40, { 0, 0, 15, 200 } };

// The mask's hit while it spins, and the energy balls. AT only: enemies' hits go to Link, not to Majora.
static ColliderCylinderInit sPlayerMaskAttackCylinderInit = {
    {
        COL_MATERIAL_NONE,
        AT_ON | AT_TYPE_PLAYER,
        AC_NONE,
        OC1_NONE,
        OC2_NONE,
        COLSHAPE_CYLINDER,
    },
    {
        ELEM_MATERIAL_UNK0,
        { DMG_MAJORA_PLAYER, 0x00, 0x02 },
        { 0x00000000, 0x00, 0x00 },
        ATELEM_ON | ATELEM_SFX_NORMAL,
        ACELEM_NONE,
        OCELEM_NONE,
    },
    { 40, 50, -25, { 0, 0, 0 } },
};

static ColliderCylinderInit sPlayerProjectileCylinderInit = {
    {
        COL_MATERIAL_NONE,
        AT_ON | AT_TYPE_PLAYER,
        AC_NONE,
        OC1_NONE,
        OC2_NONE,
        COLSHAPE_CYLINDER,
    },
    {
        ELEM_MATERIAL_UNK0,
        { DMG_MAJORA_PLAYER, 0x00, 0x02 },
        { 0x00000000, 0x00, 0x00 },
        ATELEM_ON | ATELEM_SFX_NORMAL,
        ACELEM_NONE,
        OCELEM_NONE,
    },
    { 15, 30, -15, { 0, 0, 0 } },
};

// Wrath's attacks by `MajorasWrathAttackSubAction`
static AnimationHeader* sPlayerWrathAttackAnims[] = {
    &gMajorasWrathShortSingleWhipAttackAnim, // MAJORAS_WRATH_ATTACK_SUB_ACTION_QUICK_WHIP
    &gMajorasWrathWhipFlurryAttackAnim,      // MAJORAS_WRATH_ATTACK_SUB_ACTION_FLURRY
    &gMajorasWrathDoubleWhipAttackAnim,      // MAJORAS_WRATH_ATTACK_SUB_ACTION_DOUBLE_WHIP
    &gMajorasWrathLongSingleWhipAttackAnim,  // MAJORAS_WRATH_ATTACK_SUB_ACTION_LONG_WHIP
    &gMajorasWrathSpinAttackAnim,            // MAJORAS_WRATH_ATTACK_SUB_ACTION_SPIN_ATTACK
    &gMajorasWrathTauntAnim,                 // MAJORAS_WRATH_ATTACK_SUB_ACTION_TAUNT
    &gMajorasWrathThreeAttackComboAnim,      // MAJORAS_WRATH_ATTACK_SUB_ACTION_THREE_HIT
    &gMajorasWrathKickAnim,                  // MAJORAS_WRATH_ATTACK_SUB_ACTION_KICK
};


// ------------------------------------------------------------------------------------------------
// Queries used by the Link side and by the collision code
// ------------------------------------------------------------------------------------------------

s32 Boss07_IsPlayerAttack(Actor* actor) {
    return (actor != NULL) && (actor->id == ACTOR_BOSS_07) &&
           ((actor->params == MAJORA_TYPE_PLAYER) || (actor->params == MAJORA_TYPE_PLAYER_PROJECTILE));
}

static s32 Boss07_Player_IsPlayerMajora(Actor* actor) {
    return (actor != NULL) && (actor->id == ACTOR_BOSS_07) && (actor->params == MAJORA_TYPE_PLAYER);
}

static s32 Boss07_Player_IsIdleAction(Boss07* this) {
    return (this->actionFunc == Boss07_Player_Float) || (this->actionFunc == Boss07_Player_IncarnationMove) ||
           (this->actionFunc == Boss07_Player_WrathMove);
}

s32 Boss07_Player_IsBusy(Actor* thisx) {
    Boss07* this = (Boss07*)thisx;

    if (!Boss07_Player_IsPlayerMajora(thisx)) {
        return false;
    }
    return !Boss07_Player_IsIdleAction(this) || (this->playerFormTimer != 0);
}

s32 Boss07_Player_IsHidden(Actor* thisx) {
    Boss07* this = (Boss07*)thisx;

    if (!Boss07_Player_IsPlayerMajora(thisx)) {
        return true;
    }
    return this->playerHidden;
}

s32 Boss07_Player_IsCarrying(Actor* thisx) {
    Boss07* this = (Boss07*)thisx;

    return Boss07_Player_IsPlayerMajora(thisx) && this->playerCarry && !this->playerHidden;
}

u8 Boss07_Player_GetForm(Actor* thisx) {
    Boss07* this = (Boss07*)thisx;

    return Boss07_Player_IsPlayerMajora(thisx) ? this->playerForm : MAJORA_PLAYER_FORM_MASK;
}

// ------------------------------------------------------------------------------------------------
// Helpers
// ------------------------------------------------------------------------------------------------

static f32 Boss07_Player_GetBattleScale(u8 form) {
    switch (form) {
        case MAJORA_PLAYER_FORM_INCARNATION:
            return MAJORA_PLAYER_INCARNATION_BATTLE_SCALE;
        case MAJORA_PLAYER_FORM_WRATH:
            return MAJORA_PLAYER_WRATH_BATTLE_SCALE;
        default:
            return MAJORA_PLAYER_MASK_BATTLE_SCALE;
    }
}

// The form's size as a fraction of its size in the final battle
static f32 Boss07_Player_GetSizeRatio(u8 form) {
    switch (form) {
        case MAJORA_PLAYER_FORM_INCARNATION:
            return CVarGetFloat(MAJORA_PLAYER_CVAR("SizeIncarnation"), 0.6f);
        case MAJORA_PLAYER_FORM_WRATH:
            return CVarGetFloat(MAJORA_PLAYER_CVAR("SizeWrath"), 0.5f);
        default:
            return CVarGetFloat(MAJORA_PLAYER_CVAR("SizeMask"), 0.5f);
    }
}

// The mask's center above Link's feet. Incarnation and Wrath stand on the floor.
static f32 Boss07_Player_GetHoverHeight(Boss07* this, PlayState* play) {
    Player* player = GET_PLAYER(play);

    if (this->playerForm != MAJORA_PLAYER_FORM_MASK) {
        return 0.0f;
    }
    // Majora's Mask sinks to the floor when Link dies
    if (player->stateFlags1 & PLAYER_STATE1_DEAD) {
        return 12.0f;
    }
    return CVarGetFloat(MAJORA_PLAYER_CVAR("HoverHeight"), 45.0f) + (Math_SinS(this->frameCounter * 0x800) * 3.0f);
}

static void Boss07_Player_GetHoverPos(Boss07* this, PlayState* play, Vec3f* pos) {
    Player* player = GET_PLAYER(play);

    pos->x = player->actor.world.pos.x;
    pos->y = player->actor.world.pos.y + Boss07_Player_GetHoverHeight(this, play);
    pos->z = player->actor.world.pos.z;
}

// Moments when Link has to be shown instead of Majora
static s32 Boss07_Player_ShouldHide(PlayState* play) {
    Player* player = GET_PLAYER(play);

    // Link's other forms are their own characters
    if (player->transformation != PLAYER_FORM_HUMAN) {
        return true;
    }
    // First person
    if (player->stateFlags1 & PLAYER_STATE1_100000) {
        return true;
    }
    // Riding Epona
    if (player->stateFlags1 & PLAYER_STATE1_800000) {
        return true;
    }
    // Link isn't drawn either
    if (player->stateFlags2 & PLAYER_STATE2_20000000) {
        return true;
    }
    // Crawlspaces
    if (player->stateFlags2 & PLAYER_STATE2_40000) {
        return true;
    }
    // Story cutscenes animate Link, show him instead
    if (CVarGetInteger(MAJORA_PLAYER_CVAR("ShowLinkInCutscenes"), 1) && (play->csCtx.state != CS_STATE_IDLE)) {
        return true;
    }
    return false;
}

// Link is in a state where an attack can start
static s32 Boss07_Player_CanStartAttack(Boss07* this, PlayState* play) {
    Player* player = GET_PLAYER(play);

    if (this->playerHidden || Player_InCsMode(play) || (play->csCtx.state != CS_STATE_IDLE)) {
        return false;
    }
    if ((Message_GetState(&play->msgCtx) != TEXT_STATE_NONE) || (player->textboxBtnCooldownTimer != 0)) {
        return false;
    }
    if (player->stateFlags1 &
        (PLAYER_STATE1_20 | PLAYER_STATE1_TALKING | PLAYER_STATE1_DEAD | PLAYER_STATE1_400 |
         PLAYER_STATE1_CARRYING_ACTOR | PLAYER_STATE1_2000 | PLAYER_STATE1_4000 | PLAYER_STATE1_100000 |
         PLAYER_STATE1_200000 | PLAYER_STATE1_20000000)) {
        return false;
    }
    if (player->stateFlags2 & (PLAYER_STATE2_40000 | PLAYER_STATE2_USING_OCARINA)) {
        return false;
    }
    return true;
}

static Actor* Boss07_Player_FindTarget(PlayState* play, Vec3f* from, s16 facingYaw) {
    static u8 sTargetCategories[] = { ACTORCAT_ENEMY, ACTORCAT_BOSS };
    Actor* best = NULL;
    f32 bestScore = 0.0f;
    s32 i;

    for (i = 0; i < ARRAY_COUNT(sTargetCategories); i++) {
        Actor* actor;

        for (actor = play->actorCtx.actorLists[sTargetCategories[i]].first; actor != NULL; actor = actor->next) {
            f32 dist;
            f32 score;
            s16 yawDiff;

            if ((actor->update == NULL) || (actor->id == ACTOR_BOSS_07 && Boss07_IsPlayerAttack(actor)) ||
                !(actor->flags & ACTOR_FLAG_ATTENTION_ENABLED)) {
                continue;
            }

            dist = Math_Vec3f_DistXYZ(from, &actor->focus.pos);
            if (dist >= MAJORA_PLAYER_AIM_RANGE) {
                continue;
            }

            yawDiff = Math_Vec3f_Yaw(from, &actor->focus.pos) - facingYaw;
            if (ABS(yawDiff) > MAJORA_PLAYER_AIM_CONE) {
                continue;
            }

            // Angle counts most, so what Link faces wins over something closer off to the side
            score = (f32)ABS(yawDiff) + (dist * 12.0f);
            if ((best == NULL) || (score < bestScore)) {
                best = actor;
                bestScore = score;
            }
        }
    }

    return best;
}

// Aim at Link's lock-on target, else the enemy he's facing, else straight ahead
static void Boss07_Player_UpdateAim(Boss07* this, PlayState* play) {
    Player* player = GET_PLAYER(play);
    Actor* target = player->focusActor;

    if ((target == NULL) || (target->update == NULL) || Boss07_IsPlayerAttack(target) ||
        ((target->category != ACTORCAT_ENEMY) && (target->category != ACTORCAT_BOSS))) {
        target = Boss07_Player_FindTarget(play, &this->actor.world.pos, player->actor.shape.rot.y);
    }

    this->playerTarget = target;

    if (target != NULL) {
        this->playerAimPos = target->focus.pos;
    } else {
        this->playerAimPos.x = player->actor.world.pos.x + (Math_SinS(player->actor.shape.rot.y) * 350.0f);
        this->playerAimPos.y = player->actor.world.pos.y + 30.0f;
        this->playerAimPos.z = player->actor.world.pos.z + (Math_CosS(player->actor.shape.rot.y) * 350.0f);
    }

    if (this->playerForm == MAJORA_PLAYER_FORM_MASK) {
        // The mask flies level when there's nothing to aim at
        if (target == NULL) {
            this->playerAimPos.y = this->actor.world.pos.y;
        }
    }
}

static s16 Boss07_Player_GetAimYaw(Boss07* this) {
    return Math_Vec3f_Yaw(&this->actor.world.pos, &this->playerAimPos);
}

static void Boss07_Player_SnapToLink(Boss07* this, PlayState* play) {
    Player* player = GET_PLAYER(play);

    Boss07_Player_GetHoverPos(this, play, &this->actor.world.pos);
    this->actor.prevPos = this->actor.world.pos;
    this->actor.shape.rot.y = this->actor.world.rot.y = player->actor.shape.rot.y;
    this->actor.speed = 0.0f;
}

static void Boss07_Player_Follow(Boss07* this, PlayState* play) {
    Vec3f target;
    f32 dx;
    f32 dy;
    f32 dz;

    Boss07_Player_GetHoverPos(this, play, &target);
    dx = target.x - this->actor.world.pos.x;
    dy = target.y - this->actor.world.pos.y;
    dz = target.z - this->actor.world.pos.z;

    // Warps, voiding out, ...
    if ((SQ(dx) + SQ(dy) + SQ(dz)) > SQ(250.0f)) {
        Boss07_Player_SnapToLink(this, play);
        return;
    }

    this->actor.world.pos.x += dx * 0.5f;
    this->actor.world.pos.z += dz * 0.5f;
    // The mask floats after Link, the other forms stand on his floor
    this->actor.world.pos.y += (this->playerForm == MAJORA_PLAYER_FORM_MASK) ? (dy * 0.35f) : dy;
}

// How far Majora moved this frame, along the floor
static f32 Boss07_Player_MovedXZ(Boss07* this) {
    return sqrtf(SQ(this->actor.world.pos.x - this->actor.prevPos.x) +
                 SQ(this->actor.world.pos.z - this->actor.prevPos.z));
}

// Homes in on `target` like the vanilla spin attack: turns toward it at `turnSpeed` and accelerates to `speed`
static f32 Boss07_Player_FlyTowards(Boss07* this, Vec3f* target, f32 speed, f32 turnSpeed) {
    f32 dx = target->x - this->actor.world.pos.x;
    f32 dy = target->y - this->actor.world.pos.y;
    f32 dz = target->z - this->actor.world.pos.z;
    f32 distXZ = sqrtf(SQ(dx) + SQ(dz));

    Math_ApproachS(&this->actor.world.rot.y, Math_Atan2S(dx, dz), 3, turnSpeed);
    Math_ApproachS(&this->actor.world.rot.x, Math_Atan2S(dy, distXZ), 3, turnSpeed);
    Math_ApproachF(&this->actor.speed, speed, 1.0f, 3.0f);

    return sqrtf(SQ(dx) + SQ(dy) + SQ(dz));
}

// Turns an initialized collider into one of Majora's attacks: AT only, every damage type
static void Boss07_Player_MakeAttackJntSph(ColliderJntSph* collider, u8 damage) {
    s32 i;

    collider->base.atFlags = AT_ON | AT_TYPE_PLAYER;
    collider->base.acFlags = AC_NONE;
    collider->base.ocFlags1 = OC1_NONE;
    collider->base.ocFlags2 = OC2_NONE;

    for (i = 0; i < collider->count; i++) {
        ColliderElement* elem = &collider->elements[i].base;

        elem->atDmgInfo.dmgFlags = DMG_MAJORA_PLAYER;
        elem->atDmgInfo.effect = 0;
        elem->atDmgInfo.damage = damage;
        elem->atElemFlags = ATELEM_ON | ATELEM_SFX_NORMAL;
        elem->acElemFlags = ACELEM_NONE;
        elem->ocElemFlags = OCELEM_NONE;
    }
}

// Afterimages draw Majora's current skeleton, so they can't outlive the Incarnation form
static void Boss07_Player_KillAfterimages(Boss07* this, PlayState* play) {
    static const u8 sCategories[] = { ACTORCAT_MISC, ACTORCAT_BOSS };
    s32 i;

    for (i = 0; i < ARRAY_COUNT(sCategories); i++) {
        Actor* actor;

        for (actor = play->actorCtx.actorLists[sCategories[i]].first; actor != NULL; actor = actor->next) {
            if ((actor->id == ACTOR_BOSS_07) && (actor->params == MAJORA_TYPE_INCARNATION_AFTERIMAGE) &&
                (actor->parent == &this->actor)) {
                actor->parent = NULL;
                actor->draw = NULL;
                Actor_Kill(actor);
            }
        }
    }
    this->fireTimer = 0;
}

// ------------------------------------------------------------------------------------------------
// Majora's own hits: one collider of spheres, placed and switched on every frame (whip segments, the beam)
// ------------------------------------------------------------------------------------------------

static void Boss07_Player_InitHitCollider(Boss07* this, PlayState* play) {
    static ColliderJntSphElementInit sElementsInit[MAJORA_PLAYER_HIT_SPHERES];
    static ColliderJntSphInit sInit = {
        { COL_MATERIAL_NONE, AT_ON | AT_TYPE_PLAYER, AC_NONE, OC1_NONE, OC2_NONE, COLSHAPE_JNTSPH },
        MAJORA_PLAYER_HIT_SPHERES,
        sElementsInit,
    };
    s32 i;

    for (i = 0; i < MAJORA_PLAYER_HIT_SPHERES; i++) {
        ColliderJntSphElementInit* elem = &sElementsInit[i];

        elem->base.elemMaterial = ELEM_MATERIAL_UNK0;
        elem->base.atDmgInfo.dmgFlags = DMG_MAJORA_PLAYER;
        elem->base.atDmgInfo.effect = 0;
        elem->base.atDmgInfo.damage = 2;
        elem->base.acDmgInfo.dmgFlags = 0;
        elem->base.acDmgInfo.effect = 0;
        elem->base.acDmgInfo.defense = 0;
        elem->base.atElemFlags = ATELEM_ON | ATELEM_SFX_NORMAL;
        elem->base.acElemFlags = ACELEM_NONE;
        elem->base.ocElemFlags = OCELEM_NONE;
        elem->dim.limb = 0;
        elem->dim.modelSphere.center.x = elem->dim.modelSphere.center.y = elem->dim.modelSphere.center.z = 0;
        elem->dim.modelSphere.radius = 10;
        elem->dim.scale = 100;
    }

    Collider_InitAndSetJntSph(play, &this->playerHitCollider, &this->actor, &sInit, this->playerHitElements);
    this->playerColliders |= MAJORA_PLAYER_COLLIDER_HIT;
}

static void Boss07_Player_ClearHits(Boss07* this) {
    s32 i;

    for (i = 0; i < MAJORA_PLAYER_HIT_SPHERES; i++) {
        this->playerHitElements[i].base.atElemFlags &= ~ATELEM_ON;
    }
}

static void Boss07_Player_SetHit(Boss07* this, s32 index, Vec3f* pos, f32 radius, u8 damage) {
    ColliderJntSphElement* elem = &this->playerHitElements[index];

    elem->base.atElemFlags |= ATELEM_ON;
    elem->base.atDmgInfo.damage = damage;
    elem->dim.worldSphere.center.x = pos->x;
    elem->dim.worldSphere.center.y = pos->y;
    elem->dim.worldSphere.center.z = pos->z;
    elem->dim.worldSphere.radius = radius;
}

static void Boss07_Player_SubmitHits(Boss07* this, PlayState* play) {
    s32 i;

    for (i = 0; i < MAJORA_PLAYER_HIT_SPHERES; i++) {
        if (this->playerHitElements[i].base.atElemFlags & ATELEM_ON) {
            CollisionCheck_SetAT(play, &play->colChkCtx, &this->playerHitCollider.base);
            return;
        }
    }
}

// The colliders and effects that belong to one form
static void Boss07_Player_DestroyFormColliders(Boss07* this, PlayState* play) {
    Boss07_Player_KillAfterimages(this, play);

    if (this->playerColliders & MAJORA_PLAYER_COLLIDER_CYLINDER) {
        Collider_DestroyCylinder(play, &this->generalCollider);
    }
    if (this->playerColliders & MAJORA_PLAYER_COLLIDER_BODY) {
        Collider_DestroyJntSph(play, &this->bodyCollider);
    }
    if (this->playerColliders & MAJORA_PLAYER_COLLIDER_KICK) {
        Collider_DestroyJntSph(play, &this->kickCollider);
    }
    if (this->playerColliders & MAJORA_PLAYER_COLLIDER_TIRE_MARK) {
        Effect_Destroy(play, this->effectIndex);
    }
    this->playerColliders &= MAJORA_PLAYER_COLLIDER_HIT;
}

void Boss07_Player_DestroyColliders(Boss07* this, PlayState* play) {
    Boss07_Player_DestroyFormColliders(this, play);

    if (this->playerColliders & MAJORA_PLAYER_COLLIDER_HIT) {
        Collider_DestroyJntSph(play, &this->playerHitCollider);
    }
    this->playerColliders = 0;
}

// ------------------------------------------------------------------------------------------------
// Forms
// ------------------------------------------------------------------------------------------------

static void Boss07_Player_SetForm(Boss07* this, PlayState* play, u8 form, s32 transform) {
    f32 scale = Boss07_Player_GetBattleScale(form) * Boss07_Player_GetSizeRatio(form);
    s32 i;

    Boss07_Player_DestroyFormColliders(this, play);
    Boss07_Player_ClearHits(this);
    this->playerWalkPhase = 0.0f;
    this->playerWalkWeight = 0.0f;

    this->playerForm = form;
    this->playerNextAttack = 0;
    this->playerCarry = false;
    this->playerHeldTimer = 0;
    this->maskShakeTimer = 0;
    this->damagedFlashTimer = 0;
    this->disableCollisionTimer = 0;
    this->whipCrackTimer = 0;
    this->actor.speed = 0.0f;
    this->actor.velocity.y = 0.0f;
    this->actor.shape.rot.x = this->actor.shape.rot.z = 0;
    this->actor.world.rot.x = this->actor.world.rot.z = 0;
    this->angularVelocity = 0;

    // Everything the vanilla draws read, at its resting value
    this->incarnationArmScale = this->incarnationLegScale = 1.0f;
    this->incarnationMaskScaleX = this->incarnationMaskScaleY = 1.0f;
    for (i = 0; i < MAJORAS_INCARNATION_GROW_BODYPART_MAX; i++) {
        this->incarnationIntroBodyPartsScale[i] = 1.0f;
    }
    this->wrathLeanRotX = this->wrathLeanRotY = 0.0f;
    this->cutsceneHeadRot.x = this->cutsceneHeadRot.y = this->cutsceneHeadRot.z = 0;
    this->cutsceneState = 0;
    this->shockOrbScale = this->shockSparkScale = 0.0f;
    this->deathOrbScale = 0.0f;
    this->drawDmgEffScale = this->drawDmgEffFrozenSteamScale = this->drawDmgEffAlpha = 0.0f;
    this->drawDmgEffType = 0;
    this->drawDmgEffState = 0;
    this->drawDmgEffTimer = 0;
    this->maxDecayPixels = 0;
    this->whipTopIndex = 0;
    this->whipWrapStartIndex = 0;
    this->whipWrapEndOffset = 0;
    this->beamBaseScale = this->beamLengthScale = this->eyeBeamsLengthScale = this->eyeBeamsFocusOrbScale = 0.0f;
    this->reflectedBeamLengthScale = 0.0f;

    Actor_SetScale(&this->actor, scale);
    Collider_InitAndSetCylinder(play, &this->generalCollider, &this->actor, &sPlayerMaskAttackCylinderInit);
    this->playerColliders |= MAJORA_PLAYER_COLLIDER_CYLINDER;

    switch (form) {
        case MAJORA_PLAYER_FORM_INCARNATION:
            ActorShape_Init(&this->actor.shape, 0.0f, ActorShadow_DrawCircle, 80.0f);
            SkelAnime_InitFlex(play, &this->skelAnime, &gMajorasIncarnationSkel, &gMajorasIncarnationStationaryAnim,
                               this->jointTable, this->morphTable, MAJORAS_INCARNATION_LIMB_MAX);
            // Its draw positions these spheres; they're never checked
            Collider_InitAndSetJntSph(play, &this->bodyCollider, &this->actor, &sIncarnationBodyColliderJntSphInit,
                                      this->bodyColliderElements);
            this->bodyCollider.base.atFlags = AT_NONE;
            this->bodyCollider.base.acFlags = AC_NONE;
            this->bodyCollider.base.ocFlags1 = OC1_NONE;
            this->playerColliders |= MAJORA_PLAYER_COLLIDER_BODY;
            Boss07_Player_SetupIncarnationMove(this, play);
            break;

        case MAJORA_PLAYER_FORM_WRATH:
            ActorShape_Init(&this->actor.shape, 0.0f, ActorShadow_DrawCircle, 70.0f);
            SkelAnime_InitFlex(play, &this->skelAnime, &gMajorasWrathSkel, &gMajorasWrathIdleAnim, this->jointTable,
                               this->morphTable, MAJORAS_WRATH_LIMB_MAX);
            // Its draw positions the body spheres; they're never checked. The whips hit through `playerHitCollider`.
            Collider_InitAndSetJntSph(play, &this->bodyCollider, &this->actor, &sWrathBodyColliderJntSphInit,
                                      this->bodyColliderElements);
            this->bodyCollider.base.atFlags = AT_NONE;
            this->bodyCollider.base.acFlags = AC_NONE;
            this->bodyCollider.base.ocFlags1 = OC1_NONE;
            Collider_InitAndSetJntSph(play, &this->kickCollider, &this->actor, &sWrathKickColliderJntSphInit,
                                      this->kickColliderElements);
            Boss07_Player_MakeAttackJntSph(&this->kickCollider, 4);
            this->playerColliders |= MAJORA_PLAYER_COLLIDER_BODY | MAJORA_PLAYER_COLLIDER_KICK;

            if (sWhipLength == 0) {
                sWhipLength = MAJORA_WHIP_LENGTH;
            }
            this->whipLengthScale = scale / MAJORA_PLAYER_WRATH_BATTLE_SCALE;
            // Its own shadow texture assumes the arena's floor height; a round shadow instead
            this->disableShadow = true;
            for (i = 0; i < MAJORA_WHIP_LENGTH_MAX; i++) {
                this->rightWhip.pos[i] = this->leftWhip.pos[i] = this->actor.world.pos;
                this->rightWhip.velocity[i] = this->leftWhip.velocity[i] = gZeroVec3f;
            }
            Boss07_Player_SetupWrathMove(this, play);
            break;

        default:
            ActorShape_Init(&this->actor.shape, 0.0f, ActorShadow_DrawCircle, 15.0f);
            SkelAnime_Init(play, &this->skelAnime, &gMajorasMaskSkel, &gMajorasMaskFloatingAnim, this->jointTable,
                           this->morphTable, MAJORAS_MASK_LIMB_MAX);
            this->tentacleLengthScale = scale / MAJORA_PLAYER_MASK_BATTLE_SCALE;
            this->maskEyeTexIndex = MAJORAS_MASK_EYE_NORMAL;
            this->tentacleState = MAJORAS_MASK_TENTACLE_STATE_DEFAULT;
            // The fire beam burns a trail on the floor
            Effect_Add(play, &this->effectIndex, EFFECT_TIRE_MARK, 0, 0, &sPlayerBeamTireMarkInit);
            this->playerColliders |= MAJORA_PLAYER_COLLIDER_TIRE_MARK;
            this->beamTireMarkEnabled = this->prevBeamTireMarkEnabled = false;
            Boss07_Player_SetupFloat(this, play);
            break;
    }

    this->actor.shape.shadowAlpha = 160;
    Boss07_Player_SnapToLink(this, play);

    if (transform) {
        this->playerFormTimer = MAJORA_PLAYER_FORM_GROW_FRAMES;
        Actor_Spawn(&play->actorCtx, play, ACTOR_EN_CLEAR_TAG, this->actor.world.pos.x,
                    this->actor.world.pos.y + ((form == MAJORA_PLAYER_FORM_MASK) ? 0.0f : 40.0f),
                    this->actor.world.pos.z, 0, 0, Rand_ZeroFloat(ARRAY_COUNT(sProjectileEnvColors) - 0.01f),
                    CLEAR_TAG_PARAMS(CLEAR_TAG_LARGE_LIGHT_RAYS));

        switch (form) {
            case MAJORA_PLAYER_FORM_INCARNATION:
                Actor_PlaySfx(&this->actor, NA_SE_EN_LAST2_GROW_HEAD_OLD);
                Actor_PlaySfx(&this->actor, NA_SE_EN_LAST2_VOICE_SURPRISED_OLD);
                break;
            case MAJORA_PLAYER_FORM_WRATH:
                Actor_PlaySfx(&this->actor, NA_SE_EN_LAST2_PUMP_UP_OLD);
                Actor_PlaySfx(&this->actor, NA_SE_EN_LAST3_VOICE_LAUGH_OLD);
                break;
            default:
                Actor_PlaySfx(&this->actor, NA_SE_EN_LAST1_ATTACK_OLD);
                break;
        }
    } else {
        this->playerFormTimer = 0;
    }
}

// The new form grows in: Incarnation's limbs sprout from the mask, Wrath stretches out, the mask unfolds
static void Boss07_Player_UpdateGrow(Boss07* this) {
    f32 t;
    s32 i;

    if (this->playerFormTimer == 0) {
        return;
    }
    this->playerFormTimer--;

    t = 1.0f - ((f32)this->playerFormTimer / MAJORA_PLAYER_FORM_GROW_FRAMES);
    t = SQ(t) * (3.0f - (2.0f * t));

    switch (this->playerForm) {
        case MAJORA_PLAYER_FORM_INCARNATION:
            for (i = 0; i < MAJORAS_INCARNATION_GROW_BODYPART_MAX; i++) {
                this->incarnationIntroBodyPartsScale[i] = t;
            }
            break;

        case MAJORA_PLAYER_FORM_WRATH:
            this->incarnationArmScale = 0.2f + (0.8f * t);
            Actor_SetScale(&this->actor, this->actor.scale.x * (0.4f + (0.6f * t)));
            break;

        default:
            Actor_SetScale(&this->actor, this->actor.scale.x * t);
            break;
    }
}

static void Boss07_Player_TryChangeForm(Boss07* this, PlayState* play, Input* input) {
    u8 newForm;

    if (CHECK_BTN_ALL(input->press.button, BTN_DRIGHT)) {
        newForm = (this->playerForm == MAJORA_PLAYER_FORM_INCARNATION) ? MAJORA_PLAYER_FORM_MASK
                                                                        : MAJORA_PLAYER_FORM_INCARNATION;
    } else if (CHECK_BTN_ALL(input->press.button, BTN_DLEFT)) {
        newForm = (this->playerForm == MAJORA_PLAYER_FORM_WRATH) ? MAJORA_PLAYER_FORM_MASK : MAJORA_PLAYER_FORM_WRATH;
    } else {
        return;
    }

    Boss07_Player_SetForm(this, play, newForm, true);
}

// ------------------------------------------------------------------------------------------------
// Init
// ------------------------------------------------------------------------------------------------

void Boss07_Player_Init(Boss07* this, PlayState* play) {
    // Not an enemy or a boss: not targetable, no boss music, no "enemies nearby" checks
    Actor_ChangeCategory(play, &play->actorCtx, &this->actor, ACTORCAT_MISC);
    this->actor.flags &= ~(ACTOR_FLAG_ATTENTION_ENABLED | ACTOR_FLAG_HOSTILE);
    // Keeps following Link while he talks, dies or gets an item (misc actors normally freeze then)
    this->actor.flags |= ACTOR_FLAG_FREEZE_EXCEPTION;
    // Kept when Link moves to another room
    this->actor.room = -1;
    this->actor.gravity = 0.0f;
    this->actor.colChkInfo.mass = MASS_IMMOVABLE;

    this->actor.update = Boss07_Player_Update;
    this->actor.draw = Boss07_Player_Draw;

    this->playerHidden = true;
    this->playerPrevInvincibility = 0;
    this->playerColliders = 0;
    this->playerFloorY = GET_PLAYER(play)->actor.floorHeight;
    this->playerDanceIndex = 0;
    Boss07_Player_InitHitCollider(this, play);

    // A new area starts in the form Majora was in (home.rot.z carries it over from PlayAsMajora.cpp)
    Boss07_Player_SetForm(this, play, CLAMP(this->actor.home.rot.z, 0, MAJORA_PLAYER_FORM_WRATH), false);
}

// ------------------------------------------------------------------------------------------------
// Majora's Mask
// ------------------------------------------------------------------------------------------------

void Boss07_Player_SetupFloat(Boss07* this, PlayState* play) {
    this->actionFunc = Boss07_Player_Float;
    Animation_MorphToLoop(&this->skelAnime, &gMajorasMaskFloatingAnim, -10.0f);
    this->angularVelocity = 0;
    this->actor.speed = 0.0f;
}

void Boss07_Player_Float(Boss07* this, PlayState* play) {
    Player* player = GET_PLAYER(play);
    s16 lean;

    SkelAnime_Update(&this->skelAnime);
    Boss07_Player_Follow(this, play);

    // Faces where Link faces, leans into his movement and bobs a little
    Math_ApproachS(&this->actor.shape.rot.y, player->actor.shape.rot.y, 3, 0x1800);
    this->actor.world.rot.y = this->actor.shape.rot.y;
    lean = CLAMP_MAX((s16)(Boss07_Player_MovedXZ(this) * 0x200), 0x1800);
    Math_ApproachS(&this->actor.shape.rot.x, lean + (s16)(Math_SinS(this->frameCounter * 0x1388) * 0x300), 5,
                   0x400);
    Math_ApproachS(&this->actor.shape.rot.z, (s16)(Math_SinS(this->frameCounter * 0x700) * 0x400), 5, 0x200);
}

void Boss07_Player_SetupSpinAttack(Boss07* this, PlayState* play) {
    this->actionFunc = Boss07_Player_SpinAttack;
    this->subAction = MAJORA_PLAYER_SPIN_WIND_UP;
    this->timers[0] = 10;
    this->angularVelocity = 0;
    this->speedToTarget = 0.0f;
    Boss07_Player_UpdateAim(this, play);
    Actor_PlaySfx(&this->actor, NA_SE_EN_LAST1_ATTACK_OLD);
}

void Boss07_Player_SpinAttack(Boss07* this, PlayState* play) {
    Player* player = GET_PLAYER(play);
    Vec3f hoverPos;
    f32 dist;

    this->actor.shape.rot.z -= this->angularVelocity;
    Boss07_Player_GetHoverPos(this, play, &hoverPos);

    switch (this->subAction) {
        case MAJORA_PLAYER_SPIN_WIND_UP:
            // Turns toward the target, lies flat and spins up next to Link
            Boss07_Player_Follow(this, play);
            Math_ApproachS(&this->actor.shape.rot.y, Boss07_Player_GetAimYaw(this), 2, 0x2000);
            Math_ApproachS(&this->actor.shape.rot.x, -0x4000, 2, 0x1000);
            Math_ApproachS(&this->angularVelocity, 0x2000, 1, 0x400);

            if (this->timers[0] == 0) {
                this->subAction = MAJORA_PLAYER_SPIN_OUT;
                this->actor.world.rot.y = Boss07_Player_GetAimYaw(this);
                this->actor.world.rot.x = 0;
                this->actor.speed = 8.0f;
                this->timers[0] = 45;
                Actor_PlaySfx(&this->actor, NA_SE_EN_LAST1_ATTACK_2ND_OLD);
            }
            break;

        case MAJORA_PLAYER_SPIN_OUT:
            // Keeps homing in on the target while it's alive
            if ((this->playerTarget != NULL) && (this->playerTarget->update != NULL)) {
                this->playerAimPos = this->playerTarget->focus.pos;
            }

            Actor_PlaySfx(&this->actor, NA_SE_EN_LAST1_ROLLING_OLD - SFX_FLAG);
            Math_ApproachS(&this->actor.shape.rot.x, -0x4000, 2, 0x1000);
            Math_ApproachS(&this->angularVelocity, 0x2000, 1, 0x200);
            dist = Boss07_Player_FlyTowards(this, &this->playerAimPos, MAJORA_PLAYER_SPIN_SPEED, 0xC00);

            if ((dist < 30.0f) || (this->timers[0] == 0) || (this->actor.bgCheckFlags & BGCHECKFLAG_WALL) ||
                (Math_Vec3f_DistXYZ(&this->actor.world.pos, &player->actor.world.pos) > MAJORA_PLAYER_SPIN_RANGE)) {
                this->subAction = MAJORA_PLAYER_SPIN_RETURN;
                this->timers[0] = 60;
            }
            break;

        case MAJORA_PLAYER_SPIN_RETURN:
            Actor_PlaySfx(&this->actor, NA_SE_EN_LAST1_ROLLING_OLD - SFX_FLAG);
            dist = Boss07_Player_FlyTowards(this, &hoverPos, MAJORA_PLAYER_SPIN_SPEED + 4.0f, 0x1800);

            // Back with Link (or taking too long: it just appears there)
            if ((dist < 35.0f) || (this->timers[0] == 0)) {
                if (this->timers[0] == 0) {
                    Boss07_Player_SnapToLink(this, play);
                }
                this->subAction = MAJORA_PLAYER_SPIN_END;
                this->actor.speed = 0.0f;
                this->timers[0] = 10;
            }
            break;

        case MAJORA_PLAYER_SPIN_END:
            // Spins down and stands up again
            Boss07_Player_Follow(this, play);
            Math_ApproachS(&this->angularVelocity, 0, 1, 0x380);
            Math_ApproachS(&this->actor.shape.rot.x, 0, 2, 0x900);
            Math_ApproachS(&this->actor.shape.rot.y, player->actor.shape.rot.y, 3, 0x1800);

            if (this->timers[0] == 0) {
                Boss07_Player_SetupFloat(this, play);
            }
            break;

        default:
            break;
    }
}

// The beam's direction from the mask's facing: the same rotations its draw uses
static void Boss07_Player_GetBeamDir(Boss07* this, Vec3f* dir) {
    Matrix_RotateYS(this->actor.shape.rot.y, MTXMODE_NEW);
    Matrix_RotateXS(this->actor.shape.rot.x, MTXMODE_APPLY);
    Matrix_MultVecZ(1.0f, dir);
}

void Boss07_Player_SetupFireBeam(Boss07* this, PlayState* play) {
    Player* player = GET_PLAYER(play);

    this->actionFunc = Boss07_Player_FireBeam;
    this->subAction = MAJORAS_MASK_FIRE_BEAM_SUB_ACTION_CHARGE_UP;
    this->timers[0] = 12;
    this->timers[1] = 0;
    this->speedToTarget = 0.0f;
    Boss07_Mask_StopBeam(this);
    Boss07_Player_UpdateAim(this, play);
    this->playerBeamYaw = player->actor.shape.rot.y;
    this->playerBeamPitch = 0x800; // a little down, toward the floor ahead
    Audio_PlaySfx_AtPos(&sMajoraSfxPos, NA_SE_EN_LAST1_BLOW_OLD);
}

void Boss07_Player_FireBeam(Boss07* this, PlayState* play) {
    Input* input = CONTROLLER1(&play->state);
    s32 held = CHECK_BTN_ANY(input->cur.button, BTN_B | BTN_CRIGHT);
    f32 scale = this->actor.scale.x;
    s16 targetYaw;
    s16 targetPitch;
    Vec3f dir;
    Vec3f end;
    Vec3f hitPos;
    CollisionPoly* poly;
    s32 bgId;
    f32 length;
    s32 i;

    Boss07_Player_Follow(this, play);
    this->tentacleState = MAJORAS_MASK_TENTACLE_STATE_FIRING_BEAM;
    Math_ApproachS(&this->actor.shape.rot.z, 0, 0xA, 0x400);

    // Aims at a living target; with nothing targeted the stick steers it
    if ((this->playerTarget != NULL) && (this->playerTarget->update != NULL)) {
        f32 dx = this->playerTarget->focus.pos.x - this->actor.world.pos.x;
        f32 dy = this->playerTarget->focus.pos.y - this->actor.world.pos.y;
        f32 dz = this->playerTarget->focus.pos.z - this->actor.world.pos.z;

        targetYaw = Math_Atan2S(dx, dz);
        targetPitch = -Math_Atan2S(dy, sqrtf(SQ(dx) + SQ(dz)));
        this->playerBeamYaw = targetYaw;
        this->playerBeamPitch = targetPitch;
    } else {
        this->playerBeamYaw -= input->cur.stick_x * 0x18;
        this->playerBeamPitch -= input->cur.stick_y * 0x10;
        this->playerBeamPitch = CLAMP(this->playerBeamPitch, -0x2000, 0x3000);
        targetYaw = this->playerBeamYaw;
        targetPitch = this->playerBeamPitch;
    }
    Math_ApproachS(&this->actor.shape.rot.y, targetYaw, 3, 0x1000);
    Math_ApproachS(&this->actor.shape.rot.x, targetPitch, 3, 0x1000);
    this->actor.world.rot.y = this->actor.shape.rot.y;

    switch (this->subAction) {
        case MAJORAS_MASK_FIRE_BEAM_SUB_ACTION_CHARGE_UP:
            if (this->timers[0] == 0) {
                this->subAction = MAJORAS_MASK_FIRE_BEAM_SUB_ACTION_FIRE_EYE_BEAMS;
                this->timers[0] = 6;
                this->beamBaseScale = 1.0f;
            }
            break;

        case MAJORAS_MASK_FIRE_BEAM_SUB_ACTION_FIRE_EYE_BEAMS:
            Math_ApproachF(&this->eyeBeamsLengthScale, 1.0f, 1.0f, 0.2f);
            if (this->timers[0] == 0) {
                this->subAction = MAJORAS_MASK_FIRE_BEAM_SUB_ACTION_GROW_FOCUS_LIGHT_ORB;
                this->timers[0] = 8;
            }
            break;

        case MAJORAS_MASK_FIRE_BEAM_SUB_ACTION_GROW_FOCUS_LIGHT_ORB:
            Audio_PlaySfx(NA_SE_EN_LAST1_BEAM_OLD - SFX_FLAG);
            Math_ApproachF(&this->eyeBeamsFocusOrbScale, 1.0f, 0.2f, 0.2f);
            if (this->timers[0] == 0) {
                this->subAction = MAJORAS_MASK_FIRE_BEAM_SUB_ACTION_BEAM_ACTIVE;
                this->timers[0] = 150; // the longest it lasts
                this->timers[1] = 30;  // the shortest
            }
            break;

        case MAJORAS_MASK_FIRE_BEAM_SUB_ACTION_BEAM_ACTIVE:
        case MAJORAS_MASK_FIRE_BEAM_SUB_ACTION_END:
            Math_ApproachF(&this->eyeBeamsFocusOrbScale, 1.0f, 0.2f, 0.2f);

            // The beam runs from the mask's mouth to the first wall or floor
            Boss07_Player_GetBeamDir(this, &dir);
            this->beamStartPos.x = this->actor.world.pos.x + (dir.x * 1150.0f * scale);
            this->beamStartPos.y = this->actor.world.pos.y + (dir.y * 1150.0f * scale);
            this->beamStartPos.z = this->actor.world.pos.z + (dir.z * 1150.0f * scale);
            end.x = this->beamStartPos.x + (dir.x * MAJORA_PLAYER_BEAM_RANGE);
            end.y = this->beamStartPos.y + (dir.y * MAJORA_PLAYER_BEAM_RANGE);
            end.z = this->beamStartPos.z + (dir.z * MAJORA_PLAYER_BEAM_RANGE);

            if (BgCheck_EntityLineTest1(&play->colCtx, &this->beamStartPos, &end, &hitPos, &poly, true, true, true,
                                        true, &bgId)) {
                length = Math_Vec3f_DistXYZ(&this->beamStartPos, &hitPos);

                if (this->subAction == MAJORAS_MASK_FIRE_BEAM_SUB_ACTION_BEAM_ACTIVE) {
                    // Burns a trail on floors, sparks on walls
                    if (COLPOLY_GET_NORMAL(poly->normal.y) > 0.5f) {
                        func_800AE930(&play->colCtx, Effect_GetByIndex(this->effectIndex), &hitPos, 15.0f,
                                      Math_Atan2S(this->prevBeamTireMarkPos.x - hitPos.x,
                                                  this->prevBeamTireMarkPos.z - hitPos.z),
                                      poly, bgId);
                        this->beamTireMarkEnabled = true;
                    }
                    this->prevBeamTireMarkPos = hitPos;

                    if ((this->frameCounter % 6) == 0) {
                        Actor_Spawn(&play->actorCtx, play, ACTOR_EN_CLEAR_TAG, hitPos.x, hitPos.y, hitPos.z, 0, 0,
                                    1, CLEAR_TAG_PARAMS(CLEAR_TAG_SMALL_LIGHT_RAYS));
                    }
                }
            } else {
                length = MAJORA_PLAYER_BEAM_RANGE;
            }

            // Its draw makes it 201 x scale x beamLengthScale long
            Math_ApproachF(&this->beamLengthScale, length / (201.0f * scale), 1.0f, 40.0f);

            if (this->subAction == MAJORAS_MASK_FIRE_BEAM_SUB_ACTION_BEAM_ACTIVE) {
                f32 reach = CLAMP_MAX(this->beamLengthScale * 201.0f * scale, length);
                f32 radius = (25.0f * Boss07_Player_GetSizeRatio(MAJORA_PLAYER_FORM_MASK)) + 15.0f;

                Audio_PlaySfx(NA_SE_EN_LAST1_BEAM_OLD - SFX_FLAG);

                // Hits all along it
                for (i = 0; i < MAJORA_PLAYER_BEAM_SPHERES; i++) {
                    Vec3f pos;
                    f32 d = reach * (i + 0.5f) / MAJORA_PLAYER_BEAM_SPHERES;

                    pos.x = this->beamStartPos.x + (dir.x * d);
                    pos.y = this->beamStartPos.y + (dir.y * d);
                    pos.z = this->beamStartPos.z + (dir.z * d);
                    Boss07_Player_SetHit(this, i, &pos, radius, 2);
                }

                // Held as long as the button is
                if ((this->timers[0] == 0) || (!held && (this->timers[1] == 0))) {
                    this->subAction = MAJORAS_MASK_FIRE_BEAM_SUB_ACTION_END;
                    this->timers[0] = 20;
                }
            } else {
                Math_ApproachZeroF(&this->beamBaseScale, 1.0f, 0.05f);
                if (this->timers[0] == 0) {
                    Boss07_Mask_StopBeam(this);
                    Boss07_Player_SetupFloat(this, play);
                }
            }
            break;

        default:
            break;
    }
}

static s32 Boss07_Player_IsMaskFlying(Boss07* this) {
    return (this->actionFunc == Boss07_Player_SpinAttack) &&
           ((this->subAction == MAJORA_PLAYER_SPIN_OUT) || (this->subAction == MAJORA_PLAYER_SPIN_RETURN));
}

// ------------------------------------------------------------------------------------------------
// Majora's Incarnation
// ------------------------------------------------------------------------------------------------

void Boss07_Player_SetupIncarnationMove(Boss07* this, PlayState* play) {
    this->actionFunc = Boss07_Player_IncarnationMove;
    this->playerCarry = false;
    Animation_MorphToLoop(&this->skelAnime, &gMajorasIncarnationStationaryAnim, -5.0f);
    this->subAction = false; // running
    this->skelAnime.playSpeed = 1.0f;
}

void Boss07_Player_IncarnationMove(Boss07* this, PlayState* play) {
    Player* player = GET_PLAYER(play);
    f32 moved;
    s32 running;

    Boss07_Player_Follow(this, play);
    moved = Boss07_Player_MovedXZ(this);
    // Only used for the dust, the movement is Link's
    this->actor.speed = moved;
    running = moved > 1.5f;

    if (running != this->subAction) {
        this->subAction = running;
        Animation_MorphToLoop(&this->skelAnime,
                              running ? (AnimationHeader*)&gMajorasIncarnationRunAnim
                                      : (AnimationHeader*)&gMajorasIncarnationStationaryAnim,
                              -3.0f);
    }

    if (running) {
        this->skelAnime.playSpeed = CLAMP(moved / 6.0f, 0.6f, 2.0f);
        Actor_PlaySfx(&this->actor, NA_SE_EN_LAST2_WALK_OLD - SFX_FLAG);
        this->miscTimer++;
        if (this->miscTimer >= 4) {
            Actor_PlaySfx(&this->actor, NA_SE_EN_LAST2_WALK2_OLD);
            this->miscTimer = 0;
        }
        Boss07_Incarnation_SpawnDust(this, play, 3, MAJORAS_INCARNATION_DUST_SPAWN_POS_FEET);

        // Afterimages at full speed
        if (moved > 7.0f) {
            this->fireTimer = 5;
        }
    } else {
        this->skelAnime.playSpeed = 1.0f;
    }

    SkelAnime_Update(&this->skelAnime);
    Math_ApproachS(&this->actor.shape.rot.y, player->actor.shape.rot.y, 2, 0x2000);
    this->actor.world.rot.y = this->actor.shape.rot.y;
}

void Boss07_Player_SetupIncarnationAttack(Boss07* this, PlayState* play) {
    this->actionFunc = Boss07_Player_IncarnationAttack;
    Animation_MorphToLoop(&this->skelAnime, &gMajorasIncarnationEnergyBallAttackAnim, -3.0f);
    this->animEndFrame = Animation_GetLastFrame(&gMajorasIncarnationEnergyBallAttackAnim);
    this->skelAnime.playSpeed = 1.0f;
    this->actor.speed = 0.0f;
    Boss07_Player_UpdateAim(this, play);
}

static void Boss07_Player_SpawnEnergyBall(Boss07* this, PlayState* play, Vec3f* hand) {
    Boss07* ball = (Boss07*)Actor_Spawn(&play->actorCtx, play, ACTOR_BOSS_07, hand->x, hand->y, hand->z, 0,
                                        this->actor.shape.rot.y, 0, MAJORA_PARAMS(MAJORA_TYPE_PLAYER_PROJECTILE));

    if (ball != NULL) {
        ball->playerAimPos = this->playerAimPos;
        ball->playerTarget = this->playerTarget;
        // Smaller on a smaller Incarnation
        ball->speedToTarget = Boss07_Player_GetSizeRatio(MAJORA_PLAYER_FORM_INCARNATION);
    }
}

void Boss07_Player_IncarnationAttack(Boss07* this, PlayState* play) {
    Input* input = CONTROLLER1(&play->state);

    SkelAnime_Update(&this->skelAnime);
    Boss07_Player_Follow(this, play);
    Boss07_Player_UpdateAim(this, play);
    Math_ApproachS(&this->actor.shape.rot.y, Boss07_Player_GetAimYaw(this), 3, 0x3000);
    this->actor.world.rot.y = this->actor.shape.rot.y;

    if (Animation_OnFrame(&this->skelAnime, 4.0f)) {
        Boss07_Player_SpawnEnergyBall(this, play, &this->incarnationLeftHandPos);
    }
    if (Animation_OnFrame(&this->skelAnime, 9.0f)) {
        Boss07_Player_SpawnEnergyBall(this, play, &this->incarnationRightHandPos);
    }

    // Keeps firing while B (or C-Left) is held
    if (Animation_OnFrame(&this->skelAnime, this->animEndFrame) &&
        !CHECK_BTN_ANY(input->cur.button, BTN_B | BTN_CLEFT)) {
        Boss07_Player_SetupIncarnationMove(this, play);
    }

    Boss07_Incarnation_SpawnDust(this, play, 1, MAJORAS_INCARNATION_DUST_SPAWN_POS_FEET);
}

void Boss07_Player_SetupIncarnationDance(Boss07* this, PlayState* play) {
    static AnimationHeader* sDanceAnims[MAJORA_PLAYER_DANCE_MAX] = {
        &gMajorasIncarnationSquattingDanceAnim,
        &gMajorasIncarnationMoonwalkAnim,
        &gMajorasIncarnationPirouetteAnim,
        &gMajorasIncarnationTauntDance2Anim,
    };

    this->actionFunc = Boss07_Player_IncarnationDance;
    // The dances come in turn
    this->subAction = this->playerDanceIndex % MAJORA_PLAYER_DANCE_MAX;
    this->playerDanceIndex = (this->subAction + 1) % MAJORA_PLAYER_DANCE_MAX;
    Animation_MorphToLoop(&this->skelAnime, sDanceAnims[this->subAction], -5.0f);
    this->skelAnime.playSpeed = 1.0f;
    this->timers[0] = (this->subAction == MAJORA_PLAYER_DANCE_MOONWALK) ? 120 : 90;
    this->cutsceneTimer = 0;
    this->actor.speed = 0.0f;
}

// The moonwalk slides backwards, taking Link along (walls stop him). The stick picks the way it slides.
static void Boss07_Player_Moonwalk(Boss07* this, PlayState* play) {
    Player* player = GET_PLAYER(play);
    Input* input = CONTROLLER1(&play->state);
    f32 stickMag;
    s16 stickAngle;
    f32 speed = MAJORA_PLAYER_MOONWALK_SPEED * (Boss07_Player_GetSizeRatio(MAJORA_PLAYER_FORM_INCARNATION) / 0.6f);

    Lib_GetControlStickData(&stickMag, &stickAngle, input);
    if (stickMag > 15.0f) {
        s16 slideYaw = Camera_GetInputDirYaw(GET_ACTIVE_CAM(play)) + stickAngle;

        // Faces away from where it slides
        Math_ApproachS(&this->actor.shape.rot.y, slideYaw + 0x8000, 3, 0x800);
    }

    // Link was moved here (and stopped by walls) since last frame
    this->actor.world.pos = player->actor.world.pos;
    this->actor.world.pos.x -= Math_SinS(this->actor.shape.rot.y) * speed;
    this->actor.world.pos.z -= Math_CosS(this->actor.shape.rot.y) * speed;
    this->actor.world.rot.y = this->actor.shape.rot.y;
    this->actor.speed = speed; // for the dust
    this->playerCarry = true;
}

void Boss07_Player_IncarnationDance(Boss07* this, PlayState* play) {
    Player* player = GET_PLAYER(play);

    SkelAnime_Update(&this->skelAnime);
    if (this->subAction == MAJORA_PLAYER_DANCE_MOONWALK) {
        Boss07_Player_Moonwalk(this, play);
    } else {
        Boss07_Player_Follow(this, play);
    }

    switch (this->subAction) {
        case MAJORA_PLAYER_DANCE_SQUATTING:
            if (Animation_OnFrame(&this->skelAnime, 5.0f)) {
                Actor_PlaySfx(&this->actor, ((this->cutsceneTimer % 2) == 0) ? NA_SE_EN_LAST2_VOICE_UAUOO1_OLD
                                                                               : NA_SE_EN_LAST2_VOICE_UAUOO2_OLD);
                Actor_PlaySfx(&this->actor, NA_SE_EN_LAST2_JUMP_OLD);
                this->cutsceneTimer++;
            }
            break;

        case MAJORA_PLAYER_DANCE_MOONWALK:
            Actor_PlaySfx(&this->actor, NA_SE_EN_LAST2_MOONWALK_OLD - SFX_FLAG);
            break;

        case MAJORA_PLAYER_DANCE_PIROUETTE:
            Actor_PlaySfx(&this->actor, NA_SE_EN_LAST2_BALLET_OLD - SFX_FLAG);
            this->actor.shape.rot.y += 0x2000;
            break;

        default:
            Actor_PlaySfx(&this->actor, NA_SE_EN_LAST2_WAIT_OLD - SFX_FLAG);
            break;
    }

    if ((this->subAction != MAJORA_PLAYER_DANCE_PIROUETTE) && (this->subAction != MAJORA_PLAYER_DANCE_MOONWALK)) {
        Math_ApproachS(&this->actor.shape.rot.y, player->actor.shape.rot.y, 3, 0x1000);
    }

    if (this->timers[0] == 0) {
        Boss07_Player_SetupIncarnationMove(this, play);
        // Link faces where Incarnation ended up facing
        player->actor.shape.rot.y = player->actor.world.rot.y = player->yaw = this->actor.shape.rot.y;
    }

    Boss07_Incarnation_SpawnDust(this, play, 7, MAJORAS_INCARNATION_DUST_SPAWN_POS_FEET);
}

static void Boss07_Player_UpdateAfterimages(Boss07* this, PlayState* play) {
    s32 i;

    // While running, `fireTimer` keeps afterimages spawning every other frame, like the boss
    if ((this->fireTimer != 0) && !(this->frameCounter & 1)) {
        Boss07* afterimage =
            (Boss07*)Actor_SpawnAsChild(&play->actorCtx, &this->actor, play, ACTOR_BOSS_07, this->actor.world.pos.x,
                                        this->actor.world.pos.y, this->actor.world.pos.z, this->actor.shape.rot.x,
                                        this->actor.shape.rot.y, 7, MAJORA_PARAMS(MAJORA_TYPE_INCARNATION_AFTERIMAGE));

        if (afterimage != NULL) {
            for (i = 0; i < MAJORAS_INCARNATION_LIMB_MAX; i++) {
                afterimage->skelAnime.jointTable[i] = this->skelAnime.jointTable[i];
            }
        }
    }
}

// ------------------------------------------------------------------------------------------------
// Majora's Wrath
// ------------------------------------------------------------------------------------------------

// Hip swing in binangs: a short step when walking slowly, a long stride when running
static f32 Boss07_Player_GetWalkAmplitude(f32 moved) {
    return 0x900 + (CLAMP(moved / 10.0f, 0.0f, 1.0f) * 0x900);
}

static void Boss07_Player_SetLooseWhips(Boss07* this) {
    this->rightWhip.mobility = this->leftWhip.mobility = 0.7f;
    this->rightWhip.gravity = this->leftWhip.gravity = -15.0f;
    this->rightWhip.deceleration = this->leftWhip.deceleration = 2.0f;
    this->rightWhip.tension = this->leftWhip.tension = 0.0f;
}

void Boss07_Player_SetupWrathMove(Boss07* this, PlayState* play) {
    this->actionFunc = Boss07_Player_WrathMove;
    Animation_MorphToLoop(&this->skelAnime, &gMajorasWrathIdleAnim, -10.0f);
    // The vanilla idle makes Wrath targetable
    this->actor.flags &= ~ACTOR_FLAG_ATTENTION_ENABLED;
    this->playerCarry = false;
    this->actor.speed = 0.0f;
}

void Boss07_Player_WrathMove(Boss07* this, PlayState* play) {
    Player* player = GET_PLAYER(play);
    f32 moved;

    SkelAnime_Update(&this->skelAnime);
    Boss07_Player_Follow(this, play);
    moved = Boss07_Player_MovedXZ(this);
    Boss07_Player_SetLooseWhips(this);

    // Walks: the cycle advances with the distance covered, so the feet don't slide. One cycle is four foot swings
    // (each foot sweeps forward and back while the other one carries her).
    Math_ApproachF(&this->playerWalkWeight, (moved > 0.6f) ? 1.0f : 0.0f, 1.0f, 0.12f);
    if (moved > 0.01f) {
        f32 swing = MAJORA_PLAYER_WALK_FOOT_PER_BINANG * Boss07_Player_GetWalkAmplitude(moved) * this->actor.scale.x;

        this->playerWalkPhase += (moved / (4.0f * swing)) * 2.0f * M_PIf;
        if (this->playerWalkPhase > 2.0f * M_PIf) {
            this->playerWalkPhase -= 2.0f * M_PIf;
        }
        this->actor.speed = moved; // for the walk amplitude in the draw
    } else {
        this->actor.speed = 0.0f;
    }

    // Leans into the movement a little
    Math_ApproachF(&this->wrathLeanRotX, CLAMP_MAX(moved * 0.012f, 0.15f), 0.3f, 0.04f);
    Math_ApproachS(&this->actor.shape.rot.y, player->actor.shape.rot.y, 3, 0x1800);
    this->actor.world.rot.y = this->actor.shape.rot.y;

    // Footsteps
    if (this->playerWalkWeight > 0.5f) {
        f32 s0 = sinf(this->playerWalkPhase);

        if ((s0 * this->playerSwingSignPrev) < 0.0f) {
            Actor_PlaySfx(&this->actor, NA_SE_EN_LAST2_WALK2_OLD);
        }
        this->playerSwingSignPrev = s0;
    }
}

// Wrath's walk, on top of her idle animation (called from her limb draw)
void Boss07_Player_WrathWalkLimb(Boss07* this, s32 limbIndex, Vec3f* pos, Vec3s* rot) {
    f32 w = this->playerWalkWeight;
    f32 amp;
    f32 phaseL;
    f32 phaseR;

    if ((this->playerForm != MAJORA_PLAYER_FORM_WRATH) || (w <= 0.0f)) {
        return;
    }

    amp = Boss07_Player_GetWalkAmplitude(this->actor.speed) * w;
    phaseL = this->playerWalkPhase;
    phaseR = this->playerWalkPhase + M_PIf;

    switch (limbIndex) {
        // Hips: forward is negative z
        case MAJORAS_WRATH_LIMB_LEFT_THIGH:
            rot->z -= (s16)(amp * sinf(phaseL));
            break;
        case MAJORAS_WRATH_LIMB_RIGHT_THIGH:
            rot->z -= (s16)(amp * sinf(phaseR));
            break;

        // Knees bend while the leg swings forward, lifting the foot
        case MAJORAS_WRATH_LIMB_LEFT_LOWER_LEG_ROOT:
            rot->z += (s16)(amp * 1.2f * CLAMP_MIN(cosf(phaseL), 0.0f));
            break;
        case MAJORAS_WRATH_LIMB_RIGHT_LOWER_LEG_ROOT:
            rot->z += (s16)(amp * 1.2f * CLAMP_MIN(cosf(phaseR), 0.0f));
            break;

        // Arms swing against the legs: the right arm with the left leg. Right arm forward is negative y, left positive.
        case MAJORAS_WRATH_LIMB_RIGHT_UPPER_ARM:
            rot->y -= (s16)(amp * 0.5f * sinf(phaseL));
            break;
        case MAJORAS_WRATH_LIMB_LEFT_UPPER_ARM:
            rot->y += (s16)(amp * 0.5f * sinf(phaseR));
            break;

        // At the long ends of a stride both feet rise (the legs swing in arcs): the body dips there to keep them
        // on the floor
        case MAJORAS_WRATH_LIMB_ROOT:
            pos->y -= 450.0f * (amp / 0x1200) * SQ(sinf(phaseL));
            break;

        default:
            break;
    }
}

static void Boss07_Player_SetupWrathAttack(Boss07* this, PlayState* play, u8 attack) {
    Boss07_Player_UpdateAim(this, play);

    this->actionFunc = Boss07_Wrath_Attack;
    this->subAction = attack;
    // The vanilla idle between combo attacks makes Wrath targetable
    this->actor.flags &= ~ACTOR_FLAG_ATTENTION_ENABLED;
    this->timers[1] = 0;
    Animation_MorphToPlayOnce(&this->skelAnime, sPlayerWrathAttackAnims[attack], -5.0f);
    this->animEndFrame = Animation_GetLastFrame(sPlayerWrathAttackAnims[attack]);
    this->frameCounter = 0;
    this->playerNextAttack = 0;
    this->playerCarry = false;

    // The spin attack glides toward the target, carrying Link
    if (attack == MAJORAS_WRATH_ATTACK_SUB_ACTION_SPIN_ATTACK) {
        f32 dist = CLAMP(Math_Vec3f_DistXZ(&this->actor.world.pos, &this->playerAimPos), 100.0f, 350.0f);
        s16 yaw = Boss07_Player_GetAimYaw(this);

        this->targetPos.x = this->actor.world.pos.x + (Math_SinS(yaw) * dist);
        this->targetPos.y = this->actor.world.pos.y;
        this->targetPos.z = this->actor.world.pos.z + (Math_CosS(yaw) * dist);
        this->speedToTarget = 0.0f;
    }
}

// Runs around the vanilla `Boss07_Wrath_Attack`
static void Boss07_Player_UpdateWrathAttack(Boss07* this, PlayState* play) {
    Player* player = GET_PLAYER(play);
    Input* input = CONTROLLER1(&play->state);

    // Its attacks turn toward `yawTowardsPlayer`: point it at the target
    if ((this->playerTarget != NULL) && (this->playerTarget->update != NULL)) {
        this->playerAimPos = this->playerTarget->focus.pos;
    }
    this->actor.yawTowardsPlayer = Boss07_Player_GetAimYaw(this);

    this->playerCarry = (this->subAction == MAJORAS_WRATH_ATTACK_SUB_ACTION_SPIN_ATTACK) &&
                        (this->frameCounter > 16) && (this->frameCounter <= 40);

    if (this->playerCarry) {
        // Link was moved here (and stopped by walls) since last frame
        this->actor.world.pos.x = player->actor.world.pos.x;
        this->actor.world.pos.z = player->actor.world.pos.z;
        this->actor.world.pos.y = player->actor.world.pos.y;
    } else {
        Boss07_Player_Follow(this, play);
    }

    // Holding B through the start of the quick whip turns it into the flurry
    if ((this->subAction == MAJORAS_WRATH_ATTACK_SUB_ACTION_QUICK_WHIP) && (this->frameCounter < 10) &&
        CHECK_BTN_ALL(input->cur.button, BTN_B) && (this->playerHeldTimer >= 8)) {
        Boss07_Player_SetupWrathAttack(this, play, MAJORAS_WRATH_ATTACK_SUB_ACTION_FLURRY);
        Audio_PlaySfx_AtPos(&sMajoraSfxPos, NA_SE_EN_LAST3_VOICE_THROW_OLD);
        return;
    }

    // B again during the quick whip queues the three-hit combo, and the other way round
    if (CHECK_BTN_ALL(input->press.button, BTN_B) && (this->frameCounter >= 6)) {
        if (this->subAction == MAJORAS_WRATH_ATTACK_SUB_ACTION_QUICK_WHIP) {
            this->playerNextAttack = MAJORAS_WRATH_ATTACK_SUB_ACTION_THREE_HIT + 1;
        } else if (this->subAction == MAJORAS_WRATH_ATTACK_SUB_ACTION_THREE_HIT) {
            this->playerNextAttack = MAJORAS_WRATH_ATTACK_SUB_ACTION_QUICK_WHIP + 1;
        }
    }
}

// Every whip segment hits while the whip moves fast (or cracks), during an attack. Right whip: spheres 0-48,
// left whip: 50-98.
static void Boss07_Player_UpdateWhipHits(Boss07* this, PlayState* play) {
    f32 ratio = this->actor.scale.x / MAJORA_PLAYER_WRATH_BATTLE_SCALE;
    f32 radius = (22.0f * this->whipLengthScale * 0.75f) + 6.0f;
    f32 fastEnough = 4.0f * ratio;
    s32 h;
    s32 i;

    if (this->actionFunc == Boss07_Wrath_Attack) {
        for (h = 0; h < 2; h++) {
            MajoraWhip* whip = (h == 0) ? &this->rightWhip : &this->leftWhip;
            s32 cracking = whip->tension >= 50.0f;

            for (i = 1; i < (s32)sWhipLength; i++) {
                Vec3f* v = &whip->velocity[i];

                if (cracking || ((SQ(v->x) + SQ(v->y) + SQ(v->z)) > SQ(fastEnough))) {
                    Boss07_Player_SetHit(this, (h * 50) + i - 1, &whip->pos[i], radius, 2);
                }
            }
        }
    }

    // The kick, from the foot (placed by the draw)
    if ((this->actionFunc == Boss07_Wrath_Attack) && (this->subAction == MAJORAS_WRATH_ATTACK_SUB_ACTION_KICK) &&
        (this->frameCounter >= 6)) {
        ColliderJntSphElement* foot = &this->kickCollider.elements[MAJORAS_WARTH_KICK_COLLIDER_RIGHT_FOOT];

        foot->dim.worldSphere.radius = (s16)(foot->dim.modelSphere.radius * foot->dim.scale * ratio);
        CollisionCheck_SetAT(play, &play->colChkCtx, &this->kickCollider.base);
    }

    if (this->whipCrackTimer != 0) {
        this->whipCrackTimer--;
        if ((this->actionFunc == Boss07_Wrath_Attack) && (this->whipCrackTimer == 0)) {
            Audio_PlaySfx(NA_SE_EN_LAST3_ROD_FLOOR_OLD);
        }
    }
}

// ------------------------------------------------------------------------------------------------
// Buttons
// ------------------------------------------------------------------------------------------------

static void Boss07_Player_ReadButtons(Boss07* this, PlayState* play) {
    Input* input = CONTROLLER1(&play->state);
    s32 useCButtons = !CVarGetInteger(MAJORA_PLAYER_CVAR("KeepItems"), 0);

    if (CHECK_BTN_ALL(input->cur.button, BTN_B)) {
        this->playerHeldTimer++;
    } else {
        this->playerHeldTimer = 0;
    }

    if (!Boss07_Player_IsIdleAction(this) || (this->playerFormTimer != 0) ||
        !Boss07_Player_CanStartAttack(this, play)) {
        return;
    }

    if (CHECK_BTN_ANY(input->press.button, BTN_DRIGHT | BTN_DLEFT)) {
        Boss07_Player_TryChangeForm(this, play, input);
        return;
    }

    switch (this->playerForm) {
        case MAJORA_PLAYER_FORM_MASK:
            if ((CHECK_BTN_ALL(input->press.button, BTN_B) && CHECK_BTN_ALL(input->cur.button, BTN_R)) ||
                (useCButtons && CHECK_BTN_ALL(input->press.button, BTN_CRIGHT))) {
                Boss07_Player_SetupFireBeam(this, play);
            } else if (CHECK_BTN_ALL(input->press.button, BTN_B) ||
                       (useCButtons && CHECK_BTN_ALL(input->press.button, BTN_CLEFT))) {
                Boss07_Player_SetupSpinAttack(this, play);
            }
            break;

        case MAJORA_PLAYER_FORM_INCARNATION:
            if (CHECK_BTN_ALL(input->press.button, BTN_B) ||
                (useCButtons && CHECK_BTN_ALL(input->press.button, BTN_CLEFT))) {
                Boss07_Player_SetupIncarnationAttack(this, play);
            } else if (CHECK_BTN_ALL(input->press.button, BTN_DUP)) {
                Boss07_Player_SetupIncarnationDance(this, play);
            }
            break;

        case MAJORA_PLAYER_FORM_WRATH:
            if (CHECK_BTN_ALL(input->press.button, BTN_B)) {
                Boss07_Player_SetupWrathAttack(this, play,
                                               CHECK_BTN_ALL(input->cur.button, BTN_R)
                                                   ? MAJORAS_WRATH_ATTACK_SUB_ACTION_SPIN_ATTACK
                                                   : MAJORAS_WRATH_ATTACK_SUB_ACTION_QUICK_WHIP);
            } else if (useCButtons && CHECK_BTN_ALL(input->press.button, BTN_CLEFT)) {
                Boss07_Player_SetupWrathAttack(this, play, MAJORAS_WRATH_ATTACK_SUB_ACTION_DOUBLE_WHIP);
            } else if (useCButtons && CHECK_BTN_ALL(input->press.button, BTN_CDOWN)) {
                Boss07_Player_SetupWrathAttack(this, play, MAJORAS_WRATH_ATTACK_SUB_ACTION_KICK);
            } else if (useCButtons && CHECK_BTN_ALL(input->press.button, BTN_CRIGHT)) {
                Boss07_Player_SetupWrathAttack(this, play, MAJORAS_WRATH_ATTACK_SUB_ACTION_LONG_WHIP);
            } else if (CHECK_BTN_ALL(input->press.button, BTN_DUP)) {
                Boss07_Player_SetupWrathAttack(this, play, MAJORAS_WRATH_ATTACK_SUB_ACTION_TAUNT);
            }
            break;

        default:
            break;
    }
}

// ------------------------------------------------------------------------------------------------
// Update and draw
// ------------------------------------------------------------------------------------------------

// Link got hurt: Majora flashes red (and the mask shakes) like the boss when it's hit
static void Boss07_Player_UpdateHurt(Boss07* this, PlayState* play) {
    Player* player = GET_PLAYER(play);

    if ((player->invincibilityTimer > 0) && (this->playerPrevInvincibility <= 0) && !this->playerHidden) {
        this->damagedFlashTimer = 15;

        switch (this->playerForm) {
            case MAJORA_PLAYER_FORM_INCARNATION:
                Actor_PlaySfx(&this->actor, NA_SE_EN_LAST2_DAMAGE2_OLD);
                break;
            case MAJORA_PLAYER_FORM_WRATH:
                Actor_PlaySfx(&this->actor, NA_SE_EN_LAST3_VOICE_DAMAGE_OLD);
                break;
            default:
                this->maskShakeTimer = 15;
                Actor_PlaySfx(&this->actor, NA_SE_EN_LAST1_DAMAGE1_OLD);
                break;
        }
    }
    this->playerPrevInvincibility = player->invincibilityTimer;
}

void Boss07_Player_Update(Actor* thisx, PlayState* play2) {
    PlayState* play = play2;
    Boss07* this = (Boss07*)thisx;
    Player* player = GET_PLAYER(play);
    f32 scale = Boss07_Player_GetBattleScale(this->playerForm) * Boss07_Player_GetSizeRatio(this->playerForm);
    s32 i;

    this->frameCounter++;
    this->shouldUpdateTentaclesOrWhips = true;
    Math_Vec3f_Copy(&sMajoraSfxPos, &this->actor.projectedPos);

    for (i = 0; i < ARRAY_COUNT(this->timers); i++) {
        DECR(this->timers[i]);
    }
    DECR(this->maskShakeTimer);
    DECR(this->damagedFlashTimer);
    DECR(this->fireTimer);
    DECR(this->whipCollisionTimer);

    // The size can be changed in the menu at any time
    Actor_SetScale(&this->actor, scale);
    this->tentacleLengthScale = scale / MAJORA_PLAYER_MASK_BATTLE_SCALE;
    this->whipLengthScale = scale / MAJORA_PLAYER_WRATH_BATTLE_SCALE;
    this->generalCollider.dim.radius = (s16)(scale * 800.0f);
    this->generalCollider.dim.height = (s16)(scale * 1000.0f);
    this->generalCollider.dim.yShift = -this->generalCollider.dim.height / 2;

    if (Boss07_Player_ShouldHide(play)) {
        // Link is shown: Majora waits on him, ready to come back
        if (!this->playerHidden && !Boss07_Player_IsIdleAction(this)) {
            Boss07_Player_SetForm(this, play, this->playerForm, false);
        }
        this->playerHidden = true;
        this->playerCarry = false;
        Boss07_Player_SnapToLink(this, play);
        this->playerPrevInvincibility = player->invincibilityTimer;
        return;
    }

    if (this->playerHidden) {
        this->playerHidden = false;
        Boss07_Player_SnapToLink(this, play);
    }

    // The hits are placed again every frame
    Boss07_Player_ClearHits(this);
    this->beamTireMarkEnabled = false;

    if (this->playerForm == MAJORA_PLAYER_FORM_MASK) {
        this->maskEyeTexIndex =
            (player->stateFlags1 & PLAYER_STATE1_DEAD) ? MAJORAS_MASK_EYE_DULL : MAJORAS_MASK_EYE_NORMAL;
        this->tentacleState = MAJORAS_MASK_TENTACLE_STATE_DEFAULT;
    }
    if (this->playerForm == MAJORA_PLAYER_FORM_WRATH) {
        Math_ApproachZeroF(&this->wrathLeanRotY, 1.0f, 0.02f);
        Math_ApproachZeroF(&this->wrathLeanRotX, 1.0f, 0.02f);
    }

    Boss07_Player_ReadButtons(this, play);

    if (this->actionFunc == Boss07_Wrath_Attack) {
        Boss07_Player_UpdateWrathAttack(this, play);
    }

    this->actionFunc(this, play);

    // The vanilla Wrath attack ends in its idle: the combo's next attack, or back to following Link
    if (this->actionFunc == Boss07_Wrath_Idle) {
        if (this->playerNextAttack != 0) {
            Boss07_Player_SetupWrathAttack(this, play, this->playerNextAttack - 1);
        } else {
            Boss07_Player_SetupWrathMove(this, play);
        }
    }

    Boss07_Player_UpdateGrow(this);
    Boss07_Player_UpdateHurt(this, play);

    if (Boss07_Player_IsMaskFlying(this)) {
        Actor_UpdateVelocityWithoutGravity(&this->actor);
        Actor_UpdatePos(&this->actor);
        // Walls stop it
        Actor_UpdateBgCheckInfo(play, &this->actor, 20.0f, 30.0f, 40.0f,
                                UPDBGCHECKINFO_FLAG_1 | UPDBGCHECKINFO_FLAG_4);
        this->playerFloorY = this->actor.floorHeight;
    } else {
        this->playerFloorY = player->actor.floorHeight;
        this->actor.floorHeight = player->actor.floorHeight;
    }
    if (this->playerFloorY < BGCHECK_Y_MIN + 1.0f) {
        this->playerFloorY = player->actor.world.pos.y;
    }

    if (this->playerForm == MAJORA_PLAYER_FORM_MASK) {
        this->actor.focus.pos = this->actor.world.pos;
    }

    switch (this->playerForm) {
        case MAJORA_PLAYER_FORM_INCARNATION:
            Boss07_Player_UpdateAfterimages(this, play);
            break;

        case MAJORA_PLAYER_FORM_WRATH:
            Boss07_Player_UpdateWhipHits(this, play);
            break;

        default:
            // Cuts through everything while it spins
            if (Boss07_Player_IsMaskFlying(this)) {
                Collider_UpdateCylinder(&this->actor, &this->generalCollider);
                CollisionCheck_SetAT(play, &play->colChkCtx, &this->generalCollider.base);
            }

            // The beam's burn trail ends where the beam left the floor
            if (!this->beamTireMarkEnabled && this->prevBeamTireMarkEnabled &&
                (this->playerColliders & MAJORA_PLAYER_COLLIDER_TIRE_MARK)) {
                func_800AEF44(Effect_GetByIndex(this->effectIndex));
            }
            this->prevBeamTireMarkEnabled = this->beamTireMarkEnabled;
            break;
    }

    Boss07_Player_SubmitHits(this, play);
}

// The player Wrath's bust: two domes on her chest's eyes, the eyes at the tips (see z_boss_07_player_bust.inc).
// Called from her limb draw right after the torso, with the torso's matrix.
void Boss07_Player_DrawBust(Boss07* this, PlayState* play) {
    f32 size = CVarGetFloat(MAJORA_PLAYER_CVAR("WrathBustSize"), 0.65f);
    s32 i;

    if (!CVarGetInteger(MAJORA_PLAYER_CVAR("WrathBust"), 1) || (size <= 0.0f)) {
        return;
    }

    OPEN_DISPS(play->state.gfxCtx);

    gSPDisplayList(POLY_OPA_DISP++, sPlayerWrathBustMaterialDL);

    for (i = 0; i < ARRAY_COUNT(sPlayerWrathBustDLs); i++) {
        Matrix_Push();
        Matrix_Translate(sPlayerWrathBustCenters[i].x, sPlayerWrathBustCenters[i].y, sPlayerWrathBustCenters[i].z,
                         MTXMODE_APPLY);
        Matrix_Mult(&sPlayerWrathBustFrames[i], MTXMODE_APPLY);
        Matrix_Scale(1.0f, 1.0f, size, MTXMODE_APPLY);
        MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, play->state.gfxCtx);
        gSPDisplayList(POLY_OPA_DISP++, sPlayerWrathBustDLs[i]);
        Matrix_Pop();
    }

    CLOSE_DISPS(play->state.gfxCtx);
}

void Boss07_Player_Draw(Actor* thisx, PlayState* play2) {
    Boss07* this = (Boss07*)thisx;

    if (this->playerHidden) {
        return;
    }

    switch (this->playerForm) {
        case MAJORA_PLAYER_FORM_INCARNATION:
            Boss07_Incarnation_Draw(thisx, play2);
            break;
        case MAJORA_PLAYER_FORM_WRATH:
            Boss07_Wrath_Draw(thisx, play2);
            break;
        default:
            Boss07_Mask_Draw(thisx, play2);
            break;
    }
}

// ------------------------------------------------------------------------------------------------
// The player Incarnation's energy balls
// ------------------------------------------------------------------------------------------------

void Boss07_PlayerProjectile_Init(Boss07* this, PlayState* play) {
    Actor_ChangeCategory(play, &play->actorCtx, &this->actor, ACTORCAT_MISC);
    this->actor.flags &= ~(ACTOR_FLAG_ATTENTION_ENABLED | ACTOR_FLAG_HOSTILE);
    this->actor.flags |= ACTOR_FLAG_FREEZE_EXCEPTION;
    this->actor.update = Boss07_PlayerProjectile_Update;
    this->actor.draw = Boss07_Projectile_Draw;
    Collider_InitAndSetCylinder(play, &this->generalCollider, &this->actor, &sPlayerProjectileCylinderInit);
    this->projectileColorIndex = Rand_ZeroFloat(ARRAY_COUNT(sProjectileEnvColors) - 0.01f);
    this->subAction = PROJECTILE_SUB_ACTION_SPAWN;
    this->timers[0] = 60;
    this->speedToTarget = 1.0f; // size, set by the Incarnation that fires it
    this->playerTarget = NULL;
}

void Boss07_PlayerProjectile_Update(Actor* thisx, PlayState* play2) {
    PlayState* play = play2;
    Boss07* this = (Boss07*)thisx;
    s32 i;

    this->frameCounter++;
    for (i = 0; i < ARRAY_COUNT(this->timers); i++) {
        DECR(this->timers[i]);
    }

    Actor_SetScale(&this->actor, 3.5f * CLAMP(this->speedToTarget, 0.3f, 1.0f));

    if (this->subAction == PROJECTILE_SUB_ACTION_SPAWN) {
        f32 dx = this->playerAimPos.x - this->actor.world.pos.x;
        f32 dy = this->playerAimPos.y - this->actor.world.pos.y;
        f32 dz = this->playerAimPos.z - this->actor.world.pos.z;

        this->actor.world.rot.y = Math_Atan2S(dx, dz);
        this->actor.world.rot.x = Math_Atan2S(dy, sqrtf(SQ(dx) + SQ(dz)));
        this->actor.speed = MAJORA_PLAYER_BALL_SPEED;
        this->subAction = PROJECTILE_SUB_ACTION_FLY;
        Actor_PlaySfx(&this->actor, NA_SE_EN_LAST2_FIRE_OLD);
    } else if ((this->playerTarget != NULL) && (this->playerTarget->update != NULL)) {
        // A little homing toward a living target
        f32 dx = this->playerTarget->focus.pos.x - this->actor.world.pos.x;
        f32 dy = this->playerTarget->focus.pos.y - this->actor.world.pos.y;
        f32 dz = this->playerTarget->focus.pos.z - this->actor.world.pos.z;

        Math_ApproachS(&this->actor.world.rot.y, Math_Atan2S(dx, dz), 2, 0x500);
        Math_ApproachS(&this->actor.world.rot.x, Math_Atan2S(dy, sqrtf(SQ(dx) + SQ(dz))), 2, 0x500);
    }

    Actor_UpdateVelocityWithoutGravity(&this->actor);
    Actor_UpdatePos(&this->actor);
    Actor_UpdateBgCheckInfo(play, &this->actor, 15.0f, 15.0f, 30.0f,
                            UPDBGCHECKINFO_FLAG_1 | UPDBGCHECKINFO_FLAG_2 | UPDBGCHECKINFO_FLAG_4);
    this->actor.shape.rot.z += 0x1200;

    if ((this->actor.bgCheckFlags & (BGCHECKFLAG_GROUND | BGCHECKFLAG_WALL | BGCHECKFLAG_CEILING)) ||
        (this->generalCollider.base.atFlags & AT_HIT) || (this->timers[0] == 0)) {
        Actor_Kill(&this->actor);
        Actor_Spawn(&play->actorCtx, play, ACTOR_EN_CLEAR_TAG, this->actor.world.pos.x, this->actor.world.pos.y,
                    this->actor.world.pos.z, 0, 0, this->projectileColorIndex,
                    CLEAR_TAG_PARAMS(CLEAR_TAG_SMALL_LIGHT_RAYS));
        return;
    }

    Collider_UpdateCylinder(&this->actor, &this->generalCollider);
    CollisionCheck_SetAT(play, &play->colChkCtx, &this->generalCollider.base);
}
