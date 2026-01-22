#pragma once
class CombatCreatureSettingsDlg : public H3Dlg

{

	BOOL settingsChanged = FALSE;

  private:
    CombatCreatureSettingsDlg(const int x, const int y, const H3CombatCreature *creature, const BOOL isRightClick);
    virtual ~CombatCreatureSettingsDlg();


	virtual INT DialogProc(H3Msg& msg) override;

  public:
    static BOOL ShowSettingsDlg(H3CombatCreature *creature, const BOOL isRightClick);
};
