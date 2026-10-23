// Play as Majora: Wrath's curvier figure
//
// Wrath's waist is skinned to her pelvis, so a slimmer waist and wider hips can't be made by scaling limbs. Instead,
// the display lists of her pelvis, torso and thighs are copied from the loaded game assets at runtime (nothing of the
// original model is stored here), with every vertex copied and moved by the figure deformation in z_boss_07_player.inc
// (`Boss07_Player_FigureDeformLocal`). Her limb draw swaps the copies in for the player Wrath only.
//
// Vertex loads in the asset display lists are `G_VTX_OTR_HASH` (or `G_VTX_OTR_FILEPATH`) commands. Their first word
// takes a real pointer in place of the offset (anything above 0xFFFFF), so each copied load points at its own moved
// vertices. Each load's vertices are moved through the flex matrix (G_MTX to segment 0x0D) they're drawn with.

#include <array>
#include <vector>

#include <libultraship/bridge/resourcebridge.h>

extern "C" {
#include "z64.h"
#include "overlays/actors/ovl_Boss_07/z_boss_07.h"
#include "assets/objects/object_boss07/object_boss07.h"
Gfx* ResourceMgr_LoadGfxByName(const char* path);
}

namespace {

constexpr uint8_t kOpVtx = 0x32;         // G_VTX_OTR_HASH: offset (or pointer), then the CRC64
constexpr uint8_t kOpVtxFilePath = 0x24; // G_VTX_OTR_FILEPATH: path, then count / index / offset
constexpr uint8_t kOpMtx = 0xDA;         // G_MTX
constexpr uint8_t kOpEndDL = 0xDF;       // G_ENDDL

bool IsTwoWordCommand(uint8_t op) {
    switch (op) {
        case 0x20: // G_SETTIMG_OTR_HASH
        case 0x24: // G_VTX_OTR_FILEPATH
        case 0x31: // G_DL_OTR_HASH
        case 0x32: // G_VTX_OTR_HASH
        case 0x35: // G_BRANCH_Z_OTR
        case 0x36: // G_MTX_OTR
        case 0x42: // G_MOVEMEM_HASH
            return true;
        default:
            return false;
    }
}

struct FigureLimb {
    s32 limbIndex;
    const char* path;
    s32 mtxSlot; // the limb's own flex matrix, used until the list loads another one
    std::vector<Gfx> gfx;
    std::vector<Vtx> vtx;
};

FigureLimb sLimbs[] = {
    { MAJORAS_WRATH_LIMB_PELVIS, gMajorasWrathPelvisDL, 0, {}, {} },
    { MAJORAS_WRATH_LIMB_LEFT_THIGH, gMajorasWrathLeftThighDL, 1, {}, {} },
    { MAJORAS_WRATH_LIMB_RIGHT_THIGH, gMajorasWrathRightThighDL, 4, {}, {} },
    { MAJORAS_WRATH_LIMB_TORSO, gMajorasWrathTorsoDL, 7, {}, {} },
};

std::array<f32, 4> sBuiltParams = { -1.0f, -1.0f, -1.0f, -1.0f };
bool sBuilt = false;

uint8_t Opcode(const Gfx& g) {
    return (uint8_t)(((uint32_t)g.words.w0 >> 24) & 0xFF);
}

// Where a vertex load's source vertices are, and how many / to which index
bool GetVtxLoad(const Gfx* cmd, Vtx** src, u32* count, u32* index) {
    uint8_t op = Opcode(cmd[0]);
    uint32_t w0 = (uint32_t)cmd[0].words.w0;

    if (op == kOpVtx) {
        uintptr_t offset = cmd[0].words.w1;
        uint64_t hash = ((uint64_t)(uint32_t)cmd[1].words.w0 << 32) + (uint32_t)cmd[1].words.w1;

        *count = (w0 >> 12) & 0xFF;
        *index = ((w0 >> 1) & 0x7F) - *count;
        if (offset > 0xFFFFF) {
            *src = (Vtx*)offset; // already resolved by the renderer
        } else {
            char* base = (char*)ResourceGetDataByCrc(hash);
            if (base == nullptr) {
                return false;
            }
            *src = (Vtx*)(base + offset);
        }
        return true;
    }

    if (op == kOpVtxFilePath) {
        const char* path = (const char*)cmd[0].words.w1;
        Vtx* base = (Vtx*)ResourceGetDataByName(path);

        if (base == nullptr) {
            return false;
        }
        *count = (u32)cmd[1].words.w0;
        *index = (u32)(cmd[1].words.w1 >> 16);
        *src = base + (cmd[1].words.w1 & 0xFFFF);
        return true;
    }

    return false;
}

void MoveVtx(Vtx* v, s32 mtxSlot) {
    Vec3f pos = { (f32)v->n.ob[0], (f32)v->n.ob[1], (f32)v->n.ob[2] };
    Vec3f normal = { v->n.n[0] / 127.0f, v->n.n[1] / 127.0f, v->n.n[2] / 127.0f };

    Boss07_Player_FigureDeformLocal(mtxSlot, &pos, &normal);

    v->n.ob[0] = (s16)CLAMP(pos.x, -32768.0f, 32767.0f);
    v->n.ob[1] = (s16)CLAMP(pos.y, -32768.0f, 32767.0f);
    v->n.ob[2] = (s16)CLAMP(pos.z, -32768.0f, 32767.0f);
    v->n.n[0] = (s8)CLAMP(normal.x * 127.0f, -127.0f, 127.0f);
    v->n.n[1] = (s8)CLAMP(normal.y * 127.0f, -127.0f, 127.0f);
    v->n.n[2] = (s8)CLAMP(normal.z * 127.0f, -127.0f, 127.0f);
}

bool BuildLimb(FigureLimb& limb) {
    Gfx* src = ResourceMgr_LoadGfxByName(limb.path);
    size_t length = 0;
    size_t vtxTotal = 0;
    Vtx* vtxSrc;
    u32 count;
    u32 index;

    if (src == nullptr) {
        return false;
    }

    // First pass: the list's length and how many vertices it loads, so the vertex copies never move
    for (size_t i = 0;; i++) {
        uint8_t op = Opcode(src[i]);

        if (op == kOpEndDL) {
            length = i + 1;
            break;
        }
        if (GetVtxLoad(&src[i], &vtxSrc, &count, &index)) {
            vtxTotal += count;
        }
        if (IsTwoWordCommand(op)) {
            i++;
        }
        if (i > 0x4000) {
            return false; // not a list we know
        }
    }

    limb.gfx.assign(src, src + length);
    limb.vtx.assign(vtxTotal, Vtx{});

    // Second pass: copy and move each load's vertices, and point the load at them
    s32 mtxSlot = limb.mtxSlot;
    size_t vtxUsed = 0;

    for (size_t i = 0; i < length; i++) {
        Gfx* cmd = &limb.gfx[i];
        uint8_t op = Opcode(*cmd);

        if ((op == kOpMtx) && ((cmd->words.w1 >> 24) == 0x0D)) {
            mtxSlot = (s32)((cmd->words.w1 & 0xFFFFFF) / sizeof(Mtx));
        } else if (GetVtxLoad(cmd, &vtxSrc, &count, &index)) {
            Vtx* dst = &limb.vtx[vtxUsed];

            for (u32 k = 0; k < count; k++) {
                dst[k] = vtxSrc[k];
                MoveVtx(&dst[k], CLAMP(mtxSlot, 0, 15));
            }
            vtxUsed += count;

            // A hash load with a pointer in place of its offset. The second word (the hash) is left as it is.
            cmd->words.w0 = ((uintptr_t)kOpVtx << 24) | ((count & 0xFF) << 12) | (((index + count) & 0x7F) << 1);
            cmd->words.w1 = (uintptr_t)dst;
        }

        if (IsTwoWordCommand(op)) {
            i++;
        }
    }

    return true;
}

} // namespace

// The player Wrath's moved copy of a limb's display list, or NULL to draw the original
extern "C" Gfx* PlayAsMajora_GetWrathFigureDL(s32 limbIndex) {
    std::array<f32, 4> params;

    if (!Boss07_Player_GetFigureParams(params.data())) {
        return NULL;
    }

    if (!sBuilt || (params != sBuiltParams)) {
        sBuilt = true;
        sBuiltParams = params;
        for (FigureLimb& limb : sLimbs) {
            if (!BuildLimb(limb)) {
                limb.gfx.clear();
                limb.vtx.clear();
            }
        }
    }

    for (FigureLimb& limb : sLimbs) {
        if ((limb.limbIndex == limbIndex) && !limb.gfx.empty()) {
            return limb.gfx.data();
        }
    }
    return NULL;
}
