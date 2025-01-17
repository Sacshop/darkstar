-----------------------------------
--
-- EFFECT_ACCURACY_DOWN
--
-----------------------------------

require("scripts/globals/status");

-----------------------------------
-- onEffectGain Action
-----------------------------------

function onEffectGain(target,effect)
    if (effect:getPower()>100) then
        effect:setPower(50);
    end
    target:addMod(MOD_ACCP,-effect:getPower());
end;

-----------------------------------
-- onEffectTick Action
-----------------------------------

function onEffectTick(target,effect)
    -- the effect restore accuracy of 1 every 3 ticks.
    local downACC_effect_size = effect:getPower()
    if (downACC_effect_size > 0) then
        effect:setPower(downACC_effect_size - 1)
        target:delMod(MOD_ACC,-1);
    end
end;

-----------------------------------
-- onEffectLose Action
-----------------------------------

function onEffectLose(target,effect)
    local downACC_effect_size = effect:getPower()
    if (downACC_effect_size > 0) then
        target:delMod(MOD_ACCP,-effect:getPower());
    end
end;