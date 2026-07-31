--[[
    PalSpawner - Configuration
]]

local config = {}

config.modPrefix = "[PalSpawner]"
config.debug = true

-- Keybind to open/close the spawner menu
config.toggleKey = Key.F2

-- Default spawn settings
config.defaults = {
    level = 50,
    gender = 0,        -- 0 = random, 1 = male, 2 = female
    hp = 5000,
    attack = 500,
    defense = 500,
    stamina = 5000,
    passive1 = "None",
    passive2 = "None",
    passive3 = "None",
    passive4 = "None",
    spawnMode = "palbox",  -- "palbox" or "near"
}

-- Level limits
config.limits = {
    minLevel = 1,
    maxLevel = 100,
    minHP = 100,
    maxHP = 99999,
    minAttack = 1,
    maxAttack = 9999,
    minDefense = 1,
    maxDefense = 9999,
    minStamina = 100,
    maxStamina = 99999,
}

return config
