#include "framework.h"

CreatureSettingsManager *CreatureSettingsManager::instance = nullptr;
struct PluginText
{
    static PluginText &PluginText::GetInstance();
    static LPCSTR GetHintText(const eSettingsId settingId, const H3CombatCreature *creature,
                              const eAbilitySwitchError errorType);
};
struct SpellSelectionDlg
{
    static eSpell ShowSettingsDlg(H3CombatCreature *creature, const H3Msg *msg);
};

CreatureSettingsManager::CreatureSettingsManager() : IGamePatch(_PI)
{
    // Initialize all creature settings to default
    CreatePatches();
    ResetCombatSettings();
    pluginText = &PluginText::GetInstance();
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

void CreatureSettingsManager::SwitchBattleStackAbilityByHotKey(H3CombatManager *mgr, H3Msg *msg)
{
    if (msg->IsKeyPress() && mgr->mouseCoord >= 0 && mgr->mouseCoord <= 186)
    {
        const H3CombatCreature *combatCreature = mgr->squares[mgr->mouseCoord].GetMonster();
        if (!combatCreature || combatCreature->type == eCreature::ARROW_TOWER)
            return;

        CombatCreatureSettings *creatureSettings =
            &combatCreatureSettings[combatCreature->side][combatCreature->sideIndex];

        creatureSettings->ownerCreature = combatCreature;

        BOOL saveToLog = TRUE;
        BOOL affectedAllUnits = FALSE;
        eAbilitySwitchError errorType = ABILITY_SWITCH_NO_EFFECT;
        LPCSTR resultText = nullptr;
        eSettingsId settingId = eSettingsId::NONE;

        const bool shiftPressed = STDCALL_1(SHORT, PtrAt(0x63A294), VK_SHIFT) & 0x800;
        switch (msg->KeyPressed())
        {

        case eVKey::H3VK_G: // fear
            settingId = eSettingsId::FEAR;

            if (creatureSettings->IsAffected(eSettingsId::FEAR))
            {
                errorType = ABILITY_SWITCH_NO_ERROR;
            }
            break;

        case eVKey::H3VK_J: // resist
            settingId = eSettingsId::MAGIC_RESISTANCE;

            if (creatureSettings->IsAffected(eSettingsId::MAGIC_RESISTANCE))
            {
                errorType = ABILITY_SWITCH_NO_ERROR;
            }
            break;

        case eVKey::H3VK_K: // damage

            settingId = shiftPressed ? eSettingsId::DAMAGE_VARIATION_SECOND : eSettingsId::DAMAGE_VARIATION_FIRST;
            if (creatureSettings->IsAffected(settingId))
            {
                errorType = ABILITY_SWITCH_NO_ERROR;
            }
            break;
        case eVKey::H3VK_X: // after attack ability// spell casting// resurection// double damage (shift) // wall attack
                            // aim

            errorType = ABILITY_SWITCH_NO_ERROR;
            if (shiftPressed)
            {
                if (creatureSettings->IsAffected(eSettingsId::DOUBLE_DAMAGE))
                {
                    settingId = eSettingsId::DOUBLE_DAMAGE;
                }
                else if (creatureSettings->IsAffected(eSettingsId::SPELL_CASTING))
                {
                    SpellSelectionDlg::ShowSettingsDlg(const_cast<H3CombatCreature *>(combatCreature), msg);
                }
                //                    errorType = ABILITY_SWITCH_NO_ABILITY;
            }
            else
            {
                if (creatureSettings->IsAffected(eSettingsId::AFTER_ATTACK_ABILITY))
                {
                    settingId = eSettingsId::AFTER_ATTACK_ABILITY;
                }
                else if (creatureSettings->IsAffected(eSettingsId::SPELL_CASTING))
                {
                    settingId = eSettingsId::SPELL_CASTING;
                }
                else if (creatureSettings->IsAffected(eSettingsId::RESURRECTION))
                {
                    settingId = eSettingsId::RESURRECTION;
                }
                else if (creatureSettings->IsAffected(eSettingsId::WALL_ATTACK))
                {
                    settingId = eSettingsId::WALL_ATTACK;
                }
                else
                {
                    errorType = ABILITY_SWITCH_NO_ABILITY;
                }
            }

            break;
        case eVKey::H3VK_N: // luck (shift)

            settingId = eSettingsId::POSITIVE_LUCK_UNIT;
            errorType = ABILITY_SWITCH_NO_ERROR;

            if (shiftPressed)
            {
                for (const auto &stackSide : mgr->stacks)
                {
                    for (const auto &stack : stackSide)
                    {
                        if (creatureSettings->IsAffected(eSettingsId::POSITIVE_LUCK_UNIT, &stack))
                        {
                            affectedAllUnits = TRUE;
                            settingId = eSettingsId::POSITIVE_LUCK_ALL;
                            break;
                        }
                    }
                    // break outer loop if affected all units
                    if (affectedAllUnits)
                    {
                        break;
                    }
                }
                errorType = ABILITY_SWITCH_NO_EFFECT;
            }
            else if (creatureSettings->IsAffected(eSettingsId::POSITIVE_LUCK_UNIT) && combatCreature->luck > 0)
            {
                settingId = eSettingsId::POSITIVE_LUCK_UNIT;
            }
            else
            {
                errorType = ABILITY_SWITCH_NO_EFFECT;
            }
            break;

        case eVKey::H3VK_M: // morale (shift)
            errorType = ABILITY_SWITCH_NO_ERROR;
            settingId = eSettingsId::POSITIVE_MORALE_UNIT;

            if (shiftPressed)
            {

                for (const auto &stackSide : mgr->stacks)
                {
                    for (const auto &stack : stackSide)
                    {
                        if (creatureSettings->IsAffected(eSettingsId::POSITIVE_MORALE_UNIT, &stack))
                        {
                            affectedAllUnits = TRUE;
                            settingId = eSettingsId::POSITIVE_MORALE_ALL;
                            break;
                        }
                        if (creatureSettings->IsAffected(eSettingsId::NEGATIVE_MORALE_UNIT, &stack))
                        {
                            affectedAllUnits = TRUE;
                            settingId = eSettingsId::NEGATIVE_MORALE_ALL;
                            break;
                        }
                    }
                    // break outer loop if affected all units
                    if (affectedAllUnits)
                    {
                        break;
                    }
                }
                errorType = ABILITY_SWITCH_NO_EFFECT;
            }
            else
            {
                if (creatureSettings->IsAffected(eSettingsId::POSITIVE_MORALE_UNIT) && combatCreature->morale > 0)
                {
                    settingId = eSettingsId::POSITIVE_MORALE_UNIT;
                }
                else if (creatureSettings->IsAffected(eSettingsId::NEGATIVE_MORALE_UNIT) && combatCreature->morale < 0)
                {
                    settingId = eSettingsId::NEGATIVE_MORALE_UNIT;
                }
                else
                {
                    errorType = ABILITY_SWITCH_NO_EFFECT;
                }
            }

            break;
        default:
            creatureSettings = nullptr;
            return;
        }
        creatureSettings = nullptr;

        std::string resultHint = pluginText->GetHintText(settingId, combatCreature, errorType);
        if (!resultHint.empty())
        {
            ReportActionUsage(mgr, resultHint.c_str(), eLogType::LOG_TYPE_SCREEN);
        }
    }
}

void CreatureSettingsManager::ReportActionUsage(H3CombatManager *mgr, LPCSTR msg, const eLogType logType)
{

    switch (logType)
    {
    case eLogType::LOG_TYPE_SCREEN:
        H3ScreenChat::Get()->Show(msg);
        break;
    case eLogType::LOG_TYPE_BATTLE_LOG:
        mgr->AddStatusMessage(msg);
        return;
    case eLogType::LOG_TYPE_BATTLE_HINT:
        mgr->AddStatusMessage(msg, false);
        break;
    default:
        break;
    }
}

void CreatureSettingsManager::SaveActionUsageToLog(H3CombatManager *mgr, const CombatCreatureSettings *creatureSettings)
{
}

CreatureSettingsManager &CreatureSettingsManager::GetInstance()
{

    P_CombatManager->creatureAtMousePos;
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
    tempAttacker.abilities.firstAttackDamage.damageState = eDamageState::DAMAGE_MAXIMUM;
    tempAttacker.abilities.secondAttackDamage.damageState = eDamageState::DAMAGE_MINIMUM;
    tempAttacker.abilities.doubleDamage.triggerState = eTriggerState::ALWAYS;
    tempAttacker.abilities.positiveLuck.triggerState = eTriggerState::ALWAYS;
    tempAttacker.abilities.wallAttackAim.triggerState = eTriggerState::ALWAYS;

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

int __stdcall CreatureSettingsManager::BattleMgr_ProcessAction_KeyPressed(HiHook *h, H3CombatManager *_this, H3Msg *msg)
{
    int result = THISCALL_2(int, h->GetDefaultFunc(), _this, msg);

    if (_this->autoCombat || H3AutoSolo::Get() || _this->IsHiddenBattle() ||
        IntAt(0x698A3C)) // 698A3C is some action is in proc
    {
        return result;
    }

    instance->SwitchBattleStackAbilityByHotKey(_this, msg);

    return result;
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

void CreatureSettingsManager::SetAbilityForAllCreatures(const AbilityChanger &ability, const BOOL enable) noexcept
{
    for (size_t side = 0; side < 2; side++)
    {
        for (size_t i = 0; i <= h3::limits::COMBAT_CREATURES; i++)
        {
            CombatCreatureSettings &settings = instance->combatCreatureSettings[side][i];
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
        WriteHiHook(0x04746B0, THISCALL_, BattleMgr_ProcessAction_KeyPressed);

        newRoundPatch = WriteHiHook(0x0475800, THISCALL_, BattleMgr_NewRound);
        endCombatPatch = WriteHiHook(0x0475CFD, THISCALL_, BattleMgr_SetWinner);
    }
}
