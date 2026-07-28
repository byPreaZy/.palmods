#pragma once

#include "sdk.hpp"
#include <string>

namespace Engine
{
    // Base du module + globals
    bool Initialize();
    uintptr_t GetBaseAddress();
    SDK::TUObjectArray* GetGObjects();

    // Helpers de noms
    std::wstring FNameToString(const SDK::FName& name);
    bool NameEquals(SDK::UObject* obj, const wchar_t* name);
    bool NameEquals(SDK::UClass* cls, const wchar_t* name);

    // Vérification de la hiérarchie des classes par nom
    bool IsAByName(SDK::UClass* cls, const wchar_t* baseName);

    // Recherches d'objets
    SDK::UClass*    FindClass(const wchar_t* className);
    SDK::UObject*   FindFirstObjectOfClass(SDK::UClass* cls);
    SDK::UObject*   FindCDO(const wchar_t* className);
    SDK::UObject*   FindObjectByName(const wchar_t* objectName);
    SDK::UFunction* FindFunction(const wchar_t* functionName);

    // Appels ProcessEvent / fonctions natives
    bool ProcessEvent(SDK::UObject* obj, SDK::UFunction* func, void* params);

    // Helpers mémoire sécurisés
    bool IsValidPointer(uintptr_t ptr);
}
