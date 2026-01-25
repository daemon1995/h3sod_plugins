#pragma once
#include "enums.h"
struct CombatCreatureSettings
{

    eTriggerState positiveMorale = DEFAULT;
    eTriggerState negativeMorale = DEFAULT;
    eTriggerState positiveLuck = DEFAULT;
    eTriggerState negativeLuck = DEFAULT;
    eTriggerState spellCasting = DEFAULT;
    eTriggerState doubleAttack = DEFAULT;
    eTriggerState resistance = DEFAULT;
    eTriggerState afterAttackAbility = DEFAULT;
    eTriggerState fear = DEFAULT;

    eDamageState damage = DAMAGE_DEFAULT;
};
