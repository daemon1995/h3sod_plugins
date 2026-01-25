#pragma once
#include "enums.h"
struct CombatCreatureSettings
{

	// turn probabilities
    eTriggerState positiveMorale = DEFAULT;
    eTriggerState negativeMorale = DEFAULT;
    eTriggerState fear = DEFAULT;

	// magic abilities
    eTriggerState spellCasting = DEFAULT;
    eTriggerState resurrection = DEFAULT;
    eTriggerState resistance = DEFAULT;

    // damage dealt
    eTriggerState positiveLuck = DEFAULT;
    eTriggerState negativeLuck = DEFAULT;
    eTriggerState doubleDamage = DEFAULT;
    eTriggerState wallAttackAim = DEFAULT;
    eTriggerState afterAttackAbility = DEFAULT;
    eDamageState damage = DAMAGE_DEFAULT;
};
