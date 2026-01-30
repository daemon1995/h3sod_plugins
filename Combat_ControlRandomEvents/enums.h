#pragma once

enum eTriggerState : unsigned int
{
    DEFAULT = 0,
    ALWAYS = 1,
    NEVER = 2,
    AMOUNT_OF_TRIGGER_STATES
};
enum eDamageState : unsigned int
{
    DAMAGE_DEFAULT = 0,
    DAMAGE_RANDOM = DAMAGE_DEFAULT,
    DAMAGE_MINIMUM = 1,
    DAMAGE_MAXIMUM = 2,
    DAMAGE_MIN_25 = 3,
    DAMAGE_MIN_50 = 4,
    DAMAGE_MIN_75 = 5,
    AMOUNT_OF_DAMAGE_STATES
};

enum eSettingsId
{
    NONE = -1,
    POSITIVE_MORALE_UNIT,
    POSITIVE_MORALE_ALL,
    NEGATIVE_MORALE_UNIT,
    NEGATIVE_MORALE_ALL,
    FEAR,
    SPELL_CASTING,
    RESURRECTION,
    MAGIC_RESISTANCE,
    //MASS_MAGIC_RESISTANCE,
    POSITIVE_LUCK_UNIT,
    POSITIVE_LUCK_ALL,
    NEGATIVE_LUCK_UNIT,
    NEGATIVE_LUCK_ALL,
    DOUBLE_DAMAGE,
    WALL_ATTACK,
    AFTER_ATTACK_ABILITY,
    DAMAGE_VARIATION_FIRST,
    DAMAGE_VARIATION_SECOND,
    AMOUNT_OF_SETTINGS
};
enum eAbilitySwitchError
{
    ABILITY_SWITCH_NO_ERROR = 0,
    ABILITY_SWITCH_NO_ATTEMPTS_LEFT,
    ABILITY_SWITCH_NO_EFFECT,
    ABILITY_SWITCH_NO_ABILITY,
};
struct AbilityChanger
{
    union {
        eTriggerState triggerState;
        eDamageState damageState = DAMAGE_DEFAULT;
    };
    union {
        INT duration = 0;
        eSpell spellToCast;
    };
    eTriggerState GetNextTriggerState() const
    {
        if (triggerState >= eTriggerState::NEVER)
            return eTriggerState::DEFAULT;
        else
            return static_cast<eTriggerState>(triggerState + 1);
    }
    eDamageState GetNextDamageState() const
    {
        if (damageState >= eDamageState::DAMAGE_MIN_75)
            return eDamageState::DAMAGE_DEFAULT;
        else
            return static_cast<eDamageState>(damageState + 1);
    }
};
