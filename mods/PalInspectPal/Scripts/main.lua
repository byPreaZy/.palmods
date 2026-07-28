--[[
    PalInspectPal
    Affiche les informations du Pal vise (nom, niveau, HP, etc.).
--]]

local MOD_NAME = "PalInspectPal"
local function log(msg) print("[" .. MOD_NAME .. "] " .. tostring(msg)) end
local function err(msg) print("[" .. MOD_NAME .. " ERREUR] " .. tostring(msg)) end

-- ============================================================================
-- CHEMIN ET CONFIG
-- ============================================================================

local function getModDir()
    local src = debug and debug.getinfo and debug.getinfo(1, "S").source
    if not src then return "." end
    src = tostring(src):gsub("^@", "")
    src = src:gsub("\\", "/")
    local dir = src:match("^(.*)/Scripts/main%.lua$")
    if not dir then dir = "." end
    return dir
end

local MOD_DIR = getModDir()
local CONFIG_PATH = MOD_DIR .. "/Scripts/config.lua"

local Config = nil
local function loadConfig()
    local chunk, e = loadfile(CONFIG_PATH)
    if chunk then
        local ok, cfg = pcall(chunk)
        if ok and type(cfg) == "table" then Config = cfg; return end
    end
    Config = {
        modName = MOD_NAME,
        enabled = true,
        keybind = "HOME",
        showWindow = true,
        windowWidth = 350,
        windowHeight = 200,
        scanRadius = 5000,
        intervalMs = 250,
    }
    err("config.lua invalide, valeurs par defaut chargees.")
end

-- ============================================================================
-- UTILITAIRES
-- ============================================================================

local function isValid(obj)
    if obj == nil then return false end
    local t = type(obj)
    if t ~= "table" and t ~= "userdata" then return true end
    if t == "userdata" then
        local ok, valid = pcall(function() return obj:IsValid() end)
        if ok then return valid end
        return true
    end
    if obj.IsValid == nil then return true end
    local ok, valid = pcall(function() return obj:IsValid() end)
    return ok and valid
end

local function safeGet(obj, path)
    if not obj then return nil end
    local current = obj
    for part in string.gmatch(path, "[^.]+") do
        local ok, nextObj = pcall(function() return current[part] end)
        if not ok or nextObj == nil then return nil end
        current = nextObj
    end
    return current
end

local function keyFromString(keyName)
    if not keyName or not Key then return nil end
    if Key[keyName] then return Key[keyName] end
    local upper = string.upper(keyName)
    if Key[upper] then return Key[upper] end
    local aliases = {
        PAGEUP = "PageUp", PAGEDOWN = "PageDown", DEL = "Delete", INS = "Insert",
        ENTER = "Return", RET = "Return", ESC = "Escape", SPACE = "SpaceBar",
        NUM0 = "NumPadZero", NUM1 = "NumPadOne", NUM2 = "NumPadTwo", NUM3 = "NumPadThree",
        NUM4 = "NumPadFour", NUM5 = "NumPadFive", NUM6 = "NumPadSix",
        NUM7 = "NumPadSeven", NUM8 = "NumPadEight", NUM9 = "NumPadNine",
    }
    local a = aliases[upper]
    if a and Key[a] then return Key[a] end
    return nil
end

local function getPlayerCharacter()
    local pc = FindFirstOf and FindFirstOf("PlayerController")
    if isValid(pc) then
        local char = safeGet(pc, "Pawn") or safeGet(pc, "Character")
        if isValid(char) then return char end
    end
    local names = { "PalPlayerCharacter", "BP_PlayerCharacter_Female_C", "BP_PlayerCharacter_Male_C", "PlayerCharacter" }
    for _, n in ipairs(names) do
        local obj = FindFirstOf and FindFirstOf(n)
        if isValid(obj) then return obj end
    end
    return nil
end

local function getActorLocation(actor)
    if not isValid(actor) then return nil end
    local ok, loc = pcall(function() return actor:GetActorLocation() end)
    if ok and loc and loc.X ~= nil then return loc end
    return nil
end

local function distance3D(a, b)
    if not a or not b then return nil end
    local dx = (a.X or 0) - (b.X or 0)
    local dy = (a.Y or 0) - (b.Y or 0)
    local dz = (a.Z or 0) - (b.Z or 0)
    return math.sqrt(dx * dx + dy * dy + dz * dz)
end

-- ============================================================================
-- INSPECTION
-- ============================================================================

local State = { show = true, current = nil, name = "", level = "", hp = "", maxHp = "", sp = "", maxSp = "", gender = "", element = "" }

local function getCharacterName(char)
    if not isValid(char) then return nil end
    local name = safeGet(char, "CharacterID") or safeGet(char, "Name") or safeGet(char, "NickName") or safeGet(char, "NameText")
    if name and tostring(name) ~= "" then return tostring(name) end
    local fn = safeGet(char, "GetFullName")
    if fn and type(fn) == "function" then
        local ok, res = pcall(function() return char:GetFullName() end)
        if ok and res then return tostring(res):match("[^.]+$") or tostring(res) end
    end
    return nil
end

local function getParam(char, path)
    local param = safeGet(char, "CharacterParameterComponent") or safeGet(char, "ParameterComponent") or safeGet(char, "Parameter")
    if not isValid(param) then return nil end
    return safeGet(param, path)
end

local function findNearestPal()
    local player = getPlayerCharacter()
    if not isValid(player) then return nil end
    local pLoc = getActorLocation(player)
    if not pLoc then return nil end

    local best = nil
    local bestDist = (Config.scanRadius or 5000) + 1
    local classNames = { "PalCharacter", "PalMonsterCharacter", "BP_PalMonsterCharacter_C" }
    for _, className in ipairs(classNames) do
        local all = FindAllOf and FindAllOf(className)
        if all then
            for _, char in ipairs(all) do
                if isValid(char) and char ~= player then
                    local loc = getActorLocation(char)
                    if loc then
                        local d = distance3D(pLoc, loc)
                        if d and d < bestDist then
                            best = char
                            bestDist = d
                        end
                    end
                end
            end
        end
    end
    return best
end

local function updateInfo()
    local pal = findNearestPal()
    if not isValid(pal) then
        State.current = nil
        State.name = ""
        State.level = ""
        State.hp = ""
        State.maxHp = ""
        State.sp = ""
        State.maxSp = ""
        State.gender = ""
        State.element = ""
        return
    end
    State.current = pal
    State.name = getCharacterName(pal) or "Inconnu"
    State.level = tostring(safeGet(pal, "Level") or getParam(pal, "Level") or "?")
    local hp = getParam(pal, "HP") or safeGet(pal, "HP")
    local maxHp = getParam(pal, "MaxHP") or safeGet(pal, "MaxHP")
    local sp = getParam(pal, "SP") or safeGet(pal, "SP") or getParam(pal, "MP")
    local maxSp = getParam(pal, "MaxSP") or safeGet(pal, "MaxSP") or getParam(pal, "MaxMP")
    State.hp = tostring(hp or "?")
    State.maxHp = tostring(maxHp or "?")
    State.sp = tostring(sp or "?")
    State.maxSp = tostring(maxSp or "?")
    local gender = safeGet(pal, "Gender") or getParam(pal, "Gender")
    State.gender = tostring(gender or "?")
    local element = safeGet(pal, "Element") or getParam(pal, "ElementType")
    State.element = tostring(element or "?")
end

local function draw()
    if not Config.showWindow or not ImGui or not ImGui.Begin then return end
    local ImGuiCond_FirstUseEver = 4
    local ok = pcall(function()
        ImGui.SetNextWindowSize(Config.windowWidth or 350, Config.windowHeight or 200, ImGuiCond_FirstUseEver)
        ImGui.Begin("Pal Inspect", true)
    end)
    if not ok then return end

    if State.name and State.name ~= "" then
        ImGui.Text("Nom : " .. State.name)
        ImGui.Text("Niveau : " .. State.level)
        ImGui.Text("HP : " .. State.hp .. " / " .. State.maxHp)
        ImGui.Text("SP : " .. State.sp .. " / " .. State.maxSp)
        ImGui.Text("Sexe : " .. State.gender)
        ImGui.Text("Element : " .. State.element)
    else
        ImGui.Text("Aucun Pal detecte a proximite.")
    end

    ImGui.End()
end

local function onToggle()
    Config.showWindow = not Config.showWindow
    log("Fenetre " .. (Config.showWindow and "visible" or "masquee"))
end

-- ============================================================================
-- INITIALISATION
-- ============================================================================

loadConfig()
State.show = Config.showWindow

pcall(function()
    local k = keyFromString(Config.keybind)
    if k and RegisterKeyBind then
        RegisterKeyBind(k, function()
            pcall(onToggle)
        end)
    end
end)

LoopAsync(Config.intervalMs or 250, function()
    pcall(function()
        local gcfg = _G.PalModManagerConfig and _G.PalModManagerConfig[MOD_NAME]
        if gcfg then Config = gcfg end
        if Config.enabled then updateInfo() end
        draw()
    end)
    return true
end)

log("charge avec succes. Appuyez sur " .. (Config.keybind or "F5") .. " pour basculer la fenetre.")


-- =============================================================================
-- INTEGRATION PALTRAINER APP EXTERNE
-- =============================================================================

pcall(function()
    local ipc_ok, ipc = pcall(require, "PalTrainerIPC")
    if not ipc_ok or not ipc then return end
    ipc.ensureDir()
    local commandKey = "mod." .. MOD_NAME
    local lastCmd = nil
    local lastEnabled = nil

    local function pollIPC()
        if not Config then return end
        local cmds = ipc.pollCommands()
        local val = ipc.getCommand(cmds, commandKey)
        if val ~= nil then
            if val ~= lastCmd then
                lastCmd = val
                if Config.enabled ~= val then
                    Config.enabled = val
                end
            elseif Config.enabled ~= lastEnabled then
                -- keybind or in-game toggle changed state, accept it
            end
        end
        lastEnabled = Config.enabled
        ipc.writeStatus({ [commandKey] = Config.enabled })
    end

    if LoopInGameThreadWithDelay and type(LoopInGameThreadWithDelay) == "function" then
        LoopInGameThreadWithDelay(500, pollIPC)
    else
        LoopAsync(500, function() pollIPC(); return false end)
    end
end)
