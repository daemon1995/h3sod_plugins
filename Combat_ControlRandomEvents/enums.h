#pragma once

enum eTriggerState : unsigned int
{
    TRIGGER_STATE_DEFAULT = 0,
    TRIGGER_STATE_ALWAYS = 1,
    TRIGGER_STATE_NEVER = 2,
    AMOUNT_OF_TRIGGER_STATES
};
enum eDamageState : unsigned int
{
    DAMAGE_STATE_DEFAULT = 0,
    DAMAGE_STATE_RANDOM = DAMAGE_STATE_DEFAULT,
    DAMAGE_STATE_MINIMUM = 1,
    DAMAGE_STATE_MAXIMUM = 2,
    DAMAGE_STATE_MIN_25 = 3,
    DAMAGE_STATE_MIN_50 = 4,
    DAMAGE_STATE_MIN_75 = 5,
    AMOUNT_OF_DAMAGE_STATES
};

enum eSideSettingsId
{
    SIDE_SETTING_NONE = -1,
    SIDE_SETTING_UNAFFECTED_BY_MORALE,
    SIDE_SETTING_UNAFFECTED_BY_LUCK,
    SIDE_SETTING_UNAFFECTED_BY_FEAR, // target setting supresses side setting
    SIDE_SETTING_UNAFFECTED_BY_RESISTANCE,
    AMOUNT_OF_SIDE_SETTINGS
};

enum eStackSettingsId
{
    STACK_SETTING_NONE = -1,
    STACK_SETTING_POSITIVE_MORALE,
    STACK_SETTING_NEGATIVE_MORALE,
    STACK_SETTING_FEAR,
    STACK_SETTING_SPELL_CASTING,
    STACK_SETTING_RESURRECTION,
    STACK_SETTING_MAGIC_RESISTANCE,
    STACK_SETTING_MAGIC_MIRROR,
    // MASS_MAGIC_RESISTANCE,
    STACK_SETTING_POSITIVE_LUCK,
    STACK_SETTING_NEGATIVE_LUCK,
    STACK_SETTING_DOUBLE_DAMAGE,
    STACK_SETTING_WALL_ATTACK,
    STACK_SETTING_AFTER_ATTACK_ABILITY,
    STACK_SETTING_DAMAGE_VARIATION_FIRST,
    STACK_SETTING_DAMAGE_VARIATION_SECOND,
    AMOUNT_OF_STACK_SETTINGS
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
        eDamageState damageState = DAMAGE_STATE_DEFAULT;
    };
    union {
        INT duration;
        eSpell spellToCast = eSpell::NONE;
    };
    eTriggerState GetNextTriggerState() const
    {
        if (triggerState >= eTriggerState::TRIGGER_STATE_NEVER)
            return eTriggerState::TRIGGER_STATE_DEFAULT;
        else
            return static_cast<eTriggerState>(triggerState + 1);
    }
    eDamageState GetNextDamageState() const
    {
        if (damageState >= eDamageState::DAMAGE_STATE_MIN_75)
            return eDamageState::DAMAGE_STATE_DEFAULT;
        else
            return static_cast<eDamageState>(damageState + 1);
    }
};
