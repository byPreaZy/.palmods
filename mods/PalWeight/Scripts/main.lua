--[[
    PalWeight
    Force le poids maximum du joueur pour simuler un poids infini / sac augmente.
--]]

local MOD_NAME = "PalWeight"
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
        if ok and type(cfg) == "table" then
            Config = cfg
            return
        end
    end
    Config = {
        modName = MOD_NAME,
        enabled = true,
        keybind = "SCROLL_LOCK",
        maxWeight = 999999,
        resetWeight = true,
        intervalMs = 500,
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

local function safeSet(obj, path, value)
    if not isValid(obj) then return false end
    local current = obj
    local parts = {}
    for part in string.gmatch(path, "[^.]+") do
        table.insert(parts, part)
    end
    for i = 1, #parts - 1 do
        local ok, nextObj = pcall(function() return current[parts[i]] end)
        if not ok or not isValid(nextObj) then return false end
        current = nextObj
    end
    local ok, cur = pcall(function() return current[parts[#parts]] end)
    if not ok or cur == nil then return false end
    local ok = pcall(function() current[parts[#parts]] = value end)
    return ok
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

local function getPlayerState()
    local classes = {"APalPlayerState", "PalPlayerState", "PlayerState"}
    for _, cls in ipairs(classes) do
        local ps = FindFirstOf and FindFirstOf(cls)
        if isValid(ps) then return ps end
        local all = FindAllOf and FindAllOf(cls)
        if all then
            for _, p in ipairs(all) do if isValid(p) then return p end end
        end
    end
    local pc = FindFirstOf and FindFirstOf("PalPlayerController")
    if isValid(pc) then return safeGet(pc, "PlayerState") end
    return nil
end

local function applyWeight()
    local gcfg = _G.PalModManagerConfig and _G.PalModManagerConfig[MOD_NAME]
    if gcfg then Config = gcfg end
    if not Config or not Config.enabled then return end

    local ps = getPlayerState()
    if not isValid(ps) then return end

    -- Chemin correct en 1.0 : UPalPlayerInventoryData
    local inv = safeGet(ps, "InventoryData")
    if isValid(inv) then
        local props = {"maxInventoryWeight", "maxInventoryWeight_Cached", "PassiveBuffedMaxWeight"}
        for _, prop in ipairs(props) do
            safeSet(inv, prop, Config.maxWeight or 999999)
        end
        if Config.resetWeight then
            safeSet(inv, "NowItemWeight", 0)
        end
    end
end

local function onToggle()
    if not Config then loadConfig() end
    Config.enabled = not Config.enabled
    log((Config.enabled and "ACTIF" or "INACTIF"))
end

-- ============================================================================
-- INITIALISATION
-- ============================================================================

loadConfig()

pcall(function()
    local k = keyFromString(Config.keybind)
    if k and RegisterKeyBind then
        RegisterKeyBind(k, function()
            pcall(onToggle)
        end)
    end
end)

-- execution sur le thread jeu (experimental-latest)
local function safeApplyWeight()
    local ok, e = pcall(applyWeight)
    if not ok then
        log("Erreur applyWeight: " .. tostring(e))
    end
end

if LoopInGameThreadWithDelay and type(LoopInGameThreadWithDelay) == "function" then
    LoopInGameThreadWithDelay(Config.intervalMs or 500, safeApplyWeight)
else
    LoopAsync(Config.intervalMs or 500, function()
        safeApplyWeight()
        return false
    end)
end

log("charge avec succes. Appuyez sur " .. (Config.keybind or "F2") .. " pour activer/desactiver.")


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
