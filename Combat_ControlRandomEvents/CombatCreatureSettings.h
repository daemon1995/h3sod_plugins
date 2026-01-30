#pragma once
#include "enums.h"
struct CombatCreatureSettings
{
    CombatCreatureSettings();
    const H3CombatCreature *ownerCreature = nullptr;

    union {
        struct Abilities
        {

            // all morale supresses target settings
            // turn probabilities
            AbilityChanger positiveMorale{};    // ::M
            AbilityChanger positiveMoraleAll{}; // ::SHIFT + M
            AbilityChanger negativeMorale{};    // 1 || M && SHIFT + M
            AbilityChanger negativeMoraleAll{}; // 1 || 0 ?? ::M && SHIFT + M

            AbilityChanger fear{}; // 2 ::G

            // magic abilities
            AbilityChanger spellCasting{}; // 3  ::X (SHIFT +X for dialog opening)
            AbilityChanger resurrection{}; // 4  ::X
            AbilityChanger resistance{};   // 5  ::J
            // AbilityChanger massCastResistance{};   // 5  ::SHIFT + J

            // damage dealt
            AbilityChanger positiveLuck{};    // 6 :: N
            AbilityChanger positiveLuckAll{}; // 6 :: SHIFT + N
            AbilityChanger negativeLuck{};
            AbilityChanger negativeLuckAll{};
            AbilityChanger doubleDamage{};       // 7 :: SHIFT + X
            AbilityChanger wallAttackAim{};      // 8 ::X
            AbilityChanger afterAttackAbility{}; // 9 ::X

            AbilityChanger firstAttackDamage{};  // 10 ::K
            AbilityChanger secondAttackDamage{}; // 11 ::SHIFT + K
        } abilities;
        AbilityChanger asArray[eSettingsId::AMOUNT_OF_SETTINGS]{};
    };

  public:
    void ResetSettings();
    void DecreaseDurations();
    const AbilityChanger &At(const eSettingsId id) const;
    AbilityChanger &operator[](const eSettingsId id);
    BOOL IsAffected(const eSettingsId id, const H3CombatCreature *creature = nullptr) const;

  public:
    static int BattleStack_Random(HiHook *hook, const int min, const int max, const AbilityChanger &triggerState);
};
