#include <unordered_set>

#include "framework.h"

#include "CreatureMagicRandom.h"

CreatureMagicRandom *CreatureMagicRandom::instance = nullptr;
CreaturePrioritySpells CreaturePrioritySpells::creaturePrioritySpells;

CreatureMagicRandom::CreatureMagicRandom() : IGamePatch(globalPatcher->CreateInstance(instanceName))
{
    CreatePatches();
}

static int GetCretureSpellPower(const H3CombatCreature *creature)
{
    switch (creature->type)
    {
    case eCreature::MASTER_GENIE:
        return 6;
    case eCreature::FAERIE_DRAGON:
        return 5 * creature->numberAlive;
    case eCreature::ENCHANTER:
        return 3;
    default:
        return 0;
    }
    return 0;
}

std::vector<eSpell> Reorder(const std::vector<eSpell> &available, const std::vector<eSpell> &priority)
{
    std::vector<eSpell> result;
    result.reserve(available.size());

    // check if exists
    std::unordered_set<eSpell> used(priority.begin(), priority.end());
    // 1. prioritized
    for (eSpell v : priority)
    {
        if (std::find(available.begin(), available.end(), v) != available.end())
            result.push_back(v);
    }

    // 2. all another
    for (eSpell v : available)
    {
        if (!used.count(v))
            result.push_back(v);
    }

    return result;
}

BOOL CreatureSpellData::CreateAvailableSpellsList(const H3CombatCreature *creature, std::vector<eSpell> &outList)
{
    // check max spell level allowed due to terrain and artifacts
    BOOL maxSpellLevel = 5;

    if (P_CombatManager->specialTerrain == 2)
    {
        maxSpellLevel = 1;
    }
    else
    {
        for (auto &hero : P_CombatManager->hero)
        {
            if (!hero)
                continue;
            if (hero->WearsArtifact(eArtifact::ORB_OF_INHIBITION))
                return FALSE;
            if (hero->WearsArtifact(eArtifact::RECANTERS_CLOAK))
                maxSpellLevel = 2;
        }
    }

    outList.clear();
    outList.reserve(h3::limits::SPELLS);
    const CreatureSpellData *spellDataArray = nullptr;
    std::vector<eSpell> *prioritySpells = nullptr;
    CreaturePrioritySpells &creaturePrioritySpells = CreaturePrioritySpells::creaturePrioritySpells;
    switch (creature->type)
    {
    case eCreature::MASTER_GENIE:
        prioritySpells = &creaturePrioritySpells.masterGenie.spells;

        for (size_t i = eSpell::QUICK_SAND; i < eSpell::STONE; i++)
        {
            if (P_Spell[i].level > maxSpellLevel)
                continue;

            if (P_Spell[i].friendlyMass)
            {
                // if combat isn't vs hero, skip protection spells and anti-magic/magic mirror
                switch (i)
                {
                case eSpell::PROTECTION_FROM_AIR:
                case eSpell::PROTECTION_FROM_EARTH:
                case eSpell::PROTECTION_FROM_FIRE:
                case eSpell::PROTECTION_FROM_WATER:
                case eSpell::ANTI_MAGIC:
                case eSpell::MAGIC_MIRROR:
                    if (!P_CombatManager->hero[1])
                        continue;
                default:
                    break;
                }
                outList.push_back(static_cast<eSpell>(i));
            }
        }

        break;
    case eCreature::FAERIE_DRAGON:
        prioritySpells = &creaturePrioritySpells.faerieDragon.spells;

        spellDataArray = CreatureSpellData::GetFaerieDragonArray();
        for (size_t i = 0; i < CreatureSpellData::FAERIE_DRAGON_ARRAY_SIZE; i++)
        {
            const eSpell spellId = static_cast<eSpell>(spellDataArray[i].spellId);

            if (P_Spell[spellId].level > maxSpellLevel)
                continue;

            if (spellDataArray[i].chanceToCast > 0)
                outList.push_back(spellId);
        }
        break;
    case eCreature::ENCHANTER:
        prioritySpells = &creaturePrioritySpells.enchanter.spells;

        spellDataArray = CreatureSpellData::GetEnchantersArray();
        for (size_t i = 0; i < CreatureSpellData::ENCHANTERS_ARRAY_SIZE; i++)
        {
            const eSpell spellId = static_cast<eSpell>(spellDataArray[i].spellId);
            if (P_Spell[spellId].level > maxSpellLevel)
                continue;
            outList.push_back(spellId);
        }
        break;
    default:
        return FALSE;
    }

    if (prioritySpells && prioritySpells->size())
    {
    }
    outList = Reorder(outList, *prioritySpells);
    outList.shrink_to_fit();

    return !outList.empty();
}

static eSpell GetUserSelectedSpell(const H3CombatCreature *creature)
{
    const auto &spellData =
        CombatStackSettings::GetCombatStackSettings(creature)[eStackAbility::STACK_SETTING_SPELL_CASTING];
    if (spellData.triggerState == TRIGGER_STATE_ENABLED)
        return spellData.spellToCast;

    return eSpell::NONE;
}

static void __stdcall BattleStack_CastGenieSpell(HiHook *h, H3CombatCreature *creature, const int pos)
{

    // emulate the whole spell casting process
    // cause original function is too complex to patch in parts

    const eSpell setSpellByUser = GetUserSelectedSpell(creature);
    if (setSpellByUser != eSpell::NONE)
    {
        const auto &targetStack = P_CombatManager->squares[pos].GetMonster();
        if (targetStack->CanReceiveSpell(setSpellByUser))
        {
            CombatStackSettings::GetCombatStackSettings(creature).TriggerAbility(STACK_SETTING_SPELL_CASTING);
            CreaturePrioritySpells::creaturePrioritySpells.masterGenie.UseSpell(setSpellByUser);
            const int spellPower = GetCretureSpellPower(creature);
            P_CombatManager->CastSpell(setSpellByUser, pos, 1, -1, eSecSkillLevel::ADVANCED, spellPower);
            return;
        }
    }

    THISCALL_2(void, h->GetDefaultFunc(), creature, pos);
}

static char __stdcall BattleStack_EnchanterCastsMassSpell(HiHook *h, H3CombatCreature *creature)
{

    // check if user has points
    const eSpell setSpellByUser = GetUserSelectedSpell(creature);

    if (setSpellByUser != eSpell::NONE)
    {
        CreatureSpellData *data = CreatureSpellData::GetEnchantersArray();
        const CreatureSpellData storedData = data[0];

        // emulate first spell in array being the one selected by user
        data[0].spellId = setSpellByUser; // setSpellByUser;
        data[0].chanceToCast = INT32_MAX;

        // call pseudo original function to check if spell can be cast
        char ret = creature->UseEnchanters();
        CreaturePrioritySpells::creaturePrioritySpells.enchanter.UseSpell(setSpellByUser);
        data[0] = storedData;
        // if spell can be cast, decrease user points and return success
        if (ret)
        {
            CombatStackSettings::GetCombatStackSettings(creature).TriggerAbility(STACK_SETTING_SPELL_CASTING);
            return true;
        }
    }
    return THISCALL_1(char, h->GetDefaultFunc(), creature);
}

static _LHF_(BattleStack_PrepareFaerieDragonSpell)
{
    const auto &creature = reinterpret_cast<H3CombatCreature *>(c->edi);
    const eSpell setSpellByUser = GetUserSelectedSpell(creature);
    if (setSpellByUser != eSpell::NONE)
    {
        c->eax = setSpellByUser;
    }

    return EXEC_DEFAULT;
}

static _LHF_(BattleStack_CastFaerieDragonSpell)
{
    auto *creature = reinterpret_cast<H3CombatCreature *>(c->esi);
    const eSpell setSpellByUser = GetUserSelectedSpell(creature);
    if (setSpellByUser != eSpell::NONE)
    {
        CreaturePrioritySpells::creaturePrioritySpells.faerieDragon.UseSpell(setSpellByUser);
        CombatStackSettings::GetCombatStackSettings(creature).TriggerAbility(STACK_SETTING_SPELL_CASTING);
        creature->faerieDragonSpell = setSpellByUser;
    }

    return EXEC_DEFAULT;
}

static _LHF_(BattleStack_PhoenixResurrection)
{
    const auto *creature = reinterpret_cast<H3CombatCreature *>(c->esi);
    auto &settings = CombatStackSettings::GetCombatStackSettings(creature);

    constexpr eStackAbility abilityId = STACK_SETTING_PHOENIX_RESURRECTION;

    if (settings[abilityId].resurrectionState == RESURRECTION_STATE_DEFAULT)
    {
        return EXEC_DEFAULT;
    }

    const int amountToResurrect = settings[abilityId].resurrectionState - 1;

    settings.TriggerAbility(abilityId);

    c->ebx += amountToResurrect;
    c->return_address = 0x04690DF;
    return NO_EXEC_DEFAULT;
}

static double BattleMgr_BattleSide_GetSpellAffectionRate(const int side, const eSpell spellId, const int casterKind)
{
    double result = 1.f;

    const auto &mgr = P_CombatManager->Get();
    auto &stacks = mgr->stacks[side];

    for (auto &stack : stacks)
    {
        if (stack.numberAlive < 1 || stack.activeSpellDuration[eSpell::HYPNOTIZE])
            continue;

        double stackAffection =
            THISCALL_7(double, 0x05A83A0, mgr, spellId, mgr->currentActiveSide, &stack, 0, 1, casterKind);
        if (stackAffection > 0.f && stackAffection < result)
        {
            result = stackAffection;
        }
    }

    return result;
}

struct SpellBreachingData
{
    eSpell spellId = eSpell::NONE;
    INT chanceToResist[2] = {0, 0};
    BOOL hasTriggered[2] = {false, false};
} spellBreachingData;

static void __stdcall BattleMgr_CastSpell(HiHook *h, H3CombatManager *_this, const eSpell spellId, const int pos,
                                          const int casterKind, const int pos2, const int skillLevel,
                                          const int spellPower)
{
    double maxSideResistancePower[2] = {0.f, 0.f};
    spellBreachingData.spellId = spellId;
    if (P_Spell[spellId].type != eSpellTarget::FRIENDLY)
    {
        for (size_t i = 0; i < 2; i++)
        {
            if (CombatSideSettings::GetCombatSideSettings(i).unaffectedByResistance.triggerState ==
                TRIGGER_STATE_ENABLED)
            {
                spellBreachingData.chanceToResist[i] =
                    100 - static_cast<int>(BattleMgr_BattleSide_GetSpellAffectionRate(i, spellId, casterKind) *
                                           static_cast<double>(100.f));
            }
        }
    }

    THISCALL_7(void, h->GetDefaultFunc(), _this, spellId, pos, casterKind, pos2, skillLevel, spellPower);
    for (size_t i = 0; i < 2; i++)
    {
        if (spellBreachingData.hasTriggered[i])
            CombatSideSettings::ResistanceBreachingTriggered(i, spellBreachingData.chanceToResist[i]);
    }

    spellBreachingData = {};
}

static BOOL CombatCreatureResistMightBeBreached(const H3CombatCreature *creature, const int spellAffectionRate)
{
    const int creatureResistanceChance = 100 - spellAffectionRate;

    if (creatureResistanceChance > 0 && creatureResistanceChance <= 80)
    {
        const int side = creature->side;

        if (spellBreachingData.chanceToResist[side])
        {
            return TRUE;
        }
    }
    return FALSE;
}

static _LHF_(BattleManager_BattleStack_GetResistanceRandom)
{

    // check if spell has 100% affection rate, if so, resistance can't be breached, so skip all the logic and checks
    const int spellAffectionRate = c->eax;
    if (spellAffectionRate == 100)
        return EXEC_DEFAULT;

    // only these 2 spells breach resistance cause are massively casted spells
    const auto &creature = reinterpret_cast<H3CombatCreature *>(c->edi);
    const auto spellId = spellBreachingData.spellId;
    if ((spellId == eSpell::DEATH_RIPPLE || spellId == eSpell::DESTROY_UNDEAD) &&
        CombatCreatureResistMightBeBreached(creature, spellAffectionRate))
    {
        c->edx = 1;
        return EXEC_DEFAULT;
    }

    auto &stackSettings = CombatStackSettings::GetCombatStackSettings(creature); // ;

    switch (stackSettings[STACK_SETTING_MAGIC_RESISTANCE].triggerState)
    {
    case TRIGGER_STATE_ALWAYS:
        c->ecx = 100; // -> leads to Rand(100, 100) > [resistance_value]
        break;
    case TRIGGER_STATE_NEVER:
        c->edx = 1; // -> leads to Rand(1, 1) > [resistance_value]
        break;
    default:
        return EXEC_DEFAULT;
    }
    stackSettings.TriggerAbility(STACK_SETTING_MAGIC_RESISTANCE);

    return EXEC_DEFAULT;
}
static _LHF_(BattleManager_BattleStack_GetBerserkResistanceRandom)
{

    // check if spell has 100% affection rate, if so, resistance can't be breached, so skip all the logic and checks
    const int spellAffectionRate = c->eax;
    if (spellAffectionRate == 100)
        return EXEC_DEFAULT;

    const auto &creature = *reinterpret_cast<H3CombatCreature **>(c->ebp + 0x14);
    auto &stackSettings = CombatStackSettings::GetCombatStackSettings(creature); // ;

    switch (stackSettings[STACK_SETTING_MAGIC_RESISTANCE].triggerState)
    {
    case eTriggerState::TRIGGER_STATE_ALWAYS:
        c->edx = 1; // -> leads to Rand(1, 1) <= [resistance_value]
        break;
    case eTriggerState::TRIGGER_STATE_NEVER:
        c->ecx = 100; // -> leads to Rand(100, 100) <= [resistance_value]
        break;
    default:
        return EXEC_DEFAULT;
    }
    stackSettings.TriggerAbility(STACK_SETTING_MAGIC_RESISTANCE);

    return EXEC_DEFAULT;
}

static _LHF_(BattleManager_BattleStack_GetStatusSpellResistanceRandom)
{
    // check if spell has 100% affection rate, if so, resistance can't be breached, so skip all the logic and checks
    const int spellAffectionRate = c->eax;
    if (spellAffectionRate == 100)
        return EXEC_DEFAULT;

    const auto &creature = reinterpret_cast<H3CombatCreature *>(c->esi);
    if (CombatCreatureResistMightBeBreached(creature, spellAffectionRate))
    {
        c->edx = 1;
        return EXEC_DEFAULT;
    }

    auto &stackSettings = CombatStackSettings::GetCombatStackSettings(creature); // ;

    switch (stackSettings[STACK_SETTING_MAGIC_RESISTANCE].triggerState)
    {
    case eTriggerState::TRIGGER_STATE_ALWAYS:
        c->edx = 1; // -> leads to Rand(1, 1) <= [resistance_value]
        break;
    case eTriggerState::TRIGGER_STATE_NEVER:
        c->ecx = 100; // -> leads to Rand(100, 100) <= [resistance_value]
        break;
    default:
        return EXEC_DEFAULT;
    }
    stackSettings.TriggerAbility(STACK_SETTING_MAGIC_RESISTANCE);

    return EXEC_DEFAULT;
}

static _LHF_(BattleManager_BattleStack_GetAreaSpellResistanceRandom)
{
    // check if spell has 100% affection rate, if so, resistance can't be breached, so skip all the logic and checks
    const int spellAffectionRate = c->eax;
    if (spellAffectionRate == 100)
        return EXEC_DEFAULT;

    const auto &creature = reinterpret_cast<H3CombatCreature *>(c->edi);

    // if (CombatCreatureResistMightBeBreached(creature, spellAffectionRate))
    //{
    //     c->ecx = 1;
    //     return EXEC_DEFAULT;
    // }

    auto &stackSettings = CombatStackSettings::GetCombatStackSettings(creature); // ;
    switch (stackSettings[STACK_SETTING_MAGIC_RESISTANCE].triggerState)
    {
    case eTriggerState::TRIGGER_STATE_ALWAYS:
        c->ecx = 1; // -> leads to Rand(1, 1) <= [resistance_value]
        break;
    case eTriggerState::TRIGGER_STATE_NEVER:
        c->edx = 100; // -> leads to Rand(100, 100) <= [resistance_value]
        break;
    default:
        return EXEC_DEFAULT;
    }
    stackSettings.TriggerAbility(STACK_SETTING_MAGIC_RESISTANCE);

    return EXEC_DEFAULT;
}

static _LHF_(BattleManager_BattleStack_MagicMirrorRandom)
{
    // if creature doesn't have magic mirror
    if (!c->edi || c->edi >= 100)
    {
        return EXEC_DEFAULT;
    }

    const auto &creature = reinterpret_cast<H3CombatCreature *>(c->ecx);
    auto &settings = CombatStackSettings::GetCombatStackSettings(creature);

    switch (settings.magicMirror.triggerState)
    {
    case eTriggerState::TRIGGER_STATE_ALWAYS:
        c->eax = 1; // -> leads to edi >= 1 [magic_mirror_chance]
        break;
    case eTriggerState::TRIGGER_STATE_NEVER:
        c->eax = 100; // -> leads to edi >= 100 [magic_mirror_chance]
        break;
    default:
        return EXEC_DEFAULT;
    }

    // decreases points and reports usings
    settings.TriggerAbility(STACK_SETTING_MAGIC_MIRROR);

    c->return_address = 0x059F1F0;
    return NO_EXEC_DEFAULT;
}

// reset all the personal stack settings
static void __stdcall BattleManager_Resurrection(HiHook *h, H3CombatManager *_this, const H3CombatCreature *creature,
                                                 const int resurrectionPowe, const int skillLevel)
{
    if (creature && creature->numberAlive <= 0)
    {
        CombatStackSettings::GetCombatStackSettings(creature).Reset();
    }
    THISCALL_4(void, h->GetDefaultFunc(), _this, creature, resurrectionPowe, skillLevel);
}

static _LHF_(BattleManager_AddNewStackByPitLord)
{
    CombatStackSettings::GetCombatStackSettings(c->Esi<H3CombatCreature *>()).Reset();
    return EXEC_DEFAULT;
}
void CreatureMagicRandom::CreatePatches()
{

    // Cast of any spell -- used as info for resisitance breach at side;

    WriteHiHook(0x05A0140, THISCALL_, BattleMgr_CastSpell);

    // casting creature spells
    WriteHiHook(0x0448357, THISCALL_, BattleStack_CastGenieSpell);
    WriteHiHook(0x04650F9, THISCALL_, BattleStack_EnchanterCastsMassSpell);
    WriteLoHook(0x04472A8, BattleStack_PrepareFaerieDragonSpell);
    WriteLoHook(0x044837B, BattleStack_CastFaerieDragonSpell);

    // phoenix resurrection processing
    WriteLoHook(0x04690CA, BattleStack_PhoenixResurrection);

    // resistance creaturure checking

    // single target spell casting
    WriteLoHook(0x05A0616, BattleManager_BattleStack_GetResistanceRandom);

    // mass spells have own logic
    // death ripple spell casting
    WriteLoHook(0x05A1012, BattleManager_BattleStack_GetResistanceRandom);
    // destroy undead spell casting
    WriteLoHook(0x05A120F, BattleManager_BattleStack_GetResistanceRandom);

    // berserk spell casting
    WriteLoHook(0x05A2100, BattleManager_BattleStack_GetBerserkResistanceRandom);

    // status spells mass casting
    WriteLoHook(0x05A6A5C, BattleManager_BattleStack_GetStatusSpellResistanceRandom); // done
    // Armageddon spell casting
    WriteLoHook(0x05A4F5A, BattleManager_BattleStack_GetStatusSpellResistanceRandom); // done

    // area spell casting
    WriteLoHook(0x05A4D80, BattleManager_BattleStack_GetAreaSpellResistanceRandom);

    // Magic Mirror
    WriteLoHook(0x059F1DF, BattleManager_BattleStack_MagicMirrorRandom);

    // reset resurrected stacks
    WriteHiHook(0x05A7870, THISCALL_, BattleManager_Resurrection);

    //  reset added stacks by spell
    WriteLoHook(0x05A777E, BattleManager_AddNewStackByPitLord); // pitlord

    //   WriteHiHook(0x05A715F, THISCALL_, BattleManager_AddNewStackBySpell);
    //   WriteHiHook(0x05A7600, THISCALL_, BattleManager_AddNewStackBySpell);
}

BOOL CreaturePrioritySpells::LoadUserSettings(LPCSTR _nni)
{

    iniName = CombatSettingsManager::GetDirectory() + "\\" + CombatSettingsManager::GetFileNameNoExt() + ".ini";

    LPCSTR ini = iniName.c_str();
    settingsIni.Open(ini);
    LoadSpecialist(masterGenie);
    LoadSpecialist(faerieDragon);
    LoadSpecialist(enchanter);

    return 0;
}
static std::unordered_set<eSpell> GetUniqueSpells(const eCreature creature)
{

    std::unordered_set<eSpell> uniqueSpells;
    switch (creature)
    {
    case eCreature::MASTER_GENIE:
        for (size_t i = eSpell::QUICK_SAND; i < eSpell::STONE; i++)
        {
            if (P_Spell[i].friendlyMass)
            {
                uniqueSpells.insert(static_cast<eSpell>(i));
            }
        }
        break;
    case eCreature::FAERIE_DRAGON:
        for (size_t i = 0; i < CreatureSpellData::FAERIE_DRAGON_ARRAY_SIZE; i++)
        {
            uniqueSpells.insert(static_cast<eSpell>(CreatureSpellData::GetFaerieDragonArray()[i].spellId));
        }
        break;
    case eCreature::ENCHANTER:
        for (size_t i = 0; i < CreatureSpellData::ENCHANTERS_ARRAY_SIZE; i++)
        {
            uniqueSpells.insert(static_cast<eSpell>(CreatureSpellData::GetEnchantersArray()[i].spellId));
        }
        break;
    default:
        break;
    }
    return uniqueSpells;
}

std::vector<eSpell> Deserialize(const std::string &str, const eCreature creature)
{
    std::vector<eSpell> result;
    auto uniqueSpells = GetUniqueSpells(creature);

    if (uniqueSpells.empty())
        return result;

    size_t start = 0;

    while (start < str.size())
    {
        size_t end = str.find(',', start);

        std::string token = str.substr(start, end == std::string::npos ? std::string::npos : end - start);

        eSpell v = static_cast<eSpell>(std::stoi(token));
        if (uniqueSpells.find(v) == uniqueSpells.end())
        {
            // invalid spell id, skip
            start = end == std::string::npos ? std::string::npos : end + 1;
            continue;
        }
        // защита от дублей
        if (std::find(result.begin(), result.end(), v) == result.end())
            result.push_back(v);

        if (end == std::string::npos)
            break;

        start = end + 1;
    }

    return result;
}

static std::string Serialize(const std::vector<eSpell> &v)
{
    std::string s = " ";
    for (size_t i = 0; i < v.size(); ++i)
    {
        if (i)
            s += ',';
        s += std::to_string(v[i]);
    }
    return s;
}
void CreaturePrioritySpells::SaveSpecialist(const SpellLists &spellList)
{

    std::string s = Serialize(spellList.spells);
    auto &section = settingsIni.Get(iniSection);

    auto &line = section->Get(std::to_string(spellList.creature).c_str());

    H3String strr = s.c_str();
    line->SetString(strr);
}
void CreaturePrioritySpells::LoadSpecialist(SpellLists &spellList)
{
    auto &section = settingsIni.Get(iniSection);
    std::string iniStr = section->GetString(std::to_string(spellList.creature).c_str(), h3_NullString).String();
    spellList.spells = Deserialize(iniStr, spellList.creature);
}

BOOL SaveCreaturePrioritySpells()
{
    return CreaturePrioritySpells::creaturePrioritySpells.SaveUserSettings();
}

BOOL CreaturePrioritySpells::SaveUserSettings()
{

    SaveSpecialist(masterGenie);
    SaveSpecialist(enchanter);
    SaveSpecialist(faerieDragon);

    LPCSTR ini = iniName.c_str();
    return settingsIni.Save(ini, 0);
}

CreatureMagicRandom &CreatureMagicRandom::GetInstance()
{
    if (!instance)
    {
        instance = new CreatureMagicRandom();
    }

    return *instance;
}

SpellSelectionDlg::SpellSelectionDlg(const H3CombatCreature *creature, const std::vector<eSpell> &availableSpells,
                                     const BOOL isPopup, const int width, const int height)
    : H3Dlg(width, height, -1, -1, false, false), availableSpells(availableSpells), selectedSpell(eSpell::NONE)
{

    LPCSTR defName = SpellSelectionDlg::ITEM_DEF_NAME;

    H3DefLoader spellDef(defName);
    const int itemWidth = spellDef->widthDEF;
    const int itemHeight = spellDef->heightDEF;

    const size_t spellCount = availableSpells.size();
    constexpr size_t spacing = ITEMS_PADDING;
    constexpr size_t margin = ITEMS_MARGIN;

    const int itemsPerRow = spellCount < 21 ? std::min(ITEMS_PER_ROW, spellCount) : 7;
    const int rows = (spellCount + itemsPerRow - 1) / itemsPerRow;

    const BOOL createHint = !isPopup;
    enableDoubleClick = !isPopup;
    const size_t hintHeight = createHint ? 19 : 0;

    // change dialog size based on items
    this->widthDlg = margin * 2 + itemsPerRow * itemWidth + (itemsPerRow - 1) * spacing;
    this->heightDlg = margin * 2 + rows * itemHeight + (rows - 1) * spacing + hintHeight;

    xDlg = (H3GameWidth::Get() - widthDlg) >> 1;
    yDlg = (H3GameHeight::Get() - heightDlg) >> 1;

    const auto owner = creature->GetOwner();
    const int frameColor = owner ? owner->owner : P_CombatManager->hero[0]->owner;

    this->AddBackground(true, createHint, frameColor);

    int itemId = 1;
    H3DlgDef *spellItem = nullptr;
    H3DlgItem *selectedSpellItem = nullptr;

    auto &creatureSettings = CombatStackSettings::GetCombatStackSettings(creature);
    const eSpell preselectedSpell = creatureSettings.At(eStackAbility::STACK_SETTING_SPELL_CASTING).spellToCast;

    for (size_t row = 0; row < rows; row++)
    {
        for (size_t column = 0; column < itemsPerRow; column++)
        {
            const size_t spellIndex = row * itemsPerRow + column;
            if (spellIndex < spellCount)
            {
                const int x = margin + column * (itemWidth + spacing);
                const int y = margin + row * (itemHeight + spacing);

                const eSpell spell = availableSpells[spellIndex];
                spellItem = H3DlgDef::Create(x, y, spell, defName, spell);
                AddItem(spellItem);
                if (preselectedSpell == spell)
                {
                    selectedSpellItem = spellItem;
                }
            }
            else
            {
                break;
            }
        }
    }
    spellDef.Release();

    // create selection frame
    if (spellItem)
    {
        H3RGB565 highlightColor = H3RGB888::Highlight();
        selectionFrame = CreateFrame(spellItem, highlightColor, itemId++, -3);
        selectionFrame->DeActivate();
        if (selectedSpellItem)
        {
            selectionFrame->SetX(selectedSpellItem->GetX());
            selectionFrame->SetY(selectedSpellItem->GetY());
        }
        else
        {
            selectionFrame->Hide();
        }
    }

    if (!isPopup)
    {
        CreateOKButton();
    }
}

BOOL SpellSelectionDlg::DialogProc(H3Msg &msg)
{
    if (msg.IsLeftDown())
    {
        const int itemId = msg.itemId;
        if (itemId >= eSpell::QUICK_SAND && itemId <= eSpell::AIR_ELEMENTAL)
        {

            selectedSpell = static_cast<eSpell>(itemId);
            if (selectionFrame)
            {
                if (H3DlgItem *clickedItem = GetDef(itemId))
                {
                    selectionFrame->SetX(clickedItem->GetX());
                    selectionFrame->SetY(clickedItem->GetY());
                    selectionFrame->Show();
                    Redraw();
                }
            }

            return 1;
        }

        return 0;
    }

    return 0;
}
BOOL SpellSelectionDlg::OnDoubleClick(INT itemID, H3Msg &msg)
{
    if (itemID >= eSpell::QUICK_SAND && itemID <= eSpell::AIR_ELEMENTAL)
    {
        selectedSpell = static_cast<eSpell>(itemID);
        Stop();
        return 1;
    }

    return 0;
}
eSpell SpellSelectionDlg::ShowSpellSelectionDialog(H3CombatCreature *creature, const H3Msg *msg)
{

    const BOOL isPopup = msg->IsRightClick();
    std::vector<eSpell> availableSpells;
    CreatureSpellData::CreateAvailableSpellsList(creature, availableSpells);
    if (availableSpells.empty())
    {
        return eSpell::NONE;
    }

    auto &dlg = SpellSelectionDlg(creature, availableSpells, isPopup); // , dlgSize.width, dlgSize.height);
    isPopup ? dlg.RMB_Show() : dlg.Start();

    return dlg.selectedSpell;
}
