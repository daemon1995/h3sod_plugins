#include <fstream>

#include "framework.h"

#include "PluginText.h"

using json = nlohmann::json;

PluginText *PluginText::instance = nullptr;
char PluginText::textBuffer[512]{};
UINT PluginText::codepage = CP_ACP;

static std::string utf8ToAnsi(const std::string &utf8Str, UINT codepage)
{
    int wideSize = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, NULL, 0);
    wchar_t *wideStr = new wchar_t[wideSize];
    MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, wideStr, wideSize);

    int targetSize = WideCharToMultiByte(codepage, 0, wideStr, -1, NULL, 0, NULL, NULL);
    char *targetStr = new char[targetSize];
    WideCharToMultiByte(codepage, 0, wideStr, -1, targetStr, targetSize, NULL, NULL);

    std::string result(targetStr);
    delete[] wideStr;
    delete[] targetStr;
    return result;
}

static void ReadJsonObjectToString(const json &obj, LPCSTR jsonKey, std::string &string)
{
    if (obj.contains(jsonKey) && obj[jsonKey].is_string())
        string = utf8ToAnsi(obj[jsonKey].get<std::string>(), PluginText::codepage);
}

PluginText &PluginText::GetInstance()
{
    if (!instance)
    {
        instance = new PluginText();
        instance->Load();
    }
    return *instance;
}
static LPCSTR GetCreatureName(const H3CombatCreature *creature)
{
    return creature->info.GetCreatureName(creature->numberAlive);
}
static LPCSTR GetSideName(const int side)
{
    return P_GeneralText->GetText(410 + side);
}
LPCSTR PluginText::GetCreatureAbilitySwitchText(const H3CombatCreature *creature, const eStackAbility settingId,
                                                const Ability &changer,
                                                const eAbilityStateSwitchError errorType) const noexcept
{

    LPCSTR abilityName = stackSettingsText[settingId].name.c_str();
    LPCSTR creatureName = GetCreatureName(creature);

    const auto &triggerStateName = GetStateText(settingId, changer);
    libc::sprintf(h3_TextBuffer, hintBarText.unitAbilitySwitched.c_str(), creatureName, abilityName, triggerStateName);
    return h3_TextBuffer;
}
LPCSTR PluginText::GetSideAbilitySwitchText(const int side, const eSideAbility settingId, const Ability &changer,
                                            const eAbilityStateSwitchError errorType) const noexcept
{
    const auto &triggerStateName = GetStateText(STACK_SETTING_POSITIVE_MORALE, changer);

    LPCSTR abilityName = sideSettingsText[settingId].name.c_str();
    LPCSTR sideName = GetSideName(side);
    libc::sprintf(h3_TextBuffer, hintBarText.unitAbilitySwitched.c_str(), sideName, abilityName, triggerStateName);
    return h3_TextBuffer;
}
LPCSTR PluginText::GetAbilityTriggeredText(const CombatStackSettings *creatureSettings,
                                           const CombatSideSettings *sideSettings, const int settingId,
                                           const int pointsUsed) const noexcept
{
    if (!(creatureSettings || sideSettings))
        return h3_NullString;

    const std::string *abilityName = nullptr, *textFormat = nullptr;
    std::string triggerSource;

    if (creatureSettings)
    {
        //%s/%s/%d/%d/%d
        abilityName = &stackSettingsText[settingId].name;
        textFormat = &hintBarText.unitAbilityTriggered;
        triggerSource = GetCreatureName(creatureSettings->creature);
    }
    else if (sideSettings)
    {
        //%s/%s/%d/%d/%d
        abilityName = &sideSettingsText[settingId].name;
        textFormat = &hintBarText.sideAbilityTriggered;
        triggerSource = GetSideName(sideSettings->side);
    }

    if (!abilityName || !textFormat)
        return h3_NullString;

    libc::sprintf(textBuffer, textFormat->c_str(), triggerSource.c_str(), abilityName->c_str(), pointsUsed,
                  CombatSettingsManager::GetUserPoints(), CombatSettingsManager::GetMaxUserPoints());

    return textBuffer;
}
LPCSTR PluginText::GetCreatureAbilitySwitchErrorText(const CombatStackSettings *creatureSettings, const int settingId,
                                                     const eAbilityStateSwitchError errorType) const noexcept
{

    LPCSTR abilityName = stackSettingsText[settingId].name.c_str();
    LPCSTR creatureName = GetCreatureName(creatureSettings->creature);

    switch (errorType)
    {
    case ABILITY_SWITCH_NO_ATTEMPTS_LEFT:
        libc::sprintf(h3_TextBuffer, hintBarText.combatAbilityError.noAttemptsLeft.c_str(), abilityName,
                      CombatSettingsManager::GetUserPoints());
        break;
    case ABILITY_SWITCH_SWITCH_BLOCKED:
        libc::sprintf(h3_TextBuffer, hintBarText.combatAbilityError.switchBlocked.c_str(), abilityName);
        break;
    case ABILITY_SWITCH_NO_EFFECT:
        libc::sprintf(h3_TextBuffer, hintBarText.combatAbilityError.noEffect.c_str(), abilityName, creatureName);
        break;
    case ABILITY_SWITCH_NO_ABILITY:
        libc::sprintf(h3_TextBuffer, hintBarText.combatAbilityError.noAbility.c_str(), creatureName);
        break;
    default:
        return nullptr;
    }

    return h3_TextBuffer;
}
LPCSTR PluginText::GetSideAbilitySwitchErrorText(const CombatSideSettings *sideSettings, const int settingId,
                                                 const eAbilityStateSwitchError errorType) const noexcept
{
    LPCSTR abilityName = sideSettingsText[settingId].name.c_str();
    LPCSTR sideName = GetSideName(sideSettings->side);

    switch (errorType)
    {
    case ABILITY_SWITCH_NO_ATTEMPTS_LEFT:
        libc::sprintf(h3_TextBuffer, hintBarText.combatAbilityError.noAttemptsLeft.c_str(), abilityName,
                      CombatSettingsManager::GetUserPoints());
        break;
    case ABILITY_SWITCH_SWITCH_BLOCKED:
        libc::sprintf(h3_TextBuffer, hintBarText.combatAbilityError.switchBlocked.c_str(), abilityName);
        break;
    case ABILITY_SWITCH_NO_EFFECT:
        libc::sprintf(h3_TextBuffer, hintBarText.combatAbilityError.noEffect.c_str(), abilityName, sideName);
        break;
    case ABILITY_SWITCH_NO_ABILITY:
        libc::sprintf(h3_TextBuffer, hintBarText.combatAbilityError.noAbility.c_str(), sideName);
        break;
    default:
        return nullptr;
    }

    return h3_TextBuffer;
}
LPCSTR PluginText::GetSideAbilityCustomText(const eSideAbility settingId) const noexcept
{
    return sideSettingsText[settingId].logText.c_str();
}
LPCSTR PluginText::GetCreatureAbilityCustomText(const eStackAbility settingId) const noexcept
{
    return stackSettingsText[settingId].logText.c_str();
}
LPCSTR PluginText::GetStateText(const eStackAbility settingId, const Ability &changer) const noexcept
{
    switch (settingId)
    {
    case eStackAbility::STACK_SETTING_RESURRECTION:

        if (changer.resurrectionState != RESURRECTION_STATE_DEFAULT)
        {
            libc::sprintf(textBuffer, stackSettingsText[settingId].logText.c_str(), changer.resurrectionState - 1,
                          changer.cost);
            return textBuffer;
        }
        break;
    case eStackAbility::STACK_SETTING_SPELL_CASTING:
        if (changer.spellToCast != eSpell::NONE)
        {
            return P_Spell[changer.spellToCast].name;
        } // NO BREAK

    case eStackAbility::STACK_SETTING_DAMAGE_VARIATION_FIRST:
    case eStackAbility::STACK_SETTING_DAMAGE_VARIATION_SECOND:
        return damageStates[changer.triggerState].name.c_str();
    default:
        break;
    }

    return triggerStates[changer.triggerState].name.c_str();
}

static std::string GetPluginDirectoryA()
{
    HMODULE hModule = nullptr;

    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       reinterpret_cast<LPCSTR>(&GetPluginDirectoryA), &hModule);

    char path[MAX_PATH]{};
    GetModuleFileNameA(hModule, path, MAX_PATH);

    // מבנוחאול טלÿ פאיכא
    char *lastSlash = strrchr(path, '\\');
    if (lastSlash)
        *lastSlash = '\0';

    return std::string(path);
}

void PluginText::ReadJsonStringFieldToArray(const nlohmann::json &j, const std::string &baseKey, const LPCSTR *keys,
                                            BaseText *baseTextArray, const size_t arraySize)
{

    if (j.contains(baseKey) && j[baseKey].is_object())
    {
        for (size_t i = 0; i < arraySize; i++)
        {
            if (!j[baseKey].contains(keys[i]) || !j[baseKey][keys[i]].is_object())
                continue;

            const auto &item = j[baseKey][keys[i]];
            ReadJsonObjectToString(item, "name", baseTextArray[i].name);
            ReadJsonObjectToString(item, "description", baseTextArray[i].description);
            ReadJsonObjectToString(item, "log_text", baseTextArray[i].logText);
        }
    }
}

BOOL PluginText::LoadTextFromJsonFile(const std::string &fileName)
{
    //   json j = nlohmann::json::(hdModLang);
    std::ifstream f(fileName);

    if (!f.is_open())
    {
        return false;
    }

    json j;
    f >> j;

    if (j.contains("codepage") && j["codepage"].is_number())
        codepage = j["codepage"].get<UINT>();

    static constexpr LPCSTR stackAbilityKeys[] = {
        "positive_morale",      "negative_morale",        "fear",
        "spell_casting",        "resurrection",           "magic_resistance",
        "magic_mirror",         "positive_luck",          "double_luck",
        "double_damage",        "wall_attack_aim",        "wall_attack_extended",
        "after_attack_ability", "damage_variation_first", "damage_variation_second",
        "damage_input"};
    static_assert(std::size(stackAbilityKeys) == AMOUNT_OF_STACK_SETTINGS, "Ability keys size mismatch");
    ReadJsonStringFieldToArray(j, "stack_abilities", stackAbilityKeys, stackSettingsText, std::size(stackSettingsText));

    static constexpr LPCSTR sideAbilityKeys[] = {"unaffected_by_morale", "unaffected_by_luck", "unaffected_by_fear",
                                                 "unaffected_by_magic_resistance"};
    static_assert(std::size(sideAbilityKeys) == AMOUNT_OF_SIDE_SETTINGS, "Side ability keys size mismatch");
    ReadJsonStringFieldToArray(j, "side_abilities", sideAbilityKeys, sideSettingsText, std::size(sideSettingsText));

    static constexpr LPCSTR triggerKeys[] = {"default", "always", "never"};
    ReadJsonStringFieldToArray(j, "trigger_states", triggerKeys, triggerStates, std::size(triggerStates));

    static constexpr LPCSTR damageKeys[] = {"default", "minimum", "maximum"};
    ReadJsonStringFieldToArray(j, "damage_states", damageKeys, damageStates, std::size(damageStates));

    hintBarText.LoadFromJson(j);
    return true;
}

void PluginText::Load()
{

    std::string hdModLang = HD_MOD_DEFAULT_LANG;
    const std::string hdModLangPath = globalPatcher->VarGetValue<LPCSTR>(HD_MOD_LANG_KEY, h3_NullString);
    if (!hdModLangPath.empty())
    {
        const size_t pos = hdModLangPath.find_last_of('#');
        hdModLang = hdModLangPath.substr(pos + 1, 2);
    }

    const auto &pluginDir = GetPluginDirectoryA();

    std::string jsonFilePath = pluginDir + "/lang/" + hdModLang + ".json";
    if (LoadTextFromJsonFile(jsonFilePath))
    {
        return;
    }
    else if (hdModLang != HD_MOD_DEFAULT_LANG)
    {
        jsonFilePath = pluginDir + "/lang/" + std::string(HD_MOD_DEFAULT_LANG) + ".json";
        if (LoadTextFromJsonFile(jsonFilePath))
        {
            return;
        }
    }
}

void PluginText::HintBarText::LoadFromJson(const nlohmann::json &j)
{
    if (!j.contains("hint_bar") || !j["hint_bar"].is_object())
        return;

    const auto &hintBarObj = j["hint_bar"];
    ReadJsonObjectToString(hintBarObj, "combat_start", combatStart);
    ReadJsonObjectToString(hintBarObj, "text", text);
    ReadJsonObjectToString(hintBarObj, "unit_ability_switched", unitAbilitySwitched);
    ReadJsonObjectToString(hintBarObj, "side_ability_switched", sideAbilitySwitched);
    ReadJsonObjectToString(hintBarObj, "unit_ability_triggered", unitAbilityTriggered);
    ReadJsonObjectToString(hintBarObj, "side_ability_triggered", sideAbilityTriggered);

    if (hintBarObj.contains("combat_ability_error") && hintBarObj["combat_ability_error"].is_object())
    {
        const auto &errorObj = hintBarObj["combat_ability_error"];
        ReadJsonObjectToString(errorObj, "no_attempts_left", combatAbilityError.noAttemptsLeft);
        ReadJsonObjectToString(errorObj, "no_effect", combatAbilityError.noEffect);
        ReadJsonObjectToString(errorObj, "no_ability", combatAbilityError.noAbility);
        ReadJsonObjectToString(errorObj, "switch_blocked", combatAbilityError.switchBlocked);
    }
}
