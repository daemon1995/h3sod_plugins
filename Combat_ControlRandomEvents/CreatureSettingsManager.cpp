#include "framework.h"

CreatureSettingsManager *CreatureSettingsManager::instance = nullptr;

CreatureSettingsManager::CreatureSettingsManager() : IGamePatch(_PI)
{
    // Initialize all creature settings to default
    CreatePatches();
    ResetCombatSettings();
}

void CreatureSettingsManager::ResetCombatSettings() noexcept
{
    ResetAllcreatureSettings();

    combatIsStarted = false;
    tacticsPhaseRound = false;
    userControlPoints = 0;
    userControlPointsSpent = 0;
    userActionsUsed = 0;

    actionsUsedLog.Init();
}

void CreatureSettingsManager::ResetAllcreatureSettings() noexcept
{
    for (size_t side = 0; side < 2; side++)
    {
        for (size_t i = 0; i <= h3::limits::COMBAT_CREATURES; i++)
        {
            combatCreatureSettings[side][i] = CombatCreatureSettings();
        }
    }
}

CreatureSettingsManager &CreatureSettingsManager::GetInstance()
{
    if (!instance)
        instance = new CreatureSettingsManager();
    else
        instance = {};
    return *instance;
}

void TestInitiate(CreatureSettingsManager *instance)
{

    CombatCreatureSettings tempAttacker;
    tempAttacker.abilities.positiveMorale.triggerState = eTriggerState::ALWAYS;
    tempAttacker.abilities.afterAttackAbility.triggerState = eTriggerState::ALWAYS;
    tempAttacker.abilities.damage.damageState = eDamageState::DAMAGE_MIN_100;
    tempAttacker.abilities.doubleDamage.triggerState = eTriggerState::ALWAYS;
    tempAttacker.abilities.positiveLuck.triggerState = eTriggerState::ALWAYS;
    CombatCreatureSettings tempDefender;
    tempDefender.abilities.negativeMorale.triggerState = eTriggerState::ALWAYS;

    for (size_t i = 0; i <= h3::limits::COMBAT_CREATURES; i++)
    {
        instance->SetCreatureSettings(0, i, tempAttacker);
        instance->SetCreatureSettings(1, i, tempDefender);
    }
}

void __stdcall CreatureSettingsManager::BattleMgr_StartBattle(HiHook *h, H3CombatManager *_this)
{

    THISCALL_1(void, h->GetDefaultFunc(), _this);

    // check if combat has human player

    auto &moraleHandler = CreatureMoraleRandom::GetInstance();
    IGamePatch *appliedPatches[] = {
        &CreatureMoraleRandom::GetInstance(),
        &CreatureAttackRandom::GetInstance(),
    };

    auto &newRoundPatch = instance->newRoundPatch;
    auto &endCombatPatch = instance->endCombatPatch;

    const BOOL combatHasHuman = _this->isHuman[0] || _this->isHuman[1];

    // if not a human player, disable all patches
    if (!combatHasHuman)
    {
        for (auto &i : appliedPatches)
        {
            if (i->IsEnabled())
                i->SetEnabled(false);
        }

        if (newRoundPatch->IsApplied())
            newRoundPatch->Undo();

        if (endCombatPatch->IsApplied())
            endCombatPatch->Undo();
    }
    // else, enable all patches and apply new round patch
    else
    {
        for (auto &i : appliedPatches)
        {
            if (!i->IsEnabled())
                i->SetEnabled(true);
        }

        if (!newRoundPatch->IsApplied())
            newRoundPatch->Apply();
        if (!endCombatPatch->IsApplied())
            endCombatPatch->Apply();
    }

    instance->ResetCombatSettings();

    instance->combatIsStarted = false;
    instance->tacticsPhaseRound = _this->tacticsPhase;

    if (_this->tacticsPhase)
    {
        libc::sprintf(h3_TextBuffer, "Creature Settings Manager: New Round %d", _this->turn);
        H3Messagebox(h3_TextBuffer);
    }
    else
    {
        instance->combatIsStarted = true;
        TestInitiate(instance);
    }
}

// isn't executed if combat doesn't have tactics phase
void __stdcall CreatureSettingsManager::BattleMgr_NewRound(HiHook *h, H3CombatManager *_this)
{

    THISCALL_1(void, h->GetDefaultFunc(), _this);

    if (instance->combatIsStarted)
    {
        TestInitiate(instance);
        libc::sprintf(h3_TextBuffer, "Creature Settings Manager: New Round %d", _this->turn);
        _this->AddStatusMessage(h3_TextBuffer);
        //  H3Messagebox("instance->combatIsStarted");
        if (_this->turn + 1 - instance->tacticsPhaseRound == 2)
        {
        }
    }
    else if (!_this->tacticsPhase)
    {
        instance->combatIsStarted = true;
    }
}

void __stdcall CreatureSettingsManager::BattleMgr_SetWinner(HiHook *h, H3CombatManager *_this, const INT side)
{
    THISCALL_2(void, h->GetDefaultFunc(), _this, side);
    instance->ResetCombatSettings();
}

const CombatCreatureSettings &CreatureSettingsManager::GetCreatureSettings(const H3CombatCreature *creature) noexcept
{
    return instance->combatCreatureSettings[creature->side][creature->sideIndex];
}

const CombatCreatureSettings &CreatureSettingsManager::GetCreatureSettings(const int side, const int index) noexcept
{
    return instance->combatCreatureSettings[side][index];
}
void CreatureSettingsManager::SetCreatureSettings(const H3CombatCreature *creature,
                                                  const CombatCreatureSettings &settings) noexcept
{
    instance->combatCreatureSettings[creature->side][creature->sideIndex] = settings;
}

void CreatureSettingsManager::SetCreatureSettings(const int side, const int index,
                                                  const CombatCreatureSettings &settings) noexcept
{
    instance->combatCreatureSettings[side][index] = settings;
}

int CreatureSettingsManager::GetUserPoints() noexcept
{
    return instance->userControlPoints;
}

void CreatureSettingsManager::SetUserPoints(const int newSize) noexcept
{
    instance->userControlPoints = newSize;
}

BOOL CreatureSettingsManager::DecreaseUserPoints(const int toDecrease) noexcept
{

    if (instance->userControlPoints >= toDecrease)
    {
        instance->userControlPoints -= toDecrease;
        instance->userControlPointsSpent += toDecrease;
        return true;
    }

    return false;
}

void CreatureSettingsManager::SetAbilityForAllCreatures(const AbilityChanger& ability, const BOOL enable) noexcept
{
    for (size_t side = 0; side < 2; side++)
    {
        for (size_t i = 0; i <= h3::limits::COMBAT_CREATURES; i++)
        {
            CombatCreatureSettings& settings = instance->combatCreatureSettings[side][i];
           // ability(settings, enable);
        }
	}
}

// Implementation of patch creation
void CreatureSettingsManager::CreatePatches()
{
    if (!m_isInited)
    {
        m_isInited = true;
        m_isEnabled = true;
        WriteHiHook(0x0462C8A, THISCALL_, BattleMgr_StartBattle);

        newRoundPatch = WriteHiHook(0x0475800, THISCALL_, BattleMgr_NewRound);
        endCombatPatch = WriteHiHook(0x0475CFD, THISCALL_, BattleMgr_SetWinner);
    }
}
