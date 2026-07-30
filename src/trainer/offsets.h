#ifndef OFFSETS_H
#define OFFSETS_H

#include <cstdint>

// RVAs statiques trouvés dans Palworld-Win64-Shipping.exe 1.0.1
namespace PalTrainerOffsets {
    constexpr uintptr_t GWorldRva      = 0x965b1e0;
    constexpr uintptr_t GObjectRva     = 0x94ec890;
    constexpr uintptr_t FNameRva       = 0x34c08c4;
    constexpr uintptr_t AppendStringRva= 0x34c652f;
    constexpr uintptr_t ProcessEventRva= 0x3645020;

    // En-tête UObject
    constexpr size_t UObject_ClassOffset = 0x10;
    constexpr size_t UObject_OuterOffset = 0x18;
    constexpr size_t UObject_FNameOffset = 0x20; // généralement FName Index à 0x20 ? En fait FName Index à 0x20, Number à 0x24 ? À vérifier plus tard

    // UWorld
    constexpr size_t UWorld_PersistentLevel      = 0x030;
    constexpr size_t UWorld_OwningGameInstance   = 0x1B8;
    constexpr size_t UWorld_GameState            = 0x158;

    // UGameInstance
    constexpr size_t UGameInstance_LocalPlayers = 0x038;

    // UPlayer (base de ULocalPlayer)
    constexpr size_t UPlayer_PlayerController = 0x030;

    // APlayerController
    constexpr size_t APlayerController_Player         = 0x330;
    constexpr size_t APlayerController_AcknowledgedPawn = 0x338;
    constexpr size_t APlayerController_PlayerCameraManager = 0x348;

    // AController
    constexpr size_t AController_PlayerState = 0x298;
    constexpr size_t AController_Pawn        = 0x2D0;

    // APawn
    constexpr size_t APawn_Controller = 0x290; // peut varier, non utilisé

    // ACharacter
    constexpr size_t ACharacter_Mesh              = 0x318;
    constexpr size_t ACharacter_CharacterMovement = 0x320;
    constexpr size_t ACharacter_CapsuleComponent  = 0x328;

    // AActor
    constexpr size_t AActor_RootComponent = 0x198;
    constexpr size_t AActor_NamePrivate   = 0x0148; // NetDriverName ? En fait FName à UObject+0x20

    // USceneComponent
    constexpr size_t USceneComponent_RelativeLocation = 0x128;
    constexpr size_t USceneComponent_RelativeRotation = 0x140;

    // UCharacterMovementComponent
    constexpr size_t UCharacterMovement_GravityScale      = 0x170;
    constexpr size_t UCharacterMovement_JumpZVelocity      = 0x178;
    constexpr size_t UCharacterMovement_MaxWalkSpeed       = 0x1E8;
    constexpr size_t UCharacterMovement_MaxWalkSpeedCrouched=0x1EC;
    constexpr size_t UCharacterMovement_MaxSwimSpeed       = 0x1F0;
    constexpr size_t UCharacterMovement_MaxFlySpeed        = 0x1F4;
    constexpr size_t UCharacterMovement_MaxCustomMovementSpeed=0x1F8;
    constexpr size_t UCharacterMovement_MaxAcceleration    = 0x1FC;

    // APalPlayerState
    constexpr size_t APalPlayerState_InventoryData = 0x548;

    // UPalPlayerInventoryData
    constexpr size_t UPalPlayerInventoryData_NowItemWeight   = 0x160;
    constexpr size_t UPalPlayerInventoryData_MaxInventoryWeight=0x164;

    // UPalCharacterParameterComponent
    constexpr size_t UPalCharacterParameter_IndividualParameter = 0x108;
    constexpr size_t UPalCharacterParameter_SP                  = 0x2F0;
    constexpr size_t UPalCharacterParameter_IsImmortality       = 0x538;

    // UPalIndividualCharacterParameter
    constexpr size_t UPalIndividual_SaveParameter = 0x2A0; // FPalIndividualCharacterSaveParameter
    constexpr size_t UPalIndividual_HP            = 0x2A0 + 0x078;
    constexpr size_t UPalIndividual_MP            = 0x2A0 + 0x0B0;
    constexpr size_t UPalIndividual_MaxHP         = 0x2A0 + 0x0E8;
    constexpr size_t UPalIndividual_ShieldHP      = 0x2A0 + 0x108;
    constexpr size_t UPalIndividual_ShieldMaxHP   = 0x2A0 + 0x110;
    constexpr size_t UPalIndividual_MaxMP         = 0x2A0 + 0x118;
    constexpr size_t UPalIndividual_MaxSP         = 0x2A0 + 0x120;

    // Layout de FVector : X,Y,Z en double (UE5 ?) ou floats ? Le dump indique une taille FVector de 0x18. En UE4.27 FVector est 3 floats (0xC). En UE5 cela peut être du double (0x18). Palworld 1.0 utilise UE5 ? La taille 0x18 du dump suggère des doubles. Les coordonnées de la carte d'ARXII sont des floats ? Vérifions le moteur. Engine.hpp FVector taille 0x18 -> double.
}

namespace PalTrainerRuntime {
    extern uintptr_t GWorldRva;
    extern uintptr_t GObjectRva;
    extern uintptr_t FNameRva;
    extern uintptr_t ProcessEventRva;
    extern uintptr_t AppendStringRva;
    extern uintptr_t TickRva;

    // Charger runtime_offsets.json (clé/valeur plates avec chaînes hexadécimales) pour remplacer les offsets statiques.
    // Retourne true si au moins GWorld a été trouvé.
    bool Load(const char* jsonPath);
}

#endif
