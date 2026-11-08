#ifndef Z_PLAYER_GDF_H
#define Z_PLAYER_GDF_H

#include "global.h"

// Play as Ganondorf: Ganondorf from Ocarina of Time, following the (hidden) player.
// His files come from the player's own oot.o2r (PlayAsGanondorf.cpp mounts them).

typedef enum PlayerGdfAnim {
    /* 0 */ PLAYER_GDF_ANIM_NONE,
    /* 1 */ PLAYER_GDF_ANIM_IDLE,
    /* 2 */ PLAYER_GDF_ANIM_FLOAT
} PlayerGdfAnim;

typedef struct PlayerGdf {
    Actor actor;
    SkelAnime skelAnime;
    u8 anim;
    u8 hidden;
} PlayerGdf;

s32 PlayerGdf_IsHidden(Actor* thisx);

// PlayAsGanondorf.cpp
s32 PlayAsGanondorf_AssetsReady(void);
const char* PlayAsGanondorf_ArchivePath(void);
void PlayAsGanondorf_RetryAssets(void);

#endif
