#include "framework.h"

class CreatureTurnControlRandom : public IGamePatch
{
  public:
    static CreatureTurnControlRandom &GetInstance();
};
class CreatureAttackRandom : public IGamePatch
{
  public:
    static CreatureAttackRandom &GetInstance();
};

CombatSettingsManager *CombatSettingsManager::instance = nullptr;
struct PluginText
{
    static PluginText &PluginText::GetInstance();
    LPCSTR GetHintText(const H3CombatCreature *creature, const eStackSettingsId settingId,
                       const AbilityChanger &changer, const eAbilitySwitchError errorType) const noexcept;
};
struct SpellSelectionDlg
{
    static eSpell ShowSettingsDlg(H3CombatCreature *creature, const H3Msg *msg);
};

CombatSettingsManager::CombatSettingsManager() : IGamePatch(_PI)
{
    // Initialize all creature settings to default
    CreatePatches();
    ResetCombatSettings();
    pluginText = &PluginText::GetInstance();
}

void CombatSettingsManager::ResetCombatSettings() noexcept
{

    CombatStackSettings::ResetAll();
    CombatSideSettings::ResetAll();

    combatIsStarted = false;
    tacticsPhaseRound = false;
    userControlPoints = userMaxControlPoints;
    userControlPointsSpent = 0;
    userActionsUsed = 0;

    actionsUsedLog.Init();
}

void CombatSettingsManager::SwitchBattleStackAbilityByHotKey(H3CombatManager *mgr, H3Msg *msg)
{
    if (msg->IsKeyPress() && mgr->mouseCoord >= 0 && mgr->mouseCoord <= 186)
    {
        const H3CombatCreature *combatCreature = mgr->squares[mgr->mouseCoord].GetMonster();
        if (!combatCreature || combatCreature->type == eCreature::ARROW_TOWER)
            return;

        const CombatStackSettings *combatStackSettings = &CombatStackSettings::GetCombatStackSettings(combatCreature);
        const CombatSideSettings *combatSideSettings = &CombatSideSettings::GetCombatSideSettings(combatCreature);

        BOOL saveToLog = TRUE;
        BOOL affectedAllUnits = FALSE;
        eAbilitySwitchError errorType = ABILITY_SWITCH_NO_EFFECT;
        LPCSTR resultText = nullptr;
        eStackSettingsId stackSettingId = STACK_SETTING_NONE;
        eSideSettingsId sideSettingId = SIDE_SETTING_NONE;

        const bool shiftPressed = STDCALL_1(SHORT, PtrAt(0x63A294), VK_SHIFT) & 0x800;
        const bool crtlPressed = STDCALL_1(SHORT, PtrAt(0x63A294), VK_CONTROL) & 0x800;

        float resultValue = 0.0f;
        std::string debugString;

        switch (msg->KeyPressed())
        {
        case eVKey::H3VK_B:
            if (shiftPressed)
            {
                resultValue = combatSideSettings->GetSideMaxResistance();
                libc::sprintf(h3_TextBuffer, "Max Resistance: %.2f", resultValue);

                ReportActionUsage(mgr, h3_TextBuffer, eLogType::LOG_TYPE_SCREEN);
            }

            return;

        case eVKey::H3VK_G: // fear
            stackSettingId = STACK_SETTING_FEAR;

            if (combatStackSettings->IsAffectedBySetting(STACK_SETTING_FEAR))
            {
                errorType = ABILITY_SWITCH_NO_ERROR;
            }
            break;

        case eVKey::H3VK_J: // resist
            stackSettingId = STACK_SETTING_MAGIC_RESISTANCE;

            if (shiftPressed)
            {
                sideSettingId = SIDE_SETTING_UNAFFECTED_BY_RESISTANCE;
            }
            else
            {
                if (combatStackSettings->IsAffectedBySetting(STACK_SETTING_MAGIC_RESISTANCE))
                    errorType = ABILITY_SWITCH_NO_ERROR;
            }
            //            else
            break;

        case eVKey::H3VK_K: // damage

            stackSettingId =
                shiftPressed ? STACK_SETTING_DAMAGE_VARIATION_SECOND : STACK_SETTING_DAMAGE_VARIATION_FIRST;
            if (combatStackSettings->IsAffectedBySetting(stackSettingId))
            {
                errorType = ABILITY_SWITCH_NO_ERROR;
            }
            break;
        case eVKey::H3VK_X: // after attack ability// spell casting// resurection// double damage (shift) // wall attack
                            // aim

            errorType = ABILITY_SWITCH_NO_ERROR;
            if (shiftPressed)
            {

                if (combatStackSettings->IsAffectedBySetting(STACK_SETTING_DOUBLE_DAMAGE))
                {
                    stackSettingId = STACK_SETTING_DOUBLE_DAMAGE;
                }
                else if (combatStackSettings->IsAffectedBySetting(STACK_SETTING_SPELL_CASTING))
                {
                    const eSpell selectedSpell =
                        SpellSelectionDlg::ShowSettingsDlg(const_cast<H3CombatCreature *>(combatCreature), msg);

                    if (selectedSpell != eSpell::NONE)
                    {
                        combatStackSettings->SetCreatureAbilityState(combatCreature, STACK_SETTING_SPELL_CASTING,
                                                                     {TRIGGER_STATE_ALWAYS, selectedSpell});
                        if (combatCreature->type == eCreature::FAERIE_DRAGON)
                        {
                            const_cast<H3CombatCreature *>(combatCreature)->faerieDragonSpell = selectedSpell;
                        }
                    }
                }
                //                    errorType = ABILITY_SWITCH_NO_ABILITY;
            }
            else
            {
                if (combatStackSettings->IsAffectedBySetting(STACK_SETTING_AFTER_ATTACK_ABILITY))
                {
                    stackSettingId = STACK_SETTING_AFTER_ATTACK_ABILITY;
                }
                else if (combatStackSettings->IsAffectedBySetting(STACK_SETTING_SPELL_CASTING))
                {
                    stackSettingId = STACK_SETTING_SPELL_CASTING;
                }
                else if (combatStackSettings->IsAffectedBySetting(STACK_SETTING_RESURRECTION))
                {
                    stackSettingId = STACK_SETTING_RESURRECTION;
                }
                else if (combatStackSettings->IsAffectedBySetting(STACK_SETTING_WALL_ATTACK))
                {
                    stackSettingId = STACK_SETTING_WALL_ATTACK;
                }
                else
                {
                    errorType = ABILITY_SWITCH_NO_ABILITY;
                }
            }

            break;
        case eVKey::H3VK_N: // luck (shift)

            stackSettingId = STACK_SETTING_POSITIVE_LUCK;
            errorType = ABILITY_SWITCH_NO_ERROR;

            if (shiftPressed)
            {

                sideSettingId = SIDE_SETTING_UNAFFECTED_BY_LUCK;
                if (combatSideSettings->IsAffectedBySetting(sideSettingId))
                {
                }

                errorType = ABILITY_SWITCH_NO_EFFECT;
            }
            else if (combatStackSettings->IsAffectedBySetting(STACK_SETTING_POSITIVE_LUCK) && combatCreature->luck > 0)
            {
                stackSettingId = STACK_SETTING_POSITIVE_LUCK;
            }
            else
            {
                errorType = ABILITY_SWITCH_NO_EFFECT;
            }
            break;
        case eVKey::H3VK_M: // morale (shift)
            errorType = ABILITY_SWITCH_NO_ERROR;
            stackSettingId = STACK_SETTING_POSITIVE_MORALE;

            if (shiftPressed)
            {

                combatSideSettings->IsAffectedBySetting(SIDE_SETTING_UNAFFECTED_BY_MORALE);

                // for (const auto &stackSide : mgr->stacks)
                //{
                //     for (const auto &stack : stackSide)
                //     {
                //         if (combatStackSettings->IsAffectedBySetting(eStackSettingsId::STACK_SETTING_POSITIVE_MORALE,
                //         &stack))
                //         {
                //             affectedAllUnits = TRUE;
                //             stackSettingId = eStackSettingsId::POSITIVE_MORALE_ALL;
                //             break;
                //         }
                //         if (combatStackSettings->IsAffectedBySetting(eStackSettingsId::STACK_SETTING_NEGATIVE_MORALE,
                //         &stack))
                //         {
                //             affectedAllUnits = TRUE;
                //             stackSettingId = eStackSettingsId::NEGATIVE_MORALE_ALL;
                //             break;
                //         }
                //     }
                //     // break outer loop if affected all units
                //     if (affectedAllUnits)
                //     {
                //         break;
                //     }
                // }
                errorType = ABILITY_SWITCH_NO_EFFECT;
            }
            else
            {
                if (combatStackSettings->IsAffectedBySetting(eStackSettingsId::STACK_SETTING_POSITIVE_MORALE) &&
                    combatCreature->morale > 0)
                {
                    stackSettingId = STACK_SETTING_POSITIVE_MORALE;
                }
                else if (combatStackSettings->IsAffectedBySetting(STACK_SETTING_NEGATIVE_MORALE) &&
                         combatCreature->morale < 0)
                {
                    stackSettingId = eStackSettingsId::STACK_SETTING_NEGATIVE_MORALE;
                }
                else
                {
                    errorType = ABILITY_SWITCH_NO_EFFECT;
                }
            }

            break;
        default:
            return;
        }

        // copy current setting to modify
        AbilityChanger changer = combatStackSettings->At(stackSettingId);
        if (errorType == ABILITY_SWITCH_NO_ERROR)
        {
            switch (stackSettingId)
            {
            case STACK_SETTING_DAMAGE_VARIATION_FIRST:
            case STACK_SETTING_DAMAGE_VARIATION_SECOND:
                changer.damageState = changer.GetNextDamageState();
                break;

            case STACK_SETTING_SPELL_CASTING:
                // handled in dialog
                break;
            default:
                changer.triggerState = changer.GetNextTriggerState();
                break;
            }
        }

        std::string resultHint = pluginText->GetHintText(combatCreature, stackSettingId, changer, errorType);
        if (!resultHint.empty())
        {
            CombatStackSettings::SetCreatureAbilityState(combatCreature, stackSettingId, changer);
            ReportActionUsage(mgr, resultHint.c_str(), eLogType::LOG_TYPE_SCREEN);
        }
    }
}

void CombatSettingsManager::ReportActionUsage(H3CombatManager *mgr, LPCSTR msg, const eLogType logType)
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

void CombatSettingsManager::SaveActionUsageToLog(H3CombatManager *mgr, const CombatStackSettings *creatureSettings)
{
}

CombatSettingsManager &CombatSettingsManager::GetInstance()
{

    P_CombatManager->creatureAtMousePos;
    if (!instance)
        instance = new CombatSettingsManager();
    else
        instance = {};
    return *instance;
}

void TestInitiate(CombatSettingsManager *instance)
{
    CombatStackSettings tempAttacker;
    tempAttacker.settings.positiveMorale.triggerState = TRIGGER_STATE_ALWAYS;
    tempAttacker.settings.afterAttackAbility.triggerState = TRIGGER_STATE_ALWAYS;
    tempAttacker.settings.firstAttackDamage.damageState = DAMAGE_STATE_MAXIMUM;
    tempAttacker.settings.secondAttackDamage.damageState = DAMAGE_STATE_MINIMUM;
    tempAttacker.settings.doubleDamage.triggerState = TRIGGER_STATE_ALWAYS;
    tempAttacker.settings.positiveLuck.triggerState = TRIGGER_STATE_ALWAYS;
    tempAttacker.settings.wallAttackAim.triggerState = TRIGGER_STATE_ALWAYS;
    const AbilityChanger tempAttackerAbility = {
        TRIGGER_STATE_ALWAYS,
    };
    CombatStackSettings tempDefender;
    tempDefender.settings.negativeMorale.triggerState = TRIGGER_STATE_ALWAYS;

    for (size_t i = 0; i <= h3::limits::COMBAT_CREATURES; i++)
    {
        const auto &attackerStack = &P_CombatManager->stacks[0][i];
        CombatStackSettings::SetCreatureAbilityState(attackerStack, STACK_SETTING_POSITIVE_MORALE, tempAttackerAbility);
        CombatStackSettings::SetCreatureAbilityState(attackerStack, STACK_SETTING_AFTER_ATTACK_ABILITY,
                                                     tempAttackerAbility);

        const auto &defenderStack = &P_CombatManager->stacks[1][i];
        //  instance->SetCreatureAbilityState(defenderStack, STACK_SETTING_NEGATIVE_MORALE,
        //  tempAttackerAbility);
        CombatStackSettings::SetCreatureAbilityState(defenderStack, STACK_SETTING_RESURRECTION, tempAttackerAbility);

        //  instance->SetCombatStackSettings(0, i, tempAttacker);
        //  instance->SetCombatStackSettings(1, i, tempDefender);
    }
}

void __stdcall CombatSettingsManager::BattleMgr_StartBattle(HiHook *h, H3CombatManager *_this)
{

    THISCALL_1(void, h->GetDefaultFunc(), _this);

    // check if combat has human player

    IGamePatch *appliedPatches[] = {
        &CreatureTurnControlRandom::GetInstance(),
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

int __stdcall CombatSettingsManager::BattleMgr_ProcessAction_KeyPressed(HiHook *h, H3CombatManager *_this, H3Msg *msg)
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
void __stdcall CombatSettingsManager::BattleMgr_NewRound(HiHook *h, H3CombatManager *_this)
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

void __stdcall CombatSettingsManager::BattleMgr_SetWinner(HiHook *h, H3CombatManager *_this, const INT side)
{
    THISCALL_2(void, h->GetDefaultFunc(), _this, side);
    instance->ResetCombatSettings();
}

int CombatSettingsManager::GetUserPoints() noexcept
{
    return instance->userControlPoints;
}

void CombatSettingsManager::SetUserPoints(const int newSize) noexcept
{
    instance->userControlPoints = newSize;
}

BOOL CombatSettingsManager::DecreaseUserPoints(const int toDecrease) noexcept
{

    if (instance->userControlPoints >= toDecrease)
    {
        instance->userControlPoints -= toDecrease;
        instance->userControlPointsSpent += toDecrease;
        return true;
    }

    return false;
}

// Implementation of patch creation
void CombatSettingsManager::CreatePatches()
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
