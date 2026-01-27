#pragma once
class CombatCreatureSettingsDlg : public H3Dlg

{

  protected:
    CombatCreatureSettings *viewedCretureSettings = nullptr;
    CombatCreatureSettings localCreatureSettings{};

    BOOL settingsChanged = FALSE;

    // H3DlgText* dlgTitle = nullptr;
    // H3

    struct DlgRadioGroup
    {
        RECT position;
        INT groupBoxId;
        H3DlgText *groupBoxText = nullptr;
        struct
        {
            H3DlgDefButton *button{};
            H3DlgText *text{};

        } radioButtons[3];

        eSettingsId settingId;

        DlgRadioGroup(const RECT &pos, const int firstItemId, const eSettingsId settingId,
                      const eTriggerState defaultState = DEFAULT);
    };

    std::vector<DlgRadioGroup> radioGroups;

  protected:
    CombatCreatureSettingsDlg(const int x, const int y, const H3CombatCreature *creature, const BOOL isRightClick);
    virtual ~CombatCreatureSettingsDlg();

  protected:
    virtual INT DialogProc(H3Msg &msg) override;

  private:
    void CreateSettingsItems();

  public:
    static BOOL ShowSettingsDlg(H3CombatCreature *creature, const BOOL isRightClick);
    static void __stdcall BattleMgr_ShowMonStatDlg(HiHook *hook, H3CombatManager *mgr, H3CombatCreature *creature,
                                                   BOOL isRightClick);
};
