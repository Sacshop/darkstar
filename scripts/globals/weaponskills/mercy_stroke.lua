-----------------------------------
-- Mercy Stroke
-- Dagger weapon skill
-- Skill level: N/A
-- Batardeau/Mandau: Temporarily improves params.critical hit rate.
-- Aftermath gives +5% params.critical hit rate.
-- Must have Batardeau, Mandau, or Clement Skean equipped.
-- Aligned with the Shadow Gorget & Soil Gorget.
-- Aligned with the Shadow Belt & Soil Belt.
-- Element: None
-- Modifiers: STR:60%
-- 100%TP    200%TP    300%TP
-- 3.00      3.00      3.00
-----------------------------------
require("scripts/globals/status");
require("scripts/globals/settings");
require("scripts/globals/weaponskills");
-----------------------------------
function onUseWeaponSkill(player, target, wsID)

    local params = {};
    params.numHits = 1;
    params.ftp100 = 3; params.ftp200 = 3; params.ftp300 = 3;
    params.str_wsc = 0.6; params.dex_wsc = 0.0; params.vit_wsc = 0.0; params.agi_wsc = 0.0; params.int_wsc = 0.0; params.mnd_wsc = 0.0; params.chr_wsc = 0.0;
    params.crit100 = 0.0; params.crit200 = 0.0; params.crit300 = 0.0;
    params.canCrit = false;
    params.acc100 = 0.0; params.acc200= 0.0; params.acc300= 0.0;
    params.atkmulti = 1;

    if (USE_ADOULIN_WEAPON_SKILL_CHANGES == true) then
        params.ftp100 = 6; params.ftp200 = 6; params.ftp300 = 6;
        params.str_wsc = 0.8;
    end

    local damage, criticalHit, tpHits, extraHits = doPhysicalWeaponskill(player, target, params);
    -- TODO: Whoever codes those level 85 weapons with the latent that grants this WS needs to code a check to not give the aftermath effect.
    if (damage > 0) then
        local amDuration = 20 * math.floor(player:getTP()/100);
        player:addStatusEffect(EFFECT_AFTERMATH, 5, 0, amDuration, 0, 2);
    end

    damage = damage * WEAPON_SKILL_POWER
    return tpHits, extraHits, criticalHit, damage;
end