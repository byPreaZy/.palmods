#pragma once

#include "sdk.hpp"

namespace Cheat
{
    // Cycle de vie du core
    bool Initialize();
    void Update();
    void Shutdown();

    // Bascules
    extern bool UnlimitedHealth;
    extern bool RefillHealth;
    extern bool UnlimitedStamina;
    extern bool UnlimitedSatiety;
    extern bool RefillSatiety;
    extern bool UnlimitedSanity;
    extern bool NoItemWeight;
    extern bool NoReload;
    extern bool InfiniteDurability;
    extern bool InstantCapture;
    extern bool CaptureChanceAlways;
    extern bool MassiveWorkSpeedPlayer;
    extern bool MassiveWorkSpeedAll;
    extern bool NoCraftingRequirements;
    extern bool NoBuildingRequirements;
    extern bool IgnoreBuildingOverlap;
    extern bool TemperatureAlwaysNormal;
    extern bool StopTime;
    extern bool NoCrimeReporting;
    extern bool InstantFishing;
    extern bool UnlimitedMoney;
    extern bool AllPalsRare;
    extern bool EveryoneCapturable;
    extern bool PalRandomizer;
    extern bool PalUnlimitedHealth;
    extern bool PalUnlimitedStamina;
    extern bool PalUnlimitedSatiety;
    extern bool PalUnlimitedSanity;
    extern bool PalMaxStats;
    extern bool SuperDamage;
    extern bool InstantCrafting;
    extern bool MaxWorkerSanity;
    extern bool UnlimitedBaseHP;

    // --- New cheats (parité FLiNG) ---
    extern bool InfiniteShield;        // Lock shield HP to max
    extern bool StealthMode;           // Reduce/eliminate enemy detection
    extern bool DropRateAlways;        // 100% drop rate
    extern bool FoodWontSpoil;         // Freeze food decay
    extern bool InfiniteExp;           // Auto-add XP each tick
    extern bool OneHitKill;            // Set enemy HP to 1
    extern bool PalInstantSkillCooldown; // Zero pal skill cooldowns
    extern bool UnlimitedBaseStats;    // Max happiness, work speed, productivity

    // --- Palworld 1.0 specific ---
    extern bool UnlockWorldTree;       // Unlock World Tree content
    extern bool UnlockAwakening;       // Unlock Awakening dungeon
    extern bool UnlockAllTowerBosses;  // Unlock all tower boss rematches

    // Valeurs à définir (utiliser INT_MIN / sentinelle pour désactiver)
    extern int TechPoints;
    extern int AncientTechPoints;
    extern int StatPoints;
    extern int SetLevel;
    extern int SetXP;
    extern int SetRank;
    extern int LifmunkAmount;
    extern int ItemAmount;
    extern int SetHour;
    extern int AdvanceHours;
    extern int PalLevelRandomMin;
    extern int PalLevelRandomMax;
    extern float XPMultiplier;
    extern float LootDropMultiplier;
    extern float CaptureMultiplier;
    extern float RarePalMultiplier;
    extern float DaySpeedRate;
    extern float NightSpeedRate;
    extern float FishSpeedPercent;
    extern float WalkSpeedMultiplier;
    extern float SprintSpeedMultiplier;
    extern float JumpHeightMultiplier;
    extern float WorkSpeedRate;
    extern float DamageMultiplier;
    extern float HealthRegenRate;
    extern float SatietyDecreaseRate;

    // Helpers
    void SetAllItemCounts(int amount);
    void SetLifmunkEffigyCount(int amount);
}
