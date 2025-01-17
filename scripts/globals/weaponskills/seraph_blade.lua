-----------------------------------
-- Seraph Blade
-- Sword weapon skill
-- Skill Level: 125
-- Deals light elemental damage to enemy. Damage varies with TP.
-- Ignores shadows.
-- Aligned with the Soil Gorget.
-- Aligned with the Soil Belt.
-- Element: Light
-- Modifiers: STR:40% ; MND:40%
-- 100%TP    200%TP    300%TP
-- 1.125      2.625      4.125
-----------------------------------
require("scripts/globals/magic");
require("scripts/globals/status");
require("scripts/globals/settings");
require("scripts/globals/weaponskills");
-----------------------------------

function onUseWeaponSkill(player, target, wsID)

    local params = {};
    params.ftp100 = 1; params.ftp200 = 2.5; params.ftp300 = 3;
    params.str_wsc = 0.3; params.dex_wsc = 0.0; params.vit_wsc = 0.0; params.agi_wsc = 0.0; params.int_wsc = 0.0; params.mnd_wsc = 0.3; params.chr_wsc = 0.0;
    params.ele = ELE_LIGHT;
    params.skill = SKILL_SWD;
    params.includemab = true;

    if (USE_ADOULIN_WEAPON_SKILL_CHANGES == true) then
        params.ftp100 = 1.125; params.ftp200 = 2.625; params.ftp300 = 4.125;
        params.str_wsc = 0.4; params.mnd_wsc = 0.4;
    end

    local damage, criticalHit, tpHits, extraHits = doMagicWeaponskill(player, target, params);
    damage = damage * WEAPON_SKILL_POWER
    return tpHits, extraHits, criticalHit, damage;

end
