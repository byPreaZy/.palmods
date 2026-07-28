--[[
    PalAutoLoot - Configuration
    Ramassage automatique des objets au sol.
--]]

return {
    modName = "PalAutoLoot",
    enabled = false,
    keybind = "CAPS_LOCK",
    radius = 1500,        -- rayon en cm (15m par defaut)
    intervalMs = 1000,  -- verification toutes les 1s
    classes = "PalItemPickup,PalDropItem,PalDroppedItem,PalPickupObject,PalItemActor,BP_PalItemActor_C,PalLootBox",
}
