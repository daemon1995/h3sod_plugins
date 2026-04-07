#pragma once

constexpr int ABILITY_GROUPS_HEIGHT = 50;
class CombatStackSettingsDlg : public H3Dlg

{

  protected:
    CombatStackSettings *viewedCretureSettings = nullptr;
    CombatStackSettings localCreatureSettings{};

    BOOL settingsChanged = FALSE;

    // H3DlgText* dlgTitle = nullptr;
    // H3
    H3DlgDefButton *spellSellectionButton = nullptr;

    int firstGroupItemID = -1;
    int lastGroupItemID = -1;

    struct DlgRadioGroup
    {
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
            const size_t size = radioButtons.size();
            if (index < 0 || index >= size || selectedIndex == index)
                return;
            selectedIndex = index;
            for (int i = 0; i < size; i++)
            {
                auto &radioButton = radioButtons[i].button;
                radioButton->SetFrame(i == index ? 1 : 0);
                radioButton->SetClickFrame(i == index ? 1 : 0);
                radioButton->Draw();
                radioButton->Refresh();
            }
        }

        // DlgRadioGroup(const RECT &pos, const int firstItemId, const eStackAbility settingId,
        //               const eTriggerState defaultState = TRIGGER_STATE_DEFAULT);
    };

    std::vector<DlgRadioGroup> radioGroups;

  protected:
    CombatStackSettingsDlg(const int width, const int height, const H3CombatCreature *creature,
                           const BOOL isRightClick);
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
