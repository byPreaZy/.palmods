--[[
    PalQuickStack
    Transfere rapidement un stack d'objets vers le conteneur ouvert via un raccourci.
--]]

local MOD_NAME = "PalQuickStack"
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
        keybind = "EXECUTE",
        itemId = "",
        amount = 1,
        slotIndex = 0,
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

local function getPlayerController()
    local all = FindAllOf and FindAllOf("PlayerController")
    if all then
        for _, pc in ipairs(all) do if isValid(pc) then return pc end end
    end
    local pc = FindFirstOf and FindFirstOf("PlayerController")
    return isValid(pc) and pc or nil
end

local function getPlayerState()
    local ps = FindFirstOf and FindFirstOf("PalPlayerState")
    if isValid(ps) then return ps end
    local all = FindAllOf and FindAllOf("PalPlayerState")
    if all then
        for _, p in ipairs(all) do if isValid(p) then return p end end
    end
    local pc = getPlayerController()
    if isValid(pc) then return safeGet(pc, "PlayerState") end
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
-- QUICK STACK
-- ============================================================================

local function onQuickStack()
    local gcfg = _G.PalModManagerConfig and _G.PalModManagerConfig[MOD_NAME]
    if gcfg then Config = gcfg end
    if not Config or not Config.enabled then return end

    local ps = getPlayerState()
    local pc = getPlayerController()
    local target = ps or pc
    if not isValid(target) then
        err("Joueur introuvable.")
        return
    end

    local itemId = Config.itemId or ""
    local amount = Config.amount or 1
    local slot = Config.slotIndex or 0

    -- Methodes vers conteneur ouvert
    local ok = false
    if itemId and itemId ~= "" then
        ok = tryCall(target, {
            "MoveItemToContainer", "ServerMoveItemToContainer", "TransferItemToContainer",
            "QuickStack", "ServerQuickStack", "PutItemToOpenedContainer", "ServerPutItemToOpenedContainer"
        }, itemId, amount)
        if ok then
            log("QuickStack : " .. itemId .. " x" .. amount)
            return
        end
    else
        ok = tryCall(target, {
            "MoveItemBySlot", "ServerMoveItemBySlot", "QuickStackSlot", "ServerQuickStackSlot",
            "TransferSlotToContainer", "ServerTransferSlotToContainer"
        }, slot, amount)
        if ok then
            log("QuickStack slot " .. slot .. " x" .. amount)
            return
        end
    end

    err("Aucune methode de transfert vers conteneur disponible.")
end

-- ============================================================================
-- INITIALISATION
-- ============================================================================

loadConfig()

pcall(function()
    local k = keyFromString(Config.keybind)
    if k and RegisterKeyBind then
        RegisterKeyBind(k, function()
            pcall(onQuickStack)
        end)
    end
end)

log("charge avec succes. Appuyez sur " .. (Config.keybind or "F7") .. " pour quick stack.")


-- =============================================================================
-- INTEGRATION PALTRAINER APP EXTERNE (ACTION ONE-SHOT)
-- =============================================================================

pcall(function()
    local ipc_ok, ipc = pcall(require, "PalTrainerIPC")
    if not ipc_ok or not ipc then return end
    ipc.ensureDir()
    local commandKey = "mod." .. MOD_NAME

    local function pollIPC()
        local cmds = ipc.pollCommands()
        if cmds and cmds[commandKey] then
            local raw = tostring(cmds[commandKey]):lower()
            if raw == "true" or raw == "1" then
                pcall(onQuickStack)
            end
            ipc.consumeKeys({ [commandKey] = true })
        end
        ipc.writeStatus({ [commandKey] = false })
    end

    if LoopInGameThreadWithDelay and type(LoopInGameThreadWithDelay) == "function" then
        LoopInGameThreadWithDelay(250, pollIPC)
    else
        LoopAsync(250, function() pollIPC(); return false end)
    end
end)
