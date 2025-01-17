-----------------------------------------
-- ID: 5219
-- Item: plate_of_ikra_gunkan
-- Food Effect: 30Min, All Races
-----------------------------------------
-- Health 30
-- Magic 10
-- Dexterity 3
-- Mind -2
-- Accuracy % 17
-- Accuracy Cap 28
-- Ranged ACC % 17
-- Ranged ACC Cap 28
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
    target:addStatusEffect(EFFECT_FOOD,0,0,1800,5219);
end;

-----------------------------------
-- onEffectGain Action
-----------------------------------

function onEffectGain(target,effect)
    target:addMod(MOD_HP, 30);
    target:addMod(MOD_MP, 10);
    target:addMod(MOD_DEX, 3);
    target:addMod(MOD_MND, -2);
    target:addMod(MOD_FOOD_ACCP, 17);
    target:addMod(MOD_FOOD_ACC_CAP, 28);
    target:addMod(MOD_FOOD_RACCP, 17);
    target:addMod(MOD_FOOD_RACC_CAP, 28);
end;

-----------------------------------------
-- onEffectLose Action
-----------------------------------------

function onEffectLose(target,effect)
    target:delMod(MOD_HP, 30);
    target:delMod(MOD_MP, 10);
    target:delMod(MOD_DEX, 3);
    target:delMod(MOD_MND, -2);
    target:delMod(MOD_FOOD_ACCP, 17);
    target:delMod(MOD_FOOD_ACC_CAP, 28);
    target:delMod(MOD_FOOD_RACCP, 17);
    target:delMod(MOD_FOOD_RACC_CAP, 28);
end;
