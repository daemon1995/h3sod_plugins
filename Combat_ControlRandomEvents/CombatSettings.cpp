#include "framework.h"

struct CreatureAttackRandom
{
    static BOOL CreatureAttackRandom::BattleStack_HasAfterAttackAbility(const H3CombatCreature *creature);
};

CombatStackSettings CombatStackSettings::combatStackSettings[2][h3::limits::COMBAT_CREATURES + 1]{};
CombatSideSettings CombatSideSettings::sideSettings[2]{};

void CombatStackSettings::DecreaseDurations()
{

    for (auto &i : asArray)
    {
        if (i.duration > 0)
        {
            i.duration--;
        }

        if (i.duration == 0)
        {
            i.duration = -1;
            i.triggerState = TRIGGER_STATE_DEFAULT;
        }
    }
}

BOOL CombatStackSettings::IsAffectedBySetting(const eStackSettingsId id) const
{
    if (!creature)
        return FALSE;
    if (creature->numberAlive < 1 && creature->type == eCreature::ARROW_TOWER)
        return FALSE;

    const auto &info = creature->info;
    const int creatureType = creature->type;
    //    const AbilityChanger &ability = asArray[id];
    switch (id)
    {
    case STACK_SETTING_POSITIVE_MORALE:
        if (P_CombatManager->specialTerrain == 2)
        {
            return false;
        }
        else
        {
            for (auto &hero : P_CombatManager->hero)
            {
                if (hero && hero->WearsArtifact(eArtifact::SPIRIT_OF_OPPRESSION))
                {
                    return false;
                }
            }
        }
        return true;
    case STACK_SETTING_NEGATIVE_MORALE:
        return !info.noMorale; // || creature->info.undead;
    case STACK_SETTING_FEAR:
        return !(info.noMorale || creatureType == eCreature::AZURE_DRAGON); // || creature->info.undead;

    case STACK_SETTING_SPELL_CASTING:
        return creatureType == eCreature::ENCHANTER ||
               (creatureType == eCreature::MASTER_GENIE || creatureType == eCreature::FAERIE_DRAGON) &&
                   creature->info.spellCharges > 0;

    case STACK_SETTING_RESURRECTION:
        return creatureType == eCreature::PHOENIX && creature->info.spellCharges > 0;
    case STACK_SETTING_MAGIC_RESISTANCE:
        return false;
    case STACK_SETTING_POSITIVE_LUCK:
        if (P_CombatManager->specialTerrain == 2)
        {
            return false;
        }
        else
        {
            for (auto &hero : P_CombatManager->hero)
            {
                if (hero && hero->WearsArtifact(eArtifact::HOURGLASS_OF_THE_EVIL_HOUR))
                {
                    return false;
                }
            }
        }
        return true;
    case STACK_SETTING_DOUBLE_DAMAGE:
        return creatureType == eCreature::DREAD_KNIGHT;
    case STACK_SETTING_WALL_ATTACK:
        return info.destroyWalls && P_CombatManager->siegeKind2 > 0;
    case STACK_SETTING_AFTER_ATTACK_ABILITY:
        return CreatureAttackRandom::BattleStack_HasAfterAttackAbility(creature);
    case STACK_SETTING_DAMAGE_VARIATION_FIRST:
        return info.damageLow < info.damageHigh;
    case STACK_SETTING_DAMAGE_VARIATION_SECOND:
        return info.doubleAttack && info.damageLow < info.damageHigh && (!info.shooter || info.numberShots > 1);
    default:
        break;
    }
    //   return

    return false;
}

int CombatStackSettings::BattleStack_Random(HiHook *hook, const int min, const int max,
                                            const AbilityChanger &abilityChanger)
{
    // if ability isn't duration based, decrease points per use
    if (abilityChanger.duration == 0)
    {
    }

    // if (const int userPints = CombatSettingsManager::GetUserPoints())
    {
        switch (abilityChanger.triggerState)
        {
        case eTriggerState::TRIGGER_STATE_ALWAYS:
            return min; // always trigger ability
        case eTriggerState::TRIGGER_STATE_NEVER:
            return max; // never trigger ability
        default:
            break;
        }
    }

    return FASTCALL_2(int, hook->GetDefaultFunc(), min, max);
}

void CombatStackSettings::ResetAll()
{
    for (int side = 0; side < 2; side++)
    {
        for (int index = 0; index < h3::limits::COMBAT_CREATURES + 1; index++)
        {
            combatStackSettings[side][index].Reset();
        }
    }
}
BOOL CombatSideSettings::IsAffectedBySetting(const eSideSettingsId id) const
{
    float maxSideResistance = 0.f;
    switch (id)
    {
    case SIDE_SETTING_UNAFFECTED_BY_MORALE:
        if (P_CombatManager->specialTerrain == 2)
        {
            return false;
        }
        else
        {
            for (auto &hero : P_CombatManager->hero)
            {
                if (hero && hero->WearsArtifact(eArtifact::SPIRIT_OF_OPPRESSION))
                {
                    return false;
                }
            }

            for (auto &stack : P_CombatManager->stacks[side])
            {
                if (CombatStackSettings::GetCombatStackSettings(&stack).IsAffectedBySetting(
                        STACK_SETTING_POSITIVE_MORALE) ||
                    CombatStackSettings::GetCombatStackSettings(&stack).IsAffectedBySetting(
                        STACK_SETTING_NEGATIVE_MORALE))
                {
                    return true;
                }
            }
        }
        return !settings.unaffectedByMorale.triggerState;
    case SIDE_SETTING_UNAFFECTED_BY_LUCK:
        for (auto &stack : P_CombatManager->stacks[side])
        {
            if (CombatStackSettings::GetCombatStackSettings(&stack).IsAffectedBySetting(STACK_SETTING_POSITIVE_LUCK))
            {
                return true;
            }
        }
        break;

    case SIDE_SETTING_UNAFFECTED_BY_FEAR:
        for (auto &stack : P_CombatManager->stacks[side])
        {
            if (CombatStackSettings::GetCombatStackSettings(&stack).IsAffectedBySetting(STACK_SETTING_FEAR))
            {
                return true;
            }
        }
        return false;
    case SIDE_SETTING_UNAFFECTED_BY_RESISTANCE:
        maxSideResistance = GetSideMaxResistance();
        return maxSideResistance > 0.f && maxSideResistance < 1.f;
    default:
        break;
    }
    return false;
}
float CombatSideSettings::GetSideMaxResistance() const
{

    auto &stacks = P_CombatManager->stacks[side];

    float result = 0;

    const auto &hero = P_CombatManager->hero[side];

    float heroResistance = hero ? hero->GetResistancePower() : 0;
   
    
    for (auto &stack : stacks)
    {

        if (stack.numberAlive < 1 || stack.activeSpellDuration[eSpell::HYPNOTIZE])
            continue;

        float stackResistance = 0.f; // = stack.type.magicResistance;
        switch (stack.type)
        {
        case eCreature::DWARF:
        case eCreature::CRYSTAL_DRAGON:
            stackResistance += 0.8f;
            break;
        case eCreature::BATTLE_DWARF:
            stackResistance += 0.6f;
            break;
        default:
            break;
        }

        if (hero)
        {
            stackResistance = stackResistance - 1.0f + heroResistance;
        }
        if (stack.HasUnicornsAura())
        {
            stackResistance *= 0.8f;
        }
        if (result < stackResistance)
        {
            result = stackResistance;
        }
    }
    return result;
}
void CombatSideSettings::ResetAll()
{
    for (int side = 0; side < 2; side++)
    {
        sideSettings[side] = {};
        sideSettings[side].side = side;
    }
}
