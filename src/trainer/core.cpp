/*
    DLL noyau de PalTrainerUltra
    Injectée dans Palworld 1.0.1 pour lire l'état du jeu et appliquer les bascules de cheat.
    Utilise les offsets publics de CXXHeaderDump validés contre le binaire local.
*/

#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <string>
#include <fstream>
#include <sstream>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <cmath>

#include "offsets.h"
#include "types.h"
#include "engine.hpp"
#include "cheat.hpp"
#include "logger.hpp"

static std::string ReadFileText(const char* path);
static std::vector<std::pair<std::string, std::string>> ParseSimpleJson(const std::string& text);

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------
HMODULE g_hModule = nullptr;
std::atomic_bool g_running{ false };
std::thread g_worker;
char g_dataDir[MAX_PATH] = {};
char g_jsonOutPath[MAX_PATH] = {};
char g_cmdInPath[MAX_PATH] = {};

struct CheatState {
    bool godMode = false;
    bool infiniteHP = false;
    bool infiniteSP = false;
    bool infiniteWeight = false;
    bool superSpeed = false;
    bool superJump = false;
    bool flyMode = false;
    bool noClip = false;
    float speedValue = 2000.0f;
    float jumpValue = 3000.0f;
    float weightValue = 0.0f;
    bool teleport = false;
    float teleportDistance = 5000.0f;
    bool unlockFastTravel = false;
    bool clearWeather = false;

    // Teleport to specific coordinates (one-shot, from minimap click)
    bool teleportTo = false;
    float teleportToX = 0.0f;
    float teleportToY = 0.0f;
    float teleportToZ = 0.0f;
};

CheatState g_cheats;
std::mutex g_cheatMutex;

namespace PalTrainerRuntime {
    uintptr_t GWorldRva      = PalTrainerOffsets::GWorldRva;
    uintptr_t GObjectRva     = PalTrainerOffsets::GObjectRva;
    uintptr_t FNameRva       = PalTrainerOffsets::FNameRva;
    uintptr_t ProcessEventRva= PalTrainerOffsets::ProcessEventRva;
    uintptr_t AppendStringRva= PalTrainerOffsets::AppendStringRva;
    uintptr_t TickRva        = 0;

    bool Load(const char* jsonPath) {
        std::string txt = ReadFileText(jsonPath);
        if (txt.empty()) {
            Log("PalTrainerRuntime::Load: file empty or not found: %s", jsonPath);
            return false;
        }
        Log("PalTrainerRuntime::Load: parsing %s", jsonPath);
        auto pairs = ParseSimpleJson(txt);
        for (auto& kv : pairs) {
            try {
                uintptr_t v = std::stoull(kv.second, nullptr, 16);
                if (kv.first == "GWorldRva")      { GWorldRva = v;      Log("  GWorldRva = 0x%llX", (unsigned long long)v); }
                else if (kv.first == "GObjectRva")     { GObjectRva = v;     Log("  GObjectRva = 0x%llX", (unsigned long long)v); }
                else if (kv.first == "FNameRva")       { FNameRva = v;       Log("  FNameRva = 0x%llX", (unsigned long long)v); }
                else if (kv.first == "ProcessEventRva") { ProcessEventRva = v; Log("  ProcessEventRva = 0x%llX", (unsigned long long)v); }
                else if (kv.first == "AppendStringRva") { AppendStringRva = v; Log("  AppendStringRva = 0x%llX", (unsigned long long)v); }
                else if (kv.first == "TickRva")        { TickRva = v;        Log("  TickRva = 0x%llX", (unsigned long long)v); }
            } catch (...) {}
        }
        Log("PalTrainerRuntime::Load: done, GWorldRva=0x%llX", (unsigned long long)GWorldRva);
        return GWorldRva != 0;
    }
}

struct PlayerSnapshot {
    bool valid = false;
    float x = 0, y = 0, z = 0;
    float pitch = 0, yaw = 0, roll = 0;
    float speed = 0;
    float maxSpeed = 0;
    float weight = 0;
    float maxWeight = 0;
    int64_t hp = 0;
    int64_t maxHp = 0;
    int64_t sp = 0;
    int64_t maxSp = 0;
    int64_t exp = 0;
    int32_t level = 0;
    std::string name;
};

// ---------------------------------------------------------------------------
// Helpers JSON
// ---------------------------------------------------------------------------
static std::string JsonEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 2);
    for (char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c >= 0x20) out += c;
                else {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\u%04x", (unsigned char)c);
                    out += buf;
                }
        }
    }
    return out;
}

static std::string ReadFileText(const char* path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return "";
    return std::string((std::istreambuf_iterator<char>(f)),
                        std::istreambuf_iterator<char>());
}

static void WriteFileText(const char* path, const std::string& data) {
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (f) f << data;
}

// Parseur d'objets JSON très simple pour {"key": value, ...} où les valeurs sont
// bool / nombre / chaîne (les chaînes ne sont utilisées que pour des valeurs simples sans objets imbriqués).
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
        } else {
            size_t valStart = i;
            while (i < text.size() && text[i] != ',' && text[i] != '}') ++i;
            value = text.substr(valStart, i - valStart);
            // supprimer les espaces à la fin de la valeur
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
    if (s == "true" || s == "1" || s == "True" || s == "TRUE") return true;
    if (s == "toggle") return true; // géré séparément
    return false;
}

static float ToFloat(const std::string& s, float fallback) {
    try { return std::stof(s); } catch (...) { return fallback; }
}

static int32_t ToInt(const std::string& s, int32_t fallback) {
    try { return std::stoi(s); } catch (...) { return fallback; }
}

static void SyncCheatsToCore()
{
    // Mapper l'état de l'interface Ultra vers la logique PalTrainerCore
    bool refill = g_cheats.godMode || g_cheats.infiniteHP;
    Cheat::UnlimitedHealth = refill;
    Cheat::RefillHealth    = refill;
    Cheat::UnlimitedStamina = g_cheats.infiniteSP;
    Cheat::NoItemWeight    = g_cheats.infiniteWeight;
    Cheat::UnlimitedSatiety = false;
    Cheat::NoReload        = false;
    Cheat::InstantCapture  = false;
    Cheat::CaptureChanceAlways = false;
    Cheat::MassiveWorkSpeedPlayer = false;
    Cheat::MassiveWorkSpeedAll = false;
    Cheat::NoCraftingRequirements = false;
    Cheat::NoBuildingRequirements = false;
    Cheat::IgnoreBuildingOverlap  = false;
    Cheat::TemperatureAlwaysNormal = false;
    Cheat::StopTime = false;
    Cheat::NoCrimeReporting = false;
    Cheat::InstantFishing = false;
    Cheat::UnlimitedMoney = false;
    Cheat::AllPalsRare = false;
    Cheat::EveryoneCapturable = false;
    Cheat::PalRandomizer = false;
    Cheat::PalUnlimitedHealth = g_cheats.godMode;
    Cheat::PalUnlimitedStamina = g_cheats.infiniteSP;
    Cheat::PalUnlimitedSatiety = false;
    Cheat::PalUnlimitedSanity  = false;
    Cheat::PalMaxStats = false;
    Cheat::SuperDamage = false;
    Cheat::InstantCrafting = false;
    Cheat::MaxWorkerSanity = false;
    Cheat::UnlimitedBaseHP = false;
}

static void ParseAdvancedCheats(const std::vector<std::pair<std::string, std::string>>& pairs)
{
    for (const auto& kv : pairs) {
        const std::string& k = kv.first;
        const std::string& v = kv.second;
        if (k == "unlimitedHealth") Cheat::UnlimitedHealth = ToBool(v);
        else if (k == "refillHealth") Cheat::RefillHealth = ToBool(v);
        else if (k == "unlimitedStamina") Cheat::UnlimitedStamina = ToBool(v);
        else if (k == "unlimitedSatiety") Cheat::UnlimitedSatiety = ToBool(v);
        else if (k == "refillSatiety") Cheat::RefillSatiety = ToBool(v);
        else if (k == "unlimitedSanity") Cheat::UnlimitedSanity = ToBool(v);
        else if (k == "noItemWeight") Cheat::NoItemWeight = ToBool(v);
        else if (k == "noReload") Cheat::NoReload = ToBool(v);
        else if (k == "infiniteDurability") Cheat::InfiniteDurability = ToBool(v);
        else if (k == "instantCapture") Cheat::InstantCapture = ToBool(v);
        else if (k == "captureChanceAlways") Cheat::CaptureChanceAlways = ToBool(v);
        else if (k == "massiveWorkSpeedPlayer") Cheat::MassiveWorkSpeedPlayer = ToBool(v);
        else if (k == "massiveWorkSpeedAll") Cheat::MassiveWorkSpeedAll = ToBool(v);
        else if (k == "noCraftingRequirements") Cheat::NoCraftingRequirements = ToBool(v);
        else if (k == "noBuildingRequirements") Cheat::NoBuildingRequirements = ToBool(v);
        else if (k == "ignoreBuildingOverlap") Cheat::IgnoreBuildingOverlap = ToBool(v);
        else if (k == "temperatureAlwaysNormal") Cheat::TemperatureAlwaysNormal = ToBool(v);
        else if (k == "stopTime") Cheat::StopTime = ToBool(v);
        else if (k == "noCrimeReporting") Cheat::NoCrimeReporting = ToBool(v);
        else if (k == "instantFishing") Cheat::InstantFishing = ToBool(v);
        else if (k == "unlimitedMoney") Cheat::UnlimitedMoney = ToBool(v);
        else if (k == "allPalsRare") Cheat::AllPalsRare = ToBool(v);
        else if (k == "everyoneCapturable") Cheat::EveryoneCapturable = ToBool(v);
        else if (k == "palRandomizer") Cheat::PalRandomizer = ToBool(v);
        else if (k == "palUnlimitedHealth") Cheat::PalUnlimitedHealth = ToBool(v);
        else if (k == "palUnlimitedStamina") Cheat::PalUnlimitedStamina = ToBool(v);
        else if (k == "palUnlimitedSatiety") Cheat::PalUnlimitedSatiety = ToBool(v);
        else if (k == "palUnlimitedSanity") Cheat::PalUnlimitedSanity = ToBool(v);
        else if (k == "palMaxStats") Cheat::PalMaxStats = ToBool(v);
        else if (k == "superDamage") Cheat::SuperDamage = ToBool(v);
        else if (k == "instantCrafting") Cheat::InstantCrafting = ToBool(v);
        else if (k == "maxWorkerSanity") Cheat::MaxWorkerSanity = ToBool(v);
        else if (k == "unlimitedBaseHP") Cheat::UnlimitedBaseHP = ToBool(v);
        // --- New cheats (parité FLiNG) ---
        else if (k == "infiniteShield") Cheat::InfiniteShield = ToBool(v);
        else if (k == "stealthMode") Cheat::StealthMode = ToBool(v);
        else if (k == "dropRateAlways") Cheat::DropRateAlways = ToBool(v);
        else if (k == "foodWontSpoil") Cheat::FoodWontSpoil = ToBool(v);
        else if (k == "infiniteExp") Cheat::InfiniteExp = ToBool(v);
        else if (k == "oneHitKill") Cheat::OneHitKill = ToBool(v);
        else if (k == "palInstantSkillCooldown") Cheat::PalInstantSkillCooldown = ToBool(v);
        else if (k == "unlimitedBaseStats") Cheat::UnlimitedBaseStats = ToBool(v);
        // --- Palworld 1.0 specific ---
        else if (k == "unlockWorldTree") Cheat::UnlockWorldTree = ToBool(v);
        else if (k == "unlockAwakening") Cheat::UnlockAwakening = ToBool(v);
        else if (k == "unlockAllTowerBosses") Cheat::UnlockAllTowerBosses = ToBool(v);
        // --- Phase 2: WeMod parity ---
        else if (k == "overheatRifleNoHeat") Cheat::OverheatRifleNoHeat = ToBool(v);
        else if (k == "unlimitedTorchDuration") Cheat::UnlimitedTorchDuration = ToBool(v);
        else if (k == "instantWorkProgress") Cheat::InstantWorkProgress = ToBool(v);
        else if (k == "instantAcceleration") Cheat::InstantAcceleration = ToBool(v);
        else if (k == "rewindHours") Cheat::RewindHours = ToInt(v, 0);
        else if (k == "damageMultiplier") Cheat::DamageMultiplier = ToFloat(v, 10000.0f);
        else if (k == "healthRegenRate") Cheat::HealthRegenRate = ToFloat(v, -1.0f);
        else if (k == "satietyDecreaseRate") Cheat::SatietyDecreaseRate = ToFloat(v, -1.0f);
        else if (k == "techPoints") Cheat::TechPoints = ToInt(v, -1);
        else if (k == "ancientTechPoints") Cheat::AncientTechPoints = ToInt(v, -1);
        else if (k == "statPoints") Cheat::StatPoints = ToInt(v, -1);
        else if (k == "setLevel") Cheat::SetLevel = ToInt(v, -1);
        else if (k == "setXP") Cheat::SetXP = ToInt(v, -1);
        else if (k == "setRank") Cheat::SetRank = ToInt(v, -1);
        else if (k == "lifmunkAmount") Cheat::LifmunkAmount = ToInt(v, -1);
        else if (k == "itemAmount") Cheat::ItemAmount = ToInt(v, -1);
        else if (k == "setHour") Cheat::SetHour = ToInt(v, -1);
        else if (k == "advanceHours") Cheat::AdvanceHours = ToInt(v, 0);
        else if (k == "palLevelRandomMin") Cheat::PalLevelRandomMin = ToInt(v, -1);
        else if (k == "palLevelRandomMax") Cheat::PalLevelRandomMax = ToInt(v, -1);
        else if (k == "xpMultiplier") Cheat::XPMultiplier = ToFloat(v, 1.0f);
        else if (k == "lootDropMultiplier") Cheat::LootDropMultiplier = ToFloat(v, 1.0f);
        else if (k == "captureMultiplier") Cheat::CaptureMultiplier = ToFloat(v, 1.0f);
        else if (k == "rarePalMultiplier") Cheat::RarePalMultiplier = ToFloat(v, 1.0f);
        else if (k == "daySpeedRate") Cheat::DaySpeedRate = ToFloat(v, 1.0f);
        else if (k == "nightSpeedRate") Cheat::NightSpeedRate = ToFloat(v, 1.0f);
        else if (k == "fishSpeedPercent") Cheat::FishSpeedPercent = ToFloat(v, 1.0f);
        else if (k == "walkSpeedMultiplier") Cheat::WalkSpeedMultiplier = ToFloat(v, 1.0f);
        else if (k == "sprintSpeedMultiplier") Cheat::SprintSpeedMultiplier = ToFloat(v, 1.0f);
        else if (k == "jumpHeightMultiplier") Cheat::JumpHeightMultiplier = ToFloat(v, 1.0f);
        else if (k == "workSpeedRate") Cheat::WorkSpeedRate = ToFloat(v, 10.0f);
        else if (k == "setAllItemCounts") { if (ToInt(v, -1) >= 0) Cheat::SetAllItemCounts(ToInt(v, 0)); }
        else if (k == "setLifmunkEffigyCount") { if (ToInt(v, -1) >= 0) Cheat::SetLifmunkEffigyCount(ToInt(v, 0)); }
    }
}

static void UpdateCheatState() {
    std::string cmd = ReadFileText(g_cmdInPath);
    if (cmd.empty()) return;
    auto pairs = ParseSimpleJson(cmd);
    {
        std::lock_guard<std::mutex> lock(g_cheatMutex);
        bool hasOneShot = false;
        for (auto& kv : pairs) {
            const std::string& k = kv.first;
            const std::string& v = kv.second;
            if (k == "godMode") g_cheats.godMode = ToBool(v);
            else if (k == "infiniteHP") g_cheats.infiniteHP = ToBool(v);
            else if (k == "infiniteSP") g_cheats.infiniteSP = ToBool(v);
            else if (k == "infiniteWeight") g_cheats.infiniteWeight = ToBool(v);
            else if (k == "superSpeed") g_cheats.superSpeed = ToBool(v);
            else if (k == "jumpValue") g_cheats.jumpValue = ToFloat(v, 3000.0f);
            else if (k == "speedValue") g_cheats.speedValue = ToFloat(v, 2000.0f);
            else if (k == "superJump") g_cheats.superJump = ToBool(v);
            else if (k == "flyMode") g_cheats.flyMode = ToBool(v);
            else if (k == "noClip") g_cheats.noClip = ToBool(v);
            else if (k == "weightValue") g_cheats.weightValue = ToFloat(v, 0.0f);
            else if (k == "teleport") g_cheats.teleport = ToBool(v);
            else if (k == "teleportDistance") g_cheats.teleportDistance = ToFloat(v, 5000.0f);
            else if (k == "unlockFastTravel") g_cheats.unlockFastTravel = ToBool(v);
            else if (k == "clearWeather") g_cheats.clearWeather = ToBool(v);
            else if (k == "teleportToX") { g_cheats.teleportToX = ToFloat(v, 0.0f); g_cheats.teleportTo = true; hasOneShot = true; }
            else if (k == "teleportToY") g_cheats.teleportToY = ToFloat(v, 0.0f);
            else if (k == "teleportToZ") g_cheats.teleportToZ = ToFloat(v, 0.0f);
            // One-shot action keys
            else if (k == "setAllItemCounts" || k == "setLifmunkEffigyCount" ||
                     k == "setLevel" || k == "setXP" || k == "setRank" ||
                     k == "lifmunkAmount" || k == "itemAmount" ||
                     k == "setHour" || k == "advanceHours" ||
                     k == "techPoints" || k == "ancientTechPoints" || k == "statPoints") {
                hasOneShot = true;
            }
        }
        SyncCheatsToCore();
        ParseAdvancedCheats(pairs);
        if (hasOneShot) {
            // Vider le fichier de commandes pour que les actions one-shot ne se répètent pas
            std::ofstream f(g_cmdInPath, std::ios::binary | std::ios::trunc);
        }
    }
}

// ---------------------------------------------------------------------------
// Helpers mémoire
// ---------------------------------------------------------------------------
static void* GetRootComponent(void* actor) {
    if (!actor) return nullptr;
    return deref(actor, PalTrainerOffsets::AActor_RootComponent);
}

static FVector GetComponentLocation(void* rootComponent) {
    if (!rootComponent) return {0,0,0};
    return read<FVector>(rootComponent, PalTrainerOffsets::USceneComponent_RelativeLocation);
}

static void SetComponentLocation(void* rootComponent, const FVector& loc) {
    if (!rootComponent) return;
    write<FVector>(rootComponent, PalTrainerOffsets::USceneComponent_RelativeLocation, loc);
}

static FRotator GetComponentRotation(void* rootComponent) {
    if (!rootComponent) return {0,0,0};
    return read<FRotator>(rootComponent, PalTrainerOffsets::USceneComponent_RelativeRotation);
}

static void* GetCharacterMovement(void* character) {
    if (!character) return nullptr;
    return deref(character, PalTrainerOffsets::ACharacter_CharacterMovement);
}

static void* GetPalCharacterParameterComponent(void* character) {
    if (!character) return nullptr;
    // APalCharacter ajoute CharacterParameterComponent à 0x628
    void* comp = deref(character, 0x628);
    return comp;
}

static void* GetInventoryData(void* playerState) {
    if (!playerState) return nullptr;
    return deref(playerState, PalTrainerOffsets::APalPlayerState_InventoryData);
}

static void* GetIndividualParameter(void* charParamComp) {
    if (!charParamComp) return nullptr;
    return deref(charParamComp, PalTrainerOffsets::UPalCharacterParameter_IndividualParameter);
}

// ---------------------------------------------------------------------------
// Application des cheats (uniquement fly/noClip — le reste est géré par Cheat::Update)
// ---------------------------------------------------------------------------
static void ApplyMovementExtras(void* movement) {
    if (!movement) return;
    std::lock_guard<std::mutex> lock(g_cheatMutex);
    if (g_cheats.flyMode) {
        write<float>(movement, PalTrainerOffsets::UCharacterMovement_MaxFlySpeed, g_cheats.speedValue);
    }
    if (g_cheats.noClip) {
        write<float>(movement, PalTrainerOffsets::UCharacterMovement_GravityScale, 1.0f);
    }
}

static void ApplyTeleport(void* pawn, void* rootComp) {
    std::lock_guard<std::mutex> lock(g_cheatMutex);
    if (!rootComp) return;

    // Teleport to specific coordinates (from minimap click)
    if (g_cheats.teleportTo) {
        // Only teleport if coordinates are non-zero (avoid teleporting to origin on startup)
        if (g_cheats.teleportToX != 0.0f || g_cheats.teleportToY != 0.0f || g_cheats.teleportToZ != 0.0f) {
            FVector target;
            target.X = g_cheats.teleportToX;
            target.Y = g_cheats.teleportToY;
            target.Z = g_cheats.teleportToZ;
            SetComponentLocation(rootComp, target);
        }
        g_cheats.teleportTo = false; // one-shot
        g_cheats.teleportToX = g_cheats.teleportToY = g_cheats.teleportToZ = 0.0f;
        return;
    }

    // Teleport forward based on camera orientation
    if (!g_cheats.teleport) return;
    FVector loc = GetComponentLocation(rootComp);
    FRotator rot = GetComponentRotation(rootComp);
    // Calculer le vecteur avant à partir du lacet/tangage et avancer le long de celui-ci
    double yaw = rot.Yaw * 3.14159265358979323846 / 180.0;
    double pitch = rot.Pitch * 3.14159265358979323846 / 180.0;
    double dist = g_cheats.teleportDistance;
    FVector target;
    target.X = loc.X + cos(pitch) * cos(yaw) * dist;
    target.Y = loc.Y + cos(pitch) * sin(yaw) * dist;
    target.Z = loc.Z + sin(pitch) * dist;
    SetComponentLocation(rootComp, target);
    g_cheats.teleport = false; // one-shot
}

// ---------------------------------------------------------------------------
// Lecture du statut
// ---------------------------------------------------------------------------
static PlayerSnapshot ReadPlayerSnapshot() {
    PlayerSnapshot snap;

    static time_t lastSnapLog = 0;
    bool logSnap = (time(nullptr) - lastSnapLog >= 5);
    if (logSnap) lastSnapLog = time(nullptr);

    void* pawn = Engine::GetLocalPlayerPawn();
    if (!pawn) {
        if (logSnap) Log("ReadPlayerSnapshot: Pawn NULL (Engine::GetLocalPlayerPawn)");
        return snap;
    }
    if (logSnap) Log("ReadPlayerSnapshot: Pawn=0x%llX", (unsigned long long)pawn);

    void* root = GetRootComponent(pawn);
    if (!root) {
        if (logSnap) Log("ReadPlayerSnapshot: RootComp NULL (Pawn+0x198)");
        return snap;
    }
    if (logSnap) Log("ReadPlayerSnapshot: RootComp=0x%llX", (unsigned long long)root);

    FVector loc = GetComponentLocation(root);
    FRotator rot = GetComponentRotation(root);
    snap.x = (float)loc.X; snap.y = (float)loc.Y; snap.z = (float)loc.Z;
    snap.pitch = (float)rot.Pitch; snap.yaw = (float)rot.Yaw; snap.roll = (float)rot.Roll;
    snap.valid = true;
    if (logSnap) Log("ReadPlayerSnapshot: OK pos=(%.0f, %.0f, %.0f)", snap.x, snap.y, snap.z);

    void* movement = GetCharacterMovement(pawn);
    if (movement) {
        snap.maxSpeed = read<float>(movement, PalTrainerOffsets::UCharacterMovement_MaxWalkSpeed);
    }

    void* playerState = Engine::GetLocalPlayerState();
    if (playerState) {
        void* inventory = GetInventoryData(playerState);
        if (inventory) {
            snap.weight = read<float>(inventory, PalTrainerOffsets::UPalPlayerInventoryData_NowItemWeight);
            snap.maxWeight = read<float>(inventory, PalTrainerOffsets::UPalPlayerInventoryData_MaxInventoryWeight);
        }
    }

    void* charParam = GetPalCharacterParameterComponent(pawn);
    if (charParam) {
        void* individual = GetIndividualParameter(charParam);
        if (individual) {
            snap.hp = read<FFixedPoint64>(individual, PalTrainerOffsets::UPalIndividual_HP).Value;
            snap.maxHp = read<FFixedPoint64>(individual, PalTrainerOffsets::UPalIndividual_MaxHP).Value;
            snap.sp = read<FFixedPoint64>(individual, PalTrainerOffsets::UPalIndividual_MP).Value;
            snap.maxSp = read<FFixedPoint64>(individual, PalTrainerOffsets::UPalIndividual_MaxSP).Value;
            snap.level = read<int32_t>(individual, 0x2A0 + 0x020);
        }
    }
    snap.name = "Player";
    return snap;
}

// ---------------------------------------------------------------------------
// JSON de sortie
// ---------------------------------------------------------------------------
static std::string BuildStatusJson(const PlayerSnapshot& snap) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "{\n";
    ss << "  \"ready\": " << (snap.valid ? "true" : "false") << ",\n";
    ss << "  \"player\": {\n";
    ss << "    \"x\": " << snap.x << ",\n";
    ss << "    \"y\": " << snap.y << ",\n";
    ss << "    \"z\": " << snap.z << ",\n";
    ss << "    \"pitch\": " << snap.pitch << ",\n";
    ss << "    \"yaw\": " << snap.yaw << ",\n";
    ss << "    \"roll\": " << snap.roll << ",\n";
    ss << "    \"speed\": " << snap.maxSpeed << ",\n";
    ss << "    \"weight\": " << snap.weight << ",\n";
    ss << "    \"maxWeight\": " << snap.maxWeight << ",\n";
    ss << "    \"hp\": " << snap.hp << ",\n";
    ss << "    \"maxHp\": " << snap.maxHp << ",\n";
    ss << "    \"sp\": " << snap.sp << ",\n";
    ss << "    \"maxSp\": " << snap.maxSp << ",\n";
    ss << "    \"level\": " << snap.level << ",\n";
    ss << "    \"name\": \"" << JsonEscape(snap.name) << "\"\n";
    ss << "  },\n";
    ss << "  \"entities\": [],\n";
    ss << "  \"cheats\": {\n";
    {
        std::lock_guard<std::mutex> lock(g_cheatMutex);
        ss << "    \"godMode\": " << (g_cheats.godMode ? "true" : "false") << ",\n";
        ss << "    \"infiniteHP\": " << (g_cheats.infiniteHP ? "true" : "false") << ",\n";
        ss << "    \"infiniteSP\": " << (g_cheats.infiniteSP ? "true" : "false") << ",\n";
        ss << "    \"infiniteWeight\": " << (g_cheats.infiniteWeight ? "true" : "false") << ",\n";
        ss << "    \"superSpeed\": " << (g_cheats.superSpeed ? "true" : "false") << ",\n";
        ss << "    \"superJump\": " << (g_cheats.superJump ? "true" : "false") << ",\n";
        ss << "    \"flyMode\": " << (g_cheats.flyMode ? "true" : "false") << ",\n";
        ss << "    \"noClip\": " << (g_cheats.noClip ? "true" : "false") << ",\n";
        ss << "    \"speedValue\": " << g_cheats.speedValue << ",\n";
        ss << "    \"jumpValue\": " << g_cheats.jumpValue << ",\n";
        ss << "    \"weightValue\": " << g_cheats.weightValue << ",\n";

        // État des cheats avancés de PalTrainerCore
        ss << "    \"unlimitedHealth\": " << (Cheat::UnlimitedHealth ? "true" : "false") << ",\n";
        ss << "    \"refillHealth\": " << (Cheat::RefillHealth ? "true" : "false") << ",\n";
        ss << "    \"unlimitedStamina\": " << (Cheat::UnlimitedStamina ? "true" : "false") << ",\n";
        ss << "    \"unlimitedSatiety\": " << (Cheat::UnlimitedSatiety ? "true" : "false") << ",\n";
        ss << "    \"refillSatiety\": " << (Cheat::RefillSatiety ? "true" : "false") << ",\n";
        ss << "    \"unlimitedSanity\": " << (Cheat::UnlimitedSanity ? "true" : "false") << ",\n";
        ss << "    \"noItemWeight\": " << (Cheat::NoItemWeight ? "true" : "false") << ",\n";
        ss << "    \"noReload\": " << (Cheat::NoReload ? "true" : "false") << ",\n";
        ss << "    \"infiniteDurability\": " << (Cheat::InfiniteDurability ? "true" : "false") << ",\n";
        ss << "    \"instantCapture\": " << (Cheat::InstantCapture ? "true" : "false") << ",\n";
        ss << "    \"captureChanceAlways\": " << (Cheat::CaptureChanceAlways ? "true" : "false") << ",\n";
        ss << "    \"massiveWorkSpeedPlayer\": " << (Cheat::MassiveWorkSpeedPlayer ? "true" : "false") << ",\n";
        ss << "    \"massiveWorkSpeedAll\": " << (Cheat::MassiveWorkSpeedAll ? "true" : "false") << ",\n";
        ss << "    \"noCraftingRequirements\": " << (Cheat::NoCraftingRequirements ? "true" : "false") << ",\n";
        ss << "    \"noBuildingRequirements\": " << (Cheat::NoBuildingRequirements ? "true" : "false") << ",\n";
        ss << "    \"ignoreBuildingOverlap\": " << (Cheat::IgnoreBuildingOverlap ? "true" : "false") << ",\n";
        ss << "    \"temperatureAlwaysNormal\": " << (Cheat::TemperatureAlwaysNormal ? "true" : "false") << ",\n";
        ss << "    \"stopTime\": " << (Cheat::StopTime ? "true" : "false") << ",\n";
        ss << "    \"noCrimeReporting\": " << (Cheat::NoCrimeReporting ? "true" : "false") << ",\n";
        ss << "    \"instantFishing\": " << (Cheat::InstantFishing ? "true" : "false") << ",\n";
        ss << "    \"unlimitedMoney\": " << (Cheat::UnlimitedMoney ? "true" : "false") << ",\n";
        ss << "    \"allPalsRare\": " << (Cheat::AllPalsRare ? "true" : "false") << ",\n";
        ss << "    \"everyoneCapturable\": " << (Cheat::EveryoneCapturable ? "true" : "false") << ",\n";
        ss << "    \"palRandomizer\": " << (Cheat::PalRandomizer ? "true" : "false") << ",\n";
        ss << "    \"palUnlimitedHealth\": " << (Cheat::PalUnlimitedHealth ? "true" : "false") << ",\n";
        ss << "    \"palUnlimitedStamina\": " << (Cheat::PalUnlimitedStamina ? "true" : "false") << ",\n";
        ss << "    \"palUnlimitedSatiety\": " << (Cheat::PalUnlimitedSatiety ? "true" : "false") << ",\n";
        ss << "    \"palUnlimitedSanity\": " << (Cheat::PalUnlimitedSanity ? "true" : "false") << ",\n";
        ss << "    \"palMaxStats\": " << (Cheat::PalMaxStats ? "true" : "false") << ",\n";
        ss << "    \"superDamage\": " << (Cheat::SuperDamage ? "true" : "false") << ",\n";
        ss << "    \"instantCrafting\": " << (Cheat::InstantCrafting ? "true" : "false") << ",\n";
        ss << "    \"maxWorkerSanity\": " << (Cheat::MaxWorkerSanity ? "true" : "false") << ",\n";
        ss << "    \"unlimitedBaseHP\": " << (Cheat::UnlimitedBaseHP ? "true" : "false") << ",\n";

        // --- New cheats (parité FLiNG) ---
        ss << "    \"infiniteShield\": " << (Cheat::InfiniteShield ? "true" : "false") << ",\n";
        ss << "    \"stealthMode\": " << (Cheat::StealthMode ? "true" : "false") << ",\n";
        ss << "    \"dropRateAlways\": " << (Cheat::DropRateAlways ? "true" : "false") << ",\n";
        ss << "    \"foodWontSpoil\": " << (Cheat::FoodWontSpoil ? "true" : "false") << ",\n";
        ss << "    \"infiniteExp\": " << (Cheat::InfiniteExp ? "true" : "false") << ",\n";
        ss << "    \"oneHitKill\": " << (Cheat::OneHitKill ? "true" : "false") << ",\n";
        ss << "    \"palInstantSkillCooldown\": " << (Cheat::PalInstantSkillCooldown ? "true" : "false") << ",\n";
        ss << "    \"unlimitedBaseStats\": " << (Cheat::UnlimitedBaseStats ? "true" : "false") << ",\n";
        // --- Palworld 1.0 specific ---
        ss << "    \"unlockWorldTree\": " << (Cheat::UnlockWorldTree ? "true" : "false") << ",\n";
        ss << "    \"unlockAwakening\": " << (Cheat::UnlockAwakening ? "true" : "false") << ",\n";
        ss << "    \"unlockAllTowerBosses\": " << (Cheat::UnlockAllTowerBosses ? "true" : "false") << ",\n";

        ss << "    \"damageMultiplier\": " << Cheat::DamageMultiplier << ",\n";
        ss << "    \"healthRegenRate\": " << Cheat::HealthRegenRate << ",\n";
        ss << "    \"satietyDecreaseRate\": " << Cheat::SatietyDecreaseRate << ",\n";
        ss << "    \"techPoints\": " << Cheat::TechPoints << ",\n";
        ss << "    \"ancientTechPoints\": " << Cheat::AncientTechPoints << ",\n";
        ss << "    \"statPoints\": " << Cheat::StatPoints << ",\n";
        ss << "    \"setLevel\": " << Cheat::SetLevel << ",\n";
        ss << "    \"setXP\": " << Cheat::SetXP << ",\n";
        ss << "    \"setRank\": " << Cheat::SetRank << ",\n";
        ss << "    \"lifmunkAmount\": " << Cheat::LifmunkAmount << ",\n";
        ss << "    \"itemAmount\": " << Cheat::ItemAmount << ",\n";
        ss << "    \"setHour\": " << Cheat::SetHour << ",\n";
        ss << "    \"advanceHours\": " << Cheat::AdvanceHours << ",\n";
        ss << "    \"palLevelRandomMin\": " << Cheat::PalLevelRandomMin << ",\n";
        ss << "    \"palLevelRandomMax\": " << Cheat::PalLevelRandomMax << ",\n";
        ss << "    \"xpMultiplier\": " << Cheat::XPMultiplier << ",\n";
        ss << "    \"lootDropMultiplier\": " << Cheat::LootDropMultiplier << ",\n";
        ss << "    \"captureMultiplier\": " << Cheat::CaptureMultiplier << ",\n";
        ss << "    \"rarePalMultiplier\": " << Cheat::RarePalMultiplier << ",\n";
        ss << "    \"daySpeedRate\": " << Cheat::DaySpeedRate << ",\n";
        ss << "    \"nightSpeedRate\": " << Cheat::NightSpeedRate << ",\n";
        ss << "    \"fishSpeedPercent\": " << Cheat::FishSpeedPercent << ",\n";
        ss << "    \"walkSpeedMultiplier\": " << Cheat::WalkSpeedMultiplier << ",\n";
        ss << "    \"sprintSpeedMultiplier\": " << Cheat::SprintSpeedMultiplier << ",\n";
        ss << "    \"jumpHeightMultiplier\": " << Cheat::JumpHeightMultiplier << ",\n";
        ss << "    \"workSpeedRate\": " << Cheat::WorkSpeedRate << "\n";
    }
    ss << "  }\n";
    ss << "}\n";
    return ss.str();
}

// ---------------------------------------------------------------------------
// Thread travailleur
// ---------------------------------------------------------------------------
static void WorkerLoop() {
    // Initialiser la logique de cheat avancée une fois ; elle continuera à scanner jusqu'à l'apparition des objets
    static bool cheatInit = Cheat::Initialize();
    (void)cheatInit;

    while (g_running) {
        UpdateCheatState();

        // Logique de cheat avancée de PalTrainerCore
        if (Cheat::Initialize())
            Cheat::Update();

        PlayerSnapshot snap = ReadPlayerSnapshot();
        if (snap.valid) {
            void* pawn = Engine::GetLocalPlayerPawn();
            if (pawn) {
                void* movement = GetCharacterMovement(pawn);
                void* root = GetRootComponent(pawn);

                ApplyMovementExtras(movement);
                ApplyTeleport(pawn, root);
            }
        }
        std::string json = BuildStatusJson(snap);
        WriteFileText(g_jsonOutPath, json);
        Sleep(16);
    }
}

// ---------------------------------------------------------------------------
// Point d'entrée de la DLL
// ---------------------------------------------------------------------------
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
    switch (reason) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hModule);
            g_hModule = hModule;
            GetModuleFileNameA(hModule, g_dataDir, MAX_PATH);
            {
                std::string tmp(g_dataDir);
                size_t pos = tmp.find_last_of("\\/");
                if (pos != std::string::npos) tmp = tmp.substr(0, pos);
                strncpy(g_dataDir, tmp.c_str(), MAX_PATH - 1);
                g_dataDir[MAX_PATH - 1] = '\0';
                snprintf(g_jsonOutPath, MAX_PATH, "%s\\paltrainer.json", g_dataDir);
                snprintf(g_cmdInPath, MAX_PATH, "%s\\commands.json", g_dataDir);

                LogInit(g_dataDir, "paltrainer_dll_log.txt");
                Log("=== PalTrainerCore DLL v1.0 injected ===");

                char offsetsPath[MAX_PATH];
                snprintf(offsetsPath, MAX_PATH, "%s\\runtime_offsets.json", g_dataDir);
                PalTrainerRuntime::Load(offsetsPath);
                Log("Runtime offsets loaded from %s", offsetsPath);
            }
            g_running = true;
            g_worker = std::thread(WorkerLoop);
            Log("Worker thread started");
            break;
        case DLL_PROCESS_DETACH:
            Log("DLL detaching, stopping worker...");
            g_running = false;
            if (g_worker.joinable()) g_worker.join();
            Cheat::Shutdown();
            Log("DLL cleanup complete");
            LogClose();
            break;
    }
    return TRUE;
}

