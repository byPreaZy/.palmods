--[[
    PalExpBoost
    Multiplie l'experience gagnee par le joueur et les Pals.
    Hook sur les fonctions d'ajout d'EXP pour multiplier la valeur.
]]

local MOD_NAME = "PalExpBoost"
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
    Config = { modName = MOD_NAME, enabled = true, keybind = "F8", expMultiplier = 10.0, palExpMultiplier = 10.0, debug = false }
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

local function keyFromString(keyName)
    if not keyName or not Key then return nil end
    if Key[keyName] then return Key[keyName] end
    local upper = string.upper(keyName)
    if Key[upper] then return Key[upper] end
    return nil
end

-- Method 1: Hook EXP gain functions
local function setupHooks()
    -- Hook player EXP gain
    pcall(function()
        RegisterHook("/Script/Pal.PalPlayerState:AddExp", function(self, expParam)
            if not Config.enabled then return end
            local exp = expParam and expParam:get()
            if exp and exp > 0 then
                log("Player EXP intercepte: " .. tostring(exp) .. " x" .. tostring(Config.expMultiplier))
                -- The hook fires after the function, so we add bonus EXP
                ExecuteInGameThread(function()
                    local state = self:get()
                    if isValid(state) then
                        pcall(function() state:AddExp(math.floor(exp * (Config.expMultiplier - 1))) end)
                    end
                end)
            end
        end)
    end)

    -- Hook Pal EXP gain
    pcall(function()
        RegisterHook("/Script/Pal.PalCharacterParameterComponent:AddExp", function(self, expParam)
            if not Config.enabled then return end
            local exp = expParam and expParam:get()
            if exp and exp > 0 then
                log("Pal EXP intercepte: " .. tostring(exp) .. " x" .. tostring(Config.palExpMultiplier))
                ExecuteInGameThread(function()
                    local comp = self:get()
                    if isValid(comp) then
                        pcall(function() comp:AddExp(math.floor(exp * (Config.palExpMultiplier - 1))) end)
                    end
                end)
            end
        end)
    end)

    log("Hooks EXP installes")
end

-- Method 2: Fallback - modify PalGameSetting EXP rates
local function applyGameSettingBoost()
    local setting = FindFirstOf("PalGameSetting")
    if not isValid(setting) then return end

    -- Boost EXP rate via game settings if available
    safeSet(setting, "ExpRate", Config.expMultiplier)
    log("PalGameSetting ExpRate applique")
end

-- Setup hooks on player spawn
RegisterHook("/Script/Engine.PlayerController:ClientRestart", function()
    ExecuteWithDelay(3000, function()
        setupHooks()
        applyGameSettingBoost()
    end)
end)

-- Keybind toggle
local key = keyFromString(Config.keybind)
if key then
    RegisterKeyBind(key, function()
        Config.enabled = not Config.enabled
        if Config.enabled then
            log("Active (x" .. tostring(Config.expMultiplier) .. ")")
        else
            log("Desactive")
        end
    end)
end

log("PalExpBoost charge. Touche: " .. tostring(Config.keybind) .. " | Multi: x" .. tostring(Config.expMultiplier))
