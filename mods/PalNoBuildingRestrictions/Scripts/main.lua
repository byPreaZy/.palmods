--[[
    PalNoBuildingRestrictions
    Supprime les restrictions de construction dans Palworld.
]]

local MOD_NAME = "PalNoBuildingRestrictions"
local function log(msg) print("[" .. MOD_NAME .. "] " .. tostring(msg)) end
local function err(msg) print("[" .. MOD_NAME .. " ERREUR] " .. tostring(msg)) end

local function getModDir()
    local src = debug and debug.getinfo and debug.getinfo(1, "S").source
    if not src then return "." end
    src = tostring(src):gsub("^@", ""):gsub("\\", "/")
    return src:match("^(.*)/Scripts/main%.lua$") or "."
end

local CONFIG_PATH = getModDir() .. "/Scripts/config.lua"
local Config
local function loadConfig()
    local chunk = loadfile(CONFIG_PATH)
    if chunk then
        local ok, cfg = pcall(chunk)
        if ok and type(cfg) == "table" then Config = cfg; return end
    end
    Config = { modName = MOD_NAME, enabled = true, keybind = "F5", walkableFloorAngle = 90.0, applyOnStartup = true, debug = false }
end
loadConfig()

local function isValid(obj)
    if obj == nil then return false end
    local t = type(obj)
    if t ~= "table" and t ~= "userdata" then return true end
    if t == "userdata" then
        local ok, valid = pcall(function() return obj:IsValid() end)
        return ok and valid
    end
    return true
end

local function safeSet(obj, path, value)
    if not isValid(obj) then return false end
    local current = obj
    local parts = {}
    for part in string.gmatch(path, "[^.]+") do table.insert(parts, part) end
    for i = 1, #parts - 1 do
        local ok, nextObj = pcall(function() return current[parts[i]] end)
        if not ok or not isValid(nextObj) then return false end
        current = nextObj
    end
    return pcall(function() current[parts[#parts]] = value end)
end

local function applyRestrictions()
    local setting = FindFirstOf("PalGameSetting")
    if not isValid(setting) then
        setting = FindFirstOf("BP_PalGameSetting_C")
    end
    if not isValid(setting) then
        err("PalGameSetting non trouve")
        return false
    end

    -- Remove build restrictions
    safeSet(setting, "WalkableFloorAngleForDefault", Config.walkableFloorAngle)
    safeSet(setting, "WalkableFloorAngleForRide", Config.walkableFloorAngle)
    safeSet(setting, "IsEnableSpeedCollision", false)

    log("Restrictions de construction supprimees")
    return true
end

local function keyFromString(keyName)
    if not keyName or not Key then return nil end
    if Key[keyName] then return Key[keyName] end
    local upper = string.upper(keyName)
    if Key[upper] then return Key[upper] end
    return nil
end

-- Apply on startup with delay
if Config.applyOnStartup then
    ExecuteWithDelay(5000, function() applyRestrictions() end)
end

-- Keybind toggle
local key = keyFromString(Config.keybind)
if key then
    RegisterKeyBind(key, function()
        if Config.enabled then
            applyRestrictions()
        end
    end)
end

log("PalNoBuildingRestrictions charge. Touche: " .. tostring(Config.keybind))
