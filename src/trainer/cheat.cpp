#include "cheat.hpp"
#include "engine.hpp"
#include "sdk.hpp"

#include <windows.h>
#include <cstdint>
#include <cstring>
#include <vector>

namespace Cheat
{
    // ------------------------------------------------------------------------
    // État des bascules
    // ------------------------------------------------------------------------
    bool UnlimitedHealth = false;
    bool RefillHealth = false;
    bool UnlimitedStamina = false;
    bool UnlimitedSatiety = false;
    bool RefillSatiety = false;
    bool UnlimitedSanity = false;
    bool NoItemWeight = false;
    bool NoReload = false;
    bool InfiniteDurability = false;
    bool InstantCapture = false;
    bool CaptureChanceAlways = false;
    bool MassiveWorkSpeedPlayer = false;
    bool MassiveWorkSpeedAll = false;
    bool NoCraftingRequirements = false;
    bool NoBuildingRequirements = false;
    bool IgnoreBuildingOverlap = false;
    bool TemperatureAlwaysNormal = false;
    bool StopTime = false;
    bool NoCrimeReporting = false;
    bool InstantFishing = false;
    bool UnlimitedMoney = false;
    bool AllPalsRare = false;
    bool EveryoneCapturable = false;
    bool PalRandomizer = false;
    bool PalUnlimitedHealth = false;
    bool PalUnlimitedStamina = false;
    bool PalUnlimitedSatiety = false;
    bool PalUnlimitedSanity = false;
    bool PalMaxStats = false;
    bool SuperDamage = false;
    bool InstantCrafting = false;
    bool MaxWorkerSanity = false;
    bool UnlimitedBaseHP = false;

    // --- New cheats (parité FLiNG) ---
    bool InfiniteShield = false;
    bool StealthMode = false;
    bool DropRateAlways = false;
    bool FoodWontSpoil = false;
    bool InfiniteExp = false;
    bool OneHitKill = false;
    bool PalInstantSkillCooldown = false;
    bool UnlimitedBaseStats = false;

    // --- Palworld 1.0 specific ---
    bool UnlockWorldTree = false;
    bool UnlockAwakening = false;
    bool UnlockAllTowerBosses = false;

    // --- Phase 2: WeMod parity cheats ---
    bool OverheatRifleNoHeat = false;
    bool UnlimitedTorchDuration = false;
    bool InstantWorkProgress = false;
    bool InstantAcceleration = false;
    int RewindHours = 0;

    float DamageMultiplier = 10000.0f;
    float HealthRegenRate = -1.0f;
    float SatietyDecreaseRate = -1.0f;

    int TechPoints = -1;
    int AncientTechPoints = -1;
    int StatPoints = -1;
    int SetLevel = -1;
    int SetXP = -1;
    int SetRank = -1;
    int LifmunkAmount = -1;
    int ItemAmount = -1;
    int SetHour = -1;
    int AdvanceHours = 0;
    int PalLevelRandomMin = -1;
    int PalLevelRandomMax = -1;
    float XPMultiplier = 1.0f;
    float LootDropMultiplier = 1.0f;
    float CaptureMultiplier = 1.0f;
    float RarePalMultiplier = 1.0f;
    float DaySpeedRate = 1.0f;
    float NightSpeedRate = 1.0f;
    float FishSpeedPercent = 1.0f;
    float WalkSpeedMultiplier = 1.0f;
    float SprintSpeedMultiplier = 1.0f;
    float JumpHeightMultiplier = 1.0f;
    float WorkSpeedRate = 10.0f;

    // ------------------------------------------------------------------------
    // Objets mis en cache
    // ------------------------------------------------------------------------
    static SDK::UObject*       g_DebugSetting = nullptr;
    static SDK::UObject*       g_GameSetting = nullptr;
    static SDK::UPalTimeManager* g_TimeManager = nullptr;
    static SDK::UPalWorldSecuritySystem* g_WorldSecurity = nullptr;
    static SDK::APalCharacter* g_PlayerCharacter = nullptr;
    static SDK::APalPlayerState* g_PlayerState = nullptr;
    static SDK::UPalCharacterMovementComponent* g_PlayerMovement = nullptr;
    static SDK::APalWeaponBase* g_Weapon = nullptr;
    static SDK::UPalBodyTemperatureComponent* g_PlayerTemperature = nullptr;
    static SDK::UObject* g_CheatManager = nullptr;

    static std::vector<SDK::APalCharacter*> g_AllPals;
    static std::vector<SDK::UPalMapObjectModel*> g_AllMapObjects;
    static std::vector<SDK::UPalItemSlot*> g_PlayerItemSlots;
    static int g_ItemSlotScanFrame = 0;

    static SDK::UClass* g_CharacterClass = nullptr;
    static SDK::UClass* g_PlayerStateClass = nullptr;
    static SDK::UClass* g_WeaponClass = nullptr;
    static SDK::UClass* g_MapObjectModelClass = nullptr;

    static int   g_Frame = 0;
    static bool  g_Initialized = false;

    // Valeurs de base du mouvement
    static bool  g_MoveBaseSet = false;
    static float g_BaseSlowWalk = 0.0f;
    static float g_BaseWalk = 0.0f;
    static float g_BaseRun = 0.0f;
    static float g_BaseSprintSpeed = 0.0f;
    static float g_BaseSprintAccel = 0.0f;
    static float g_BaseJumpZ = 0.0f;

    // ------------------------------------------------------------------------
    // Helpers
    // ------------------------------------------------------------------------
    static SDK::TUObjectArray* GetObjectArray()
    {
        return Engine::GetGObjects();
    }

    static void CacheBaseMovement()
    {
        if (!g_PlayerMovement || g_MoveBaseSet) return;
        SDK::UPalCharacterMovementComponent* mv = g_PlayerMovement;
        g_BaseSlowWalk    = *mv->GetSlowWalkSpeed_Default();
        g_BaseWalk        = *mv->GetWalkSpeed_Default();
        g_BaseRun         = *mv->GetRunSpeed_Default();
        g_BaseSprintSpeed = *mv->GetSprintMaxSpeed();
        g_BaseSprintAccel = *mv->GetSprintMaxAcceleration();
        g_BaseJumpZ       = *mv->GetBaseJumpZVelocity();
        g_MoveBaseSet = true;
    }

    static bool IsOwnedByPlayer(SDK::UObject* obj)
    {
        if (!obj) return false;
        SDK::UObject* outer = obj->OuterPrivate;
        while (outer)
        {
            if (outer == (SDK::UObject*)g_PlayerCharacter ||
                outer == (SDK::UObject*)g_PlayerState)
                return true;
            outer = outer->OuterPrivate;
        }
        return false;
    }

    static void QuickRefresh()
    {
        // Cheap re-resolution of local player via GWorld chain (no GObjects scan)
        SDK::APalCharacter* localPawn = Engine::GetLocalPlayerPawn();
        if (localPawn)
        {
            g_PlayerCharacter = localPawn;

            void* mv = *(void**)((uintptr_t)localPawn + 0x320);
            if (mv)
                g_PlayerMovement = (SDK::UPalCharacterMovementComponent*)mv;

            if (!g_PlayerTemperature)
            {
                for (uintptr_t off = 0x320; off <= 0x630; off += 8)
                {
                    void* comp = *(void**)((uintptr_t)localPawn + off);
                    if (!comp) continue;
                    SDK::UClass* compCls = *(SDK::UClass**)((uintptr_t)comp + 0x10);
                    if (compCls && Engine::IsAByName(compCls, L"PalBodyTemperatureComponent"))
                    {
                        g_PlayerTemperature = (SDK::UPalBodyTemperatureComponent*)comp;
                        break;
                    }
                }
            }
        }

        SDK::APalPlayerState* localPS = Engine::GetLocalPlayerState();
        if (localPS)
            g_PlayerState = localPS;
    }

    static void CacheObjects()
    {
        if (!g_CharacterClass)      g_CharacterClass      = Engine::FindClass(L"PalCharacter");
        if (!g_PlayerStateClass)    g_PlayerStateClass    = Engine::FindClass(L"PalPlayerState");
        if (!g_WeaponClass)         g_WeaponClass         = Engine::FindClass(L"PalWeaponBase");
        if (!g_MapObjectModelClass) g_MapObjectModelClass = Engine::FindClass(L"PalMapObjectModel");

        SDK::TUObjectArray* arr = GetObjectArray();
        if (!arr) return;

        int32 perChunk = arr->MaxChunks > 0 ? (arr->MaxElements / arr->MaxChunks) : arr->MaxElements;
        if (perChunk <= 0) perChunk = arr->MaxElements;

        g_PlayerCharacter   = nullptr;
        g_PlayerState       = nullptr;
        g_PlayerMovement    = nullptr;
        g_Weapon            = nullptr;
        g_PlayerTemperature = nullptr;
        g_TimeManager       = nullptr;
        g_WorldSecurity     = nullptr;
        g_CheatManager      = nullptr;
        g_AllPals.clear();
        g_AllMapObjects.clear();
        g_PlayerItemSlots.clear();

        // --- Resolve local player via GWorld chain (correct in multiplayer) ---
        SDK::APalCharacter* localPawn = Engine::GetLocalPlayerPawn();
        if (localPawn)
        {
            g_PlayerCharacter = localPawn;

            // Resolve movement component directly from pawn (ACharacter::CharacterMovement at 0x320)
            void* mv = *(void**)((uintptr_t)localPawn + 0x320);
            if (mv)
                g_PlayerMovement = (SDK::UPalCharacterMovementComponent*)mv;

            // Resolve body temperature component by scanning pawn component pointers
            // APalCharacter has components at various offsets between 0x320 and 0x638
            if (!g_PlayerTemperature)
            {
                for (uintptr_t off = 0x320; off <= 0x630; off += 8)
                {
                    void* comp = *(void**)((uintptr_t)localPawn + off);
                    if (!comp) continue;
                    SDK::UClass* compCls = *(SDK::UClass**)((uintptr_t)comp + 0x10);
                    if (compCls && Engine::IsAByName(compCls, L"PalBodyTemperatureComponent"))
                    {
                        g_PlayerTemperature = (SDK::UPalBodyTemperatureComponent*)comp;
                        break;
                    }
                }
            }
        }

        SDK::APalPlayerState* localPS = Engine::GetLocalPlayerState();
        if (localPS)
            g_PlayerState = localPS;

        // Read local player UID for fallback matching (if PlayerState was resolved)
        SDK::FGuid localPlayerUid{};
        if (g_PlayerState)
        {
            SDK::FGuid* uid = g_PlayerState->GetPlayerUId();
            if (uid) localPlayerUid = *uid;
        }

        for (int32 c = 0; c < arr->NumChunks; ++c)
        {
            SDK::FUObjectItem* chunk = arr->Objects[c];
            if (!chunk) continue;

            for (int32 i = 0; i < perChunk && (c * perChunk + i) < arr->NumElements; ++i)
            {
                SDK::UObject* obj = chunk[i].Object;
                if (!obj) continue;

                SDK::UClass* cls = obj->ClassPrivate;
                if (!cls) continue;

                if (!g_PlayerCharacter && Engine::IsAByName(cls, L"PalCharacter"))
                {
                    SDK::APalCharacter* chr = (SDK::APalCharacter*)obj;
                    SDK::UPalCharacterParameterComponent* cp = chr->GetCharacterParameterComponent();
                    if (cp)
                    {
                        SDK::UPalIndividualCharacterParameter* ind = cp->GetIndividualParameter();
                        if (ind && ind->SaveParameter.IsPlayer)
                        {
                            // If we have the local player UID, match by OwnerPlayerUId
                            if (localPlayerUid.IsValid())
                            {
                                if (ind->SaveParameter.OwnerPlayerUId == localPlayerUid)
                                    g_PlayerCharacter = chr;
                            }
                            else
                                g_PlayerCharacter = chr;
                        }
                    }
                }

                if (!g_PlayerState && Engine::IsAByName(cls, L"PalPlayerState"))
                {
                    SDK::APalPlayerState* ps = (SDK::APalPlayerState*)obj;
                    if (ps->GetInventoryData() || ps->GetTechnologyData())
                        g_PlayerState = ps;
                }

                if (!g_Weapon && Engine::IsAByName(cls, L"PalWeaponBase"))
                {
                    if (IsOwnedByPlayer(obj))
                        g_Weapon = (SDK::APalWeaponBase*)obj;
                }

                if (Engine::IsAByName(cls, L"PalCharacter"))
                {
                    SDK::APalCharacter* chr = (SDK::APalCharacter*)obj;
                    SDK::UPalCharacterParameterComponent* cp = chr->GetCharacterParameterComponent();
                    if (cp)
                    {
                        SDK::UPalIndividualCharacterParameter* ind = cp->GetIndividualParameter();
                        if (ind && !ind->SaveParameter.IsPlayer)
                            g_AllPals.push_back(chr);
                    }
                }

                if (Engine::IsAByName(cls, L"PalMapObjectModel"))
                    g_AllMapObjects.push_back((SDK::UPalMapObjectModel*)obj);

                if (!g_PlayerMovement && Engine::IsAByName(cls, L"PalCharacterMovementComponent"))
                {
                    if (IsOwnedByPlayer(obj))
                        g_PlayerMovement = (SDK::UPalCharacterMovementComponent*)obj;
                }

                if (!g_PlayerTemperature && Engine::IsAByName(cls, L"PalBodyTemperatureComponent"))
                {
                    if (IsOwnedByPlayer(obj))
                        g_PlayerTemperature = (SDK::UPalBodyTemperatureComponent*)obj;
                }

                if (!g_TimeManager && Engine::IsAByName(cls, L"PalTimeManager"))
                    g_TimeManager = (SDK::UPalTimeManager*)obj;

                if (!g_WorldSecurity && Engine::IsAByName(cls, L"PalWorldSecuritySystem"))
                    g_WorldSecurity = (SDK::UPalWorldSecuritySystem*)obj;

                if (!g_CheatManager && Engine::IsAByName(cls, L"PalCheatManager"))
                    g_CheatManager = obj;
            }
        }

        if (!g_CheatManager)
            g_CheatManager = Engine::FindCDO(L"PalCheatManager");

        if (g_PlayerMovement)
            CacheBaseMovement();
    }

    // ------------------------------------------------------------------------
    // Blocs d'application
    // ------------------------------------------------------------------------
    static void ApplyDebugSettings()
    {
        SDK::UPalDebugSetting* ds = (SDK::UPalDebugSetting*)g_DebugSetting;
        if (!ds) return;

        *ds->GetbIsMutekiALL()                = UnlimitedHealth;
        *ds->GetbIsFixedSP()                  = UnlimitedStamina;
        *ds->GetbIsHungerDisable()            = UnlimitedSatiety;
        *ds->GetbIgnoreItemDurabilityDecrease() = InfiniteDurability;
        *ds->GetbNotDecreaseWeaponItem()      = InfiniteDurability;
        *ds->GetbNotRequiredBulletWhenReload() = NoReload;
        *ds->GetbIsCaptureSuccessAlways()     = CaptureChanceAlways || InstantCapture;
        *ds->GetbIgnoreOverWeightMove()       = NoItemWeight;
        *ds->GetbNotConsumeMaterialsInBuild() = NoBuildingRequirements;
        *ds->GetbNotConsumeMaterialsInCraft() = NoCraftingRequirements;
        *ds->GetbIsIgnoreBuildRestrictionBaseCamp() = IgnoreBuildingOverlap || NoBuildingRequirements;

        *ds->GetbIsInstantFishing()           = InstantFishing;
        *ds->GetbIsInstantWorkProgress()      = InstantWorkProgress;
        *ds->GetbIsDisableHeatOverload()      = OverheatRifleNoHeat;
        if (FishSpeedPercent != 1.0f)
            *ds->GetFishSpeedRate()           = FishSpeedPercent;

        float timeRate = 1.0f;
        if (StopTime)
            timeRate = 0.0f;
        else if (DaySpeedRate != 1.0f || NightSpeedRate != 1.0f)
            timeRate = (DaySpeedRate + NightSpeedRate) * 0.5f;
        *ds->GetDebugRatePalWorldTime() = timeRate;

        *ds->GetWorkExtraRate() = (MassiveWorkSpeedPlayer || MassiveWorkSpeedAll || InstantWorkProgress) ? WorkSpeedRate : 1.0f;
        *ds->GetbIsFullPowerForPlayer() = MassiveWorkSpeedPlayer;

        if (PalLevelRandomMin > 0)
            *ds->GetForceFixLevelForWildPal() = PalLevelRandomMin;
        else if (PalLevelRandomMax > 0)
            *ds->GetForceFixLevelForWildPal() = PalLevelRandomMax;
    }

    static void ApplyGameSettings()
    {
        SDK::UPalGameSetting* gs = (SDK::UPalGameSetting*)g_GameSetting;
        if (!gs) return;

        if (AllPalsRare)
        {
            *gs->GetRarePalAppearanceProbability() = 1.0f;
            if (RarePalMultiplier > 0.0f)
                *gs->GetRarePalLevelMultiply() = RarePalMultiplier;
        }

        if (NoItemWeight)
            *gs->GetDefaultMaxInventoryWeight() = 999999.0f;

        if (UnlimitedHealth || RefillHealth)
            *gs->GetAutoHPRegene_Percent_perSecond() = 100.0f;
        else if (HealthRegenRate > 0.0f)
            *gs->GetAutoHPRegene_Percent_perSecond() = HealthRegenRate;

        if (UnlimitedSatiety || RefillSatiety)
        {
            *gs->GetStomachDecreace_perSecond_Player() = 0.0f;
            *gs->GetStomachDecreace_perSecond_Monster() = 0.0f;
        }
        else if (SatietyDecreaseRate >= 0.0f)
        {
            *gs->GetStomachDecreace_perSecond_Player()  = SatietyDecreaseRate;
            *gs->GetStomachDecreace_perSecond_Monster() = SatietyDecreaseRate;
        }

        if (LootDropMultiplier != 1.0f)
            *gs->GetDropItemRate() = LootDropMultiplier;

        if (XPMultiplier != 1.0f)
            *gs->GetExpRate() = XPMultiplier;
    }

    static void ApplyPlayerState()
    {
        if (!g_PlayerState) return;

        SDK::UPalTechnologyData* tech = g_PlayerState->GetTechnologyData();
        if (tech)
        {
            if (TechPoints >= 0)       *tech->GetTechnologyPoint()     = TechPoints;
            if (AncientTechPoints >= 0)*tech->GetBossTechnologyPoint() = AncientTechPoints;
        }

        SDK::UPalPlayerInventoryData* inv = g_PlayerState->GetInventoryData();
        if (inv)
        {
            if (NoItemWeight)
            {
                *inv->GetNowItemWeight()            = 1.0f;
                *inv->GetMaxInventoryWeight()       = 999999.0f;
                *inv->GetMaxInventoryWeightCached() = 999999.0f;
                *inv->GetPassiveBuffedMaxWeight()   = 999999.0f;
            }
        }
    }

    static void ApplyPlayerCharacter()
    {
        if (!g_PlayerCharacter) return;

        SDK::UPalCharacterParameterComponent* cp = g_PlayerCharacter->GetCharacterParameterComponent();
        SDK::UPalStaticCharacterParameterComponent* scp = g_PlayerCharacter->GetStaticCharacterParameterComponent();
        if (!cp) return;

        SDK::UPalIndividualCharacterParameter* ind = cp->GetIndividualParameter();
        if (!ind) return;

        SDK::FPalIndividualCharacterSaveParameter& sp = ind->SaveParameter;

        if (UnlimitedHealth || RefillHealth)
        {
            sp.HP.Value        = sp.MaxHP.Value;
            sp.ShieldHP.Value  = sp.ShieldMaxHP.Value;
        }

        if (InfiniteShield)
        {
            sp.ShieldHP.Value  = sp.ShieldMaxHP.Value;
        }

        if (UnlimitedStamina)
        {
            SDK::FFixedPoint64* spPtr = cp->GetSP();
            if (spPtr) spPtr->Value = sp.MaxSP.Value;
        }

        if (UnlimitedSatiety || RefillSatiety)
            sp.FullStomach = sp.MaxFullStomach;

        if (UnlimitedSanity)
            sp.SanityValue = 100.0f;

        if (TemperatureAlwaysNormal && g_PlayerTemperature)
            *g_PlayerTemperature->GetCurrentBodyState() = 0;

        if (StatPoints >= 0)
            sp.UnusedStatusPoint = StatPoints;

        if (SetLevel > 0) sp.Level = SetLevel;
        if (SetXP >= 0)   sp.Exp   = SetXP;
        if (SetRank > 0)  sp.Rank  = SetRank;

        if (InstantCrafting)
            sp.CraftSpeed = 100000;

        if (FoodWontSpoil)
        {
            // Freeze food decay by setting food decay rate to 0
            // The game checks FullStomach and decreases it; we keep it maxed
            sp.FullStomach = sp.MaxFullStomach;
        }

        if (UnlimitedBaseStats)
        {
            sp.Talent_HP      = 100;
            sp.Talent_Melee   = 100;
            sp.Talent_Shot    = 100;
            sp.Talent_Defense = 100;
            sp.Support        = 100;
            sp.CraftSpeed     = 100000;
        }

        if (SuperDamage)
        {
            sp.Talent_Melee = (int32)DamageMultiplier;
            sp.Talent_Shot  = (int32)DamageMultiplier;
        }

        if (UnlimitedHealth)
            *cp->GetIsEnableMuteki() = true;

        if (scp)
        {
            if (EveryoneCapturable)
                *scp->GetIsUncapturable() = false;
            if (CaptureMultiplier > 0.0f)
                *scp->GetCaptureSuccessRate() = CaptureMultiplier;
        }

        if (g_PlayerMovement && g_MoveBaseSet)
        {
            SDK::UPalCharacterMovementComponent* mv = g_PlayerMovement;
            if (WalkSpeedMultiplier != 1.0f)
            {
                *mv->GetSlowWalkSpeed_Default() = g_BaseSlowWalk * WalkSpeedMultiplier;
                *mv->GetWalkSpeed_Default()     = g_BaseWalk * WalkSpeedMultiplier;
            }
            if (SprintSpeedMultiplier != 1.0f)
            {
                *mv->GetSprintMaxSpeed()        = g_BaseSprintSpeed * SprintSpeedMultiplier;
                *mv->GetSprintMaxAcceleration() = g_BaseSprintAccel * SprintSpeedMultiplier;
                *mv->GetRunSpeed_Default()      = g_BaseRun * SprintSpeedMultiplier;
            }
            if (JumpHeightMultiplier != 1.0f)
            {
                *mv->GetBaseJumpZVelocity() = g_BaseJumpZ * JumpHeightMultiplier;
            }
            if (InstantAcceleration)
            {
                *mv->GetBaseMaxAcceleration() = 999999.0f;
            }
        }

        if (UnlimitedTorchDuration && g_Weapon)
        {
            SDK::UPalDynamicWeaponItemDataBase* torchDyn = g_Weapon->GetOwnWeaponDynamicData();
            if (torchDyn)
            {
                *torchDyn->GetDurability()    = *torchDyn->GetMaxDurability();
                *torchDyn->GetOldDurability() = *torchDyn->GetMaxDurability();
            }
        }
    }

    static void ApplyWeapon()
    {
        if (!g_Weapon) return;

        SDK::UPalDynamicWeaponItemDataBase* dyn = g_Weapon->GetOwnWeaponDynamicData();
        if (dyn)
        {
            if (NoReload)
            {
                SDK::UPalStaticWeaponItemData* st = g_Weapon->GetOwnWeaponStaticData();
                int32 mag = st ? *st->GetMagazineSize() : 99;
                *dyn->GetRemainingBullets() = mag;
            }
            if (InfiniteDurability)
            {
                *dyn->GetDurability()    = *dyn->GetMaxDurability();
                *dyn->GetOldDurability() = *dyn->GetMaxDurability();
            }
        }

        if (NoReload)
            *g_Weapon->GetCoolDownTime() = 0.0f;

        if (OverheatRifleNoHeat)
        {
            float* heat = g_Weapon->GetHeatValue();
            if (heat) *heat = 0.0f;
        }

        if (SuperDamage)
        {
            SDK::UPalStaticWeaponItemData* st = g_Weapon->GetOwnWeaponStaticData();
            if (st)
            {
                *st->GetAttackValue()  = (int32)DamageMultiplier;
                *st->GetDefenseValue() = (int32)DamageMultiplier;
            }
        }
    }

    static void ApplyCheatManagerFunctions()
    {
        if (!g_CheatManager) return;

        static SDK::UFunction* fnForceReleaseWanted = nullptr;
        static SDK::UFunction* fnSetSanityToBaseCampPal = nullptr;
        static SDK::UFunction* fnGetRelic = nullptr;
        static SDK::UFunction* fnRandomizePassive = nullptr;
        static SDK::UFunction* fnAddMoney = nullptr;
        static SDK::UFunction* fnMutekiPlayer = nullptr;
        static SDK::UFunction* fnMutekiFriend = nullptr;
        static int crimeFrame = 0;
        static int sanityFrame = 0;
        static int moneyFrame = 0;
        static int mutekiFrame = 0;
        static int flagFrame = 0;

        if (NoCrimeReporting && ++crimeFrame % 30 == 1)
        {
            if (!fnForceReleaseWanted)
                fnForceReleaseWanted = Engine::FindFunction(L"ForceReleaseWanted");
            if (fnForceReleaseWanted)
            {
                uint8_t dummy = 0;
                Engine::ProcessEvent(g_CheatManager, fnForceReleaseWanted, &dummy);
            }
        }

        if (MaxWorkerSanity && ++sanityFrame % 30 == 1)
        {
            if (!fnSetSanityToBaseCampPal)
                fnSetSanityToBaseCampPal = Engine::FindFunction(L"SetSanityToBaseCampPal");
            if (fnSetSanityToBaseCampPal)
            {
                struct { float Sanity; } params = { 100.0f };
                Engine::ProcessEvent(g_CheatManager, fnSetSanityToBaseCampPal, &params);
            }
        }

        if (LifmunkAmount >= 1)
        {
            int count = LifmunkAmount;
            bool gotRelic = false;
            if (!fnGetRelic)
                fnGetRelic = Engine::FindFunction(L"GetRelic");
            if (fnGetRelic)
            {
                struct { int32 Count; } params = { (int32)count };
                Engine::ProcessEvent(g_CheatManager, fnGetRelic, &params);
                gotRelic = true;
            }
            if (!gotRelic)
            {
                // Fallback géré dans ApplyItemAmounts en parcourant les emplacements.
            }
            LifmunkAmount = gotRelic ? -1 : LifmunkAmount;
        }

        if (PalRandomizer)
        {
            if (!fnRandomizePassive)
                fnRandomizePassive = Engine::FindFunction(L"RandomizePassive_PlayerWeapon");
            if (fnRandomizePassive)
            {
                uint8_t dummy = 0;
                Engine::ProcessEvent(g_CheatManager, fnRandomizePassive, &dummy);
            }
            PalRandomizer = false;
        }

        if (UnlimitedMoney && ++moneyFrame % 60 == 1)
        {
            if (!fnAddMoney)
                fnAddMoney = Engine::FindFunction(L"AddMoney");
            if (fnAddMoney)
            {
                struct { int64 AddValue; } params = { 99999999 };
                Engine::ProcessEvent(g_CheatManager, fnAddMoney, &params);
            }
        }

        if ((UnlimitedHealth || PalUnlimitedHealth) && ++mutekiFrame % 60 == 1)
        {
            if (UnlimitedHealth)
            {
                if (!fnMutekiPlayer)
                    fnMutekiPlayer = Engine::FindFunction(L"MutekiForPlayer");
                if (fnMutekiPlayer)
                {
                    uint8_t dummy = 0;
                    Engine::ProcessEvent(g_CheatManager, fnMutekiPlayer, &dummy);
                }
            }
            if (PalUnlimitedHealth)
            {
                if (!fnMutekiFriend)
                    fnMutekiFriend = Engine::FindFunction(L"MutekiForFriend");
                if (fnMutekiFriend)
                {
                    uint8_t dummy = 0;
                    Engine::ProcessEvent(g_CheatManager, fnMutekiFriend, &dummy);
                }
            }
        }

        if (++flagFrame % 60 == 1)
        {
            if (UnlimitedStamina || PalUnlimitedStamina)
            {
                static SDK::UFunction* fnFixedSP = nullptr;
                if (!fnFixedSP) fnFixedSP = Engine::FindFunction(L"FixedSP");
                if (fnFixedSP)
                {
                    uint8_t dummy = 0;
                    Engine::ProcessEvent(g_CheatManager, fnFixedSP, &dummy);
                }
            }
            if (CaptureChanceAlways || InstantCapture)
            {
                static SDK::UFunction* fnCaptureSuccessAlways = nullptr;
                if (!fnCaptureSuccessAlways) fnCaptureSuccessAlways = Engine::FindFunction(L"CaptureSuccessAlways");
                if (fnCaptureSuccessAlways)
                {
                    uint8_t dummy = 0;
                    Engine::ProcessEvent(g_CheatManager, fnCaptureSuccessAlways, &dummy);
                }
            }
            if (NoBuildingRequirements)
            {
                static SDK::UFunction* fnNotConsumeMaterialsInBuild = nullptr;
                if (!fnNotConsumeMaterialsInBuild) fnNotConsumeMaterialsInBuild = Engine::FindFunction(L"NotConsumeMaterialsInBuild");
                if (fnNotConsumeMaterialsInBuild)
                {
                    uint8_t dummy = 0;
                    Engine::ProcessEvent(g_CheatManager, fnNotConsumeMaterialsInBuild, &dummy);
                }
            }
            if (NoCraftingRequirements)
            {
                static SDK::UFunction* fnNotConsumeMaterialsInCraft = nullptr;
                if (!fnNotConsumeMaterialsInCraft) fnNotConsumeMaterialsInCraft = Engine::FindFunction(L"NotConsumeMaterialsInCraft");
                if (fnNotConsumeMaterialsInCraft)
                {
                    uint8_t dummy = 0;
                    Engine::ProcessEvent(g_CheatManager, fnNotConsumeMaterialsInCraft, &dummy);
                }
            }
        }
    }

    static void ApplyWorld()
    {
        // Gestion des crimes déplacée vers ApplyCheatManagerFunctions pour des appels ProcessEvent centralisés.
        (void)0;
    }

    static void ApplyTimeFunctions()
    {
        static SDK::UFunction* fnSetPalWorldTime = nullptr;
        static SDK::UFunction* fnAddGameTimeHours = nullptr;
        static SDK::UFunction* fnSetPalWorldTimeScale = nullptr;
        static int lastSetHour = -2;

        if (!g_CheatManager) return;

        if (SetHour >= 0 && SetHour != lastSetHour)
        {
            if (!fnSetPalWorldTime)
                fnSetPalWorldTime = Engine::FindFunction(L"SetPalWorldTime");
            if (fnSetPalWorldTime)
            {
                struct { int32 Hour; } params = { (int32)SetHour };
                Engine::ProcessEvent(g_CheatManager, fnSetPalWorldTime, &params);
                lastSetHour = SetHour;
            }
        }

        if (AdvanceHours != 0)
        {
            if (!fnAddGameTimeHours)
                fnAddGameTimeHours = Engine::FindFunction(L"AddGameTime_Hours");
            if (fnAddGameTimeHours)
            {
                struct { int32 Hours; } params = { (int32)AdvanceHours };
                Engine::ProcessEvent(g_CheatManager, fnAddGameTimeHours, &params);
            }
            AdvanceHours = 0;
        }

        if (RewindHours != 0)
        {
            if (!fnAddGameTimeHours)
                fnAddGameTimeHours = Engine::FindFunction(L"AddGameTime_Hours");
            if (fnAddGameTimeHours)
            {
                struct { int32 Hours; } params = { -(int32)RewindHours };
                Engine::ProcessEvent(g_CheatManager, fnAddGameTimeHours, &params);
            }
            RewindHours = 0;
        }

        if (fnSetPalWorldTimeScale)
        {
            // Réservé aux appels directs d'échelle de temps si le chemin debug-setting s'avère insuffisant.
            (void)fnSetPalWorldTimeScale;
        }
    }

    static void ApplyMapObjectHealth()
    {
        if (!UnlimitedBaseHP) return;
        for (SDK::UPalMapObjectModel* model : g_AllMapObjects)
        {
            if (!model) continue;
            SDK::FPalMapObjectStatusValue* hp = model->GetHP();
            if (hp && hp->Max > 0.0f)
                hp->Current = hp->Max;
        }
    }

    static void ApplyAllPals()
    {
        for (SDK::APalCharacter* chr : g_AllPals)
        {
            if (!chr) continue;
            SDK::UPalCharacterParameterComponent* cp = chr->GetCharacterParameterComponent();
            SDK::UPalStaticCharacterParameterComponent* scp = chr->GetStaticCharacterParameterComponent();
            if (!cp) continue;
            SDK::UPalIndividualCharacterParameter* ind = cp->GetIndividualParameter();
            if (!ind) continue;
            SDK::FPalIndividualCharacterSaveParameter& sp = ind->SaveParameter;

            if (PalUnlimitedHealth)
            {
                sp.HP.Value       = sp.MaxHP.Value;
                sp.ShieldHP.Value = sp.ShieldMaxHP.Value;
            }
            if (PalUnlimitedStamina)
            {
                SDK::FFixedPoint64* spPtr = cp->GetSP();
                if (spPtr) spPtr->Value = sp.MaxSP.Value;
            }
            if (PalUnlimitedSatiety)
                sp.FullStomach = sp.MaxFullStomach;
            if (PalUnlimitedSanity || MaxWorkerSanity)
                sp.SanityValue = 100.0f;

            if (PalMaxStats)
            {
                sp.Talent_HP     = 100;
                sp.Talent_Melee  = 100;
                sp.Talent_Shot   = 100;
                sp.Talent_Defense= 100;
                sp.Rank_HP       = 5;
                sp.Rank_Attack   = 5;
                sp.Rank_Defence  = 5;
                sp.Rank_CraftSpeed = 5;
                sp.Support       = 100;
            }

            if (OneHitKill)
            {
                // Set enemy pal HP to 1 for one-hit kills
                sp.HP.Value = 1;
            }

            if (PalInstantSkillCooldown)
            {
                // Zero out skill cooldowns for owned pals
                // Check if this pal is owned by the player
                if (IsOwnedByPlayer(reinterpret_cast<SDK::UObject*>(chr)))
                {
                    // Attempt to reset cooldown timers via the parameter component
                    // The game stores skill cooldowns in the character's skill component
                    // We set the cooldown values to 0 via available offsets
                    SDK::UPalCharacterParameterComponent* pcp = cp;
                    (void)pcp; // Cooldown reset would go here if offset is known
                }
            }

            if (SuperDamage)
            {
                sp.Talent_Melee = (int32)DamageMultiplier;
                sp.Talent_Shot  = (int32)DamageMultiplier;
            }

            if (scp)
            {
                if (EveryoneCapturable)
                    *scp->GetIsUncapturable() = false;
                if (CaptureMultiplier > 0.0f)
                    *scp->GetCaptureSuccessRate() = CaptureMultiplier;
            }
        }
    }

    static void ApplyNewCheats()
    {
        // StealthMode: reduce enemy aggro/detection range
        if (StealthMode && g_DebugSetting)
        {
            SDK::UPalDebugSetting* ds = (SDK::UPalDebugSetting*)g_DebugSetting;
            // Disable enemy detection via debug settings
            // The game has a debug flag for ignoring player detection
            (void)ds; // Detection disable would go here if offset is known
        }

        // DropRateAlways: force 100% drop rate via game settings
        if (DropRateAlways && g_GameSetting)
        {
            SDK::UPalGameSetting* gs = (SDK::UPalGameSetting*)g_GameSetting;
            // Set drop rate multiplier to maximum
            // The game stores drop rate in the game settings
            (void)gs; // Drop rate override would go here if offset is known
        }

        // InfiniteExp: auto-add XP each tick via cheat manager
        if (InfiniteExp && g_CheatManager)
        {
            static int expFrame = 0;
            if (++expFrame % 30 == 1)
            {
                static SDK::UFunction* fnAddExp = nullptr;
                if (!fnAddExp) fnAddExp = Engine::FindFunction(L"AddExp");
                if (fnAddExp)
                {
                    struct { int32 Exp; } params = { 100000 };
                    Engine::ProcessEvent(g_CheatManager, fnAddExp, &params);
                }
            }
        }

        // UnlockWorldTree: unlock World Tree content via cheat manager
        if (UnlockWorldTree && g_CheatManager)
        {
            static int wtFrame = 0;
            if (++wtFrame % 120 == 1)
            {
                static SDK::UFunction* fnUnlockWorldTree = nullptr;
                if (!fnUnlockWorldTree) fnUnlockWorldTree = Engine::FindFunction(L"UnlockWorldTree");
                if (fnUnlockWorldTree)
                {
                    uint8_t dummy = 0;
                    Engine::ProcessEvent(g_CheatManager, fnUnlockWorldTree, &dummy);
                }
            }
        }

        // UnlockAwakening: unlock Awakening dungeon via cheat manager
        if (UnlockAwakening && g_CheatManager)
        {
            static int awFrame = 0;
            if (++awFrame % 120 == 1)
            {
                static SDK::UFunction* fnUnlockAwakening = nullptr;
                if (!fnUnlockAwakening) fnUnlockAwakening = Engine::FindFunction(L"UnlockAwakeningDungeon");
                if (fnUnlockAwakening)
                {
                    uint8_t dummy = 0;
                    Engine::ProcessEvent(g_CheatManager, fnUnlockAwakening, &dummy);
                }
            }
        }

        // UnlockAllTowerBosses: unlock all tower boss rematches
        if (UnlockAllTowerBosses && g_CheatManager)
        {
            static int tbFrame = 0;
            if (++tbFrame % 120 == 1)
            {
                static SDK::UFunction* fnUnlockTowerBosses = nullptr;
                if (!fnUnlockTowerBosses) fnUnlockTowerBosses = Engine::FindFunction(L"UnlockAllTowerBoss");
                if (fnUnlockTowerBosses)
                {
                    uint8_t dummy = 0;
                    Engine::ProcessEvent(g_CheatManager, fnUnlockTowerBosses, &dummy);
                }
            }
        }
    }

    static void ScanPlayerItemSlots()
    {
        g_PlayerItemSlots.clear();
        if (!g_PlayerCharacter) return;

        SDK::TUObjectArray* arr = GetObjectArray();
        if (!arr) return;

        int32 perChunk = arr->MaxChunks > 0 ? (arr->MaxElements / arr->MaxChunks) : arr->MaxElements;
        if (perChunk <= 0) perChunk = arr->MaxElements;

        for (int32 c = 0; c < arr->NumChunks; ++c)
        {
            SDK::FUObjectItem* chunk = arr->Objects[c];
            if (!chunk) continue;

            for (int32 i = 0; i < perChunk && (c * perChunk + i) < arr->NumElements; ++i)
            {
                SDK::UObject* obj = chunk[i].Object;
                if (!obj) continue;

                if (!Engine::IsAByName(obj->ClassPrivate, L"PalItemSlot"))
                    continue;

                if (IsOwnedByPlayer(obj))
                    g_PlayerItemSlots.push_back((SDK::UPalItemSlot*)obj);
            }
        }
    }

    static void ApplyItemAmounts()
    {
        if (ItemAmount < 0 && LifmunkAmount < 0) return;
        if (!g_PlayerCharacter) return;

        // Rescan item slots every 30 frames
        if (++g_ItemSlotScanFrame % 30 == 0 || g_PlayerItemSlots.empty())
            ScanPlayerItemSlots();

        for (SDK::UPalItemSlot* slot : g_PlayerItemSlots)
        {
            if (!slot) continue;

            if (ItemAmount >= 1)
                *slot->GetStackCount() = ItemAmount;

            if (LifmunkAmount >= 1)
            {
                std::wstring name = Engine::FNameToString(slot->GetItemId()->StaticId);
                if (name.find(L"Lifmunk") != std::wstring::npos)
                    *slot->GetStackCount() = LifmunkAmount;
            }
        }
    }

    // ------------------------------------------------------------------------
    // API publique
    // ------------------------------------------------------------------------
    bool Initialize()
    {
        if (g_Initialized) return true;
        if (!Engine::Initialize()) return false;

        g_DebugSetting = Engine::FindCDO(L"PalDebugSetting");
        g_GameSetting  = Engine::FindCDO(L"PalGameSetting");

        CacheObjects();
        g_Initialized = true;
        return true;
    }

    void Update()
    {
        if (!g_Initialized) return;

        ++g_Frame;

        // Quick refresh every 30 frames (cheap GWorld chain only)
        if (g_Frame % 30 == 0)
            QuickRefresh();

        // Full GObjects scan every 120 frames or if key pointers are invalid
        bool needFullScan = (g_Frame % 120 == 0);
        if (g_PlayerCharacter && !Engine::IsValidPointer((uintptr_t)g_PlayerCharacter))
            needFullScan = true;
        if (g_PlayerState && !Engine::IsValidPointer((uintptr_t)g_PlayerState))
            needFullScan = true;
        if (needFullScan)
            CacheObjects();

        ApplyDebugSettings();
        ApplyGameSettings();
        ApplyPlayerState();
        ApplyPlayerCharacter();
        ApplyWeapon();
        ApplyWorld();
        ApplyTimeFunctions();
        ApplyCheatManagerFunctions();
        ApplyAllPals();
        ApplyMapObjectHealth();
        ApplyNewCheats();
        ApplyItemAmounts();
    }

    void Shutdown()
    {
        g_Initialized = false;
    }

    void SetAllItemCounts(int amount)
    {
        ItemAmount = amount;
    }

    void SetLifmunkEffigyCount(int amount)
    {
        LifmunkAmount = amount;
    }
}
