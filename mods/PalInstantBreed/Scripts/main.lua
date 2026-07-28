--[[
    PalInstantBreed
    Force la reproduction instantanee a la ferme de reproduction.
--]]

local MOD_NAME = "PalInstantBreed"
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
    Config = { modName = MOD_NAME, enabled = true, keybind = "END", intervalMs = 500 }
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

local function tryCall(obj, methods, ...)
    if not isValid(obj) then return false end
    local args = {...}
    local n = select("#", ...)
    for _, m in ipairs(methods) do
        local ok, fn = pcall(function() return obj[m] end)
        if ok and type(fn) == "function" then
            local ok2, res = pcall(function() return fn(obj, table.unpack(args, 1, n)) end)
            if ok2 then return true, res end
        end
    end
    return false
end

-- ============================================================================
-- REPRODUCTION
-- ============================================================================

local function completeBreed(farm)
    if not isValid(farm) then return end
    -- 1.0 : UPalMapObjectBreedFarmModel / UPalMapObjectBreedFarmParameterComponent
    local required = safeGet(farm, "BreedRequiredRealTime") or 0.01
    safeSet(farm, "BreedProgressTime", required)
    safeSet(farm, "BreedRequiredRealTime", 0.01)
    -- Force le "Rep" du modele pour que le jeu valide la progression
    tryCall(farm, { "OnRep_UpdateBreedProgress", "CanProceedBreeding" })
end

local function processFarms()
    local gcfg = _G.PalModManagerConfig and _G.PalModManagerConfig[MOD_NAME]
    if gcfg then Config = gcfg end
    if not Config or not Config.enabled then return end

    -- Classes 1.0 : modele concret + acteur + anciens BP (fallback)
    local classNames = {
        "PalMapObjectBreedFarmModel",
        "PalMapObjectBreedFarmParameterComponent",
        "PalBuildObjectBreedFarm",
        "BP_BuildObject_BreedFarm_C",
        "BP_PalBuildObjectBreedFarm_C",
    }
    for _, className in ipairs(classNames) do
        local all = FindAllOf and FindAllOf(className)
        if all then
            for _, farm in ipairs(all) do
                if isValid(farm) then
                    completeBreed(farm)
                end
            end
        end
    end
end

local function registerSafeHook(path, callback)
    if not RegisterHook then return end
    local ok, why = pcall(function() RegisterHook(path, callback) end)
    if not ok then err("Hook ignore : " .. tostring(path) .. " : " .. tostring(why)) end
end

-- Les hooks sur UFunctions /Script/Pal.* sont obsoletes en 1.0
local function installBreedHooks()
end

local function onToggle()
    Config.enabled = not Config.enabled
    log((Config.enabled and "ACTIF" or "INACTIF"))
end

-- ============================================================================
-- INITIALISATION
-- ============================================================================

loadConfig()
installBreedHooks()

pcall(function()
    local k = keyFromString(Config.keybind)
    if k and RegisterKeyBind then
        RegisterKeyBind(k, function()
            pcall(onToggle)
        end)
    end
end)

LoopAsync(Config.intervalMs or 500, function()
    pcall(processFarms)
    return true
end)

log("charge avec succes. Appuyez sur " .. (Config.keybind or "INSERT") .. " pour activer/desactiver.")


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
