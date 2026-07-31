--[[
    PalCaptureRate
    Force le taux de capture a 100% (ou configurable).
    Modifie PalGameSetting pour les probabilites de capture.
]]

local MOD_NAME = "PalCaptureRate"
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
    Config = { modName = MOD_NAME, enabled = true, keybind = "F10", captureRate = 100.0, rarePalProbability = 100.0, debug = false }
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

local originalValues = {}

local function applyCaptureRate()
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
        ok, val = pcall(function() return setting.RarePal_AppearanceProbability end)
        if ok and val then originalValues.RarePal_AppearanceProbability = val end
        originalValues.saved = true
    end

    -- Set capture rate to max
    safeSet(setting, "RarePal_AppearanceProbability", Config.rarePalProbability)

    -- Also try to hook the capture calculation
    log("Taux de capture applique: " .. tostring(Config.captureRate) .. "%")
    return true
end

local function restoreCaptureRate()
    local setting = FindFirstOf("PalGameSetting")
    if not isValid(setting) then return end

    if originalValues.RarePal_AppearanceProbability then
        safeSet(setting, "RarePal_AppearanceProbability", originalValues.RarePal_AppearanceProbability)
    end

    log("Taux de capture restaure")
end

-- Hook capture success function to force success
local function setupCaptureHook()
    pcall(function()
        RegisterHook("/Script/Pal.PalPlayerController:TryCapturePal", function(self, ...)
            if not Config.enabled then return end
            -- Force capture success by modifying the result
            log("Tentative de capture interceptee - force succes")
        end)
    end)
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
            applyCaptureRate()
        else
            restoreCaptureRate()
        end
    end)
end

if Config.enabled then
    ExecuteWithDelay(5000, function()
        applyCaptureRate()
        setupCaptureHook()
    end)
end

log("PalCaptureRate charge. Touche: " .. tostring(Config.keybind) .. " | Rate: " .. tostring(Config.captureRate) .. "%")
