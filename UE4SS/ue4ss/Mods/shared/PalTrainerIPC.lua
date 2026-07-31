--[[
    PalTrainerIPC - Inter-process communication helper for PalTrainer external app.
    Reads/writes plain text files in ue4ss/PalTrainer/.
    Commands:  PalTrainer/commands.txt   (key=value)
    Status:    PalTrainer/status.txt     (key=value)

    Usage:
        local ipc = require("PalTrainerIPC")
        local cmds = ipc.pollCommands()
        local state = ipc.getCommand(cmds, "mod.PalWeight")
        ipc.writeStatus({ ["mod.PalWeight"] = true })
--]]

local M = {}

M.dir           = "PalTrainer"
M.commandsPath  = M.dir .. "/commands.txt"
M.statusPath    = M.dir .. "/status.txt"

function M.ensureDir()
    -- Best-effort directory creation, silenced for Windows CMD
    pcall(function() os.execute('mkdir "' .. M.dir .. '" >nul 2>nul') end)
end

function M.readFile(path)
    local f = io.open(path, "r")
    if not f then return nil end
    local content = f:read("*a")
    f:close()
    return content
end

function M.writeFile(path, content)
    local f = io.open(path, "w")
    if not f then return false end
    f:write(content or "")
    f:close()
    return true
end

function M.parse(content)
    local t = {}
    if not content then return t end
    for line in content:gmatch("[^\r\n]+") do
        line = line:gsub("^%s+", ""):gsub("%s+$", "")
        if line ~= "" and line:sub(1,1) ~= "#" and line:sub(1,1) ~= ";" then
            local k, v = line:match("^([^=]+)%s*=%s*(.*)$")
            if k then
                k = k:gsub("^%s+", ""):gsub("%s+$", "")
                t[k] = v
            end
        end
    end
    return t
end

-- Read current commands file
function M.pollCommands()
    return M.parse(M.readFile(M.commandsPath))
end

-- Coerce a string value to boolean / toggle / nil
function M.toBool(value)
    if value == nil then return nil end
    local v = tostring(value):lower()
    if v == "true" or v == "1" or v == "on" or v == "yes" then return true end
    if v == "false" or v == "0" or v == "off" or v == "no" then return false end
    if v == "toggle" then return "toggle" end
    return nil
end

-- Get a command by exact key
function M.getCommand(commands, key)
    if not commands then return nil end
    return M.toBool(commands[key])
end

-- Remove a set of keys from commands file (used for one-shot actions)
function M.consumeKeys(keysToRemove)
    if not keysToRemove or next(keysToRemove) == nil then return end
    local content = M.readFile(M.commandsPath)
    if not content then return end
    local out = {}
    for line in content:gmatch("[^\r\n]+") do
        local k = line:match("^%s*([^=%s]+)%s*=")
        if k then
            k = k:gsub("^%s+", ""):gsub("%s+$", "")
            if not keysToRemove[k] then
                table.insert(out, line)
            end
        else
            table.insert(out, line)
        end
    end
    M.writeFile(M.commandsPath, table.concat(out, "\n") .. "\n")
end

-- Write status table as key=value lines
function M.writeStatus(statusTable)
    if not statusTable then return end
    local lines = {}
    for k, v in pairs(statusTable) do
        table.insert(lines, k .. "=" .. tostring(v))
    end
    M.writeFile(M.statusPath, table.concat(lines, "\n") .. "\n")
end


M.mapPath = M.dir .. "/map.json"

-- Minimal JSON serializer
function M.toJSON(value)
    local t = type(value)
    if t == "table" then
        -- array-like ?
        local isArray = #value > 0
        if isArray then
            local parts = {}
            for _, v in ipairs(value) do
                table.insert(parts, M.toJSON(v))
            end
            return "[" .. table.concat(parts, ",") .. "]"
        else
            local parts = {}
            for k, v in pairs(value) do
                table.insert(parts, string.format("%q:%s", tostring(k), M.toJSON(v)))
            end
            return "{" .. table.concat(parts, ",") .. "}"
        end
    elseif t == "number" then
        return tostring(value)
    elseif t == "boolean" then
        return value and "true" or "false"
    else
        return string.format("%q", tostring(value))
    end
end

function M.writeMap(mapTable)
    return M.writeFile(M.mapPath, M.toJSON(mapTable))
end

M.ensureDir()

return M
