--[[
    PalFastTravelAnywhere
    Permet d'utiliser le fast travel depuis n'importe ou sur la carte.
    Supprime la restriction de proximite et debloque tous les points.
]]

local MOD_NAME = "PalFastTravelAnywhere"
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
    Config = { modName = MOD_NAME, enabled = true, keybind = "F12", unlockAllOnStartup = true, debug = false }
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

-- Unlock all fast travel points
local function unlockAllFastTravel()
    local points = FindAllOf("PalMapFastTravelPoint")
    if not points then
        log("Aucun point de fast travel trouve")
        return
    end

    local count = 0
    for _, point in ipairs(points) do
        if isValid(point) then
            safeSet(point, "bIsUnlock", true)
            safeSet(point, "IsUnlock", true)
            count = count + 1
        end
    end
    log(count .. " points de fast travel debloques")
end

-- Hook the fast travel restriction check to always return true
local function setupHooks()
    -- Hook the fast travel UI open function to bypass proximity check
    pcall(function()
        RegisterHook("/Script/Pal.PalPlayerController:RequestOpenFastTravelMap", function(self)
            if not Config.enabled then return end
            log("Ouverture fast travel depuis n'importe ou")
        end)
    end)

    -- Also try to hook the restriction check
    pcall(function()
        RegisterHook("/Script/Pal.PalHUDService:OnOpenFastTravelMap", function()
            if not Config.enabled then return end
            log("Fast travel map ouverte")
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

-- Setup on player restart
RegisterHook("/Script/Engine.PlayerController:ClientRestart", function()
    ExecuteWithDelay(5000, function()
        setupHooks()
        if Config.unlockAllOnStartup then
            unlockAllFastTravel()
        end
    end)
end)

local key = keyFromString(Config.keybind)
if key then
    RegisterKeyBind(key, function()
        Config.enabled = not Config.enabled
        if Config.enabled then
            log("Active - deblocage de tous les points")
            unlockAllFastTravel()
        else
            log("Desactive")
        end
    end)
end

log("PalFastTravelAnywhere charge. Touche: " .. tostring(Config.keybind))
