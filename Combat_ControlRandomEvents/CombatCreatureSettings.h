#pragma once
#include "enums.h"
struct CombatCreatureSettings
{
    CombatCreatureSettings();


    union {
        struct Abilities
        {
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

            AbilityChanger damage{};
        } abilities;
        AbilityChanger asArray[eSettingsId::AMOUNT_OF_SETTINGS]{};
    };

  public:
    void ResetSettings();
    void DecreaseDurations();
    const AbilityChanger &At(const eSettingsId id) const;
    AbilityChanger &operator[](const eSettingsId id);

  public:
    static int BattleStack_Random(HiHook *hook, const int min, const int max, const AbilityChanger &triggerState);
};
