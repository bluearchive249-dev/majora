// Play as Majora
//
// Replaces Link with Majora from the Moon fight, with the boss's own moveset instead of Link's.
//
// Link is still there underneath. He stays invisible and keeps handling walking, collision, doors, ladders, loading
// zones and the camera, while Majora (ACTOR_BOSS_07 with MAJORA_TYPE_PLAYER) follows him and does the attacking.
// See z_boss_07_player.inc for Majora's side.
//
// This file:
//  - spawns Majora next to Link (again after every scene change) and removes it when the option is turned off,
//  - lets Majora's actor load outside the Moon (its object isn't in other scenes' object lists),
//  - filters Link's buttons so he can't use his sword, shield, rolls or (optionally) C-button items,
//  - hides Link and his shadow while Majora is shown,
//  - keeps Majora's form (Mask, Incarnation, Wrath) across areas, and lets Wrath's spin attack carry Link along.

#include <libultraship/bridge/consolevariablebridge.h>
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/ShipInit.hpp"

extern "C" {
#include "variables.h"
#include "overlays/actors/ovl_Boss_07/z_boss_07.h"
}

#define CVAR_PLAY_AS_MAJORA(var) "gModes.PlayAsMajora." var
#define CVAR_ENABLED_NAME CVAR_PLAY_AS_MAJORA("Enabled")
#define CVAR_ENABLED CVarGetInteger(CVAR_ENABLED_NAME, 0)

static bool sLinkShadowHidden = false;
static u8 sForm = MAJORA_PLAYER_FORM_MASK; // kept across areas

static Boss07* FindPlayerMajora(PlayState* play) {
    // It's spawned as a boss and moves itself to the misc list on its first frame
    static const u8 sCategories[] = { ACTORCAT_MISC, ACTORCAT_BOSS };

    for (u8 category : sCategories) {
        for (Actor* actor = play->actorCtx.actorLists[category].first; actor != NULL; actor = actor->next) {
            if ((actor->id == ACTOR_BOSS_07) && (actor->params == MAJORA_TYPE_PLAYER) && (actor->update != NULL)) {
                return (Boss07*)actor;
            }
        }
    }
    return NULL;
}

static bool ShouldHideLink(PlayState* play) {
    if (!CVAR_ENABLED || (play == NULL)) {
        return false;
    }
    Boss07* majora = FindPlayerMajora(play);
    return (majora != NULL) && !Boss07_Player_IsHidden(&majora->actor);
}

static void SetLinkShadowHidden(Player* player, bool hidden) {
    if (hidden) {
        player->actor.shape.shadowAlpha = 0;
        sLinkShadowHidden = true;
    } else if (sLinkShadowHidden) {
        player->actor.shape.shadowAlpha = 255;
        sLinkShadowHidden = false;
    }
}

// Runs after Link's update every frame, enabled or not, so turning the option off also removes Majora
static void OnLinkUpdate(Actor* actor) {
    PlayState* play = gPlayState;
    Player* player = (Player*)actor;

    if ((play == NULL) || (actor != &GET_PLAYER(play)->actor)) {
        return;
    }

    Boss07* majora = FindPlayerMajora(play);

    if (!CVAR_ENABLED || (gSaveContext.gameMode != GAMEMODE_NORMAL)) {
        if (majora != NULL) {
            Actor_Kill(&majora->actor);
        }
        SetLinkShadowHidden(player, false);
        return;
    }

    if (majora == NULL) {
        // rot.z: the form to start in
        Actor_Spawn(&play->actorCtx, play, ACTOR_BOSS_07, player->actor.world.pos.x, player->actor.world.pos.y,
                    player->actor.world.pos.z, 0, player->actor.shape.rot.y, sForm, MAJORA_PARAMS(MAJORA_TYPE_PLAYER));
    } else {
        sForm = Boss07_Player_GetForm(&majora->actor);

        // Wrath's spin attack glides toward the target and takes Link with it. Walls still stop him.
        if (Boss07_Player_IsCarrying(&majora->actor)) {
            player->actor.world.pos.x = majora->actor.world.pos.x;
            player->actor.world.pos.z = majora->actor.world.pos.z;
            player->speedXZ = 0.0f;
            player->actor.speed = 0.0f;
            Actor_UpdateBgCheckInfo(play, &player->actor, 26.0f, 18.0f, 60.0f,
                                    UPDBGCHECKINFO_FLAG_1 | UPDBGCHECKINFO_FLAG_4);
        }
    }

    SetLinkShadowHidden(player, ShouldHideLink(play));
}

static void FilterLinkInput(Input* input) {
    PlayState* play = gPlayState;

    if (!ShouldHideLink(play)) {
        return;
    }

    Boss07* majora = FindPlayerMajora(play);
    bool busy = Boss07_Player_IsBusy(&majora->actor);

    // B = sword, R = shield, D-pad: Majora's forms and taunts. Z still locks on: Majora attacks the lock-on target.
    u16 blocked = BTN_B | BTN_R | BTN_DRIGHT | BTN_DLEFT | BTN_DUP;

    if (!CVarGetInteger(CVAR_PLAY_AS_MAJORA("KeepItems"), 0)) {
        blocked |= BTN_CLEFT | BTN_CDOWN | BTN_CRIGHT;
    }

    // A still opens doors, talks, climbs and grabs, but no rolls, jump attacks or side hops
    u16 doAction = play->interfaceCtx.aButtonDoAction;
    if (busy || (doAction == DO_ACTION_NONE) || (doAction == DO_ACTION_ATTACK) || (doAction == DO_ACTION_JUMP) ||
        (doAction == DO_ACTION_PUTAWAY)) {
        blocked |= BTN_A;
    }

    input->cur.button &= ~blocked;
    input->press.button &= ~blocked;
    input->rel.button &= ~blocked;

    // Link stays put while Majora attacks
    if (busy) {
        input->cur.stick_x = input->cur.stick_y = 0;
        input->rel.stick_x = input->rel.stick_y = 0;
        input->press.stick_x = input->press.stick_y = 0;
    }
}

static void RegisterPlayAsMajora() {
    sLinkShadowHidden = false;

    // Always registered, so turning the option off also removes Majora
    COND_ID_HOOK(OnActorUpdate, ACTOR_PLAYER, true, OnLinkUpdate);

    COND_ID_HOOK(ShouldActorDraw, ACTOR_PLAYER, CVAR_ENABLED, [](Actor* actor, bool* should) {
        if ((gPlayState != NULL) && (actor == &GET_PLAYER(gPlayState)->actor) && ShouldHideLink(gPlayState)) {
            *should = false;
        }
    });

    COND_HOOK(OnPassPlayerInputs, CVAR_ENABLED, FilterLinkInput);

    // Majora's Incarnation outruns Link: his run speed is raised while it's out
    COND_VB_SHOULD(VB_SPEED_MODIFIER_WALK, CVAR_ENABLED, {
        f32* speedTarget = va_arg(args, f32*);

        if (ShouldHideLink(gPlayState) &&
            (Boss07_Player_GetForm(&FindPlayerMajora(gPlayState)->actor) == MAJORA_PLAYER_FORM_INCARNATION)) {
            *speedTarget *= CVarGetFloat(CVAR_PLAY_AS_MAJORA("IncarnationSpeed"), 2.0f);
        }
    });

    // Majora's object is only in the Moon's object list
    COND_VB_SHOULD(VB_ENABLE_OBJECT_DEPENDENCY, CVAR_ENABLED, {
        s16 objectId = (s16)va_arg(args, int);
        if (objectId == OBJECT_BOSS07) {
            *should = false;
        }
    });
}

static RegisterShipInitFunc initFunc(RegisterPlayAsMajora, { CVAR_ENABLED_NAME });
