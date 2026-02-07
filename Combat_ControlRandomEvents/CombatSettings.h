#pragma once
#include "enums.h"
struct CombatStackSettings
{

    static CombatStackSettings combatStackSettings[2][h3::limits::COMBAT_CREATURES + 1];

    const H3CombatCreature *creature = nullptr;

    union {
        struct Settings
        {
            // all morale supresses target settings
            // turn probabilities
            AbilityChanger positiveMorale{}; // ::M
            AbilityChanger negativeMorale{}; // ::M
            AbilityChanger fear{};           // ::G
            // magic abilities
            AbilityChanger spellCasting{}; // ::X (SHIFT +X for dialog opening)
            AbilityChanger resurrection{}; // ::X
            AbilityChanger resistance{};   // ::J
            AbilityChanger magicMirror{};  // ::CTRL + J
            // damage dealt
            AbilityChanger positiveLuck{};       // ::N
            AbilityChanger negativeLuck{};       // ::N
            AbilityChanger doubleDamage{};       // ::SHIFT + X
            AbilityChanger wallAttackAim{};      // ::X
            AbilityChanger afterAttackAbility{}; // ::X

            AbilityChanger firstAttackDamage{};  // ::K
            AbilityChanger secondAttackDamage{}; // ::SHIFT + K
        } settings;
        AbilityChanger asArray[eStackSettingsId::AMOUNT_OF_STACK_SETTINGS]{};
    };

  public:
    CombatStackSettings() {};

  public:
    inline void Reset()
    {
        *this = {};
    }
    inline const AbilityChanger &At(const eStackSettingsId id) const
    {
        return asArray[id];
    }
    inline const AbilityChanger &operator[](const eStackSettingsId id) const
    {
        return asArray[id];
    }

    void DecreaseDurations();
    BOOL IsAffectedBySetting(const eStackSettingsId id) const;

  public:
    static int BattleStack_Random(HiHook *hook, const int min, const int max, const AbilityChanger &triggerState);

    static inline const CombatStackSettings &GetCombatStackSettings(const H3CombatCreature *creature) noexcept
    {
        return combatStackSettings[creature->side][creature->sideIndex];
    }
    static inline const CombatStackSettings &GetCombatStackSettings(const int side, const int index) noexcept
    {
        return combatStackSettings[side][index];
    }

    static inline void SetCombatStackSettings(const H3CombatCreature *creature,
                                              const CombatStackSettings &settings) noexcept
    {
        combatStackSettings[creature->side][creature->sideIndex] = settings;
    }
    static inline void SetCombatStackSettings(const int side, const int index,
                                              const CombatStackSettings &settings) noexcept
    {
        combatStackSettings[side][index] = settings;
    }
    static inline void SetCreatureAbilityState(const H3CombatCreature *creature, const eStackSettingsId settingId,
                                               const AbilityChanger &state) noexcept
    {
        combatStackSettings[creature->side][creature->sideIndex].asArray[settingId] = state;
    }
    static inline void AssignCombatCreature(const H3CombatCreature *creature)
    {
        combatStackSettings[creature->side][creature->sideIndex].creature = creature;
    }
    static inline void SetCreatureAbilityState(const int side, const int index, const eStackSettingsId settingId,
                                               const AbilityChanger &state) noexcept
    {
        combatStackSettings[side][index].asArray[settingId] = state;
    }
    static void ResetAll();
};

struct CombatSideSettings
{
    static CombatSideSettings sideSettings[2];
    int side = -1;
    union {
        struct Settings
        {
            AbilityChanger unaffectedByMorale{};     // ::SHIFT + M
            AbilityChanger unaffectedByLuck{};       // ::SHIFT + N
            AbilityChanger unaffectedByFear{};       // ::SHIFT + G
            AbilityChanger unaffectedByResistance{}; // ::SHIFT + J
        } settings;
        AbilityChanger asArray[AMOUNT_OF_SIDE_SETTINGS]{};
    };

    CombatSideSettings() {};

  public:
    BOOL IsAffectedBySetting(const eSideSettingsId id) const;
    float GetSideMaxResistance() const;

  public:
    static void ResetAll();
    static const CombatSideSettings &GetCombatSideSettings(const H3CombatCreature *creature) noexcept
    {
        return sideSettings[creature->side];
    }
    static const CombatSideSettings &GetCombatSideSettings(const int side) noexcept
    {
        return sideSettings[side];
    }
};
