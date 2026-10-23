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
 * Milestone 1: Majora's Mask. It floats at Link's head height, its tentacles trailing on the floor, and turns with
 * him. B (or C-Left): spin attack. The mask winds up, flies spinning at the lock-on target (or the enemy in front,
 * or straight ahead), cuts through everything on the way and comes back.
 */
// clang-format on

#define MAJORA_PLAYER_CVAR(name) ("gModes.PlayAsMajora." name)

void Boss07_Player_Update(Actor* thisx, PlayState* play2);
void Boss07_Player_Draw(Actor* thisx, PlayState* play2);

void Boss07_Player_SetupFloat(Boss07* this, PlayState* play);
void Boss07_Player_Float(Boss07* this, PlayState* play);
void Boss07_Player_SetupSpinAttack(Boss07* this, PlayState* play);
void Boss07_Player_SpinAttack(Boss07* this, PlayState* play);

typedef enum MajoraPlayerSpinSubAction {
    /* 0 */ MAJORA_PLAYER_SPIN_WIND_UP,
    /* 1 */ MAJORA_PLAYER_SPIN_OUT,
    /* 2 */ MAJORA_PLAYER_SPIN_RETURN,
    /* 3 */ MAJORA_PLAYER_SPIN_END
} MajoraPlayerSpinSubAction;

#define MAJORA_PLAYER_SPIN_SPEED 20.0f
#define MAJORA_PLAYER_SPIN_RANGE 900.0f // turns back this far from Link
#define MAJORA_PLAYER_AIM_RANGE 700.0f
#define MAJORA_PLAYER_AIM_CONE 0x2800

// The mask's hit while it spins. AT only: enemies' hits go to Link, not to the mask.
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

// ------------------------------------------------------------------------------------------------
// Queries used by the Link side and by the collision code
// ------------------------------------------------------------------------------------------------

s32 Boss07_IsPlayerAttack(Actor* actor) {
    return (actor != NULL) && (actor->id == ACTOR_BOSS_07) && (actor->params == MAJORA_TYPE_PLAYER);
}

s32 Boss07_Player_IsBusy(Actor* thisx) {
    Boss07* this = (Boss07*)thisx;

    if (!Boss07_IsPlayerAttack(thisx)) {
        return false;
    }
    return this->actionFunc == Boss07_Player_SpinAttack;
}

s32 Boss07_Player_IsHidden(Actor* thisx) {
    Boss07* this = (Boss07*)thisx;

    if (!Boss07_IsPlayerAttack(thisx)) {
        return true;
    }
    return this->playerHidden;
}

// ------------------------------------------------------------------------------------------------
// Helpers
// ------------------------------------------------------------------------------------------------

static f32 Boss07_Player_GetScale(void) {
    return CVarGetFloat(MAJORA_PLAYER_CVAR("Scale"), 0.05f);
}

// The mask's center above Link's feet
static f32 Boss07_Player_GetHoverHeight(void) {
    return CVarGetFloat(MAJORA_PLAYER_CVAR("HoverHeight"), 45.0f);
}

static void Boss07_Player_GetHoverPos(Boss07* this, PlayState* play, Vec3f* pos) {
    Player* player = GET_PLAYER(play);
    f32 height = Boss07_Player_GetHoverHeight();

    // Majora's Mask sinks to the floor when Link dies
    if (player->stateFlags1 & PLAYER_STATE1_DEAD) {
        height = 12.0f;
    }

    pos->x = player->actor.world.pos.x;
    pos->y = player->actor.world.pos.y + height + (Math_SinS(this->frameCounter * 0x800) * 3.0f);
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

            if ((actor->update == NULL) || Boss07_IsPlayerAttack(actor) ||
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
        this->playerAimPos.y = this->actor.world.pos.y;
        this->playerAimPos.z = player->actor.world.pos.z + (Math_CosS(player->actor.shape.rot.y) * 350.0f);
    }
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
    this->actor.world.pos.y += dy * 0.35f;
    this->actor.world.pos.z += dz * 0.5f;
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

static void Boss07_Player_ReadButtons(Boss07* this, PlayState* play) {
    Input* input = CONTROLLER1(&play->state);
    s32 useCButtons = !CVarGetInteger(MAJORA_PLAYER_CVAR("KeepItems"), 0);

    if ((this->actionFunc != Boss07_Player_Float) || !Boss07_Player_CanStartAttack(this, play)) {
        return;
    }

    if (CHECK_BTN_ALL(input->press.button, BTN_B) || (useCButtons && CHECK_BTN_ALL(input->press.button, BTN_CLEFT))) {
        Boss07_Player_SetupSpinAttack(this, play);
    }
}

// ------------------------------------------------------------------------------------------------
// Init
// ------------------------------------------------------------------------------------------------

void Boss07_Player_Init(Boss07* this, PlayState* play) {
    f32 scale = Boss07_Player_GetScale();

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

    this->playerForm = MAJORA_PLAYER_FORM_MASK;
    this->playerHidden = true;
    this->playerPrevInvincibility = 0;

    ActorShape_Init(&this->actor.shape, 0.0f, ActorShadow_DrawCircle, 15.0f);
    this->actor.shape.shadowAlpha = 160;
    SkelAnime_Init(play, &this->skelAnime, &gMajorasMaskSkel, &gMajorasMaskFloatingAnim, this->jointTable,
                   this->morphTable, MAJORAS_MASK_LIMB_MAX);
    Collider_InitAndSetCylinder(play, &this->generalCollider, &this->actor, &sPlayerMaskAttackCylinderInit);

    Actor_SetScale(&this->actor, scale);
    this->tentacleLengthScale = scale / 0.1f;
    this->maskEyeTexIndex = MAJORAS_MASK_EYE_NORMAL;
    this->tentacleState = MAJORAS_MASK_TENTACLE_STATE_DEFAULT;

    Boss07_Player_SnapToLink(this, play);
    this->playerFloorY = GET_PLAYER(play)->actor.floorHeight;
    Boss07_Player_SetupFloat(this, play);
}

// ------------------------------------------------------------------------------------------------
// Majora's Mask: floating with Link
// ------------------------------------------------------------------------------------------------

void Boss07_Player_SetupFloat(Boss07* this, PlayState* play) {
    this->actionFunc = Boss07_Player_Float;
    Animation_MorphToLoop(&this->skelAnime, &gMajorasMaskFloatingAnim, -10.0f);
    this->angularVelocity = 0;
    this->actor.speed = 0.0f;
}

void Boss07_Player_Float(Boss07* this, PlayState* play) {
    Player* player = GET_PLAYER(play);
    f32 moved;
    s16 lean;

    SkelAnime_Update(&this->skelAnime);
    Boss07_Player_Follow(this, play);
    moved = sqrtf(SQ(this->actor.world.pos.x - this->actor.prevPos.x) +
                  SQ(this->actor.world.pos.z - this->actor.prevPos.z));

    // Faces where Link faces, leans into his movement and bobs a little
    Math_ApproachS(&this->actor.shape.rot.y, player->actor.shape.rot.y, 3, 0x1800);
    this->actor.world.rot.y = this->actor.shape.rot.y;
    lean = CLAMP_MAX((s16)(moved * 0x200), 0x1800);
    Math_ApproachS(&this->actor.shape.rot.x, lean + (s16)(Math_SinS(this->frameCounter * 0x1388) * 0x300), 5,
                   0x400);
    Math_ApproachS(&this->actor.shape.rot.z, (s16)(Math_SinS(this->frameCounter * 0x700) * 0x400), 5, 0x200);
}

// ------------------------------------------------------------------------------------------------
// Majora's Mask: spin attack
// ------------------------------------------------------------------------------------------------

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
            Math_ApproachS(&this->actor.shape.rot.y, Math_Vec3f_Yaw(&this->actor.world.pos, &this->playerAimPos), 2,
                           0x2000);
            Math_ApproachS(&this->actor.shape.rot.x, -0x4000, 2, 0x1000);
            Math_ApproachS(&this->angularVelocity, 0x2000, 1, 0x400);

            if (this->timers[0] == 0) {
                this->subAction = MAJORA_PLAYER_SPIN_OUT;
                this->actor.world.rot.y = Math_Vec3f_Yaw(&this->actor.world.pos, &this->playerAimPos);
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

// ------------------------------------------------------------------------------------------------
// Update and draw
// ------------------------------------------------------------------------------------------------

// Link got hurt: the mask shakes and flashes red like the boss when it's hit
static void Boss07_Player_UpdateHurt(Boss07* this, PlayState* play) {
    Player* player = GET_PLAYER(play);

    if ((player->invincibilityTimer > 0) && (this->playerPrevInvincibility <= 0) && !this->playerHidden) {
        this->maskShakeTimer = 15;
        this->damagedFlashTimer = 15;
        Actor_PlaySfx(&this->actor, NA_SE_EN_LAST1_DAMAGE1_OLD);
    }
    this->playerPrevInvincibility = player->invincibilityTimer;
}

static s32 Boss07_Player_IsFlying(Boss07* this) {
    return (this->actionFunc == Boss07_Player_SpinAttack) &&
           ((this->subAction == MAJORA_PLAYER_SPIN_OUT) || (this->subAction == MAJORA_PLAYER_SPIN_RETURN));
}

void Boss07_Player_Update(Actor* thisx, PlayState* play2) {
    PlayState* play = play2;
    Boss07* this = (Boss07*)thisx;
    Player* player = GET_PLAYER(play);
    f32 scale = Boss07_Player_GetScale();
    s32 i;

    this->frameCounter++;
    this->shouldUpdateTentaclesOrWhips = true;

    for (i = 0; i < ARRAY_COUNT(this->timers); i++) {
        DECR(this->timers[i]);
    }
    DECR(this->maskShakeTimer);
    DECR(this->damagedFlashTimer);

    // The size can be changed in the menu at any time
    Actor_SetScale(&this->actor, scale);
    this->tentacleLengthScale = scale / 0.1f;
    this->generalCollider.dim.radius = (s16)(scale * 800.0f);
    this->generalCollider.dim.height = (s16)(scale * 1000.0f);
    this->generalCollider.dim.yShift = -this->generalCollider.dim.height / 2;

    if (Boss07_Player_ShouldHide(play)) {
        // Link is shown: Majora waits on him, ready to come back
        if (!this->playerHidden && (this->actionFunc != Boss07_Player_Float)) {
            Boss07_Player_SetupFloat(this, play);
        }
        this->playerHidden = true;
        Boss07_Player_SnapToLink(this, play);
        this->playerPrevInvincibility = player->invincibilityTimer;
        return;
    }

    if (this->playerHidden) {
        this->playerHidden = false;
        Boss07_Player_SnapToLink(this, play);
    }

    this->maskEyeTexIndex = (player->stateFlags1 & PLAYER_STATE1_DEAD) ? MAJORAS_MASK_EYE_DULL : MAJORAS_MASK_EYE_NORMAL;
    this->tentacleState = MAJORAS_MASK_TENTACLE_STATE_DEFAULT;

    Boss07_Player_ReadButtons(this, play);
    this->actionFunc(this, play);
    Boss07_Player_UpdateHurt(this, play);

    if (Boss07_Player_IsFlying(this)) {
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

    this->actor.focus.pos = this->actor.world.pos;

    // Cuts through everything while it spins
    if (Boss07_Player_IsFlying(this)) {
        Collider_UpdateCylinder(&this->actor, &this->generalCollider);
        CollisionCheck_SetAT(play, &play->colChkCtx, &this->generalCollider.base);
    }
}

void Boss07_Player_Draw(Actor* thisx, PlayState* play2) {
    Boss07* this = (Boss07*)thisx;

    if (this->playerHidden) {
        return;
    }
    Boss07_Mask_Draw(thisx, play2);
}
