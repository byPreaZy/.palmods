--[[
    PalNoStaminaFly
    Supprime le cout de stamina pour le vol (fly) sur les Pals montables.
    Modifie les valeurs FlyHover_SP, FlyHorizon_SP, etc. dans PalGameSetting.
]]

local MOD_NAME = "PalNoStaminaFly"
local function log(msg) print("[" .. MOD_NAME .. "] " .. tostring(msg)) end

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
    Config = { modName = MOD_NAME, enabled = true, keybind = "F9", debug = false }
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

-- Store original values for restore
local originalValues = {}

local function applyNoFlyStamina()
    local setting = FindFirstOf("PalGameSetting")
    if not isValid(setting) then
        setting = FindFirstOf("BP_PalGameSetting_C")
    end
    if not isValid(setting) then
        log("PalGameSetting non trouve")
        return false
    end

    -- Save originals
    if not originalValues.saved then
        local ok, val
        ok, val = pcall(function() return setting.FlyHover_SP end)
        if ok and val then originalValues.FlyHover_SP = val end
        ok, val = pcall(function() return setting.FlyHorizon_SP end)
        if ok and val then originalValues.FlyHorizon_SP = val end
        ok, val = pcall(function() return setting.FlyHorizon_Dash_SP end)
        if ok and val then originalValues.FlyHorizon_Dash_SP = val end
        ok, val = pcall(function() return setting.FlyVertical_SP end)
        if ok and val then originalValues.FlyVertical_SP = val end
        originalValues.saved = true
    end

    -- Set all fly stamina costs to 0
    safeSet(setting, "FlyHover_SP", 0.0)
    safeSet(setting, "FlyHorizon_SP", 0.0)
    safeSet(setting, "FlyHorizon_Dash_SP", 0.0)
    safeSet(setting, "FlyVertical_SP", 0.0)

    log("Stamina de vol supprimee")
    return true
end

local function restoreFlyStamina()
    local setting = FindFirstOf("PalGameSetting")
    if not isValid(setting) then return end

    if originalValues.FlyHover_SP then safeSet(setting, "FlyHover_SP", originalValues.FlyHover_SP) end
    if originalValues.FlyHorizon_SP then safeSet(setting, "FlyHorizon_SP", originalValues.FlyHorizon_SP) end
    if originalValues.FlyHorizon_Dash_SP then safeSet(setting, "FlyHorizon_Dash_SP", originalValues.FlyHorizon_Dash_SP) end
    if originalValues.FlyVertical_SP then safeSet(setting, "FlyVertical_SP", originalValues.FlyVertical_SP) end

    log("Stamina de vol restauree")
end

local function keyFromString(keyName)
    if not keyName or not Key then return nil end
    if Key[keyName] then return Key[keyName] end
    local upper = string.upper(keyName)
    if Key[upper] then return Key[upper] end
    return nil
end

local key = keyFromString(Config.keybind)
if key then
    RegisterKeyBind(key, function()
        Config.enabled = not Config.enabled
        if Config.enabled then
            applyNoFlyStamina()
        else
            restoreFlyStamina()
        end
    end)
end

if Config.enabled then
    ExecuteWithDelay(5000, function() applyNoFlyStamina() end)
end

log("PalNoStaminaFly charge. Touche: " .. tostring(Config.keybind))
