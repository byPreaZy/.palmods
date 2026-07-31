// overlay_minimap.inl - Mini-carte overlay zoomable/filtrable + lancement du serveur web

static bool IsJsonSpace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static void JsonSkipSpace(const std::string& s, size_t& i) {
    while (i < s.size() && IsJsonSpace(s[i])) ++i;
}

static std::string JsonParseString(const std::string& s, size_t& i) {
    std::string out;
    if (s[i] == '"') ++i;
    while (i < s.size()) {
        char c = s[i++];
        if (c == '"') break;
        if (c == '\\' && i < s.size()) out.push_back(s[i++]);
        else out.push_back(c);
    }
    return out;
}

static double JsonParseNumber(const std::string& s, size_t& i) {
    size_t start = i;
    if (s[i] == '-' || s[i] == '+') ++i;
    while (i < s.size() && (std::isdigit((unsigned char)s[i]) || s[i] == '.' || s[i] == 'e' || s[i] == 'E' || s[i] == '+' || s[i] == '-')) ++i;
    try { return std::stod(s.substr(start, i - start)); } catch (...) { return 0.0; }
}

static void JsonSkipValue(const std::string& s, size_t& i) {
    JsonSkipSpace(s, i);
    if (i >= s.size()) return;
    if (s[i] == '"') { JsonParseString(s, i); return; }
    if (s[i] == '{') {
        int depth = 1;
        ++i;
        while (i < s.size() && depth > 0) {
            if (s[i] == '"') { JsonParseString(s, i); continue; }
            if (s[i] == '{') ++depth;
            else if (s[i] == '}') --depth;
            ++i;
        }
        return;
    }
    if (s[i] == '[') {
        int depth = 1;
        ++i;
        while (i < s.size() && depth > 0) {
            if (s[i] == '"') { JsonParseString(s, i); continue; }
            if (s[i] == '[') ++depth;
            else if (s[i] == ']') --depth;
            ++i;
        }
        return;
    }
    while (i < s.size() && s[i] != ',' && s[i] != ']' && s[i] != '}') ++i;
}

static bool LoadMapObjects(const char* path, std::vector<Poi>& out) {
    std::string text = ReadFileTextA(path);
    if (text.empty()) return false;
    size_t i = 0;
    JsonSkipSpace(text, i);
    if (i >= text.size() || text[i] != '[') return false;
    ++i;
    while (i < text.size()) {
        JsonSkipSpace(text, i);
        if (i < text.size() && text[i] == ']') break;
        if (text[i] != '{') { JsonSkipValue(text, i); continue; }
        ++i;
        Poi poi;
        while (i < text.size()) {
            JsonSkipSpace(text, i);
            if (i < text.size() && text[i] == '}') { ++i; break; }
            if (text[i] != '"') { JsonSkipValue(text, i); continue; }
            std::string key = JsonParseString(text, i);
            JsonSkipSpace(text, i);
            if (i < text.size() && text[i] == ':') ++i;
            JsonSkipSpace(text, i);
            if (key == "id" && text[i] == '"') poi.id = JsonParseString(text, i);
            else if (key == "label" && text[i] == '"') poi.label = JsonParseString(text, i);
            else if (key == "type" && text[i] == '"') poi.type = JsonParseString(text, i);
            else if (key == "location" && text[i] == '{') {
                ++i;
                while (i < text.size()) {
                    JsonSkipSpace(text, i);
                    if (i < text.size() && text[i] == '}') { ++i; break; }
                    if (text[i] != '"') { JsonSkipValue(text, i); continue; }
                    std::string locKey = JsonParseString(text, i);
                    JsonSkipSpace(text, i);
                    if (i < text.size() && text[i] == ':') ++i;
                    JsonSkipSpace(text, i);
                    double val = JsonParseNumber(text, i);
                    if (locKey == "X") poi.x = (float)val;
                    else if (locKey == "Y") poi.y = (float)val;
                    else if (locKey == "Z") poi.z = (float)val;
                    else JsonSkipValue(text, i);
                    JsonSkipSpace(text, i);
                    if (i < text.size() && text[i] == ',') ++i;
                }
            } else JsonSkipValue(text, i);
            JsonSkipSpace(text, i);
            if (i < text.size() && text[i] == ',') ++i;
        }
        out.push_back(poi);
        JsonSkipSpace(text, i);
        if (i < text.size() && text[i] == ',') ++i;
    }
    return true;
}

static void InitializePoiFilters() {
    static const char* types[] = {
        "fastTravelPoint", "towerTravelPoint", "dungeon", "egg", "treasure",
        "strongEnemy", "alpha_pal", "predator_pal", "bounty", "enemyCamp", "oilrig"
    };
    for (auto t : types) {
        if (!g_poiFilter.count(t)) g_poiFilter[t] = true;
    }
}

static void LoadPrefs(); // forward declaration
static void LoadPalSpawns(); // forward declaration

static void LoadPois() {
    std::vector<std::string> candidates;
    candidates.push_back(g_dataDir + "/web/mapObjects.json");
    candidates.push_back(g_dataDir + "/overlay_assets/mapObjects.json");
    candidates.push_back(g_dataDir + "/../web/mapObjects.json");
    candidates.push_back(g_dataDir + "/mapObjects.json");
    for (const auto& p : candidates) {
        if (LoadMapObjects(p.c_str(), g_pois)) {
            InitializePoiFilters();
            LoadPrefs();
            LoadPalSpawns();
            return;
        }
    }
    InitializePoiFilters();
    LoadPrefs();
    LoadPalSpawns();
}

static ImU32 GetPoiColor(const std::string& type) {
    if (type == "fastTravelPoint") return IM_COL32(0, 170, 255, 255);
    if (type == "towerTravelPoint") return IM_COL32(255, 60, 60, 255);
    if (type == "dungeon") return IM_COL32(180, 60, 255, 255);
    if (type == "egg") return IM_COL32(255, 220, 60, 255);
    if (type == "treasure") return IM_COL32(255, 140, 0, 255);
    if (type == "strongEnemy") return IM_COL32(200, 30, 30, 255);
    if (type == "alpha_pal") return IM_COL32(220, 50, 50, 255);
    if (type == "predator_pal") return IM_COL32(180, 40, 40, 255);
    if (type == "bounty") return IM_COL32(50, 220, 50, 255);
    if (type == "enemyCamp") return IM_COL32(170, 120, 60, 255);
    if (type == "oilrig") return IM_COL32(160, 160, 160, 255);
    return IM_COL32(200, 200, 200, 255);
}

static const char* GetPoiLabelFr(const std::string& type) {
    if (type == "fastTravelPoint") return "Voyage rapide";
    if (type == "towerTravelPoint") return "Tour";
    if (type == "dungeon") return "Donjon";
    if (type == "egg") return "Œuf";
    if (type == "treasure") return "Trésor";
    if (type == "strongEnemy") return "Ennemi fort";
    if (type == "alpha_pal") return "Alpha Pal";
    if (type == "predator_pal") return "Pal prédateur";
    if (type == "bounty") return "Prime";
    if (type == "enemyCamp") return "Camp ennemi";
    if (type == "oilrig") return "Plateforme pétrolière";
    return type.c_str();
}

struct IconTexture {
    ID3D11ShaderResourceView* srv = nullptr;
    int width = 0;
    int height = 0;
};
static std::map<std::string, IconTexture> g_poiTextures;

static bool LoadTextureFromBMP(const char* path, int* outW, int* outH, ID3D11ShaderResourceView** out_srv) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;

    uint8_t header[54];
    if (!f.read((char*)header, 54)) return false;
    if (header[0] != 'B' || header[1] != 'M') return false;

    uint32_t pixelOffset = 0, compression = 0;
    int32_t width = 0, height = 0;
    uint16_t bpp = 0;
    std::memcpy(&pixelOffset, header + 10, 4);
    std::memcpy(&width, header + 18, 4);
    std::memcpy(&height, header + 22, 4);
    std::memcpy(&bpp, header + 28, 2);
    std::memcpy(&compression, header + 30, 4);

    if (compression != 0) return false;
    if (width <= 0 || height == 0 || (bpp != 24 && bpp != 32)) return false;

    bool topDown = height < 0;
    int w = (int)width;
    int h = (height < 0) ? -height : height;
    int rowBytes = ((bpp * w + 31) / 32) * 4;

    std::vector<uint8_t> src(rowBytes * h);
    f.seekg(pixelOffset, std::ios::beg);
    if (!f.read((char*)src.data(), src.size())) return false;

    std::vector<uint8_t> rgba((size_t)w * h * 4);
    for (int y = 0; y < h; ++y) {
        int srcY = topDown ? y : (h - 1 - y);
        const uint8_t* srcRow = src.data() + srcY * rowBytes;
        for (int x = 0; x < w; ++x) {
            size_t dst = ((size_t)y * w + x) * 4;
            if (bpp == 32) {
                int src = x * 4;
                rgba[dst + 0] = srcRow[src + 2];
                rgba[dst + 1] = srcRow[src + 1];
                rgba[dst + 2] = srcRow[src + 0];
                rgba[dst + 3] = srcRow[src + 3];
            } else {
                int src = x * 3;
                rgba[dst + 0] = srcRow[src + 2];
                rgba[dst + 1] = srcRow[src + 1];
                rgba[dst + 2] = srcRow[src + 0];
                rgba[dst + 3] = 255;
            }
        }
    }

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = w;
    desc.Height = h;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA sub = {};
    sub.pSysMem = rgba.data();
    sub.SysMemPitch = (UINT)(w * 4);

    ID3D11Texture2D* tex = nullptr;
    if (FAILED(g_pd3dDevice->CreateTexture2D(&desc, &sub, &tex)))
        return false;

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    HRESULT hr = g_pd3dDevice->CreateShaderResourceView(tex, &srvDesc, out_srv);
    tex->Release();
    if (SUCCEEDED(hr)) {
        *outW = w;
        *outH = h;
        return true;
    }
    return false;
}

static void LoadIconTextures() {
    static const char* types[] = {
        "fastTravelPoint", "towerTravelPoint", "dungeon", "egg", "treasure",
        "strongEnemy", "alpha_pal", "predator_pal", "bounty", "enemyCamp", "oilrig", "base"
    };
    for (const char* t : types) {
        if (g_poiTextures.count(t)) continue;
        std::vector<std::string> candidates;
        candidates.push_back(g_dataDir + "/overlay_assets/icons/" + t + ".bmp");
        candidates.push_back(g_dataDir + "/assets/icons/" + t + ".bmp");
        candidates.push_back(g_dataDir + "/../overlay_assets/icons/" + t + ".bmp");
        candidates.push_back(g_dataDir + "/../assets/icons/" + t + ".bmp");
        candidates.push_back(g_dataDir + "/../core/overlay_assets/icons/" + t + ".bmp");
        for (const auto& p : candidates) {
            IconTexture it;
            if (LoadTextureFromBMP(p.c_str(), &it.width, &it.height, &it.srv)) {
                g_poiTextures[t] = it;
                break;
            }
        }
    }
}

static void ReleaseIconTextures() {
    for (auto& kv : g_poiTextures) {
        if (kv.second.srv) { kv.second.srv->Release(); kv.second.srv = nullptr; }
    }
    g_poiTextures.clear();
}

// ----------------------------------------------------------------------------
// Lecture autonome de la position du joueur via ReadProcessMemory
// ----------------------------------------------------------------------------
static HANDLE g_memProcessHandle = nullptr;
static DWORD  g_memProcessPid = 0;
static uintptr_t g_memBaseAddr = 0;
static uintptr_t g_memGWorldRva = 0;
static bool g_memOffsetsLoaded = false;

static DWORD FindPalworldPid() {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(pe);
    DWORD pid = 0;
    if (Process32FirstW(snap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, L"Palworld-Win64-Shipping.exe") == 0) {
                pid = pe.th32ProcessID;
                break;
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return pid;
}

static uintptr_t GetModuleBaseRemote(HANDLE hProc) {
    HMODULE hMods[1024];
    DWORD cbNeeded = 0;
    if (EnumProcessModules(hProc, hMods, sizeof(hMods), &cbNeeded)) {
        DWORD count = cbNeeded / sizeof(HMODULE);
        for (DWORD i = 0; i < count; ++i) {
            wchar_t modName[MAX_PATH];
            if (GetModuleBaseNameW(hProc, hMods[i], modName, MAX_PATH)) {
                if (_wcsicmp(modName, L"Palworld-Win64-Shipping.exe") == 0) {
                    return (uintptr_t)hMods[i];
                }
            }
        }
    }
    return 0;
}

static bool LoadMemOffsets() {
    if (g_memOffsetsLoaded) return g_memGWorldRva != 0;
    g_memOffsetsLoaded = true;
    std::string offsetsPath = g_dataDir + "/runtime_offsets.json";
    std::string txt = ReadFileTextA(offsetsPath.c_str());
    if (txt.empty()) return false;
    auto pairs = ParseSimpleJson(txt);
    for (auto& kv : pairs) {
        if (kv.first == "GWorldRva") {
            try { g_memGWorldRva = std::stoull(kv.second, nullptr, 16); } catch (...) {}
        }
    }
    return g_memGWorldRva != 0;
}

static bool NeedRescanOffsetsMM() {
    std::string offsetsPath = g_dataDir + "/runtime_offsets.json";
    DWORD pid = FindPalworldPid();
    if (pid == 0) return false;
    HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (!hProc) return false;
    wchar_t gamePathW[MAX_PATH] = {};
    GetModuleFileNameExW(hProc, nullptr, gamePathW, MAX_PATH);
    CloseHandle(hProc);
    if (!gamePathW[0]) return false;
    WIN32_FILE_ATTRIBUTE_DATA gameAttr, offAttr;
    if (!GetFileAttributesExW(gamePathW, GetFileExInfoStandard, &gameAttr)) return true;
    if (!GetFileAttributesExA(offsetsPath.c_str(), GetFileExInfoStandard, &offAttr)) return true;
    ULARGE_INTEGER gameTime, offTime;
    gameTime.LowPart = gameAttr.ftLastWriteTime.dwLowDateTime;
    gameTime.HighPart = gameAttr.ftLastWriteTime.dwHighDateTime;
    offTime.LowPart = offAttr.ftLastWriteTime.dwLowDateTime;
    offTime.HighPart = offAttr.ftLastWriteTime.dwHighDateTime;
    return offTime.QuadPart < gameTime.QuadPart;
}

static bool RunScannerMM() {
    std::wstring scanner = StringToWString(g_dataDir) + L"\\PalOffsetScanner.exe";
    DWORD pid = FindPalworldPid();
    if (pid == 0) return false;
    HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (!hProc) return false;
    wchar_t gamePathW[MAX_PATH] = {};
    GetModuleFileNameExW(hProc, nullptr, gamePathW, MAX_PATH);
    CloseHandle(hProc);
    if (!gamePathW[0]) return false;
    std::wstring args = L"\"" + scanner + L"\" \"" + gamePathW + L"\"";
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi = {};
    if (!CreateProcessW(scanner.c_str(), &args[0], nullptr, nullptr, FALSE, 0, nullptr, StringToWString(g_dataDir).c_str(), &si, &pi)) {
        return false;
    }
    WaitForSingleObject(pi.hProcess, 30000);
    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return exitCode == 0;
}

static bool ReadMem(HANDLE hProc, uintptr_t addr, void* buf, size_t len) {
    SIZE_T bytesRead = 0;
    return ReadProcessMemory(hProc, (LPCVOID)addr, buf, len, &bytesRead) && bytesRead == len;
}

static uintptr_t ReadPtr(HANDLE hProc, uintptr_t addr) {
    uintptr_t val = 0;
    ReadMem(hProc, addr, &val, sizeof(val));
    return val;
}

static bool ReadPlayerFromMemory() {
    DWORD pid = FindPalworldPid();
    if (pid == 0) {
        if (g_memProcessHandle) { CloseHandle(g_memProcessHandle); g_memProcessHandle = nullptr; g_memProcessPid = 0; }
        g_memDataSource = "Palworld non lance";
        return false;
    }
    if (pid != g_memProcessPid || !g_memProcessHandle) {
        if (g_memProcessHandle) { CloseHandle(g_memProcessHandle); g_memProcessHandle = nullptr; }
        g_memProcessHandle = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);
        if (!g_memProcessHandle) {
            g_memDataSource = "Erreur OpenProcess";
            return false;
        }
        g_memProcessPid = pid;
        g_memBaseAddr = GetModuleBaseRemote(g_memProcessHandle);
        if (!g_memBaseAddr) {
            g_memDataSource = "Module base introuvable";
            return false;
        }
    }
    if (!LoadMemOffsets() || !g_memGWorldRva) {
        // Try rescanning offsets if they're missing or stale
        if (NeedRescanOffsetsMM()) {
            g_memDataSource = "Scan des offsets...";
            RunScannerMM();
            g_memOffsetsLoaded = false;
            LoadMemOffsets();
        }
        if (!g_memGWorldRva) {
            g_memDataSource = "Offsets non charges";
            return false;
        }
    }

    uintptr_t gWorld = ReadPtr(g_memProcessHandle, g_memBaseAddr + g_memGWorldRva);
    if (!gWorld) {
        // GWorld null — offsets may be stale, try rescan once
        static time_t lastRescan = 0;
        time_t now = time(nullptr);
        if (now - lastRescan > 30) {
            lastRescan = now;
            g_memDataSource = "Rescan offsets (GWorld null)...";
            Log("ReadPlayerFromMemory: GWorld null, rescanning offsets...");
            RunScannerMM();
            g_memOffsetsLoaded = false;
            if (LoadMemOffsets() && g_memGWorldRva) {
                gWorld = ReadPtr(g_memProcessHandle, g_memBaseAddr + g_memGWorldRva);
            }
        }
        if (!gWorld) { g_memDataSource = "GWorld null"; return false; }
    }

    static time_t lastChainLog = 0;
    bool logChain = (time(nullptr) - lastChainLog >= 5);
    if (logChain) {
        lastChainLog = time(nullptr);
        Log("ReadPlayerFromMemory: GWorld=0x%llX base=0x%llX rva=0x%llX",
            (unsigned long long)gWorld, (unsigned long long)g_memBaseAddr, (unsigned long long)g_memGWorldRva);
    }

    uintptr_t gameInstance = ReadPtr(g_memProcessHandle, gWorld + 0x1B8);
    if (!gameInstance) {
        if (logChain) Log("ReadPlayerFromMemory: GameInstance NULL (GWorld+0x1B8)");
        g_memDataSource = "GameInstance null";
        return false;
    }

    uintptr_t lpData = ReadPtr(g_memProcessHandle, gameInstance + 0x038);
    int32_t lpCount = 0;
    ReadMem(g_memProcessHandle, gameInstance + 0x038 + 0x08, &lpCount, sizeof(lpCount));
    if (!lpData || lpCount <= 0) {
        if (logChain) Log("ReadPlayerFromMemory: LocalPlayer empty (GI=0x%llX lpData=0x%llX count=%d)",
            (unsigned long long)gameInstance, (unsigned long long)lpData, lpCount);
        g_memDataSource = "Pas de LocalPlayer";
        return false;
    }

    uintptr_t localPlayer = ReadPtr(g_memProcessHandle, lpData);
    if (!localPlayer) {
        if (logChain) Log("ReadPlayerFromMemory: LocalPlayer NULL (lpData=0x%llX)", (unsigned long long)lpData);
        g_memDataSource = "LocalPlayer null";
        return false;
    }

    uintptr_t controller = ReadPtr(g_memProcessHandle, localPlayer + 0x030);
    if (!controller) {
        if (logChain) Log("ReadPlayerFromMemory: Controller NULL (LP=0x%llX +0x030)", (unsigned long long)localPlayer);
        g_memDataSource = "Controller null";
        return false;
    }

    uintptr_t pawn = ReadPtr(g_memProcessHandle, controller + 0x338);
    if (!pawn) pawn = ReadPtr(g_memProcessHandle, controller + 0x2D0);
    if (!pawn) {
        if (logChain) Log("ReadPlayerFromMemory: Pawn NULL (Ctrl=0x%llX +0x338/0x2D0)", (unsigned long long)controller);
        g_memDataSource = "Pawn null";
        return false;
    }

    uintptr_t rootComp = ReadPtr(g_memProcessHandle, pawn + 0x198);
    if (!rootComp) {
        if (logChain) Log("ReadPlayerFromMemory: RootComp NULL (Pawn=0x%llX +0x198)", (unsigned long long)pawn);
        g_memDataSource = "RootComp null";
        return false;
    }

    double loc[3] = {0, 0, 0};
    if (!ReadMem(g_memProcessHandle, rootComp + 0x128, loc, sizeof(loc))) {
        if (logChain) Log("ReadPlayerFromMemory: Position read FAIL (RootComp=0x%llX +0x128)", (unsigned long long)rootComp);
        g_memDataSource = "Lecture position echec";
        return false;
    }

    g_player.x = (float)loc[0];
    g_player.y = (float)loc[1];
    g_player.z = (float)loc[2];
    g_player.valid = true;
    g_player.name = "Player";
    g_memDataSource = "via memoire";
    if (logChain) Log("ReadPlayerFromMemory: OK pos=(%.0f, %.0f, %.0f)", g_player.x, g_player.y, g_player.z);
    return true;
}

// ----------------------------------------------------------------------------
// Favoris
// ----------------------------------------------------------------------------
struct FavoritePoi {
    std::string id;
    std::string label;
    float x = 0, y = 0, z = 0;
};
static std::vector<FavoritePoi> g_favorites;
static bool g_showAddFavoritePopup = false;
static char g_favoriteNameBuf[128] = "";
static bool g_showFiltersWindow = false;

// ----------------------------------------------------------------------------
// Pals à proximité (lecture via ReadProcessMemory)
// ----------------------------------------------------------------------------
struct PalEntity {
    float x = 0, y = 0, z = 0;
    int level = 0;
    bool isAlpha = false;
    bool isBoss = false;
    std::string name;
    std::string palId; // e.g. "Lamball", "Anubis"
    bool isShiny = false;
    bool isPlayer = false;
};
static std::vector<PalEntity> g_nearbyPals;
static bool g_showPals = false;
static float g_palScanRadius = 2000.0f;
static time_t g_lastPalScan = 0;
static bool g_showEspLines = false;
static bool g_showEspNames = false;

// ----------------------------------------------------------------------------
// World entities (chests, eggs, effigies, skillfruits) via RPM
// ----------------------------------------------------------------------------
struct WorldEntity {
    float x = 0, y = 0, z = 0;
    int entityType = 0; // 0=chest, 1=egg, 2=effigy, 3=skillfruit
    std::string label;
};
static std::vector<WorldEntity> g_worldEntities;
static bool g_showWorldEntities = true;
static float g_entityScanRadius = 5000.0f;
static time_t g_lastEntityScan = 0;

static ImU32 GetEntityColor(int entityType) {
    switch (entityType) {
        case 0: return IM_COL32(255, 200, 0, 255);   // chest = gold
        case 1: return IM_COL32(255, 220, 60, 255);   // egg = yellow
        case 2: return IM_COL32(180, 60, 255, 255);   // effigy = purple
        case 3: return IM_COL32(60, 255, 100, 255);   // skillfruit = green
        default: return IM_COL32(200, 200, 200, 255);
    }
}

static const char* GetEntityTypeName(int entityType) {
    switch (entityType) {
        case 0: return "Coffre";
        case 1: return "Oeuf";
        case 2: return "Effigie";
        case 3: return "Fruit compétence";
        default: return "Inconnu";
    }
}

static void ReadNearbyWorldEntities() {
    g_worldEntities.clear();
    if (!g_player.valid || !g_memProcessHandle || !g_memBaseAddr) return;

    uintptr_t gWorld = ReadPtr(g_memProcessHandle, g_memBaseAddr + g_memGWorldRva);
    if (!gWorld) return;

    uintptr_t persistentLevel = ReadPtr(g_memProcessHandle, gWorld + 0x38);
    if (!persistentLevel) return;

    uintptr_t actorsData = ReadPtr(g_memProcessHandle, persistentLevel + 0x98);
    int32_t actorCount = 0;
    ReadMem(g_memProcessHandle, persistentLevel + 0x98 + 0x08, &actorCount, sizeof(actorCount));
    if (!actorsData || actorCount <= 0) return;

    int maxScan = std::min(actorCount, 3000);
    float radiusSq = g_entityScanRadius * g_entityScanRadius;

    for (int i = 0; i < maxScan; ++i) {
        uintptr_t actor = ReadPtr(g_memProcessHandle, actorsData + i * sizeof(uintptr_t));
        if (!actor) continue;

        // Read actor class
        uintptr_t actorClass = ReadPtr(g_memProcessHandle, actor + 0x10);
        if (!actorClass) continue;

        // Read class FName index
        int32_t nameIndex = 0;
        ReadMem(g_memProcessHandle, actorClass + 0x18, &nameIndex, sizeof(nameIndex));

        // Read RootComponent for position
        uintptr_t rootComp = ReadPtr(g_memProcessHandle, actor + 0x198);
        if (!rootComp) continue;

        double loc[3] = {0, 0, 0};
        if (!ReadMem(g_memProcessHandle, rootComp + 0x128, loc, sizeof(loc))) continue;

        float ax = (float)loc[0], ay = (float)loc[1], az = (float)loc[2];
        if (std::abs(ax) < 1.0f && std::abs(ay) < 1.0f) continue;

        float dx = ax - g_player.x, dy = ay - g_player.y;
        float distSq = dx * dx + dy * dy;
        if (distSq > radiusSq) continue;

        // Heuristic entity type detection based on known Palworld actor class name indices
        // These indices correspond to common map object classes in Palworld 1.0
        // PalMapObjectModel_Chest -> chest
        // PalMapObjectModel_Egg -> egg
        // PalMapObjectModel_Relic -> effigy
        // PalMapObjectModel_SkillFruit -> skillfruit
        // Since we can't resolve FName to string without the name table,
        // we use a heuristic: check the actor's OuterPrivate chain and compare
        // against known class name indices from the offset scanner

        // Alternative heuristic: check actor size/scale and specific offsets
        // Chests typically have a small bounding box
        // Eggs are small spherical objects
        // Effigies are tall thin objects
        // Skillfruits hang from trees

        // For now, we use a simple approach: read a known offset that indicates
        // the map object type. In Palworld 1.0, PalMapObjectModel has a
        // MapObjectModelType field at a known offset.
        // MapObjectModelType: 0 = chest, 1 = egg, 2 = relic/effigy, 3 = skillfruit

        // Try reading the MapObjectModelType from a common offset
        // This offset may need adjustment per game version
        int32_t modelType = -1;
        ReadMem(g_memProcessHandle, actor + 0x6A0, &modelType, sizeof(modelType));

        if (modelType >= 0 && modelType <= 3) {
            WorldEntity ent;
            ent.x = ax;
            ent.y = ay;
            ent.z = az;
            ent.entityType = modelType;
            ent.label = GetEntityTypeName(modelType);
            g_worldEntities.push_back(ent);
            if (g_worldEntities.size() >= 200) break;
        }
    }
}
static bool g_autoZoom = false;
static bool g_showCompass = true;

// ----------------------------------------------------------------------------
// Pal Spawn Database
// ----------------------------------------------------------------------------
struct PalSpawn {
    std::string palName;
    float x = 0, y = 0, z = 0;
    int levelMin = 0, levelMax = 0;
    bool isDay = false;
    bool isNight = false;
    bool isAlpha = false;
};
static std::vector<PalSpawn> g_palSpawns;
static bool g_palSpawnsLoaded = false;
static bool g_showPalSpawns = false;
static bool g_spawnFilterDay = true;
static bool g_spawnFilterNight = true;
static bool g_spawnFilterAlpha = true;
static bool g_spawnFilterNonAlpha = true;
static int g_spawnFilterLevelMin = 0;
static int g_spawnFilterLevelMax = 100;
static char g_spawnSearchBuf[128] = "";
static std::string g_selectedPalName; // empty = show all
static std::vector<std::string> g_palSpawnNames;

static void LoadPalSpawns() {
    std::string path = g_dataDir + "/pal_spawns.json";
    std::string text = ReadFileTextA(path.c_str());
    if (text.empty()) {
        path = g_dataDir + "/data/pal_spawns.json";
        text = ReadFileTextA(path.c_str());
    }
    if (text.empty()) {
        path = g_dataDir + "/../data/pal_spawns.json";
        text = ReadFileTextA(path.c_str());
    }
    if (text.empty()) {
        // Try relative path from exe directory
        path = "data/pal_spawns.json";
        text = ReadFileTextA(path.c_str());
    }
    if (text.empty()) return;

    g_palSpawns.clear();
    g_palSpawnNames.clear();

    // Simple JSON array parser for our flat format
    size_t i = 0;
    JsonSkipSpace(text, i);
    if (i >= text.size() || text[i] != '[') return;
    i++;

    while (i < text.size()) {
        JsonSkipSpace(text, i);
        if (i >= text.size() || text[i] == ']') break;

        if (text[i] == '{') {
            i++;
            PalSpawn spawn;
            while (i < text.size() && text[i] != '}') {
                JsonSkipSpace(text, i);
                if (i >= text.size() || text[i] == '}') break;
                std::string key = JsonParseString(text, i);
                JsonSkipSpace(text, i);
                if (i < text.size() && text[i] == ':') i++;
                JsonSkipSpace(text, i);

                if (key == "palName") {
                    spawn.palName = JsonParseString(text, i);
                } else if (key == "x") spawn.x = (float)JsonParseNumber(text, i);
                else if (key == "y") spawn.y = (float)JsonParseNumber(text, i);
                else if (key == "z") spawn.z = (float)JsonParseNumber(text, i);
                else if (key == "levelMin") spawn.levelMin = (int)JsonParseNumber(text, i);
                else if (key == "levelMax") spawn.levelMax = (int)JsonParseNumber(text, i);
                else if (key == "isDay") {
                    if (i < text.size() && text[i] == 't') { spawn.isDay = true; i += 4; }
                    else if (i < text.size() && text[i] == 'f') { spawn.isDay = false; i += 5; }
                } else if (key == "isNight") {
                    if (i < text.size() && text[i] == 't') { spawn.isNight = true; i += 4; }
                    else if (i < text.size() && text[i] == 'f') { spawn.isNight = false; i += 5; }
                } else if (key == "isAlpha") {
                    if (i < text.size() && text[i] == 't') { spawn.isAlpha = true; i += 4; }
                    else if (i < text.size() && text[i] == 'f') { spawn.isAlpha = false; i += 5; }
                } else {
                    if (i < text.size() && (text[i] == 't' || text[i] == 'f')) {
                        while (i < text.size() && text[i] != ',' && text[i] != '}') i++;
                    } else {
                        JsonParseNumber(text, i);
                    }
                }
                JsonSkipSpace(text, i);
                if (i < text.size() && text[i] == ',') i++;
            }
            if (i < text.size() && text[i] == '}') i++;

            // Track unique pal names
            bool found = false;
            for (const auto& n : g_palSpawnNames) {
                if (n == spawn.palName) { found = true; break; }
            }
            if (!found) g_palSpawnNames.push_back(spawn.palName);

            g_palSpawns.push_back(spawn);
        }

        JsonSkipSpace(text, i);
        if (i < text.size() && text[i] == ',') i++;
    }

    g_palSpawnsLoaded = true;
}

static void ReadNearbyPals() {
    g_nearbyPals.clear();
    if (!g_player.valid || !g_memProcessHandle || !g_memBaseAddr) return;

    // Read ULevel (PersistentLevel) from GWorld
    uintptr_t gWorld = ReadPtr(g_memProcessHandle, g_memBaseAddr + g_memGWorldRva);
    if (!gWorld) return;

    // UWorld->PersistentLevel (offset 0x38 in UE5)
    uintptr_t persistentLevel = ReadPtr(g_memProcessHandle, gWorld + 0x38);
    if (!persistentLevel) return;

    // ULevel->Actors (TArray: Data ptr at +0x98, Count at +0xA0 in UE5)
    uintptr_t actorsData = ReadPtr(g_memProcessHandle, persistentLevel + 0x98);
    int32_t actorCount = 0;
    ReadMem(g_memProcessHandle, persistentLevel + 0x98 + 0x08, &actorCount, sizeof(actorCount));
    if (!actorsData || actorCount <= 0) return;

    // Limit scan for performance
    int maxScan = std::min(actorCount, 2000);
    float radiusSq = g_palScanRadius * g_palScanRadius;

    for (int i = 0; i < maxScan; ++i) {
        uintptr_t actor = ReadPtr(g_memProcessHandle, actorsData + i * sizeof(uintptr_t));
        if (!actor) continue;

        // Read RootComponent (offset 0x198 in UE5 AActor)
        uintptr_t rootComp = ReadPtr(g_memProcessHandle, actor + 0x198);
        if (!rootComp) continue;

        // Read RelativeLocation (double[3] at offset 0x128 in USceneComponent)
        double loc[3] = {0, 0, 0};
        if (!ReadMem(g_memProcessHandle, rootComp + 0x128, loc, sizeof(loc))) continue;

        float ax = (float)loc[0], ay = (float)loc[1], az = (float)loc[2];
        float dx = ax - g_player.x, dy = ay - g_player.y, dz = az - g_player.z;
        float distSq = dx * dx + dy * dy + dz * dz;
        if (distSq > radiusSq) continue;

        // Heuristic: check if this actor has valid character-like coordinates
        // Skip actors at origin or too far from ground
        if (std::abs(ax) < 1.0f && std::abs(ay) < 1.0f) continue;

        PalEntity pal;
        pal.x = ax;
        pal.y = ay;
        pal.z = az;

        // Try to read actor class name for identification
        // AActor->UClass* at offset 0x10 ( UObjectBase->ClassPrivate )
        uintptr_t actorClass = ReadPtr(g_memProcessHandle, actor + 0x10);
        if (actorClass) {
            // UClass->FName (NamePrivate) at offset 0x18 in UObjectBase
            // FName: ComparisonIndex (int32) at offset 0x00
            int32_t nameIndex = 0;
            ReadMem(g_memProcessHandle, actorClass + 0x18, &nameIndex, sizeof(nameIndex));
            // We can't easily resolve FName to string without the name table
            // But we can check known pal character class patterns
            // Read the CharacterParameterComponent if it exists
            // Pal characters typically have a component at offset 0x2A8 or similar
            // that contains level and pal ID

            // Try to read level from common offsets in pal character
            // These are approximate and may need adjustment per game version
            int32_t level = 0;
            // Offset 0x5B0 is a common level offset in pal characters
            ReadMem(g_memProcessHandle, actor + 0x5B0, &level, sizeof(level));
            if (level > 0 && level <= 100) {
                pal.level = level;
            }
        }

        // Detect alpha/boss by checking scale or specific flags
        // Alpha pals typically have larger scale
        float scale[3] = {1.0f, 1.0f, 1.0f};
        ReadMem(g_memProcessHandle, rootComp + 0x140, scale, sizeof(scale)); // RelativeScale3D
        if (scale[0] > 1.5f || scale[1] > 1.5f || scale[2] > 1.5f) {
            pal.isAlpha = true;
        }

        // Skip if too many
        g_nearbyPals.push_back(pal);
        if (g_nearbyPals.size() >= 80) break; // Cap for performance
    }
}

// ----------------------------------------------------------------------------
// Animation timing
// ----------------------------------------------------------------------------
static float g_animTime = 0.0f;

static std::string FindFavoritesPath() {
    return g_dataDir + "/favorites.json";
}

static void LoadFavorites() {
    std::string path = FindFavoritesPath();
    std::string text = ReadFileTextA(path.c_str());
    if (text.empty()) return;
    size_t i = 0;
    JsonSkipSpace(text, i);
    if (i >= text.size() || text[i] != '[') return;
    ++i;
    while (i < text.size()) {
        JsonSkipSpace(text, i);
        if (i < text.size() && text[i] == ']') break;
        if (text[i] != '{') { JsonSkipValue(text, i); continue; }
        ++i;
        FavoritePoi fav;
        while (i < text.size()) {
            JsonSkipSpace(text, i);
            if (i < text.size() && text[i] == '}') { ++i; break; }
            if (text[i] != '"') { JsonSkipValue(text, i); continue; }
            std::string key = JsonParseString(text, i);
            JsonSkipSpace(text, i);
            if (i < text.size() && text[i] == ':') ++i;
            JsonSkipSpace(text, i);
            if (key == "id" && text[i] == '"') fav.id = JsonParseString(text, i);
            else if (key == "label" && text[i] == '"') fav.label = JsonParseString(text, i);
            else if (key == "x") fav.x = (float)JsonParseNumber(text, i);
            else if (key == "y") fav.y = (float)JsonParseNumber(text, i);
            else if (key == "z") fav.z = (float)JsonParseNumber(text, i);
            else JsonSkipValue(text, i);
            JsonSkipSpace(text, i);
            if (i < text.size() && text[i] == ',') ++i;
        }
        g_favorites.push_back(fav);
        JsonSkipSpace(text, i);
        if (i < text.size() && text[i] == ',') ++i;
    }
}

static std::string JsonEscapeString(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 2);
    for (char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if ((unsigned char)c >= 0x20) out += c;
                else { char buf[8]; snprintf(buf, sizeof(buf), "\\u%04x", (unsigned char)c); out += buf; }
        }
    }
    return out;
}

static void SaveFavorites() {
    std::string path = FindFavoritesPath();
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) return;
    f << "[\n";
    for (size_t i = 0; i < g_favorites.size(); ++i) {
        f << "  {\"id\":\"" << JsonEscapeString(g_favorites[i].id) << "\","
          << "\"label\":\"" << JsonEscapeString(g_favorites[i].label) << "\","
          << "\"x\":" << g_favorites[i].x << ","
          << "\"y\":" << g_favorites[i].y << ","
          << "\"z\":" << g_favorites[i].z << "}";
        if (i + 1 < g_favorites.size()) f << ",";
        f << "\n";
    }
    f << "]\n";
}

static void AddFavorite(const std::string& label, float x, float y, float z) {
    FavoritePoi fav;
    fav.id = "fav_" + std::to_string((uint64_t)time(nullptr)) + "_" + std::to_string(g_favorites.size());
    fav.label = label.empty() ? "Favori " + std::to_string(g_favorites.size() + 1) : label;
    fav.x = x; fav.y = y; fav.z = z;
    g_favorites.push_back(fav);
    SaveFavorites();
}

static void RemoveFavorite(const std::string& id) {
    g_favorites.erase(std::remove_if(g_favorites.begin(), g_favorites.end(),
        [&id](const FavoritePoi& f) { return f.id == id; }), g_favorites.end());
    SaveFavorites();
}

static void ClampMinimapView() {
    float half = 0.5f / g_minimapZoom;
    g_minimapCenter.x = std::clamp(g_minimapCenter.x, half, 1.0f - half);
    g_minimapCenter.y = std::clamp(g_minimapCenter.y, half, 1.0f - half);
}

static std::string FindServerScriptPath() {
    std::vector<std::string> candidates;
    candidates.push_back(g_dataDir + "/server/server.py");
    candidates.push_back(g_dataDir + "/../server/server.py");
    candidates.push_back(g_dataDir + "/../PalTrainerUltra/server/server.py");
    for (const auto& p : candidates) {
        std::ifstream test(p);
        if (test.good()) return p;
    }
    return "";
}

static bool StartWebServer() {
    if (g_webServerStarted) return true;
    std::string script = FindServerScriptPath();
    if (script.empty()) {
        SetStatus("Serveur web introuvable.");
        return false;
    }
    SetEnvironmentVariableA("PALTRAINER_DATA_DIR", g_dataDir.c_str());
    SetEnvironmentVariableA("PALTRAINER_PORT", "8765");

    const char* pythons[] = { "pythonw.exe", "python.exe" };
    for (auto py : pythons) {
        std::wstring cmd = StringToWString(std::string(py) + " \"" + script + "\"");
        STARTUPINFOW si = { sizeof(si) };
        PROCESS_INFORMATION pi = {};
        DWORD flags = CREATE_NO_WINDOW | CREATE_NEW_PROCESS_GROUP | DETACHED_PROCESS;
        if (CreateProcessW(nullptr, &cmd[0], nullptr, nullptr, FALSE, flags, nullptr, nullptr, &si, &pi)) {
            g_childPids.push_back(pi.dwProcessId);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            g_webServerStarted = true;
            return true;
        }
    }
    SetStatus("Échec du lancement du serveur web (python introuvable).");
    return false;
}

static void OpenWebMap() {
    StartWebServer();
    ShellExecuteA(nullptr, "open", "http://127.0.0.1:8765", nullptr, nullptr, SW_SHOWNORMAL);
    g_webServerStarted = true;
}

// --- Préférences ---
static float g_menuWidth = 260.0f;
static bool g_prefsDirty = false;
static time_t g_prefsLastSave = 0;
static ImVec2 g_menuWindowPos(30.0f, 30.0f);
static ImVec2 g_menuWindowSize(220.0f, 420.0f);
static ImVec2 g_mapWindowPos(310.0f, 30.0f);
static ImVec2 g_mapWindowSize(560.0f, 600.0f);
static bool g_menuWindowOpen = true;
static bool g_mapWindowOpen = true;

// Module windows (each tab becomes its own draggable window)
static ImVec2 g_cartelWinPos(30.0f, 460.0f);
static ImVec2 g_cartelWinSize(220.0f, 400.0f);
static bool g_cartelWinOpen = true;
static ImVec2 g_palsWinPos(270.0f, 460.0f);
static ImVec2 g_palsWinSize(240.0f, 360.0f);
static bool g_palsWinOpen = false;
static ImVec2 g_spawnsWinPos(530.0f, 460.0f);
static ImVec2 g_spawnsWinSize(280.0f, 420.0f);
static bool g_spawnsWinOpen = false;
static ImVec2 g_filtersWinPos(840.0f, 30.0f);
static ImVec2 g_filtersWinSize(200.0f, 360.0f);
static bool g_filtersWinOpen = false;
static ImVec2 g_favorisWinPos(840.0f, 400.0f);
static ImVec2 g_favorisWinSize(220.0f, 360.0f);
static bool g_favorisWinOpen = false;
static ImVec2 g_cheatsWinPos(270.0f, 30.0f);
static ImVec2 g_cheatsWinSize(260.0f, 400.0f);
static bool g_cheatsWinOpen = false;

static std::string FindPrefsPath() {
    return g_dataDir + "/minimap_prefs.json";
}

static void SavePrefs() {
    std::string path = FindPrefsPath();
    FILE* f = fopen(path.c_str(), "w");
    if (!f) return;
    fprintf(f, "{\n");
    fprintf(f, "  \"menuWidth\": %.1f,\n", g_menuWidth);
    fprintf(f, "  \"menuPosX\": %.1f,\n", g_menuWindowPos.x);
    fprintf(f, "  \"menuPosY\": %.1f,\n", g_menuWindowPos.y);
    fprintf(f, "  \"menuSizeX\": %.1f,\n", g_menuWindowSize.x);
    fprintf(f, "  \"menuSizeY\": %.1f,\n", g_menuWindowSize.y);
    fprintf(f, "  \"mapPosX\": %.1f,\n", g_mapWindowPos.x);
    fprintf(f, "  \"mapPosY\": %.1f,\n", g_mapWindowPos.y);
    fprintf(f, "  \"mapSizeX\": %.1f,\n", g_mapWindowSize.x);
    fprintf(f, "  \"mapSizeY\": %.1f,\n", g_mapWindowSize.y);
    fprintf(f, "  \"cartelPosX\": %.1f,\n  \"cartelPosY\": %.1f,\n  \"cartelSizeX\": %.1f,\n  \"cartelSizeY\": %.1f,\n", g_cartelWinPos.x, g_cartelWinPos.y, g_cartelWinSize.x, g_cartelWinSize.y);
    fprintf(f, "  \"palsPosX\": %.1f,\n  \"palsPosY\": %.1f,\n  \"palsSizeX\": %.1f,\n  \"palsSizeY\": %.1f,\n", g_palsWinPos.x, g_palsWinPos.y, g_palsWinSize.x, g_palsWinSize.y);
    fprintf(f, "  \"spawnsPosX\": %.1f,\n  \"spawnsPosY\": %.1f,\n  \"spawnsSizeX\": %.1f,\n  \"spawnsSizeY\": %.1f,\n", g_spawnsWinPos.x, g_spawnsWinPos.y, g_spawnsWinSize.x, g_spawnsWinSize.y);
    fprintf(f, "  \"filtersPosX\": %.1f,\n  \"filtersPosY\": %.1f,\n  \"filtersSizeX\": %.1f,\n  \"filtersSizeY\": %.1f,\n", g_filtersWinPos.x, g_filtersWinPos.y, g_filtersWinSize.x, g_filtersWinSize.y);
    fprintf(f, "  \"favorisPosX\": %.1f,\n  \"favorisPosY\": %.1f,\n  \"favorisSizeX\": %.1f,\n  \"favorisSizeY\": %.1f,\n", g_favorisWinPos.x, g_favorisWinPos.y, g_favorisWinSize.x, g_favorisWinSize.y);
    fprintf(f, "  \"cheatsPosX\": %.1f,\n  \"cheatsPosY\": %.1f,\n  \"cheatsSizeX\": %.1f,\n  \"cheatsSizeY\": %.1f,\n", g_cheatsWinPos.x, g_cheatsWinPos.y, g_cheatsWinSize.x, g_cheatsWinSize.y);
    fprintf(f, "  \"zoom\": %.4f,\n", g_minimapZoom);
    fprintf(f, "  \"followPlayer\": %s,\n", g_minimapFollowPlayer ? "true" : "false");
    fprintf(f, "  \"showPals\": %s,\n", g_showPals ? "true" : "false");
    fprintf(f, "  \"palScanRadius\": %.1f,\n", g_palScanRadius);
    fprintf(f, "  \"mapQuality\": %d,\n", g_mapQuality);
    fprintf(f, "  \"mapArea\": %d,\n", g_currentMapArea);
    fprintf(f, "  \"winWidth\": %d,\n", g_winWidth);
    fprintf(f, "  \"winHeight\": %d,\n", g_winHeight);
    fprintf(f, "  \"alwaysOnTop\": %s,\n", g_alwaysOnTop ? "true" : "false");
    fprintf(f, "  \"autoZoom\": %s,\n", g_autoZoom ? "true" : "false");
    fprintf(f, "  \"showCompass\": %s\n", g_showCompass ? "true" : "false");
    fprintf(f, "}\n");
    fclose(f);
}

static void LoadPrefs() {
    std::string path = FindPrefsPath();
    std::string text = ReadFileTextA(path.c_str());
    if (text.empty()) return;
    size_t i = 0;
    JsonSkipSpace(text, i);
    if (i >= text.size() || text[i] != '{') return;
    i++;
    while (i < text.size()) {
        JsonSkipSpace(text, i);
        if (i >= text.size() || text[i] == '}') break;
        std::string key = JsonParseString(text, i);
        JsonSkipSpace(text, i);
        if (i < text.size() && text[i] == ':') i++;
        JsonSkipSpace(text, i);
        if (key == "menuWidth") g_menuWidth = (float)JsonParseNumber(text, i);
        else if (key == "menuPosX") g_menuWindowPos.x = (float)JsonParseNumber(text, i);
        else if (key == "menuPosY") g_menuWindowPos.y = (float)JsonParseNumber(text, i);
        else if (key == "menuSizeX") g_menuWindowSize.x = (float)JsonParseNumber(text, i);
        else if (key == "menuSizeY") g_menuWindowSize.y = (float)JsonParseNumber(text, i);
        else if (key == "mapPosX") g_mapWindowPos.x = (float)JsonParseNumber(text, i);
        else if (key == "mapPosY") g_mapWindowPos.y = (float)JsonParseNumber(text, i);
        else if (key == "mapSizeX") g_mapWindowSize.x = (float)JsonParseNumber(text, i);
        else if (key == "mapSizeY") g_mapWindowSize.y = (float)JsonParseNumber(text, i);
        else if (key == "cartelPosX") g_cartelWinPos.x = (float)JsonParseNumber(text, i);
        else if (key == "cartelPosY") g_cartelWinPos.y = (float)JsonParseNumber(text, i);
        else if (key == "cartelSizeX") g_cartelWinSize.x = (float)JsonParseNumber(text, i);
        else if (key == "cartelSizeY") g_cartelWinSize.y = (float)JsonParseNumber(text, i);
        else if (key == "palsPosX") g_palsWinPos.x = (float)JsonParseNumber(text, i);
        else if (key == "palsPosY") g_palsWinPos.y = (float)JsonParseNumber(text, i);
        else if (key == "palsSizeX") g_palsWinSize.x = (float)JsonParseNumber(text, i);
        else if (key == "palsSizeY") g_palsWinSize.y = (float)JsonParseNumber(text, i);
        else if (key == "spawnsPosX") g_spawnsWinPos.x = (float)JsonParseNumber(text, i);
        else if (key == "spawnsPosY") g_spawnsWinPos.y = (float)JsonParseNumber(text, i);
        else if (key == "spawnsSizeX") g_spawnsWinSize.x = (float)JsonParseNumber(text, i);
        else if (key == "spawnsSizeY") g_spawnsWinSize.y = (float)JsonParseNumber(text, i);
        else if (key == "filtersPosX") g_filtersWinPos.x = (float)JsonParseNumber(text, i);
        else if (key == "filtersPosY") g_filtersWinPos.y = (float)JsonParseNumber(text, i);
        else if (key == "filtersSizeX") g_filtersWinSize.x = (float)JsonParseNumber(text, i);
        else if (key == "filtersSizeY") g_filtersWinSize.y = (float)JsonParseNumber(text, i);
        else if (key == "favorisPosX") g_favorisWinPos.x = (float)JsonParseNumber(text, i);
        else if (key == "favorisPosY") g_favorisWinPos.y = (float)JsonParseNumber(text, i);
        else if (key == "favorisSizeX") g_favorisWinSize.x = (float)JsonParseNumber(text, i);
        else if (key == "favorisSizeY") g_favorisWinSize.y = (float)JsonParseNumber(text, i);
        else if (key == "cheatsPosX") g_cheatsWinPos.x = (float)JsonParseNumber(text, i);
        else if (key == "cheatsPosY") g_cheatsWinPos.y = (float)JsonParseNumber(text, i);
        else if (key == "cheatsSizeX") g_cheatsWinSize.x = (float)JsonParseNumber(text, i);
        else if (key == "cheatsSizeY") g_cheatsWinSize.y = (float)JsonParseNumber(text, i);
        else if (key == "zoom") g_minimapZoom = (float)JsonParseNumber(text, i);
        else if (key == "followPlayer") {
            if (i < text.size() && text[i] == 't') { g_minimapFollowPlayer = true; i += 4; }
            else if (i < text.size() && text[i] == 'f') { g_minimapFollowPlayer = false; i += 5; }
        }
        else if (key == "showPals") {
            if (i < text.size() && text[i] == 't') { g_showPals = true; i += 4; }
            else if (i < text.size() && text[i] == 'f') { g_showPals = false; i += 5; }
        }
        else if (key == "palScanRadius") g_palScanRadius = (float)JsonParseNumber(text, i);
        else if (key == "mapQuality") { g_mapQuality = (int)JsonParseNumber(text, i); g_mapQualityChanged = true; }
        else if (key == "mapArea") g_currentMapArea = (int)JsonParseNumber(text, i);
        else if (key == "winWidth") g_winWidth = (int)JsonParseNumber(text, i);
        else if (key == "winHeight") g_winHeight = (int)JsonParseNumber(text, i);
        else if (key == "alwaysOnTop") {
            if (i < text.size() && text[i] == 't') { g_alwaysOnTop = true; i += 4; }
            else if (i < text.size() && text[i] == 'f') { g_alwaysOnTop = false; i += 5; }
        }
        else if (key == "autoZoom") {
            if (i < text.size() && text[i] == 't') { g_autoZoom = true; i += 4; }
            else if (i < text.size() && text[i] == 'f') { g_autoZoom = false; i += 5; }
        }
        else if (key == "showCompass") {
            if (i < text.size() && text[i] == 't') { g_showCompass = true; i += 4; }
            else if (i < text.size() && text[i] == 'f') { g_showCompass = false; i += 5; }
        }
        else {
            if (i < text.size() && (text[i] == 't' || text[i] == 'f')) {
                while (i < text.size() && text[i] != ',' && text[i] != '}') i++;
            } else {
                JsonParseNumber(text, i);
            }
        }
        JsonSkipSpace(text, i);
        if (i < text.size() && text[i] == ',') i++;
    }
    // Apply always-on-top on load
    if (g_alwaysOnTop && g_hwnd) {
        SetWindowPos(g_hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }
}

static void RenderMiniMap() {
    ImVec2 mapDisplaySize(0, 0);
    g_animTime += ImGui::GetIO().DeltaTime;
    // --- Auto-save des prefs (toutes les 3s si dirty) ---
    if (g_prefsDirty) {
        time_t now = time(nullptr);
        if (now - g_prefsLastSave >= 3) {
            SavePrefs();
            g_prefsDirty = false;
            g_prefsLastSave = now;
        }
    }

    if (IsMinimap()) {
    // --- Fenêtre 1: Menu (fenêtre indépendante et déplaçable) ---
    {
        ImGui::SetNextWindowPos(g_menuWindowPos, ImGuiCond_Once);
        ImGui::SetNextWindowSize(g_menuWindowSize, ImGuiCond_Once);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 8));
        ImGui::Begin("Menu", &g_menuWindowOpen, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoNav);

        // Sauvegarder position/taille si modifiées
        ImVec2 curPos = ImGui::GetWindowPos();
        ImVec2 curSize = ImGui::GetWindowSize();
        if (curPos.x != g_menuWindowPos.x || curPos.y != g_menuWindowPos.y) {
            g_menuWindowPos = curPos;
            g_prefsDirty = true;
        }
        if (curSize.x != g_menuWindowSize.x || curSize.y != g_menuWindowSize.y) {
            g_menuWindowSize = curSize;
            g_menuWidth = curSize.x;
            g_prefsDirty = true;
        }

        float bw = ImGui::GetContentRegionAvail().x;

        // Titre
        {
            float winW = ImGui::GetWindowWidth();
            const char* title = (g_currentMapArea == 1) ? "Arbre Monde" : "Palpagos";
            float tw = ImGui::CalcTextSize(title).x;
            ImGui::SetCursorPosX((winW - tw) * 0.5f);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.72f, 0.30f, 1.0f));
            ImGui::Text("%s", title);
            ImGui::PopStyleColor();
            ImGui::SetCursorPosX((winW - ImGui::CalcTextSize("Mini-carte").x) * 0.5f);
            ImGui::TextDisabled("Mini-carte");
        }

        ImGui::Spacing();

        // Statut connexion
        {
            float statusPulse = 0.5f + 0.5f * std::sin(g_animTime * 2.5f);
            if (g_injected) {
                float a = 0.7f + statusPulse * 0.3f;
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.9f, 0.3f, a));
                ImGui::TextUnformatted("  ● DLL injectee");
                ImGui::PopStyleColor();
            } else if (g_player.valid) {
                float a = 0.7f + statusPulse * 0.3f;
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.9f, 0.3f, a));
                ImGui::TextUnformatted("  ● Connecte");
                ImGui::PopStyleColor();
            } else {
                float a = 0.5f + statusPulse * 0.5f;
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.5f, 0.2f, a));
                ImGui::TextUnformatted("  ○ Deconnecte");
                ImGui::PopStyleColor();
            }
            if (!g_memDataSourceStr.empty()) {
                ImGui::TextDisabled("  %s", g_memDataSourceStr.c_str());
            }
        }

        ImGui::Spacing();

        // Actions
        {
            if (g_injected) {
                ImGui::TextDisabled("DLL injectee");
            } else if (g_attachInProgress) {
                ImGui::TextDisabled("Injection...");
            } else {
                if (ImGui::Button("Injecter DLL", ImVec2(bw, 0)))
                    StartAttach();
            }
            if (ImGui::Button("Carte web", ImVec2(bw, 0)))
                OpenWebMap();
        }

        ImGui::Spacing();
        if (ImGui::Button("Quitter", ImVec2(bw, 0))) {
            g_menuWindowOpen = false;
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Toggles pour afficher/cacher chaque module
        ImGui::TextDisabled("Modules:");
        ImGui::Checkbox("Carte", &g_cartelWinOpen);
        ImGui::Checkbox("Pals", &g_palsWinOpen);
        ImGui::Checkbox("Spawns", &g_spawnsWinOpen);
        ImGui::Checkbox("Filtres", &g_filtersWinOpen);
        ImGui::Checkbox("Favoris", &g_favorisWinOpen);
        ImGui::Checkbox("Cheats", &g_cheatsWinOpen);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextDisabled("Raccourcis:");
        ImGui::TextDisabled("  F1: Suivre  F2/F3: Zoom");
        ImGui::TextDisabled("  F4: Top  F5: Pals  F6: ESP");
        ImGui::TextDisabled("  Clic-droit: TP");

        ImGui::End();
        ImGui::PopStyleVar();
    }

    // --- Fenêtre Carte (settings) ---
    if (g_cartelWinOpen) {
        ImGui::SetNextWindowPos(g_cartelWinPos, ImGuiCond_Once);
        ImGui::SetNextWindowSize(g_cartelWinSize, ImGuiCond_Once);
        ImGui::Begin("Carte", &g_cartelWinOpen, ImGuiWindowFlags_NoCollapse);
        ImVec2 curPos = ImGui::GetWindowPos();
        ImVec2 curSize = ImGui::GetWindowSize();
        if (curPos.x != g_cartelWinPos.x || curPos.y != g_cartelWinPos.y) { g_cartelWinPos = curPos; g_prefsDirty = true; }
        if (curSize.x != g_cartelWinSize.x || curSize.y != g_cartelWinSize.y) { g_cartelWinSize = curSize; g_prefsDirty = true; }

        bool changed = false;
        changed |= ImGui::Checkbox("Suivre joueur", &g_minimapFollowPlayer);
        ImGui::Spacing();

        ImGui::TextDisabled("Zoom");
        {
            float bw2 = (ImGui::GetContentRegionAvail().x - 6) * 0.5f;
            if (ImGui::Button("-##zoom_out", ImVec2(bw2, 0))) {
                g_minimapZoom /= 1.2f;
                if (g_minimapZoom < 1.0f) g_minimapZoom = 1.0f;
                ClampMinimapView();
                g_prefsDirty = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("+##zoom_in", ImVec2(bw2, 0))) {
                g_minimapZoom = std::min(g_minimapZoom * 1.2f, 20.0f);
                ClampMinimapView();
                g_prefsDirty = true;
            }
            if (ImGui::Button("RaZ##reset", ImVec2(-1, 0))) {
                g_minimapZoom = 1.0f;
                g_minimapCenter = ImVec2(0.5f, 0.5f);
                g_minimapFollowPlayer = true;
                g_prefsDirty = true;
            }
            ImGui::TextDisabled("Niveau x%.1f", g_minimapZoom);
        }

        ImGui::Spacing();
        ImGui::TextDisabled("Zone");
        {
            float bw2 = (ImGui::GetContentRegionAvail().x - 6) * 0.5f;
            if (ImGui::Button("Palpagos", ImVec2(bw2, 0))) {
                g_currentMapArea = 0;
                g_minimapCenter = ImVec2(0.5f, 0.5f);
                g_minimapZoom = 1.0f;
                g_minimapFollowPlayer = false;
                g_prefsDirty = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Arbre", ImVec2(bw2, 0))) {
                g_currentMapArea = 1;
                g_minimapCenter = ImVec2(0.5f, 0.5f);
                g_minimapZoom = 1.0f;
                g_minimapFollowPlayer = false;
                g_prefsDirty = true;
            }
        }

        ImGui::Spacing();
        ImGui::TextDisabled("Qualite");
        {
            const char* qLabels[] = { "SD", "HD", "UHD" };
            float bw2 = (ImGui::GetContentRegionAvail().x - 8) / 3.0f;
            for (int i = 0; i < 3; ++i) {
                if (i > 0) ImGui::SameLine();
                bool selected = (g_mapQuality == i);
                if (selected) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.30f, 0.26f, 0.12f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.30f, 0.14f, 1.0f));
                }
                if (ImGui::Button(qLabels[i], ImVec2(bw2, 0))) {
                    g_mapQuality = i;
                    g_mapQualityChanged = true;
                    g_prefsDirty = true;
                }
                if (selected) {
                    ImGui::PopStyleColor(2);
                }
            }
        }

        ImGui::Spacing();
        if (ImGui::Checkbox("Toujours au-dessus", &g_alwaysOnTop)) {
            SetWindowPos(g_hwnd, g_alwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            g_prefsDirty = true;
        }
        if (ImGui::Checkbox("Zoom auto (vitesse)", &g_autoZoom)) g_prefsDirty = true;
        if (ImGui::Checkbox("Boussole", &g_showCompass)) g_prefsDirty = true;

        ImGui::End();
    }

    // --- Fenêtre Pals ---
    if (g_palsWinOpen) {
        ImGui::SetNextWindowPos(g_palsWinPos, ImGuiCond_Once);
        ImGui::SetNextWindowSize(g_palsWinSize, ImGuiCond_Once);
        ImGui::Begin("Pals", &g_palsWinOpen, ImGuiWindowFlags_NoCollapse);
        ImVec2 curPos = ImGui::GetWindowPos();
        ImVec2 curSize = ImGui::GetWindowSize();
        if (curPos.x != g_palsWinPos.x || curPos.y != g_palsWinPos.y) { g_palsWinPos = curPos; g_prefsDirty = true; }
        if (curSize.x != g_palsWinSize.x || curSize.y != g_palsWinSize.y) { g_palsWinSize = curSize; g_prefsDirty = true; }

        ImGui::Spacing();
        if (ImGui::Checkbox("Afficher les pals", &g_showPals)) g_prefsDirty = true;
        if (g_showPals) {
            ImGui::Spacing();
            if (ImGui::SliderFloat("Rayon de scan", &g_palScanRadius, 200.0f, 5000.0f, "%.0fm")) g_prefsDirty = true;
            if (!g_nearbyPals.empty()) {
                ImGui::TextDisabled("%d pal(s) detecte(s)", (int)g_nearbyPals.size());

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Text("ESP - Entités proches:");
                ImGui::Spacing();

                // Sort by distance
                std::vector<std::pair<int, float>> palDistances;
                for (int i = 0; i < (int)g_nearbyPals.size(); ++i) {
                    const auto& pal = g_nearbyPals[i];
                    float dx = pal.x - g_player.x;
                    float dy = pal.y - g_player.y;
                    float dz = pal.z - g_player.z;
                    float dist = std::sqrt(dx*dx + dy*dy + dz*dz);
                    palDistances.push_back({i, dist});
                }
                std::sort(palDistances.begin(), palDistances.end(),
                    [](const auto& a, const auto& b) { return a.second < b.second; });

                ImGui::BeginChild("##esp_list", ImVec2(-1, 150), true);
                if (ImGui::BeginTable("##esp_table", 4, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY)) {
                    ImGui::TableSetupColumn("Type");
                    ImGui::TableSetupColumn("Lv");
                    ImGui::TableSetupColumn("Dist");
                    ImGui::TableSetupColumn("Position");
                    ImGui::TableHeadersRow();

                    for (const auto& [idx, dist] : palDistances) {
                        const auto& pal = g_nearbyPals[idx];
                        ImGui::TableNextRow();

                        ImU32 rowColor;
                        if (dist < 500.0f) rowColor = IM_COL32(60, 180, 60, 40);
                        else if (dist < 1500.0f) rowColor = IM_COL32(180, 180, 60, 40);
                        else rowColor = IM_COL32(180, 60, 60, 40);
                        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, rowColor);

                        ImGui::TableSetColumnIndex(0);
                        if (pal.isAlpha)
                            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Alpha");
                        else if (pal.isBoss)
                            ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.8f, 1.0f), "Boss");
                        else
                            ImGui::TextUnformatted("Pal");

                        ImGui::TableSetColumnIndex(1);
                        if (pal.level > 0)
                            ImGui::Text("%d", pal.level);
                        else
                            ImGui::TextDisabled("?");

                        ImGui::TableSetColumnIndex(2);
                        ImGui::Text("%.0fm", dist);

                        ImGui::TableSetColumnIndex(3);
                        ImGui::TextDisabled("%.0f,%.0f", pal.x, pal.y);
                    }
                    ImGui::EndTable();
                }
                ImGui::EndChild();

                ImGui::Checkbox("Lignes ESP", &g_showEspLines);
                ImGui::SameLine();
                ImGui::Checkbox("Noms ESP", &g_showEspNames);
            } else if (g_player.valid) {
                ImGui::TextDisabled("Aucun pal dans le rayon");
            }
        }
        ImGui::End();
    }

    // --- Fenêtre Spawns ---
    if (g_spawnsWinOpen) {
        ImGui::SetNextWindowPos(g_spawnsWinPos, ImGuiCond_Once);
        ImGui::SetNextWindowSize(g_spawnsWinSize, ImGuiCond_Once);
        ImGui::Begin("Spawns", &g_spawnsWinOpen, ImGuiWindowFlags_NoCollapse);
        ImVec2 curPos = ImGui::GetWindowPos();
        ImVec2 curSize = ImGui::GetWindowSize();
        if (curPos.x != g_spawnsWinPos.x || curPos.y != g_spawnsWinPos.y) { g_spawnsWinPos = curPos; g_prefsDirty = true; }
        if (curSize.x != g_spawnsWinSize.x || curSize.y != g_spawnsWinSize.y) { g_spawnsWinSize = curSize; g_prefsDirty = true; }

        ImGui::Spacing();
        if (!g_palSpawnsLoaded) {
            ImGui::TextDisabled("Aucune donnee de spawn trouvee");
            ImGui::TextDisabled("Placez pal_spawns.json dans le dossier data");
        } else {
            if (ImGui::Checkbox("Afficher les spawns", &g_showPalSpawns)) g_prefsDirty = true;
            ImGui::Spacing();

            ImGui::TextDisabled("Recherche:");
            ImGui::PushItemWidth(-1);
            ImGui::InputText("##spawn_search", g_spawnSearchBuf, sizeof(g_spawnSearchBuf));
            ImGui::PopItemWidth();
            ImGui::Spacing();

            ImGui::Checkbox("Jour", &g_spawnFilterDay);
            ImGui::SameLine();
            ImGui::Checkbox("Nuit", &g_spawnFilterNight);
            ImGui::Checkbox("Alpha", &g_spawnFilterAlpha);
            ImGui::SameLine();
            ImGui::Checkbox("Normal", &g_spawnFilterNonAlpha);
            ImGui::Spacing();

            ImGui::TextDisabled("Niveau:");
            ImGui::PushItemWidth(-1);
            int levelRange[2] = { g_spawnFilterLevelMin, g_spawnFilterLevelMax };
            if (ImGui::SliderInt2("##spawn_level", levelRange, 0, 100, "%d")) {
                g_spawnFilterLevelMin = levelRange[0];
                g_spawnFilterLevelMax = levelRange[1];
            }
            ImGui::PopItemWidth();
            ImGui::Spacing();

            if (!g_selectedPalName.empty()) {
                ImGui::TextDisabled("Selection: %s", g_selectedPalName.c_str());
                if (ImGui::SmallButton("Effacer selection")) g_selectedPalName.clear();
                ImGui::Spacing();
            }

            float tabH = ImGui::GetContentRegionAvail().y - 20;
            if (tabH < 40) tabH = 40;
            ImGui::BeginChild("##spawns_list", ImVec2(-1, tabH), false);
            std::string search = g_spawnSearchBuf;
            for (const auto& name : g_palSpawnNames) {
                if (!search.empty()) {
                    std::string lowerName = name;
                    std::string lowerSearch = search;
                    for (auto& c : lowerName) c = tolower(c);
                    for (auto& c : lowerSearch) c = tolower(c);
                    if (lowerName.find(lowerSearch) == std::string::npos) continue;
                }
                bool selected = (g_selectedPalName == name);
                if (selected) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.30f, 0.26f, 0.12f, 1.0f));
                }
                if (ImGui::SmallButton(name.c_str())) {
                    if (selected) g_selectedPalName.clear();
                    else g_selectedPalName = name;
                }
                if (selected) ImGui::PopStyleColor();
            }
            ImGui::EndChild();
            ImGui::TextDisabled("%d especes, %d spawns", (int)g_palSpawnNames.size(), (int)g_palSpawns.size());
        }
        ImGui::End();
    }

    // --- Fenêtre Filtres ---
    if (g_filtersWinOpen) {
        ImGui::SetNextWindowPos(g_filtersWinPos, ImGuiCond_Once);
        ImGui::SetNextWindowSize(g_filtersWinSize, ImGuiCond_Once);
        ImGui::Begin("Filtres", &g_filtersWinOpen, ImGuiWindowFlags_NoCollapse);
        ImVec2 curPos = ImGui::GetWindowPos();
        ImVec2 curSize = ImGui::GetWindowSize();
        if (curPos.x != g_filtersWinPos.x || curPos.y != g_filtersWinPos.y) { g_filtersWinPos = curPos; g_prefsDirty = true; }
        if (curSize.x != g_filtersWinSize.x || curSize.y != g_filtersWinSize.y) { g_filtersWinSize = curSize; g_prefsDirty = true; }

        ImGui::Spacing();
        float tabH = ImGui::GetContentRegionAvail().y;
        ImGui::BeginChild("##filters_scroll_menu", ImVec2(-1, tabH), false);
        for (auto& kv : g_poiFilter) {
            const char* label = GetPoiLabelFr(kv.first);
            ImGui::Checkbox(label, &kv.second);
        }
        ImGui::Separator();
        ImGui::Checkbox("Entités temps réel", &g_showWorldEntities);
        if (g_showWorldEntities) {
            ImGui::SliderFloat("Rayon de scan##entity", &g_entityScanRadius, 1000.0f, 20000.0f, "%.0f");
        }
        ImGui::EndChild();
        ImGui::End();
    }

    // --- Fenêtre Favoris ---
    if (g_favorisWinOpen) {
        ImGui::SetNextWindowPos(g_favorisWinPos, ImGuiCond_Once);
        ImGui::SetNextWindowSize(g_favorisWinSize, ImGuiCond_Once);
        ImGui::Begin("Favoris", &g_favorisWinOpen, ImGuiWindowFlags_NoCollapse);
        ImVec2 curPos = ImGui::GetWindowPos();
        ImVec2 curSize = ImGui::GetWindowSize();
        if (curPos.x != g_favorisWinPos.x || curPos.y != g_favorisWinPos.y) { g_favorisWinPos = curPos; g_prefsDirty = true; }
        if (curSize.x != g_favorisWinSize.x || curSize.y != g_favorisWinSize.y) { g_favorisWinSize = curSize; g_prefsDirty = true; }

        ImGui::Spacing();
        if (g_favorites.empty()) {
            ImGui::TextDisabled("Aucun favori");
        } else {
            float tabH = ImGui::GetContentRegionAvail().y - 28;
            if (tabH < 40) tabH = 40;
            ImGui::BeginChild("##favs_scroll_menu", ImVec2(-1, tabH), false);
            for (int i = (int)g_favorites.size() - 1; i >= 0; --i) {
                ImGui::PushID(i);
                ImGui::Text("%s", g_favorites[i].label.c_str());
                ImGui::SameLine();
                if (ImGui::SmallButton("Aller")) {
                    g_minimapFollowPlayer = false;
                    g_currentMapArea = WorldToMapArea(g_favorites[i].x, g_favorites[i].y);
                    float u = 0.5f, v = 0.5f;
                    WorldToMapUVArea(g_favorites[i].x, g_favorites[i].y, g_currentMapArea, u, v);
                    g_minimapCenter = ImVec2(u, v);
                    g_minimapZoom = std::max(g_minimapZoom, 4.0f);
                    ClampMinimapView();
                }
                ImGui::SameLine();
                if (g_injected && ImGui::SmallButton("TP")) {
                    g_cheats.teleportToPending = true;
                    g_cheats.teleportToX = g_favorites[i].x;
                    g_cheats.teleportToY = g_favorites[i].y;
                    g_cheats.teleportToZ = g_favorites[i].z;
                    WriteCommands();
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("X")) {
                    RemoveFavorite(g_favorites[i].id);
                }
                ImGui::PopID();
            }
            ImGui::EndChild();
        }
        ImGui::Spacing();
        if (ImGui::Button("+ Favori", ImVec2(-1, 0))) {
            g_showAddFavoritePopup = true;
            g_favoriteNameBuf[0] = '\0';
        }
        ImGui::End();
    }

    // --- Fenêtre Cheats ---
    if (g_cheatsWinOpen) {
        ImGui::SetNextWindowPos(g_cheatsWinPos, ImGuiCond_Once);
        ImGui::SetNextWindowSize(g_cheatsWinSize, ImGuiCond_Once);
        ImGui::Begin("Cheats", &g_cheatsWinOpen, ImGuiWindowFlags_NoCollapse);
        ImVec2 curPos = ImGui::GetWindowPos();
        ImVec2 curSize = ImGui::GetWindowSize();
        if (curPos.x != g_cheatsWinPos.x || curPos.y != g_cheatsWinPos.y) { g_cheatsWinPos = curPos; g_prefsDirty = true; }
        if (curSize.x != g_cheatsWinSize.x || curSize.y != g_cheatsWinSize.y) { g_cheatsWinSize = curSize; g_prefsDirty = true; }

        ImGui::Spacing();
        if (!g_injected) {
            ImGui::TextDisabled("Injectez la DLL pour activer les cheats");
        } else {
            bool changed = false;
            changed |= ImGui::Checkbox("God Mode", &g_cheats.godMode);
            changed |= ImGui::Checkbox("HP infinis", &g_cheats.infiniteHP);
            changed |= ImGui::Checkbox("Endurance infinie", &g_cheats.infiniteSP);
            changed |= ImGui::Checkbox("Poids infini", &g_cheats.infiniteWeight);
            ImGui::Spacing();
            changed |= ImGui::Checkbox("Super vitesse", &g_cheats.superSpeed);
            if (g_cheats.superSpeed) {
                ImGui::SliderFloat("Vitesse##speedval", &g_cheats.speedValue, 500.0f, 10000.0f, "%.0f");
                changed = true;
            }
            changed |= ImGui::Checkbox("Super saut", &g_cheats.superJump);
            if (g_cheats.superJump) {
                ImGui::SliderFloat("Saut##jumpval", &g_cheats.jumpValue, 1000.0f, 10000.0f, "%.0f");
                changed = true;
            }
            ImGui::Spacing();
            changed |= ImGui::Checkbox("Fly Mode", &g_cheats.flyMode);
            changed |= ImGui::Checkbox("No-Clip", &g_cheats.noClip);
            ImGui::Spacing();
            changed |= ImGui::Checkbox("Debloquer fast travel", &g_cheats.unlockFastTravel);
            changed |= ImGui::Checkbox("Temps clair", &g_cheats.clearWeather);
            ImGui::Spacing();
            if (ImGui::Button("Teleport vers joueur (avant)", ImVec2(-1, 0))) {
                if (g_player.valid) {
                    g_cheats.teleportToPending = true;
                    g_cheats.teleportToX = g_player.x + 500.0f;
                    g_cheats.teleportToY = g_player.y;
                    g_cheats.teleportToZ = g_player.z;
                    changed = true;
                }
            }
            if (changed) WriteCommands();

            // --- Cheats avancés ---
            ImGui::Spacing();
            ImGui::Separator();
            if (ImGui::CollapsingHeader("Actions")) {
                if (g_cheats.advValues.find("setAllItemCounts") == g_cheats.advValues.end())
                    g_cheats.advValues["setAllItemCounts"] = -1.0f;
                if (g_cheats.advValues.find("setLifmunkEffigyCount") == g_cheats.advValues.end())
                    g_cheats.advValues["setLifmunkEffigyCount"] = -1.0f;

                bool advChanged = false;
                advChanged |= ImGui::InputFloat("Tous les objets##setAllItemCounts", &g_cheats.advValues["setAllItemCounts"], 1, 100, "%.0f");
                if (ImGui::Button("Appliquer##setAllItemCounts")) advChanged = true;
                ImGui::SameLine();
                if (ImGui::Button("RAZ##setAllItemCounts")) { g_cheats.advValues["setAllItemCounts"] = -1.0f; advChanged = true; }

                advChanged |= ImGui::InputFloat("Effigies Lifmunk##setLifmunkEffigyCount", &g_cheats.advValues["setLifmunkEffigyCount"], 1, 10, "%.0f");
                if (ImGui::Button("Appliquer##setLifmunk")) advChanged = true;
                ImGui::SameLine();
                if (ImGui::Button("RAZ##setLifmunk")) { g_cheats.advValues["setLifmunkEffigyCount"] = -1.0f; advChanged = true; }
                if (advChanged) WriteCommands();
            }

            if (ImGui::CollapsingHeader("Interrupteurs avancés")) {
                ImGui::BeginChild("AdvTogglesMM", ImVec2(0, 200), true);
                bool advChanged = false;
                for (auto& kv : g_cheats.advBools) {
                    advChanged |= ImGui::Checkbox(LabelFr(kv.first).c_str(), &kv.second);
                }
                ImGui::EndChild();
                if (advChanged) WriteCommands();
            }

            if (ImGui::CollapsingHeader("Valeurs avancees")) {
                ImGui::BeginChild("AdvValuesMM", ImVec2(0, 200), true);
                bool advChanged = false;
                for (auto& kv : g_cheats.advValues) {
                    if (kv.first == "setAllItemCounts" || kv.first == "setLifmunkEffigyCount") continue;
                    advChanged |= ImGui::InputFloat(LabelFr(kv.first).c_str(), &kv.second, 0, 0, "%.2f");
                }
                ImGui::EndChild();
                if (advChanged) WriteCommands();
            }
        }
        ImGui::End();
    }

    // --- Fenêtre 2: Mini-carte (fenêtre indépendante et déplaçable) ---
    {
        ImGui::SetNextWindowPos(g_mapWindowPos, ImGuiCond_Once);
        ImGui::SetNextWindowSize(g_mapWindowSize, ImGuiCond_Once);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("Mini-carte", &g_mapWindowOpen, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoNav);

        ImVec2 curPos = ImGui::GetWindowPos();
        ImVec2 curSize = ImGui::GetWindowSize();
        if (curPos.x != g_mapWindowPos.x || curPos.y != g_mapWindowPos.y) {
            g_mapWindowPos = curPos;
            g_prefsDirty = true;
        }
        if (curSize.x != g_mapWindowSize.x || curSize.y != g_mapWindowSize.y) {
            g_mapWindowSize = curSize;
            g_prefsDirty = true;
        }
    }

    // --- Scan des pals ---
    if (g_showPals && g_player.valid) {
        time_t now = time(nullptr);
        if (now - g_lastPalScan >= 1) {
            g_lastPalScan = now;
            ReadNearbyPals();
        }
    } else {
        g_nearbyPals.clear();
    }

    // --- Scan des world entities (coffres, oeufs, effigies, skillfruits) ---
    if (g_showWorldEntities && g_player.valid) {
        time_t now = time(nullptr);
        if (now - g_lastEntityScan >= 2) {
            g_lastEntityScan = now;
            ReadNearbyWorldEntities();
        }
    } else {
        g_worldEntities.clear();
    }

    // La carte prend toute la place
    {
        ImVec2 mapAvail = ImGui::GetContentRegionAvail();
        float side = std::min(mapAvail.x, mapAvail.y);
        if (side < 64.0f) side = 64.0f;
        mapDisplaySize = ImVec2(side, side);
    }
    } else { // Overlay mode — embedded minimap
    ImGui::SetNextWindowSize(ImVec2(420, 560), ImGuiCond_FirstUseEver);
    ImGui::Begin("Mini-carte", nullptr, ImGuiWindowFlags_None);

    // --- Ligne 1 : boutons principaux ---
    if (ImGui::Button("Carte web")) OpenWebMap();
    ImGui::SameLine();
    ImGui::Checkbox("Suivre", &g_minimapFollowPlayer);
    ImGui::SameLine();
    if (ImGui::Button("+ Favori##mm")) {
        g_showAddFavoritePopup = true;
        g_favoriteNameBuf[0] = '\0';
    }
    ImGui::SameLine();
    if (ImGui::Button(g_showFiltersWindow ? "Fermer##filtres_mm" : "Filtres##filtres_mm"))
        g_showFiltersWindow = !g_showFiltersWindow;

    // --- Ligne injection DLL ---
    {
        if (g_injected) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.9f, 0.3f, 1.0f));
            ImGui::Text("DLL injectée");
            ImGui::PopStyleColor();
        } else if (g_attachInProgress) {
            ImGui::Text("Injection en cours...");
        } else {
            if (ImGui::Button("Injecter DLL##mm")) {
                StartAttach();
            }
        }
        ImGui::SameLine();
        if (!g_memDataSourceStr.empty()) {
            ImGui::TextDisabled("(%s)", g_memDataSourceStr.c_str());
        }
    }

    // --- Ligne 2 : zoom + switcher de zone ---
    if (ImGui::Button("-##zoom_out_mm", ImVec2(26, 0))) {
        g_minimapZoom /= 1.2f;
        if (g_minimapZoom < 1.0f) g_minimapZoom = 1.0f;
        ClampMinimapView();
    }
    ImGui::SameLine();
    if (ImGui::Button("+##zoom_in_mm", ImVec2(26, 0))) {
        g_minimapZoom = std::min(g_minimapZoom * 1.2f, 20.0f);
        ClampMinimapView();
    }
    ImGui::SameLine();
    if (ImGui::Button("RaZ##reset_mm", ImVec2(40, 0))) {
        g_minimapZoom = 1.0f;
        g_minimapCenter = ImVec2(0.5f, 0.5f);
        g_minimapFollowPlayer = true;
    }
    ImGui::SameLine();
    ImGui::Text("Zoom x%.1f", g_minimapZoom);
    ImGui::SameLine();
    if (ImGui::Button("Palpagos##area_main_mm", ImVec2(70, 0))) {
        g_currentMapArea = 0;
        g_minimapCenter = ImVec2(0.5f, 0.5f);
        g_minimapZoom = 1.0f;
        g_minimapFollowPlayer = false;
    }
    ImGui::SameLine();
    if (ImGui::Button("Arbre Monde##area_tree_mm", ImVec2(80, 0))) {
        g_currentMapArea = 1;
        g_minimapCenter = ImVec2(0.5f, 0.5f);
        g_minimapZoom = 1.0f;
        g_minimapFollowPlayer = false;
    }

    if (!g_memDataSourceStr.empty()) {
        ImGui::SameLine();
        ImGui::TextDisabled("(%s)", g_memDataSourceStr.c_str());
    }

    // --- Qualite de la carte ---
    {
        ImGui::Text("Qualite:");
        ImGui::SameLine();
        const char* qLabels[] = { "SD 2048", "HD 4096", "UHD 8192" };
        for (int i = 0; i < 3; ++i) {
            if (i > 0) ImGui::SameLine();
            if (ImGui::RadioButton(qLabels[i], g_mapQuality == i)) {
                g_mapQuality = i;
                g_mapQualityChanged = true;
            }
        }
    }

    ImVec2 avail = ImGui::GetContentRegionAvail();
    float side = std::min(std::max(std::min(avail.x, avail.y), 64.0f), 1024.0f);
    mapDisplaySize = ImVec2(side, side);

    if (g_mapTexture || g_mapTreeTexture) {
        if (g_appMode == AppMode::Overlay) {
        // Contrôles de zoom (mode trainer)
        if (ImGui::Button("-", ImVec2(24, 0))) {
            g_minimapZoom /= 1.2f;
            if (g_minimapZoom < 1.0f) g_minimapZoom = 1.0f;
            ClampMinimapView();
        }
        ImGui::SameLine();
        ImGui::Text("Zoom x%.1f", g_minimapZoom);
        ImGui::SameLine();
        if (ImGui::Button("+", ImVec2(24, 0))) {
            g_minimapZoom = std::min(g_minimapZoom * 1.2f, 20.0f);
            ClampMinimapView();
        }
        ImGui::SameLine();
        if (ImGui::Button("R\xc3\xa9initialiser")) {
            g_minimapZoom = 1.0f;
            g_minimapCenter = ImVec2(0.5f, 0.5f);
            g_minimapFollowPlayer = true;
        }
        ImGui::SameLine();
        // S\xc3\xa9lecteur de zone (mode trainer)
        if (ImGui::Button("Palpagos##area_main_trainer")) {
            g_currentMapArea = 0;
            g_minimapCenter = ImVec2(0.5f, 0.5f);
            g_minimapZoom = 1.0f;
            g_minimapFollowPlayer = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Arbre Monde##area_tree_trainer")) {
            g_currentMapArea = 1;
            g_minimapCenter = ImVec2(0.5f, 0.5f);
            g_minimapZoom = 1.0f;
            g_minimapFollowPlayer = false;
        }
        }

        ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();
        ImVec2 canvas_p1 = ImVec2(canvas_p0.x + mapDisplaySize.x, canvas_p0.y + mapDisplaySize.y);

        if (g_minimapFollowPlayer && g_player.valid) {
            int playerArea = WorldToMapArea(g_player.x, g_player.y);
            if (playerArea != g_currentMapArea) {
                g_currentMapArea = playerArea;
            }
            float pu = 0.5f, pv = 0.5f;
            WorldToMapUVArea(g_player.x, g_player.y, g_currentMapArea, pu, pv);
            g_minimapCenter = ImVec2(pu, pv);
            ClampMinimapView();
        }

        // Auto-zoom based on player speed
        if (g_autoZoom && g_minimapFollowPlayer && g_player.valid) {
            float speed = g_player.speed;
            float targetZoom = 1.0f;
            if (speed > 2000.0f) targetZoom = 1.5f;
            else if (speed > 1000.0f) targetZoom = 2.0f;
            else if (speed > 500.0f) targetZoom = 3.0f;
            else if (speed > 100.0f) targetZoom = 4.0f;
            else targetZoom = 5.0f;
            g_minimapZoom = g_minimapZoom + (targetZoom - g_minimapZoom) * 0.05f;
            ClampMinimapView();
        }


        float range = 1.0f / g_minimapZoom;
        ImVec2 uv0(g_minimapCenter.x - range * 0.5f, g_minimapCenter.y - range * 0.5f);
        ImVec2 uv1(g_minimapCenter.x + range * 0.5f, g_minimapCenter.y + range * 0.5f);

        ImGui::InvisibleButton("minimap_canvas", mapDisplaySize);
        bool hovered = ImGui::IsItemHovered();
        bool active = ImGui::IsItemActive();
        ImDrawList* dl = ImGui::GetWindowDrawList();

        // Select texture based on current area
        ID3D11ShaderResourceView* activeTex = (g_currentMapArea == 1 && g_mapTreeTexture) ? g_mapTreeTexture : g_mapTexture;
        dl->AddImage((ImTextureID)activeTex, canvas_p0, canvas_p1, uv0, uv1);

        char coord[64];
        snprintf(coord, sizeof(coord), "X:%.0f Y:%.0f Z:%.0f", g_player.x, g_player.y, g_player.z);
        dl->AddText(ImVec2(canvas_p0.x + 5, canvas_p0.y + 5), IM_COL32(255, 255, 255, 255), coord);

        // Compass (top-right of map)
        if (g_showCompass) {
            float cx = canvas_p1.x - 20;
            float cy = canvas_p0.y + 20;
            dl->AddCircleFilled(ImVec2(cx, cy), 15, IM_COL32(0, 0, 0, 120), 16);
            dl->AddCircle(ImVec2(cx, cy), 15, IM_COL32(200, 180, 100, 200), 16, 1.0f);
            // N (up)
            dl->AddText(ImVec2(cx - 4, cy - 14), IM_COL32(255, 80, 80, 255), "N");
            // S (down)
            dl->AddText(ImVec2(cx - 4, cy + 10), IM_COL32(200, 200, 200, 200), "S");
            // E (right)
            dl->AddText(ImVec2(cx + 10, cy - 6), IM_COL32(200, 200, 200, 200), "E");
            // W (left)
            dl->AddText(ImVec2(cx - 18, cy - 6), IM_COL32(200, 200, 200, 200), "W");
        }

        ImGuiIO& io = ImGui::GetIO();
        if (hovered) {
            float wheel = io.MouseWheel;
            if (wheel != 0.0f) {
                ImVec2 mousePos = io.MousePos;
                ImVec2 local = ImVec2(mousePos.x - canvas_p0.x, mousePos.y - canvas_p0.y);
                float fracX = local.x / mapDisplaySize.x;
                float fracY = local.y / mapDisplaySize.y;
                float mouseU = uv0.x + fracX * range;
                float mouseV = uv0.y + fracY * range;
                float newZoom = std::clamp(g_minimapZoom * (1.0f + wheel * 0.15f), 1.0f, 20.0f);
                float newRange = 1.0f / newZoom;
                g_minimapCenter.x = mouseU - (fracX - 0.5f) * newRange;
                g_minimapCenter.y = mouseV - (fracY - 0.5f) * newRange;
                g_minimapZoom = newZoom;
                ClampMinimapView();
                g_minimapFollowPlayer = false;
            }
            if (active) {
                ImVec2 mousePos = io.MousePos;
                if (g_minimapDragging) {
                    ImVec2 delta = ImVec2(mousePos.x - g_minimapLastMouse.x, mousePos.y - g_minimapLastMouse.y);
                    if (delta.x != 0.0f || delta.y != 0.0f) {
                        g_minimapCenter.x -= delta.x / mapDisplaySize.x * range;
                        g_minimapCenter.y -= delta.y / mapDisplaySize.y * range;
                        ClampMinimapView();
                        g_minimapFollowPlayer = false;
                    }
                }
                g_minimapDragging = true;
                g_minimapLastMouse = mousePos;
            } else {
                g_minimapDragging = false;
            }
        } else {
            g_minimapDragging = false;
        }

        // Right-click teleport on map
        if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && g_injected && g_player.valid) {
            ImVec2 mousePos = io.MousePos;
            ImVec2 local = ImVec2(mousePos.x - canvas_p0.x, mousePos.y - canvas_p0.y);
            float fracX = local.x / mapDisplaySize.x;
            float fracY = local.y / mapDisplaySize.y;
            float clickU = uv0.x + fracX * range;
            float clickV = uv0.y + fracY * range;
            float worldX = 0, worldY = 0;
            MapUVToWorldArea(clickU, clickV, g_currentMapArea, worldX, worldY);
            g_cheats.teleportToPending = true;
            g_cheats.teleportToX = worldX;
            g_cheats.teleportToY = worldY;
            g_cheats.teleportToZ = g_player.z;
            WriteCommands();
        }

        range = 1.0f / g_minimapZoom;
        uv0 = ImVec2(g_minimapCenter.x - range * 0.5f, g_minimapCenter.y - range * 0.5f);
        uv1 = ImVec2(g_minimapCenter.x + range * 0.5f, g_minimapCenter.y + range * 0.5f);

        // Points d'intérêt
        std::string hoveredPoi;
        if (!g_pois.empty()) {
            for (const auto& poi : g_pois) {
                auto it = g_poiFilter.find(poi.type);
                if (it == g_poiFilter.end() || !it->second) continue;
                // Skip POIs not in current area
                int poiArea = WorldToMapArea(poi.x, poi.y);
                if (poiArea != g_currentMapArea) continue;
                float u = 0.5f, v = 0.5f;
                WorldToMapUVArea(poi.x, poi.y, g_currentMapArea, u, v);
                if (u < uv0.x || u > uv1.x || v < uv0.y || v > uv1.y) continue;
                float px = canvas_p0.x + (u - uv0.x) / range * mapDisplaySize.x;
                float py = canvas_p0.y + (v - uv0.y) / range * mapDisplaySize.y;

                auto texIt = g_poiTextures.find(poi.type);
                if (texIt != g_poiTextures.end() && texIt->second.srv) {
                    float iconSize = 28.0f;
                    ImVec2 pmin(px - iconSize * 0.5f, py - iconSize * 0.5f);
                    ImVec2 pmax(px + iconSize * 0.5f, py + iconSize * 0.5f);
                    dl->AddImage((ImTextureID)texIt->second.srv, pmin, pmax, ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 255));
                } else {
                    ImU32 col = GetPoiColor(poi.type);
                    dl->AddCircleFilled(ImVec2(px, py), 4.5f, col);
                    dl->AddCircle(ImVec2(px, py), 5.5f, IM_COL32(0, 0, 0, 220), 0, 1.0f);
                }

                if (hovered && hoveredPoi.empty()) {
                    float dx = io.MousePos.x - px;
                    float dy = io.MousePos.y - py;
                    if (dx * dx + dy * dy < 12.0f * 12.0f) {
                        hoveredPoi = poi.label + "\n" + GetPoiLabelFr(poi.type);
                        if (g_injected) hoveredPoi += "\n[Shift+Clic-droit pour teleporter]";
                    }
                }
            }
        }

        // Shift+right-click on POI to teleport
        if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && io.KeyShift && g_injected && g_player.valid) {
            for (const auto& poi : g_pois) {
                auto it = g_poiFilter.find(poi.type);
                if (it == g_poiFilter.end() || !it->second) continue;
                int poiArea = WorldToMapArea(poi.x, poi.y);
                if (poiArea != g_currentMapArea) continue;
                float u = 0.5f, v = 0.5f;
                WorldToMapUVArea(poi.x, poi.y, g_currentMapArea, u, v);
                if (u < uv0.x || u > uv1.x || v < uv0.y || v > uv1.y) continue;
                float px = canvas_p0.x + (u - uv0.x) / range * mapDisplaySize.x;
                float py = canvas_p0.y + (v - uv0.y) / range * mapDisplaySize.y;
                float dx = io.MousePos.x - px;
                float dy = io.MousePos.y - py;
                if (dx * dx + dy * dy < 12.0f * 12.0f) {
                    g_cheats.teleportToPending = true;
                    g_cheats.teleportToX = poi.x;
                    g_cheats.teleportToY = poi.y;
                    g_cheats.teleportToZ = poi.z;
                    WriteCommands();
                    break;
                }
            }
        }


        // Pal Spawns
        std::string hoveredSpawn;
        if (g_showPalSpawns && g_palSpawnsLoaded && !g_palSpawns.empty()) {
            for (const auto& spawn : g_palSpawns) {
                // Filter by selected pal name
                if (!g_selectedPalName.empty() && spawn.palName != g_selectedPalName) continue;
                // Filter by day/night
                if (!g_spawnFilterDay && spawn.isDay && !spawn.isNight) continue;
                if (!g_spawnFilterNight && spawn.isNight && !spawn.isDay) continue;
                if (!g_spawnFilterDay && !g_spawnFilterNight) continue;
                // Filter by alpha
                if (spawn.isAlpha && !g_spawnFilterAlpha) continue;
                if (!spawn.isAlpha && !g_spawnFilterNonAlpha) continue;
                // Filter by level
                if (spawn.levelMax < g_spawnFilterLevelMin || spawn.levelMin > g_spawnFilterLevelMax) continue;

                int spawnArea = WorldToMapArea(spawn.x, spawn.y);
                if (spawnArea != g_currentMapArea) continue;
                float u = 0.5f, v = 0.5f;
                WorldToMapUVArea(spawn.x, spawn.y, g_currentMapArea, u, v);
                if (u < uv0.x || u > uv1.x || v < uv0.y || v > uv1.y) continue;
                float px = canvas_p0.x + (u - uv0.x) / range * mapDisplaySize.x;
                float py = canvas_p0.y + (v - uv0.y) / range * mapDisplaySize.y;

                ImU32 spawnCol;
                if (spawn.isAlpha) {
                    spawnCol = IM_COL32(220, 50, 50, 200);
                    dl->AddCircleFilled(ImVec2(px, py), 5.0f, spawnCol);
                    dl->AddCircle(ImVec2(px, py), 6.0f, IM_COL32(160, 20, 20, 255), 0, 1.5f);
                } else {
                    // Color by day/night
                    if (spawn.isDay && spawn.isNight) spawnCol = IM_COL32(100, 200, 255, 180);
                    else if (spawn.isDay) spawnCol = IM_COL32(255, 200, 50, 180);
                    else spawnCol = IM_COL32(100, 100, 255, 180);
                    dl->AddCircleFilled(ImVec2(px, py), 3.5f, spawnCol);
                    dl->AddCircle(ImVec2(px, py), 4.5f, IM_COL32(0, 0, 0, 150), 0, 1.0f);
                }

                if (hovered && hoveredSpawn.empty()) {
                    float dx = io.MousePos.x - px;
                    float dy = io.MousePos.y - py;
                    float hitR = spawn.isAlpha ? 10.0f : 8.0f;
                    if (dx * dx + dy * dy < hitR * hitR) {
                        char buf[256];
                        const char* timeStr = (spawn.isDay && spawn.isNight) ? "Jour/Nuit" : (spawn.isDay ? "Jour" : "Nuit");
                        snprintf(buf, sizeof(buf), "%s%s (Lv %d-%d)\n%s\nX:%.0f Y:%.0f",
                            spawn.isAlpha ? "Alpha " : "", spawn.palName.c_str(),
                            spawn.levelMin, spawn.levelMax, timeStr, spawn.x, spawn.y);
                        hoveredSpawn = buf;
                    }
                }
            }
        }

        // Favoris
        std::string hoveredFav;
        for (const auto& fav : g_favorites) {
            int favArea = WorldToMapArea(fav.x, fav.y);
            if (favArea != g_currentMapArea) continue;
            float u = 0.5f, v = 0.5f;
            WorldToMapUVArea(fav.x, fav.y, g_currentMapArea, u, v);
            if (u < uv0.x || u > uv1.x || v < uv0.y || v > uv1.y) continue;
            float px = canvas_p0.x + (u - uv0.x) / range * mapDisplaySize.x;
            float py = canvas_p0.y + (v - uv0.y) / range * mapDisplaySize.y;

            dl->AddCircleFilled(ImVec2(px, py), 7.0f, IM_COL32(255, 200, 0, 255));
            dl->AddCircle(ImVec2(px, py), 8.5f, IM_COL32(180, 140, 0, 255), 0, 2.0f);
            dl->AddText(ImVec2(px - 3, py - 6), IM_COL32(255, 255, 255, 255), "*");

            if (hovered && hoveredFav.empty()) {
                float dx = io.MousePos.x - px;
                float dy = io.MousePos.y - py;
                if (dx * dx + dy * dy < 12.0f * 12.0f) {
                    char buf[256];
                    snprintf(buf, sizeof(buf), "%s\nX:%.0f Y:%.0f Z:%.0f\n[Clic-droit pour supprimer]", fav.label.c_str(), fav.x, fav.y, fav.z);
                    hoveredFav = buf;
                }
            }
        }

        // Pals à proximité
        std::string hoveredPal;
        if (g_showPals && !g_nearbyPals.empty()) {
            int palIdx = 0;
            for (const auto& pal : g_nearbyPals) {
                int palArea = WorldToMapArea(pal.x, pal.y);
                if (palArea != g_currentMapArea) continue;
                float u = 0.5f, v = 0.5f;
                WorldToMapUVArea(pal.x, pal.y, g_currentMapArea, u, v);
                if (u < uv0.x || u > uv1.x || v < uv0.y || v > uv1.y) continue;
                float px = canvas_p0.x + (u - uv0.x) / range * mapDisplaySize.x;
                float py = canvas_p0.y + (v - uv0.y) / range * mapDisplaySize.y;

                // Draw pal as orange circle with subtle pulse
                float palPulse = 0.5f + 0.5f * std::sin(g_animTime * 4.0f + (float)palIdx * 0.5f);
                float palR = 3.5f + palPulse * 1.5f;
                ImU32 palColor = IM_COL32(255, 140, 30, 220);
                ImU32 palGlow = IM_COL32(255, 140, 30, 30);
                ImU32 palBorder = IM_COL32(180, 80, 10, 255);
                if (pal.isAlpha) {
                    palR *= 1.8f;
                    palColor = IM_COL32(220, 50, 50, 240);
                    palGlow = IM_COL32(220, 50, 50, 50);
                    palBorder = IM_COL32(160, 20, 20, 255);
                }
                dl->AddCircleFilled(ImVec2(px, py), palR + 2.0f, palGlow);
                dl->AddCircleFilled(ImVec2(px, py), palR, palColor);
                dl->AddCircle(ImVec2(px, py), palR + 1.0f, palBorder, 0, 1.5f);
                if (pal.isAlpha) {
                    // Crown/star indicator for alpha
                    dl->AddText(ImVec2(px - 4, py - palR - 10), IM_COL32(255, 200, 0, 255), "A");
                }

                if (hovered && hoveredPal.empty()) {
                    float dx = io.MousePos.x - px;
                    float dy = io.MousePos.y - py;
                    float hitR = pal.isAlpha ? 14.0f : 10.0f;
                    if (dx * dx + dy * dy < hitR * hitR) {
                        float dist = std::sqrt((pal.x - g_player.x) * (pal.x - g_player.x) +
                                               (pal.y - g_player.y) * (pal.y - g_player.y));
                        char buf[256];
                        if (pal.isAlpha) {
                            snprintf(buf, sizeof(buf), "Alpha Pal (Lv %d)\nX:%.0f Y:%.0f Z:%.0f\nDistance: %.0fm", pal.level, pal.x, pal.y, pal.z, dist);
                        } else if (pal.level > 0) {
                            snprintf(buf, sizeof(buf), "Pal (Lv %d)\nX:%.0f Y:%.0f Z:%.0f\nDistance: %.0fm", pal.level, pal.x, pal.y, pal.z, dist);
                        } else {
                            snprintf(buf, sizeof(buf), "Pal\nX:%.0f Y:%.0f Z:%.0f\nDistance: %.0fm", pal.x, pal.y, pal.z, dist);
                        }
                        hoveredPal = buf;
                    }
                }
                palIdx++;
            }
        }

        // ESP lines from player to pals
        if (g_showEspLines && g_showPals && !g_nearbyPals.empty() && g_player.valid) {
            int playerArea = WorldToMapArea(g_player.x, g_player.y);
            if (playerArea == g_currentMapArea) {
                float pu = 0.5f, pv = 0.5f;
                WorldToMapUVArea(g_player.x, g_player.y, g_currentMapArea, pu, pv);
                float ppx = canvas_p0.x + (pu - uv0.x) / range * mapDisplaySize.x;
                float ppy = canvas_p0.y + (pv - uv0.y) / range * mapDisplaySize.y;

                for (const auto& pal : g_nearbyPals) {
                    int palArea = WorldToMapArea(pal.x, pal.y);
                    if (palArea != g_currentMapArea) continue;
                    float u = 0.5f, v = 0.5f;
                    WorldToMapUVArea(pal.x, pal.y, g_currentMapArea, u, v);
                    if (u < uv0.x || u > uv1.x || v < uv0.y || v > uv1.y) continue;
                    float px = canvas_p0.x + (u - uv0.x) / range * mapDisplaySize.x;
                    float py = canvas_p0.y + (v - uv0.y) / range * mapDisplaySize.y;

                    float dx = pal.x - g_player.x, dy = pal.y - g_player.y;
                    float dist = std::sqrt(dx * dx + dy * dy);
                    ImU32 lineColor;
                    if (dist < 500.0f) lineColor = IM_COL32(60, 255, 60, 120);
                    else if (dist < 1500.0f) lineColor = IM_COL32(255, 255, 60, 100);
                    else lineColor = IM_COL32(255, 60, 60, 80);
                    dl->AddLine(ImVec2(ppx, ppy), ImVec2(px, py), lineColor, 1.0f);

                    if (g_showEspNames) {
                        char nameBuf[32];
                        if (pal.isAlpha)
                            snprintf(nameBuf, sizeof(nameBuf), "Alpha Lv%d", pal.level);
                        else
                            snprintf(nameBuf, sizeof(nameBuf), "Lv%d", pal.level);
                        dl->AddText(ImVec2(px + 6, py - 6), IM_COL32(255, 255, 255, 200), nameBuf);
                    }
                }
            }
        }

        // World entities (coffres, oeufs, effigies, skillfruits)
        std::string hoveredEntity;
        if (g_showWorldEntities && !g_worldEntities.empty()) {
            for (const auto& ent : g_worldEntities) {
                int entArea = WorldToMapArea(ent.x, ent.y);
                if (entArea != g_currentMapArea) continue;
                float u = 0.5f, v = 0.5f;
                WorldToMapUVArea(ent.x, ent.y, g_currentMapArea, u, v);
                if (u < uv0.x || u > uv1.x || v < uv0.y || v > uv1.y) continue;
                float px = canvas_p0.x + (u - uv0.x) / range * mapDisplaySize.x;
                float py = canvas_p0.y + (v - uv0.y) / range * mapDisplaySize.y;

                ImU32 entColor = GetEntityColor(ent.entityType);
                // Draw as diamond shape
                float r = 5.0f;
                dl->AddCircleFilled(ImVec2(px, py), r + 2.0f, IM_COL32((entColor >> 24) & 0xFF, (entColor >> 16) & 0xFF, (entColor >> 8) & 0xFF, 30));
                dl->AddQuadFilled(
                    ImVec2(px, py - r),
                    ImVec2(px + r, py),
                    ImVec2(px, py + r),
                    ImVec2(px - r, py),
                    entColor
                );
                dl->AddQuad(
                    ImVec2(px, py - r),
                    ImVec2(px + r, py),
                    ImVec2(px, py + r),
                    ImVec2(px - r, py),
                    IM_COL32(0, 0, 0, 200), 1.5f
                );

                if (hovered && hoveredEntity.empty()) {
                    float dx = io.MousePos.x - px;
                    float dy = io.MousePos.y - py;
                    if (dx * dx + dy * dy < 12.0f * 12.0f) {
                        float dist = std::sqrt((ent.x - g_player.x) * (ent.x - g_player.x) +
                                               (ent.y - g_player.y) * (ent.y - g_player.y));
                        char buf[256];
                        snprintf(buf, sizeof(buf), "%s\nX:%.0f Y:%.0f Z:%.0f\nDistance: %.0fm",
                                 GetEntityTypeName(ent.entityType), ent.x, ent.y, ent.z, dist);
                        hoveredEntity = buf;
                    }
                }
            }
        }

        // Joueur
        if (g_player.valid) {
            float u = 0.5f, v = 0.5f;
            WorldToMapUVArea(g_player.x, g_player.y, g_currentMapArea, u, v);
            float px = canvas_p0.x + (u - uv0.x) / range * mapDisplaySize.x;
            float py = canvas_p0.y + (v - uv0.y) / range * mapDisplaySize.y;
            if (px >= canvas_p0.x - 8 && px <= canvas_p1.x + 8 && py >= canvas_p0.y - 8 && py <= canvas_p1.y + 8) {
                // Pulsing outer ring
                float pulse = 0.5f + 0.5f * std::sin(g_animTime * 3.0f);
                float ringR = 10.0f + pulse * 6.0f;
                int ringAlpha = (int)(120 * (1.0f - pulse * 0.5f));
                dl->AddCircle(ImVec2(px, py), ringR, IM_COL32(0, 255, 0, ringAlpha), 0, 2.0f);
                // Glow
                dl->AddCircleFilled(ImVec2(px, py), 9.0f, IM_COL32(0, 255, 0, 40));
                // Core
                dl->AddCircleFilled(ImVec2(px, py), 6.0f, IM_COL32(0, 255, 0, 255));
                dl->AddCircle(ImVec2(px, py), 8.0f, IM_COL32(255, 255, 255, 255), 0, 2.0f);
            }
        }

        if (!hoveredPoi.empty()) {
            ImGui::SetTooltip("%s", hoveredPoi.c_str());
        }
        if (!hoveredSpawn.empty()) {
            ImGui::SetTooltip("%s", hoveredSpawn.c_str());
        }
        if (!hoveredPal.empty()) {
            ImGui::SetTooltip("%s", hoveredPal.c_str());
        }
        if (!hoveredEntity.empty()) {
            ImGui::SetTooltip("%s", hoveredEntity.c_str());
        }
        if (!hoveredFav.empty()) {
            ImGui::SetTooltip("%s", hoveredFav.c_str());
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && hovered) {
                std::string idToRemove;
                for (const auto& fav : g_favorites) {
                    int favArea = WorldToMapArea(fav.x, fav.y);
                    if (favArea != g_currentMapArea) continue;
                    float u = 0.5f, v = 0.5f;
                    WorldToMapUVArea(fav.x, fav.y, g_currentMapArea, u, v);
                    float px = canvas_p0.x + (u - uv0.x) / range * mapDisplaySize.x;
                    float py = canvas_p0.y + (v - uv0.y) / range * mapDisplaySize.y;
                    float dx = io.MousePos.x - px;
                    float dy = io.MousePos.y - py;
                    if (dx * dx + dy * dy < 12.0f * 12.0f) {
                        idToRemove = fav.id;
                        break;
                    }
                }
                if (!idToRemove.empty()) RemoveFavorite(idToRemove);
            }
        }

        // Overlay coordonnées joueur sur la carte (coin bas-gauche)
        if (g_player.valid) {
            const char* coordFmt = "X:%.0f Y:%.0f Z:%.0f";
            char coordText[128];
            snprintf(coordText, sizeof(coordText), coordFmt, g_player.x, g_player.y, g_player.z);
            ImVec2 textSize = ImGui::CalcTextSize(coordText);
            float pad = 6.0f;
            ImVec2 boxMin(canvas_p0.x + 4, canvas_p1.y - textSize.y - pad * 2 - 4);
            ImVec2 boxMax(canvas_p0.x + textSize.x + pad * 2 + 4, canvas_p1.y - 4);
            dl->AddRectFilled(boxMin, boxMax, IM_COL32(0, 0, 0, 140), 4.0f);
            dl->AddText(ImVec2(boxMin.x + pad, boxMin.y + pad), IM_COL32(0, 255, 100, 230), coordText);
        }

        // Overlay compteur de pals (coin haut-droit)
        if (g_showPals && !g_nearbyPals.empty()) {
            char palText[64];
            snprintf(palText, sizeof(palText), "%d pals", (int)g_nearbyPals.size());
            ImVec2 textSize = ImGui::CalcTextSize(palText);
            float pad = 6.0f;
            ImVec2 boxMin(canvas_p1.x - textSize.x - pad * 2 - 4, canvas_p0.y + 4);
            ImVec2 boxMax(canvas_p1.x - 4, canvas_p0.y + textSize.y + pad * 2 + 4);
            dl->AddRectFilled(boxMin, boxMax, IM_COL32(0, 0, 0, 140), 4.0f);
            dl->AddText(ImVec2(boxMin.x + pad, boxMin.y + pad), IM_COL32(255, 140, 30, 230), palText);
        }

        // Overlay compteur d'entites (coin haut-droit, sous pals)
        if (g_showWorldEntities && !g_worldEntities.empty()) {
            char entText[64];
            snprintf(entText, sizeof(entText), "%d entités", (int)g_worldEntities.size());
            ImVec2 textSize = ImGui::CalcTextSize(entText);
            float pad = 6.0f;
            float yOffset = (g_showPals && !g_nearbyPals.empty()) ? 28.0f : 4.0f;
            ImVec2 boxMin(canvas_p1.x - textSize.x - pad * 2 - 4, canvas_p0.y + yOffset);
            ImVec2 boxMax(canvas_p1.x - 4, canvas_p0.y + yOffset + textSize.y + pad * 2);
            dl->AddRectFilled(boxMin, boxMax, IM_COL32(0, 0, 0, 140), 4.0f);
            dl->AddText(ImVec2(boxMin.x + pad, boxMin.y + pad), IM_COL32(255, 200, 0, 230), entText);
        }

    } else {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
        ImGui::TextWrapped("TEXTURE DE CARTE INTROUVABLE !");
        ImGui::PopStyleColor();
        ImGui::Spacing();
        ImGui::TextDisabled("Dossier de l'application :");
        ImGui::TextWrapped("%s", g_dataDir.c_str());
        ImGui::Spacing();
        ImGui::TextDisabled("Fichiers attendus :");
        ImGui::TextWrapped("%s/assets/maps/map_2048.rgba", g_dataDir.c_str());
        ImGui::TextWrapped("%s/assets/maps/map_tree_2048.rgba", g_dataDir.c_str());
        ImGui::Spacing();
        ImGui::PushTextWrapPos(0.0f);
        ImGui::TextWrapped("Verifiez que vous avez extrait l'archive complete (PalTrainerUltra/) et lancez l'application depuis ce dossier.");
        ImGui::PopTextWrapPos();
    }


    if (g_appMode == AppMode::Overlay) {
    // Filtres et favoris intégrés (mode trainer)
    if (ImGui::CollapsingHeader("Filtres de carte")) {
        if (ImGui::BeginTable("filters", 2, ImGuiTableFlags_SizingStretchSame)) {
            for (auto& kv : g_poiFilter) {
                ImGui::TableNextColumn();
                const char* label = GetPoiLabelFr(kv.first);
                ImGui::Checkbox(label, &kv.second);
            }
            ImGui::EndTable();
        }
    }

    if (!g_favorites.empty() && ImGui::CollapsingHeader("Favoris")) {
        for (int i = (int)g_favorites.size() - 1; i >= 0; --i) {
            ImGui::PushID(i);
            ImGui::Text("%s", g_favorites[i].label.c_str());
            ImGui::SameLine();
            ImGui::TextDisabled("X:%.0f Y:%.0f", g_favorites[i].x, g_favorites[i].y);
            ImGui::SameLine();
            if (ImGui::Button("Aller")) {
                g_minimapFollowPlayer = false;
                g_currentMapArea = WorldToMapArea(g_favorites[i].x, g_favorites[i].y);
                float u = 0.5f, v = 0.5f;
                WorldToMapUVArea(g_favorites[i].x, g_favorites[i].y, g_currentMapArea, u, v);
                g_minimapCenter = ImVec2(u, v);
                g_minimapZoom = std::max(g_minimapZoom, 4.0f);
                ClampMinimapView();
            }
            ImGui::SameLine();
            if (ImGui::Button("Suppr")) {
                RemoveFavorite(g_favorites[i].id);
            }
            ImGui::PopID();
        }
    }
    }
    } // end else (Overlay mode)

    // Popup pour ajouter un favori (commun aux deux modes)
    if (g_showAddFavoritePopup) {
        ImGui::OpenPopup("Ajouter un favori");
        g_showAddFavoritePopup = false;
    }
    if (ImGui::BeginPopup("Ajouter un favori")) {
        ImGui::Text("Nom du favori :");
        ImGui::InputText("##favname", g_favoriteNameBuf, sizeof(g_favoriteNameBuf));
        ImGui::SameLine();
        if (g_player.valid) {
            if (ImGui::Button("OK") || ImGui::IsKeyPressed(ImGuiKey_Enter)) {
                AddFavorite(g_favoriteNameBuf, g_player.x, g_player.y, g_player.z);
                g_favoriteNameBuf[0] = '\0';
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Annuler")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::TextDisabled("Position: X:%.0f Y:%.0f Z:%.0f", g_player.x, g_player.y, g_player.z);
        } else {
            ImGui::TextDisabled("Joueur non localisé. Lancez Palworld et attendez la connexion.");
            if (ImGui::Button("Annuler")) {
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
    }

    ImGui::PopStyleVar();
    ImGui::End();
}
