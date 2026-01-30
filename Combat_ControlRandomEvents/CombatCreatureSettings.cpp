#include "framework.h"

CombatCreatureSettings::CombatCreatureSettings()
{
}

void CombatCreatureSettings::ResetSettings()
{
    libc::memset(this, 0, sizeof(CombatCreatureSettings));
}

void CombatCreatureSettings::DecreaseDurations()
{

    for (auto &i : asArray)
    {
        if (i.duration > 0)
        {
            i.duration--;
        }
        if (i.duration == 0)
        {
            i.triggerState = DEFAULT;
        }
    }
}

const AbilityChanger &CombatCreatureSettings::At(const eSettingsId id) const
{
    return asArray[id];
}

AbilityChanger &CombatCreatureSettings::operator[](const eSettingsId id)
{
    return asArray[id];
}

BOOL CombatCreatureSettings::IsAffected(const eSettingsId id, const H3CombatCreature *creature) const
{
    if (!creature || ownerCreature)
        creature = ownerCreature;

    if (!creature)
        return FALSE;
    if (creature->numberAlive < 1 && creature->type == eCreature::ARROW_TOWER)
        return FALSE;

    const auto &info = creature->info;
    const int creatureType = creature->type;
    //    const AbilityChanger &ability = asArray[id];
    switch (id)
    {
    case POSITIVE_MORALE_UNIT:
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
    case NEGATIVE_MORALE_UNIT:
        return !info.noMorale; // || creature->info.undead;
    case FEAR:
        return !(info.noMorale || creatureType == eCreature::AZURE_DRAGON); // || creature->info.undead;

    case SPELL_CASTING:
        return creatureType == eCreature::ENCHANTER ||
               (creatureType == eCreature::MASTER_GENIE || creatureType == eCreature::FAERIE_DRAGON) &&
                   creature->info.spellCharges > 0;

    case RESURRECTION:
        return creatureType == eCreature::PHOENIX;
    case MAGIC_RESISTANCE:
        return false;
    case POSITIVE_LUCK_UNIT:
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
    case DOUBLE_DAMAGE:
        return creatureType == eCreature::DREAD_KNIGHT;
    case WALL_ATTACK:
        return info.destroyWalls;
    case AFTER_ATTACK_ABILITY:
        return CreatureAttackRandom::BattleStack_HasAfterAttackAbility(creature);
    case DAMAGE_VARIATION_FIRST:
        return info.damageLow < info.damageHigh;
    case DAMAGE_VARIATION_SECOND:
        return info.doubleAttack && info.damageLow < info.damageHigh && (!info.shooter || info.numberShots > 1);
    default:
        break;
    }
    //   return

    return false;
}

int CombatCreatureSettings::BattleStack_Random(HiHook *hook, const int min, const int max,
                                               const AbilityChanger &abilityChanger)
{
    // if ability isn't duration based, decrease points per use
    if (abilityChanger.duration == 0)
    {
    }

    // if (const int userPints = CreatureSettingsManager::GetUserPoints())
    {
        switch (abilityChanger.triggerState)
        {
        case eTriggerState::ALWAYS:
            return min; // always trigger ability
        case eTriggerState::NEVER:
            return max; // never trigger ability
        default:
            break;
        }
    }

    return FASTCALL_2(int, hook->GetDefaultFunc(), min, max);
}
