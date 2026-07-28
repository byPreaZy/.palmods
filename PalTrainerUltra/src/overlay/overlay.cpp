// PalTrainerOverlay.exe
// Overlay ImGui en jeu / toujours au premier plan de style WeMod pour PalTrainerUltra.
// Construit avec Dear ImGui + DirectX 11.

#include <windows.h>
#include <shellapi.h>
#include <windowsx.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <string>
#include <fstream>
#include <vector>
#include <map>
#include <cstdint>
#include <cstdio>
#include <cmath>
#include <cctype>
#include <algorithm>
#include <cstring>
#include <sstream>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <ctime>

#include <tlhelp32.h>
#include <psapi.h>

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/backends/imgui_impl_dx11.h"
#include "../trainer/logger.hpp"

// ----------------------------------------------------------------------------
// Globals D3D11
// ----------------------------------------------------------------------------
static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static int                      g_winWidth = 0, g_winHeight = 0;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;
static bool                     g_SwapChainOccluded = false;

// ----------------------------------------------------------------------------
// État de l'application
// ----------------------------------------------------------------------------
struct PlayerState {
    bool valid = false;
    float x = 0, y = 0, z = 0;
    float speed = 0;
    float weight = 0;
    float maxWeight = 0;
    int64_t hp = 0, maxHp = 0;
    int64_t sp = 0, maxSp = 0;
    int32_t level = 0;
    std::string name;
};

struct CheatState {
    bool godMode = false;
    bool infiniteHP = false;
    bool infiniteSP = false;
    bool infiniteWeight = false;
    bool superSpeed = false;
    bool superJump = false;
    bool flyMode = false;
    bool noClip = false;
    bool teleport = false;
    bool unlockFastTravel = false;
    bool clearWeather = false;
    float speedValue = 2000.0f;
    float jumpValue = 3000.0f;
    float weightValue = 0.0f;
    float teleportDistance = 5000.0f;

    // Teleport to specific coordinates (one-shot)
    bool teleportToPending = false;
    float teleportToX = 0.0f;
    float teleportToY = 0.0f;
    float teleportToZ = 0.0f;

    // Miroir de l'état des cheats avancés de PalTrainerCore depuis paltrainer.json
    std::map<std::string, bool> advBools;
    std::map<std::string, float> advValues;
};

static PlayerState g_player;
static CheatState g_cheats;
static std::string g_dataDir;       // où vivent paltrainer.json / commands.json
static ID3D11ShaderResourceView* g_mapTexture = nullptr;
static int g_mapWidth = 4096, g_mapHeight = 4096;
static ID3D11ShaderResourceView* g_mapTreeTexture = nullptr;
static int g_mapTreeWidth = 4096, g_mapTreeHeight = 4096;
static int g_currentMapArea = 0; // 0=MainMap, 1=Tree
static int g_mapQuality = 0; // 0=SD(2048), 1=HD(4096), 2=UHD(8192)
static bool g_mapQualityChanged = false;
static bool g_alwaysOnTop = false;

// État du lanceur de style WeMod
static DWORD g_gamePid = 0;
static std::atomic<bool> g_injected{false};
static std::atomic<bool> g_attachInProgress{false};
static std::atomic<bool> g_autoAttachAttempted{false};
static std::thread g_attachThread;
static std::atomic<bool> g_showOverlay{true};
static std::string g_statusMsg;
static std::mutex g_statusMutex;
static HWND g_hwnd = nullptr;

// ----------------------------------------------------------------------------
// Mini-carte
// ----------------------------------------------------------------------------
struct Poi {
    std::string id;
    std::string label;
    std::string type;
    float x = 0, y = 0, z = 0;
};
static std::vector<Poi> g_pois;
static std::map<std::string, bool> g_poiFilter;
static float g_minimapZoom = 1.0f;
static ImVec2 g_minimapCenter = ImVec2(0.5f, 0.5f);
static bool g_minimapFollowPlayer = true;
static bool g_minimapDragging = false;
static ImVec2 g_minimapLastMouse = ImVec2(0.0f, 0.0f);
static bool g_webServerStarted = false;

// ----------------------------------------------------------------------------
// Déclarations anticipées
// ----------------------------------------------------------------------------
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ----------------------------------------------------------------------------
// Helpers
// ----------------------------------------------------------------------------
static std::string WStringToString(const std::wstring& w) {
    if (w.empty()) return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string s(size - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, &s[0], size, nullptr, nullptr);
    return s;
}

static std::wstring StringToWString(const std::string& s) {
    if (s.empty()) return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring w(size - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &w[0], size);
    return w;
}

static void SetStatus(const std::string& msg) {
    std::lock_guard<std::mutex> lock(g_statusMutex);
    g_statusMsg = msg;
}

static std::string GetStatus() {
    std::lock_guard<std::mutex> lock(g_statusMutex);
    return g_statusMsg;
}

#ifndef PALTRAINER_MINIMAP_ONLY
static const std::string& LabelFr(const std::string& key) {
    static std::map<std::string, std::string> labels = {
        { "setAllItemCounts", "Définir tous les comptes d'objets" },
        { "setLifmunkEffigyCount", "Définir les effigies Lifmunk" },
        { "unlimitedHealth", "Santé illimitée" },
        { "refillHealth", "Régénérer la santé" },
        { "unlimitedStamina", "Endurance illimitée" },
        { "unlimitedSatiety", "Faim illimitée" },
        { "refillSatiety", "Régénérer la faim" },
        { "unlimitedSanity", "Santé mentale illimitée" },
        { "temperatureAlwaysNormal", "Température corporelle normale" },
        { "healthRegenRate", "Taux de régénération de santé" },
        { "satietyDecreaseRate", "Taux de perte de faim" },
        { "noItemWeight", "Pas de poids d'objet" },
        { "noReload", "Pas de rechargement" },
        { "infiniteDurability", "Durabilité infinie" },
        { "instantCrafting", "Artisanat instantané" },
        { "instantCapture", "Capture instantanée" },
        { "captureChanceAlways", "Capture toujours réussie" },
        { "everyoneCapturable", "Tout le monde capturable" },
        { "allPalsRare", "Tous les Pals rares" },
        { "palRandomizer", "Randomiseur de Pals" },
        { "captureMultiplier", "Multiplicateur de capture" },
        { "rarePalMultiplier", "Multiplicateur de Pals rares" },
        { "noCraftingRequirements", "Artisanat sans requis" },
        { "noBuildingRequirements", "Construction sans requis" },
        { "ignoreBuildingOverlap", "Ignorer le chevauchement de construction" },
        { "massiveWorkSpeedPlayer", "Vitesse de travail massive (joueur)" },
        { "massiveWorkSpeedAll", "Vitesse de travail massive (tous)" },
        { "workSpeedRate", "Taux de vitesse de travail" },
        { "stopTime", "Arrêter le temps" },
        { "noCrimeReporting", "Pas de signalement de crimes" },
        { "instantFishing", "Pêche instantanée" },
        { "unlimitedMoney", "Argent illimité" },
        { "setHour", "Régler l'heure" },
        { "advanceHours", "Avancer les heures" },
        { "daySpeedRate", "Vitesse du jour" },
        { "nightSpeedRate", "Vitesse de la nuit" },
        { "fishSpeedPercent", "% vitesse de pêche" },
        { "xpMultiplier", "Multiplicateur d'XP" },
        { "lootDropMultiplier", "Multiplicateur de butin" },
        { "palUnlimitedHealth", "Santé illimitée des Pals" },
        { "palUnlimitedStamina", "Endurance illimitée des Pals" },
        { "palUnlimitedSatiety", "Faim illimitée des Pals" },
        { "palUnlimitedSanity", "Santé mentale illimitée des Pals" },
        { "palMaxStats", "Stats max des Pals" },
        { "superDamage", "Dégâts super" },
        { "maxWorkerSanity", "Santé mentale max des ouvriers" },
        { "unlimitedBaseHP", "PV illimités de la base" },
        { "damageMultiplier", "Multiplicateur de dégâts" },
        { "palLevelRandomMin", "Niveau aléatoire min des Pals" },
        { "palLevelRandomMax", "Niveau aléatoire max des Pals" },
        { "setLevel", "Définir le niveau" },
        { "setXP", "Définir l'XP" },
        { "setRank", "Définir le rang" },
        { "statPoints", "Points de stats" },
        { "techPoints", "Points de technologie" },
        { "ancientTechPoints", "Points de techno ancienne" },
        { "walkSpeedMultiplier", "Mult. vitesse marche" },
        { "sprintSpeedMultiplier", "Mult. vitesse sprint" },
        { "jumpHeightMultiplier", "Mult. hauteur saut" },
        { "infiniteShield", "Bouclier infini" },
        { "stealthMode", "Mode furtif" },
        { "dropRateAlways", "100% taux de butin" },
        { "foodWontSpoil", "Nourriture non périssable" },
        { "infiniteExp", "XP infinie" },
        { "oneHitKill", "Tueur d'un coup" },
        { "palInstantSkillCooldown", "Compétences Pal sans cooldown" },
        { "unlimitedBaseStats", "Stats de base illimitées" },
        { "unlockWorldTree", "Débloquer l'Arbre-Monde" },
        { "unlockAwakening", "Débloquer le donjon d'Éveil" },
        { "unlockAllTowerBosses", "Débloquer tous les boss de tours" }
    };
    auto it = labels.find(key);
    return (it != labels.end()) ? it->second : key;
}
#endif

static std::string ReadFileTextA(const char* path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return "";
    return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

static void WriteFileTextA(const char* path, const std::string& data) {
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (f) f << data;
}

static std::string FindArg(int argc, wchar_t** argv, const wchar_t* key) {
    for (int i = 1; i < argc - 1; ++i) {
        if (wcscmp(argv[i], key) == 0)
            return WStringToString(argv[i + 1]);
    }
    return "";
}

// Petit parseur d'objets JSON pour {"key":"value", "key":number, ...}
static std::vector<std::pair<std::string, std::string>> ParseSimpleJson(const std::string& text) {
    std::vector<std::pair<std::string, std::string>> out;
    size_t i = 0;
    auto skip = [&](size_t& p) {
        while (p < text.size() && (text[p] == ' ' || text[p] == '\t' || text[p] == '\n' || text[p] == '\r')) ++p;
    };
    skip(i);
    if (i >= text.size() || text[i] != '{') return out;
    ++i;
    while (i < text.size()) {
        skip(i);
        if (i < text.size() && text[i] == '}') break;
        if (text[i] != '"') { ++i; continue; }
        ++i;
        size_t keyStart = i;
        while (i < text.size() && text[i] != '"') ++i;
        std::string key = text.substr(keyStart, i - keyStart);
        if (i < text.size() && text[i] == '"') ++i;
        skip(i);
        if (i < text.size() && text[i] == ':') ++i;
        skip(i);
        std::string value;
        if (i < text.size() && text[i] == '"') {
            ++i;
            size_t valStart = i;
            while (i < text.size() && text[i] != '"') {
                if (text[i] == '\\' && i + 1 < text.size()) ++i;
                ++i;
            }
            value = text.substr(valStart, i - valStart);
            if (i < text.size() && text[i] == '"') ++i;
        } else if (i < text.size() && text[i] == '{') {
            // Ignorer l'objet imbriqué
            int depth = 1;
            size_t objStart = i;
            ++i;
            while (i < text.size() && depth > 0) {
                if (text[i] == '"') {
                    ++i;
                    while (i < text.size() && text[i] != '"') {
                        if (text[i] == '\\' && i + 1 < text.size()) ++i;
                        ++i;
                    }
                    if (i < text.size()) ++i;
                } else {
                    if (text[i] == '{') ++depth;
                    else if (text[i] == '}') --depth;
                    ++i;
                }
            }
            value = text.substr(objStart, i - objStart);
        } else if (i < text.size() && text[i] == '[') {
            int depth = 1;
            size_t arrStart = i;
            ++i;
            while (i < text.size() && depth > 0) {
                if (text[i] == '"') {
                    ++i;
                    while (i < text.size() && text[i] != '"') {
                        if (text[i] == '\\' && i + 1 < text.size()) ++i;
                        ++i;
                    }
                    if (i < text.size()) ++i;
                } else {
                    if (text[i] == '[') ++depth;
                    else if (text[i] == ']') --depth;
                    ++i;
                }
            }
            value = text.substr(arrStart, i - arrStart);
        } else {
            size_t valStart = i;
            while (i < text.size() && text[i] != ',' && text[i] != '}') ++i;
            value = text.substr(valStart, i - valStart);
            while (!value.empty() && (value.back() == ' ' || value.back() == '\t' || value.back() == '\n' || value.back() == '\r'))
                value.pop_back();
        }
        out.emplace_back(key, value);
        skip(i);
        if (i < text.size() && text[i] == ',') ++i;
    }
    return out;
}

static bool ToBool(const std::string& s) {
    return s == "true" || s == "1" || s == "True" || s == "TRUE";
}

static float ToFloat(const std::string& s, float fallback) {
    try { return std::stof(s); } catch (...) { return fallback; }
}

static int64_t ToInt64(const std::string& s, int64_t fallback) {
    try { return std::stoll(s); } catch (...) { return fallback; }
}

static int32_t ToInt32(const std::string& s, int32_t fallback) {
    try { return std::stoi(s); } catch (...) { return fallback; }
}

#ifdef PALTRAINER_MINIMAP_ONLY
// Forward declarations - defined in overlay_minimap.inl
static bool ReadPlayerFromMemory();
static std::string g_memDataSource;
static std::string g_memDataSourceStr;
static void WriteCommands();
static void ClampMinimapView();
static void RenderMiniMap();
#else
static std::string g_memDataSource;
static std::string g_memDataSourceStr;
#endif

static void ReadPlayerState() {
    std::string path = g_dataDir + "/paltrainer.json";
    std::string txt = ReadFileTextA(path.c_str());
    bool jsonReady = false;
    if (!txt.empty()) {
        auto pairs = ParseSimpleJson(txt);
        g_player.valid = false;
        for (auto& kv : pairs) {
            if (kv.first == "ready") {
                jsonReady = ToBool(kv.second);
            } else if (kv.first == "player") {
                auto inner = ParseSimpleJson(kv.second);
                for (auto& kv2 : inner) {
                    if (kv2.first == "x") g_player.x = ToFloat(kv2.second, 0.0f);
                    else if (kv2.first == "y") g_player.y = ToFloat(kv2.second, 0.0f);
                    else if (kv2.first == "z") g_player.z = ToFloat(kv2.second, 0.0f);
                    else if (kv2.first == "speed") g_player.speed = ToFloat(kv2.second, 0.0f);
                    else if (kv2.first == "weight") g_player.weight = ToFloat(kv2.second, 0.0f);
                    else if (kv2.first == "maxWeight") g_player.maxWeight = ToFloat(kv2.second, 0.0f);
                    else if (kv2.first == "hp") g_player.hp = ToInt64(kv2.second, 0);
                    else if (kv2.first == "maxHp") g_player.maxHp = ToInt64(kv2.second, 0);
                    else if (kv2.first == "sp") g_player.sp = ToInt64(kv2.second, 0);
                    else if (kv2.first == "maxSp") g_player.maxSp = ToInt64(kv2.second, 0);
                    else if (kv2.first == "level") g_player.level = ToInt32(kv2.second, 0);
                    else if (kv2.first == "name") g_player.name = kv2.second;
                }
                g_player.valid = true;
            } else if (kv.first == "cheats") {
                auto inner = ParseSimpleJson(kv.second);
                for (auto& kv2 : inner) {
                    if (kv2.first == "godMode") g_cheats.godMode = ToBool(kv2.second);
                    else if (kv2.first == "infiniteHP") g_cheats.infiniteHP = ToBool(kv2.second);
                    else if (kv2.first == "infiniteSP") g_cheats.infiniteSP = ToBool(kv2.second);
                    else if (kv2.first == "infiniteWeight") g_cheats.infiniteWeight = ToBool(kv2.second);
                    else if (kv2.first == "superSpeed") g_cheats.superSpeed = ToBool(kv2.second);
                    else if (kv2.first == "superJump") g_cheats.superJump = ToBool(kv2.second);
                    else if (kv2.first == "flyMode") g_cheats.flyMode = ToBool(kv2.second);
                    else if (kv2.first == "noClip") g_cheats.noClip = ToBool(kv2.second);
                    else if (kv2.first == "speedValue") g_cheats.speedValue = ToFloat(kv2.second, 2000.0f);
                    else if (kv2.first == "jumpValue") g_cheats.jumpValue = ToFloat(kv2.second, 3000.0f);
                    else if (kv2.first == "weightValue") g_cheats.weightValue = ToFloat(kv2.second, 0.0f);
                    else if (kv2.first == "teleport") g_cheats.teleport = ToBool(kv2.second);
                    else if (kv2.first == "teleportDistance") g_cheats.teleportDistance = ToFloat(kv2.second, 5000.0f);
                    else if (kv2.first == "unlockFastTravel") g_cheats.unlockFastTravel = ToBool(kv2.second);
                    else if (kv2.first == "clearWeather") g_cheats.clearWeather = ToBool(kv2.second);
                    else if (kv2.second == "true" || kv2.second == "false" || kv2.second == "1" || kv2.second == "0")
                        g_cheats.advBools[kv2.first] = ToBool(kv2.second);
                    else
                        g_cheats.advValues[kv2.first] = ToFloat(kv2.second, 0.0f);
                }
            }
        }
    }
#ifndef PALTRAINER_MINIMAP_ONLY
    (void)jsonReady;
#else
    if (!jsonReady) {
        ReadPlayerFromMemory();
    }
    g_memDataSourceStr = g_memDataSource;

    // Write paltrainer.json for the web server (autonomous mode, no DLL)
    {
        std::stringstream ss;
        ss << "{\"ready\":" << (g_player.valid ? "true" : "false");
        if (g_player.valid) {
            ss << ",\"player\":{";
            ss << "\"x\":" << g_player.x << ",";
            ss << "\"y\":" << g_player.y << ",";
            ss << "\"z\":" << g_player.z << ",";
            ss << "\"speed\":" << g_player.speed << ",";
            ss << "\"weight\":" << g_player.weight << ",";
            ss << "\"maxWeight\":" << g_player.maxWeight << ",";
            ss << "\"hp\":" << g_player.hp << ",";
            ss << "\"maxHp\":" << g_player.maxHp << ",";
            ss << "\"sp\":" << g_player.sp << ",";
            ss << "\"maxSp\":" << g_player.maxSp << ",";
            ss << "\"level\":" << g_player.level << ",";
            ss << "\"name\":\"" << g_player.name << "\"";
            ss << "}";
        }
        ss << "}";
        std::string outPath = g_dataDir + "/paltrainer.json";
        FILE* f = nullptr;
        if (fopen_s(&f, outPath.c_str(), "w") == 0 && f) {
            fputs(ss.str().c_str(), f);
            fclose(f);
        }
    }
#endif
}

static void WriteCommands() {
    std::stringstream ss;
    ss << "{\n";
    ss << "  \"godMode\": " << (g_cheats.godMode ? "true" : "false") << ",\n";
    ss << "  \"infiniteHP\": " << (g_cheats.infiniteHP ? "true" : "false") << ",\n";
    ss << "  \"infiniteSP\": " << (g_cheats.infiniteSP ? "true" : "false") << ",\n";
    ss << "  \"infiniteWeight\": " << (g_cheats.infiniteWeight ? "true" : "false") << ",\n";
    ss << "  \"superSpeed\": " << (g_cheats.superSpeed ? "true" : "false") << ",\n";
    ss << "  \"superJump\": " << (g_cheats.superJump ? "true" : "false") << ",\n";
    ss << "  \"flyMode\": " << (g_cheats.flyMode ? "true" : "false") << ",\n";
    ss << "  \"noClip\": " << (g_cheats.noClip ? "true" : "false") << ",\n";
    ss << "  \"speedValue\": " << g_cheats.speedValue << ",\n";
    ss << "  \"jumpValue\": " << g_cheats.jumpValue << ",\n";
    ss << "  \"weightValue\": " << g_cheats.weightValue << ",\n";
    ss << "  \"teleport\": " << (g_cheats.teleport ? "true" : "false") << ",\n";
    ss << "  \"teleportDistance\": " << g_cheats.teleportDistance << ",\n";
    ss << "  \"unlockFastTravel\": " << (g_cheats.unlockFastTravel ? "true" : "false") << ",\n";
    ss << "  \"clearWeather\": " << (g_cheats.clearWeather ? "true" : "false");

    for (const auto& kv : g_cheats.advBools) {
        ss << ",\n";
        ss << "  \"" << kv.first << "\": " << (kv.second ? "true" : "false");
    }
    for (const auto& kv : g_cheats.advValues) {
        // Clés d'actions one-shot : n'émettre que si actuellement >= 0, puis réinitialiser pour qu'elles ne se déclenchent qu'une fois
        bool isAction = (kv.first == "setAllItemCounts" || kv.first == "setLifmunkEffigyCount");
        if (isAction && kv.second < 0.0f) continue;
        ss << ",\n";
        ss << "  \"" << kv.first << "\": " << kv.second;
        if (isAction) g_cheats.advValues[kv.first] = -1.0f;
    }

    if (g_cheats.teleportToPending) {
        ss << ",\n";
        ss << "  \"teleportToX\": " << g_cheats.teleportToX << ",\n";
        ss << "  \"teleportToY\": " << g_cheats.teleportToY << ",\n";
        ss << "  \"teleportToZ\": " << g_cheats.teleportToZ;
        g_cheats.teleportToPending = false;
    }

    ss << "\n}\n";
    std::string path = g_dataDir + "/commands.json";
    WriteFileTextA(path.c_str(), ss.str());
}

// ----------------------------------------------------------------------------
// Lanceur de style WeMod : détection du processus, scanner, injection de DLL
// ----------------------------------------------------------------------------
static DWORD FindProcessId(const wchar_t* name) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(pe);
    DWORD pid = 0;
    if (Process32FirstW(snap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, name) == 0) {
                pid = pe.th32ProcessID;
                break;
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return pid;
}

static bool GetProcessImagePath(DWORD pid, std::wstring& out) {
    HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!hProc) return false;
    wchar_t path[MAX_PATH];
    DWORD size = MAX_PATH;
    bool ok = QueryFullProcessImageNameW(hProc, 0, path, &size) != 0;
    CloseHandle(hProc);
    if (ok) out = path;
    return ok;
}

static bool NeedRescanOffsets(const std::wstring& gamePath) {
    std::string offsetsPath = g_dataDir + "/runtime_offsets.json";
    WIN32_FILE_ATTRIBUTE_DATA gameAttr, offAttr;
    if (!GetFileAttributesExW(gamePath.c_str(), GetFileExInfoStandard, &gameAttr)) return true;
    if (!GetFileAttributesExA(offsetsPath.c_str(), GetFileExInfoStandard, &offAttr)) return true;
    ULARGE_INTEGER gameTime, offTime;
    gameTime.LowPart = gameAttr.ftLastWriteTime.dwLowDateTime;
    gameTime.HighPart = gameAttr.ftLastWriteTime.dwHighDateTime;
    offTime.LowPart = offAttr.ftLastWriteTime.dwLowDateTime;
    offTime.HighPart = offAttr.ftLastWriteTime.dwHighDateTime;
    return offTime.QuadPart < gameTime.QuadPart;
}

static bool RunScanner(const std::wstring& gamePath) {
    std::wstring scanner = StringToWString(g_dataDir) + L"\\PalOffsetScanner.exe";
    std::wstring args = L"\"" + scanner + L"\" \"" + gamePath + L"\"";
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi = {};
    if (!CreateProcessW(scanner.c_str(), &args[0], nullptr, nullptr, FALSE, 0, nullptr, StringToWString(g_dataDir).c_str(), &si, &pi)) {
        return false;
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return exitCode == 0;
}

static bool InjectDll(DWORD pid, const std::wstring& dllPath) {
    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProc) return false;
    SIZE_T len = (dllPath.size() + 1) * sizeof(wchar_t);
    LPVOID remote = VirtualAllocEx(hProc, NULL, len, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    bool ok = false;
    if (remote && WriteProcessMemory(hProc, remote, dllPath.c_str(), len, NULL)) {
        HMODULE hKernel = GetModuleHandleW(L"kernel32.dll");
        LPTHREAD_START_ROUTINE load = (LPTHREAD_START_ROUTINE)GetProcAddress(hKernel, "LoadLibraryW");
        if (load) {
            HANDLE hThread = CreateRemoteThread(hProc, NULL, 0, load, remote, 0, NULL);
            if (hThread) {
                WaitForSingleObject(hThread, 30000);
                CloseHandle(hThread);
                ok = true;
            }
        }
    }
    if (remote) VirtualFreeEx(hProc, remote, 0, MEM_RELEASE);
    CloseHandle(hProc);
    return ok;
}

static void AttachWorker() {
    g_attachInProgress = true;
    SetStatus("Recherche de Palworld...");
    Log("AttachWorker: searching for Palworld");

    DWORD pid = FindProcessId(L"Palworld-Win64-Shipping.exe");
    if (!pid) {
        SetStatus("Palworld n'est pas lancé. Lance le jeu, puis clique sur Jouer.");
        Log("AttachWorker: Palworld not found");
        g_attachInProgress = false;
        return;
    }
    g_gamePid = pid;
    Log("AttachWorker: Palworld found pid=%lu", pid);

    SetStatus("Palworld trouvé. Résolution du chemin de l'exécutable...");
    std::wstring gamePath;
    if (!GetProcessImagePath(pid, gamePath)) {
        SetStatus("Échec de la récupération du chemin de Palworld.");
        Log("AttachWorker: failed to get game path");
        g_attachInProgress = false;
        return;
    }
    Log("AttachWorker: game path resolved");

    if (NeedRescanOffsets(gamePath)) {
        SetStatus("Scan des offsets à l'exécution de Palworld...");
        Log("AttachWorker: starting offset scan");
        if (!RunScanner(gamePath)) {
            SetStatus("Échec du scanner d'offsets. Lance PalTrainerOverlay.exe en tant qu'administrateur et vérifie le chemin de Palworld.");
            Log("AttachWorker: offset scan FAILED");
            g_attachInProgress = false;
            return;
        }
        Log("AttachWorker: offset scan complete");
    }

    SetStatus("Injection de PalTrainerCore.dll...");
    std::wstring dllPath = StringToWString(g_dataDir) + L"\\PalTrainerCore.dll";
    if (!InjectDll(pid, dllPath)) {
        SetStatus("Échec de l'injection. Assure-toi que PalTrainerOverlay.exe est exécuté en tant qu'administrateur.");
        Log("AttachWorker: DLL injection FAILED for pid=%lu", pid);
        g_attachInProgress = false;
        return;
    }
    Log("AttachWorker: injection successful");

    SetStatus("Attaché ! Chargement du trainer...");
    g_injected = true;
    g_attachInProgress = false;
}

static void StartAttach() {
    if (g_attachInProgress || g_injected) return;
    if (g_attachThread.joinable()) g_attachThread.join();
    g_attachThread = std::thread(AttachWorker);
}

// ----------------------------------------------------------------------------
// Transformations de coordonnées — bounds-derived Palworld 1.0
// ----------------------------------------------------------------------------
struct MapAreaBounds {
    float minX, minY, maxX, maxY;
};
static const MapAreaBounds MAP_AREAS[] = {
    // MainMap (Palpagos: inclut Sakurajima, Feybreak, Sunreach)
    { -1099400.0f, -724400.0f, 349400.0f, 724400.0f },
    // Tree (World Tree / Arbre Monde)
    { 347351.5f, -818197.0f, 689148.5f, -476400.0f },
};
static const int NUM_MAP_AREAS = 2;
static const float MAP_TEX_SIZE = 8192.0f;

static int WorldToMapArea(float worldX, float worldY) {
    // Tree first (priority)
    for (int i = 1; i >= 0; --i) {
        const auto& b = MAP_AREAS[i];
        if (worldX >= b.minX && worldX <= b.maxX && worldY >= b.minY && worldY <= b.maxY)
            return i;
    }
    return 0; // fallback MainMap
}

static void WorldToMapUV(float worldX, float worldY, float& u, float& v) {
    int area = WorldToMapArea(worldX, worldY);
    const auto& b = MAP_AREAS[area];
    float cmPerPx = (b.maxX - b.minX) / MAP_TEX_SIZE;
    float px = (worldY - b.minY) / cmPerPx;
    float py = (worldX - b.minX) / cmPerPx;
    u = std::clamp(px / MAP_TEX_SIZE, 0.0f, 1.0f);
    v = std::clamp(1.0f - py / MAP_TEX_SIZE, 0.0f, 1.0f);
}

static void WorldToMapUVArea(float worldX, float worldY, int area, float& u, float& v) {
    const auto& b = MAP_AREAS[area];
    float cmPerPx = (b.maxX - b.minX) / MAP_TEX_SIZE;
    float px = (worldY - b.minY) / cmPerPx;
    float py = (worldX - b.minX) / cmPerPx;
    u = std::clamp(px / MAP_TEX_SIZE, 0.0f, 1.0f);
    v = std::clamp(1.0f - py / MAP_TEX_SIZE, 0.0f, 1.0f);
}

static void MapUVToWorldArea(float u, float v, int area, float& worldX, float& worldY) {
    const auto& b = MAP_AREAS[area];
    float cmPerPx = (b.maxX - b.minX) / MAP_TEX_SIZE;
    float px = u * MAP_TEX_SIZE;
    float py = (1.0f - v) * MAP_TEX_SIZE;
    worldY = b.minY + px * cmPerPx;
    worldX = b.minX + py * cmPerPx;
}

// ----------------------------------------------------------------------------
// Chargement des textures
// ----------------------------------------------------------------------------
static bool LoadMapTexture(const char* path, int width, int height, ID3D11ShaderResourceView** out_srv) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    size_t expected = (size_t)width * height * 4;
    std::vector<uint8_t> pixels(expected);
    f.read((char*)pixels.data(), expected);
    if ((size_t)f.gcount() != expected) return false;

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA sub = {};
    sub.pSysMem = pixels.data();
    sub.SysMemPitch = width * 4;

    ID3D11Texture2D* tex = nullptr;
    if (FAILED(g_pd3dDevice->CreateTexture2D(&desc, &sub, &tex)))
        return false;

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    HRESULT hr = g_pd3dDevice->CreateShaderResourceView(tex, &srvDesc, out_srv);
    tex->Release();
    return SUCCEEDED(hr);
}

static void ReloadMapTextures() {
    if (g_mapTexture) { g_mapTexture->Release(); g_mapTexture = nullptr; }
    if (g_mapTreeTexture) { g_mapTreeTexture->Release(); g_mapTreeTexture = nullptr; }

    const char* suffixes[] = { "2048", "4096", "8192" };
    const int sizes[] = { 2048, 4096, 8192 };

    Log("ReloadMapTextures: dataDir=%s requestedQuality=%s", g_dataDir.c_str(), suffixes[g_mapQuality]);

    // Try requested quality, then fallback to lower qualities
    for (int q = g_mapQuality; q >= 0; q--) {
        const char* suf = suffixes[q];
        int sz = sizes[q];

        std::string mapPath = g_dataDir + "/overlay_assets/map_" + suf + ".rgba";
        g_mapWidth = sz; g_mapHeight = sz;
        Log("  Trying: %s", mapPath.c_str());
        if (!LoadMapTexture(mapPath.c_str(), g_mapWidth, g_mapHeight, &g_mapTexture)) {
            mapPath = g_dataDir + "/assets/maps/map_" + suf + ".rgba";
            Log("  Trying: %s", mapPath.c_str());
            if (!LoadMapTexture(mapPath.c_str(), g_mapWidth, g_mapHeight, &g_mapTexture)) {
                continue;
            }
        }
        Log("  OK: map texture loaded (%s)", suf);

        std::string treeMapPath = g_dataDir + "/overlay_assets/map_tree_" + suf + ".rgba";
        g_mapTreeWidth = sz; g_mapTreeHeight = sz;
        Log("  Trying: %s", treeMapPath.c_str());
        if (!LoadMapTexture(treeMapPath.c_str(), g_mapTreeWidth, g_mapTreeHeight, &g_mapTreeTexture)) {
            treeMapPath = g_dataDir + "/assets/maps/map_tree_" + suf + ".rgba";
            Log("  Trying: %s", treeMapPath.c_str());
            if (!LoadMapTexture(treeMapPath.c_str(), g_mapTreeWidth, g_mapTreeHeight, &g_mapTreeTexture)) {
                Log("  WARNING: tree map texture not found at %s", suf);
            } else {
                Log("  OK: tree map texture loaded (%s)", suf);
            }
        } else {
            Log("  OK: tree map texture loaded (%s)", suf);
        }
        return;
    }

    Log("  ERROR: no map texture found at any quality!");
    g_mapWidth = 2048; g_mapHeight = 2048;
    g_mapTreeWidth = 2048; g_mapTreeHeight = 2048;
}

#include "overlay_minimap.inl"

// ----------------------------------------------------------------------------
// Helpers D3D11 (principalement de l'exemple Dear ImGui)
// ----------------------------------------------------------------------------
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    sd.Flags = 0;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2,
        D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer = nullptr;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    if (pBackBuffer) {
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
        pBackBuffer->Release();
    }
}

void CleanupRenderTarget() {
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

#ifndef PALTRAINER_MINIMAP_ONLY
static void RenderLauncher() {
    ImGui::SetNextWindowPos(ImVec2(40, 40), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(400, 180), ImGuiCond_FirstUseEver);
    ImGui::Begin("PalTrainerUltra v1.0", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
    ImGui::Text("État :");
    ImGui::TextWrapped("%s", GetStatus().c_str());
    ImGui::Separator();
    if (g_attachInProgress) {
        ImGui::Text("Traitement en cours, veuillez patienter...");
    } else if (!g_injected) {
        if (ImGui::Button("Jouer", ImVec2(160, 30))) {
            StartAttach();
        }
        ImGui::SameLine();
        if (ImGui::Button("Quitter", ImVec2(80, 30))) {
            ::PostQuitMessage(0);
        }
    } else {
        ImGui::Text("Connecté ! Basculement vers le trainer...");
    }
    ImGui::End();
}

static void RenderTrainer() {
    // ---- Fenêtre de la mini-carte ----
    RenderMiniMap();

    // ---- Fenêtre du trainer ----
    {
        bool changed = false;
        ImGui::Begin("Trainer", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        changed |= ImGui::Checkbox("Mode Dieu", &g_cheats.godMode);
        changed |= ImGui::Checkbox("PV infinis", &g_cheats.infiniteHP);
        changed |= ImGui::Checkbox("PS infinis", &g_cheats.infiniteSP);
        changed |= ImGui::Checkbox("Poids infini", &g_cheats.infiniteWeight);
        changed |= ImGui::Checkbox("Super vitesse", &g_cheats.superSpeed);
        changed |= ImGui::Checkbox("Super saut", &g_cheats.superJump);
        changed |= ImGui::Checkbox("Mode vol", &g_cheats.flyMode);
        changed |= ImGui::Checkbox("No Clip", &g_cheats.noClip);
        changed |= ImGui::SliderFloat("Vitesse", &g_cheats.speedValue, 500.0f, 5000.0f);
        changed |= ImGui::SliderFloat("Saut", &g_cheats.jumpValue, 500.0f, 5000.0f);
        changed |= ImGui::SliderFloat("Poids", &g_cheats.weightValue, -1000.0f, 10000.0f);
        if (ImGui::Button("Téléportation avant")) {
            g_cheats.teleport = true;
            changed = true;
        }
        changed |= ImGui::SliderFloat("Distance de téléportation", &g_cheats.teleportDistance, 100.0f, 10000.0f);
        changed |= ImGui::Checkbox("Déverr. voyages rapides", &g_cheats.unlockFastTravel);
        changed |= ImGui::Checkbox("Dégager la météo", &g_cheats.clearWeather);

        ImGui::Separator();
        ImGui::Text("Joueur : %s Niv%d", g_player.name.c_str(), g_player.level);
        ImGui::Text("PV %lld / %lld  PS %lld / %lld", g_player.hp, g_player.maxHp, g_player.sp, g_player.maxSp);
        ImGui::Text("Poids %.1f / %.1f", g_player.weight, g_player.maxWeight);

        if (changed) WriteCommands();

        ImGui::End();
    }

    // ---- Fenêtre des cheats avancés de PalTrainerCore ----
    {
        if (g_cheats.advValues.find("setAllItemCounts") == g_cheats.advValues.end())
            g_cheats.advValues["setAllItemCounts"] = -1.0f;
        if (g_cheats.advValues.find("setLifmunkEffigyCount") == g_cheats.advValues.end())
            g_cheats.advValues["setLifmunkEffigyCount"] = -1.0f;

        bool changed = false;
        ImGui::SetNextWindowPos(ImVec2(370, 40), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(330, 520), ImGuiCond_FirstUseEver);
        ImGui::Begin("Avancé", nullptr, 0);

        if (ImGui::CollapsingHeader("Actions", ImGuiTreeNodeFlags_DefaultOpen)) {
            changed |= ImGui::InputFloat("Définir tous les comptes d'objets##setAllItemCounts", &g_cheats.advValues["setAllItemCounts"], 1, 100, "%.0f");
            if (ImGui::Button("Appliquer##setAllItemCounts")) changed = true;
            ImGui::SameLine();
            if (ImGui::Button("Réinitialiser##setAllItemCounts")) { g_cheats.advValues["setAllItemCounts"] = -1.0f; changed = true; }

            changed |= ImGui::InputFloat("Définir les effigies Lifmunk##setLifmunkEffigyCount", &g_cheats.advValues["setLifmunkEffigyCount"], 1, 10, "%.0f");
            if (ImGui::Button("Appliquer##setLifmunk")) changed = true;
            ImGui::SameLine();
            if (ImGui::Button("Réinitialiser##setLifmunk")) { g_cheats.advValues["setLifmunkEffigyCount"] = -1.0f; changed = true; }
        }

        if (ImGui::CollapsingHeader("Interrupteurs", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::BeginChild("AdvToggles", ImVec2(0, 180), true);
            for (auto& kv : g_cheats.advBools) {
                changed |= ImGui::Checkbox(LabelFr(kv.first).c_str(), &kv.second);
            }
            ImGui::EndChild();
        }

        if (ImGui::CollapsingHeader("Valeurs", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::BeginChild("AdvValues", ImVec2(0, 180), true);
            for (auto& kv : g_cheats.advValues) {
                if (kv.first == "setAllItemCounts" || kv.first == "setLifmunkEffigyCount") continue;
                changed |= ImGui::InputFloat(LabelFr(kv.first).c_str(), &kv.second, 0, 0, "%.2f");
            }
            ImGui::EndChild();
        }

        if (changed) WriteCommands();
        ImGui::End();
    }
}
#endif

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
        case WM_SIZE:
            if (wParam == SIZE_MINIMIZED) return 0;
            g_ResizeWidth = (UINT)LOWORD(lParam);
            g_ResizeHeight = (UINT)HIWORD(lParam);
            return 0;
        case WM_SYSCOMMAND:
            if ((wParam & 0xFFF0) == SC_KEYMENU) return 0;
            break;
        case WM_HOTKEY:
            if (wParam == 1) {
                g_showOverlay = !g_showOverlay;
                ::ShowWindow(g_hwnd, g_showOverlay ? SW_SHOW : SW_HIDE);
            }
            return 0;
        case WM_DESTROY:
            ::PostQuitMessage(0);
            return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}

// ----------------------------------------------------------------------------
// Principal
// ----------------------------------------------------------------------------
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR lpCmdLine, int nCmdShow) {
    // Répertoire de données : le même que l'EXE par défaut, ou --data <dir>
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(hInstance, exePath, MAX_PATH);
    std::wstring wdir(exePath);
    size_t pos = wdir.find_last_of(L"\\/");
    if (pos != std::wstring::npos) wdir = wdir.substr(0, pos);
    g_dataDir = WStringToString(wdir);

    LogInit(g_dataDir.c_str());
    Log("=== PalTrainerOverlay v1.0 starting ===");
    Log("Data dir: %s", g_dataDir.c_str());

    // Vérification des fichiers critiques
    {
        std::string mapDir = g_dataDir + "/assets/maps";
        const char* qualities[] = { "2048", "4096", "8192" };
        for (int i = 0; i < 3; i++) {
            std::string mapFile = mapDir + "/map_" + qualities[i] + ".rgba";
            std::ifstream testFile(mapFile.c_str(), std::ios::binary);
            if (!testFile) {
                Log("WARNING: Missing: %s", mapFile.c_str());
            } else {
                Log("OK: Map texture found: %s", mapFile.c_str());
            }
            testFile.close();
        }
    }

    // Analyser l'argument --data s'il est fourni
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(lpCmdLine, &argc);
    if (argv) {
        std::string dataArg = FindArg(argc, argv, L"--data");
        if (!dataArg.empty()) g_dataDir = dataArg;
        LocalFree(argv);
    }

    // Créer la fenêtre
#ifdef PALTRAINER_MINIMAP_ONLY
    const wchar_t* windowClass = L"PalTrainerMiniMap";
    const wchar_t* windowTitle = L"PalTrainer MiniMap";
    int winW = 1, winH = 1;
    DWORD exFlags = WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW;
    DWORD winFlags = WS_POPUP;
    int posX = 0;
    int posY = 0;
#else
    const wchar_t* windowClass = L"PalTrainerOverlay";
    const wchar_t* windowTitle = L"PalTrainerUltra v1.0";
    int winW = 500, winH = 700;
    DWORD exFlags = WS_EX_TOPMOST | WS_EX_LAYERED;
    DWORD winFlags = WS_POPUP | WS_CAPTION | WS_THICKFRAME | WS_SYSMENU | WS_VISIBLE;
    int posX = 100, posY = 100;
#endif
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, hInstance, nullptr, nullptr, nullptr, nullptr, windowClass, nullptr };
    ::RegisterClassExW(&wc);

    HWND hwnd = ::CreateWindowExW(
        exFlags,
        wc.lpszClassName,
        windowTitle,
        winFlags,
        posX, posY, winW, winH,
        nullptr, nullptr, wc.hInstance, nullptr);
    g_hwnd = hwnd;

    // Suivre la taille de la fenêtre native
    RECT rc;
    GetClientRect(hwnd, &rc);
    g_winWidth = rc.right - rc.left;
    g_winHeight = rc.bottom - rc.top;

    // Enregistrer le raccourci Insert pour basculer la visibilité de l'overlay
    ::RegisterHotKey(hwnd, 1, 0, VK_INSERT);

#ifdef PALTRAINER_MINIMAP_ONLY
    // Colorkey magenta — fenêtre host invisible
    ::SetLayeredWindowAttributes(hwnd, RGB(255, 0, 255), 0, LWA_COLORKEY);
#else
    // Trainer overlay : colorkey magenta
    ::SetLayeredWindowAttributes(hwnd, RGB(255, 0, 255), 0, LWA_COLORKEY);
#endif

    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Afficher la fenêtre
#ifdef PALTRAINER_MINIMAP_ONLY
    ::ShowWindow(hwnd, SW_SHOWNOACTIVATE);
#else
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
#endif
    ::UpdateWindow(hwnd);

    // Configurer ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
#ifdef PALTRAINER_MINIMAP_ONLY
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
#endif

    ImGui::StyleColorsDark();
#ifdef PALTRAINER_MINIMAP_ONLY
    {
        ImGuiStyle& st = ImGui::GetStyle();
        st.WindowRounding = 10.0f;
        st.FrameRounding = 6.0f;
        st.GrabRounding = 5.0f;
        st.ChildRounding = 8.0f;
        st.PopupRounding = 8.0f;
        st.ScrollbarRounding = 8.0f;
        st.TabRounding = 6.0f;
        st.WindowBorderSize = 0.0f;
        st.FrameBorderSize = 0.0f;
        st.WindowPadding = ImVec2(8, 8);
#ifdef PALTRAINER_MINIMAP_ONLY
        // When viewports are enabled, tweak platform windows style
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 8.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
#endif
        st.FramePadding = ImVec2(6, 3);
        st.ItemSpacing = ImVec2(6, 5);
        st.ItemInnerSpacing = ImVec2(4, 4);
        st.ScrollbarSize = 10.0f;
        st.GrabMinSize = 8.0f;

        ImVec4* c = st.Colors;
        // Dark theme with golden accents — professional look
        c[ImGuiCol_WindowBg]        = ImVec4(0.03f, 0.03f, 0.04f, 0.92f);
        c[ImGuiCol_ChildBg]         = ImVec4(0.05f, 0.05f, 0.06f, 0.95f);
        c[ImGuiCol_PopupBg]         = ImVec4(0.06f, 0.06f, 0.07f, 0.98f);
        c[ImGuiCol_Border]          = ImVec4(0.20f, 0.18f, 0.12f, 0.60f);
        c[ImGuiCol_BorderShadow]    = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
        c[ImGuiCol_Text]            = ImVec4(0.92f, 0.90f, 0.85f, 1.0f);
        c[ImGuiCol_TextDisabled]    = ImVec4(0.50f, 0.48f, 0.42f, 1.0f);
        c[ImGuiCol_Button]          = ImVec4(0.14f, 0.14f, 0.16f, 1.0f);
        c[ImGuiCol_ButtonHovered]   = ImVec4(0.22f, 0.20f, 0.14f, 1.0f);
        c[ImGuiCol_ButtonActive]    = ImVec4(0.30f, 0.26f, 0.12f, 1.0f);
        c[ImGuiCol_Header]          = ImVec4(0.12f, 0.12f, 0.14f, 1.0f);
        c[ImGuiCol_HeaderHovered]   = ImVec4(0.20f, 0.18f, 0.12f, 1.0f);
        c[ImGuiCol_HeaderActive]    = ImVec4(0.28f, 0.24f, 0.12f, 1.0f);
        c[ImGuiCol_FrameBg]         = ImVec4(0.08f, 0.08f, 0.10f, 1.0f);
        c[ImGuiCol_FrameBgHovered]  = ImVec4(0.14f, 0.14f, 0.16f, 1.0f);
        c[ImGuiCol_FrameBgActive]   = ImVec4(0.18f, 0.18f, 0.20f, 1.0f);
        c[ImGuiCol_CheckMark]       = ImVec4(0.85f, 0.72f, 0.30f, 1.0f);
        c[ImGuiCol_SliderGrab]      = ImVec4(0.50f, 0.42f, 0.18f, 1.0f);
        c[ImGuiCol_SliderGrabActive]= ImVec4(0.85f, 0.72f, 0.30f, 1.0f);
        c[ImGuiCol_ScrollbarBg]     = ImVec4(0.04f, 0.04f, 0.05f, 0.5f);
        c[ImGuiCol_ScrollbarGrab]   = ImVec4(0.18f, 0.18f, 0.20f, 1.0f);
        c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.28f, 0.26f, 0.18f, 1.0f);
        c[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.38f, 0.34f, 0.20f, 1.0f);
        c[ImGuiCol_Separator]       = ImVec4(0.18f, 0.16f, 0.10f, 0.50f);
        c[ImGuiCol_SeparatorHovered]= ImVec4(0.30f, 0.26f, 0.14f, 0.80f);
        c[ImGuiCol_SeparatorActive] = ImVec4(0.50f, 0.42f, 0.18f, 1.0f);
        c[ImGuiCol_Tab]             = ImVec4(0.10f, 0.10f, 0.12f, 1.0f);
        c[ImGuiCol_TabHovered]      = ImVec4(0.22f, 0.20f, 0.14f, 1.0f);
        c[ImGuiCol_TabActive]       = ImVec4(0.18f, 0.16f, 0.10f, 1.0f);
        c[ImGuiCol_TableHeaderBg]   = ImVec4(0.08f, 0.08f, 0.10f, 1.0f);
        c[ImGuiCol_TableBorderStrong] = ImVec4(0.20f, 0.18f, 0.12f, 0.60f);
        c[ImGuiCol_TableBorderLight]  = ImVec4(0.14f, 0.12f, 0.08f, 0.40f);
    }
#endif
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Charger la texture de la carte
    ReloadMapTextures();
    LoadPois();
    LoadIconTextures();
    LoadFavorites();
    StartWebServer();

#ifdef PALTRAINER_MINIMAP_ONLY
    SetStatus("Mini-carte prête. En attente des données du jeu...");
#else
    SetStatus("Prêt. En attente de Palworld...");

    // Auto-attach au démarrage (réessaie jusqu'à ce que Palworld soit lancé)
    if (!g_autoAttachAttempted) {
        g_autoAttachAttempted = true;
        StartAttach();
    }
#endif

    // Boucle principale
    bool done = false;
    while (!done) {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT) done = true;
        }
        if (done) break;

        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) {
            Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        if (g_ResizeWidth != 0 && g_ResizeHeight != 0) {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_winWidth = g_ResizeWidth;
            g_winHeight = g_ResizeHeight;
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        if (g_mapQualityChanged) {
            g_mapQualityChanged = false;
            ReloadMapTextures();
        }

#ifdef PALTRAINER_MINIMAP_ONLY
        // Hotkeys (global, work even when window not focused)
        {
            static bool f1Prev = false, f2Prev = false, f3Prev = false, f4Prev = false, f5Prev = false;
            bool f1 = (GetAsyncKeyState(VK_F1) & 0x8000) != 0;
            bool f2 = (GetAsyncKeyState(VK_F2) & 0x8000) != 0;
            bool f3 = (GetAsyncKeyState(VK_F3) & 0x8000) != 0;
            bool f4 = (GetAsyncKeyState(VK_F4) & 0x8000) != 0;
            bool f5 = (GetAsyncKeyState(VK_F5) & 0x8000) != 0;
            if (f1 && !f1Prev) g_minimapFollowPlayer = !g_minimapFollowPlayer;
            if (f2 && !f2Prev) { g_minimapZoom = std::min(g_minimapZoom * 1.3f, 20.0f); ClampMinimapView(); }
            if (f3 && !f3Prev) { g_minimapZoom = std::max(g_minimapZoom / 1.3f, 1.0f); ClampMinimapView(); }
            if (f4 && !f4Prev) {
                g_alwaysOnTop = !g_alwaysOnTop;
                SetWindowPos(g_hwnd, g_alwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            }
            if (f5 && !f5Prev) g_showPals = !g_showPals;
            f1Prev = f1; f2Prev = f2; f3Prev = f3; f4Prev = f4; f5Prev = f5;
        }

        ReadPlayerState();
        RenderMiniMap();
#else
        if (g_injected) {
            ReadPlayerState();
            RenderTrainer();
        } else {
            RenderLauncher();
        }
#endif

        // Rendu
        ImGui::Render();
#ifdef PALTRAINER_MINIMAP_ONLY
        const float clear_color[4] = { 1.0f, 0.0f, 1.0f, 1.0f }; // magenta = transparent (colorkey)
#else
        const float clear_color[4] = { 1.0f, 0.0f, 1.0f, 1.0f }; // magenta pour la couleur clé
#endif
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        HRESULT hr = g_pSwapChain->Present(1, 0);
        if (hr == DXGI_STATUS_OCCLUDED) g_SwapChainOccluded = true;

#ifdef PALTRAINER_MINIMAP_ONLY
        // Update and render additional platform windows
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
#endif

#ifdef PALTRAINER_MINIMAP_ONLY
        Sleep(1);
#else
        Sleep(8);
#endif
    }

    // Nettoyage
#ifdef PALTRAINER_MINIMAP_ONLY
    if (g_memProcessHandle) { CloseHandle(g_memProcessHandle); g_memProcessHandle = nullptr; }
#endif
    if (g_mapTexture) g_mapTexture->Release();
    if (g_mapTreeTexture) g_mapTreeTexture->Release();
    ReleaseIconTextures();
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
    Log("Overlay shutting down cleanly");
    LogClose();
    return 0;
}
