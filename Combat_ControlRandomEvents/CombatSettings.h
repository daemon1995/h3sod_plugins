#pragma once
#include "enums.h"
struct CombatStackSettings
{
    static constexpr int CREATURE_ABILITY_TURNS_DURATION = 5;
    static CombatStackSettings combatStackSettings[2][h3::limits::COMBAT_CREATURES + 1];
    
    const H3CombatCreature *creature = nullptr;

    union {
        struct
        {
            // all morale supresses target settings
            // turn probabilities
            Ability positiveMorale; // ::M
            Ability negativeMorale; // ::M
            Ability fear;           // ::G
            // magic abilities
            Ability spellCasting; // ::X (SHIFT +X for dialog opening)
            Ability resurrection; // ::X
            Ability resistance;   // ::J
            Ability magicMirror;  // ::CTRL + J
            // damage dealt
            Ability positiveLuck;       // ::N
            Ability negativeLuck;       // ::N
            Ability doubleDamage;       // ::SHIFT + X
            Ability wallAttackAim;      // ::X
            Ability wallAttackExtended; // ::SHIFT + X
            Ability afterAttackAbility; // ::X

            Ability firstAttackDamage;  // ::K
            Ability secondAttackDamage; // ::SHIFT + K
            Ability inputDirectDamage;  // ::CTRL + K
        };
        Ability asArray[eStackAbility::AMOUNT_OF_STACK_SETTINGS]{};
    };

  public:
    CombatStackSettings() {};

  public:
    inline void Reset()
    {
        *this = {};
    }
    inline const Ability &At(const eStackAbility id) const
    {
        return asArray[id];
    }
    inline const Ability &operator[](const eStackAbility id) const
    {
        return asArray[id];
    }

    BOOL IsAffectedBySetting(const eStackAbility id) const;
    eAbilityStateSwitchError SwitchToNextAbilityState(const eStackAbility id, Ability &outState) const;
    Ability GetNextSpellStateToCast() const noexcept;
    Ability GetNextResurrectionState() const noexcept;
    BOOL TriggerAbility(const eStackAbility id);

  public:
    static int BattleStack_Random(HiHook *hook, const int min, const int max, const Ability &triggerState);
    static int BattleStack_ContinuousRandom(const H3CombatCreature *combatCreature, const eStackAbility stackSettingId,
                                            const eSideAbility sideSettingId, HiHook *hook, const int min,
                                            const int max);
    static void ResetAll();
    static void HandleNewCombatRound();
    static inline CombatStackSettings &GetCombatStackSettings(const H3CombatCreature *creature) noexcept
    {
        return combatStackSettings[creature->side][creature->sideIndex];
    }
    static inline CombatStackSettings &GetCombatStackSettings(const int side, const int index) noexcept
    {
        return combatStackSettings[side][index];
    }
    static inline void AssignCombatCreature(const H3CombatCreature *creature)
    {
        combatStackSettings[creature->side][creature->sideIndex].creature = creature;
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
    static inline void SetCreatureAbilityState(const H3CombatCreature *creature, const eStackAbility settingId,
                                               const Ability &state) noexcept
    {
        combatStackSettings[creature->side][creature->sideIndex].asArray[settingId] = state;
    }
    static inline void SetCreatureAbilityState(const int side, const int index, const eStackAbility settingId,
                                               const Ability &state) noexcept
    {
        combatStackSettings[side][index].asArray[settingId] = state;
    }
};

struct CombatSideSettings
{
    static constexpr int SIDE_ABILITY_TURNS_DURATION = 2;
    static CombatSideSettings sideSettings[2];

    int side = -1;
    union {
        struct
        {
            Ability unaffectedByMorale;     // ::SHIFT + M
            Ability unaffectedByLuck;       // ::SHIFT + N
            Ability unaffectedByFear;       // ::SHIFT + G
            Ability unaffectedByResistance; // ::SHIFT + J
        };
        Ability asArray[AMOUNT_OF_SIDE_SETTINGS]{};
    };

    CombatSideSettings() {};

  public:
    BOOL IsAffectedBySetting(const eSideAbility id) const;
    float GetSideMaxResistance() const;
    static void ResistanceBreachingTriggered(const int side, const int maxStackResistance);
    static void HandleNewCombatRound();

  public:
    eAbilityStateSwitchError SwitchToNextAbilityState(const eSideAbility id, Ability &outState) const;
    BOOL TriggerAbility(const eSideAbility id);

  public:
    static void ResetAll();
    static CombatSideSettings &GetCombatSideSettings(const H3CombatCreature *creature) noexcept
    {
        return sideSettings[creature->side];
    }
    static CombatSideSettings &GetCombatSideSettings(const int side) noexcept
    {
        return sideSettings[side];
    }
    inline const Ability &At(const eSideAbility id) const
    {
        return asArray[id];
    }

    static inline void SetSideAbilityState(const H3CombatCreature *creature, const eSideAbility settingId,
                                           const Ability &state) noexcept
    {
        sideSettings[creature->side].asArray[settingId] = state;
    }

    static inline void SetSideAbilityState(const int side, const eSideAbility settingId, const Ability &state) noexcept
    {
        sideSettings[side].asArray[settingId] = state;
    }
};
