#include "pch.h"
#include "ztl/ztl.h"

#include <string>
#include <unordered_map>

#include "wvs/util.h"

// =====================================================
// CACHE
// =====================================================
static std::unordered_map<int, std::string> g_mapNames;
static std::unordered_map<int, std::string> g_mobNames;
static std::unordered_map<int, int> g_mobLevels;
static std::unordered_map<int, std::string> g_npcNames;

static std::unordered_map<int, int> g_mobToCard;
static std::unordered_map<int, IWzCanvasPtr> g_cardIcons;

// =====================================================
// HELPERS
// =====================================================
namespace wz {

inline IWzPropertyPtr Get(const wchar_t* path) {
    return get_rm()->GetObjectA(path).GetUnknown();
}

inline IWzPropertyPtr GetItem(IWzPropertyPtr parent, const wchar_t* key) {
    return parent ? parent->item[key].GetUnknown() : nullptr;
}

inline std::string GetStr(IWzPropertyPtr p, const wchar_t* key, const char* def = "Unknown") {
    if (!p)
        return def;

    try {
        Ztl_variant_t v = p->item[key];
        if (v.vt == VT_BSTR)
            return (const char*)_bstr_t(v);
    } catch (...) {
    }

    return def;
}

inline int GetInt(IWzPropertyPtr p, const wchar_t* key, int def = 0) {
    if (!p)
        return def;
    return get_int32(p->item[key], def);
}
} // namespace wz

// =====================================================
// MAP NAME
// =====================================================
std::string GetMapById(int id) {

    auto it = g_mapNames.find(id);
    if (it != g_mapNames.end())
        return it->second;

    static const wchar_t* categories[] = {
        L"maple", L"victoria", L"ossyria", L"elin",
        L"weddingGL", L"MasteriaGL", L"HalloweenGL",
        L"jp", L"etc", L"singapore", L"event", L"Episode1GL"
    };

    IWzPropertyPtr root = wz::Get(L"String/Map.img");
    if (!root)
        return "Unknown";

    Ztl_bstr_t sId = std::to_wstring(id).c_str();

    for (auto cat : categories) {

        IWzPropertyPtr category = wz::GetItem(root, cat);
        if (!category)
            continue;

        IWzPropertyPtr map = category->item[sId].GetUnknown();
        if (!map)
            continue;

        std::string street = wz::GetStr(map, L"streetName");
        std::string name = wz::GetStr(map, L"mapName");

        std::string result = street + " - " + name;
        g_mapNames[id] = result;

        return result;
    }

    return "Unknown";
}

// =====================================================
// MOB NAME
// =====================================================
std::string GetMobNameById(int id) {

    auto it = g_mobNames.find(id);
    if (it != g_mobNames.end())
        return it->second;

    IWzPropertyPtr root = wz::Get(L"String/Mob.img");
    if (!root)
        return "Unknown";

    Ztl_bstr_t sId = std::to_wstring(id).c_str();

    IWzPropertyPtr mob = root->item[sId].GetUnknown();
    if (!mob)
        return "Unknown";

    std::string name = wz::GetStr(mob, L"name");

    g_mobNames[id] = name;
    return name;
}

// =====================================================
// MOB LEVEL
// =====================================================
int GetMobLevelById(int id) {

    auto it = g_mobLevels.find(id);
    if (it != g_mobLevels.end())
        return it->second;

    wchar_t path[64];
    swprintf(path, L"Mob/%07d.img", id);

    IWzPropertyPtr mob = wz::Get(path);
    if (!mob)
        return 0;

    IWzPropertyPtr info = wz::GetItem(mob, L"info");
    int level = wz::GetInt(info, L"level", 0);

    g_mobLevels[id] = level;
    return level;
}

// =====================================================
// NPC NAME
// =====================================================
std::string GetNpcById(int id) {

    auto it = g_npcNames.find(id);
    if (it != g_npcNames.end())
        return it->second;

    IWzPropertyPtr root = wz::Get(L"String/Npc.img");
    if (!root)
        return "Unknown";

    Ztl_bstr_t sId = std::to_wstring(id).c_str();

    IWzPropertyPtr npc = root->item[sId].GetUnknown();
    if (!npc)
        return "Unknown";

    std::string name = wz::GetStr(npc, L"name");

    g_npcNames[id] = name;
    return name;
}

// =====================================================
// MOB -> CARD ID
// =====================================================
int GetCardIdByMobId(int mobId) {

    auto it = g_mobToCard.find(mobId);
    if (it != g_mobToCard.end())
        return it->second;

    IWzPropertyPtr consume = wz::Get(L"Item/Consume.img");
    if (!consume)
        return 0;

    IWzPropertyPtr group = wz::GetItem(consume, L"0238");
    if (!group)
        return 0;

    int count = group->Getcount();

    for (int i = 0; i < count; i++) {

        wchar_t idx[16];
        _itow_s(i, idx, 10);

        IWzPropertyPtr item = group->item[idx].GetUnknown();
        if (!item)
            continue;

        IWzPropertyPtr info = wz::GetItem(item, L"info");
        if (!info)
            continue;

        int mob = wz::GetInt(info, L"mob", 0);

        if (mob == mobId) {
            g_mobToCard[mobId] = i;
            return i;
        }
    }

    return 0;
}

// =====================================================
// CARD ICON
// =====================================================
IWzCanvasPtr GetCardIconByCardId(int cardId) {

    auto it = g_cardIcons.find(cardId);
    if (it != g_cardIcons.end())
        return it->second;

    try {
        wchar_t path[128];
        swprintf(path, L"Item/Consume.img/0238/%d/info/iconRaw", cardId);

        IWzCanvasPtr icon = get_rm()->GetObjectA(path).GetUnknown();

        if (icon) {
            g_cardIcons[cardId] = icon;
            return icon;
        }

    } catch (...) {
    }

    return nullptr;
}
