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

CombatStackSettingsDlg::CombatStackSettingsDlg(const int x, const int y, const H3CombatCreature *creature,
                                               const BOOL isRightClick)
    : H3Dlg(400, 400, -1, -1, !isRightClick, true)
{

    viewedCretureSettings = &CombatStackSettings::GetCombatStackSettings(creature);
    localCreatureSettings = *viewedCretureSettings;

    if (!isRightClick)
    {
        CreateOKButton();
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

    size_t counter = 0;
    int startItemId = 1;
    for (size_t i = 0; i < eStackAbility::AMOUNT_OF_STACK_SETTINGS; i++)
    {
        // if ability exists
        if (bitMask & (1 << i))
        {
            constexpr INT groupBoxX = 25;

            constexpr INT groupBoxTextHeight = 20;
            constexpr INT groupBoxButtonHeight = 20;
            constexpr INT groupBoxHeight = groupBoxTextHeight + groupBoxButtonHeight;
            // constexpr INT groupBoxHeight = 40;

            const INT groupBoxWidth = this->widthDlg - groupBoxX * 2;

            DlgRadioGroup dlgRadioGroup;

            dlgRadioGroup.position.left = groupBoxX;
            dlgRadioGroup.position.top = counter * 25;
            dlgRadioGroup.position.bottom = groupBoxHeight; // counter * 25;
            dlgRadioGroup.position.right = groupBoxHeight;
            // counter * 25;

            const int textY = counter * groupBoxHeight;
            LPCSTR abilityName = PluginText::GetDlgText(eStackAbility(i), localCreatureSettings.creature);
            // const int itemId = startItemId + counter * 3;
            dlgRadioGroup.groupBoxText = CreateText(groupBoxX, textY, groupBoxWidth, groupBoxTextHeight, abilityName,
                                                    NH3Dlg::Text::MEDIUM, eTextColor::REGULAR, startItemId++);

            dlgRadioGroup.settingId = eStackAbility(i);

            dlgRadioGroup.position.left = groupBoxX;
            for (auto &radioButton : dlgRadioGroup.radioButtons)
            {
                const int y = 30 + (counter * 25);
                // radioButton.
            }

            dlgRadioGroup.groupBoxId = counter++;

            radioGroups.emplace_back(dlgRadioGroup);
        }
    }

    // this->CreateOnOffCheckbox
}

BOOL CombatStackSettingsDlg::ShowSettingsDlg(H3CombatCreature *creature, const BOOL isRightClick)
{

    CombatStackSettingsDlg dlg(-1, -1, creature, isRightClick);

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
            CombatStackSettingsDlg::ShowSettingsDlg(creature, isRightClick);
        }
    }
    else
    {
        THISCALL_3(void, hook->GetDefaultFunc(), mgr, creature, isRightClick);
    }
}
