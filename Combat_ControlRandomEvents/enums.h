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
