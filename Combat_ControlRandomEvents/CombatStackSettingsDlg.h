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
        static constexpr int INDEXES_PER_GROUP = 3;
        RECT position;
        INT groupBoxId = 0;
        INT selectedIndex = 0;
        eStackAbility settingId;

        H3DlgText *groupBoxText = nullptr;
        struct DlgRadioButton
        {
            H3DlgDefButton *button = nullptr;
            H3DlgText *text = nullptr;
        };
        std::vector<DlgRadioButton> radioButtons;

        void ToggleSelectedIndex(const int index)
        {
            if (index < 0 || index >= INDEXES_PER_GROUP || selectedIndex == index)
                return;
            selectedIndex = index;
            for (int i = 0; i < INDEXES_PER_GROUP; i++)
            {
                radioButtons[i].button->SetFrame(i == index ? 1 : 0);
            }
        }

        // DlgRadioGroup(const RECT &pos, const int firstItemId, const eStackAbility settingId,
        //               const eTriggerState defaultState = TRIGGER_STATE_DEFAULT);
    };

    std::vector<DlgRadioGroup> radioGroups;

  protected:
    CombatStackSettingsDlg(const int width, const int height, const H3CombatCreature *creature, const BOOL isRightClick);
    virtual ~CombatStackSettingsDlg();

  protected:
    virtual BOOL DialogProc(H3Msg &msg) override;
    virtual void OnOK() override;

  private:
    void CreateSettingsItems();

  public:
    static BOOL ShowSettingsDlg(H3CombatCreature *creature, const size_t abilitiesAmount, const BOOL isRightClick);
    static void __stdcall BattleMgr_ShowMonStatDlg(HiHook *hook, H3CombatManager *mgr, H3CombatCreature *creature,
                                                   BOOL isRightClick);
};
