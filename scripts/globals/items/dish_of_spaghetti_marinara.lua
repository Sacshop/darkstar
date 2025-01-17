-----------------------------------------
-- ID: 5719
-- Item: dish_of_spaghetti_marinara
-- Food Effect: 30Min, All Races
-----------------------------------------
-- HP % 15 (cap 120)
-- Vitality 2
-- Defense 5
-- Store TP 7
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
    target:addStatusEffect(EFFECT_FOOD,0,0,1800,5719);
end;

-----------------------------------------
-- onEffectGain Action
-----------------------------------------

function onEffectGain(target,effect)
    target:addMod(MOD_FOOD_HPP, 15);
    target:addMod(MOD_FOOD_HP_CAP, 120);
    target:addMod(MOD_VIT, 2);
    target:addMod(MOD_DEF, 5);
    target:addMod(MOD_STORETP, 7);
end;

-----------------------------------------
-- onEffectLose Action
-----------------------------------------

function onEffectLose(target,effect)
    target:delMod(MOD_FOOD_HPP, 15);
    target:delMod(MOD_FOOD_HP_CAP, 120);
    target:delMod(MOD_VIT, 2);
    target:delMod(MOD_DEF, 5);
    target:delMod(MOD_STORETP, 7);
end;
