SvConfig = {}

--[[
    Who may paint and clean synced blood. The defaults use ACE permissions, so add to server.cfg:

        add_ace group.admin bloodfx.paint allow
        add_ace group.admin bloodfx.clean allow

    Swap these for a job check if you prefer, for example with QBCore:

        local QBCore = exports['qb-core']:GetCoreObject()
        function SvConfig.CanPaint(source)
            local player = QBCore.Functions.GetPlayer(source)
            return player ~= nil and player.PlayerData.job.name == 'police'
        end
]]
function SvConfig.CanPaint(source)
    return IsPlayerAceAllowed(tostring(source), 'bloodfx.paint')
end

function SvConfig.CanClean(source)
    return IsPlayerAceAllowed(tostring(source), 'bloodfx.clean') or SvConfig.CanPaint(source)
end
