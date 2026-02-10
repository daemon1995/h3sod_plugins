#include <fstream>

#include "framework.h"

#include "PluginText.h"

using json = nlohmann::json;

PluginText *PluginText::instance = nullptr;
PluginText &PluginText::GetInstance()
{
    if (!instance)
    {
        instance = new PluginText();
        instance->Load();
    }
    return *instance;
}
LPCSTR PluginText::GetHintText(const H3CombatCreature *creature, const eStackSettingsId settingId,
                               const AbilityState &changer, const eAbilitySwitchError errorType) const noexcept
{

    if (settingId <= eStackSettingsId::STACK_SETTING_NONE || settingId >= eStackSettingsId::AMOUNT_OF_STACK_SETTINGS)
        return h3_NullString;

    const auto &hintBarText = instance->hintBarText;
    LPCSTR abilityName = instance->stackSettingsText[settingId].name.c_str();
    LPCSTR creatureName = creature->info.GetCreatureName(creature->numberAlive);

    switch (errorType)
    {
    case ABILITY_SWITCH_NO_ATTEMPTS_LEFT:
        libc::sprintf(h3_TextBuffer, hintBarText.combatAbilityError.noAttemptsLeft.c_str(), abilityName,
                      CombatSettingsManager::GetUserPoints());
        break;
    case ABILITY_SWITCH_NO_EFFECT:
        libc::sprintf(h3_TextBuffer, hintBarText.combatAbilityError.noEffect.c_str(), abilityName, creatureName);
        break;
    case ABILITY_SWITCH_NO_ABILITY:
        libc::sprintf(h3_TextBuffer, hintBarText.combatAbilityError.noAbility.c_str(), creatureName);
        break;
    default:

        const auto &triggerStateName = GetStateText(settingId, changer);

        libc::sprintf(h3_TextBuffer, hintBarText.unitAbilitySwitched.c_str(), creatureName, abilityName,
                      triggerStateName);

        // switch (settingId)
        //{
        // case eStackSettingsId::POSITIVE_MORALE_ALL:
        // case eStackSettingsId::NEGATIVE_MORALE_ALL:
        // case eStackSettingsId::POSITIVE_LUCK_ALL:
        // case eStackSettingsId::NEGATIVE_LUCK_ALL:
        //     libc::sprintf(h3_TextBuffer, hintBarText.combatAbilitySwitched.c_str(), abilityName, triggerStateName);
        //     break;
        // default:
        //     libc::sprintf(h3_TextBuffer, hintBarText.unitAbilitySwitched.c_str(), creatureName, abilityName,
        //                   triggerStateName);
        //     break;
        // }
        break;
    }
    return h3_TextBuffer;
}
LPCSTR PluginText::GetStateText(const eStackSettingsId settingId, const AbilityState &changer) const noexcept
{
    switch (settingId)
    {
    case eStackSettingsId::STACK_SETTING_DAMAGE_VARIATION_FIRST:
    case eStackSettingsId::STACK_SETTING_DAMAGE_VARIATION_SECOND:
        return instance->damageStates[changer.triggerState].name.c_str();
    case eStackSettingsId::STACK_SETTING_SPELL_CASTING:
        if (changer.spellToCast != eSpell::NONE)
        {
            return P_Spell[changer.spellToCast].name;
        } // NO BREAK
    default:
        return instance->triggerStates[changer.triggerState].name.c_str();
    }

    return LPCSTR();
}
//
// LPCSTR PluginText::GetDlgText(const eStackSettingsId settingId, const H3CombatCreature *creature)
//{
//    return LPCSTR();
//}
//
// LPCSTR PluginText::GetHintText(const eStackSettingsId settingId, const H3CombatCreature *creature)
//{
//    return LPCSTR();
//}
//
// LPCSTR PluginText::GetStateText(const eStackSettingsId settingId, const AbilityState &changer)
//{
//    if (settingId < eStackSettingsId::STACK_SETTING_POSITIVE_MORALE || settingId >=
//    eStackSettingsId::AMOUNT_OF_STACK_SETTINGS)
//        return h3_NullString;
//
//    if (settingId == eStackSettingsId::STACK_SETTING_DAMAGE_VARIATION_FIRST)
//    {
//        return instance->stateText.damageStates[changer.damageState];
//    }
//    else
//    {
//        return instance->stateText.triggerStates[changer.triggerState];
//    }
//
//    return h3_NullString;
//}

std::string GetPluginDirectoryA()
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
            if (item.contains("name") && item["name"].is_string())
            {
                baseTextArray[i].name = item["name"].get<std::string>();
            }
            if (item.contains("description") && item["description"].is_string())
            {
                baseTextArray[i].description = item["description"].get<std::string>();
            }
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

    static constexpr LPCSTR stackAbilityKeys[] = {
        "positive_morale",        "negative_morale",         "fear",         "spell_casting",
        "resurrection",           "magic_resistance",        "magic_mirror", "positive_luck",
        "negative_luck",          "double_damage",           "wall_attack",  "after_attack_ability",
        "damage_variation_first", "damage_variation_second", "damage_input"};
    static_assert(std::size(stackAbilityKeys) == AMOUNT_OF_STACK_SETTINGS, "Ability keys size mismatch");
    ReadJsonStringFieldToArray(j, "stack_abilities", stackAbilityKeys, stackSettingsText, std::size(stackSettingsText));

    static constexpr LPCSTR sideAbilityKeys[] = {"unaffected_by_morale", "unaffected_by_luck", "unaffected_by_fear",
                                                 "unaffected_by_magic_resistance"};
    static_assert(std::size(sideAbilityKeys) == AMOUNT_OF_SIDE_SETTINGS, "Side ability keys size mismatch");
    ReadJsonStringFieldToArray(j, "side_abilities", sideAbilityKeys, sideSettingsText, std::size(sideSettingsText));

    static constexpr LPCSTR damageKeys[] = {"default", "minimum", "maximum", "minimum_25", "minimum_50", "minimum_75"};
    ReadJsonStringFieldToArray(j, "damage_states", damageKeys, damageStates, std::size(damageStates));

    static constexpr LPCSTR triggerKeys[] = {"default", "always", "never"};
    ReadJsonStringFieldToArray(j, "trigger_states", triggerKeys, triggerStates, std::size(triggerStates));

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
    if (hintBarObj.contains("combat_start") && hintBarObj["combat_start"].is_string())
    {
        combatStart = hintBarObj["combat_start"].get<std::string>();
    }
    if (hintBarObj.contains("text") && hintBarObj["text"].is_string())
    {
        text = hintBarObj["text"].get<std::string>();
    }
    if (hintBarObj.contains("unit_ability_switched") && hintBarObj["unit_ability_switched"].is_string())
    {
        unitAbilitySwitched = hintBarObj["unit_ability_switched"].get<std::string>();
    }
    if (hintBarObj.contains("combat_ability_switched") && hintBarObj["combat_ability_switched"].is_string())
    {
        combatAbilitySwitched = hintBarObj["combat_ability_switched"].get<std::string>();
    }
    if (hintBarObj.contains("combat_ability_triggered") && hintBarObj["combat_ability_triggered"].is_string())
    {
        unitAbilityTriggered = hintBarObj["unit_ability_triggered"].get<std::string>();
    }
    if (hintBarObj.contains("combat_ability_error") && hintBarObj["combat_ability_error"].is_object())
    {
        const auto &errorObj = hintBarObj["combat_ability_error"];
        if (errorObj.contains("no_attempts_left") && errorObj["no_attempts_left"].is_string())
        {
            combatAbilityError.noAttemptsLeft = errorObj["no_attempts_left"].get<std::string>();
        }
        if (errorObj.contains("no_effect") && errorObj["no_effect"].is_string())
        {
            combatAbilityError.noEffect = errorObj["no_effect"].get<std::string>();
        }
        if (errorObj.contains("no_ability") && errorObj["no_ability"].is_string())
        {
            combatAbilityError.noAbility = errorObj["no_ability"].get<std::string>();
        }
    }
}
