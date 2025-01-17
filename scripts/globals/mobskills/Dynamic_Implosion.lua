---------------------------------------------
--  Dynamic Implosion
--
--  Description: Deals damage to players within an area of effect. Additional effect: Stun (Status Effect)
--  Type: Physical 
--  Utsusemi/Blink absorb: Unknown
--  Range: Unknown radial
--  Notes:
---------------------------------------------

require("scripts/globals/settings");
require("scripts/globals/status");
require("scripts/globals/monstertpmoves");

---------------------------------------------

function onMobSkillCheck(target,mob,skill)
    return 0;
end;

function onMobWeaponSkill(target, mob, skill)
    
    local numhits = 1;
    local accmod = 1;
    local dmgmod = 2.5;
    
    local info = MobPhysicalMove(mob,target,skill,numhits,accmod,dmgmod,TP_DMG_VARIES,1,2,3);
    local dmg = MobFinalAdjustments(info.dmg,mob,skill,target,MOBSKILL_PHYSICAL,MOBPARAM_SLASH,info.hitslanded);

    MobPhysicalStatusEffectMove(mob, target, skill, EFFECT_STUN, 1, 0, 7);
    
    target:delHP(dmg);
    return dmg;
    
end;
