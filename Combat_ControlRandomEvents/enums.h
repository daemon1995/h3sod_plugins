#pragma once

enum eTriggerState : unsigned int
{
    DEFAULT = 0,
    ALWAYS = 1,
    NEVER = 2,
};
enum eDamageState : unsigned int
{
    DAMAGE_DEFAULT = 0,
    DAMAGE_RANDOM = DAMAGE_DEFAULT,
    DAMAGE_MINIMAL = 1,
    DAMAGE_MAXIMAL = 2,
    DAMAGE_MIN_25 = 3,
    DAMAGE_MIN_50 = 4,
    DAMAGE_MIN_75 = 5,
    DAMAGE_MIN_100 = DAMAGE_MAXIMAL,
};

enum eSettingsId
{
    POSITIVE_MORALE,
    NEGATIVE_MORALE,
    FEAR,
    SPELL_CASTING,
    RESURRECTION,
    RESISTANCE,
    POSITIVE_LUCK,
    NEGATIVE_LUCK,
    DOUBLE_DAMAGE,
    WALL_ATTACK_AIM,
    AFTER_ATTACK_ABILITY,
    DAMAGE,
    AMOUNT_OF_SETTINGS
};
struct AbilityChanger
{
    union {
        eTriggerState triggerState;
        eDamageState damageState = DAMAGE_DEFAULT;
    };
    INT duration = 0;
};
