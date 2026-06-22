#include "pch.h"

#include <thread>
#include <chrono>
#include <string>
#include <vector>

#include "discord/discord_rpc.h"
#include "getwzinfo.h"          // GetMapById — shared with the world-map tooltip feature
#include "discordintegration.h"

// =====================================================================
// CONFIG — FILL THESE IN (Discord Developer Portal)
// =====================================================================
//  1. https://discord.com/developers/applications -> New Application
//  2. copy the Application ID into DISCORD_APPLICATION_ID
//  3. Rich Presence -> Art Assets: upload a large image, set LARGE_IMAGE_KEY
//     to its (lowercase) key name.
// Without a valid id + asset, presence either no-ops or shows no art.
static const char* DISCORD_APPLICATION_ID = "1518476683198337115"; // BeiDou app
static const char* LARGE_IMAGE_KEY        = "icon1";               // BeiDou art asset key
static const char* SERVER_NAME            = "BeiDou";              // large-image hover text

static const int   REFRESH_SECONDS = 15;   // Discord rate-limits presence to ~1 / 15s
static const int   WARMUP_TICKS    = 3;    // ignore the first few ticks at startup

// =====================================================================
// v83 ADDRESSES / OFFSETS (verified against the Orion edit, base 0x400000)
// =====================================================================
namespace addr {
constexpr uintptr_t CUserLocal_Inst   = 0x00BEBF98; // *(void**) -> CUserLocal*  (TSingleton)
constexpr uintptr_t CharacterStat     = 0x00BF3CD8; // *(void**) -> GW_CharacterStat*
constexpr uintptr_t CUIMiniMap_Inst   = 0x00BED788; // *(void**) -> CUIMiniMap*
constexpr uintptr_t GetJobCode        = 0x0095FFC3; // int  __fastcall(CUserLocal*, int)
constexpr uintptr_t GetJobName        = 0x004A77EF; // char* __cdecl(int jobId)

constexpr int OFS_Ign   = 0x4;   // GW_CharacterStat -> char ign[]
constexpr int OFS_Level = 0x33;  // GW_CharacterStat -> ZtlSecure<level> (fused) ; CUserLocal -> level (raw)
constexpr int OFS_MapId = 0x668; // CUIMiniMap -> current field id
} // namespace addr

// =====================================================================
// RAW READERS  (SEH-guarded, POD-only — kept free of any C++ object so the
// std::string composers below never mix __try with object unwinding [C2712].
// All game memory is touched ONLY through these.)
// =====================================================================
typedef int(__fastcall* GetJobCode_t)(void*, int);
typedef char*(__cdecl* GetJobName_t)(int);

static bool seh_isLoggedIn() {
    __try {
        BYTE* inst = *reinterpret_cast<BYTE**>(addr::CUserLocal_Inst);
        if (!inst) return false;
        BYTE level = inst[addr::OFS_Level];
        return level >= 1 && level <= 250;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

static int seh_level() {
    __try {
        uintptr_t stat = *reinterpret_cast<uintptr_t*>(addr::CharacterStat);
        if (!stat) return 0;
        // ZtlSecure fuse: value = byte[0] ^ byte[1]
        unsigned char* p = reinterpret_cast<unsigned char*>(stat + addr::OFS_Level);
        return static_cast<unsigned char>(p[0] ^ p[1]);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }
}

static int seh_jobId() {
    __try {
        void* inst = *reinterpret_cast<void**>(addr::CUserLocal_Inst);
        if (!inst) return -1;
        return reinterpret_cast<GetJobCode_t>(addr::GetJobCode)(inst, 0);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

static int seh_mapId() {
    __try {
        uintptr_t base = *reinterpret_cast<uintptr_t*>(addr::CUIMiniMap_Inst);
        if (!base) return 0;
        return *reinterpret_cast<int*>(base + addr::OFS_MapId);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }
}

static bool seh_jobName(int jobId, char* dst, int cap) {
    __try {
        const char* s = reinterpret_cast<GetJobName_t>(addr::GetJobName)(jobId);
        if (!s) return false;
        int i = 0;
        for (; i < cap - 1 && s[i]; ++i) dst[i] = s[i];
        dst[i] = 0;
        return i > 0;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

static bool seh_charName(char* dst, int cap) {
    __try {
        uintptr_t stat = *reinterpret_cast<uintptr_t*>(addr::CharacterStat);
        if (!stat) return false;
        const char* s = reinterpret_cast<const char*>(stat + addr::OFS_Ign);
        int i = 0;
        for (; i < cap - 1 && s[i]; ++i) dst[i] = s[i];
        dst[i] = 0;
        return i > 0;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// =====================================================================
// STRING COMPOSERS  (no __try here — only call the POD readers above)
// =====================================================================
static std::string ANSItoUTF8(const std::string& str) {
    if (str.empty()) return "";
    int wlen = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, nullptr, 0);
    if (wlen <= 0) return "";
    std::wstring wide(wlen, 0);
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, &wide[0], wlen);

    int u8len = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (u8len <= 0) return "";
    std::string out(u8len, 0);
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, &out[0], u8len, nullptr, nullptr);
    return std::string(out.c_str()); // trim trailing NUL
}

static std::string getCharName() {
    char buf[64];
    if (!seh_charName(buf, sizeof(buf))) return "";
    return ANSItoUTF8(buf);
}

static std::string getJobName() {
    int id = seh_jobId();
    if (id < 0) return "";
    char buf[64];
    if (!seh_jobName(id, buf, sizeof(buf))) return "";
    return ANSItoUTF8(buf);
}

static std::string getMapName() {
    int id = seh_mapId();
    if (id <= 0) return "";
    // NOTE: GetMapById reads the WZ (WzLib) — like the Orion reference, this runs on the
    // refresh thread. GetMapById guards its own WZ access with try/catch and returns
    // "Unknown" on failure.
    std::string full = GetMapById(id);
    if (full.empty() || full == "Unknown") return "";
    // getwzinfo returns "street - mapName"; show just the map name.
    size_t split = full.find(" - ");
    std::string name = (split != std::string::npos) ? full.substr(split + 3) : full;
    return ANSItoUTF8(name);
}

// =====================================================================
// STATE
// =====================================================================
int  DiscordIntegration::runCounter   = 0;
bool DiscordIntegration::show_charname  = false;
bool DiscordIntegration::show_charlevel = true;
bool DiscordIntegration::show_charjob   = true;
bool DiscordIntegration::show_map       = true;

static long long g_startTimestamp = 0;

// =====================================================================
// REFRESH LOOP
// =====================================================================
void DiscordIntegration::RefreshLoop() {
    static std::string lastDetails;
    static std::string lastState;

    for (;;) {
        Discord_RunCallbacks();

        std::string details;
        std::string state;

        if (seh_isLoggedIn() && runCounter >= WARMUP_TICKS) {
            std::vector<std::string> parts;

            if (show_charname) {
                std::string name = getCharName();
                if (!name.empty()) parts.push_back(name);
            }
            if (show_charjob) {
                std::string job = getJobName();
                if (!job.empty()) parts.push_back(job);
            }
            if (show_charlevel) {
                parts.push_back("Lv. " + std::to_string(seh_level()));
            }

            for (size_t i = 0; i < parts.size(); ++i) {
                if (i) details += "  |  ";
                details += parts[i];
            }
            if (details.empty()) details = "In Game";

            if (show_map) {
                std::string map = getMapName();
                if (!map.empty()) state = "In " + map;
            }
        } else {
            details = "In the Menus";
        }

        DiscordRichPresence presence;
        memset(&presence, 0, sizeof(presence));
        presence.type           = PLAYING;
        presence.details        = details.c_str();
        presence.state          = state.c_str();
        presence.largeImageKey  = LARGE_IMAGE_KEY;
        presence.largeImageText = SERVER_NAME;
        presence.startTimestamp = g_startTimestamp;

        // Dedupe: only push when something changed (respects the rate limit).
        if (details != lastDetails || state != lastState) {
            Discord_UpdatePresence(&presence);
            lastDetails = details;
            lastState   = state;
        }

        ++runCounter;
        std::this_thread::sleep_for(std::chrono::seconds(REFRESH_SECONDS));
    }
}

// =====================================================================
// INSTALL
// =====================================================================
void DiscordIntegration::Start() {
    DiscordEventHandlers handlers;
    memset(&handlers, 0, sizeof(handlers));
    // autoRegister = false: we don't register a discord-<id> URL protocol.
    Discord_Initialize(DISCORD_APPLICATION_ID, &handlers, false, nullptr);

    auto now = std::chrono::system_clock::now();
    g_startTimestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

    std::thread(&DiscordIntegration::RefreshLoop).detach();
}

void AttachDiscordRPC() {
    DiscordIntegration::Start();
}
