#pragma once

class CombatStackSettingsDlg : public H3Dlg

{

  protected:
    CombatStackSettings *viewedCretureSettings = nullptr;
    CombatStackSettings localCreatureSettings{};
    
    BOOL settingsChanged = FALSE;

    // H3DlgText* dlgTitle = nullptr;
    // H3

    struct DlgRadioGroup
    {
        RECT position;
        INT groupBoxId;
        eStackAbility settingId;

        H3DlgText *groupBoxText = nullptr;
        struct
        {
            H3DlgDefButton *button{};
            H3DlgText *text{};

        } radioButtons[3];


        // DlgRadioGroup(const RECT &pos, const int firstItemId, const eStackAbility settingId,
        //               const eTriggerState defaultState = TRIGGER_STATE_DEFAULT);
    };

    std::vector<DlgRadioGroup> radioGroups;

  protected:
    CombatStackSettingsDlg(const int x, const int y, const H3CombatCreature *creature, const BOOL isRightClick);
    virtual ~CombatStackSettingsDlg();

  protected:
    virtual BOOL DialogProc(H3Msg &msg) override;
	virtual void OnOK() override;

  private:
    void CreateSettingsItems();

  public:
    static BOOL ShowSettingsDlg(H3CombatCreature *creature, const BOOL isRightClick);
    static void __stdcall BattleMgr_ShowMonStatDlg(HiHook *hook, H3CombatManager *mgr, H3CombatCreature *creature,
                                                   BOOL isRightClick);
};
