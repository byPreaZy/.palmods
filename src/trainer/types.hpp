#pragma once

#include <cstdint>
#include <cstddef>
#include <string>

using int8  = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;
using uint8  = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;

namespace SDK
{

// Déclarations anticipées
class UObject;
class UClass;
class AActor;
class UActorComponent;
class UCharacterMovementComponent;
class UPalStaticItemDataBase;
class UPalDynamicItemDataBase;
class APalCharacter;
class APalPlayerCharacter;
class UPalCharacterParameterComponent;
class UPalIndividualCharacterParameter;
class UPalCharacterMovementComponent;
class APalPlayerState;
class UPalPlayerInventoryData;
class UPalTechnologyData;
class UPalItemContainer;
class UPalItemSlot;
class UPalGameSetting;
class UPalDebugSetting;
class UPalCheatManager;
class APalWeaponBase;
class UPalDynamicWeaponItemDataBase;
class UPalStaticWeaponItemData;
class UPalMapObjectModel;
class UPalTimeManager;
class UPalWorldSecuritySystem;
class UPalStaticCharacterParameterComponent;
class UPalTemperatureComponent;
class UPalBodyTemperatureComponent;
class UPalShooterComponent;
class UPalInteractComponent;
class UPalBuilderComponent;
class UPalPlayerOtomoData;
class UPalPlayerDataPalStorage;
class UPalPlayerRecordData;

// Types de conteneurs de base

template<typename T>
struct TArray
{
    T* Data;
    int32 Count;
    int32 Max;

    T& operator[](int32 i) { return Data[i]; }
    const T& operator[](int32 i) const { return Data[i]; }
    int32 Num() const { return Count; }
    bool IsValid() const { return Data != nullptr; }
};

template<typename K, typename V>
struct TMap
{
    uint8_t Pad[0x50];
};

template<typename T>
struct TSubclassOf
{
    UClass* Class;
};

template<typename T>
struct TWeakObjectPtr
{
    int32 ObjectIndex;
    int32 ObjectSerialNumber;
};

// Structures du moteur

struct FName
{
    uint32 ComparisonIndex;
    uint32 Number;

    bool operator==(const FName& other) const {
        return ComparisonIndex == other.ComparisonIndex && Number == other.Number;
    }
};

struct FString : public TArray<wchar_t>
{
    std::wstring ToWString() const {
        if (!Data || Count <= 0) return {};
        return { Data, (size_t)Count };
    }
};

struct FGuid
{
    uint32 A;
    uint32 B;
    uint32 C;
    uint32 D;

    bool operator==(const FGuid& other) const {
        return A == other.A && B == other.B && C == other.C && D == other.D;
    }

    bool IsValid() const {
        return A != 0 || B != 0 || C != 0 || D != 0;
    }
};

struct FVector
{
    double X;
    double Y;
    double Z;
};

struct FQuat
{
    double X;
    double Y;
    double Z;
    double W;
};

struct FTransform
{
    FQuat Rotation;
    FVector Translation;
    FVector Scale3D;
};

struct FFixedPoint64
{
    int64 Value;
};

struct FPalInstanceID
{
    uint8_t Pad[0x30];
};

struct FPalContainerId
{
    uint8_t Pad[0x10];
};

struct FPalDynamicItemId
{
    uint8_t Pad[0x20];
};

struct FPalItemId
{
    FName StaticId;
    FPalDynamicItemId DynamicId;
};

struct FPalItemData
{
    class UPalStaticItemDataBase* StaticData;
    class UPalDynamicItemDataBase* DynamicData;
};

struct FPalMapObjectStatusValue
{
    float Current;
    float Max;
};

// Espaces réservés pour les structs UE/Pal utilisées dans les layouts
struct FDateTimePlaceholder { uint64_t Ticks; };
struct FFloatContainer { uint8_t Pad[0x10]; };
struct FPalCharacterSlotId { uint8_t Pad[0x14]; };

// Espace réservé pour les structs inconnues / grandes
struct FPalPlayerDataInventoryInfo { uint8_t Pad[0x60]; };

} // namespace SDK
