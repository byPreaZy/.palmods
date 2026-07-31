--[[
    PalBaseExpansion
    Agrandit le rayon de construction de la Palbox.
    Utilise NotifyOnNewObject pour detecter les nouvelles bases et
    appliquer le AreaRange configure.
    Source: pwmodding.wiki tutorial "Moar Digging"
]]

local MOD_NAME = "PalBaseExpansion"
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
    Config = { modName = MOD_NAME, enabled = true, keybind = "F11", areaRange = 6000.0, applyToExisting = true, debug = false }
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

local function expandBase(baseModel)
    if not isValid(baseModel) then return end
    if not Config.enabled then return end

    log("Base detectee, application AreaRange=" .. tostring(Config.areaRange))
    ExecuteWithDelay(10000, function()
        if isValid(baseModel) then
            safeSet(baseModel, "AreaRange", Config.areaRange)
            log("AreaRange applique avec succes")
        end
    end)
end

-- Detect new base camps
NotifyOnNewObject("/Script/Pal.PalBaseCampModel", function(baseModel)
    log("Nouvelle base detectee")
    expandBase(baseModel)
end)

-- Apply to existing bases on startup
if Config.applyToExisting then
    ExecuteWithDelay(5000, function()
        local bases = FindAllOf("PalBaseCampModel")
        if bases then
            for _, base in ipairs(bases) do
                if isValid(base) then
                    expandBase(base)
                end
            end
        end
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
            log("Active (AreaRange=" .. tostring(Config.areaRange) .. ")")
            -- Apply to all existing bases
            local bases = FindAllOf("PalBaseCampModel")
            if bases then
                for _, base in ipairs(bases) do
                    if isValid(base) then
                        safeSet(base, "AreaRange", Config.areaRange)
                    end
                end
                log("Bases existantes mises a jour")
            end
        else
            log("Desactive")
        end
    end)
end

log("PalBaseExpansion charge. Touche: " .. tostring(Config.keybind) .. " | Range: " .. tostring(Config.areaRange))
