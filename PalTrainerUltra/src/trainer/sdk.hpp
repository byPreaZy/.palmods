#pragma once

#include "types.hpp"

namespace SDK
{

// ----------------------------------------------------------------------------
// Tableau d'objets globaux
// ----------------------------------------------------------------------------

struct FUObjectItem
{
    class UObject* Object;
    int32 Flags;
    int32 ClusterIndex;
    int32 SerialNumber;
    int32 pad;
};

struct TUObjectArray
{
    FUObjectItem** Objects;
    int32 NumElements;
    int32 MaxElements;
    int32 NumChunks;
    int32 MaxChunks;
};

namespace Offsets
{
    constexpr size_t GObjects     = 0x087B7100;
    constexpr size_t AppendString = 0x02CDEF30;
    constexpr size_t ProcessEvent = 0x02E610C0;
}
// ----------------------------------------------------------------------------
// Types de base du moteur
// ----------------------------------------------------------------------------

class UObject
{
public:
    void** VTable;
    uint32 ObjectFlags;
    uint32 InternalIndex;
    class UClass* ClassPrivate;
    FName NamePrivate;
    UObject* OuterPrivate;
};

class UClass : public UObject
{
public:
    uint8_t _pad[0x238];

    // UStruct
    UClass* GetSuperStruct() const { return *(UClass**)((uint8_t*)this + 0x40); }
    // UClass
    UObject* GetDefaultObject() const { return *(UObject**)((uint8_t*)this + 0x110); }
};

class UFunction : public UObject
{
public:
    uint8_t _pad[0x40];

    // Suffisant pour l'utilisation du pointeur / ProcessEvent ; la véritable UFunction est plus grande.
};

// ----------------------------------------------------------------------------
// Layout du paramètre de sauvegarde (taille 0x218)
// ----------------------------------------------------------------------------

struct FPalIndividualCharacterSaveParameter
{
    FName CharacterID;                          // 0x000
    FName UniqueNPCID;                          // 0x008
    uint8_t Gender;                             // 0x010
    uint8_t _pad011[0x7];
    TSubclassOf<class APalCharacter> CharacterClass; // 0x018
    int32 Level;                                // 0x020
    int32 Rank;                                 // 0x024
    int32 Rank_HP;                              // 0x028
    int32 Rank_Attack;                          // 0x02C
    int32 Rank_Defence;                         // 0x030
    int32 Rank_CraftSpeed;                      // 0x034
    int32 Exp;                                  // 0x038
    uint8_t _pad03C[0x4];
    FString NickName;                           // 0x040
    bool IsRarePal;                             // 0x050
    uint8_t _pad051[0x7];
    TArray<int32> EquipWaza;                    // 0x058
    TArray<int32> MasteredWaza;                 // 0x068
    FFixedPoint64 HP;                           // 0x078
    int32 Talent_HP;                            // 0x080
    int32 Talent_Melee;                         // 0x084
    int32 Talent_Shot;                          // 0x088
    int32 Talent_Defense;                       // 0x08C
    float FullStomach;                          // 0x090
    uint8_t PhysicalHealth;                     // 0x094
    uint8_t WorkerSick;                         // 0x095
    uint8_t _pad096[0x2];
    TArray<FName> PassiveSkillList;             // 0x098
    int32 DyingTimer;                           // 0x0A8
    uint8_t _pad0AC[0x4];
    FFixedPoint64 MP;                           // 0x0B0
    bool IsPlayer;                              // 0x0B8
    uint8_t _pad0B9[0x7];
    FDateTimePlaceholder OwnedTime;             // 0x0C0
    FGuid OwnerPlayerUId;                       // 0x0C8
    TArray<FGuid> OldOwnerPlayerUIds;           // 0x0D8
    FFixedPoint64 MaxHP;                        // 0x0E8
    int32 Support;                              // 0x0F0
    int32 CraftSpeed;                           // 0x0F4
    TArray<uint8_t> CraftSpeeds;                // 0x0F8
    FFixedPoint64 ShieldHP;                     // 0x108
    FFixedPoint64 ShieldMaxHP;                  // 0x110
    FFixedPoint64 MaxMP;                        // 0x118
    FFixedPoint64 MaxSP;                        // 0x120
    uint8_t HungerType;                         // 0x128
    uint8_t _pad129[0x3];
    float SanityValue;                          // 0x12C
    uint8_t BaseCampWorkerEventType;            // 0x130
    uint8_t _pad131[0x3];
    float BaseCampWorkerEventProgressTime;      // 0x134
    FPalContainerId ItemContainerId;            // 0x138
    FPalContainerId EquipItemContainerId;       // 0x148
    FPalCharacterSlotId SlotID;                 // 0x158
    float MaxFullStomach;                       // 0x16C
    float FullStomachDecreaseRate_Tribe;        // 0x170
    int32 UnusedStatusPoint;                    // 0x174
    TArray<uint8_t> GotStatusPointList;         // 0x178
    FFloatContainer DecreaseFullStomachRates;   // 0x188
    FFloatContainer AffectSanityRates;          // 0x198
    FFloatContainer CraftSpeedRates;            // 0x1A8
    FVector LastJumpedLocation;                 // 0x1B8
    FName FoodWithStatusEffect;                 // 0x1D0
    int32 Tiemr_FoodWithStatusEffect;           // 0x1D8
    uint8_t CurrentWorkSuitability;             // 0x1DC
    bool bAppliedDeathPenarty;                  // 0x1DD
    uint8_t _pad1DE[0x2];
    float PalReviveTimer;                       // 0x1E0
    int32 VoiceID;                              // 0x1E4
    uint8_t Dynamic[0x2C];                      // 0x1E8
    uint8_t _pad214[0x4];
};

// ----------------------------------------------------------------------------
// Classes Pal (basées sur des accesseurs, offsets du plan d'intégration)
// ----------------------------------------------------------------------------

class UPalCharacterParameterComponent
{
public:
    uint8_t _pad[0x420];

    bool* GetIsEnableMuteki()      { return (bool*)((uint8_t*)this + 0x0A2); }
    bool* GetIsSPOverheat()        { return (bool*)((uint8_t*)this + 0x0C8); }
    UPalIndividualCharacterParameter* GetIndividualParameter() { return *(UPalIndividualCharacterParameter**)((uint8_t*)this + 0x108); }
    FFixedPoint64* GetSP()         { return (FFixedPoint64*)((uint8_t*)this + 0x2F0); }
    float* GetDyingHP()            { return (float*)((uint8_t*)this + 0x410); }
    float* GetDyingMaxHP()         { return (float*)((uint8_t*)this + 0x414); }
    class UPalItemContainer* GetItemContainer() { return *(class UPalItemContainer**)((uint8_t*)this + 0x418); }
};

class UPalIndividualCharacterParameter
{
public:
    uint8_t _pad[0x270];
    FPalIndividualCharacterSaveParameter SaveParameter; // 0x270
    uint8_t _pad488[0x310];
    class UPalItemContainer* EquipItemContainer; // 0x798
    FGuid BaseCampId;                           // 0x7A0
    FString Debug_CurrentAIActionName;          // 0x7B0

    APalCharacter* GetIndividualActor() const { return *(APalCharacter**)((uint8_t*)this + 0x1D8); }
};

class APalCharacter
{
public:
    uint8_t _pad[0x638];

    UPalCharacterParameterComponent* GetCharacterParameterComponent() const { return *(UPalCharacterParameterComponent**)((uint8_t*)this + 0x628); }
    UPalStaticCharacterParameterComponent* GetStaticCharacterParameterComponent() const { return *(UPalStaticCharacterParameterComponent**)((uint8_t*)this + 0x630); }
};

class APalPlayerCharacter
{
public:
    uint8_t _pad[0x818];

    class UPalShooterComponent* GetShooterComponent() const { return *(class UPalShooterComponent**)((uint8_t*)this + 0x800); }
    class UPalInteractComponent* GetInteractComponent() const { return *(class UPalInteractComponent**)((uint8_t*)this + 0x808); }
    class UPalBuilderComponent* GetBuilderComponent() const { return *(class UPalBuilderComponent**)((uint8_t*)this + 0x810); }
};

class UPalCharacterMovementComponent
{
public:
    uint8_t _pad[0x1700];

    float* GetSprintMaxSpeed()          { return (float*)((uint8_t*)this + 0xFB0); }
    float* GetSprintMaxAcceleration()   { return (float*)((uint8_t*)this + 0xFB4); }
    float* GetSprintYawRate()           { return (float*)((uint8_t*)this + 0xFB8); }
    float* GetSlowWalkSpeed_Default()   { return (float*)((uint8_t*)this + 0x15E0); }
    float* GetWalkSpeed_Default()       { return (float*)((uint8_t*)this + 0x15E4); }
    float* GetRunSpeed_Default()        { return (float*)((uint8_t*)this + 0x15E8); }
    float* GetRideSprintSpeed_Default() { return (float*)((uint8_t*)this + 0x15EC); }
    float* GetBaseJumpZVelocity()       { return (float*)((uint8_t*)this + 0x0178); }
    float* GetBaseGravityScale()        { return (float*)((uint8_t*)this + 0x0170); }
    float* GetBaseMaxWalkSpeed()        { return (float*)((uint8_t*)this + 0x01E8); }
    float* GetBaseMaxAcceleration()     { return (float*)((uint8_t*)this + 0x01FC); }
    float* GetBaseAirControl()          { return (float*)((uint8_t*)this + 0x0220); }
    float* GetInWaterRate()             { return (float*)((uint8_t*)this + 0x1604); }
    float* GetDashSwimMaxSpeed()        { return (float*)((uint8_t*)this + 0x1608); }
};

class APalPlayerState
{
public:
    uint8_t _pad[0x560];

    FGuid* GetPlayerUId()                   { return (FGuid*)((uint8_t*)this + 0x4B0); }
    class UPalPlayerOtomoData* GetOtomoData() { return *(class UPalPlayerOtomoData**)((uint8_t*)this + 0x530); }
    UPalPlayerInventoryData* GetInventoryData() { return *(UPalPlayerInventoryData**)((uint8_t*)this + 0x540); }
    class UPalPlayerDataPalStorage* GetPalStorage() { return *(class UPalPlayerDataPalStorage**)((uint8_t*)this + 0x548); }
    UPalTechnologyData* GetTechnologyData() { return *(UPalTechnologyData**)((uint8_t*)this + 0x550); }
    class UPalPlayerRecordData* GetRecordData() { return *(class UPalPlayerRecordData**)((uint8_t*)this + 0x558); }
};

class UPalPlayerInventoryData
{
public:
    uint8_t _pad[0x180];

    float* GetNowItemWeight()            { return (float*)((uint8_t*)this + 0x150); }
    float* GetMaxInventoryWeight()       { return (float*)((uint8_t*)this + 0x154); }
    float* GetMaxInventoryWeightCached() { return (float*)((uint8_t*)this + 0x158); }
    float* GetPassiveBuffedMaxWeight()   { return (float*)((uint8_t*)this + 0x168); }
    FGuid* GetOwnerPlayerUId()           { return (FGuid*)((uint8_t*)this + 0x16C); }
};

class UPalTechnologyData
{
public:
    uint8_t _pad[0x150];

    int32* GetTechnologyPoint()     { return (int32*)((uint8_t*)this + 0x140); }
    int32* GetBossTechnologyPoint() { return (int32*)((uint8_t*)this + 0x144); }
};

class UPalItemContainer
{
public:
    uint8_t _pad[0xB0];

    FPalContainerId* GetID()              { return (FPalContainerId*)((uint8_t*)this + 0x38); }
    TArray<UPalItemSlot*>* GetSlotArray() { return (TArray<UPalItemSlot*>*)((uint8_t*)this + 0x60); }
    float* GetCorruptionMultiplier()      { return (float*)((uint8_t*)this + 0xA0); }
};

class UPalItemSlot
{
public:
    uint8_t _pad[0x148];

    int32* GetSlotIndex()           { return (int32*)((uint8_t*)this + 0xC8); }
    FPalItemId* GetItemId()         { return (FPalItemId*)((uint8_t*)this + 0xDC); }
    int32* GetStackCount()          { return (int32*)((uint8_t*)this + 0x104); }
    float* GetCorruptionProgressValue() { return (float*)((uint8_t*)this + 0x108); }
    class UPalDynamicItemDataBase* GetDynamicItemData() { return *(class UPalDynamicItemDataBase**)((uint8_t*)this + 0x140); }
};

class UPalGameSetting
{
public:
    uint8_t _pad[0x1000];

    int32* GetCharacterMaxLevel()                  { return (int32*)((uint8_t*)this + 0x028); }
    float* GetRarePalAppearanceProbability()       { return (float*)((uint8_t*)this + 0x054); }
    float* GetRarePalLevelMultiply()               { return (float*)((uint8_t*)this + 0x058); }
    float* GetAutoHPRegene_Percent_perSecond()     { return (float*)((uint8_t*)this + 0x2D4); }
    float* GetAutoHPRegene_Percent_perSecond_Sleeping() { return (float*)((uint8_t*)this + 0x2D8); }
    float* GetStomachDecreace_perSecond_Player()   { return (float*)((uint8_t*)this + 0x2EC); }
    float* GetStomachDecreace_perSecond_Monster()  { return (float*)((uint8_t*)this + 0x2E8); }
    float* GetDefaultMaxInventoryWeight()          { return (float*)((uint8_t*)this + 0xD04); }
    float* GetInventoryWeightAlertRate()           { return (float*)((uint8_t*)this + 0xCC0); }
};

class UPalDebugSetting
{
public:
    uint8_t _pad[0x1C0];

    int32* GetForceFixLevelForWildPal() { return (int32*)((uint8_t*)this + 0x0F0); }
    float* GetDebugRatePalWorldTime()   { return (float*)((uint8_t*)this + 0x144); }
    bool* GetbIgnoreOverWeightMove()    { return (bool*)((uint8_t*)this + 0x148); }
    bool* GetbIgnoreItemDurabilityDecrease() { return (bool*)((uint8_t*)this + 0x14C); }
    bool* GetbIsMutekiALL()             { return (bool*)((uint8_t*)this + 0x14E); }
    bool* GetbIsFixedSP()               { return (bool*)((uint8_t*)this + 0x154); }
    bool* GetbIsFullPowerForPlayer()    { return (bool*)((uint8_t*)this + 0x156); }
    bool* GetbIsCaptureSuccessAlways()  { return (bool*)((uint8_t*)this + 0x157); }
    bool* GetbIsIgnoreBuildRestrictionBaseCamp() { return (bool*)((uint8_t*)this + 0x164); }
    bool* GetbNotConsumeMaterialsInBuild() { return (bool*)((uint8_t*)this + 0x16C); }
    bool* GetbNotConsumeMaterialsInCraft() { return (bool*)((uint8_t*)this + 0x1D5); }
    bool* GetbIsHungerDisable()         { return (bool*)((uint8_t*)this + 0x177); }
    bool* GetbNotDecreaseWeaponItem()   { return (bool*)((uint8_t*)this + 0x179); }
    bool* GetbNotRequiredBulletWhenReload() { return (bool*)((uint8_t*)this + 0x17A); }
    bool* GetbIsDisableDropItem()       { return (bool*)((uint8_t*)this + 0x188); }
    float* GetWorkExtraRate()           { return (float*)((uint8_t*)this + 0x1B0); }
};

class UPalCheatManager
{
public:
    uint8_t _pad[0x1000];
};

class APalWeaponBase
{
public:
    uint8_t _pad[0x520];

    float* GetCoolDownTime()       { return (float*)((uint8_t*)this + 0x44C); }
    FPalItemId* GetOwnItemID()     { return (FPalItemId*)((uint8_t*)this + 0x46C); }
    FPalItemData* GetOwnItemData() { return (FPalItemData*)((uint8_t*)this + 0x498); }
    UPalStaticWeaponItemData* GetOwnWeaponStaticData() { return *(UPalStaticWeaponItemData**)((uint8_t*)this + 0x4A8); }
    UPalDynamicWeaponItemDataBase* GetOwnWeaponDynamicData() { return *(UPalDynamicWeaponItemDataBase**)((uint8_t*)this + 0x4B0); }
    bool* GetIsOneBulletReloadWeapon() { return (bool*)((uint8_t*)this + 0x515); }
};

class UPalStaticWeaponItemData
{
public:
    uint8_t _pad[0x170];

    float* GetDurability()         { return (float*)((uint8_t*)this + 0x120); }
    int32* GetMagazineSize()       { return (int32*)((uint8_t*)this + 0x158); }
    float* GetSneakAttackRate()    { return (float*)((uint8_t*)this + 0x15C); }
    int32* GetAttackValue()        { return (int32*)((uint8_t*)this + 0x160); }
    int32* GetDefenseValue()       { return (int32*)((uint8_t*)this + 0x164); }
};

class UPalDynamicWeaponItemDataBase
{
public:
    uint8_t _pad[0x90];

    float* GetDurability()         { return (float*)((uint8_t*)this + 0x70); }
    float* GetMaxDurability()      { return (float*)((uint8_t*)this + 0x74); }
    float* GetOldDurability()      { return (float*)((uint8_t*)this + 0x78); }
    int32* GetRemainingBullets()   { return (int32*)((uint8_t*)this + 0x7C); }
};

class UPalMapObjectModel
{
public:
    uint8_t _pad[0x220];

    FPalMapObjectStatusValue* GetHP() { return (FPalMapObjectStatusValue*)((uint8_t*)this + 0x214); }
};

class UPalTimeManager
{
public:
    uint8_t _pad[0x1000];
};

class UPalWorldSecuritySystem
{
public:
    uint8_t _pad[0x1000];
};

class UPalStaticCharacterParameterComponent
{
public:
    uint8_t _pad[0x478];

    float* GetCaptureSuccessRate() { return (float*)((uint8_t*)this + 0x358); }
    bool* GetIsUncapturable()      { return (bool*)((uint8_t*)this + 0x470); }
};

class UPalBodyTemperatureComponent
{
public:
    uint8_t _pad[0x148];

    uint8_t* GetCurrentBodyState() { return (uint8_t*)((uint8_t*)this + 0x140); }
};

class UPalTemperatureComponent
{
public:
    uint8_t _pad[0x5E0];

    int32* GetCurrentTemperature() { return (int32*)((uint8_t*)this + 0x5D8); }
};

} // namespace SDK
