-----------------------------------
-- Shoulder Tackle
-- Hand-to-Hand weapon skill
-- Skill Level: 40
-- Stuns target. Chance of stunning varies with TP.
-- Will stack with Sneak Attack.
-- Aligned with the Aqua Gorget & Thunder Gorget.
-- Aligned with the Aqua Belt & Thunder Belt.
-- Element: None
-- Modifiers: VIT:100%
-- 100%TP    200%TP    300%TP
-- 1.00      1.00      1.00
-----------------------------------
require("scripts/globals/status");
require("scripts/globals/settings");
require("scripts/globals/weaponskills");
-----------------------------------

function onUseWeaponSkill(player, target, wsID)

    local params = {};
    params.numHits = 1;
    params.ftp100 = 1; params.ftp200 = 1; params.ftp300 = 1;
    params.str_wsc = 0.0; params.dex_wsc = 0.0; params.vit_wsc = 0.3; params.agi_wsc = 0.0; params.int_wsc = 0.0; params.mnd_wsc = 0.0; params.chr_wsc = 0.0;
    params.crit100 = 0.0; params.crit200 = 0.0; params.crit300 = 0.0;
    params.canCrit = false;
    params.acc100 = 0.0; params.acc200= 0.0; params.acc300= 0.0;
    params.atkmulti = 1;
    local damage, criticalHit, tpHits, extraHits = doPhysicalWeaponskill(player, target, params);

    if (USE_ADOULIN_WEAPON_SKILL_CHANGES == true) then
        params.vit_wsc = 1.0;
    end

    if damage > 0 then
        local tp = player:getTP();
        local duration = (tp/50);
        if (target:hasStatusEffect(EFFECT_STUN) == false) then
            target:addStatusEffect(EFFECT_STUN, 1, 0, duration);
        end
    end
    damage = damage * WEAPON_SKILL_POWER
    return tpHits, extraHits, criticalHit, damage;

end
