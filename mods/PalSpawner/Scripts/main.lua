--[[
    PalSpawner
    Menu ImGui complet pour spawner des Pals avec stats et passives custom.
    Compatible Palworld v1.0.2 / UE4SS
]]

local MOD_NAME = "PalSpawner"
local function log(msg) print("[" .. MOD_NAME .. "] " .. tostring(msg)) end
local function err(msg) print("[" .. MOD_NAME .. " ERREUR] " .. tostring(msg)) end

local config = require("config")
local palDB = require("pal_list")
local passivesDB = require("passives")

local MOD_PREFIX = config.modPrefix or "[PalSpawner]"
local DEBUG = config.debug

-- ============================================================================
-- ETAT
-- ============================================================================

local state = {
    showMenu = false,
    initialized = false,
    searchFilter = "",
    selectedPalIdx = 1,
    pendingSpawn = nil,
    spawnResult = "",
    spawnResultTimer = 0,
}

-- Current spawn settings
local spawn = {
    level = config.defaults.level,
    gender = config.defaults.gender,
    hp = config.defaults.hp,
    attack = config.defaults.attack,
    defense = config.defaults.defense,
    stamina = config.defaults.stamina,
    passiveIdx = {1, 1, 1, 1},  -- index into passives list (1 = None)
    spawnMode = config.defaults.spawnMode,
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
    for part in string.gmatch(path, "[^.]+") do
        table.insert(parts, part)
    end
    for i = 1, #parts - 1 do
        local ok, nextObj = pcall(function() return current[parts[i]] end)
        if not ok or not isValid(nextObj) then return false end
        current = nextObj
    end
    local ok = pcall(function() current[parts[#parts]] = value end)
    return ok
end

-- ============================================================================
-- ACCESSEURS JEU
-- ============================================================================

local function getPlayerController()
    local pcs = FindAllOf and FindAllOf("PlayerController")
    if pcs then
        for _, pc in ipairs(pcs) do
            if isValid(pc) then return pc end
        end
    end
    local pc = FindFirstOf and FindFirstOf("PlayerController")
    return isValid(pc) and pc or nil
end

local function getPlayerCharacter()
    local controller = getPlayerController()
    if isValid(controller) then
        local char = safeGet(controller, "Pawn")
        if isValid(char) then return char end
    end
    local candidates = {"PalPlayerCharacter", "BP_PlayerCharacter_Female_C", "BP_PlayerCharacter_Male_C"}
    for _, className in ipairs(candidates) do
        local obj = FindFirstOf(className)
        if isValid(obj) then return obj end
    end
    return nil
end

local function getPalUtility()
    local util = StaticFindObject("/Script/Pal.Default__PalUtility")
    if isValid(util) then return util end
    util = FindFirstOf("PalUtility")
    return isValid(util) and util or nil
end

-- ============================================================================
-- LOGIQUE DE SPAWN
-- ============================================================================

local function getSelectedPal()
    return palDB.pals[state.selectedPalIdx]
end

local function getSelectedPassives()
    local result = {}
    for i = 1, 4 do
        local idx = spawn.passiveIdx[i]
        if idx and idx > 1 then
            local skill = passivesDB.skills[idx - 1]
            if skill then
                table.insert(result, skill.internal)
            end
        end
    end
    return result
end

local function enableCheats()
    local pc = getPlayerController()
    if not isValid(pc) then
        err("PlayerController indisponible")
        return false
    end
    pcall(function() pc:EnableCheats() end)
    return true
end

local function spawnPalViaCheatManager(internalName, level)
    local pc = getPlayerController()
    if not isValid(pc) then return false end

    local cm = safeGet(pc, "CheatManager")
    if isValid(cm) then
        -- Try SpawnMonster first (most reliable)
        local ok = pcall(function()
            cm:SpawnMonster(FName(internalName), level)
        end)
        if ok then return true end

        -- Try DebugSpawn
        ok = pcall(function()
            cm:DebugSpawn(FName(internalName), level)
        end)
        if ok then return true end
    end

    -- Try PalUtility
    local util = getPalUtility()
    if isValid(util) then
        local ok = pcall(function()
            util:SpawnPal(FName(internalName), level)
        end)
        if ok then return true end
    end

    return false
end

local function applyCustomStats(palActor)
    if not isValid(palActor) then return end

    local param = safeGet(palActor, "CharacterParameterComponent")
    if not isValid(param) then return end

    local saveParam = safeGet(param, "IndividualParameter.SaveParameter")
    if not saveParam then return end

    -- Apply HP
    if spawn.hp > 0 then
        pcall(function()
            if saveParam.MaxHP and saveParam.MaxHP.Value then
                saveParam.MaxHP.Value = spawn.hp
            end
            if saveParam.HP and saveParam.HP.Value then
                saveParam.HP.Value = spawn.hp
            end
        end)
    end

    -- Apply Attack
    if spawn.attack > 0 then
        pcall(function()
            if saveParam.Attack and saveParam.Attack.Value then
                saveParam.Attack.Value = spawn.attack
            end
        end)
    end

    -- Apply Defense
    if spawn.defense > 0 then
        pcall(function()
            if saveParam.Defense and saveParam.Defense.Value then
                saveParam.Defense.Value = spawn.defense
            end
        end)
    end

    -- Apply Stamina
    if spawn.stamina > 0 then
        pcall(function()
            if saveParam.MaxSP and saveParam.MaxSP.Value then
                saveParam.MaxSP.Value = spawn.stamina
            end
            if saveParam.SP and saveParam.SP.Value then
                saveParam.SP.Value = spawn.stamina
            end
        end)
    end

    -- Apply Level
    pcall(function()
        if saveParam.Level then
            saveParam.Level = spawn.level
        end
    end)

    -- Apply Passive Skills
    local passives = getSelectedPassives()
    if #passives > 0 then
        pcall(function()
            local skillList = safeGet(param, "PassiveSkillList")
            if skillList then
                for i, skillName in ipairs(passives) do
                    if i <= 4 then
                        skillList[i] = FName(skillName)
                    end
                end
            end
        end)

        -- Also try via SaveParameter
        pcall(function()
            local palPassives = safeGet(saveParam, "PassiveSkillList")
            if palPassives then
                for i, skillName in ipairs(passives) do
                    if i <= 4 then
                        palPassives[i] = FName(skillName)
                    end
                end
            end
        end)
    end

    -- Apply Gender
    if spawn.gender > 0 then
        pcall(function()
            saveParam.Gender = spawn.gender - 1  -- 0=male, 1=female
        end)
    end
end

local function findLastSpawnedPal(internalName)
    -- Search for recently spawned PalCharacter matching the internal name
    local allPals = FindAllOf("PalCharacter")
    if not allPals then return nil end

    local player = getPlayerCharacter()
    for i = #allPals, 1, -1 do
        local pal = allPals[i]
        if isValid(pal) and pal ~= player then
            local palName = safeGet(pal, "CharacterParameterComponent.IndividualParameter.SaveParameter.CharacterID")
            if palName and tostring(palName):find(internalName) then
                return pal
            end
            -- Also try the Pal's class name
            local ok, className = pcall(function() return pal:GetClass() end)
            if ok and className then
                local cn = tostring(className)
                if cn:find(internalName) then
                    return pal
                end
            end
        end
    end
    return nil
end

local function doSpawn()
    local pal = getSelectedPal()
    if not pal then
        state.spawnResult = "Erreur: aucun Pal selectionne"
        state.spawnResultTimer = 3.0
        return
    end

    if not enableCheats() then
        state.spawnResult = "Erreur: impossible d'activer les cheats"
        state.spawnResultTimer = 3.0
        return
    end

    log(string.format("Spawning %s (%s) level %d", pal.name, pal.internal, spawn.level))

    local success = spawnPalViaCheatManager(pal.internal, spawn.level)

    if success then
        state.spawnResult = string.format("Pal spawn: %s (Lv.%d)", pal.name, spawn.level)
        state.spawnResultTimer = 3.0

        -- Try to find and modify the spawned pal
        ExecuteWithDelay(500, function()
            local spawnedPal = findLastSpawnedPal(pal.internal)
            if spawnedPal then
                applyCustomStats(spawnedPal)
                log("Stats custom appliquees avec succes")
            else
                log("Pal spawn mais non trouve pour modification des stats")
            end
        end)
    else
        state.spawnResult = "Erreur: echec du spawn"
        state.spawnResultTimer = 3.0
        err("Echec du spawn pour " .. pal.internal)
    end
end

-- ============================================================================
-- MENU IMGUI
-- ============================================================================

local function drawPalList()
    ImGui.Text("Liste des Pals (" .. #palDB.pals .. ")")
    ImGui.Separator()

    -- Search filter
    local changed, newText = ImGui.InputText("Rechercher", state.searchFilter, 128)
    if changed then state.searchFilter = newText end

    ImGui.Separator()

    -- Pal list with filter
    local filter = string.lower(state.searchFilter or "")
    local listHeight = 300
    if ImGui.BeginListBox("##PalList", -1, listHeight) then
        for i, pal in ipairs(palDB.pals) do
            local displayText = string.format("[%s] %s (%s/%s)", pal.dex, pal.name, pal.type1, pal.type2 ~= "" and pal.type2 or "-")
            if filter == "" or string.find(string.lower(displayText), filter) then
                local isSelected = (i == state.selectedPalIdx)
                if ImGui.Selectable(displayText, isSelected) then
                    state.selectedPalIdx = i
                end
                if isSelected then
                    ImGui.SetItemDefaultFocus()
                end
            end
        end
        ImGui.EndListBox()
    end
end

local function drawStatsTab()
    local pal = getSelectedPal()
    if not pal then
        ImGui.Text("Aucun Pal selectionne")
        return
    end

    ImGui.Text("Stats pour: " .. pal.name)
    ImGui.Separator()

    -- Level
    local changed, value = ImGui.SliderInt("Niveau", spawn.level,
        config.limits.minLevel, config.limits.maxLevel)
    if changed then spawn.level = value end

    -- Gender
    ImGui.Text("Genre:")
    ImGui.SameLine()
    if ImGui.RadioButton("Aleatoire", spawn.gender == 0) then spawn.gender = 0 end
    ImGui.SameLine()
    if ImGui.RadioButton("Male", spawn.gender == 1) then spawn.gender = 1 end
    ImGui.SameLine()
    if ImGui.RadioButton("Femelle", spawn.gender == 2) then spawn.gender = 2 end

    ImGui.Separator()

    -- HP
    changed, value = ImGui.SliderInt("HP Max", spawn.hp,
        config.limits.minHP, config.limits.maxHP)
    if changed then spawn.hp = value end

    -- Attack
    changed, value = ImGui.SliderInt("Attaque", spawn.attack,
        config.limits.minAttack, config.limits.maxAttack)
    if changed then spawn.attack = value end

    -- Defense
    changed, value = ImGui.SliderInt("Defense", spawn.defense,
        config.limits.minDefense, config.limits.maxDefense)
    if changed then spawn.defense = value end

    -- Stamina
    changed, value = ImGui.SliderInt("Stamina Max", spawn.stamina,
        config.limits.minStamina, config.limits.maxStamina)
    if changed then spawn.stamina = value end
end

local function drawPassivesTab()
    ImGui.Text("Competences Passives (max 4)")
    ImGui.Separator()

    -- Build passive names list with "None" at index 1
    local passiveNames = {"None"}
    for _, skill in ipairs(passivesDB.skills) do
        table.insert(passiveNames, string.format("%s (T%d) - %s", skill.name, skill.tier, skill.desc))
    end

    for i = 1, 4 do
        ImGui.Text("Slot " .. i .. ":")
        ImGui.SameLine()
        local changed, idx = ImGui.Combo("##passive" .. i, spawn.passiveIdx[i], passiveNames, #passiveNames)
        if changed then spawn.passiveIdx[i] = idx end
    end

    ImGui.Separator()
    ImGui.TextWrapped("Les passives seront appliquees apres le spawn.")
    ImGui.Text("Passives selectionnes:")
    local passives = getSelectedPassives()
    if #passives == 0 then
        ImGui.Text("  (aucun)")
    else
        for _, p in ipairs(passives) do
            ImGui.Text("  - " .. p)
        end
    end
end

local function drawSpawnTab()
    local pal = getSelectedPal()
    if not pal then
        ImGui.Text("Aucun Pal selectionne")
        return
    end

    ImGui.Text("Spawn de: " .. pal.name)
    ImGui.Text("Nom interne: " .. pal.internal)
    ImGui.Text("Type: " .. pal.type1 .. (pal.type2 ~= "" and " / " .. pal.type2 or ""))
    ImGui.Text("Niveau: " .. spawn.level)
    ImGui.Separator()

    -- Spawn mode
    ImGui.Text("Mode de spawn:")
    ImGui.SameLine()
    if ImGui.RadioButton("Pres du joueur", spawn.spawnMode == "near") then spawn.spawnMode = "near" end
    ImGui.SameLine()
    if ImGui.RadioButton("Ajouter a la Palbox", spawn.spawnMode == "palbox") then spawn.spawnMode = "palbox" end

    ImGui.Separator()

    -- Spawn button
    if ImGui.Button("Spawner le Pal", 200, 40) then
        doSpawn()
    end

    -- Result message
    if state.spawnResultTimer > 0 then
        ImGui.SameLine()
        ImGui.Text(state.spawnResult)
    end

    ImGui.Separator()
    ImGui.TextWrapped("Note: Le spawn fonctionne en single-player. En multi, vous devez etre admin.")
end

local function drawMenu()
    if not state.showMenu then return end
    if not ImGui or not ImGui.Begin then
        err("ImGui indisponible")
        return
    end

    local cond = (ImGui.Cond and ImGui.Cond.FirstUseEver) or 4
    ImGui.SetNextWindowSize(600, 550, cond)

    if ImGui.Begin("PalSpawner - Palworld v1.0.2") then
        -- Header
        ImGui.Text("PalSpawner - Menu de spawn de Pals")
        ImGui.Separator()

        if ImGui.BeginTabBar("PalSpawnerTabs", 0) then
            if ImGui.BeginTabItem("Liste des Pals") then
                drawPalList()
                ImGui.EndTabItem()
            end
            if ImGui.BeginTabItem("Stats") then
                drawStatsTab()
                ImGui.EndTabItem()
            end
            if ImGui.BeginTabItem("Passives") then
                drawPassivesTab()
                ImGui.EndTabItem()
            end
            if ImGui.BeginTabItem("Spawn") then
                drawSpawnTab()
                ImGui.EndTabItem()
            end
            ImGui.EndTabBar()
        end
    end

    ImGui.End()
end

-- ============================================================================
-- INITIALISATION
-- ============================================================================

local function init()
    if state.initialized then return end
    log("Initialisation PalSpawner...")
    state.initialized = true
    log("PalSpawner pret. Appuyez sur F2 pour ouvrir le menu.")
end

-- ============================================================================
-- KEYBIND
-- ============================================================================

if config.toggleKey then
    RegisterKeyBind(config.toggleKey, function()
        state.showMenu = not state.showMenu
        if state.showMenu then
            init()
        end
    end)
end

-- ============================================================================
-- RENDER LOOP
-- ============================================================================

local renderHooked = false
local function setupRenderHook()
    if renderHooked then return end
    renderHooked = true

    -- Hook into the HUD render to draw our ImGui menu
    RegisterHook("/Script/Pal.PalHUDService:OnTick", function()
        if state.spawnResultTimer > 0 then
            state.spawnResultTimer = state.spawnResultTimer - 0.016
        end
        drawMenu()
    end)

    -- Fallback: also try hooking PalPlayerController
    RegisterHook("/Script/Engine.PlayerController:ClientRestart", function()
        ExecuteWithDelay(3000, function()
            if not renderHooked then
                setupRenderHook()
            end
        end)
    end)
end

-- Try immediate setup
ExecuteAsync(function()
    local hud = FindFirstOf("PalHUDService")
    if isValid(hud) then
        setupRenderHook()
    else
        NotifyOnNewObject("/Script/Pal.PalHUDService", function()
            setupRenderHook()
        end)
    end
end)

init()
