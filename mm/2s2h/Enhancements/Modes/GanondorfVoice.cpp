// Play as Ganondorf: his own voice from Ocarina of Time
//
// Majora's Mask's sound banks have no Ganondorf, and adding instruments to its sound effect sequence and soundfont is
// a big job, so his voice lines get a small mixer of their own instead: the samples are read from the player's own
// oot.o2r (nothing of them is in this repository), decoded from the N64's ADPCM to 16-bit sound once, and mixed into
// the game's audio right before it goes out (BenPort.cpp, OTRAudio_Thread), placed left/right and by distance like
// the game's own sounds.
//
// Which sample each of his sounds plays, and at what rate, is from Ocarina of Time's sound effect font
// (01_Sound_Effects_2): the rate is the sample's tuning times the 32 kHz output.

#include <cmath>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <spdlog/spdlog.h>
#include <libultraship/bridge/consolevariablebridge.h>
#include "GanondorfVoice.h"

namespace {

struct VoiceSample {
    const char* name; // in oot.o2r: audio/samples/<name>_META
    float tuning;
    std::vector<int16_t> pcm;
    float rate = 0.0f; // samples per second
};

// In the order of GanondorfVoiceId
VoiceSample sSamples[GDF_VOICE_MAX] = {
    { "Ganondorf Laughing", 0.3969f },           // GDF_VOICE_LAUGH
    { "Ganondorf Chuckles", 0.3746f },           // GDF_VOICE_CHUCKLE
    { "Ganondorf Hurt 1", 0.5f },                // GDF_VOICE_HURT1
    { "Ganondorf Hurt 2", 0.5f },                // GDF_VOICE_HURT2
    { "Ganondorf Attacks", 0.4454f },            // GDF_VOICE_ATTACK
    { "Ganondorf Heavy Attack", 0.4454f },       // GDF_VOICE_HEAVY_ATTACK
    { "Ganondorf Rebuffs", 0.4454f },            // GDF_VOICE_REBUFF
    { "Ganondorf Strikes the Ground", 0.4454f }, // GDF_VOICE_STRIKE_GROUND
    { "Ganondorf Recovers", 0.4454f },           // GDF_VOICE_RECOVER
    { "Ganondorf Breathes (Unused)", 0.4454f },  // GDF_VOICE_BREATHE
    { "Ganondorf Dying Breath", 0.3969f },       // GDF_VOICE_DYING
    { "Ganondorf Yells in Defiance", 0.5f },     // GDF_VOICE_DEFIANCE
};

#define GDF_VOICE_OUTPUT_RATE 32000.0f

struct Playing {
    const VoiceSample* sample = nullptr;
    double pos = 0.0;
    float gainL = 0.0f;
    float gainR = 0.0f;
};

// One voice (a new line cuts the one before, he has one mouth) and one for the rest
Playing sVoice;
std::mutex sMutex;
bool sLoaded = false;

int16_t Clamp16(int32_t v) {
    return (int16_t)((v < -32768) ? -32768 : ((v > 32767) ? 32767 : v));
}

// The N64's ADPCM: frames of a header byte (scale, predictor) and 16 samples, 4 bits each (9 bytes, codec 0) or 2
// bits each (5 bytes, codec 3, "small ADPCM"). Each sample is predicted from the two before it by the predictor's
// coefficients (order 2: two rows of 8).
std::vector<int16_t> DecodeAdpcm(const uint8_t* data, uint32_t size, int codec, const std::vector<int16_t>& book,
                                 int order, int npredictors) {
    std::vector<int16_t> out;
    const uint32_t frameSize = (codec == 0) ? 9 : 5;
    int32_t prev1 = 0;
    int32_t prev2 = 0;

    if ((order != 2) || (npredictors <= 0) || (book.size() < (size_t)(npredictors * order * 8))) {
        return out;
    }
    out.reserve((size / frameSize) * 16);

    for (uint32_t f = 0; f + frameSize <= size; f += frameSize) {
        const uint8_t* frame = data + f;
        int32_t scale = 1 << (frame[0] >> 4);
        int pred = frame[0] & 0xF;
        int32_t ins[16];

        if (pred >= npredictors) {
            pred = 0;
        }
        const int16_t* c0 = &book[pred * order * 8];
        const int16_t* c1 = c0 + 8;

        for (int i = 0; i < 16; i++) {
            int32_t v;

            if (codec == 0) {
                v = (frame[1 + i / 2] >> ((i % 2 == 0) ? 4 : 0)) & 0xF;
                if (v >= 8) {
                    v -= 16;
                }
            } else {
                v = (frame[1 + i / 4] >> (6 - 2 * (i % 4))) & 3;
                if (v >= 2) {
                    v -= 4;
                }
            }
            ins[i] = v * scale;
        }

        for (int g = 0; g < 2; g++) {
            int32_t* ix = &ins[g * 8];
            int16_t group[8];

            for (int j = 0; j < 8; j++) {
                int32_t acc = c0[j] * prev2 + c1[j] * prev1;

                for (int k = 0; k < j; k++) {
                    acc += c1[j - k - 1] * ix[k];
                }
                acc += ix[j] << 11;
                group[j] = Clamp16(acc >> 11);
                out.push_back(group[j]);
            }
            prev2 = group[6];
            prev1 = group[7];
        }
    }
    return out;
}

// A sample resource as Ship of Harkinian writes it (version 2, little endian): the 64-byte resource header, codec,
// medium, two flags, data size, the data, the loop (start, end, count, state), and the codebook.
bool ParseSample(const std::vector<char>& file, VoiceSample& sample) {
    const uint8_t* b = (const uint8_t*)file.data();
    size_t n = file.size();
    size_t o = 0x40;
    auto u32 = [&](size_t at) -> uint32_t {
        uint32_t v;
        memcpy(&v, b + at, 4);
        return v;
    };

    if (n < o + 8) {
        return false;
    }
    int codec = b[o];
    uint32_t size = u32(o + 4);
    o += 8;
    if ((codec != 0 && codec != 3) || (o + size > n)) {
        return false;
    }
    const uint8_t* data = b + o;
    o += size;
    if (o + 16 > n) {
        return false;
    }
    o += 12;                     // loop start, end, count
    o += 4 + 2 * (size_t)u32(o); // loop state
    if (o + 12 > n) {
        return false;
    }
    int order = (int32_t)u32(o);
    int npredictors = (int32_t)u32(o + 4);
    uint32_t bookCount = u32(o + 8);
    o += 12;
    if (o + 2 * (size_t)bookCount > n) {
        return false;
    }
    std::vector<int16_t> book(bookCount);
    memcpy(book.data(), b + o, 2 * (size_t)bookCount);

    sample.pcm = DecodeAdpcm(data, size, codec, book, order, npredictors);
    sample.rate = sample.tuning * GDF_VOICE_OUTPUT_RATE;
    return !sample.pcm.empty();
}

} // namespace

bool GanondorfVoice_Load(const std::function<std::shared_ptr<std::vector<char>>(const std::string&)>& loadFile) {
    int loaded = 0;

    for (int i = 0; i < GDF_VOICE_MAX; i++) {
        std::string path = std::string("audio/samples/") + sSamples[i].name + "_META";
        auto file = loadFile(path);

        if ((file != nullptr) && ParseSample(*file, sSamples[i])) {
            loaded++;
        } else {
            SPDLOG_WARN("Play as Ganondorf: couldn't read his voice sample {}", path);
        }
    }
    SPDLOG_INFO("Play as Ganondorf: {} of his voice lines read from oot.o2r", loaded);
    std::lock_guard<std::mutex> lock(sMutex);
    sLoaded = loaded > 0;
    return sLoaded;
}

extern "C" s32 GanondorfVoice_Play(s32 id, Vec3f* projectedPos) {
    if ((id < 0) || (id >= GDF_VOICE_MAX) || sSamples[id].pcm.empty() ||
        !CVarGetInteger("gModes.PlayAsGanondorf.Voice", 1)) {
        return false;
    }

    // Placed like the game's sounds: left/right by where he is on screen, quieter the further away
    float gain = 1.0f;
    float pan = 0.0f;
    if (projectedPos != NULL) {
        float dist = sqrtf(projectedPos->x * projectedPos->x + projectedPos->y * projectedPos->y +
                           projectedPos->z * projectedPos->z);
        gain = 1.0f - fminf(fmaxf((dist - 300.0f) / 2200.0f, 0.0f), 0.85f);
        pan = fminf(fmaxf(projectedPos->x / fmaxf(fabsf(projectedPos->z), 100.0f), -1.0f), 1.0f) * 0.6f;
    }
    gain *= CVarGetFloat("gModes.PlayAsGanondorf.VoiceVolume", 1.0f);

    std::lock_guard<std::mutex> lock(sMutex);
    if (!sLoaded) {
        return false;
    }
    sVoice.sample = &sSamples[id];
    sVoice.pos = 0.0;
    sVoice.gainL = gain * (pan <= 0.0f ? 1.0f : 1.0f - pan);
    sVoice.gainR = gain * (pan >= 0.0f ? 1.0f : 1.0f + pan);
    return true;
}

extern "C" void GanondorfVoice_Mix(s16* buf, u32 frames) {
    std::lock_guard<std::mutex> lock(sMutex);
    Playing& p = sVoice;

    if (p.sample == nullptr) {
        return;
    }

    // The game's own volume settings apply to him too
    float volume = CVarGetFloat("gSettings.Audio.MasterVolume", 1.0f) *
                   CVarGetFloat("gSettings.Audio.SoundEffectsVolume", 1.0f) * 0.9f;
    const std::vector<int16_t>& pcm = p.sample->pcm;
    double step = p.sample->rate / GDF_VOICE_OUTPUT_RATE;

    for (u32 i = 0; i < frames; i++) {
        size_t a = (size_t)p.pos;

        if (a + 1 >= pcm.size()) {
            p.sample = nullptr;
            break;
        }
        float frac = (float)(p.pos - (double)a);
        float s = (pcm[a] + (pcm[a + 1] - pcm[a]) * frac) * volume;

        buf[2 * i] = Clamp16(buf[2 * i] + (int32_t)(s * p.gainL));
        buf[2 * i + 1] = Clamp16(buf[2 * i + 1] + (int32_t)(s * p.gainR));
        p.pos += step;
    }
}
