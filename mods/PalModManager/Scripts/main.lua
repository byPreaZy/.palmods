--[[
    PalModManager
    Fenetre ImGui unifiee pour configurer tous les mods Palworld ULTRA MAX.
    Touche par defaut : F10
--]]

local MOD_NAME = "PalModManager"
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
local MODS_DIR = MOD_DIR:match("^(.*)/[^/]+$") or ""
local CONFIG_PATH = MOD_DIR .. "/Scripts/config.lua"

local Config = nil
local State = {
    initialized = false,
    showWindow = false,
    modConfigs = {},
    modOrder = {},
    statusText = "",
    statusTime = 0,
    pendingReload = false,
}

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

local function keyFromString(keyName)
    if not keyName or not Key then return nil end
    if Key[keyName] then return Key[keyName] end
    local upper = string.upper(keyName)
    if Key[upper] then return Key[upper] end
    local aliases = {
        PAGEUP = "PageUp",
        PAGEDOWN = "PageDown",
        DEL = "Delete",
        INS = "Insert",
        ENTER = "Return",
        RET = "Return",
        ESC = "Escape",
        SPACE = "SpaceBar",
        NUM0 = "NumPadZero",
        NUM1 = "NumPadOne",
        NUM2 = "NumPadTwo",
        NUM3 = "NumPadThree",
        NUM4 = "NumPadFour",
        NUM5 = "NumPadFive",
        NUM6 = "NumPadSix",
        NUM7 = "NumPadSeven",
        NUM8 = "NumPadEight",
        NUM9 = "NumPadNine",
    }
    local a = aliases[upper]
    if a and Key[a] then return Key[a] end
    return nil
end

-- ============================================================================
-- CHARGEMENT/SAUVEGARDE CONFIG
-- ============================================================================

local function loadConfigFile(path)
    local chunk, e = loadfile(path)
    if not chunk then
        err("loadfile " .. path .. " : " .. tostring(e))
        return nil
    end
    local ok, cfg = pcall(chunk)
    if not ok then
        err("exec " .. path .. " : " .. tostring(cfg))
        return nil
    end
    if type(cfg) ~= "table" then
        err("config " .. path .. " n'est pas une table")
        return nil
    end
    return cfg
end

local function serializeValue(v, indent)
    indent = indent or ""
    local t = type(v)
    if t == "boolean" then
        return tostring(v)
    elseif t == "number" then
        return tostring(v)
    elseif t == "string" then
        local s = v:gsub("\\", "\\\\"):gsub('"', '\\"')
        return '"' .. s .. '"'
    elseif t == "table" then
        local lines = {}
        for k, val in pairs(v) do
            local line
            if type(k) == "string" then
                line = "    " .. indent .. k .. " = " .. serializeValue(val, indent .. "    ") .. ","
            elseif type(k) == "number" then
                line = "    " .. indent .. "[" .. k .. "] = " .. serializeValue(val, indent .. "    ") .. ","
            end
            if line then table.insert(lines, line) end
        end
        table.sort(lines)
        return "{\n" .. table.concat(lines, "\n") .. "\n" .. indent .. "}"
    else
        return "nil"
    end
end

local function saveConfigFile(path, cfg)
    local text = "return " .. serializeValue(cfg, "") .. "\n"
    local f, e = io.open(path, "w")
    if not f then
        err("sauvegarde " .. path .. " : " .. tostring(e))
        return false
    end
    f:write(text)
    f:close()
    return true
end

local function loadManagerConfig()
    Config = loadConfigFile(CONFIG_PATH)
    if not Config then
        Config = {
            modName = MOD_NAME,
            enabled = true,
            keybind = "HELP",
            showAtStartup = false,
            windowTitle = "Pal Mod Manager",
            windowWidth = 650,
            windowHeight = 700,
            knownMods = {},
        }
    end
end

local function discoverMods()
    State.modConfigs = {}
    State.modOrder = {}
    local known = Config.knownMods or {}
    for _, modName in ipairs(known) do
        local modPath = MODS_DIR .. "/" .. modName
        local cfgPath = modPath .. "/Scripts/config.lua"
        if io.open(cfgPath, "r") then
            local cfg = loadConfigFile(cfgPath)
            if cfg then
                State.modConfigs[modName] = cfg
                table.insert(State.modOrder, modName)
            end
        else
            -- mod absent, on l'ignore silencieusement
        end
    end
    -- tri alphabetique
    table.sort(State.modOrder)
end

local function refreshConfig(modName)
    local cfgPath = MODS_DIR .. "/" .. modName .. "/Scripts/config.lua"
    local cfg = loadConfigFile(cfgPath)
    if cfg then
        State.modConfigs[modName] = cfg
        if _G.PalModManagerConfig then
            _G.PalModManagerConfig[modName] = cfg
        end
    end
end

local function saveAllConfigs()
    if not _G.PalModManagerConfig then _G.PalModManagerConfig = {} end
    local ok = true
    for _, modName in ipairs(State.modOrder) do
        local cfg = State.modConfigs[modName]
        local cfgPath = MODS_DIR .. "/" .. modName .. "/Scripts/config.lua"
        if not saveConfigFile(cfgPath, cfg) then
            ok = false
        else
            _G.PalModManagerConfig[modName] = cfg
        end
    end
    if ok then
        State.statusText = "Configurations sauvegardees."
        State.statusTime = os.time and os.time() or 0
    else
        State.statusText = "Erreur lors de la sauvegarde."
        State.statusTime = os.time and os.time() or 0
    end
end

local function reloadAllConfigs()
    discoverMods()
    State.statusText = "Configurations rechargees."
    State.statusTime = os.time and os.time() or 0
end

-- ============================================================================
-- UI IMGUI
-- ============================================================================

local function drawControl(key, cfg, label)
    local value = cfg[key]
    local t = type(value)
    local changed = false
    local newValue = value

    if key == "enabled" then
        changed, newValue = ImGui.Checkbox(label or "Active", value)
    elseif key == "keybind" then
        ImGui.Text(label or "Raccourci")
        ImGui.SameLine()
        local c, txt = ImGui.InputText("##" .. key, value or "", 32)
        if c then newValue = txt; changed = true end
    elseif t == "boolean" then
        changed, newValue = ImGui.Checkbox(label or key, value)
    elseif t == "number" then
        -- distinction entier / flottant
        if value == math.floor(value) then
            changed, newValue = ImGui.InputInt(label or key, value)
        else
            changed, newValue = ImGui.InputFloat(label or key, value, 0.01, 0.1, 3)
        end
    elseif t == "string" then
        local c, txt = ImGui.InputText(label or key, value or "", 128)
        if c then newValue = txt; changed = true end
    end

    if changed then
        cfg[key] = newValue
        return true
    end
    return false
end

local function drawModSection(modName)
    local cfg = State.modConfigs[modName]
    if not cfg then return false end

    local changed = false
    local open = ImGui.CollapsingHeader(modName)
    if open then
        -- Champs geres
        local ordered = { "enabled", "keybind" }
        local seen = { enabled = true, keybind = true }
        for _, k in ipairs(ordered) do
            if cfg[k] ~= nil then
                if drawControl(k, cfg, k) then
                    changed = true
                end
            end
        end

        -- Champs additionnels
        local extraKeys = {}
        for k, _ in pairs(cfg) do
            if not seen[k] and not string.find(k, "^modName$") and not string.find(k, "^version$") and not string.find(k, "^description$") then
                table.insert(extraKeys, k)
            end
        end
        table.sort(extraKeys)
        for _, k in ipairs(extraKeys) do
            if drawControl(k, cfg, k) then
                changed = true
            end
        end
    end
    return changed
end

local function drawWindow()
    if not State.showWindow then return end
    if not ImGui then return end

    local ok = pcall(function()
        ImGui.SetNextWindowSize(Config.windowWidth or 650, Config.windowHeight or 700, ImGuiCond.FirstUseEver)
        ImGui.Begin(Config.windowTitle or "Pal Mod Manager", true)
    end)
    if not ok then
        State.showWindow = false
        err("ImGui non disponible.")
        return
    end

    ImGui.Text("Configuration de tous les mods Lua")
    ImGui.Separator()

    if ImGui.Button("Sauvegarder") then
        saveAllConfigs()
    end
    ImGui.SameLine()
    if ImGui.Button("Recharger") then
        reloadAllConfigs()
    end
    ImGui.SameLine()
    if ImGui.Button("Masquer (" .. (Config.keybind or "F10") .. ")") then
        State.showWindow = false
    end

    if State.statusText and State.statusText ~= "" then
        ImGui.Text("[ " .. State.statusText .. " ]")
    end

    ImGui.Separator()

    local anyChange = false
    for _, modName in ipairs(State.modOrder) do
        if drawModSection(modName) then
            anyChange = true
        end
    end

    -- Sauvegarde automatique si modification
    if anyChange then
        saveAllConfigs()
    end

    ImGui.End()
end

-- ============================================================================
-- INITIALISATION
-- ============================================================================

local function init()
    if State.initialized then return end
    loadManagerConfig()
    discoverMods()
    State.showWindow = Config.showAtStartup or false
    State.initialized = true
    log("Initialisation terminee. Mods detectes : " .. #State.modOrder)
end

local function toggleWindow()
    State.showWindow = not State.showWindow
    log((State.showWindow and "Ouverture" or "Fermeture") .. " du manager.")
end

-- ============================================================================
-- BOUCLE PRINCIPALE
-- ============================================================================

LoopAsync(100, function()
    pcall(function()
        if not State.initialized then
            init()
        end
        drawWindow()
    end)
    return true
end)

pcall(function()
    if Config and Config.keybind and Key then
        local k = keyFromString(Config.keybind)
        if k and RegisterKeyBind then
            RegisterKeyBind(k, function()
                pcall(toggleWindow)
            end)
        end
    end
end)

log("charge avec succes. Appuyez sur " .. (Config and Config.keybind or "F10") .. " pour ouvrir le manager.")
