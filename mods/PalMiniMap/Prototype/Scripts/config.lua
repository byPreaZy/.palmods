--[[
    PalMiniMap Prototype - Configuration MAX
    Super minimap Lua via ImGui pour Palworld 1.0 / UE4SS
--]]

local config = {}

config.modPrefix = "[PalMiniMap]"
config.debug = true

config.keys = {
    toggleMap     = Key.NUM_ZERO,
    toggleMove    = Key.NUM_ONE,
    zoomIn        = Key.ADD,
    zoomOut       = Key.SUBTRACT,
    superZoom     = Key.MULTIPLY,
    toggleRotation = Key.DIVIDE,
    toggleFilterMenu = Key.NUM_TWO,
    calibratePOI  = Key.NUM_THREE,
}

config.display = {
    enabled = true,
    posX = 20,
    posY = 20,
    size = 320,
    opacity = 0.92,
    zoom = 1.5,
    superZoom = 5.0,
    rotateWithCamera = true,
    showCoordinates = true,
    showCompass = true,
    showGrid = true,
    -- Rendu du terrain
    showTerrain = true,
    terrainResolution = 32,       -- nombre de cellules de la grille terrain (plus = plus precis)
    terrainRange = 800.0,         -- portee du scan terrain autour du joueur (metres)
    terrainMode = "heightmap",    -- heightmap | satellite | topographic
    terrainUpdateInterval = 0.5,  -- secondes entre mises a jour du cache terrain
    useEntitySamples = true,      -- utiliser les entites proches pour ameliorer la heightmap
    -- Points d'interet publics
    showPOI = true,
    showPOITowers = true,
    showPOIFastTravel = true,
    showPOIAlpha = true,
    showPOIDungeons = true,
    showPOISanctuaries = true,
    showPOISealed = true,
    showPOIBounty = true,
    showPOISkillFruit = true,
    showPOIMerchants = true,
    showPOIEffigies = false,      -- desactive par defaut (trop dense)
    showPOIResources = false,     -- desactive par defaut
    showPOIRelics = false,        -- desactive par defaut
    showPOILabels = false,
    -- Marqueurs
    colorByPalType = true,        -- colorer les Pals selon leur type elementaire
    showPalType = true,           -- afficher le type elementaire dans le label
}

config.filters = {
    player = true,
    pals = true,
    enemyPals = true,
    selectedPalsOnly = false,  -- si true, n'affiche que les Pals de la liste selectedPals
    selectedPals = {},         -- liste d'IDs de Pals a afficher (ex: {"Lamball", "Pengullet"})
    chests = true,
    eggs = true,
    dungeons = true,
    fastTravel = true,
    players = true,
    playerArrows = true,       -- fleches directionnelles pour les autres joueurs
    resources = false,
}

config.colors = {
    player    = 0xFF00FF00,
    pal       = 0xFF00FFFF,
    enemy     = 0xFF0000FF,
    chest     = 0xFFFF8000,
    egg       = 0xFFFFFF00,
    dungeon   = 0xFFFF00FF,
    fastTravel= 0xFF00FF80,
    tower     = 0xFFFF00FF,
    alpha     = 0xFF0000FF,
    sanctuary = 0xFF00FF80,
    sealed    = 0xFF00FFFF,
    bounty    = 0xFFFF8000,
    skillFruit= 0xFF80FF80,
    merchant  = 0xFFFFFF00,
    effigy    = 0xFF00FF00,
    resource  = 0xFF8080FF,
    relic     = 0xFFFF00FF,
    playerOther=0xFFFFFFFF,
    neutral   = 0xFFFFFFFF,
    grid      = 0x44FFFFFF,
    compass   = 0xFF00FF00,
    -- Terrain
    water     = 0xFF1040FF,
    beach     = 0xFFCCCC60,
    grass     = 0xFF108010,
    forest    = 0xFF206020,
    rock      = 0xFF606060,
    snow      = 0xFFFFFFFF,
    slope     = 0xFF404040,
    -- Joueurs multijoueur
    playerArrow = 0xFFFFFF00,
}

config.ranges = {
    pals = 1000,
    chests = 400,
    eggs = 600,
    dungeons = 2000,
    fastTravel = 1500,
    players = 1000,
    resources = 600,
    poi = 3000,
    tower = 2500,
    fastTravel = 2000,
    alpha = 2500,
    dungeon = 2000,
    sanctuary = 2500,
    sealed = 2000,
    bounty = 2000,
    skillFruit = 1500,
    merchant = 2000,
    effigy = 1200,
    resource = 1000,
    relic = 2000,
}

return config
