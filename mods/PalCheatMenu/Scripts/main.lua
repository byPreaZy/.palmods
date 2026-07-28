
-- Robustesse : sauvegarde du RegisterHook d'origine pour un usage local securise
local _RegisterHook = RegisterHook

-- Constantes ImGui numeriques (evite la dependance aux globals ImGuiCond / ImGuiWindowFlags)
local IMGUI_COND_FIRSTUSEEVER = 4
local IMGUI_WINDOWFLAGS_NONE = 0
local IMGUI_WINDOWFLAGS_NOTITLEBAR = 1
local IMGUI_WINDOWFLAGS_NORESIZE = 2
local IMGUI_WINDOWFLAGS_NOMOVE = 4
local IMGUI_WINDOWFLAGS_NOSCROLLBAR = 8
local IMGUI_WINDOWFLAGS_NOSCROLLWITHMOUSE = 16
local IMGUI_WINDOWFLAGS_NOCOLLAPSE = 32
local IMGUI_WINDOWFLAGS_ALWAYSAUTORESIZE = 64
local IMGUI_WINDOWFLAGS_NOSAVEDSETTINGS = 256
local IMGUI_WINDOWFLAGS_NOFOCUSONAPPEARING = 4096
local IMGUI_WINDOWFLAGS_NONAV = 8192
local IMGUI_WINDOWFLAGS_NODECORATION = IMGUI_WINDOWFLAGS_NOTITLEBAR + IMGUI_WINDOWFLAGS_NORESIZE + IMGUI_WINDOWFLAGS_NOSCROLLBAR + IMGUI_WINDOWFLAGS_NOCOLLAPSE

-- On ne remplace pas ImGui par un proxy : on verifie son existence a chaque frame

--[[
    PalCheatMenu MAX
    Menu de triches ultime pour Palworld 1.0 (Steam) / UE4SS
    Auteur : genere par Cascade

    Organisation :
    - PalCheatMenu/Scripts/main.lua  (ce fichier)
    - PalCheatMenu/Scripts/config.lua (configuration)
    - PalCheatMenu/Info.json         (metadonnees)
--]]

local config = require("config")
local ipc_ok, PalTrainerIPC = pcall(require, "PalTrainerIPC")
local MOD_PREFIX = config.modPrefix or "[PalCheatMenu]"
local DEBUG = config.debug

-- ============================================================================
-- ETAT DU MOD
-- ============================================================================

local state = {
    initialized = false,
    showMenu = true,
    activeTab = "Joueur",
    imguiMissingLogged = false,
    defaultValues = {
        walkSpeed = nil,
        sprintSpeed = nil,
        jumpZVelocity = nil,
        gravityScale = nil,
    },
    pendingValues = {},
    timeFrozen = false,
    frozenTime = 12.0,
}

-- ============================================================================
-- UTILITAIRES
-- ============================================================================

local function log(msg)
    if DEBUG then
        print(string.format("%s %s", MOD_PREFIX, tostring(msg)))
    end
end

local function err(msg)
    print(string.format("%s [ERREUR] %s", MOD_PREFIX, tostring(msg)))
end

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
    local ok, cur = pcall(function() return current[parts[#parts]] end)
    if not ok or cur == nil then return false end
    local ok = pcall(function() current[parts[#parts]] = value end)
    return ok
end

local function safeGet(obj, path)
    if not obj then return nil end
    local parts = {}
    for part in string.gmatch(path, "[^.]+") do
        table.insert(parts, part)
    end
    local current = obj
    for _, part in ipairs(parts) do
        local ok, nextObj = pcall(function() return current[part] end)
        if not ok or nextObj == nil then return nil end
        current = nextObj
    end
    return current
end

local function callFirstMethod(obj, methodList, ...)
    if not isValid(obj) then return end
    local args = {...}
    local nArgs = select('#', ...)
    for _, method in ipairs(methodList) do
        local ok, res = pcall(function()
            if obj[method] then
                obj[method](obj, table.unpack(args, 1, nArgs))
                return true
            end
            return false
        end)
        if ok and res then
            return true
        end
    end
    return false
end

-- ============================================================================
-- ACCESSEURS JEU
-- ============================================================================

local function getPlayerController()
    local pcs = FindAllOf("PlayerController")
    if pcs then
        for _, pc in ipairs(pcs) do
            if isValid(pc) then return pc end
        end
    end
    local pc = FindFirstOf("PlayerController")
    return isValid(pc) and pc or nil
end

local function getPlayerCharacter()
    local controller = getPlayerController()
    if isValid(controller) then
        local char = safeGet(controller, "Pawn")
        if isValid(char) then return char end
    end

    local candidates = {
        "PalPlayerCharacter",
        "BP_PlayerCharacter_Female_C",
        "BP_PlayerCharacter_Male_C",
    }
    for _, className in ipairs(candidates) do
        local obj = FindFirstOf(className)
        if isValid(obj) then return obj end
    end
    return nil
end

local function getPlayerState()
    local controller = getPlayerController()
    if not isValid(controller) then return nil end
    return safeGet(controller, "PlayerState")
end

local function getPlayerParameter()
    local player = getPlayerCharacter()
    if not isValid(player) then return nil end
    return safeGet(player, "CharacterParameterComponent") or safeGet(player, "ParameterComponent")
end

local function getMovementComponent()
    local player = getPlayerCharacter()
    if not isValid(player) then return nil end
    return safeGet(player, "CharacterMovement")
end

local function getGameSetting()
    local setting = FindFirstOf("PalGameSetting")
    if isValid(setting) then return setting end
    setting = FindFirstOf("BP_PalGameSetting_C")
    if isValid(setting) then return setting end
    return nil
end

local function getWorld()
    local player = getPlayerCharacter()
    if isValid(player) then
        local ok, world = pcall(function() return player:GetWorld() end)
        if ok and isValid(world) then return world end
        return safeGet(player, "World")
    end
    local world = FindFirstOf("World")
    return isValid(world) and world or nil
end

local function getGameState()
    local world = getWorld()
    if world then
        local gs = safeGet(world, "GameState")
        if isValid(gs) then return gs end
    end
    local gs = FindFirstOf("PalGameState") or FindFirstOf("GameState")
    return isValid(gs) and gs or nil
end

local function getPals()
    local pals = {}
    local found = FindAllOf("PalCharacter")
    if found then
        for _, p in ipairs(found) do
            if isValid(p) and p ~= getPlayerCharacter() then
                table.insert(pals, p)
            end
        end
    end
    return pals
end

-- ============================================================================
-- CHEATS JOUEUR
-- ============================================================================

local function cacheMovementDefaults()
    if state.defaultValues.walkSpeed ~= nil then return end
    local movement = getMovementComponent()
    if not movement then return end

    local ok, walk = pcall(function() return movement.MaxWalkSpeed end)
    if ok and walk then state.defaultValues.walkSpeed = walk end

    local ok2, sprint = pcall(function() return movement.MaxSprintSpeed or movement.MaxRunSpeed end)
    if ok2 and sprint then state.defaultValues.sprintSpeed = sprint end

    local ok3, jump = pcall(function() return movement.JumpZVelocity end)
    if ok3 and jump then state.defaultValues.jumpZVelocity = jump end

    local ok4, gravity = pcall(function() return movement.GravityScale end)
    if ok4 and gravity then state.defaultValues.gravityScale = gravity end
end

local function applyGodMode()
    if not config.cheats.godMode then return end
    local param = getPlayerParameter()
    if not isValid(param) then return end
    -- 1.0 : invincibilite via Muteki + protection anti-mort
    pcall(function() param:SetMuteki("Cheat", true) end)
    safeSet(param, "bIsEnableMuteki", true)
    safeSet(param, "bIsDebugMuteki", true)
    pcall(function() param:ResetDyingHP() end)
    -- Refill HP si possible (FFixedPoint64 via .Value)
    local saveParam = safeGet(param, "IndividualParameter.SaveParameter")
    if saveParam then
        local maxHP = safeGet(saveParam, "MaxHP")
        if maxHP and maxHP.Value then
            pcall(function() saveParam.HP.Value = maxHP.Value end)
        end
    end
end

local function applyUnlimitedStamina()
    if not config.cheats.unlimitedStamina then return end
    local param = getPlayerParameter()
    if not isValid(param) then return end
    -- 1.0 : ResetSP remet a fond, on supprime l'overheat
    pcall(function() param:ResetSP() end)
    safeSet(param, "IsSPOverheat", false)
    local saveParam = safeGet(param, "IndividualParameter.SaveParameter")
    if saveParam then
        local maxSP = safeGet(saveParam, "MaxSP")
        if maxSP and maxSP.Value then
            pcall(function() saveParam.SP.Value = maxSP.Value end)
        end
    end
end

local function applyUnlimitedHungerThirst()
    if not config.cheats.unlimitedHungerThirst then return end
    local param = getPlayerParameter()
    if not isValid(param) then return end
    local saveParam = safeGet(param, "IndividualParameter.SaveParameter")
    if saveParam then
        local maxFull = safeGet(saveParam, "MaxFullStomach") or 100
        pcall(function() saveParam.FullStomach = maxFull end)
        safeSet(saveParam, "FullStomach", maxFull)
        safeSet(saveParam, "HungerType", 0)
    end
    -- Fallbacks
    safeSet(param, "FullStomach", 100)
    safeSet(param, "Hunger", 100)
    safeSet(param, "Thirst", 100)
end

local function applySuperSpeed()
    local movement = getMovementComponent()
    if not movement then return end
    cacheMovementDefaults()

    if config.cheats.superSpeed then
        if state.defaultValues.walkSpeed then
            safeSet(movement, "MaxWalkSpeed", state.defaultValues.walkSpeed * config.values.superSpeedMultiplier)
        end
        if state.defaultValues.sprintSpeed then
            safeSet(movement, "MaxSprintSpeed", state.defaultValues.sprintSpeed * config.values.superSpeedMultiplier)
            safeSet(movement, "SprintMaxSpeed", state.defaultValues.sprintSpeed * config.values.superSpeedMultiplier)
            safeSet(movement, "MaxRunSpeed", state.defaultValues.sprintSpeed * config.values.superSpeedMultiplier)
        end
    else
        if state.defaultValues.walkSpeed then
            safeSet(movement, "MaxWalkSpeed", state.defaultValues.walkSpeed)
        end
        if state.defaultValues.sprintSpeed then
            safeSet(movement, "MaxSprintSpeed", state.defaultValues.sprintSpeed)
            safeSet(movement, "SprintMaxSpeed", state.defaultValues.sprintSpeed)
            safeSet(movement, "MaxRunSpeed", state.defaultValues.sprintSpeed)
        end
    end
end

local function applySuperJump()
    local movement = getMovementComponent()
    if not movement then return end
    cacheMovementDefaults()

    if config.cheats.superJump then
        safeSet(movement, "JumpZVelocity", config.values.superJumpVelocity)
    else
        if state.defaultValues.jumpZVelocity then
            safeSet(movement, "JumpZVelocity", state.defaultValues.jumpZVelocity)
        end
    end
end

local function applyNoFallDamage()
    local movement = getMovementComponent()
    if not movement then return end
    if config.cheats.noFallDamage then
        safeSet(movement, "MaxFallVelocity", 9999999)
        safeSet(movement, "JumpOffJumpZFactor", 0)
        safeSet(movement, "bNotifyApex", false)
        safeSet(movement, "AirControl", 1.0)
    elseif state.defaultValues.gravityScale then
        -- restauration impossible sans backup exact
    end
end

local function applyFlyNoClip()
    local movement = getMovementComponent()
    if not movement then return end
    if config.cheats.fly or config.cheats.noClip then
        safeSet(movement, "MovementMode", 5) -- MOVE_Flying
        safeSet(movement, "GravityScale", 0.0)
    else
        safeSet(movement, "MovementMode", 1) -- MOVE_Walking
        if state.defaultValues.gravityScale then
            safeSet(movement, "GravityScale", state.defaultValues.gravityScale)
        else
            safeSet(movement, "GravityScale", 1.0)
        end
    end
end

local function applyInvisibility()
    local player = getPlayerCharacter()
    if not isValid(player) then return end
    local mesh = safeGet(player, "Mesh")
    if mesh then
        safeSet(mesh, "bOwnerNoSee", config.cheats.invisibility)
        safeSet(mesh, "bVisible", not config.cheats.invisibility)
        pcall(function() player:SetActorHiddenInGame(config.cheats.invisibility) end)
    end
end

local function addExperience(amount)
    local playerState = getPlayerState()
    if not playerState then return end
    callFirstMethod(playerState,
        { "AddExp", "AddExperience", "AddPlayerExp", "GainExp", "Server_AddExperience" },
        amount)
    log("Experience ajoutee : " .. amount)
end

local function addTechPoints(amount)
    local playerState = getPlayerState()
    if not playerState then return end
    callFirstMethod(playerState,
        { "AddTechnologyPoint", "AddTechPoint", "AddTech", "GainTechnologyPoint", "Server_AddTechnologyPoint" },
        amount)
    log("Points tech ajoutes : " .. amount)
end

local function setTimeOfDay(hour)
    local gameState = getGameState()
    if not gameState then
        err("GameState non disponible pour regler l'heure.")
        return
    end
    callFirstMethod(gameState, { "SetTimeOfDay", "ServerSetTimeOfDay", "SetWorldTime" }, hour)
    log("Heure reglee a : " .. hour)
end

local function freezeTime()
    if not config.cheats.freezeTime then return end
    local gameState = getGameState()
    if not gameState then return end
    safeSet(gameState, "GlobalTimeDilation", 1.0)
    -- certaines builds Palworld utilisent une variable TimeOfDay
    state.frozenTime = safeGet(gameState, "TimeOfDay") or state.frozenTime
    safeSet(gameState, "TimeOfDay", state.frozenTime)
end

local function teleportForward(distance)
    local player = getPlayerCharacter()
    if not player then return end
    local loc = player:GetActorLocation()
    local rot = player:GetActorRotation()
    if not loc or not rot then return end
    local rad = math.rad(rot.Yaw)
    local nx = loc.X + math.cos(rad) * distance
    local ny = loc.Y + math.sin(rad) * distance
    local ok = pcall(function()
        player:SetActorLocation({ X = nx, Y = ny, Z = loc.Z })
    end)
    if ok then
        log(string.format("Teleporte a X:%.0f Y:%.0f Z:%.0f", nx, ny, loc.Z))
    else
        err("Teleporation echouee.")
    end
end

local function unlockFastTravel()
    local gameState = getGameState()
    if not gameState then return end
    callFirstMethod(gameState, { "UnlockAllFastTravelPoint", "UnlockAllFastTravel" })
    log("Tous les points de voyage rapide debloques.")
end

local function setWeatherClear()
    local gameState = getGameState()
    if not gameState then return end
    callFirstMethod(gameState, { "SetWeather", "ChangeWeather", "SetWeatherClear" })
    log("Meteo reglee.")
end

-- ============================================================================
-- CHEATS PAL
-- ============================================================================

local function applyPalGodMode()
    if not config.cheats.palGodMode then return end
    for _, pal in ipairs(getPals()) do
        local param = safeGet(pal, "CharacterParameterComponent")
        if isValid(param) then
            pcall(function() param:SetMuteki("Cheat", true) end)
            safeSet(param, "bIsEnableMuteki", true)
            safeSet(param, "bIsDebugMuteki", true)
            local saveParam = safeGet(param, "IndividualParameter.SaveParameter")
            if saveParam then
                local maxHP = safeGet(saveParam, "MaxHP")
                if maxHP and maxHP.Value then
                    pcall(function() saveParam.HP.Value = maxHP.Value end)
                end
            end
        end
    end
end

local function applyPalSanity()
    if not config.cheats.palSanity then return end
    for _, pal in ipairs(getPals()) do
        local param = safeGet(pal, "CharacterParameterComponent")
        if isValid(param) then
            local saveParam = safeGet(param, "IndividualParameter.SaveParameter")
            if saveParam then
                safeSet(saveParam, "SanityValue", 100)
            end
            safeSet(pal, "Sanity", 100)
            safeSet(pal, "SanityValue", 100)
        end
    end
end

local function applyInstantHatch()
    if not config.cheats.instantHatch then return end
    local classNames = {
        "PalMapObjectHatchingEggModel",
        "PalMapObjectHatchingEggParameterComponent",
        "PalMapObjectPalEgg",
        "PalItem_Egg",
        "BP_PalEgg_C",
    }
    for _, className in ipairs(classNames) do
        local all = FindAllOf and FindAllOf(className)
        if all then
            for _, egg in ipairs(all) do
                if isValid(egg) then
                    safeSet(egg, "AutoWorkAmountBySec", 999999)
                    safeSet(egg, "bWorkable", true)
                    safeSet(egg, "CurrentPalEggTemperatureDiff", 0)
                    pcall(function() egg:UpdateWorkAmountBySec(999999) end)
                    pcall(function() egg:SetTemperatureDiff(0) end)
                    callFirstMethod(egg, { "Hatch", "InstantHatch", "ForceHatch", "ObtainHatchedCharacter_ServerInternal" })
                end
            end
        end
    end
end

local function applyMaxPalStats()
    if not config.cheats.maxPalStats then return end
    for _, pal in ipairs(getPals()) do
        local param = safeGet(pal, "CharacterParameterComponent")
        if isValid(param) then
            local saveParam = safeGet(param, "IndividualParameter.SaveParameter")
            if saveParam then
                safeSet(saveParam, "Talent_HP", 100)
                safeSet(saveParam, "Talent_Melee", 100)
                safeSet(saveParam, "Talent_Shot", 100)
                safeSet(saveParam, "Talent_Defense", 100)
                safeSet(saveParam, "Support", 100)
                safeSet(saveParam, "CraftSpeed", 99999)
                safeSet(saveParam, "SanityValue", 100)
            end
            safeSet(param, "WorkSpeed", 99999)
            safeSet(param, "CraftSpeed", 99999)
            safeSet(param, "MoveSpeed", 99999)
        end
    end
end

-- ============================================================================
-- COMBAT / INVENTAIRE
-- ============================================================================

local function applyNoReloadPassive()
    if not config.cheats.noReload and not config.cheats.unlimitedAmmo then return end
    local player = getPlayerCharacter()
    if not isValid(player) then return end
    local shooter = safeGet(player, "ShooterComponent") or safeGet(player, "WeaponHolderComponent") or safeGet(player, "PalShooterComponent")
    if not shooter then
        -- Recherche par classe si le composant n'est pas dans le personnage
        shooter = FindFirstOf("PalShooterComponent")
    end
    if not isValid(shooter) then return end
    local weapon = safeGet(shooter, "HasWeapon") or safeGet(shooter, "CurrentWeapon") or safeGet(shooter, "CacheNextWeapon")
    if isValid(weapon) then
        local maxAmmo = safeGet(weapon, "MaxAmmo") or safeGet(weapon, "MaxMagazine") or safeGet(weapon, "MagazineSize") or 9999
        if maxAmmo then
            safeSet(weapon, "CurrentAmmo", maxAmmo)
            safeSet(weapon, "Ammo", maxAmmo)
            safeSet(weapon, "RemainingAmmo", maxAmmo)
            safeSet(weapon, "LoadedAmmo", maxAmmo)
        end
    end
    -- Pas de rechargement force
    safeSet(shooter, "bIsReloading", false)
end

local function applyUnlimitedItemsPassive()
    if not config.cheats.unlimitedItems then return end
    local playerState = getPlayerState()
    if not playerState then return end
    local inventory = safeGet(playerState, "Inventory") or safeGet(playerState, "ItemContainer")
    if not inventory then return end
    -- Placeholder : iterer les slots et forcer la durabilite au max
end

local function duplicateItems()
    if not config.cheats.duplicateItems then return end
    local playerState = getPlayerState()
    if not playerState then return end
    local inventory = safeGet(playerState, "Inventory") or safeGet(playerState, "ItemContainer")
    if inventory then
        callFirstMethod(inventory, { "Duplicate", "DuplicateAllItems" })
        log("Duplication d'items demandee.")
    end
end

-- ============================================================================
-- FONCTIONS AVANCEES
-- ============================================================================

local function addLevels(count)
    local playerState = getPlayerState()
    if not playerState then return end
    callFirstMethod(playerState,
        { "AddLevel", "Server_AddLevel", "LevelUp", "AddPlayerLevel" },
        count)
    log("Niveaux ajoutes : " .. count)
end

local function spawnItem(itemId, count)
    local player = getPlayerCharacter()
    if not player then return end
    local inventory = safeGet(getPlayerState(), "Inventory") or safeGet(getPlayerState(), "ItemContainer")
    if inventory then
        callFirstMethod(inventory,
            { "AddItem", "Server_AddItem", "GiveItem", "AddItemByName" },
            itemId, count)
    end
    log(string.format("Spawn item : %s x%d", tostring(itemId), count))
end

local function revealMap()
    local gameState = getGameState()
    if not gameState then return end
    callFirstMethod(gameState, { "RevealMap", "UnlockAllMap", "Server_RevealMap" })
    log("Carte devoilee.")
end

local function applyPalFastWork()
    if not config.cheats.palFastWork then return end
    for _, pal in ipairs(getPals()) do
        local param = safeGet(pal, "CharacterParameterComponent")
        if isValid(param) then
            local saveParam = safeGet(param, "IndividualParameter.SaveParameter")
            if saveParam then
                safeSet(saveParam, "CraftSpeed", 99999)
            end
            safeSet(param, "WorkSpeed", 99999)
            safeSet(param, "CraftSpeed", 99999)
        end
    end
end

local function applyPalNoHunger()
    if not config.cheats.palNoHunger then return end
    for _, pal in ipairs(getPals()) do
        local param = safeGet(pal, "CharacterParameterComponent")
        if isValid(param) then
            local saveParam = safeGet(param, "IndividualParameter.SaveParameter")
            if saveParam then
                local maxFull = safeGet(saveParam, "MaxFullStomach") or 100
                pcall(function() saveParam.FullStomach = maxFull end)
                safeSet(saveParam, "FullStomach", maxFull)
                safeSet(saveParam, "SanityValue", 100)
            end
            safeSet(pal, "Hunger", 100)
            safeSet(pal, "Sanity", 100)
            safeSet(pal, "SanityValue", 100)
            safeSet(pal, "FullStomach", 100)
        end
    end
end

local function registerSafeHook(functionPath, callback)
    if not _RegisterHook then return end
    local ok, err = pcall(_RegisterHook, functionPath, callback)
    if not ok then
        log("Hook ignore : " .. tostring(functionPath) .. " : " .. tostring(err))
    end
end

-- ============================================================================
-- HOOKS AVANCES
-- ============================================================================

-- Les hooks UFunctions /Script/Pal.* sont obsoletes en 1.0 : remplaces par des boucles/property edits
local function setupAdvancedHooks()
    -- Plus de RegisterHook ici : apply*LoopAsync s'en charge
end

-- ============================================================================
-- MENU IMGUI MAX
-- ============================================================================

local function setCategory(category, value)
    local map = {
        player = { "godMode", "unlimitedStamina", "unlimitedHungerThirst", "noFallDamage", "fly", "noClip", "invisibility", "superSpeed", "superJump" },
        combat = { "unlimitedAmmo", "noReload", "oneHitKill", "noRecoil", "noSpread", "rapidFire", "maxCaptureRate" },
        inventory = { "unlimitedItems", "instantCraft", "ignoreCost", "duplicateItems", "infiniteDurability" },
        pal = { "palGodMode", "palSanity", "instantHatch", "maxPalStats", "palFastWork", "palNoHunger" },
        world = { "freezeTime", "revealMap" },
    }
    for _, key in ipairs(map[category] or {}) do
        config.cheats[key] = value
    end
    log((value and "Categorie activee : " or "Categorie desactivee : ") .. category)
end

local function drawHeader()
    ImGui.Text("PAL CHEAT MENU - ULTRA MAX")
    ImGui.Separator()

    if ImGui.Button("Tout activer") then
        for k, _ in pairs(config.cheats) do
            config.cheats[k] = true
        end
        log("Tous les cheats actives.")
    end
    ImGui.SameLine()
    if ImGui.Button("Tout desactiver") then
        for k, _ in pairs(config.cheats) do
            config.cheats[k] = false
        end
        log("Tous les cheats desactives.")
    end
    ImGui.SameLine()
    if ImGui.Button("PvP") then
        setCategory("combat", true)
        setCategory("player", true)
    end
    ImGui.SameLine()
    if ImGui.Button("Builder") then
        setCategory("inventory", true)
        setCategory("pal", true)
        setCategory("world", true)
    end
    ImGui.SameLine()
    if ImGui.Button("Fermer (F1)") then
        state.showMenu = false
    end
end

local function drawPlayerTab()
    local changed, value

    changed, value = ImGui.Checkbox("God Mode", config.cheats.godMode)
    if changed then config.cheats.godMode = value end

    changed, value = ImGui.Checkbox("Endurance infinie", config.cheats.unlimitedStamina)
    if changed then config.cheats.unlimitedStamina = value end

    changed, value = ImGui.Checkbox("Faim / Soif infinies", config.cheats.unlimitedHungerThirst)
    if changed then config.cheats.unlimitedHungerThirst = value end

    changed, value = ImGui.Checkbox("Pas de degats de chute", config.cheats.noFallDamage)
    if changed then config.cheats.noFallDamage = value end

    changed, value = ImGui.Checkbox("Vol (Fly)", config.cheats.fly)
    if changed then
        config.cheats.fly = value
        if value then config.cheats.noClip = false end
    end

    changed, value = ImGui.Checkbox("No Clip", config.cheats.noClip)
    if changed then
        config.cheats.noClip = value
        if value then config.cheats.fly = false end
    end

    changed, value = ImGui.Checkbox("Invisibilite", config.cheats.invisibility)
    if changed then config.cheats.invisibility = value end

    ImGui.Separator()
    ImGui.Text("Multiplicateurs")

    changed, value = ImGui.SliderFloat("Vitesse x", config.values.superSpeedMultiplier,
        config.limits.minSpeedMultiplier, config.limits.maxSpeedMultiplier)
    if changed then config.values.superSpeedMultiplier = value end

    changed, value = ImGui.SliderFloat("Saut", config.values.superJumpVelocity,
        config.limits.minJumpVelocity, config.limits.maxJumpVelocity)
    if changed then config.values.superJumpVelocity = value end

    changed, value = ImGui.Checkbox("Super vitesse", config.cheats.superSpeed)
    if changed then config.cheats.superSpeed = value end

    changed, value = ImGui.Checkbox("Super saut", config.cheats.superJump)
    if changed then config.cheats.superJump = value end
end

local function drawCombatTab()
    local changed, value

    changed, value = ImGui.Checkbox("Munitions infinies", config.cheats.unlimitedAmmo)
    if changed then config.cheats.unlimitedAmmo = value end

    changed, value = ImGui.Checkbox("Pas de rechargement", config.cheats.noReload)
    if changed then config.cheats.noReload = value end

    changed, value = ImGui.Checkbox("One-Hit Kill", config.cheats.oneHitKill)
    if changed then config.cheats.oneHitKill = value end

    changed, value = ImGui.Checkbox("No Recoil", config.cheats.noRecoil)
    if changed then config.cheats.noRecoil = value end

    changed, value = ImGui.Checkbox("No Spread", config.cheats.noSpread)
    if changed then config.cheats.noSpread = value end

    changed, value = ImGui.Checkbox("Rapid Fire", config.cheats.rapidFire)
    if changed then config.cheats.rapidFire = value end

    changed, value = ImGui.SliderFloat("Cadence x", config.values.rapidFireRate,
        config.limits.minRapidFire, config.limits.maxRapidFire)
    if changed then config.values.rapidFireRate = value end

    changed, value = ImGui.Checkbox("Taux capture max", config.cheats.maxCaptureRate)
    if changed then config.cheats.maxCaptureRate = value end

    changed, value = ImGui.SliderFloat("Capture x", config.values.captureRate,
        config.limits.minCaptureRate, config.limits.maxCaptureRate)
    if changed then config.values.captureRate = value end
end

local function drawInventoryTab()
    local changed, value

    changed, value = ImGui.Checkbox("Objets infinis", config.cheats.unlimitedItems)
    if changed then config.cheats.unlimitedItems = value end

    changed, value = ImGui.Checkbox("Craft instantane", config.cheats.instantCraft)
    if changed then config.cheats.instantCraft = value end

    changed, value = ImGui.Checkbox("Ignorer couts", config.cheats.ignoreCost)
    if changed then config.cheats.ignoreCost = value end

    changed, value = ImGui.Checkbox("Dupliquer items", config.cheats.duplicateItems)
    if changed then config.cheats.duplicateItems = value end

    changed, value = ImGui.Checkbox("Durabilite infinie", config.cheats.infiniteDurability)
    if changed then config.cheats.infiniteDurability = value end

    if ImGui.Button("Dupliquer maintenant") then
        duplicateItems()
    end
    ImGui.SameLine()
    if ImGui.Button("Reveler map") then
        revealMap()
    end
end

local function drawPalTab()
    local changed, value

    changed, value = ImGui.Checkbox("Pal God Mode", config.cheats.palGodMode)
    if changed then config.cheats.palGodMode = value end

    changed, value = ImGui.Checkbox("Pal Sanity infinie", config.cheats.palSanity)
    if changed then config.cheats.palSanity = value end

    changed, value = ImGui.Checkbox("Eclosion instantanee", config.cheats.instantHatch)
    if changed then config.cheats.instantHatch = value end

    changed, value = ImGui.Checkbox("Stats Pals max", config.cheats.maxPalStats)
    if changed then config.cheats.maxPalStats = value end

    changed, value = ImGui.Checkbox("Pals travail ultra rapide", config.cheats.palFastWork)
    if changed then config.cheats.palFastWork = value end

    changed, value = ImGui.Checkbox("Pals pas faim", config.cheats.palNoHunger)
    if changed then config.cheats.palNoHunger = value end
end

local function drawWorldTab()
    if ImGui.Button("Regler l'heure") then
        setTimeOfDay(config.values.timeHour)
    end
    ImGui.SameLine()
    local changed, value = ImGui.SliderFloat("Heure", config.values.timeHour,
        config.limits.minTimeHour, config.limits.maxTimeHour)
    if changed then config.values.timeHour = value end

    changed, value = ImGui.Checkbox("Freeze Time", config.cheats.freezeTime)
    if changed then config.cheats.freezeTime = value end

    if ImGui.Button("+ Experience") then
        addExperience(config.values.experienceAmount)
    end
    ImGui.SameLine()
    changed, value = ImGui.SliderInt("XP", config.values.experienceAmount,
        config.limits.minExperience, config.limits.maxExperience)
    if changed then config.values.experienceAmount = value end

    if ImGui.Button("+ Points tech") then
        addTechPoints(config.values.techPointsAmount)
    end
    ImGui.SameLine()
    changed, value = ImGui.SliderInt("Tech", config.values.techPointsAmount,
        config.limits.minTechPoints, config.limits.maxTechPoints)
    if changed then config.values.techPointsAmount = value end

    if ImGui.Button("Unlock Fast Travel") then
        unlockFastTravel()
    end
    ImGui.SameLine()
    if ImGui.Button("Teleporter devant") then
        teleportForward(config.values.teleportDistance)
    end
    ImGui.SameLine()
    if ImGui.Button("Meteo claire") then
        setWeatherClear()
    end

    changed, value = ImGui.Checkbox("Reveler carte", config.cheats.revealMap)
    if changed then
        config.cheats.revealMap = value
        if value then revealMap() end
    end

    ImGui.Separator()
    ImGui.Text("Spawn item")
    local changedText, newText = ImGui.InputText("ID Item", config.values.spawnItemId, 64)
    if changedText then config.values.spawnItemId = newText end

    changed, value = ImGui.SliderInt("Quantite", config.values.spawnItemCount,
        config.limits.minSpawnCount, config.limits.maxSpawnCount)
    if changed then config.values.spawnItemCount = value end

    if ImGui.Button("Spawner") then
        spawnItem(config.values.spawnItemId, config.values.spawnItemCount)
    end
    ImGui.SameLine()
    if ImGui.Button("+ Niveaux") then
        addLevels(config.values.addLevelCount)
    end
    ImGui.SameLine()
    changed, value = ImGui.SliderInt("Niveaux", config.values.addLevelCount,
        config.limits.minAddLevel, config.limits.maxAddLevel)
    if changed then config.values.addLevelCount = value end
end

local function drawMenu()
    if not state.showMenu then return end
    if not ImGui or not ImGui.Begin then
        if not state.imguiMissingLogged then
            err("ImGui global absent: menu personnalise impossible avec cette version d'UE4SS")
            state.imguiMissingLogged = true
        end
        return
    end
    state.imguiMissingLogged = false

    -- ImGuiCond_FirstUseEver = 4 ; certains builds n'exposent pas la constante globale
    local cond = (ImGui.Cond and ImGui.Cond.FirstUseEver) or 4
    ImGui.SetNextWindowSize(700, 650, cond)

    if ImGui.Begin("PalCheatMenu MAX") then
        drawHeader()

        if ImGui.BeginTabBar("Tabs", 0) then
            if ImGui.BeginTabItem("Joueur") then
                drawPlayerTab()
                ImGui.EndTabItem()
            end
            if ImGui.BeginTabItem("Combat") then
                drawCombatTab()
                ImGui.EndTabItem()
            end
            if ImGui.BeginTabItem("Inventaire") then
                drawInventoryTab()
                ImGui.EndTabItem()
            end
            if ImGui.BeginTabItem("Pals") then
                drawPalTab()
                ImGui.EndTabItem()
            end
            if ImGui.BeginTabItem("Monde") then
                drawWorldTab()
                ImGui.EndTabItem()
            end
            ImGui.EndTabBar()
        end
    end

    ImGui.End()
end

local function openGameCheatMenu()
    pcall(function()
        local pc = getPlayerController()
        if not isValid(pc) then
            err("PlayerController indisponible pour ouvrir DebugWindow")
            return
        end
        pcall(function() pc:EnableCheats() end)
        local cm = safeGet(pc, "CheatManager")
        if isValid(cm) and cm.DebugWindow then
            pcall(function() cm:DebugWindow() end)
            log("Fenetre de triche integree ouverte.")
        else
            err("CheatManager.DebugWindow indisponible")
        end
    end)
end

-- ============================================================================
-- INITIALISATION
-- ============================================================================

local function init()
    if state.initialized then return end
    log("Initialisation PalCheatMenu MAX...")
    cacheMovementDefaults()
    setupAdvancedHooks()
    state.initialized = true
    log("Initialisation terminee.")
end

-- ============================================================================
-- HOOKS PRINCIPAUX
-- ============================================================================

registerSafeHook("/Script/Engine.PlayerController:ClientRestart", function()
    ExecuteInGameThread(function() init() end)
end)

registerSafeHook("/Script/Engine.PlayerController:ServerAcknowledgePossession", function()
    if not state.initialized then
        ExecuteInGameThread(function() init() end)
    end
end)

-- helper pour executer une fonction en toute securite dans la boucle jeu

-- ============================================================================
-- COMMUNICATION APP EXTERNE PALTRAINER
-- ============================================================================

local function getIPC()
    if not PalTrainerIPC then return nil end
    PalTrainerIPC.ensureDir()
    return PalTrainerIPC
end

local function pollTrainer()
    local ipc = getIPC()
    if not ipc then return end

    local cmds = ipc.pollCommands()

    -- Applique les toggles
    if cmds then
        for field, _ in pairs(config.cheats) do
            local val = ipc.getCommand(cmds, "cheat." .. field)
            if val ~= nil then
                if val == "toggle" then
                    config.cheats[field] = not config.cheats[field]
                else
                    config.cheats[field] = val
                end
            end
        end

        -- Applique les valeurs
        for field, _ in pairs(config.values) do
            local raw = cmds["value." .. field]
            if raw ~= nil then
                local num = tonumber(raw)
                config.values[field] = num and num or raw
            end
        end

        -- Actions one-shot
        local consumed = {}
        local function tryAction(key, fn)
            if cmds[key] then
                consumed[key] = true
                safeCall(fn)
            end
        end
        tryAction("action.addExperience", function() addExperience(config.values.experienceAmount) end)
        tryAction("action.addTechPoints", function() addTechPoints(config.values.techPointsAmount) end)
        tryAction("action.setDayTime", function() setTimeOfDay(config.values.timeHour) end)
        tryAction("action.unlockFastTravel", unlockFastTravel)
        tryAction("action.teleportForward", function() teleportForward(config.values.teleportDistance) end)
        tryAction("action.setWeatherClear", setWeatherClear)
        tryAction("action.addLevels", function() addLevels(config.values.addLevelCount) end)
        tryAction("action.spawnItem", function() spawnItem(config.values.spawnItemId, config.values.spawnItemCount) end)

        if next(consumed) then
            ipc.consumeKeys(consumed)
        end
    end

    -- Ecrit le status pour l'app externe
    local status = { ["PalCheatMenu.ready"] = true }
    for k, v in pairs(config.cheats) do status["cheat." .. k] = v end
    for k, v in pairs(config.values) do status["value." .. k] = v end
    ipc.writeStatus(status)
end

local function safeCall(fn)
    local ok, e = pcall(fn)
    if not ok then
        err(tostring(e))
    end
end

local cheatFunctions = {
    applyGodMode, applyUnlimitedStamina, applyUnlimitedHungerThirst,
    applySuperSpeed, applySuperJump, applyNoFallDamage, applyFlyNoClip,
    applyInvisibility, applyNoReloadPassive, applyUnlimitedItemsPassive,
    applyPalGodMode, applyPalSanity, applyInstantHatch, applyMaxPalStats,
    applyPalFastWork, applyPalNoHunger, freezeTime,
}

local function mainLoop()
    if not state.initialized then
        safeCall(init)
    end

    for _, fn in ipairs(cheatFunctions) do
        safeCall(fn)
    end

    safeCall(pollTrainer)
end

-- UE4SS experimental-latest : boucle unique sur le game thread
if LoopInGameThreadWithDelay and type(LoopInGameThreadWithDelay) == "function" then
    LoopInGameThreadWithDelay(100, function()
        mainLoop()
    end)
else
    -- fallback anciennes versions
    LoopAsync(500, function()
        mainLoop()
        return false
    end)
end

-- ============================================================================
-- RACCOURCIS CLAVIER
-- ============================================================================

pcall(function() RegisterKeyBind(config.keys.toggleMenu, function()
    log("PalTrainerApp: ouvre PalTrainerApp\run.bat pour le menu externe")
end) end)

pcall(function() RegisterKeyBind(config.keys.toggleAll, function()
    local allOn = true
    for _, v in pairs(config.cheats) do
        if not v then allOn = false; break end
    end
    for k, _ in pairs(config.cheats) do
        config.cheats[k] = not allOn
    end
    log(not allOn and "Tout active" or "Tout desactive")
end) end)

local keyMap = {
    toggleGodMode = { "godMode", "God Mode" },
    toggleStamina = { "unlimitedStamina", "Endurance infinie" },
    toggleHungerThirst = { "unlimitedHungerThirst", "Faim/Soif" },
    toggleNoFallDamage = { "noFallDamage", "Pas de degats de chute" },
    toggleFly = { "fly", "Vol" },
    toggleNoClip = { "noClip", "No Clip" },
    toggleInvisibility = { "invisibility", "Invisibilite" },
    toggleSpeed = { "superSpeed", "Super vitesse" },
    toggleJump = { "superJump", "Super saut" },
    toggleAmmo = { "unlimitedAmmo", "Munitions infinies" },
    toggleNoReload = { "noReload", "Pas de rechargement" },
    toggleOneHitKill = { "oneHitKill", "One-Hit Kill" },
    toggleInstantCraft = { "instantCraft", "Craft instantane" },
    toggleItems = { "unlimitedItems", "Objets infinis" },
    toggleIgnoreCost = { "ignoreCost", "Ignorer couts" },
    toggleDuplicate = { "duplicateItems", "Dupliquer" },
    togglePalGodMode = { "palGodMode", "Pal God Mode" },
    togglePalSanity = { "palSanity", "Pal Sanity" },
    toggleInstantHatch = { "instantHatch", "Eclosion instantanee" },
    toggleMaxPalStats = { "maxPalStats", "Stats Pals max" },
    togglePalFastWork = { "palFastWork", "Pals travail rapide" },
    togglePalNoHunger = { "palNoHunger", "Pals pas faim" },
    toggleTimeFreeze = { "freezeTime", "Freeze Time" },
    toggleNoRecoil = { "noRecoil", "No Recoil" },
    toggleNoSpread = { "noSpread", "No Spread" },
    toggleRapidFire = { "rapidFire", "Rapid Fire" },
    toggleInfiniteDurability = { "infiniteDurability", "Durabilite infinie" },
    toggleRevealMap = { "revealMap", "Reveler carte" },
    toggleMaxCaptureRate = { "maxCaptureRate", "Taux capture max" },
}

local function registerToggleKey(keySetting, mapping)
    local key = config.keys[keySetting]
    if not key then return end
    pcall(function() RegisterKeyBind(key, function()
        local field, label = mapping[1], mapping[2]
        config.cheats[field] = not config.cheats[field]
        log(label .. " " .. (config.cheats[field] and "ON" or "OFF"))
    end) end)
end

for keySetting, mapping in pairs(keyMap) do
    registerToggleKey(keySetting, mapping)
end

pcall(function() RegisterKeyBind(config.keys.addExperience, function()
    addExperience(config.values.experienceAmount)
end) end)

pcall(function() RegisterKeyBind(config.keys.addTechPoints, function()
    addTechPoints(config.values.techPointsAmount)
end) end)

pcall(function() RegisterKeyBind(config.keys.setDayTime, function()
    setTimeOfDay(config.values.timeHour)
end) end)

pcall(function() RegisterKeyBind(config.keys.unlockFastTravel, function()
    unlockFastTravel()
end) end)

pcall(function() RegisterKeyBind(config.keys.teleportForward, function()
    teleportForward(config.values.teleportDistance)
end) end)

pcall(function() RegisterKeyBind(config.keys.setWeatherClear, function()
    setWeatherClear()
end) end)

pcall(function() RegisterKeyBind(config.keys.addLevels, function()
    addLevels(config.values.addLevelCount)
end) end)

pcall(function() RegisterKeyBind(config.keys.spawnItem, function()
    spawnItem(config.values.spawnItemId, config.values.spawnItemCount)
end) end)

log("PalCheatMenu MAX charge avec succes.")
