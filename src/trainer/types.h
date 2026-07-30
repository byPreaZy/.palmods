#ifndef TYPES_H
#define TYPES_H

#include <cstdint>

// Définitions de types minimales de style UE5 dérivées du CXXHeaderDump public pour Palworld 1.0.1
// Le jeu utilise des FVector/FRotator en double précision (taille 0x18) dans cette version.

struct FVector {
    double X;
    double Y;
    double Z;
};

struct FRotator {
    double Pitch;
    double Yaw;
    double Roll;
};

struct FVector2D {
    double X;
    double Y;
};

struct FName {
    uint32_t Index;
    uint32_t Number;
};

// FString est un TArray<TCHAR> : pointeur de données, compte, capacité
struct FString {
    wchar_t* Data;
    int32_t Count;
    int32_t Capacity;
};

template <typename T>
struct TArray {
    T* Data;
    int32_t Count;
    int32_t Capacity;
};

struct FFixedPoint64 {
    int64_t Value;
};

// Base minimale de UObject : l'en-tête est opaque ; nous n'utilisons que les offsets.
struct UObject {
    void* vtable;
    uint32_t ObjectFlags;
    uint32_t InternalIndex;
    class UClass* Class;
    UObject* Outer;
    FName Name;
};

// Les noms sont recherchés via le pool FName. Nous gardons les signatures des helpers minimales.
using UObjectPtr = void*;

static inline void* fieldPtr(void* obj, size_t offset) {
    return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(obj) + offset);
}

template <typename T>
static inline T read(void* obj, size_t offset) {
    return *reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(obj) + offset);
}

template <typename T>
static inline void write(void* obj, size_t offset, T value) {
    *reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(obj) + offset) = value;
}

static inline void* deref(void* obj, size_t offset) {
    return *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(obj) + offset);
}

#endif
