#include "engine.hpp"

#include "offsets.h"

#include <windows.h>
#include <cstdint>
#include <cstring>

namespace Engine
{
    static uintptr_t g_Base = 0;
    static SDK::TUObjectArray* g_GObjects = nullptr;
    static void(*g_ProcessEvent)(SDK::UObject*, SDK::UFunction*, void*) = nullptr;

    static uintptr_t GetModuleBase()
    {
        return (uintptr_t)GetModuleHandleA(nullptr);
    }

    static uintptr_t GetAppendStringRva()
    {
        if (PalTrainerRuntime::AppendStringRva)
            return PalTrainerRuntime::AppendStringRva;
        return PalTrainerOffsets::AppendStringRva;
    }

    bool IsValidPointer(uintptr_t ptr);

    bool Initialize()
    {
        g_Base = GetModuleBase();
        if (!g_Base) return false;

        // GObjectRva est l'adresse d'une variable pointeur vers TUObjectArray dans ce binaire.
        uintptr_t goPtr = g_Base + PalTrainerRuntime::GObjectRva;
        if (IsValidPointer(goPtr))
        {
            SDK::TUObjectArray** p = (SDK::TUObjectArray**)goPtr;
            if (IsValidPointer((uintptr_t)p) && *p)
                g_GObjects = *p;
        }

        // Fallback : adresse de tableau directe (anciens layouts)
        if (!g_GObjects)
            g_GObjects = (SDK::TUObjectArray*)goPtr;

        if (g_GObjects && (g_GObjects->NumElements <= 0 || g_GObjects->NumElements > 0x1000000))
        {
            SDK::TUObjectArray* alt = (SDK::TUObjectArray*)(goPtr + 0x10);
            if (alt->NumElements > 0 && alt->NumElements <= 0x1000000)
                g_GObjects = alt;
            else
                g_GObjects = nullptr;
        }

        if (!g_GObjects) return false;

        g_ProcessEvent = (decltype(g_ProcessEvent))(g_Base + PalTrainerRuntime::ProcessEventRva);
        return true;
    }

    uintptr_t GetBaseAddress()
    {
        return g_Base;
    }

    SDK::TUObjectArray* GetGObjects()
    {
        return g_GObjects;
    }

    bool IsValidPointer(uintptr_t ptr)
    {
        if (!ptr) return false;
        MEMORY_BASIC_INFORMATION mbi{};
        if (!VirtualQuery((LPCVOID)ptr, &mbi, sizeof(mbi))) return false;
        return (mbi.State == MEM_COMMIT) && (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE));
    }

    std::wstring FNameToString(const SDK::FName& name)
    {
        using AppendStringFn = void(*)(const SDK::FName*, SDK::FString*);
        static AppendStringFn append = nullptr;
        if (!append && g_Base)
        {
            uintptr_t rva = GetAppendStringRva();
            if (rva) append = (AppendStringFn)(g_Base + rva);
        }

        SDK::FString out{};
        if (append)
            append(&name, &out);

        return out.ToWString();
    }

    static bool WStrEquals(const std::wstring& a, const wchar_t* b)
    {
        if (a.empty()) return false;
        return a == b;
    }

    bool NameEquals(SDK::UObject* obj, const wchar_t* name)
    {
        if (!obj) return false;
        return WStrEquals(FNameToString(obj->NamePrivate), name);
    }

    bool NameEquals(SDK::UClass* cls, const wchar_t* name)
    {
        if (!cls) return false;
        return WStrEquals(FNameToString(cls->NamePrivate), name);
    }

    bool IsAByName(SDK::UClass* cls, const wchar_t* baseName)
    {
        while (cls)
        {
            if (NameEquals(cls, baseName)) return true;
            cls = cls->GetSuperStruct();
        }
        return false;
    }

    SDK::UClass* FindClass(const wchar_t* className)
    {
        SDK::TUObjectArray* arr = g_GObjects;
        if (!arr) return nullptr;

        int32 perChunk = arr->MaxChunks > 0 ? arr->MaxElements / arr->MaxChunks : arr->MaxElements;
        if (perChunk <= 0) perChunk = arr->MaxElements;

        for (int32 c = 0; c < arr->NumChunks; ++c)
        {
            SDK::FUObjectItem* chunk = arr->Objects[c];
            if (!chunk) continue;

            for (int32 i = 0; i < perChunk && (c * perChunk + i) < arr->NumElements; ++i)
            {
                SDK::UObject* obj = chunk[i].Object;
                if (!obj) continue;
                if (!IsAByName(obj->ClassPrivate, L"Class")) continue;
                if (NameEquals(obj->ClassPrivate, className))
                    return (SDK::UClass*)obj;
            }
        }
        return nullptr;
    }

    SDK::UObject* FindFirstObjectOfClass(SDK::UClass* cls)
    {
        if (!cls) return nullptr;
        SDK::TUObjectArray* arr = g_GObjects;
        if (!arr) return nullptr;

        int32 perChunk = arr->MaxChunks > 0 ? arr->MaxElements / arr->MaxChunks : arr->MaxElements;
        if (perChunk <= 0) perChunk = arr->MaxElements;

        for (int32 c = 0; c < arr->NumChunks; ++c)
        {
            SDK::FUObjectItem* chunk = arr->Objects[c];
            if (!chunk) continue;

            for (int32 i = 0; i < perChunk && (c * perChunk + i) < arr->NumElements; ++i)
            {
                SDK::UObject* obj = chunk[i].Object;
                if (!obj) continue;
                if (obj->ClassPrivate == cls)
                    return obj;
            }
        }
        return nullptr;
    }

    SDK::UObject* FindCDO(const wchar_t* className)
    {
        SDK::UClass* cls = FindClass(className);
        if (!cls) return nullptr;
        return cls->GetDefaultObject();
    }

    SDK::UObject* FindObjectByName(const wchar_t* objectName)
    {
        SDK::TUObjectArray* arr = g_GObjects;
        if (!arr) return nullptr;

        int32 perChunk = arr->MaxChunks > 0 ? arr->MaxElements / arr->MaxChunks : arr->MaxElements;
        if (perChunk <= 0) perChunk = arr->MaxElements;

        for (int32 c = 0; c < arr->NumChunks; ++c)
        {
            SDK::FUObjectItem* chunk = arr->Objects[c];
            if (!chunk) continue;

            for (int32 i = 0; i < perChunk && (c * perChunk + i) < arr->NumElements; ++i)
            {
                SDK::UObject* obj = chunk[i].Object;
                if (!obj) continue;
                if (NameEquals(obj, objectName))
                    return obj;
            }
        }
        return nullptr;
    }

    SDK::UFunction* FindFunction(const wchar_t* functionName)
    {
        SDK::TUObjectArray* arr = g_GObjects;
        if (!arr) return nullptr;

        int32 perChunk = arr->MaxChunks > 0 ? arr->MaxElements / arr->MaxChunks : arr->MaxElements;
        if (perChunk <= 0) perChunk = arr->MaxElements;

        for (int32 c = 0; c < arr->NumChunks; ++c)
        {
            SDK::FUObjectItem* chunk = arr->Objects[c];
            if (!chunk) continue;

            for (int32 i = 0; i < perChunk && (c * perChunk + i) < arr->NumElements; ++i)
            {
                SDK::UObject* obj = chunk[i].Object;
                if (!obj) continue;
                if (!IsAByName(obj->ClassPrivate, L"Function")) continue;
                if (NameEquals(obj, functionName))
                    return (SDK::UFunction*)obj;
            }
        }
        return nullptr;
    }

    bool ProcessEvent(SDK::UObject* obj, SDK::UFunction* func, void* params)
    {
        if (!obj || !func || !g_ProcessEvent) return false;
        g_ProcessEvent(obj, func, params);
        return true;
    }
}
