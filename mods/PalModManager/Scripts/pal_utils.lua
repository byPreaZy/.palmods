--[[
    PalModManager - Utilitaires partages (optionnels)
    Peut etre charge par d'autres mods via loadfile si disponible.
--]]

local U = {}

function U.isValid(obj)
    if obj == nil then return false end
    if type(obj) ~= "table" then return true end
    if obj.IsValid == nil then return true end
    local ok, valid = pcall(function() return obj:IsValid() end)
    return ok and valid
end

function U.safeGet(obj, path)
    if obj == nil then return nil end
    local current = obj
    for part in string.gmatch(path, "[^.]+") do
        local ok, nextObj = pcall(function() return current[part] end)
        if not ok or nextObj == nil then return nil end
        current = nextObj
    end
    return current
end

function U.safeSet(obj, path, value)
    if not U.isValid(obj) then return false end
    local current = obj
    local parts = {}
    for part in string.gmatch(path, "[^.]+") do
        table.insert(parts, part)
    end
    for i = 1, #parts - 1 do
        local ok, nextObj = pcall(function() return current[parts[i]] end)
        if not ok or nextObj == nil then return false end
        current = nextObj
    end
    local ok = pcall(function() current[parts[#parts]] = value end)
    return ok
end

function U.getPlayerController()
    local all = FindAllOf and FindAllOf("PlayerController")
    if all then
        for _, pc in ipairs(all) do
            if U.isValid(pc) then return pc end
        end
    end
    local pc = FindFirstOf and FindFirstOf("PlayerController")
    return U.isValid(pc) and pc or nil
end

function U.getPlayerCharacter()
    local pc = U.getPlayerController()
    if U.isValid(pc) then
        local char = U.safeGet(pc, "Pawn") or U.safeGet(pc, "Character")
        if U.isValid(char) then return char end
    end
    local candidates = { "PalPlayerCharacter", "BP_PlayerCharacter_Female_C", "BP_PlayerCharacter_Male_C", "PlayerCharacter" }
    for _, name in ipairs(candidates) do
        local obj = FindFirstOf and FindFirstOf(name)
        if U.isValid(obj) then return obj end
    end
    return nil
end

function U.getPlayerState()
    local pc = U.getPlayerController()
    if U.isValid(pc) then
        local ps = U.safeGet(pc, "PlayerState")
        if U.isValid(ps) then return ps end
    end
    local ps = FindFirstOf and FindFirstOf("PalPlayerState") or FindFirstOf and FindFirstOf("PlayerState")
    return U.isValid(ps) and ps or nil
end

function U.getPlayerLocation()
    local char = U.getPlayerCharacter()
    if not U.isValid(char) then return nil end
    local ok, loc = pcall(function() return char:GetActorLocation() end)
    if ok and loc and loc.X ~= nil then return loc end
    return nil
end

function U.getActorLocation(actor)
    if not U.isValid(actor) then return nil end
    local ok, loc = pcall(function() return actor:GetActorLocation() end)
    if ok and loc and loc.X ~= nil then return loc end
    return nil
end

function U.distance2D(a, b)
    if not a or not b then return nil end
    local dx = (a.X or 0) - (b.X or 0)
    local dy = (a.Y or 0) - (b.Y or 0)
    return math.sqrt(dx * dx + dy * dy)
end

function U.distance3D(a, b)
    if not a or not b then return nil end
    local dx = (a.X or 0) - (b.X or 0)
    local dy = (a.Y or 0) - (b.Y or 0)
    local dz = (a.Z or 0) - (b.Z or 0)
    return math.sqrt(dx * dx + dy * dy + dz * dz)
end

function U.distance2DSquared(a, b)
    if not a or not b then return nil end
    local dx = (a.X or 0) - (b.X or 0)
    local dy = (a.Y or 0) - (b.Y or 0)
    return dx * dx + dy * dy
end

function U.keyFromString(keyName)
    if not keyName then return nil end
    if not Key then return nil end
    local k = Key[keyName]
    if k then return k end
    local upper = string.upper(keyName)
    k = Key[upper]
    if k then return k end
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
    local alias = aliases[upper]
    if alias and Key[alias] then return Key[alias] end
    return nil
end

function U.tryCall(obj, methods, ...)
    if not U.isValid(obj) then return nil end
    local args = {...}
    local n = select("#", ...)
    for _, m in ipairs(methods) do
        local ok, fn = pcall(function() return obj[m] end)
        if ok and type(fn) == "function" then
            local ok2, res = pcall(function() return fn(obj, table.unpack(args, 1, n)) end)
            if ok2 then return res, m end
        end
    end
    return nil
end

return U
