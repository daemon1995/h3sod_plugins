#include "framework.h"

CombatCreatureSettingsDlg::CombatCreatureSettingsDlg(const int x, const int y, const H3CombatCreature *creature,
                                                     const BOOL isRightClick)
    : H3Dlg(400, 400, -1, -1, !isRightClick, true)
{

    if (!isRightClick)
    {
        CreateOKButton();
    }
}

CombatCreatureSettingsDlg::~CombatCreatureSettingsDlg()
{
}

INT CombatCreatureSettingsDlg::DialogProc(H3Msg &msg)
{

    return 0;
}

void CombatCreatureSettingsDlg::CreateSettingsItems()
{
    // this->CreateOnOffCheckbox
}

BOOL CombatCreatureSettingsDlg::ShowSettingsDlg(H3CombatCreature *creature, const BOOL isRightClick)
{

    CombatCreatureSettingsDlg dlg(-1, -1, creature, isRightClick);

    isRightClick ? dlg.RMB_Show() : dlg.Start();

    return dlg.settingsChanged;
}

CombatCreatureSettingsDlg::DlgRadioGroup::DlgRadioGroup(const RECT &pos, const int firstItemId,
                                                        const eSettingsId settingId, const eTriggerState defaultState)
{
    position = pos;
    constexpr int margin = 5;
    const int titleWidth = pos.right - pos.left - (margin << 1);

    // groupBoxText = H3DlgText::Create(pos.left + margin, pos.top + margin, titleWidth,20,"h3_NullString",firstItemId,
    // NH3Dlg::Assets::FONT_NORMAL,
    //                                  NH3Dlg::Assets::TEXTURE_FONT_NORMAL, "Setting");

    for (size_t i = 0; i < std::size(radioButtons); i++)
    {
    }
}

void __stdcall CombatCreatureSettingsDlg::BattleMgr_ShowMonStatDlg(HiHook *hook, H3CombatManager *mgr,
                                                                   H3CombatCreature *creature, BOOL isRightClick)
{

    const bool shiftPressed = STDCALL_1(SHORT, PtrAt(0x63A294), VK_SHIFT) & 0x800;
    if (shiftPressed)
    {
        CombatCreatureSettingsDlg::ShowSettingsDlg(creature, isRightClick);
    }
    else
    {
        THISCALL_3(void, hook->GetDefaultFunc(), mgr, creature, isRightClick);
    }
}
