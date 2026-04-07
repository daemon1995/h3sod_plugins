#pragma once
struct PluginText : IPluginText
{
    static constexpr LPCSTR HD_MOD_LANG_KEY = "HD.Language";
    static constexpr LPCSTR HD_MOD_DEFAULT_LANG = "en";
    static char textBuffer[512];
    static UINT codepage;
    static PluginText *instance;

    struct DlgText
    {
        LPCSTR title = nullptr;

    } dlgText;
    /*
        "hint_bar": {
        "combat_start": "Control Combat randomness plugin inited. Attempts Left: \"%d\"\nSHIFT+Left/Right-click on
    combat creature to change it's settings", "text": "text", "unit_ability_switched": "For unit \"%s\" the ability
    \"%s\" is switched to the state \"%s.\"", "combat_ability_switched": "For all the combat units the ability \"%s\" is
    switched to state \"%s.\"", "unit_ability_triggered": "For unit \"%s\" the ability \"%s\" is switched with a state
    \"%s\". Attempts used: \"%d\". Attempts left: \"%d\"", "combat_ability_error": { "no_attempts_left": "Warning: No
    attempts left for ability \"%s.\". Attempts used: \"%d.\"", "no_effect": "Warning: Ability \"%s\" can't affect the
    unit \"%s.\""
        }
    },*/
    struct HintBarText
    {
        std::string combatStart;
        std::string text;

        std::string unitAbilitySwitched;
        std::string sideAbilitySwitched;

        std::string unitAbilityTriggered;
        std::string sideAbilityTriggered;
        struct
        {
            std::string noAttemptsLeft;
            std::string noEffect;
            std::string noAbility;
            std::string switchBlocked;
        } combatAbilityError;
        void LoadFromJson(const nlohmann::json &j);
    } hintBarText;

    struct BaseText
    {
        std::string name;
        std::string description;
        std::string logText;
    } stackSettingsText[AMOUNT_OF_STACK_SETTINGS], sideSettingsText[AMOUNT_OF_SIDE_SETTINGS],
        triggerStates[AMOUNT_OF_TRIGGER_STATES], wallAttackStates[AMOUNT_OF_TRIGGER_STATES],
        damageStates[AMOUNT_OF_TRIGGER_STATES];

    struct BattleResultText
    {
        std::string cheater;
        std::string victory;
        std::string defeat;
        void LoadFromJson(const nlohmann::json &j);

    } battleResultText;

  protected:
    void Load() override;

    BOOL LoadTextFromJsonFile(const std::string &fileName);
    void ReadJsonStringFieldToArray(const nlohmann::json &j, const std::string &baseKey, const LPCSTR *keys,
                                    BaseText *baseTextArray, const size_t arraySize);

  public:
    LPCSTR GetStateText(const eStackAbility settingId, const Ability &changer) const noexcept;

  public:
    static PluginText &GetInstance();
    static LPCSTR GetDlgText(const eStackAbility settingId, const H3CombatCreature *creature);
    static LPCSTR GetStackAbilityName(const eStackAbility settingId)
    {
        if (settingId < 0 || settingId >= AMOUNT_OF_STACK_SETTINGS)
            return nullptr;
        return GetInstance().stackSettingsText[settingId].name.c_str();
    }
    static LPCSTR GetStackAbilityStateName(const eStackAbility settingId, const eTriggerState state)
    {
        if (settingId < 0 || settingId >= AMOUNT_OF_STACK_SETTINGS)
            return nullptr;
        return GetInstance().stackSettingsText[settingId].name.c_str();
    }

    LPCSTR GetAbilitySwitchText(const CombatStackSettings *creatureSettings,
                                const CombatSideSettings *sideSettingsconst, const int settingId,
                                const Ability &changer) const noexcept;

    LPCSTR GetAbilityTriggeredText(const CombatStackSettings *creatureSettings, const CombatSideSettings *sideSettings,
                                   const int settingId, const int pointsUsed) const noexcept;
    LPCSTR GetCreatureAbilitySwitchErrorText(const CombatStackSettings *creatureSettings, const int settingId,
                                             const eAbilityStateSwitchError errorType) const noexcept;
    LPCSTR GetSideAbilitySwitchErrorText(const CombatSideSettings *sideSettings, const int settingId,
                                         const eAbilityStateSwitchError errorType) const noexcept;
    LPCSTR GetSideAbilityCustomText(const eSideAbility settingId) const noexcept;
    LPCSTR GetCreatureAbilityCustomText(const eStackAbility settingId) const noexcept;
};
