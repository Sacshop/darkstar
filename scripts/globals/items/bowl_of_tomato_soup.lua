-----------------------------------------
-- ID: 4420
-- Item: bowl_of_tomato_soup
-- Food Effect: 3Hrs, All Races
-----------------------------------------
-- Agility 3
-- Vitality -1
-- HP Recovered While Healing 5
-- Ranged Accuracy % 9 (cap 15)
-----------------------------------------

require("scripts/globals/status");

-----------------------------------------
-- OnItemCheck
-----------------------------------------

function onItemCheck(target)
local result = 0;
    if (target:hasStatusEffect(EFFECT_FOOD) == true or target:hasStatusEffect(EFFECT_FIELD_SUPPORT_FOOD) == true) then
        result = 246;
    end
return result;
end;

-----------------------------------------
-- OnItemUse
-----------------------------------------

function onItemUse(target)
    target:addStatusEffect(EFFECT_FOOD,0,0,10800,4420);
end;

-----------------------------------------
-- onEffectGain Action
-----------------------------------------

function onEffectGain(target,effect)
    target:addMod(MOD_AGI, 3);
    target:addMod(MOD_VIT, -1);
    target:addMod(MOD_HPHEAL, 5);
    target:addMod(MOD_FOOD_RACCP, 9);
    target:addMod(MOD_FOOD_RACC_CAP, 15);
end;

-----------------------------------------
-- onEffectLose Action
-----------------------------------------

function onEffectLose(target,effect)
    target:delMod(MOD_AGI, 3);
    target:delMod(MOD_VIT, -1);
    target:delMod(MOD_HPHEAL, 5);
    target:delMod(MOD_FOOD_RACCP, 9);
    target:delMod(MOD_FOOD_RACC_CAP, 15);
end;
