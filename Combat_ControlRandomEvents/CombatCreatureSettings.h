#pragma once
#include "enums.h"
struct CombatCreatureSettings
{
    struct AbilityChanger
    {
        eTriggerState triggerState = DEFAULT;
        UINT duration = 0;
    };
    struct DamageChanger
    {
        eDamageState triggerState = DAMAGE_DEFAULT;
        UINT duration = 0;
    };


    // turn probabilities
    AbilityChanger positiveMorale{};
    AbilityChanger negativeMorale{};
    AbilityChanger fear{};

    // magic abilities
    AbilityChanger spellCasting{};
    AbilityChanger resurrection{};
    AbilityChanger resistance{};

    // damage dealt
    AbilityChanger positiveLuck{};
    AbilityChanger negativeLuck{};
    AbilityChanger doubleDamage{};
    AbilityChanger wallAttackAim{};
    AbilityChanger afterAttackAbility{};

    DamageChanger damage{};

  public:
    static int BattleStack_Random(HiHook *hook, const int min, const int max, const AbilityChanger &triggerState);
};
