#include <fstream>

#include "framework.h"

#include "CreatureSettingsManager.h"
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
LPCSTR PluginText::GetHintText(const eSettingsId settingId, const H3CombatCreature *creature,
                               const eAbilitySwitchError errorType)
{

    if (settingId <= eSettingsId::NONE || settingId >= eSettingsId::AMOUNT_OF_SETTINGS)
        return h3_NullString;

    const auto &hintBarText = instance->hintBarText;
    LPCSTR abilityName = instance->abilityText[settingId].name.c_str();
    LPCSTR creatureName = creature->info.GetCreatureName(creature->numberAlive);

    switch (errorType)
    {
    case ABILITY_SWITCH_NO_ATTEMPTS_LEFT:
        libc::sprintf(h3_TextBuffer, hintBarText.combatAbilityError.noAttemptsLeft.c_str(), abilityName,
                      CreatureSettingsManager::GetUserPoints());
        break;
    case ABILITY_SWITCH_NO_EFFECT:
        libc::sprintf(h3_TextBuffer, hintBarText.combatAbilityError.noEffect.c_str(), abilityName, creatureName);
        break;
    case ABILITY_SWITCH_NO_ABILITY:
        libc::sprintf(h3_TextBuffer, hintBarText.combatAbilityError.noAbility.c_str(), creatureName);
        break;
    default:
        const auto& triggerStateName = instance->triggerStates[eTriggerState::ALWAYS].name.c_str();

        switch (settingId)
        {
        case eSettingsId::POSITIVE_MORALE_ALL:
        case eSettingsId::NEGATIVE_MORALE_ALL:
        case eSettingsId::POSITIVE_LUCK_ALL:
        case eSettingsId::NEGATIVE_LUCK_ALL:
            libc::sprintf(h3_TextBuffer, hintBarText.combatAbilitySwitched.c_str(), abilityName, triggerStateName);
            break;
        default:
            libc::sprintf(h3_TextBuffer, hintBarText.unitAbilitySwitched.c_str(), creatureName, abilityName,
                          triggerStateName);
            break;
        }
        break;
    }
    return h3_TextBuffer;
}
//
// LPCSTR PluginText::GetDlgText(const eSettingsId settingId, const H3CombatCreature *creature)
//{
//    return LPCSTR();
//}
//
// LPCSTR PluginText::GetHintText(const eSettingsId settingId, const H3CombatCreature *creature)
//{
//    return LPCSTR();
//}
//
// LPCSTR PluginText::GetStateText(const eSettingsId settingId, const AbilityChanger &changer)
//{
//    if (settingId < eSettingsId::POSITIVE_MORALE_UNIT || settingId >= eSettingsId::AMOUNT_OF_SETTINGS)
//        return h3_NullString;
//
//    if (settingId == eSettingsId::DAMAGE_VARIATION_FIRST)
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

    static constexpr LPCSTR abilityKeys[] = {
        "positive_morale_unit",
        "positive_morale_all",
        "negative_morale_unit",
        "negative_morale_all",
        "fear",
        "spell_casting",
        "resurrection",
        "magic_resistance",
        "positive_luck_unit",
        "positive_luck_all",
        "negative_luck_unit",
        "negative_luck_all",
        "double_damage",
        "wall_attack",
        "after_attack_ability",
        "damage_variation_first",
        "damage_variation_second",
    };

    static_assert(std::size(abilityKeys) == eSettingsId::AMOUNT_OF_SETTINGS, "Ability keys size mismatch");

    ReadJsonStringFieldToArray(j, "abilities", abilityKeys, abilityText, std::size(abilityKeys));

    static constexpr LPCSTR damageKeys[] = {"default", "minimum", "maximum", "minimum_25", "minimum_50", "minimum_75"};
    ReadJsonStringFieldToArray(j, "damage_states", damageKeys, damageStates, std::size(damageKeys));

    static constexpr LPCSTR triggerKeys[] = {"default", "always", "never"};
    ReadJsonStringFieldToArray(j, "trigger_states", triggerKeys, triggerStates, std::size(triggerKeys));

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
