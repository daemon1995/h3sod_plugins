#include "framework.h"

#include "CombatStackSettingsDlg.h"
#include "PluginText.h"

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
    : H3Dlg(width, height) // , -1, -1, false, true)
{

    viewedCretureSettings = &CombatStackSettings::GetCombatStackSettings(creature);
    localCreatureSettings = *viewedCretureSettings;

    if (!isRightClick)
    {
        CreateOKButton();
        CreateCancelButton();
    }
    CreateSettingsItems();
}

CombatStackSettingsDlg::~CombatStackSettingsDlg()
{
}

INT CombatStackSettingsDlg::DialogProc(H3Msg &msg)
{

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

    const DWORD bitMask = GetAbilitiesBitMask(viewedCretureSettings);

    auto settingsCopy = localCreatureSettings;

    size_t counter = 0;
    int startItemId = 1;
    for (size_t i = 0; i < eStackAbility::AMOUNT_OF_STACK_SETTINGS; i++)
    {
        // if ability exists
        if (bitMask & (1 << i))
        {
            const eStackAbility abilityId = eStackAbility(i);

            const size_t togglesAmount = localCreatureSettings.GetAbilityStatesAmount(abilityId);
            if (togglesAmount < 2)
                continue;

            constexpr INT groupBoxX = 25;

            constexpr INT groupBoxTextHeight = 20;
            constexpr INT groupBoxButtonHeight = 24; // 32x24
            constexpr INT groupBoxHeight = groupBoxTextHeight + groupBoxButtonHeight;
            // constexpr INT groupBoxHeight = 40;

            const INT groupBoxWidth = this->widthDlg - groupBoxX * 2;

            DlgRadioGroup dlgRadioGroup;

            dlgRadioGroup.settingId = abilityId;

            dlgRadioGroup.position.left = groupBoxX;
            dlgRadioGroup.position.top = counter * 25;
            dlgRadioGroup.position.bottom = groupBoxHeight; // counter * 25;
            dlgRadioGroup.position.right = groupBoxHeight;
            // counter * 25;

            const int textY = counter * groupBoxHeight;
            LPCSTR abilityName = PluginText::GetDlgText(abilityId, settingsCopy.creature);
            // const int itemId = startItemId + counter * 3;
            dlgRadioGroup.groupBoxText = CreateText(groupBoxX, textY, groupBoxWidth, groupBoxTextHeight, abilityName,
                                                    NH3Dlg::Text::MEDIUM, eTextColor::REGULAR, startItemId++);

            dlgRadioGroup.position.left = groupBoxX;
            int radioButtonX = groupBoxX;

            auto &radioButtons = dlgRadioGroup.radioButtons;
            radioButtons.resize(togglesAmount);

            const int y = textY + 25;

            const int togglesSpacing = 5;
            const int toggleWidth = (groupBoxWidth - (togglesAmount - 1) * togglesSpacing) / togglesAmount;

            Ability ability;

            settingsCopy.asArray[abilityId] = ability;

            for (size_t toggleIndex = 0; toggleIndex < togglesAmount; toggleIndex++)
            {

                auto &radioButton = radioButtons[toggleIndex];
                const int toggleX = radioButtonX; // +toggleIndex * (toggleWidth + togglesSpacing);

                radioButton.button = CreateOnOffCheckbox(toggleX, y, startItemId++, 0);

                radioButton.text = CreateText(toggleX + 36, y, toggleWidth - 36, groupBoxButtonHeight,
                                              PluginText::GetInstance().GetStateText(abilityId, ability),
                                              NH3Dlg::Text::SMALL, eTextColor::REGULAR, startItemId++);

                settingsCopy.SwitchToNextAbilityState(abilityId, ability);
                settingsCopy.asArray[abilityId] = ability;
                // ability.triggerState = static_cast<eTriggerState>(ability.triggerState + 1);
                radioButtonX += toggleWidth + togglesSpacing;
            }

            dlgRadioGroup.groupBoxId = counter++;

            radioGroups.emplace_back(dlgRadioGroup);
        }
    }

    // this->CreateOnOffCheckbox
}

BOOL CombatStackSettingsDlg::ShowSettingsDlg(H3CombatCreature *creature, const size_t abilitiesAmount,
                                             const BOOL isRightClick)
{

    CombatStackSettingsDlg dlg(400, 400, creature, isRightClick);

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
            CombatSettingsManager::WriteMessageToLog(std::to_string(abilitiesAmount).c_str());
            CombatStackSettingsDlg::ShowSettingsDlg(creature, abilitiesAmount, isRightClick);
        }
    }
    else
    {
        THISCALL_3(void, hook->GetDefaultFunc(), mgr, creature, isRightClick);
    }
}
