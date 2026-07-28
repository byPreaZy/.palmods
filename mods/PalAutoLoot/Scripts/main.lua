--[[
    PalAutoLoot
    Ramasse automatiquement les objets tombes dans un rayon autour du joueur.
--]]

local MOD_NAME = "PalAutoLoot"
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
        enabled = false,
        keybind = "CAPS_LOCK",
        radius = 1500,
        intervalMs = 1000,
        classes = "PalItemPickup,PalDropItem,PalDroppedItem,PalPickupObject,PalItemActor,BP_PalItemActor_C,PalLootBox",
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
-- RAMASSAGE
-- ============================================================================

local touched = {}
local lastPrune = 0

local function parseClassList(str)
    local list = {}
    if not str then return list end
    for s in string.gmatch(str, "([^,]+)") do
        s = s:gsub("^%s+", ""):gsub("%s+$", "")
        if s ~= "" then table.insert(list, s) end
    end
    return list
end

local function tryPickup(obj, player)
    if not isValid(obj) then return false end
    local ok = pcall(function()
        if obj.PickUp then obj:PickUp(player) end
        if obj.Pickup then obj:Pickup(player) end
        if obj.OnPickup then obj:OnPickup(player) end
        if obj.OnInteract then obj:OnInteract(player) end
        if obj.Interact then obj:Interact(player) end
    end)
    return ok
end

local function autoLoot()
    local gcfg = _G.PalModManagerConfig and _G.PalModManagerConfig[MOD_NAME]
    if gcfg then Config = gcfg end
    if not Config or not Config.enabled then return end

    local player = getPlayerCharacter()
    if not isValid(player) then return end
    local pLoc = getActorLocation(player)
    if not pLoc then return end

    local now = os.time and os.time() or 0
    if now - lastPrune > 30 then
        touched = {}
        lastPrune = now
    end

    local radius = Config.radius or 1500
    local radiusSq = radius * radius
    local classes = parseClassList(Config.classes)
    local picked = 0

    for _, className in ipairs(classes) do
        local all = FindAllOf and FindAllOf(className)
        if all then
            for _, obj in ipairs(all) do
                if isValid(obj) and not touched[obj] then
                    local loc = getActorLocation(obj)
                    if loc then
                        local d = distance3D(pLoc, loc)
                        if d and d <= radius then
                            if tryPickup(obj, player) then
                                touched[obj] = now
                                picked = picked + 1
                            end
                        end
                    end
                end
            end
        end
    end

    if picked > 0 and Config.debug then
        log(picked .. " objet(s) ramasse(s).")
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

LoopAsync(Config.intervalMs or 1000, function()
    pcall(autoLoot)
    return true
end)

log("charge avec succes. Appuyez sur " .. (Config.keybind or "F3") .. " pour activer/desactiver.")


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
