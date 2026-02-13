#include "framework.h"

// Forward declarations
struct CreatureAttackRandom
{
    static BOOL CreatureAttackRandom::BattleStack_HasAfterAttackAbility(const H3CombatCreature *creature);
};
struct CreatureSpellData
{
  public:
    static BOOL CreateAvailableSpellsList(const H3CombatCreature *creature, std::vector<eSpell> &outList);
};

// Static member definitions
CombatStackSettings CombatStackSettings::combatStackSettings[2][h3::limits::COMBAT_CREATURES + 1]{};
CombatSideSettings CombatSideSettings::sideSettings[2]{};

// Method implementations

BOOL CombatStackSettings::IsAffectedBySetting(const eStackAbility id) const
{
    if (!creature)
        return FALSE;
    if (creature->numberAlive < 1 && creature->type == eCreature::ARROW_TOWER)
        return FALSE;

    const auto &info = creature->info;
    const int creatureType = creature->type;
    const auto &owner = creature->GetOwner();
    //    const Ability &ability = asArray[id];
    switch (id)
    {
    case STACK_SETTING_POSITIVE_MORALE:

        for (auto &hero : P_CombatManager->hero)
        {
            if (hero && hero->WearsArtifact(eArtifact::SPIRIT_OF_OPPRESSION))
            {
                return false;
            }
        }
        return !(P_CombatManager->specialTerrain == 2 || info.noMorale);
    case STACK_SETTING_NEGATIVE_MORALE:
        return !info.noMorale; // || creature->info.undead;
    case STACK_SETTING_FEAR:
        return !(info.noMorale || creatureType == eCreature::AZURE_DRAGON); // || creature->info.undead;
    case STACK_SETTING_SPELL_CASTING:
        return creatureType == eCreature::ENCHANTER ||
               (creatureType == eCreature::MASTER_GENIE || creatureType == eCreature::FAERIE_DRAGON) &&
                   creature->info.spellCharges > 0;

    case STACK_SETTING_RESURRECTION:
        return creatureType == eCreature::PHOENIX && creature->info.spellCharges > 0 && creature->numberAtStart % 5;
    case STACK_SETTING_MAGIC_RESISTANCE:
        switch (creatureType)
        {
        case eCreature::DWARF:
        case eCreature::CRYSTAL_DRAGON:
        case eCreature::BATTLE_DWARF:
            return true;
        default:
            return creature->HasUnicornsAura() || (owner && owner->GetResistancePower());
        }
        break;
    case STACK_SETTING_MAGIC_MIRROR:
        return creature->MagicMirrorEffect();
    case STACK_SETTING_POSITIVE_LUCK:
        for (auto &hero : P_CombatManager->hero)
        {
            if (hero && hero->WearsArtifact(eArtifact::HOURGLASS_OF_THE_EVIL_HOUR))
            {
                return false;
            }
        }
        return P_CombatManager->specialTerrain != 2;

    case STACK_SETTING_DOUBLE_LUCK:
        return info.doubleAttack && IsAffectedBySetting(STACK_SETTING_POSITIVE_LUCK);
    case STACK_SETTING_DOUBLE_DAMAGE:
        return creatureType == eCreature::DREAD_KNIGHT ||
               (creatureType == eCreature::BALLISTA && owner &&
                owner->secSkill[eSecondary::ARTILLERY]); // ballista's double damage
    case STACK_SETTING_WALL_ATTACK_AIM:
        return info.destroyWalls && P_CombatManager->siegeKind2 > 0 && creature->side == 0 && info.numberShots > 0 &&
               (creatureType != eCreature::CATAPULT ||
                owner && owner->secSkill[eSecondary::BALLISTICS] > eSecSkillLevel::NONE); // attker side
    case STACK_SETTING_WALL_ATTACK_NO_DAMAGE:
        return creatureType == eCreature::CATAPULT && info.numberShots > 0 &&
               (!owner || owner->secSkill[eSecondary::BALLISTICS] ==
                              eSecSkillLevel::NONE); // info.destroyWalls && P_CombatManager->siegeKind2 > 0 &&
                                                     // creature->side == 0;
                                                     // // attker side
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

eAbilityStateSwitchError CombatStackSettings::SwitchToNextAbilityState(const eStackAbility id, Ability &result) const
{

    result = asArray[id]; // { currentState, asArray[id].duration };

    const eTriggerState currentState = result.triggerState;

    switch (id)
    {
    case STACK_SETTING_POSITIVE_MORALE:
    case STACK_SETTING_NEGATIVE_MORALE:
    case STACK_SETTING_FEAR:
    case STACK_SETTING_POSITIVE_LUCK:
        if (result.isTriggered)
        {
            return ABILITY_SWITCH_SWITCH_BLOCKED; // already triggered, can't switch
        }
        if (currentState >= TRIGGER_STATE_NEVER)
        {
            result.triggerState = TRIGGER_STATE_DEFAULT;
            result.cost = 0;
            result.duration = -1;
        }
        else
        {
            result.triggerState = static_cast<eTriggerState>(currentState + 1);
            result.cost = 1;
            result.duration = CREATURE_ABILITY_TURNS_DURATION;
        }
        break;
    case STACK_SETTING_SPELL_CASTING:
        result = GetNextSpellStateToCast();
        break;
    case STACK_SETTING_RESURRECTION:
        result = GetNextResurrectionState();
        break;
        // bool on/off settings
    case STACK_SETTING_DOUBLE_LUCK:
    case STACK_SETTING_DAMAGE_INPUT:
        if (currentState)
        {
            result.triggerState = TRIGGER_STATE_DEFAULT;
            result.cost = 0;
        }
        else
        {
            result.triggerState = TRIGGER_STATE_ENABLED;
            result.cost = 2;
        }
        break;
    case STACK_SETTING_WALL_ATTACK_AIM:
    case STACK_SETTING_WALL_ATTACK_NO_DAMAGE:
        if (currentState)
        {
            result.triggerState = TRIGGER_STATE_DEFAULT;
            result.cost = 0;
        }
        else
        {
            result.triggerState = TRIGGER_STATE_ENABLED;
            result.cost = 1;
        }
        break;
        // settings with 3 states (default, always, never)
    default:
        if (currentState >= TRIGGER_STATE_NEVER)
        {
            result.triggerState = TRIGGER_STATE_DEFAULT;
            result.cost = 0;
        }
        else
        {
            result.triggerState = static_cast<eTriggerState>(currentState + 1);
            result.cost = 1;
        }
        break;
    }

    return ABILITY_SWITCH_SUCCESS;
}

Ability CombatStackSettings::GetNextSpellStateToCast() const noexcept
{

    Ability returnResult{};
    if (IsAffectedBySetting(STACK_SETTING_SPELL_CASTING))
    {
        std::vector<eSpell> availableSpells;
        if (CreatureSpellData::CreateAvailableSpellsList(creature, availableSpells))
        {

            if (spellCasting.spellToCast == eSpell::NONE)
            {
                returnResult.triggerState = TRIGGER_STATE_ENABLED;
                returnResult.spellToCast = availableSpells.front();
                returnResult.cost = 1;
                return returnResult;
            }

            if (spellCasting.spellToCast == availableSpells.back())
            {
                return returnResult;
            }

            const size_t length = availableSpells.size();
            for (size_t i = 0; i < length; i++)
            {
                if (this->spellCasting.spellToCast == availableSpells[i])
                {

                    returnResult.triggerState = TRIGGER_STATE_ENABLED;
                    returnResult.spellToCast = availableSpells[i + 1];
                    returnResult.cost = 1;
                    return returnResult;
                }
            }
        }
    }

    return returnResult;
}

Ability CombatStackSettings::GetNextResurrectionState() const noexcept
{
    const int remainder = creature->numberAtStart % 5;

    Ability result{};

    if (remainder == 0)
    {
        return result;
    }

    switch (resurrection.resurrectionState)
    {

    case RESURRECTION_STATE_DEFAULT:
        result.resurrectionState = RESURRECTION_STATE_0_FROM_ANY;
        result.cost = 1;
        break;

    case RESURRECTION_STATE_0_FROM_ANY:
        result.resurrectionState = RESURRECTION_STATE_1_FROM_ANY;
        result.cost = 1;
        break;

    case RESURRECTION_STATE_1_FROM_ANY:
        switch (remainder)
        {
        case 2:
            result.resurrectionState = RESURRECTION_STATE_2_FROM_2;
            result.cost = 2;
            break;
        case 3:
        case 4:
            result.resurrectionState = RESURRECTION_STATE_3_FROM_3_OR_4;
            result.cost = 2;
            break;
        default:
            break;
        }
        break;
    case RESURRECTION_STATE_3_FROM_3_OR_4:
        if (remainder == 4)
        {
            result.resurrectionState = RESURRECTION_STATE_4_FROM_4;
            result.cost = 3;
        }
    case RESURRECTION_STATE_2_FROM_2:
    case RESURRECTION_STATE_4_FROM_4:

        // result; no changes cause leads to default behaviour
        break;
    default:
        break;
    }

    return result;
}

BOOL CombatStackSettings::TriggerAbility(const eStackAbility id)
{

    Ability &abilityState = asArray[id];
    if (abilityState.triggerState == TRIGGER_STATE_DISABLED)
        return FALSE; // ability is not active, nothing to trigger

    int pointsToDecrease = 0; // abilityState.cost;
    switch (id)
    {

    case STACK_SETTING_POSITIVE_MORALE:
    case STACK_SETTING_NEGATIVE_MORALE:
    case STACK_SETTING_FEAR:
    case STACK_SETTING_POSITIVE_LUCK:

        if (!abilityState.isTriggered)
        {
            abilityState.isTriggered = true;
            pointsToDecrease = abilityState.cost;
        }
        break;

    case STACK_SETTING_SPELL_CASTING:
        pointsToDecrease = abilityState.cost;
        abilityState.triggerState = TRIGGER_STATE_DISABLED;
        abilityState.cost = 0;
        break;
    default:
        pointsToDecrease = abilityState.cost;
        abilityState = {}; // reset used data
        break;
    }
    if (pointsToDecrease)
    {
        CombatSettingsManager::DecreaseUserPoints(pointsToDecrease);
        CombatSettingsManager::ReportActionUsage(this, nullptr, id, pointsToDecrease);
    }

    return TRUE;
}

int CombatStackSettings::BattleStack_Random(HiHook *hook, const int min, const int max, const Ability &abilityChanger)
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

    return FASTCALL_2(int, hook->GetDefaultFunc(), min, max);
}

int CombatStackSettings::BattleStack_ContinuousRandom(const H3CombatCreature *combatCreature,
                                                      const eStackAbility stackSettingId,
                                                      const eSideAbility sideSettingId, HiHook *hook, const int min,
                                                      const int max)
{

    auto &stackSettings = CombatStackSettings::GetCombatStackSettings(combatCreature);
    Ability settings = stackSettings.At(stackSettingId);
    if (settings.triggerState == TRIGGER_STATE_DISABLED)
    {
        const int side = combatCreature->side;

        auto &sideSettings = CombatSideSettings::GetCombatSideSettings(side);
        auto &sideSetting = sideSettings.At(sideSettingId);
        if (sideSetting.triggerState == TRIGGER_STATE_ENABLED)
        {
            settings = sideSetting;
            settings.triggerState = TRIGGER_STATE_NEVER; // if side is unaffected by, then stack is also
            // unaffected by, so set it to never
            sideSettings.TriggerAbility(sideSettingId);
        }
    }
    else
    {
        stackSettings.TriggerAbility(stackSettingId);
    }

    return CombatStackSettings::BattleStack_Random(hook, min, max, settings);
}

void CombatStackSettings::HandleNewCombatRound()
{
    for (int side = 0; side < 2; side++)
    {
        for (auto &stackSettings : combatStackSettings[side])
        {
            for (auto &setting : stackSettings.asArray)
            {
                setting.DecreaseTriggeredDuration();
            }
        }
    }
}
void CombatStackSettings::ResetAll()
{
    for (int side = 0; side < 2; side++)
    {
        for (int index = 0; index < h3::limits::COMBAT_CREATURES + 1; index++)
        {
            combatStackSettings[side][index].Reset();
            if (P_CombatManager)
            {
                combatStackSettings[side][index].creature = &P_CombatManager->stacks[side][index];
            }
        }
    }
}
BOOL CombatSideSettings::IsAffectedBySetting(const eSideAbility id) const
{
    float maxSideResistance = 0.f;
    switch (id)
    {
    case SIDE_SETTING_UNAFFECTED_BY_MORALE:
        for (auto &stack : P_CombatManager->stacks[side])
        {
            if (CombatStackSettings::GetCombatStackSettings(&stack).IsAffectedBySetting(
                    STACK_SETTING_POSITIVE_MORALE) ||
                CombatStackSettings::GetCombatStackSettings(&stack).IsAffectedBySetting(STACK_SETTING_NEGATIVE_MORALE))
            {
                return true;
            }
        }
        break;
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
        break;
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

    float result = 1.f;

    const auto &hero = P_CombatManager->hero[side];

    float heroResistance = hero ? hero->GetResistancePower() : 0.f;
    for (auto &stack : stacks)
    {

        if (stack.numberAlive < 1 || stack.activeSpellDuration[eSpell::HYPNOTIZE])
            continue;

        float spellSuccess = 1.f; // = stack.type.magicResistance;
        switch (stack.type)
        {
        case eCreature::DWARF:
        case eCreature::CRYSTAL_DRAGON:
            spellSuccess = 0.8f;
            break;
        case eCreature::BATTLE_DWARF:
            spellSuccess = 0.6f;
            break;
        default:
            break;
        }

        if (hero)
        {
            spellSuccess = spellSuccess - (1.f - heroResistance);
        }
        if (stack.HasUnicornsAura())
        {
            spellSuccess *= 0.8f;
        }
        if (result > spellSuccess)
        {
            result = spellSuccess;
        }
    }
    return 1.f - result;
}

void CombatSideSettings::HandleNewCombatRound()
{
    for (int side = 0; side < 2; side++)
    {
        for (auto &setting : sideSettings[side].asArray)
        {
            setting.DecreaseTriggeredDuration();
        }
        // reset resistance breach at the start of each round
        sideSettings[side].unaffectedByResistance = {};
    }
}

eAbilityStateSwitchError CombatSideSettings::SwitchToNextAbilityState(const eSideAbility id, Ability &returnValue) const
{
    returnValue = asArray[id];
    if (returnValue.isTriggered)
    {
        return ABILITY_SWITCH_SWITCH_BLOCKED;
    }

    const auto currentState = returnValue.triggerState;
    if (returnValue.triggerState == TRIGGER_STATE_DISABLED)
    {
        returnValue.triggerState = TRIGGER_STATE_ENABLED;
        returnValue.duration = SIDE_ABILITY_TURNS_DURATION;
        returnValue.cost = 1;
    }
    else
    {
        returnValue = {};
    }
    return ABILITY_SWITCH_SUCCESS;
}
BOOL CombatSideSettings::TriggerAbility(const eSideAbility id)
{

    Ability &abilityState = asArray[id];
    if (abilityState.triggerState == TRIGGER_STATE_DISABLED)
        return FALSE;

    int pointsToDecrease = 0; // abilityState.cost;
    if (id == SIDE_SETTING_UNAFFECTED_BY_RESISTANCE)
    {
        pointsToDecrease = abilityState.cost;
        abilityState = {};
    }
    else if (!abilityState.isTriggered)
    {
        abilityState.isTriggered = true;
        pointsToDecrease = abilityState.cost;
    }
    if (pointsToDecrease)
    {
        CombatSettingsManager::DecreaseUserPoints(pointsToDecrease);
        CombatSettingsManager::ReportActionUsage(nullptr, this, id, pointsToDecrease);
    }

    return TRUE;
}

void CombatSideSettings::ResistanceBreachingTriggered(const int side, const int maxStackResistance)
{
    if (maxStackResistance < 1 || maxStackResistance > 80)
        return;

    auto &setting = sideSettings[side].unaffectedByResistance;
    const int pointsToDecrease = maxStackResistance / 20 + 1; // decrease 1 point per 20% of resistance
    setting.cost = pointsToDecrease;
    sideSettings[side].TriggerAbility(SIDE_SETTING_UNAFFECTED_BY_RESISTANCE);
}
void CombatSideSettings::ResetAll()
{
    for (int side = 0; side < 2; side++)
    {
        sideSettings[side] = {};
        sideSettings[side].side = side;
    }
}
