--[[
    PalInstantFish
    Force la reussite / simplifie le mini-jeu de peche.
--]]

local MOD_NAME = "PalInstantFish"
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
        keybind = "PAGE_UP",
        autoReel = true,
        autoHook = true,
        intervalMs = 100,
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

local function forceSuccess(comp)
    if not isValid(comp) then return end
    -- Forcer les etats vers le succes (noms 1.0 + fallback)
    safeSet(comp, "CurrentFishingState", 3)
    safeSet(comp, "ReelProgress", 1.0)
    safeSet(comp, "FishHP", 0)
    safeSet(comp, "FishStamina", 0)
    safeSet(comp, "FishEnergy", 0)
    safeSet(comp, "bIsHooked", true)
    safeSet(comp, "bFishingSuccess", true)
    safeSet(comp, "FishingResult", 1)
    safeSet(comp, "Tension", 0.0)
    safeSet(comp, "MinigameProgress", 1.0)
    safeSet(comp, "AutoReel", true)
    safeSet(comp, "AutoHook", true)
    local ok, fn = pcall(function() return comp["SuccessFishing"] or comp["OnFishingSuccess"] or comp["FinishFishing"] or comp["RequestSuccess"] end)
    if ok and fn and type(fn) == "function" then
        pcall(function() fn(comp) end)
    end
end

-- ============================================================================
-- HOOKS ET BOUCLE
-- ============================================================================

local function registerSafeHook(path, callback)
    if not RegisterHook then return end
    local ok, why = pcall(function() RegisterHook(path, callback) end)
    if not ok then err("Hook ignore : " .. tostring(path) .. " : " .. tostring(why)) end
end

-- Les hooks sur UFunctions /Script/Pal.* sont obsoletes en 1.0
local function installFishingHooks()
end

local function loopFishing()
    local gcfg = _G.PalModManagerConfig and _G.PalModManagerConfig[MOD_NAME]
    if gcfg then Config = gcfg end
    if not Config or not Config.enabled then return end

    -- Recherche du composant de peche sur le joueur (classes 1.0 + fallback)
    local player = getPlayerCharacter()

    local classNames = {
        "PalFishingComponent",
        "BP_PalFishingComponent_C",
        "PalPlayerFishingComponent",
        "BP_PalPlayerFishingComponent_C",
    }
    for _, className in ipairs(classNames) do
        local all = FindAllOf and FindAllOf(className)
        if all then
            for _, comp in ipairs(all) do
                if isValid(comp) then
                    local owner = safeGet(comp, "Owner")
                    if owner and owner == player or true then
                        forceSuccess(comp)
                    end
                end
            end
        end
    end

    -- Recherche d'acteur de peche proche
    local actorNames = { "BP_FishingPoint_C", "PalFishingPoint", "PalFishingPointActor", "BP_FishingPointActor_C" }
    for _, actorName in ipairs(actorNames) do
        local actors = FindAllOf and FindAllOf(actorName)
        if actors then
            for _, actor in ipairs(actors) do
                if isValid(actor) then
                    safeSet(actor, "bAutoSuccess", true)
                    safeSet(actor, "AutoSuccess", true)
                end
            end
        end
    end
end

local function onToggle()
    Config.enabled = not Config.enabled
    log((Config.enabled and "ACTIF" or "INACTIF"))
end

-- ============================================================================
-- INITIALISATION
-- ============================================================================

loadConfig()
installFishingHooks()

pcall(function()
    local k = keyFromString(Config.keybind)
    if k and RegisterKeyBind then
        RegisterKeyBind(k, function()
            pcall(onToggle)
        end)
    end
end)

LoopAsync(Config.intervalMs or 100, function()
    pcall(loopFishing)
    return true
end)

log("charge avec succes. Appuyez sur " .. (Config.keybind or "F8") .. " pour activer/desactiver.")


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
