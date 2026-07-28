// PalTrainerLauncher.exe
// Launcher centralisé pour PalTrainerUltra — détection, injection, lancement des composants.
// ImGui + DirectX 11, style WeMod/FLiNG.

#include <windows.h>
#include <shellapi.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <string>
#include <vector>
#include <cstdint>
#include <cstdio>
#include <cmath>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
#include <tlhelp32.h>
#include <psapi.h>

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/backends/imgui_impl_dx11.h"
#include "../trainer/logger.hpp"

// ----------------------------------------------------------------------------
// D3D11 globals
// ----------------------------------------------------------------------------
static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;
static HWND                     g_hwnd = nullptr;

// ----------------------------------------------------------------------------
// État du launcher
// ----------------------------------------------------------------------------
static std::atomic<bool>  g_gameFound{false};
static std::atomic<bool>  g_injected{false};
static std::atomic<bool>  g_attachInProgress{false};
static bool                g_autoInject = false;
static std::atomic<DWORD> g_gamePid{0};
static std::string        g_statusMsg = "En attente de Palworld...";
static std::mutex         g_statusMutex;
static std::thread        g_pollThread;
static std::thread        g_attachThread;
static std::atomic<bool>  g_running{true};
static std::string        g_dataDir;

enum class AppState {
    Waiting,
    GameFound,
    Injecting,
    Injected,
    Error
};
static std::atomic<AppState> g_appState{AppState::Waiting};

// ----------------------------------------------------------------------------
// Helpers
// ----------------------------------------------------------------------------
static std::wstring StringToWString(const std::string& s) {
    if (s.empty()) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring ws(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &ws[0], len);
    if (len > 0) ws.resize(len - 1);
    return ws;
}

static std::string WStringToString(const std::wstring& ws) {
    if (ws.empty()) return "";
    int len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string s(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, &s[0], len, nullptr, nullptr);
    if (len > 0) s.resize(len - 1);
    return s;
}

static void SetStatus(const std::string& msg) {
    std::lock_guard<std::mutex> lock(g_statusMutex);
    g_statusMsg = msg;
}

static std::string GetStatus() {
    std::lock_guard<std::mutex> lock(g_statusMutex);
    return g_statusMsg;
}

static std::string GetExeDir() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    std::wstring wpath(path);
    size_t pos = wpath.find_last_of(L"\\/");
    if (pos != std::wstring::npos) wpath = wpath.substr(0, pos);
    return WStringToString(wpath);
}

// ----------------------------------------------------------------------------
// Process detection
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

// ----------------------------------------------------------------------------
// Offset scanner check
// ----------------------------------------------------------------------------
static bool NeedRescanOffsets(const std::wstring& gamePath) {
    std::string offsetsPath = g_dataDir + "\\runtime_offsets.json";
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
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = {};
    if (!CreateProcessW(scanner.c_str(), &args[0], nullptr, nullptr, FALSE, 0, nullptr,
                        StringToWString(g_dataDir).c_str(), &si, &pi)) {
        return false;
    }
    WaitForSingleObject(pi.hProcess, 60000);
    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return exitCode == 0;
}

// ----------------------------------------------------------------------------
// DLL injection
// ----------------------------------------------------------------------------
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

// ----------------------------------------------------------------------------
// Launch external process
// ----------------------------------------------------------------------------
static bool LaunchExe(const std::string& exeName, bool wait = false) {
    std::wstring path = StringToWString(g_dataDir) + L"\\" + StringToWString(exeName);
    if (GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES) {
        SetStatus("Fichier introuvable: " + exeName);
        return false;
    }
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi = {};
    std::wstring args = L"\"" + path + L"\"";
    if (!CreateProcessW(path.c_str(), &args[0], nullptr, nullptr, FALSE, 0, nullptr,
                        StringToWString(g_dataDir).c_str(), &si, &pi)) {
        SetStatus("Échec du lancement: " + exeName);
        return false;
    }
    if (wait) WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return true;
}

static bool LaunchBat(const std::string& batName) {
    std::wstring path = StringToWString(g_dataDir) + L"\\" + StringToWString(batName);
    if (GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES) {
        SetStatus("Fichier introuvable: " + batName);
        return false;
    }
    HINSTANCE h = ShellExecuteW(g_hwnd, L"open", path.c_str(), nullptr,
                                StringToWString(g_dataDir).c_str(), SW_SHOWNORMAL);
    if ((INT_PTR)h <= 32) {
        SetStatus("Échec du lancement: " + batName);
        return false;
    }
    return true;
}

static bool LaunchPython(const std::string& scriptName) {
    std::wstring script = StringToWString(g_dataDir) + L"\\scripts\\" + StringToWString(scriptName);
    if (GetFileAttributesW(script.c_str()) == INVALID_FILE_ATTRIBUTES) {
        SetStatus("Script introuvable: " + scriptName);
        return false;
    }
    std::wstring cmd = L"python \"" + script + L"\"";
    HINSTANCE h = ShellExecuteW(g_hwnd, L"open", L"python", (LPWSTR)cmd.c_str(),
                                StringToWString(g_dataDir).c_str(), SW_SHOWNORMAL);
    if ((INT_PTR)h <= 32) {
        SetStatus("Python non trouvé. Installe Python 3.");
        return false;
    }
    return true;
}

// ----------------------------------------------------------------------------
// Attach worker (detect + scan + inject + launch overlay)
// ----------------------------------------------------------------------------
static void AttachWorker() {
    g_attachInProgress = true;
    g_appState = AppState::Injecting;
    SetStatus("Recherche de Palworld...");
    Log("AttachWorker: searching for Palworld");

    DWORD pid = FindProcessId(L"Palworld-Win64-Shipping.exe");
    if (!pid) {
        SetStatus("Palworld n'est pas lancé. Lance le jeu puis clique sur JOUER.");
        Log("AttachWorker: Palworld not found");
        g_appState = AppState::Error;
        g_attachInProgress = false;
        return;
    }
    g_gamePid = pid;
    Log("AttachWorker: Palworld found pid=%lu", pid);

    SetStatus("Palworld détecté. Résolution du chemin...");
    std::wstring gamePath;
    if (!GetProcessImagePath(pid, gamePath)) {
        SetStatus("Échec de la récupération du chemin de Palworld.");
        Log("AttachWorker: failed to get game path");
        g_appState = AppState::Error;
        g_attachInProgress = false;
        return;
    }
    Log("AttachWorker: game path resolved");

    if (NeedRescanOffsets(gamePath)) {
        SetStatus("Scan des offsets en cours (peut prendre 30-60s)...");
        Log("AttachWorker: starting offset scan");
        if (!RunScanner(gamePath)) {
            SetStatus("Échec du scanner d'offsets. Lance PalTrainerLauncher en tant qu'administrateur.");
            Log("AttachWorker: offset scan FAILED");
            g_appState = AppState::Error;
            g_attachInProgress = false;
            return;
        }
        Log("AttachWorker: offset scan complete");
    }

    SetStatus("Injection de PalTrainerCore.dll...");
    std::wstring dllPath = StringToWString(g_dataDir) + L"\\PalTrainerCore.dll";
    if (GetFileAttributesW(dllPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        SetStatus("PalTrainerCore.dll introuvable dans " + g_dataDir);
        Log("AttachWorker: PalTrainerCore.dll not found in %s", g_dataDir.c_str());
        g_appState = AppState::Error;
        g_attachInProgress = false;
        return;
    }
    if (!InjectDll(pid, dllPath)) {
        SetStatus("Échec de l'injection. Lance PalTrainerLauncher en tant qu'administrateur.");
        Log("AttachWorker: DLL injection FAILED for pid=%lu", pid);
        g_appState = AppState::Error;
        g_attachInProgress = false;
        return;
    }
    Log("AttachWorker: injection successful");

    SetStatus("Injection réussie ! Lancement de l'overlay...");
    g_injected = true;
    g_appState = AppState::Injected;

    LaunchExe("PalTrainerOverlay.exe");
    Log("AttachWorker: overlay launched");

    SetStatus("Overlay lancé. Tu peux fermer le launcher ou utiliser les outils ci-dessous.");
    g_attachInProgress = false;
}

static void StartAttach() {
    if (g_attachInProgress || g_injected) return;
    if (g_attachThread.joinable()) g_attachThread.join();
    g_attachThread = std::thread(AttachWorker);
}

// ----------------------------------------------------------------------------
// Game polling thread
// ----------------------------------------------------------------------------
static void PollWorker() {
    Log("PollWorker: started");
    while (g_running) {
        DWORD pid = FindProcessId(L"Palworld-Win64-Shipping.exe");
        if (pid && !g_gameFound) {
            g_gameFound = true;
            g_gamePid = pid;
            Log("PollWorker: Palworld detected pid=%lu", pid);
            if (g_appState == AppState::Waiting) {
                g_appState = AppState::GameFound;
                SetStatus("Palworld détecté ! Clique sur JOUER pour démarrer.");
                if (g_autoInject) {
                    StartAttach();
                }
            }
        } else if (!pid && g_gameFound && !g_injected) {
            g_gameFound = false;
            g_gamePid = 0;
            Log("PollWorker: Palworld closed");
            if (g_appState == AppState::GameFound) {
                g_appState = AppState::Waiting;
                SetStatus("Palworld a été fermé. En attente...");
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    Log("PollWorker: stopped");
}

// ----------------------------------------------------------------------------
// D3D11 setup
// ----------------------------------------------------------------------------
bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
        featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice,
        &featureLevel, &g_pd3dDeviceContext) != S_OK)
        return false;

    ID3D11Texture2D* pBackBuffer = nullptr;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    if (pBackBuffer) {
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
        pBackBuffer->Release();
    }
    return true;
}

void CleanupDeviceD3D() {
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;
    if (msg == WM_DESTROY) { ::PostQuitMessage(0); return 0; }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

// ----------------------------------------------------------------------------
// UI Rendering
// ----------------------------------------------------------------------------
static void StyleDark() {
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = 8.0f;
    s.FrameRounding = 6.0f;
    s.GrabRounding = 4.0f;
    s.WindowPadding = ImVec2(16, 16);
    s.FramePadding = ImVec2(10, 6);
    s.ItemSpacing = ImVec2(10, 8);

    ImVec4* c = s.Colors;
    c[ImGuiCol_WindowBg]       = ImVec4(0.06f, 0.06f, 0.08f, 1.0f);
    c[ImGuiCol_ChildBg]        = ImVec4(0.08f, 0.08f, 0.10f, 1.0f);
    c[ImGuiCol_Border]         = ImVec4(0.15f, 0.15f, 0.18f, 1.0f);
    c[ImGuiCol_FrameBg]        = ImVec4(0.12f, 0.12f, 0.15f, 1.0f);
    c[ImGuiCol_FrameBgHovered] = ImVec4(0.18f, 0.18f, 0.22f, 1.0f);
    c[ImGuiCol_FrameBgActive]  = ImVec4(0.22f, 0.22f, 0.28f, 1.0f);
    c[ImGuiCol_Button]         = ImVec4(0.20f, 0.22f, 0.35f, 1.0f);
    c[ImGuiCol_ButtonHovered]  = ImVec4(0.30f, 0.32f, 0.50f, 1.0f);
    c[ImGuiCol_ButtonActive]   = ImVec4(0.40f, 0.42f, 0.60f, 1.0f);
    c[ImGuiCol_Text]           = ImVec4(0.90f, 0.90f, 0.95f, 1.0f);
    c[ImGuiCol_TextDisabled]   = ImVec4(0.45f, 0.45f, 0.50f, 1.0f);
    c[ImGuiCol_CheckMark]      = ImVec4(0.30f, 0.60f, 1.0f, 1.0f);
    c[ImGuiCol_Separator]      = ImVec4(0.15f, 0.15f, 0.18f, 1.0f);
}

static ImU32 GetStatusColor(AppState state) {
    switch (state) {
        case AppState::Waiting:    return IM_COL32(180, 140, 0, 255);
        case AppState::GameFound:  return IM_COL32(60, 200, 60, 255);
        case AppState::Injecting:  return IM_COL32(60, 140, 255, 255);
        case AppState::Injected:   return IM_COL32(60, 220, 60, 255);
        case AppState::Error:      return IM_COL32(220, 60, 60, 255);
    }
    return IM_COL32(200, 200, 200, 255);
}

static void RenderLauncher() {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(600, 400));
    ImGui::Begin("##launcher", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBringToFrontOnFocus);

    // ---- En-tête ----
    {
        ImGui::Dummy(ImVec2(0, 4));
        ImGui::PushFont(nullptr);
        ImGui::SetWindowFontScale(1.6f);
        ImGui::TextColored(ImVec4(0.4f, 0.6f, 1.0f, 1.0f), "PalTrainerUltra");
        ImGui::SetWindowFontScale(1.0f);
        ImGui::PopFont();
        ImGui::SameLine();
        ImGui::TextDisabled("v1.0 — Palworld 1.0");
        ImGui::Separator();
    }

    // ---- Statut ----
    {
        AppState state = g_appState;
        ImU32 color = GetStatusColor(state);
        float r = ((color >> IM_COL32_R_SHIFT) & 0xFF) / 255.0f;
        float g = ((color >> IM_COL32_G_SHIFT) & 0xFF) / 255.0f;
        float b = ((color >> IM_COL32_B_SHIFT) & 0xFF) / 255.0f;

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(r, g, b, 1.0f), "[*] Statut:");
        ImGui::SameLine();
        // Indicator dot
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 dotPos = ImGui::GetCursorScreenPos();
        dotPos.x += 4; dotPos.y += 6;
        dl->AddCircleFilled(dotPos, 5.0f, color);
        ImGui::Dummy(ImVec2(16, 0));
        ImGui::SameLine();
        ImGui::TextWrapped("%s", GetStatus().c_str());
        ImGui::Spacing();
    }

    // ---- Bouton JOUER ----
    {
        ImGui::Spacing();
        ImGui::Spacing();
        AppState state = g_appState;
        bool canPlay = (state == AppState::GameFound || state == AppState::Waiting) &&
                       !g_attachInProgress && !g_injected;

        if (g_attachInProgress) {
            ImGui::BeginDisabled();
            ImGui::Button("TRAITEMENT...", ImVec2(-1, 50));
            ImGui::EndDisabled();
        } else if (g_injected) {
            ImGui::BeginDisabled();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.35f, 0.15f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.15f, 0.35f, 0.15f, 1.0f));
            ImGui::Button("INJECTÉ — Overlay en cours", ImVec2(-1, 50));
            ImGui::PopStyleColor(2);
            ImGui::EndDisabled();
        } else if (g_gameFound) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.45f, 0.15f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.55f, 0.20f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.25f, 0.65f, 0.25f, 1.0f));
            ImGui::SetWindowFontScale(1.3f);
            if (ImGui::Button("JOUER", ImVec2(-1, 50))) {
                StartAttach();
            }
            ImGui::SetWindowFontScale(1.0f);
            ImGui::PopStyleColor(3);
        } else {
            ImGui::BeginDisabled();
            ImGui::Button("EN ATTENTE DE PALWORLD...", ImVec2(-1, 50));
            ImGui::EndDisabled();
        }

        ImGui::Spacing();
        ImGui::Checkbox("Auto-injecter quand Palworld démarre", &g_autoInject);
    }

    ImGui::Separator();
    ImGui::Spacing();

    // ---- Panneau outils ----
    {
        ImGui::TextDisabled("Outils:");
        ImGui::Spacing();

        float halfW = (600 - 48) * 0.5f - 4;
        float tagW = 50.0f;
        float btnW = halfW - tagW - 6;

        // Helper lambda for tags
        auto Tag = [](const char* label, bool inj) {
            ImGui::TextColored(inj ? ImVec4(0.9f, 0.3f, 0.3f, 1.0f) : ImVec4(0.3f, 0.8f, 0.3f, 1.0f), "[%s]", label);
            ImGui::SameLine();
        };

        Tag("INJ", true);
        if (ImGui::Button("Lancer Overlay", ImVec2(btnW, 32))) {
            LaunchExe("PalTrainerOverlay.exe");
        }
        ImGui::SameLine();
        Tag("SOLO", false);
        if (ImGui::Button("Lancer Minimap", ImVec2(btnW, 32))) {
            LaunchExe("PalTrainerMiniMap.exe");
        }

        Tag("SOLO", false);
        if (ImGui::Button("Lancer Carte Web", ImVec2(btnW, 32))) {
            LaunchBat("start_map.bat");
        }
        ImGui::SameLine();
        Tag("SOLO", false);
        if (ImGui::Button("Scanner d'Offsets", ImVec2(btnW, 32))) {
            LaunchExe("PalOffsetScanner.exe");
        }

        Tag("SOLO", false);
        if (ImGui::Button("Calculateur Reproduction", ImVec2(btnW, 32))) {
            LaunchPython("breeding_calculator.py");
        }
        ImGui::SameLine();
        Tag("SOLO", false);
        if (ImGui::Button("Éditeur Sauvegarde", ImVec2(btnW, 32))) {
            LaunchPython("save_editor.py");
        }
    }

    // ---- Bas: dossier + quitter ----
    {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::TextDisabled("Dossier: %s", g_dataDir.c_str());
        ImGui::Spacing();
        ImGui::SameLine(600 - 120);
        if (ImGui::Button("Quitter", ImVec2(80, 28))) {
            g_running = false;
            ::PostQuitMessage(0);
        }
    }

    ImGui::End();
}

// ----------------------------------------------------------------------------
// WinMain
// ----------------------------------------------------------------------------
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int) {
    g_dataDir = GetExeDir();
    LogInit(g_dataDir.c_str());
    Log("=== PalTrainerLauncher v1.0 starting ===");
    Log("Data dir: %s", g_dataDir.c_str());

    // Fenêtre
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, hInstance,
                       nullptr, nullptr, nullptr, nullptr, L"PalTrainerLauncher", nullptr };
    wc.hIcon = LoadIconW(hInstance, L"APP_ICON");
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowExW(0, wc.lpszClassName, L"PalTrainerUltra — Launcher",
                                  WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                                  CW_USEDEFAULT, CW_USEDEFAULT, 600, 400, nullptr, nullptr, wc.hInstance, nullptr);
    g_hwnd = hwnd;

    if (!CreateDeviceD3D(hwnd)) {
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    StyleDark();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Démarrer le thread de polling
    g_pollThread = std::thread(PollWorker);

    // Vérification initiale
    DWORD pid = FindProcessId(L"Palworld-Win64-Shipping.exe");
    if (pid) {
        g_gameFound = true;
        g_gamePid = pid;
        g_appState = AppState::GameFound;
        SetStatus("Palworld détecté ! Clique sur JOUER pour démarrer.");
    }

    // Boucle principale
    MSG msg;
    while (g_running) {
        while (::PeekMessageW(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessageW(&msg);
            if (msg.message == WM_QUIT) { g_running = false; }
        }
        if (!g_running) break;

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        RenderLauncher();

        ImGui::Render();
        const float clearColor[4] = { 0.06f, 0.06f, 0.08f, 1.0f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clearColor);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0);
        Sleep(16);
    }

    // Nettoyage
    g_running = false;
    if (g_pollThread.joinable()) g_pollThread.join();
    if (g_attachThread.joinable()) g_attachThread.join();

    Log("Launcher shutting down cleanly");
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
    LogClose();
    return 0;
}
