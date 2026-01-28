#pragma once
struct PluginText : IPluginText
{
    static constexpr LPCSTR HD_MOD_LANG_KEY = "HD.Language";
    static constexpr LPCSTR HD_MOD_DEFAULT_LANG = "en";

    struct DlgText
    {
        LPCSTR title = nullptr;

    } dlgText;
    /*
        "hint_bar": {
        "combat_start": "Control Combat randomness plugin inited. Attempts Left: \"%d\"\nSHIFT+Left/Right-click on
    combat creature to change it's settings", "text": "text", "unit_ability_switched": "For unit \"%s\" the ability
    \"%s\" is switched to the state \"%s.\"", "combat_ability_switched": "For all the combat units the ability \"%s\" is
    switched to state \"%s.\"", "combat_ability_triggered": "For unit \"%s\" the ability \"%s\" is switched with a state
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
        std::string combatAbilitySwitched;
        std::string combatAbilityTriggered;
        struct
        {
            std::string noAttemptsLeft;
            std::string noEffect;
            std::string noAbility;
        } combatAbilityError;
        void LoadFromJson(const nlohmann::json &j);
    } hintBarText;

    struct BaseText
    {
        std::string name;
        std::string description;
    } abilityText[eSettingsId::AMOUNT_OF_SETTINGS], damageStates[eDamageState::AMOUNT_OF_DAMAGE_STATES],
        triggerStates[eTriggerState::AMOUNT_OF_TRIGGER_STATES];

    enum eTextError : int
    {
    };

    static PluginText *instance;

  protected:
    void Load() override;

    BOOL LoadTextFromJsonFile(const std::string &fileName);
    void ReadJsonStringFieldToArray(const nlohmann::json &j, const std::string &baseKey, const LPCSTR *keys,
                                     BaseText *baseTextArray, const size_t arraySize);

  public:
    static PluginText &GetInstance();
    static LPCSTR GetDlgText(const eSettingsId settingId, const H3CombatCreature *creature);
    static LPCSTR GetHintText(const eSettingsId settingId, const H3CombatCreature *creature, const BOOL success);
    static LPCSTR GetStateText(const eSettingsId settingId, const AbilityChanger &changer);
};
