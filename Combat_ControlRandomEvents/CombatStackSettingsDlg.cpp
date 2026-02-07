#include "framework.h"

CombatStackSettingsDlg::CombatStackSettingsDlg(const int x, const int y, const H3CombatCreature *creature,
                                                     const BOOL isRightClick)
    : H3Dlg(400, 400, -1, -1, !isRightClick, true)
{

    if (!isRightClick)
    {
        CreateOKButton();
    }
}

CombatStackSettingsDlg::~CombatStackSettingsDlg()
{
}

INT CombatStackSettingsDlg::DialogProc(H3Msg &msg)
{

    return 0;
}

void CombatStackSettingsDlg::CreateSettingsItems()
{
    // this->CreateOnOffCheckbox
}

BOOL CombatStackSettingsDlg::ShowSettingsDlg(H3CombatCreature *creature, const BOOL isRightClick)
{

    CombatStackSettingsDlg dlg(-1, -1, creature, isRightClick);

    isRightClick ? dlg.RMB_Show() : dlg.Start();

    return dlg.settingsChanged;
}

CombatStackSettingsDlg::DlgRadioGroup::DlgRadioGroup(const RECT &pos, const int firstItemId,
                                                        const eStackSettingsId settingId, const eTriggerState defaultState)
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

void __stdcall CombatStackSettingsDlg::BattleMgr_ShowMonStatDlg(HiHook *hook, H3CombatManager *mgr,
                                                                   H3CombatCreature *creature, BOOL isRightClick)
{

    const bool shiftPressed = STDCALL_1(SHORT, PtrAt(0x63A294), VK_SHIFT) & 0x800;
    if (shiftPressed)
    {
        CombatStackSettingsDlg::ShowSettingsDlg(creature, isRightClick);
    }
    else
    {
        THISCALL_3(void, hook->GetDefaultFunc(), mgr, creature, isRightClick);
    }
}
