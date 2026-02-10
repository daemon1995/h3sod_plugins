#pragma once
#include "enums.h"
struct CombatStackSettings
{

    static CombatStackSettings combatStackSettings[2][h3::limits::COMBAT_CREATURES + 1];

    const H3CombatCreature *creature = nullptr;

    union {
        struct
        {
            // all morale supresses target settings
            // turn probabilities
            AbilityState positiveMorale; // ::M
            AbilityState negativeMorale; // ::M
            AbilityState fear;           // ::G
            // magic abilities
            AbilityState spellCasting; // ::X (SHIFT +X for dialog opening)
            AbilityState resurrection; // ::X
            AbilityState resistance;   // ::J
            AbilityState magicMirror;  // ::CTRL + J
            // damage dealt
            AbilityState positiveLuck;       // ::N
            AbilityState negativeLuck;       // ::N
            AbilityState doubleDamage;       // ::SHIFT + X
            AbilityState wallAttackAim;      // ::X
            AbilityState afterAttackAbility; // ::X

            AbilityState firstAttackDamage;  // ::K
            AbilityState secondAttackDamage; // ::SHIFT + K
            AbilityState inputDirectDamage;  // ::CTRL + K
        };
        AbilityState asArray[eStackSettingsId::AMOUNT_OF_STACK_SETTINGS]{};
    };

  public:
    CombatStackSettings() {};

  public:
    inline void Reset()
    {
        *this = {};
    }
    inline const AbilityState &At(const eStackSettingsId id) const
    {
        return asArray[id];
    }
    inline const AbilityState &operator[](const eStackSettingsId id) const
    {
        return asArray[id];
    }

    void DecreaseDurations(const eStackSettingsId id);
    BOOL IsAffectedBySetting(const eStackSettingsId id) const;
    AbilityState GetNextAbilityState(const eStackSettingsId id) const;
    AbilityState GetNextSpellStateToCast() const noexcept;

  public:
    static int  BattleStack_Random(HiHook *hook, const int min, const int max, const AbilityState &triggerState);

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
                                               const AbilityState &state) noexcept
    {
        combatStackSettings[creature->side][creature->sideIndex].asArray[settingId] = state;
    }
    static inline void AssignCombatCreature(const H3CombatCreature *creature)
    {
        combatStackSettings[creature->side][creature->sideIndex].creature = creature;
    }
    static inline void SetCreatureAbilityState(const int side, const int index, const eStackSettingsId settingId,
                                               const AbilityState &state) noexcept
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
        struct
        {
            AbilityState unaffectedByMorale;     // ::SHIFT + M
            AbilityState unaffectedByLuck;       // ::SHIFT + N
            AbilityState unaffectedByFear;       // ::SHIFT + G
            AbilityState unaffectedByResistance; // ::SHIFT + J
        };
        AbilityState asArray[AMOUNT_OF_SIDE_SETTINGS]{};
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
