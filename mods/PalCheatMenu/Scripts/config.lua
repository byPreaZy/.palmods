--[[
    PalCheatMenu - Configuration MAX
    Tous les raccourcis et parametres sont modifiables ici.
--]]

local config = {}

-- Prefixe des logs dans la console UE4SS
config.modPrefix = "[PalCheatMenu]"

-- Touches de raccourci (voir UE4SS Key enum)
-- Les combinaisons Ctrl/Shift ne sont pas natives dans RegisterKeyBind,
-- on utilise donc des touches simples supplementaires.
config.keys = {
    -- Menu principal
    toggleMenu         = Key.F1,
    toggleAll          = Key.PAUSE,

    -- Joueur
    toggleGodMode      = Key.INS,
    toggleStamina      = Key.F3,
    toggleHungerThirst = Key.F4,
    toggleNoFallDamage = Key.F5,
    toggleFly          = Key.F6,
    toggleNoClip       = Key.F7,
    toggleInvisibility = Key.F8,
    toggleSpeed        = Key.F9,
    toggleJump         = Key.NUM_LOCK,
    toggleAmmo         = Key.F11,

    -- Combat
    toggleNoReload     = nil,
    toggleOneHitKill   = nil,

    -- Inventaire / Craft
    toggleInstantCraft = nil,
    toggleItems        = nil,
    toggleIgnoreCost   = nil,
    toggleDuplicate    = nil,

    -- Pal
    togglePalGodMode   = nil,
    togglePalSanity    = nil,
    toggleInstantHatch = nil,
    toggleMaxPalStats  = nil,

    -- Monde / Progression
    addExperience      = nil,
    addTechPoints      = nil,
    toggleTimeFreeze   = nil,
    setDayTime         = nil,
    unlockFastTravel   = nil,
    teleportForward    = nil,
    setWeatherClear    = nil,

    -- Avance
    toggleNoRecoil     = nil,
    toggleNoSpread     = nil,
    toggleRapidFire    = nil,
    toggleInfiniteDurability = nil,
    toggleRevealMap    = nil,
    toggleMaxCaptureRate = nil,
    togglePalFastWork  = nil,
    togglePalNoHunger  = nil,
    addLevels          = nil,
    spawnItem          = nil,
}

-- Parametres de gameplay modifies (MAX)
config.values = {
    superSpeedMultiplier = 5.0,    -- x5 vitesse par defaut
    superJumpVelocity = 5000.0,    -- saut demesure
    experienceAmount = 1000000,    -- +1 000 000 XP
    techPointsAmount = 100,        -- +100 points tech
    timeHour = 12.0,               -- midi
    teleportDistance = 1000.0,     -- teleportation en metres
    addLevelCount = 50,            -- niveaux ajoutes
    spawnItemId = "PalSphere",     -- item a spawner (identifiant)
    spawnItemCount = 1,            -- quantite a spawner
    rapidFireRate = 10.0,          -- multiplicateur de cadence de tir
    captureRate = 100.0,           -- taux de capture x
}

-- Plages d'ajustement pour les sliders
config.limits = {
    minSpeedMultiplier = 1.0,
    maxSpeedMultiplier = 20.0,
    minJumpVelocity = 500.0,
    maxJumpVelocity = 20000.0,
    minExperience = 1000,
    maxExperience = 10000000,
    minTechPoints = 1,
    maxTechPoints = 1000,
    minTimeHour = 0.0,
    maxTimeHour = 24.0,
    minAddLevel = 1,
    maxAddLevel = 100,
    minSpawnCount = 1,
    maxSpawnCount = 999,
    minRapidFire = 1.0,
    maxRapidFire = 50.0,
    minCaptureRate = 1.0,
    maxCaptureRate = 1000.0,
}

-- Liste des cheats actifs par defaut
config.cheats = {
    -- Joueur
    godMode          = false,
    unlimitedStamina = false,
    unlimitedHungerThirst = false,
    noFallDamage     = false,
    fly              = false,
    noClip           = false,
    invisibility     = false,
    superSpeed       = false,
    superJump        = false,

    -- Combat
    unlimitedAmmo    = false,
    noReload         = false,
    oneHitKill       = false,

    -- Inventaire / Craft
    instantCraft     = false,
    unlimitedItems   = false,
    ignoreCost       = false,
    duplicateItems   = false,

    -- Pal
    palGodMode       = false,
    palSanity        = false,
    instantHatch     = false,
    maxPalStats      = false,
    palFastWork      = false,
    palNoHunger      = false,

    -- Monde
    freezeTime       = false,

    -- Avance
    noRecoil         = false,
    noSpread         = false,
    rapidFire        = false,
    infiniteDurability = false,
    revealMap        = false,
    maxCaptureRate   = false,
}

-- Logging verbeux pour le debug (a desactiver en release)
config.debug = true

return config
