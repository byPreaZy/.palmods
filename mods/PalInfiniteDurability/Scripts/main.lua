--[[
    PalInfiniteDurability
    Maintient la durabilite des armes et outils au maximum.
]]

local MOD_NAME = "PalInfiniteDurability"
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
    Config = { modName = MOD_NAME, enabled = true, keybind = "F6", intervalMs = 500, debug = false }
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

local active = false

local function repairAllWeapons()
    local items = FindAllOf("PalWeaponItem")
    if not items then return end
    for _, item in ipairs(items) do
        if isValid(item) then
            local maxDura = safeGet(item, "MaxDurability")
            if maxDura then
                pcall(function() item.Durability = maxDura end)
            end
        end
    end
end

local function startLoop()
    if active then return end
    active = true
    LoopAsync(Config.intervalMs, function()
        if not active or not Config.enabled then return true end
        repairAllWeapons()
        return false
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
            log("Active")
            startLoop()
        else
            log("Desactive")
        end
    end)
end

if Config.enabled then
    ExecuteWithDelay(3000, function() startLoop() end)
end

log("PalInfiniteDurability charge. Touche: " .. tostring(Config.keybind))
