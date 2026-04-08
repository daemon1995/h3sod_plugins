#include "framework.h"

#include "CombatStackSettingsDlg.h"
#include "PluginText.h"
struct SpellSelectionDlg
{
    static eSpell ShowSpellSelectionDialog(const H3CombatCreature *creature, const H3Msg *msg);
};
static DWORD GetAbilitiesBitMask(const CombatStackSettings *settings)
{

    DWORD result = 0;
    for (size_t i = 0; i < eStackAbility::AMOUNT_OF_STACK_SETTINGS; i++)
    {
        if (settings->asArray[eStackAbility(i)].triggerState != eTriggerState::TRIGGER_STATE_DEFAULT ||
            settings->IsAffectedBySetting(eStackAbility(i)))
        {
            result |= (1 << i);
        }
    }
    return result;
}

CombatStackSettingsDlg::CombatStackSettingsDlg(const int width, const int height, const H3CombatCreature *creature,
                                               const BOOL isRightClick)
    : H3Dlg(width, height, -1, -1, !isRightClick)
{

    viewedCretureSettings = &CombatStackSettings::GetCombatStackSettings(creature);
    localCreatureSettings = *viewedCretureSettings;

    if (!isRightClick)
    {
        constexpr size_t hintHeight = 20;
        CreateOKButton(25, heightDlg - 50 - hintHeight);
        CreateCancelButton(widthDlg - 25 - 64, heightDlg - 50 - hintHeight);
    }
    CreateSettingsItems();
}

INT CombatStackSettingsDlg::DialogProc(H3Msg &msg)
{

    const int itemId = msg.itemId;
    int groupIndex = -1;
    switch (msg.subtype)
    {

    case eMsgSubtype::RBUTTON_DOWN:
    case eMsgSubtype::LBUTTON_CLICK:
        if (spellSellectionButton == GetDefButton(itemId))
        {
            auto resultSpell = SpellSelectionDlg::ShowSpellSelectionDialog(localCreatureSettings.creature, &msg);

            if (resultSpell != eSpell::NONE)
            {
                groupIndex = itemId / 10 - 1;
                const int toggleIndex = itemId - radioGroups[groupIndex].groupBoxId * 10 - 1;
                const auto abilityId = radioGroups[groupIndex].settingId;
                Ability &ability = localCreatureSettings.asArray[abilityId];
                radioGroups[groupIndex].radioButtons[1].text->SetText(
                    PluginText::GetInstance().GetStateText(abilityId, ability));
            }
            break;
        }
    case eMsgSubtype::LBUTTON_DOWN:
        if (itemId >= firstGroupItemID && itemId <= lastGroupItemID)
        {
            groupIndex = itemId / 10 - 1;
            const int toggleIndex = itemId - radioGroups[groupIndex].groupBoxId * 10 - 1;
            const auto abilityId = radioGroups[groupIndex].settingId;
            if (toggleIndex >= 0)
            {
                if (msg.subtype == eMsgSubtype::LBUTTON_DOWN)
                {
                    //   radioGroups[groupIndex].ToggleSelectedIndex(toggleIndex);
                }
                else
                {
                    auto text = radioGroups[groupIndex].radioButtons[toggleIndex].text;
                    if (text)
                    {
                        //   H3Messagebox::RMB(text->GetH3String().String());
                    }
                }
            }
            else if (msg.subtype == eMsgSubtype::RBUTTON_DOWN)
            {
                H3Messagebox::RMB(PluginText::GetDlgText(abilityId, localCreatureSettings.creature));
            }
            libc::sprintf(h3_TextBuffer, "Group index: %d, Toggle index: %d, Item ID: %d", groupIndex, toggleIndex,
                          itemId);
            CombatSettingsManager::WriteMessageToLog(h3_TextBuffer);
        }
        break;
    default:
        break;
    }

    if (hintBar && hintBar->IsVisible())
    {
        hintBar->ShowHint(&msg);
    }

    return 0;
}

// close dlg and save the settings
void CombatStackSettingsDlg::OnOK()
{
    if (settingsChanged && viewedCretureSettings)
    {

        *viewedCretureSettings = localCreatureSettings;
    }
}

void CombatStackSettingsDlg::CreateSettingsItems()
{

    constexpr INT groupBoxX = 25;
    constexpr INT groupBoxHeight = ABILITY_GROUPS_HEIGHT;
    constexpr INT groupBoxTextHeight = 20;
    constexpr INT groupBoxButtonHeight = 24; // 32x24
    constexpr INT groupBoxMargin = 2;
    constexpr INT groupBoxSpan = 2;

    static_assert(groupBoxHeight >= groupBoxTextHeight + groupBoxButtonHeight + groupBoxMargin * 2 + groupBoxSpan,
                  "Group box height is too small to fit text and buttons with margins and spacing");

    const DWORD bitMask = GetAbilitiesBitMask(viewedCretureSettings);

    int abilitiesAmount = 0;
    for (size_t i = 0; i < 31; i++)
    {
        if (bitMask & (1 << i))
            abilitiesAmount++;
    }

    auto settingsCopy = localCreatureSettings;

    size_t counter = 0;
    int abilityNameY = 16 + groupBoxMargin;
    firstGroupItemID = 10;
    for (size_t i = 0; i < eStackAbility::AMOUNT_OF_STACK_SETTINGS; i++)
    {
        // if ability exists
        if (bitMask & (1 << i))
        {
            const eStackAbility abilityId = eStackAbility(i);

            const size_t togglesAmount = localCreatureSettings.GetAbilityStatesAmount(abilityId);
            if (togglesAmount < 2)
                continue;

            DlgRadioGroup dlgRadioGroup;
            dlgRadioGroup.groupBoxId = ++counter;
            int startItemId = counter * 10;

            const INT groupBoxWidth = this->widthDlg - groupBoxX * 2;

            dlgRadioGroup.settingId = abilityId;

            LPCSTR abilityName = PluginText::GetDlgText(abilityId, settingsCopy.creature);
            dlgRadioGroup.groupBoxText =
                CreateText(groupBoxX, abilityNameY, groupBoxWidth, groupBoxTextHeight, abilityName,
                           NH3Dlg::Text::MEDIUM, eTextColor::REGULAR, startItemId++);

            //            dlgRadioGroup.position.left = groupBoxX;
            int radioButtonX = groupBoxX;

            auto &radioButtons = dlgRadioGroup.radioButtons;
            radioButtons.resize(togglesAmount);

            const int toggleY = abilityNameY + groupBoxTextHeight + groupBoxSpan;

            const int togglesSpacing = 5;
            const int toggleWidth = (groupBoxWidth - (togglesAmount - 1) * togglesSpacing) / togglesAmount;

            if (abilityId == eStackAbility::STACK_SETTING_SPELL_CASTING)
            {
                Ability &ability = settingsCopy.asArray[abilityId];

                auto &radioButton = radioButtons[0];
                const int toggleX = radioButtonX;
                const int checkboxState = ability.triggerState == TRIGGER_STATE_DISABLED;
                if (checkboxState)
                {
                    dlgRadioGroup.selectedIndex = 0;
                }
                // text and check box should have same id for correct click processing
                auto checkBoxBttn = CreateOnOffCheckbox(toggleX, toggleY, startItemId, checkboxState, checkboxState);

                radioButton.text = CreateText(toggleX + 36, toggleY, toggleWidth - 36, groupBoxButtonHeight,
                                              PluginText::GetInstance().GetStateText(abilityId, ability),
                                              NH3Dlg::Text::SMALL, eTextColor::REGULAR, startItemId++);
                H3Random::Rand(1, 100);
                H3Random::MultiplayerRandom(1, 100);
                radioButtonX += toggleWidth + togglesSpacing;

                radioButton = radioButtons[1];
                radioButton.button = CreateButton(radioButtonX, toggleY, startItemId, "iam007.def");
                radioButton.button->ColorDefToPlayer(P_CurrentPlayerID);
                radioButton.text =
                    CreateText(radioButtonX + 36, toggleY, toggleWidth - 36, groupBoxButtonHeight, "Spell sellection",
                               NH3Dlg::Text::SMALL, eTextColor::REGULAR, startItemId++);
                spellSellectionButton = radioButton.button;
            }
            else
            {
                Ability ability;
                settingsCopy.asArray[abilityId] = ability;

                for (size_t toggleIndex = 0; toggleIndex < togglesAmount; toggleIndex++)
                {
                    auto &radioButton = radioButtons[toggleIndex];
                    const int toggleX = radioButtonX;
                    const int checkboxState =
                        localCreatureSettings.asArray[abilityId].triggerState == ability.triggerState;
                    if (checkboxState)
                    {
                        dlgRadioGroup.selectedIndex = toggleIndex;
                    }

                    // text and check box should have same id for correct click processing
                    auto checkBoxBttn =
                        CreateOnOffCheckbox(toggleX, toggleY, startItemId, checkboxState, checkboxState);

                    radioButton.text = CreateText(toggleX + 36, toggleY, toggleWidth - 36, groupBoxButtonHeight,
                                                  PluginText::GetInstance().GetStateText(abilityId, ability),
                                                  NH3Dlg::Text::SMALL, eTextColor::REGULAR, startItemId++);
                    radioButton.button = checkBoxBttn;
                    settingsCopy.SwitchToNextAbilityState(abilityId, ability);
                    settingsCopy.asArray[abilityId] = ability;
                    // ability.triggerState = static_cast<eTriggerState>(ability.triggerState + 1);
                    radioButtonX += toggleWidth + togglesSpacing;
                }
            }
            lastGroupItemID = startItemId - 1;
            radioGroups.emplace_back(dlgRadioGroup);
            abilityNameY += groupBoxHeight;
        }
    }

    // this->CreateOnOffCheckbox
}

int GetDlgHeight(const size_t abilitiesAmount, const BOOL isRightClick)
{

    int result = ABILITY_GROUPS_HEIGHT * abilitiesAmount + 32;

    if (isRightClick == false)
    {
        constexpr size_t hintHeight = 20;
        result += hintHeight + 32 + 10; // space for OK and Cancel buttons and hint bar
    }

    return Clamp(100, result, P_CombatManager->dlg->GetHeight() - 5);
}
BOOL CombatStackSettingsDlg::ShowSettingsDlg(H3CombatCreature *creature, const size_t abilitiesAmount,
                                             const BOOL isRightClick)
{

    const int dlgHeight = GetDlgHeight(abilitiesAmount, isRightClick);
    CombatStackSettingsDlg dlg(400, dlgHeight, creature, isRightClick);

    isRightClick ? dlg.RMB_Show() : dlg.Start();

    return dlg.settingsChanged;
}

// CombatStackSettingsDlg::DlgRadioGroup::DlgRadioGroup(const RECT &pos, const int firstItemId,
//                                                      const eStackAbility settingId, const eTriggerState defaultState)
//{
//     position = pos;
//     constexpr int margin = 5;
//     const int titleWidth = pos.right - pos.left - (margin << 1);
//
//     // groupBoxText = H3DlgText::Create(pos.left + margin, pos.top + margin,
//     titleWidth,20,"h3_NullString",firstItemId,
//     // NH3Dlg::Assets::FONT_NORMAL,
//     //                                  NH3Dlg::Assets::TEXTURE_FONT_NORMAL, "Setting");
//
//     for (size_t i = 0; i < std::size(radioButtons); i++)
//     {
//     }
// }

void __stdcall CombatStackSettingsDlg::BattleMgr_ShowMonStatDlg(HiHook *hook, H3CombatManager *mgr,
                                                                H3CombatCreature *creature, BOOL isRightClick)
{

    const bool shiftPressed = STDCALL_1(SHORT, PtrAt(0x63A294), VK_SHIFT) & 0x800;
    if (shiftPressed)
    {

        const DWORD abilitiesBitMask = GetAbilitiesBitMask(&CombatStackSettings::GetCombatStackSettings(creature));

        size_t abilitiesAmount = 0;
        for (size_t i = 0; i < 31; i++)
        {
            if (abilitiesBitMask & (1 << i))
                abilitiesAmount++;
        }
        if (abilitiesAmount)
        {
            //   CombatSettingsManager::WriteMessageToLog(std::to_string(abilitiesAmount).c_str());
            CombatStackSettingsDlg::ShowSettingsDlg(creature, abilitiesAmount, isRightClick);
        }
    }
    else
    {
        THISCALL_3(void, hook->GetDefaultFunc(), mgr, creature, isRightClick);
    }
}
