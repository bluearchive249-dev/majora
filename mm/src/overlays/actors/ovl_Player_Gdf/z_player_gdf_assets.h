#ifndef Z_PLAYER_GDF_ASSETS_H
#define Z_PLAYER_GDF_ASSETS_H

// Names of Ganondorf's files in Ocarina of Time's archive (oot.o2r). The files themselves are the player's own,
// read at runtime (PlayAsGanondorf.cpp); only their names are here, as in Ship of Harkinian's asset headers.

#include "align_asset_macro.h"

// object_ganon
static const ALIGN_ASSET(2) char gGanondorfSkel[] = "__OTR__objects/object_ganon/gGanondorfSkel";
static const ALIGN_ASSET(2) char gGanondorfEyesDL[] = "__OTR__objects/object_ganon/gGanondorfEyesDL";
static const ALIGN_ASSET(2) char gGanondorfNormalEyeTex[] = "__OTR__objects/object_ganon/gGanondorfNormalEyeTex";
static const ALIGN_ASSET(2) char gGanondorfRightHandOpenDL[] = "__OTR__objects/object_ganon/gGanondorfRightHandOpenDL";

// object_ganon_anime1
static const ALIGN_ASSET(2) char gGanondorfFloatAnim[] = "__OTR__objects/object_ganon_anime1/gGanondorfFloatAnim";

// object_ganon_anime2
static const ALIGN_ASSET(2) char gGanondorfStandIdleAnim[] =
    "__OTR__objects/object_ganon_anime2/gGanondorfStandIdleAnim";

#endif
