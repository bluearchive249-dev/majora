// Play as Ganondorf
//
// Replaces Link with Ganondorf from Ocarina of Time (ACTOR_PLAYER_GDF, z_player_gdf.c), ported from the Ship of
// Harkinian mod (Blastwright, "Play as Ganondorf").
//
// Majora's Mask has no Ganondorf, so his model and animations come from the player's own Ocarina of Time archive
// (oot.o2r, made by Ship of Harkinian from their own ROM). Nothing from it is in this repository. Only his own files
// are taken from it (see sGdfPrefixes): mounting the whole archive would replace Majora's Mask's gameplay_keep,
// icons and every other file with the same path, and 2Ship refuses archives of another game anyway.
//
// Link is still there underneath, like in Play as Majora: he stays invisible and keeps handling walking, collision,
// doors, ladders, loading zones and the camera, while Ganondorf follows him. Ganondorf himself (his walk, moves,
// magic and cape) is the Ship of Harkinian mod's code (z_boss_ganon.c), compiled here as ovl_Player_Gdf.
//
// This file:
//  - finds oot.o2r and mounts Ganondorf's files from it (GanondorfArchive),
//  - spawns Ganondorf next to Link (again after every scene change) and removes him when the option is turned off,
//  - hides Link and his shadow while Ganondorf is shown, and takes his buttons for Ganondorf's moves,
//  - his levitation (L up, Z down) and sprint (hold A), and his run speed.

#include <filesystem>
#include <spdlog/spdlog.h>
#include <libultraship/bridge/consolevariablebridge.h>
#include <ship/Context.h>
#include <ship/resource/ResourceManager.h>
#include <ship/resource/archive/ArchiveManager.h>
#include <ship/resource/archive/Archive.h>
#include <ship/resource/archive/O2rArchive.h>
#include "2s2h/BenPort.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/ShipInit.hpp"

extern "C" {
#include "variables.h"

// ovl_Player_Gdf (its header maps Ocarina of Time names onto Majora's Mask's, so it's kept out of here)
#define GDF_PARAMS_PLAYER 0x10
s32 PlayerGdf_IsHidden(Actor* thisx);
s32 PlayerGdf_CanAct(Actor* thisx);
s32 BossGanon_Player_IsBusy(Actor* thisx);
}

#define CVAR_PLAY_AS_GANONDORF(var) "gModes.PlayAsGanondorf." var
#define CVAR_ENABLED_NAME CVAR_PLAY_AS_GANONDORF("Enabled")
#define CVAR_ENABLED CVarGetInteger(CVAR_ENABLED_NAME, 0)

// ------------------------------------------------------------------------------------------------
// Ganondorf's files from oot.o2r
// ------------------------------------------------------------------------------------------------

// Everything of his that the port uses. None of these paths exist in Majora's Mask.
static const char* sGdfPrefixes[] = {
    "objects/object_ganon/",        // his model, eyes, light balls, magic
    "objects/object_ganon_anime1/", // boss fight animations
    "objects/object_ganon_anime2/", // more of them (stand, idle, block, magic, ...)
    "objects/object_ganon_anime3/", // the organ and the tower collapse
    "objects/object_gndd/",         // young Ganondorf from the cutscenes: his walk is retargeted
    "objects/object_gnd/",          // Phantom Ganon (skin)
    "objects/object_ganon2/",       // Ganon (transformation)
    "overlays/ovl_En_Ganon_Mant/",  // his cape's texture
    "overlays/ovl_Boss_Ganon/",     // light ball, lightning and vortex models
    "overlays/ovl_Boss_Ganon2/",    // Ganon's effects
};

static bool IsGanondorfFile(const std::string& path) {
    for (const char* prefix : sGdfPrefixes) {
        if (path.rfind(prefix, 0) == 0) {
            return true;
        }
    }
    return false;
}

// oot.o2r seen through a filter: only Ganondorf's files are listed, and it has no game version (it isn't a game
// archive here, just a source of files), so the archive manager takes it next to mm.o2r
class GanondorfArchive : public Ship::Archive {
  public:
    GanondorfArchive(const std::string& path) : Ship::Archive(path) {
    }

    bool Open() override {
        mInner = std::make_shared<Ship::O2rArchive>(GetPath());
        if (!mInner->Open()) {
            mInner = nullptr;
            return false;
        }
        size_t count = 0;
        for (auto& [hash, name] : *mInner->ListFiles()) {
            if (IsGanondorfFile(name)) {
                IndexFile(name);
                count++;
            }
        }
        SPDLOG_INFO("Play as Ganondorf: {} of Ganondorf's files found in {}", count, GetPath());
        return count > 0;
    }

    bool Close() override {
        bool ok = true;
        if (mInner != nullptr) {
            ok = mInner->Close();
            mInner = nullptr;
        }
        return ok;
    }

    bool WriteFile(const std::string& filename, const std::vector<uint8_t>& data) override {
        return false;
    }

    std::shared_ptr<Ship::File> LoadFile(const std::string& filePath) override {
        if ((mInner == nullptr) || !HasFile(filePath)) {
            return nullptr; // also "version" and "manifest.json": no game version, never rejected
        }
        return mInner->LoadFile(filePath);
    }

    std::shared_ptr<Ship::File> LoadFile(uint64_t hash) override {
        auto files = ListFiles();
        auto it = files->find(hash);
        if ((mInner == nullptr) || (it == files->end())) {
            return nullptr;
        }
        return mInner->LoadFile(it->second);
    }

  private:
    std::shared_ptr<Ship::O2rArchive> mInner;
};

static bool sGdfAssetsMounted = false;
static std::string sGdfArchivePath = "";

static std::string FindOotArchive() {
    // A path set in the menu (or the config file) first, then oot.o2r next to 2ship.o2r / mm.o2r
    std::string path = CVarGetString(CVAR_PLAY_AS_GANONDORF("OotArchive"), "");
    if (!path.empty() && std::filesystem::is_regular_file(path)) {
        return path;
    }
    path = Ship::Context::LocateFileAcrossAppDirs("oot.o2r", appShortName);
    if (!path.empty() && std::filesystem::is_regular_file(path)) {
        return path;
    }
    return "";
}

static void MountGanondorfAssets() {
    if (sGdfAssetsMounted) {
        return;
    }
    std::string path = FindOotArchive();
    if (path.empty()) {
        SPDLOG_WARN("Play as Ganondorf: oot.o2r not found, Ganondorf can't be shown");
        return;
    }

    auto archive = std::make_shared<GanondorfArchive>(path);
    archive->Load();
    if (!archive->IsLoaded()) {
        SPDLOG_ERROR("Play as Ganondorf: no Ganondorf files in {}", path);
        return;
    }
    if (Ship::Context::GetRawInstance()->GetResourceManager()->GetArchiveManager()->AddArchive(archive) == nullptr) {
        SPDLOG_ERROR("Play as Ganondorf: couldn't add {}", path);
        return;
    }
    sGdfAssetsMounted = true;
    sGdfArchivePath = path;
}

extern "C" s32 PlayAsGanondorf_AssetsReady(void) {
    return sGdfAssetsMounted;
}

extern "C" const char* PlayAsGanondorf_ArchivePath(void) {
    return sGdfArchivePath.c_str();
}

// From the menu: look for the archive again (after copying it in, or changing the path)
extern "C" void PlayAsGanondorf_RetryAssets(void) {
    MountGanondorfAssets();
}

// ------------------------------------------------------------------------------------------------
// Ganondorf in the game
// ------------------------------------------------------------------------------------------------

#define GDF_LEVITATE_VERTICAL_SPEED 6.0f

static bool sLinkShadowHidden = false;
static bool sSprinting = false; // holding A on the ground: he sprints
static bool sLevitating = false;
static bool sSoftLanding = false; // let go of the levitation: no fall damage until Link lands
static f32 sLevitateY = 0.0f;
static f32 sLevitateSpeed = 0.0f;

static bool IsEnabled() {
    // (not together with Play as Majora: one of them replaces Link)
    return CVAR_ENABLED && !CVarGetInteger("gModes.PlayAsMajora.Enabled", 0);
}

static Actor* FindPlayerGanondorf(PlayState* play) {
    for (Actor* actor = play->actorCtx.actorLists[ACTORCAT_MISC].first; actor != NULL; actor = actor->next) {
        if ((actor->id == ACTOR_PLAYER_GDF) && (actor->params == GDF_PARAMS_PLAYER) && (actor->update != NULL)) {
            return actor;
        }
    }
    return NULL;
}

static bool ShouldHideLink(PlayState* play) {
    if (!IsEnabled() || (play == NULL) || !sGdfAssetsMounted) {
        return false;
    }
    Actor* gdf = FindPlayerGanondorf(play);
    return (gdf != NULL) && !PlayerGdf_IsHidden(gdf);
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

extern "C" s32 PlayAsGanondorf_KeepItems(void) {
    return CVarGetInteger(CVAR_PLAY_AS_GANONDORF("KeepItems"), 0);
}

extern "C" s32 PlayAsGanondorf_IsSprinting(void) {
    return sSprinting && IsEnabled();
}

extern "C" s32 PlayAsGanondorf_IsLevitating(void) {
    return sLevitating;
}

// Not ported yet: the organ, Ganon and his crawlspace flame
extern "C" s32 PlayAsGanondorf_UseOrganOcarina(void) {
    return false;
}

extern "C" s32 PlayAsGanondorf_OrganButtonPressed(void) {
    return false;
}

extern "C" s32 PlayAsGanondorf_IsGanonOut(PlayState* play) {
    return false;
}

extern "C" s32 PlayAsGanondorf_IsCrawlFlame(PlayState* play, Player* player) {
    return false;
}

static f32 GetRunSpeedFactor() {
    return CVarGetFloat(CVAR_PLAY_AS_GANONDORF("RunSpeed"), 1.5f) *
           (sSprinting ? CVarGetFloat(CVAR_PLAY_AS_GANONDORF("SprintSpeed"), 1.5f) : 1.0f);
}

static void StopLevitating() {
    if (sLevitating) {
        sLevitating = false;
        sSoftLanding = true;
    }
}

// L rises, Z sinks, neither hovers in place. The stick moves him like running on the ground. Runs after Link's own
// update, so it overrides his falling every frame.
static void UpdateLevitation(PlayState* play, Player* player, Actor* gdf) {
    Input* input = &play->state.input[0];
    bool up = CHECK_BTN_ALL(input->cur.button, BTN_L);
    bool down = CHECK_BTN_ALL(input->cur.button, BTN_Z);
    bool notAllowed = (player->stateFlags1 &
                       (PLAYER_STATE1_200000 | PLAYER_STATE1_2000 | PLAYER_STATE1_4 | PLAYER_STATE1_DEAD |
                        PLAYER_STATE1_20000000 | PLAYER_STATE1_TALKING | PLAYER_STATE1_800000 | PLAYER_STATE1_400 |
                        PLAYER_STATE1_10000000 | PLAYER_STATE1_CARRYING_ACTOR | PLAYER_STATE1_8000000)) ||
                      (player->stateFlags2 & PLAYER_STATE2_40000) || PlayerGdf_IsHidden(gdf);

    if (!CVarGetInteger(CVAR_PLAY_AS_GANONDORF("Levitate"), 1)) {
        notAllowed = true;
    }

    if (!sLevitating) {
        if (up && !notAllowed && PlayerGdf_CanAct(gdf)) {
            sLevitating = true;
            sSoftLanding = false;
            sLevitateY = player->actor.world.pos.y;
            sLevitateSpeed = player->speedXZ;
        } else {
            // He floats back down after letting go, no fall damage
            if (sSoftLanding) {
                if (player->actor.bgCheckFlags & BGCHECKFLAG_GROUND) {
                    sSoftLanding = false;
                } else {
                    player->fallStartHeight = (s16)player->actor.world.pos.y;
                    player->fallDistance = 0;
                }
            }
            return;
        }
    }

    if (notAllowed) {
        StopLevitating();
        return;
    }

    // Up and down
    if (up && !down) {
        sLevitateY += GDF_LEVITATE_VERTICAL_SPEED;
    } else if (down && !up) {
        sLevitateY -= GDF_LEVITATE_VERTICAL_SPEED;

        // Landed: Link takes over on the ground again
        if ((player->actor.floorHeight > BGCHECK_Y_MIN + 1.0f) && (sLevitateY <= player->actor.floorHeight)) {
            player->actor.world.pos.y = player->actor.floorHeight;
            player->actor.velocity.y = 0.0f;
            StopLevitating();
            return;
        }
    }

    // Around, like running: camera-relative stick, as Link on the ground
    f32 stickMagnitude;
    s16 stickAngle;
    Lib_GetControlStickData(&stickMagnitude, &stickAngle, input);

    if (BossGanon_Player_IsBusy(gdf) || (stickMagnitude < 8.0f)) {
        stickMagnitude = 0.0f;
    }

    f32 targetSpeed = (stickMagnitude / 60.0f) * 6.0f * GetRunSpeedFactor();
    Math_StepToF(&sLevitateSpeed, targetSpeed, (targetSpeed > sLevitateSpeed) ? 1.5f : 2.0f);

    if (stickMagnitude > 0.0f) {
        s16 worldYaw = (s16)(Camera_GetInputDirYaw(GET_ACTIVE_CAM(play)) + stickAngle);
        Math_ScaledStepToS(&player->actor.shape.rot.y, worldYaw, 0x1800);
    }
    player->yaw = player->actor.world.rot.y = player->actor.shape.rot.y;

    player->actor.world.pos.x += Math_SinS(player->actor.shape.rot.y) * sLevitateSpeed;
    player->actor.world.pos.z += Math_CosS(player->actor.shape.rot.y) * sLevitateSpeed;
    player->actor.world.pos.y = sLevitateY;

    // Walls and ceilings still stop him
    Actor_UpdateBgCheckInfo(play, &player->actor, 26.0f, 18.0f, Player_GetHeight(player),
                            UPDBGCHECKINFO_FLAG_1 | UPDBGCHECKINFO_FLAG_2);
    sLevitateY = player->actor.world.pos.y;

    // Link's own falling does nothing while levitating
    player->speedXZ = 0.0f;
    player->actor.speed = 0.0f;
    player->actor.velocity.x = player->actor.velocity.y = player->actor.velocity.z = 0.0f;
    player->fallStartHeight = (s16)player->actor.world.pos.y;
    player->fallDistance = 0;
}

// Runs after Link's update every frame, enabled or not, so turning the option off also removes Ganondorf
static void OnLinkUpdate(Actor* actor) {
    PlayState* play = gPlayState;

    if ((play == NULL) || (actor != &GET_PLAYER(play)->actor)) {
        return;
    }

    Player* player = (Player*)actor;
    Actor* gdf = FindPlayerGanondorf(play);

    if (!IsEnabled() || !sGdfAssetsMounted || (gSaveContext.gameMode != GAMEMODE_NORMAL)) {
        if (gdf != NULL) {
            Actor_Kill(gdf);
        }
        SetLinkShadowHidden(player, false);
        sLevitating = sSoftLanding = false;
        return;
    }

    if (gdf == NULL) {
        // New area: start on the ground
        sLevitating = sSoftLanding = false;
        Actor_Spawn(&play->actorCtx, play, ACTOR_PLAYER_GDF, player->actor.world.pos.x, player->actor.world.pos.y,
                    player->actor.world.pos.z, 0, player->actor.shape.rot.y, 0, GDF_PARAMS_PLAYER);
        gdf = FindPlayerGanondorf(play);
    }
    if (gdf != NULL) {
        UpdateLevitation(play, player, gdf);
    }

    SetLinkShadowHidden(player, ShouldHideLink(play));
}

// Link's buttons while Ganondorf is out: B, R, L, Z, D-pad (and the C-buttons, unless kept) are his moves
static void FilterLinkInput(Input* input) {
    PlayState* play = gPlayState;

    sSprinting = false;
    if (!ShouldHideLink(play)) {
        return;
    }
    Actor* gdf = FindPlayerGanondorf(play);
    Player* player = GET_PLAYER(play);
    bool busy = BossGanon_Player_IsBusy(gdf);

    // B = his light ball, R = block (R + B the ground pound), L = levitate, D-pad Up = taunt.
    // Z still locks on (and sinks him while levitating).
    u16 blocked = BTN_B | BTN_R | BTN_L | BTN_DUP | BTN_DDOWN;

    if (!PlayAsGanondorf_KeepItems()) {
        blocked |= BTN_CLEFT | BTN_CDOWN | BTN_CRIGHT;
    }

    // A still opens doors, talks, climbs and grabs, but no rolls, jump attacks or side hops: where it would roll,
    // holding it makes him sprint
    u16 doAction = play->interfaceCtx.aButtonDoAction;
    bool aFree = (doAction == DO_ACTION_NONE) || (doAction == DO_ACTION_ATTACK) || (doAction == DO_ACTION_JUMP) ||
                 (doAction == DO_ACTION_PUTAWAY);

    sSprinting = CVarGetInteger(CVAR_PLAY_AS_GANONDORF("Sprint"), 1) && !busy && !sLevitating && aFree &&
                 CHECK_BTN_ALL(input->cur.button, BTN_A) && (player->actor.bgCheckFlags & BGCHECKFLAG_GROUND);
    if (busy || sLevitating || aFree) {
        blocked |= BTN_A;
    }

    input->cur.button &= ~blocked;
    input->press.button &= ~blocked;
    input->rel.button &= ~blocked;

    // He stays put while he attacks, and levitation does its own movement
    if (busy || sLevitating) {
        input->cur.stick_x = input->cur.stick_y = 0;
        input->rel.stick_x = input->rel.stick_y = 0;
        input->press.stick_x = input->press.stick_y = 0;
    }
}

// The camera (z_camera.c) frames the player by his height: Ganondorf's while he's shown. Areas under the open sky use
// wider camera settings, so they have their own distance. Levitating, he's higher up.
#define GDF_STANDING_CAMERA_HEIGHT 120.0f
#define GDF_LEVITATING_CAMERA_HEIGHT 150.0f

extern "C" f32 PlayAsGanondorf_GetCameraHeight(Player* player) {
    f32 height = Player_GetHeight(player);
    PlayState* play = gPlayState;

    if ((play == NULL) || (player != GET_PLAYER(play)) || !ShouldHideLink(play)) {
        return height;
    }

    bool outdoors = (play->skyboxId == SKYBOX_NORMAL_SKY) && !play->envCtx.skyboxDisabled;
    f32 distance = outdoors ? CVarGetFloat(CVAR_PLAY_AS_GANONDORF("CameraDistanceOutdoors"), 0.75f)
                            : CVarGetFloat(CVAR_PLAY_AS_GANONDORF("CameraDistance"), 1.0f);
    f32 ganondorfHeight = (sLevitating ? GDF_LEVITATING_CAMERA_HEIGHT : GDF_STANDING_CAMERA_HEIGHT) * distance;

    return MAX(height, ganondorfHeight);
}

static void RegisterPlayAsGanondorf() {
    sLinkShadowHidden = false;

    // Always registered, so turning the option off also removes Ganondorf
    COND_ID_HOOK(OnActorUpdate, ACTOR_PLAYER, true, OnLinkUpdate);

    COND_ID_HOOK(ShouldActorDraw, ACTOR_PLAYER, CVAR_ENABLED, [](Actor* actor, bool* should) {
        if ((gPlayState != NULL) && (actor == &GET_PLAYER(gPlayState)->actor) && ShouldHideLink(gPlayState)) {
            *should = false;
        }
    });

    COND_HOOK(OnPassPlayerInputs, CVAR_ENABLED, FilterLinkInput);

    // His long stride: Link runs faster while Ganondorf is out, more when sprinting
    COND_VB_SHOULD(VB_SPEED_MODIFIER_WALK, CVAR_ENABLED, {
        f32* speedTarget = va_arg(args, f32*);

        if (ShouldHideLink(gPlayState)) {
            *speedTarget *= GetRunSpeedFactor();
        }
    });
}

static RegisterShipInitFunc initFunc(RegisterPlayAsGanondorf, { CVAR_ENABLED_NAME });

// Mounted once at startup, whether the option is on or not, so it can be turned on in game
static RegisterShipInitFunc assetsInitFunc(MountGanondorfAssets, {});
