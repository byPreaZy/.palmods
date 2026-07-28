
-- Robustesse : interception des hooks pour eviter les crashs de UFunctions manquantes
local _RegisterHook = RegisterHook
if _RegisterHook then
    RegisterHook = function(path, callback)
        local ok, err = pcall(_RegisterHook, path, callback)
        if not ok then
            if log then
                log("[PalMiniMap] Hook ignore : " .. tostring(path))
            end
        end
    end
end

-- Robustesse : si ImGui n'est pas disponible, utiliser un proxy dynamique
if not ImGui then
    local noop = function() return nil end
    local proxy = setmetatable({}, {
        __index = function(t, k)
            local real = rawget(_G, "ImGui")
            if real and real ~= t then
                return real[k]
            end
            return noop
        end
    })
    ImGui = proxy
end

--[[
    PalMiniMap Prototype MAX
    Super minimap Lua via ImGui pour Palworld 1.0 (Steam) / UE4SS
    Auteur : genere par Cascade
--]]

local config = require("config")
local assets = require("assets_data")
local ipc_ok, PalTrainerIPC = pcall(require, "PalTrainerIPC")
local MOD_PREFIX = config.modPrefix or "[PalMiniMap]"
local DEBUG = config.debug

local state = {
    initialized = false,
    showMap = config.display.enabled,
    showFilterMenu = false,
    moveMode = false,
    superZoomActive = false,
    zoom = config.display.zoom,
    posX = config.display.posX,
    posY = config.display.posY,
    size = config.display.size,
    rotateWithCamera = config.display.rotateWithCamera,
    -- Cache terrain
    terrainCache = {},
    terrainLastUpdate = 0,
    -- Joueurs multijoueur (couleurs stables)
    playerColors = {},
    playerColorIndex = 1,
    -- Liste des Pals detectes pour le menu de selection
    palList = {},
    palListLastUpdate = 0,
    showPalSelector = false,
    -- Calibration in-game des POI
    calibrationMode = false,
    calibrationPOIIndex = 1,
    userPOIOverrides = {},
}

local function log(msg)
    if DEBUG then
        print(string.format("%s %s", MOD_PREFIX, tostring(msg)))
    end
end

local function err(msg)
    print(string.format("%s [ERREUR] %s", MOD_PREFIX, tostring(msg)))
end

local function isValid(obj)
    return obj ~= nil and type(obj) == "table" and obj.IsValid ~= nil and obj:IsValid()
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

local function getPlayerCharacter()
    local candidates = { "PalPlayerCharacter", "BP_PlayerCharacter_Female_C", "BP_PlayerCharacter_Male_C" }
    for _, className in ipairs(candidates) do
        local obj = FindFirstOf(className)
        if isValid(obj) then return obj end
    end
    return nil
end

local function getPlayerController()
    local pc = FindFirstOf("PlayerController")
    return isValid(pc) and pc or nil
end

local function getWorld()
    local player = getPlayerCharacter()
    if isValid(player) then
        return safeGet(player, "World")
    end
    return nil
end

-- Couleurs stables pour les joueurs multijoueur
local PLAYER_COLOR_PALETTE = {
    0xFFFF0000, 0xFF00FF00, 0xFF0000FF, 0xFFFFFF00,
    0xFFFF00FF, 0xFF00FFFF, 0xFFFFA500, 0xFF800080,
    0xFF008000, 0xFF800000, 0xFF008080, 0xFFFFC0CB,
}

local function getStablePlayerColor(player)
    if not isValid(player) then return 0xFFFFFFFF end
    local fullName = player.GetFullName and player:GetFullName() or tostring(player)
    if not state.playerColors[fullName] then
        state.playerColors[fullName] = PLAYER_COLOR_PALETTE[state.playerColorIndex]
        state.playerColorIndex = (state.playerColorIndex % #PLAYER_COLOR_PALETTE) + 1
    end
    return state.playerColors[fullName]
end

local function getGroundHeight(wx, wy, world)
    if not world then world = getWorld() end
    if not world then return nil end
    local start = { X = wx, Y = wy, Z = 100000.0 }
    local finish = { X = wx, Y = wy, Z = -100000.0 }
    local hitResult = {}
    local ok = pcall(function()
        -- Signature approximative : LineTraceSingle(Start, End, TraceChannel, QueryParams, OutHit)
        world:LineTraceSingle(start, finish, 0, nil, hitResult)
    end)
    if ok and hitResult and hitResult.Location then
        return hitResult.Location.Z
    end
    return nil
end

local function mixColors(c1, c2, ratio)
    if not c2 then return c1 end
    local r1 = (c1 >> 16) & 0xFF
    local g1 = (c1 >> 8) & 0xFF
    local b1 = c1 & 0xFF
    local r2 = (c2 >> 16) & 0xFF
    local g2 = (c2 >> 8) & 0xFF
    local b2 = c2 & 0xFF
    local r = math.floor(r1 * (1 - ratio) + r2 * ratio)
    local g = math.floor(g1 * (1 - ratio) + g2 * ratio)
    local b = math.floor(b1 * (1 - ratio) + b2 * ratio)
    return 0xFF000000 + (r << 16) + (g << 8) + b
end

local function getTerrainColor(z, slope, biomeColor)
    local mode = config.display.terrainMode
    local base
    if mode == "satellite" then
        if z <= 50 then base = config.colors.water
        elseif z <= 80 then base = config.colors.beach
        elseif z <= 400 then base = config.colors.grass
        elseif z <= 800 then base = config.colors.forest
        elseif z <= 1400 then base = config.colors.rock
        else base = config.colors.snow end
    elseif mode == "topographic" then
        if slope and slope > 45 then base = config.colors.slope
        else
            local band = math.floor(z / 100) % 2
            base = band == 0 and config.colors.grass or config.colors.forest
        end
    else -- heightmap
        if z <= 50 then base = config.colors.water
        elseif z <= 80 then base = config.colors.beach
        elseif z <= 400 then base = config.colors.grass
        elseif z <= 800 then base = config.colors.forest
        elseif z <= 1400 then base = config.colors.rock
        else base = config.colors.snow end
    end
    if biomeColor then
        return mixColors(base, biomeColor, 0.3)
    end
    return base
end

local function getEntityName(obj)
    if not isValid(obj) then return nil end
    local name = safeGet(obj, "CharacterID") or safeGet(obj, "PalName") or safeGet(obj, "Name")
    if name then return tostring(name) end
    local full = obj.GetFullName and obj:GetFullName() or ""
    local short = string.match(full, "/([^/]+)$") or full
    short = string.gsub(short, "_C$", "")
    short = string.gsub(short, "BP_", "")
    short = string.gsub(short, "PalCharacter", "")
    return short ~= "" and short or nil
end

local function getPalName(obj)
    return getEntityName(obj)
end

local function isSelectedPal(obj)
    if not config.filters.selectedPalsOnly then return true end
    local name = getPalName(obj)
    if not name then return false end
    for _, selected in ipairs(config.filters.selectedPals) do
        if string.find(string.lower(name), string.lower(selected)) then
            return true
        end
    end
    return false
end

-- ============================================================================
-- UTILITAIRES ASSETS (Pals, types, biomes, POI)
-- ============================================================================

local function findPalByName(name)
    if not name or not assets or not assets.PALS then return nil end
    local lowerName = string.lower(name)
    for _, pal in ipairs(assets.PALS) do
        if string.lower(pal.name) == lowerName then
            return pal
        end
    end
    -- Recherche partielle
    for _, pal in ipairs(assets.PALS) do
        if string.find(string.lower(pal.name), lowerName) or string.find(lowerName, string.lower(pal.name)) then
            return pal
        end
    end
    return nil
end

local function getPalColor(name)
    local pal = findPalByName(name)
    if pal and pal.type1 and assets.TYPE_COLORS[pal.type1] then
        return assets.TYPE_COLORS[pal.type1]
    end
    return config.colors.pal
end

local function getPalInfo(name)
    local pal = findPalByName(name)
    if pal then
        local t2 = (pal.type2 and pal.type2 ~= "None" and pal.type2 ~= "") and ("/" .. pal.type2) or ""
        return pal.type1 .. t2
    end
    return nil
end

local function getRegionAt(px, py)
    if not assets or not assets.REGIONS then return nil end
    for _, region in ipairs(assets.REGIONS) do
        local dx = px - region.x
        local dy = py - region.y
        if math.sqrt(dx * dx + dy * dy) <= region.radius then
            return region
        end
    end
    return nil
end

local function getBiomeColor(px, py, defaultColor)
    local region = getRegionAt(px, py)
    if region then
        return region.color, region.biome
    end
    return defaultColor, nil
end

local function getEffectivePOIList()
    if not assets or not assets.POI then return {} end
    local list = {}
    for i, poi in ipairs(assets.POI) do
        list[i] = poi
    end
    -- Appliquer les overrides utilisateur
    for i, override in pairs(state.userPOIOverrides) do
        if list[i] then
            list[i] = { type = list[i].type, name = list[i].name, region = list[i].region, x = override.x, y = override.y }
        end
    end
    return list
end

local function loadUserPOIOverrides()
    local ok, data = pcall(function()
        local file = io.open("user_poi_overrides.lua", "r")
        if not file then return nil end
        local content = file:read("*a")
        file:close()
        return load(content) and load(content)() or nil
    end)
    if ok and type(data) == "table" then
        state.userPOIOverrides = data
        log("Overrides POI utilisateur charges.")
    else
        state.userPOIOverrides = {}
    end
end

local function saveUserPOIOverrides()
    local ok, err = pcall(function()
        local file = io.open("user_poi_overrides.lua", "w")
        if not file then return false end
        file:write("return {\n")
        for i, override in pairs(state.userPOIOverrides) do
            file:write(string.format("  [%d] = { x = %.2f, y = %.2f },\n", i, override.x, override.y))
        end
        file:write("}\n")
        file:close()
        return true
    end)
    if not ok then
        err("Impossible de sauvegarder les overrides POI : " .. tostring(err))
    end
end

local function calibrateCurrentPOI()
    local poiList = getEffectivePOIList()
    local idx = state.calibrationPOIIndex
    if idx < 1 or idx > #poiList then
        err("Index POI invalide pour la calibration.")
        return
    end
    local px, py, pz = getPlayerLocation()
    if not px then
        err("Position joueur introuvable pour la calibration.")
        return
    end
    state.userPOIOverrides[idx] = { x = px, y = py }
    saveUserPOIOverrides()
    log(string.format("POI #%d calibre : %s -> (%.2f, %.2f)", idx, poiList[idx].name, px, py))
end

local function drawCalibrationWindow()
    if not state.calibrationMode then return end
    local ok = pcall(function()
        ImGui.Begin("PalMiniMap - Calibration POI", true)
    end)
    if not ok then return end

    local poiList = getEffectivePOIList()
    ImGui.Text(string.format("POI disponibles : %d", #poiList))
    ImGui.Separator()

    local names = {}
    for _, poi in ipairs(poiList) do
        table.insert(names, poi.name)
    end
    local comboString = table.concat(names, "\0") .. "\0"

    local changed, value = ImGui.Combo("Selection POI", state.calibrationPOIIndex - 1, comboString)
    if changed then
        state.calibrationPOIIndex = value + 1
    end

    local px, py, pz = getPlayerLocation()
    if px then
        ImGui.Text(string.format("Position actuelle : X:%.0f Y:%.0f Z:%.0f", px, py, pz))
    end

    if ImGui.Button("Enregistrer position comme POI") then
        calibrateCurrentPOI()
    end

    ImGui.SameLine()

    if ImGui.Button("Fermer calibration") then
        state.calibrationMode = false
    end

    ImGui.End()
end

local function isPOITypeVisible(poiType)
    if not config.display.showPOI then return false end
    if poiType == "tower"      then return config.display.showPOITowers end
    if poiType == "fastTravel" then return config.display.showPOIFastTravel end
    if poiType == "alpha"      then return config.display.showPOIAlpha end
    if poiType == "dungeon"    then return config.display.showPOIDungeons end
    if poiType == "sanctuary"  then return config.display.showPOISanctuaries end
    if poiType == "sealed"     then return config.display.showPOISealed end
    if poiType == "bounty"     then return config.display.showPOIBounty end
    if poiType == "skillFruit" then return config.display.showPOISkillFruit end
    if poiType == "merchant"   then return config.display.showPOIMerchants end
    if poiType == "effigy"     then return config.display.showPOIEffigies end
    if poiType == "resource"   then return config.display.showPOIResources end
    if poiType == "relic"        then return config.display.showPOIRelics end
    return true
end

local function drawPOIMarkers(drawList, px, py, effectiveZoom, size, rotRad, cx, cy)
    local poiList = getEffectivePOIList()
    if #poiList == 0 then return end
    for _, poi in ipairs(poiList) do
        if not isPOITypeVisible(poi.type) then goto continue end

        local mx, my = worldToMiniMap(poi.x, poi.y, px, py, effectiveZoom, size)
        local dx = mx - size / 2
        local dy = my - size / 2
        local rx, ry
        if state.rotateWithCamera then
            rx = dx * math.cos(rotRad) - dy * math.sin(rotRad)
            ry = dx * math.sin(rotRad) + dy * math.cos(rotRad)
        else
            rx = dx
            ry = dy
        end

        local color = config.colors.neutral
        local radius = 4
        if poi.type == "tower" then
            color = config.colors.tower
            radius = 6
        elseif poi.type == "sanctuary" then
            color = config.colors.sanctuary
            radius = 5
        elseif poi.type == "fastTravel" then
            color = config.colors.fastTravel
            radius = 4
        elseif poi.type == "alpha" then
            color = config.colors.alpha
            radius = 5
        elseif poi.type == "dungeon" then
            color = config.colors.dungeon
            radius = 4
        elseif poi.type == "sealed" then
            color = config.colors.sealed
            radius = 5
        elseif poi.type == "bounty" then
            color = config.colors.bounty
            radius = 4
        elseif poi.type == "skillFruit" then
            color = config.colors.skillFruit
            radius = 4
        elseif poi.type == "merchant" then
            color = config.colors.merchant
            radius = 5
        elseif poi.type == "effigy" then
            color = config.colors.effigy
            radius = 3
        elseif poi.type == "resource" then
            color = config.colors.resource
            radius = 3
        elseif poi.type == "relic" then
            color = config.colors.relic
            radius = 4
        end

        -- LOD / distance culling : cache les petits marqueurs eloignes
        local dist = math.sqrt((poi.x - px)^2 + (poi.y - py)^2)
        local maxRange = config.ranges[poi.type] or config.ranges.poi or 2000
        if dist > maxRange then goto continue end

        -- Simplify dense markers (effigy/resource) when zoomed out
        if (poi.type == "effigy" or poi.type == "resource" or poi.type == "relic") and effectiveZoom < 1.0 and dist > 300 then
            radius = 2
        end

        local sx, sy = cx + rx, cy + ry
        drawList:AddCircleFilled(sx, sy, radius, color, 6)
        if config.display.showPOILabels then
            drawList:AddText(sx + 8, sy - 6, 0xFFFFFFFF, poi.name)
        end

        ::continue::
    end
end

local function getPlayerLocation()
    local player = getPlayerCharacter()
    if not player then return nil, nil, nil end
    local loc = player:GetActorLocation()
    if loc then return loc.X, loc.Y, loc.Z end
    return nil, nil, nil
end

local function getPlayerRotation()
    local player = getPlayerCharacter()
    if not player then return 0 end
    local controller = getPlayerController()
    if isValid(controller) then
        local rot = controller:GetControlRotation()
        if rot then return rot.Yaw end
    end
    local rot = player:GetActorRotation()
    if rot then return rot.Yaw end
    return 0
end

local function worldToMiniMap(wx, wy, px, py, zoom, size)
    local scale = zoom * (size / 1500)
    local mx = (wx - px) * scale
    local my = (wy - py) * scale
    return size / 2 + mx, size / 2 - my
end

local function distance2D(x1, y1, x2, y2)
    local dx = x2 - x1
    local dy = y2 - y1
    return math.sqrt(dx * dx + dy * dy)
end

-- ============================================================================
-- DESSIN DE LA CARTE
-- ============================================================================

local function drawGrid(drawList, wx, wy, size, step, color)
    if not config.display.showGrid then return end
    local half = size / 2
    for i = 0, size, step do
        drawList:AddLine(wx + i, wy, wx + i, wy + size, color, 1)
        drawList:AddLine(wx, wy + i, wx + size, wy + i, color, 1)
    end
end

local function drawCompass(drawList, cx, cy, radius, yawRad)
    if not config.display.showCompass then return end
    local r = radius - 12
    local nx = cx + math.sin(yawRad) * r
    local ny = cy - math.cos(yawRad) * r
    drawList:AddLine(cx, cy, nx, ny, config.colors.compass, 3)
    drawList:AddCircleFilled(cx, cy, 4, config.colors.compass, 6)
end

local function drawEntityMarker(drawList, x, y, color, radius, label)
    drawList:AddCircleFilled(x, y, radius, color, 6)
    if label then
        drawList:AddText(x + 8, y - 6, 0xFFFFFFFF, label)
    end
end

local function scanEntities(className, maxRange, px, py, list)
    local found = FindAllOf(className)
    if found then
        for _, obj in ipairs(found) do
            if isValid(obj) then
                local loc = obj:GetActorLocation()
                if loc then
                    local dist = distance2D(px, py, loc.X, loc.Y)
                    if dist <= maxRange then
                        table.insert(list, { obj = obj, x = loc.X, y = loc.Y, z = loc.Z, dist = dist })
                    end
                end
            end
        end
    end
end

local function getAllPlayers()
    local players = {}
    local classes = { "BP_PlayerCharacter_Female_C", "BP_PlayerCharacter_Male_C" }
    for _, className in ipairs(classes) do
        local found = FindAllOf(className)
        if found then
            for _, obj in ipairs(found) do
                if isValid(obj) then
                    table.insert(players, obj)
                end
            end
        end
    end
    return players
end

local function getPlayerRotationFromObj(obj)
    if not isValid(obj) then return 0 end
    local controller = safeGet(obj, "Controller")
    if isValid(controller) then
        local rot = controller:GetControlRotation()
        if rot then return rot.Yaw end
    end
    local rot = obj:GetActorRotation()
    if rot then return rot.Yaw end
    return 0
end

local function updatePalList(px, py)
    local now = os.time and os.time() or 0
    if now - state.palListLastUpdate < 2 then return end
    state.palListLastUpdate = now

    local found = FindAllOf("PalCharacter") or {}
    local seen = {}
    for _, obj in ipairs(found or {}) do
        if isValid(obj) and obj ~= getPlayerCharacter() then
            local loc = obj:GetActorLocation()
            if loc and distance2D(px, py, loc.X, loc.Y) <= config.ranges.pals then
                local name = getPalName(obj) or "Pal inconnu"
                if not seen[name] then
                    seen[name] = true
                end
            end
        end
    end

    -- Fusionner avec la liste existante sans supprimer les anciennes selections
    local newList = {}
    for name, _ in pairs(seen) do
        table.insert(newList, name)
    end
    table.sort(newList)

    -- Conserver l'ordre et les selections
    state.palList = newList
end

local function sampleEntityHeights(px, py, range)
    -- Utiliser les entites proches comme echantillons de hauteur supplementaires
    local samples = {}
    local classes = { "PalCharacter", "PalMapObject_WorldContainer", "BP_PalMapObject_Pickup_C" }
    for _, className in ipairs(classes) do
        local found = FindAllOf(className)
        if found then
            for _, obj in ipairs(found) do
                if isValid(obj) then
                    local loc = obj:GetActorLocation()
                    if loc and distance2D(px, py, loc.X, loc.Y) <= range then
                        table.insert(samples, { x = loc.X, y = loc.Y, z = loc.Z })
                    end
                end
            end
        end
    end
    return samples
end

local function interpolatedHeight(wx, wy, world, samples, pz)
    -- Hauteur directe par raycast
    local z = getGroundHeight(wx, wy, world)
    if z ~= nil then return z end

    -- Interpolation a partir des echantillons d'entites proches
    if #samples > 0 then
        local totalWeight = 0
        local weightedZ = 0
        for _, s in ipairs(samples) do
            local dx = wx - s.x
            local dy = wy - s.y
            local dist = math.sqrt(dx * dx + dy * dy)
            if dist < 1 then return s.z end
            local weight = 1.0 / (dist * dist)
            weightedZ = weightedZ + s.z * weight
            totalWeight = totalWeight + weight
        end
        if totalWeight > 0 then
            return weightedZ / totalWeight
        end
    end

    -- Fallback : hauteur du joueur
    return pz
end

local function updateTerrainCache(px, py, pz)
    if not config.display.showTerrain then return end
    local now = os.time and os.time() or 0
    if now - state.terrainLastUpdate < config.display.terrainUpdateInterval then return end
    state.terrainLastUpdate = now

    local res = config.display.terrainResolution
    local range = config.display.terrainRange
    local world = getWorld()
    if not world then return end

    local cache = {}
    local step = range / res
    local samples = {}
    if config.display.useEntitySamples then
        samples = sampleEntityHeights(px, py, range)
    end

    for i = -res / 2, res / 2 do
        for j = -res / 2, res / 2 do
            local wx = px + i * step
            local wy = py + j * step
            local z = interpolatedHeight(wx, wy, world, samples, pz)
            local ni = math.floor(i + res / 2)
            local nj = math.floor(j + res / 2)
            if not cache[ni] then cache[ni] = {} end
            cache[ni][nj] = z
        end
    end
    state.terrainCache = cache
end

local function drawTerrain(drawList, wx, wy, px, py, effectiveZoom, rotRad, size)
    if not config.display.showTerrain then return end
    local res = config.display.terrainResolution
    if res < 4 then return end
    local range = config.display.terrainRange
    local step = range / res
    local cellScreen = (effectiveZoom * (size / 1500)) * step

    for i = -res / 2, res / 2 - 1 do
        for j = -res / 2, res / 2 - 1 do
            local wx1 = px + i * step
            local wy1 = py + j * step
            local wx2 = px + (i + 1) * step
            local wy2 = py + (j + 1) * step

            -- Centre de la cellule pour rotation
            local cxm = (wx1 + wx2) / 2
            local cym = (wy1 + wy2) / 2

            local mx, my = worldToMiniMap(cxm, cym, px, py, effectiveZoom, size)
            local dx = mx - size / 2
            local dy = my - size / 2
            local rx, ry
            if state.rotateWithCamera then
                rx = dx * math.cos(rotRad) - dy * math.sin(rotRad)
                ry = dx * math.sin(rotRad) + dy * math.cos(rotRad)
            else
                rx = dx
                ry = dy
            end

            local ni = math.floor(i + res / 2)
            local nj = math.floor(j + res / 2)
            local col1 = state.terrainCache[ni] or {}
            local col2 = state.terrainCache[ni + 1] or {}
            local z1 = col1[nj] or pz
            local z2 = col2[nj] or z1
            local z3 = col1[nj + 1] or z1
            local z4 = col2[nj + 1] or z1
            local z = (z1 + z2 + z3 + z4) / 4
            local slope = math.max(math.abs(z2 - z1), math.abs(z4 - z3))

            local biomeColor, _ = getBiomeColor(cxm, cym)
            local color = getTerrainColor(z, slope, biomeColor)
            local x1 = wx + size / 2 + rx - cellScreen / 2
            local y1 = wy + size / 2 + ry - cellScreen / 2
            local x2 = x1 + cellScreen
            local y2 = y1 + cellScreen
            drawList:AddRectFilled(x1, y1, x2, y2, color, 0, 0)
        end
    end
end

local function drawPlayerArrow(drawList, cx, cy, x, y, yaw, color, label)
    -- Fleche directionnelle pointant vers l'orientation du joueur
    local len = 12
    local head = 5
    local rad = math.rad(-yaw)
    local ex = x + math.sin(rad) * len
    local ey = y - math.cos(rad) * len

    drawList:AddLine(x, y, ex, ey, color, 3)
    drawList:AddCircleFilled(x, y, 4, color, 6)

    -- Petites barbes
    local radLeft = rad - math.rad(150)
    local radRight = rad + math.rad(150)
    local lx = ex + math.sin(radLeft) * head
    local ly = ey - math.cos(radLeft) * head
    local rx = ex + math.sin(radRight) * head
    local ry = ey - math.cos(radRight) * head
    drawList:AddLine(ex, ey, lx, ly, color, 2)
    drawList:AddLine(ex, ey, rx, ry, color, 2)

    if label then
        drawList:AddText(x + 8, y - 6, 0xFFFFFFFF, label)
    end
end

-- ============================================================================
-- MINIMAP RENDU
-- ============================================================================

local function drawMiniMap()
    if not state.showMap then return end
    if not ImGui or not ImGui.Begin then return end

    local ImGuiCond_FirstUseEver = 4
    local ImGuiWindowFlags_NoScrollbar = 8
    local ImGuiWindowFlags_NoScrollWithMouse = 16
    local ImGuiWindowFlags_NoMove = 4

    local ok = pcall(function()
        ImGui.SetNextWindowPos(state.posX, state.posY, ImGuiCond_FirstUseEver)
        ImGui.SetNextWindowSize(state.size, state.size + 60, ImGuiCond_FirstUseEver)
        ImGui.Begin("PalMiniMap MAX", true,
            ImGuiWindowFlags_NoScrollbar +
            ImGuiWindowFlags_NoScrollWithMouse +
            (state.moveMode and 0 or ImGuiWindowFlags_NoMove))
    end)
    if not ok then
        state.showMap = false
        return
    end

    local px, py, pz = getPlayerLocation()
    if not px then
        ImGui.Text("Joueur non detecte...")
        ImGui.End()
        return
    end

    local wx, wy = ImGui.GetWindowPos()
    local cx = wx + state.size / 2
    local cy = wy + state.size / 2
    local drawList = ImGui.GetWindowDrawList()
    local yaw = getPlayerRotation()
    local rotRad = math.rad(-yaw)
    local effectiveZoom = state.superZoomActive and config.display.superZoom or state.zoom

    -- Mise a jour des donnees
    updateTerrainCache(px, py, pz)
    updatePalList(px, py)

    -- Fond + terrain
    drawList:AddRectFilled(wx, wy, wx + state.size, wy + state.size, 0xDD000000, 0, 0)
    drawTerrain(drawList, wx, wy, px, py, effectiveZoom, rotRad, state.size)
    drawGrid(drawList, wx, wy, state.size, 40, config.colors.grid)

    -- Entites (non joueurs)
    local entities = {}

    if config.filters.pals or config.filters.enemyPals then
        scanEntities("PalCharacter", config.ranges.pals, px, py, entities)
    end
    if config.filters.chests then
        scanEntities("PalMapObject_WorldContainer", config.ranges.chests, px, py, entities)
        scanEntities("BP_TreasureBox_C", config.ranges.chests, px, py, entities)
    end
    if config.filters.eggs then
        scanEntities("PalItem_Egg", config.ranges.eggs, px, py, entities)
        scanEntities("BP_PalEgg_C", config.ranges.eggs, px, py, entities)
    end
    if config.filters.dungeons then
        scanEntities("BP_DungeonEntrance_C", config.ranges.dungeons, px, py, entities)
    end
    if config.filters.fastTravel then
        scanEntities("BP_FastTravelPoint_C", config.ranges.fastTravel, px, py, entities)
    end
    if config.filters.resources then
        scanEntities("BP_PalMapObject_Pickup_C", config.ranges.resources, px, py, entities)
    end

    -- Marqueurs
    for _, e in ipairs(entities) do
        local mx, my = worldToMiniMap(e.x, e.y, px, py, effectiveZoom, state.size)
        local dx = mx - state.size / 2
        local dy = my - state.size / 2
        local rx, ry

        if state.rotateWithCamera then
            rx = dx * math.cos(rotRad) - dy * math.sin(rotRad)
            ry = dx * math.sin(rotRad) + dy * math.cos(rotRad)
        else
            rx = dx
            ry = dy
        end

        local color = config.colors.neutral or 0xFFFFFFFF
        local radius = 3
        local label = nil

        local className = e.obj and e.obj.GetFullName and e.obj:GetFullName() or ""

        if string.find(className, "PlayerCharacter") then
            color = config.colors.playerOther
            radius = 4
        elseif string.find(className, "PalCharacter") then
            if e.obj == getPlayerCharacter() then
                -- ignore le joueur ici ; gere plus bas
                goto continue
            else
                if not isSelectedPal(e.obj) then
                    goto continue
                end
                local palName = getPalName(e.obj)
                local palInfo = getPalInfo(palName)
                if config.display.colorByPalType then
                    color = getPalColor(palName)
                else
                    color = config.filters.pals and config.colors.pal or config.colors.enemy
                end
                if config.filters.enemyPals and not config.filters.pals then
                    color = config.colors.enemy
                end
                radius = 4
                if palName then
                    if config.display.showPalType and palInfo then
                        label = string.format("%s (%s)", palName, palInfo)
                    else
                        label = palName
                    end
                end
            end
        elseif string.find(className, "Chest") or string.find(className, "Container") then
            color = config.colors.chest
            radius = 3
        elseif string.find(className, "Egg") then
            color = config.colors.egg
            radius = 3
        elseif string.find(className, "Dungeon") then
            color = config.colors.dungeon
            radius = 6
        elseif string.find(className, "FastTravel") then
            color = config.colors.fastTravel
            radius = 5
        elseif string.find(className, "Pickup") then
            color = config.colors.resource
            radius = 3
        end

        if color and color ~= 0 then
            drawEntityMarker(drawList, cx + rx, cy + ry, color, radius, label)
        end

        ::continue::
    end

    -- Joueurs multijoueur avec fleches de couleurs
    if config.filters.players then
        local playerIndex = 1
        for _, player in ipairs(getAllPlayers()) do
            if player == getPlayerCharacter() then goto nextPlayer end
            local loc = player:GetActorLocation()
            if not loc then goto nextPlayer end
            local dist = distance2D(px, py, loc.X, loc.Y)
            if dist > config.ranges.players then goto nextPlayer end

            local mx, my = worldToMiniMap(loc.X, loc.Y, px, py, effectiveZoom, state.size)
            local dx = mx - state.size / 2
            local dy = my - state.size / 2
            local rx, ry
            if state.rotateWithCamera then
                rx = dx * math.cos(rotRad) - dy * math.sin(rotRad)
                ry = dx * math.sin(rotRad) + dy * math.cos(rotRad)
            else
                rx = dx
                ry = dy
            end

            local pcolor = getStablePlayerColor(player)
            local pyaw = getPlayerRotationFromObj(player)
            local label = string.format("P%d (%.0fm)", playerIndex, dist)
            drawPlayerArrow(drawList, cx, cy, cx + rx, cy + ry, pyaw, pcolor, label)
            playerIndex = playerIndex + 1

            ::nextPlayer::
        end
    end

    -- Points d'interet (POI) publics
    drawPOIMarkers(drawList, px, py, effectiveZoom, state.size, rotRad, cx, cy)

    -- Joueur centre
    drawList:AddCircleFilled(cx, cy, 7, config.colors.player, 8)
    drawCompass(drawList, cx, cy, 7, math.rad(yaw))

    -- Coordonnees et infos
    if config.display.showCoordinates then
        local region = getRegionAt(px, py)
        local biomeName = region and region.biome or "?"
        ImGui.Text(string.format("X:%.0f Y:%.0f Z:%.0f | %s | Zoom:%.2f%s",
            px, py, pz, biomeName, effectiveZoom, state.superZoomActive and " [SUPER]" or ""))
    end

    ImGui.End()
end

-- ============================================================================
-- MENU DE FILTRES
-- ============================================================================

local function toggleFilter(key, label)
    local changed, value = ImGui.Checkbox(label, config.filters[key])
    if changed then config.filters[key] = value end
end

local function toggleDisplay(key, label)
    local changed, value = ImGui.Checkbox(label, config.display[key])
    if changed then config.display[key] = value end
end

local function drawFilterMenu()
    if not state.showFilterMenu then return end

    local ok = pcall(function()
        ImGui.Begin("PalMiniMap - Filtres", true)
    end)
    if not ok then return end

    ImGui.Text("Filtres d'entites")
    ImGui.Separator()

    toggleFilter("pals", "Pals amicaux")
    toggleFilter("enemyPals", "Pals hostiles")
    toggleFilter("chests", "Coffres")
    toggleFilter("eggs", "Oeufs")
    toggleFilter("dungeons", "Donjons")
    toggleFilter("fastTravel", "Voyage rapide")
    toggleFilter("players", "Autres joueurs")
    toggleFilter("playerArrows", "Fleches joueurs (multi)")
    toggleFilter("resources", "Ressources")

    ImGui.Separator()
    ImGui.Text("Selection des Pals")
    toggleFilter("selectedPalsOnly", "Afficher seulement la selection")

    if ImGui.Button("Ouvrir selecteur de Pals") then
        state.showPalSelector = not state.showPalSelector
    end

    ImGui.Separator()
    ImGui.Text("Affichage")
    toggleDisplay("showCoordinates", "Coordonnees")
    toggleDisplay("showCompass", "Boussole")
    toggleDisplay("showGrid", "Quadrillage")
    toggleDisplay("showTerrain", "Rendu terrain")
    toggleDisplay("useEntitySamples", "Echantillons d'entites (heightmap)")

    ImGui.Separator()
    ImGui.Text("Points d'interet")
    toggleDisplay("showPOI", "Activer POI")
    toggleDisplay("showPOITowers", "Tours")
    toggleDisplay("showPOIFastTravel", "Voyage rapide")
    toggleDisplay("showPOIAlpha", "Alpha Boss")
    toggleDisplay("showPOIDungeons", "Donjons")
    toggleDisplay("showPOISanctuaries", "Sanctuaires")
    toggleDisplay("showPOISealed", "Sealed Realms")
    toggleDisplay("showPOIBounty", "Bounty Targets")
    toggleDisplay("showPOISkillFruit", "Skill Fruit Trees")
    toggleDisplay("showPOIMerchants", "Marchands")
    toggleDisplay("showPOIEffigies", "Lifmunk Effigies")
    toggleDisplay("showPOIResources", "Clusters ressources")
    toggleDisplay("showPOIRelics", "Relics/Statues")
    toggleDisplay("showPOILabels", "Labels des POI")

    changed, value = ImGui.Checkbox("Rotation camera", state.rotateWithCamera)
    if changed then state.rotateWithCamera = value end

    changed, value = ImGui.SliderFloat("Zoom", state.zoom, 0.1, 10.0)
    if changed then state.zoom = value end

    changed, value = ImGui.SliderInt("Taille", state.size, 150, 600)
    if changed then state.size = value end

    changed, value = ImGui.SliderInt("Resolution terrain", config.display.terrainResolution, 8, 128)
    if changed then config.display.terrainResolution = value end

    changed, value = ImGui.SliderFloat("Portee terrain", config.display.terrainRange, 100, 3000)
    if changed then config.display.terrainRange = value end

    changed, value = ImGui.Combo("Mode terrain", config.display.terrainMode, "heightmap\0satellite\0topographic\0")
    if changed then config.display.terrainMode = value end

    ImGui.Separator()
    ImGui.Text("Calibration POI")
    if ImGui.Button("Ouvrir calibration POI") then
        state.calibrationMode = true
    end

    ImGui.End()
end

-- ============================================================================
-- SELECTEUR DE PALS
-- ============================================================================

local function drawPalSelector(px, py)
    if not state.showPalSelector then return end

    local ok = pcall(function()
        ImGui.Begin("PalMiniMap - Selection des Pals", true)
    end)
    if not ok then return end

    ImGui.Text("Cochez les Pals a afficher sur la minimap")
    ImGui.Separator()

    if #state.palList == 0 then
        ImGui.Text("Aucun Pal detecte a proximite.")
    else
        for _, name in ipairs(state.palList) do
            local selected = false
            for _, selectedName in ipairs(config.filters.selectedPals) do
                if selectedName == name then
                    selected = true
                    break
                end
            end
            local changed, value = ImGui.Checkbox(name, selected)
            if changed then
                if value then
                    table.insert(config.filters.selectedPals, name)
                else
                    for i, selectedName in ipairs(config.filters.selectedPals) do
                        if selectedName == name then
                            table.remove(config.filters.selectedPals, i)
                            break
                        end
                    end
                end
            end
        end
    end

    if ImGui.Button("Tout selectionner") then
        config.filters.selectedPals = {}
        for _, name in ipairs(state.palList) do
            table.insert(config.filters.selectedPals, name)
        end
    end
    ImGui.SameLine()
    if ImGui.Button("Tout deselectionner") then
        config.filters.selectedPals = {}
    end
    ImGui.SameLine()
    if ImGui.Button("Fermer") then
        state.showPalSelector = false
    end

    ImGui.End()
end


-- ============================================================================
-- EXPORT CARTE EXTERNE (PalTrainerApp)
-- ============================================================================

local MAP_ENTITY_CLASSES = {
    { class = "PalCharacter",              type = "pal",        filter = "pals" },
    { class = "PalMapObject_WorldContainer", type = "chest",    filter = "chests" },
    { class = "BP_TreasureBox_C",          type = "chest",      filter = "chests" },
    { class = "PalItem_Egg",               type = "egg",        filter = "eggs" },
    { class = "BP_PalEgg_C",               type = "egg",        filter = "eggs" },
    { class = "BP_DungeonEntrance_C",      type = "dungeon",    filter = "dungeons" },
    { class = "BP_FastTravelPoint_C",      type = "fastTravel", filter = "fastTravel" },
    { class = "BP_PalMapObject_Pickup_C",  type = "resource",   filter = "resources" },
}

local function collectMapEntities(px, py)
    local all = {}
    for _, entry in ipairs(MAP_ENTITY_CLASSES) do
        local filter = entry.filter
        local active = true
        if filter == "pals" then
            active = config.filters.pals or config.filters.enemyPals
        else
            active = config.filters[filter]
        end

        if active then
            local before = #all
            scanEntities(entry.class, config.ranges[filter] or 3000, px, py, all)
            for i = before + 1, #all do
                all[i].type = entry.type
                all[i].name = getEntityName(all[i].obj) or entry.type
            end
        end
    end

    -- Other players
    if config.filters.players then
        for _, p in ipairs(getAllPlayers()) do
            if p ~= getPlayerCharacter() and isValid(p) then
                local loc = p:GetActorLocation()
                if loc and distance2D(px, py, loc.X, loc.Y) <= config.ranges.players then
                    table.insert(all, {
                        obj = p,
                        x = loc.X, y = loc.Y, z = loc.Z,
                        type = "player",
                        name = "Joueur",
                        dist = distance2D(px, py, loc.X, loc.Y)
                    })
                end
            end
        end
    end

    -- Serializable list
    local result = {}
    for _, e in ipairs(all) do
        if e.type then
            table.insert(result, {
                type = e.type,
                x = math.floor(e.x + 0.5),
                y = math.floor(e.y + 0.5),
                z = math.floor(e.z + 0.5),
                name = e.name or e.type
            })
        end
    end
    return result
end

local function exportMapData()
    if not PalTrainerIPC then return end
    if not state.showMap then return end
    local px, py, pz = getPlayerLocation()
    if not px then return end
    local yaw = getPlayerRotation()
    local entities = collectMapEntities(px, py)
    PalTrainerIPC.writeMap({
        player = { x = math.floor(px + 0.5), y = math.floor(py + 0.5), z = math.floor(pz + 0.5), yaw = yaw },
        entities = entities
    })
end

local function pollTrainer()
    if not PalTrainerIPC then return end
    PalTrainerIPC.ensureDir()
    local cmds = PalTrainerIPC.pollCommands()

    local val = PalTrainerIPC.getCommand(cmds, "mod.PalMiniMap")
    if val ~= nil then
        if val == "toggle" then
            config.display.enabled = not config.display.enabled
            state.showMap = config.display.enabled
        else
            config.display.enabled = val
            state.showMap = val
        end
    end

    -- Filtres de carte depuis l'app externe
    local filterKeys = { "pals", "enemyPals", "chests", "eggs", "dungeons", "fastTravel", "players", "resources" }
    for _, key in ipairs(filterKeys) do
        local fv = PalTrainerIPC.getCommand(cmds, "map.filter." .. key)
        if fv ~= nil then
            config.filters[key] = fv
        end
    end

    PalTrainerIPC.writeStatus({ ["mod.PalMiniMap"] = state.showMap })
end

-- ============================================================================
-- INITIALISATION
-- ============================================================================

local function init()
    if state.initialized then return end
    state.initialized = true

    -- Chargement de la police open-source Noto Sans (si presente)
    local fontPath = "Mods/PalMiniMapPrototype/Fonts/NotoSans-Regular.ttf"
    local ok, result = pcall(function()
        local io = ImGui.GetIO()
        if io and io.Fonts then
            io.Fonts:AddFontFromFileTTF(fontPath, 14.0)
        end
    end)
    if ok then
        log("Police Noto Sans chargee : " .. fontPath)
    else
        log("Police Noto Sans non chargee (optionnelle) : " .. tostring(result))
    end

    -- Charger les calibrations POI utilisateur
    loadUserPOIOverrides()

    log("PalMiniMap Prototype MAX initialise.")
end

-- ============================================================================
-- HOOKS
-- ============================================================================

RegisterHook("/Script/Engine.PlayerController:ClientRestart", function()
    ExecuteInGameThread(function() init() end)
end)

RegisterHook("/Script/Engine.PlayerController:ServerAcknowledgePossession", function()
    if not state.initialized then
        ExecuteInGameThread(function() init() end)
    end
end)

local function tick()
    pcall(function()
        pollTrainer()
        exportMapData()
    end)
    pcall(drawMiniMap)
    pcall(drawFilterMenu)
    pcall(drawCalibrationWindow)
    local px, py, pz = getPlayerLocation()
    if px then pcall(function() drawPalSelector(px, py) end) end
end

-- PlayerTick remplace par LoopAsync (hook non disponible sur cette version)
LoopAsync(100, function()
    pcall(function()
        ExecuteInGameThread(tick)
    end)
    return true
end)

-- ============================================================================
-- RACCOURCIS
-- ============================================================================

pcall(function() RegisterKeyBind(config.keys.toggleMap, function()
    state.showMap = not state.showMap
    log("Minimap " .. (state.showMap and "ON" or "OFF"))
end) end)

pcall(function() RegisterKeyBind(config.keys.toggleMove, function()
    state.moveMode = not state.moveMode
    log("Mode deplacement " .. (state.moveMode and "ON" or "OFF"))
end) end)

pcall(function() RegisterKeyBind(config.keys.zoomIn, function()
    state.zoom = state.zoom * 1.2
    log("Zoom : " .. state.zoom)
end) end)

pcall(function() RegisterKeyBind(config.keys.zoomOut, function()
    state.zoom = state.zoom / 1.2
    log("Zoom : " .. state.zoom)
end) end)

pcall(function() RegisterKeyBind(config.keys.superZoom, function()
    state.superZoomActive = not state.superZoomActive
    log("Super zoom " .. (state.superZoomActive and "ON" or "OFF"))
end) end)

pcall(function() RegisterKeyBind(config.keys.toggleRotation, function()
    state.rotateWithCamera = not state.rotateWithCamera
    log("Rotation camera " .. (state.rotateWithCamera and "ON" or "OFF"))
end) end)

pcall(function() RegisterKeyBind(config.keys.toggleFilterMenu, function()
    state.showFilterMenu = not state.showFilterMenu
    log("Menu filtres " .. (state.showFilterMenu and "ON" or "OFF"))
end) end)

pcall(function() RegisterKeyBind(config.keys.calibratePOI, function()
    if state.calibrationMode then
        calibrateCurrentPOI()
    else
        state.calibrationMode = true
        log("Mode calibration POI ON")
    end
end) end)

log("PalMiniMap Prototype MAX charge.")


local function getWorldActors()
    local actors = {}
    local all = FindAllOf("Actor") or {}
    for _, a in ipairs(all) do
        if isValid(a) then
            table.insert(actors, a)
        end
    end
    return actors
end
