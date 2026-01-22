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

BOOL CombatCreatureSettingsDlg::ShowSettingsDlg(H3CombatCreature *creature, const BOOL isRightClick)
{

    CombatCreatureSettingsDlg dlg(-1, -1, creature, isRightClick);

    isRightClick ? dlg.RMB_Show() : dlg.Start();

    return dlg.settingsChanged;
}
