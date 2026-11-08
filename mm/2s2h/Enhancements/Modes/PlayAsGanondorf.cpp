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
// doors, ladders, loading zones and the camera, while Ganondorf follows him.
//
// This file:
//  - finds oot.o2r and mounts Ganondorf's files from it (GanondorfArchive),
//  - spawns Ganondorf next to Link (again after every scene change) and removes him when the option is turned off,
//  - hides Link and his shadow while Ganondorf is shown.

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
#include "overlays/actors/ovl_Player_Gdf/z_player_gdf.h"
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

static bool sLinkShadowHidden = false;

static PlayerGdf* FindPlayerGanondorf(PlayState* play) {
    for (Actor* actor = play->actorCtx.actorLists[ACTORCAT_MISC].first; actor != NULL; actor = actor->next) {
        if ((actor->id == ACTOR_PLAYER_GDF) && (actor->update != NULL)) {
            return (PlayerGdf*)actor;
        }
    }
    return NULL;
}

static bool ShouldHideLink(PlayState* play) {
    if (!CVAR_ENABLED || (play == NULL) || !sGdfAssetsMounted) {
        return false;
    }
    PlayerGdf* gdf = FindPlayerGanondorf(play);
    return (gdf != NULL) && !PlayerGdf_IsHidden(&gdf->actor);
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

// Runs after Link's update every frame, enabled or not, so turning the option off also removes Ganondorf
static void OnLinkUpdate(Actor* actor) {
    PlayState* play = gPlayState;

    if ((play == NULL) || (actor != &GET_PLAYER(play)->actor)) {
        return;
    }

    Player* player = (Player*)actor;
    PlayerGdf* gdf = FindPlayerGanondorf(play);

    if (!CVAR_ENABLED || !sGdfAssetsMounted || (gSaveContext.gameMode != GAMEMODE_NORMAL)) {
        if (gdf != NULL) {
            Actor_Kill(&gdf->actor);
        }
        SetLinkShadowHidden(player, false);
        return;
    }

    if (gdf == NULL) {
        Actor_Spawn(&play->actorCtx, play, ACTOR_PLAYER_GDF, player->actor.world.pos.x, player->actor.world.pos.y,
                    player->actor.world.pos.z, 0, player->actor.shape.rot.y, 0, 0);
    }

    SetLinkShadowHidden(player, ShouldHideLink(play));
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
}

static RegisterShipInitFunc initFunc(RegisterPlayAsGanondorf, { CVAR_ENABLED_NAME });

// Mounted once at startup, whether the option is on or not, so it can be turned on in game
static RegisterShipInitFunc assetsInitFunc(MountGanondorfAssets, {});
