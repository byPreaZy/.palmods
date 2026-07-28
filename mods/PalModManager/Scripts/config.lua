--[[
    PalModManager - Configuration
    Fenetre de configuration unifiee pour les mods Palworld ULTRA MAX.
--]]

return {
    modName = "PalModManager",
    enabled = true,
    keybind = "HELP",
    showAtStartup = false,
    windowTitle = "Pal Mod Manager",
    windowWidth = 650,
    windowHeight = 700,

    -- Liste des mods geres par la fenetre
    knownMods = {
        "PalWeight",
        "PalAutoLoot",
        "PalCaptureCounter",
        "PalInspectPal",
        "PalQuickDrop",
        "PalQuickStack",
        "PalInstantFish",
        "PalInstantHatch",
        "PalInstantBreed",
    },
}
