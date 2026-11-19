#ifndef GANONDORF_VOICE_H
#define GANONDORF_VOICE_H

// Play as Ganondorf: his voice lines from Ocarina of Time (GanondorfVoice.cpp)

#ifdef __cplusplus
#include <functional>
#include <memory>
#include <string>
#include <vector>
extern "C" {
#endif

#include "z64math.h"

typedef enum GanondorfVoiceId {
    GDF_VOICE_LAUGH,
    GDF_VOICE_CHUCKLE,
    GDF_VOICE_HURT1,
    GDF_VOICE_HURT2,
    GDF_VOICE_ATTACK,
    GDF_VOICE_HEAVY_ATTACK,
    GDF_VOICE_REBUFF,
    GDF_VOICE_STRIKE_GROUND,
    GDF_VOICE_RECOVER,
    GDF_VOICE_BREATHE,
    GDF_VOICE_DYING,
    GDF_VOICE_DEFIANCE,
    GDF_VOICE_MAX
} GanondorfVoiceId;

// Plays one of his lines (cutting off the one before); false if it couldn't (no oot.o2r, or the Voice option off)
s32 GanondorfVoice_Play(s32 id, Vec3f* projectedPos);
// Mixes it into the game's audio (stereo, 32 kHz): BenPort.cpp
void GanondorfVoice_Mix(s16* buf, u32 frames);

#ifdef __cplusplus
}
// Reads and decodes his samples from oot.o2r, given a way to read its files
bool GanondorfVoice_Load(const std::function<std::shared_ptr<std::vector<char>>(const std::string&)>& loadFile);
#endif

#endif
