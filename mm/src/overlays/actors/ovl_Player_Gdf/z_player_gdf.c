/*
 * File: z_player_gdf.c
 * Overlay: ovl_Player_Gdf
 * Description: 2S2H "Play as Ganondorf": Ganondorf from Ocarina of Time in place of Link
 *
 * Link is still there underneath, invisible, doing the walking, collision, doors and camera. This actor stands where
 * he is and draws Ganondorf. His model and animations come from the player's own oot.o2r (PlayAsGanondorf.cpp).
 *
 * First step of the port from Ship of Harkinian's mod (z_boss_ganon.c): his model, eyes and shadow, standing idle,
 * and his levitation pose while moving. The walk, moveset, magic, cape and trident come next.
 */

#include "z_player_gdf.h"
#include "z_player_gdf_assets.h"
#include <libultraship/bridge/consolevariablebridge.h>

#define FLAGS (ACTOR_FLAG_UPDATE_CULLING_DISABLED | ACTOR_FLAG_DRAW_CULLING_DISABLED)

#define PLAYER_GDF_CVAR(var) "gModes.PlayAsGanondorf." var

// His limbs (gGanondorfSkel), as numbered by the skeleton drawing (root = 1)
#define GDF_LIMB_HEAD 11  // the eyes are drawn on it
#define GDF_LIMB_FOCUS 19 // what targeting and the camera look at

#define GDF_SCALE 0.01f         // as in the Ocarina of Time boss fight
#define GDF_MOVE_SPEED_MIN 1.0f // Link faster than this: Ganondorf moves

void PlayerGdf_Init(Actor* thisx, PlayState* play);
void PlayerGdf_Destroy(Actor* thisx, PlayState* play);
void PlayerGdf_Update(Actor* thisx, PlayState* play);
void PlayerGdf_Draw(Actor* thisx, PlayState* play);

ActorProfile Player_Gdf_Profile = {
    /**/ ACTOR_PLAYER_GDF,
    /**/ ACTORCAT_MISC,
    /**/ FLAGS,
    /**/ GAMEPLAY_KEEP, // his files are read by name from oot.o2r, no object to load
    /**/ sizeof(PlayerGdf),
    /**/ PlayerGdf_Init,
    /**/ PlayerGdf_Destroy,
    /**/ PlayerGdf_Update,
    /**/ PlayerGdf_Draw,
};

// Link is shown instead of Ganondorf in these cases (as in Play as Majora)
static s32 PlayerGdf_ShouldHide(PlayState* play) {
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
    // Story cutscenes animate Link: show him instead
    if (CVarGetInteger(PLAYER_GDF_CVAR("ShowLinkInCutscenes"), 1) && (play->csCtx.state != CS_STATE_IDLE)) {
        return true;
    }
    return false;
}

s32 PlayerGdf_IsHidden(Actor* thisx) {
    return ((PlayerGdf*)thisx)->hidden;
}

static void PlayerGdf_SnapToLink(PlayerGdf* this, PlayState* play) {
    Player* player = GET_PLAYER(play);

    Math_Vec3f_Copy(&this->actor.world.pos, &player->actor.world.pos);
    Math_Vec3f_Copy(&this->actor.prevPos, &player->actor.prevPos);
    this->actor.shape.rot.y = this->actor.world.rot.y = player->actor.shape.rot.y;
}

static void PlayerGdf_SetAnim(PlayerGdf* this, u8 anim) {
    if (this->anim == anim) {
        return;
    }
    this->anim = anim;

    switch (anim) {
        case PLAYER_GDF_ANIM_FLOAT:
            Animation_MorphToLoop(&this->skelAnime, (AnimationHeader*)gGanondorfFloatAnim, -8.0f);
            break;
        default:
            Animation_MorphToLoop(&this->skelAnime, (AnimationHeader*)gGanondorfStandIdleAnim, -8.0f);
            break;
    }
}

void PlayerGdf_Init(Actor* thisx, PlayState* play) {
    PlayerGdf* this = (PlayerGdf*)thisx;

    // Spawned only once his files are mounted (PlayAsGanondorf.cpp), but never draw a missing skeleton
    if (!PlayAsGanondorf_AssetsReady()) {
        Actor_Kill(&this->actor);
        return;
    }

    Actor_SetScale(&this->actor, GDF_SCALE);
    ActorShape_Init(&this->actor.shape, 0.0f, ActorShadow_DrawCircle, 50.0f);

    SkelAnime_InitFlex(play, &this->skelAnime, (FlexSkeletonHeader*)gGanondorfSkel,
                       (AnimationHeader*)gGanondorfStandIdleAnim, NULL, NULL, 0);
    this->anim = PLAYER_GDF_ANIM_IDLE;

    this->hidden = PlayerGdf_ShouldHide(play);
    PlayerGdf_SnapToLink(this, play);
}

void PlayerGdf_Destroy(Actor* thisx, PlayState* play) {
    PlayerGdf* this = (PlayerGdf*)thisx;

    if (this->skelAnime.jointTable != NULL) {
        SkelAnime_Free(&this->skelAnime, play);
    }
}

void PlayerGdf_Update(Actor* thisx, PlayState* play) {
    PlayerGdf* this = (PlayerGdf*)thisx;
    Player* player = GET_PLAYER(play);

    this->hidden = PlayerGdf_ShouldHide(play);
    this->actor.shape.shadowAlpha = this->hidden ? 0 : 255;

    PlayerGdf_SnapToLink(this, play);

    PlayerGdf_SetAnim(this, (player->speedXZ > GDF_MOVE_SPEED_MIN) ? PLAYER_GDF_ANIM_FLOAT : PLAYER_GDF_ANIM_IDLE);
    SkelAnime_Update(&this->skelAnime);
}

static void PlayerGdf_PostLimbDraw(PlayState* play, s32 limbIndex, Gfx** dList, Vec3s* rot, Actor* thisx) {
    PlayerGdf* this = (PlayerGdf*)thisx;

    if (limbIndex == GDF_LIMB_FOCUS) {
        Matrix_MultZero(&this->actor.focus.pos);
    } else if (limbIndex == GDF_LIMB_HEAD) {
        OPEN_DISPS(play->state.gfxCtx);

        MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx);
        gSPDisplayList(POLY_XLU_DISP++, (Gfx*)gGanondorfEyesDL);

        CLOSE_DISPS(play->state.gfxCtx);
    }
}

void PlayerGdf_Draw(Actor* thisx, PlayState* play) {
    PlayerGdf* this = (PlayerGdf*)thisx;

    if (this->hidden) {
        return;
    }

    OPEN_DISPS(play->state.gfxCtx);

    Gfx_SetupDL25_Opa(play->state.gfxCtx);
    Gfx_SetupDL25_Xlu(play->state.gfxCtx);

    gSPSegment(POLY_XLU_DISP++, 0x08, Lib_SegmentedToVirtual((void*)gGanondorfNormalEyeTex));

    SkelAnime_DrawFlexOpa(play, this->skelAnime.skeleton, this->skelAnime.jointTable, this->skelAnime.dListCount, NULL,
                          PlayerGdf_PostLimbDraw, &this->actor);

    CLOSE_DISPS(play->state.gfxCtx);
}
