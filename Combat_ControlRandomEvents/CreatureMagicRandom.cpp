#include "framework.h"

#include "CreatureMagicRandom.h"

CreatureMagicRandom *CreatureMagicRandom::instance = nullptr;
CreatureMagicRandom::CreatureMagicRandom() : IGamePatch(globalPatcher->CreateInstance(instanceName))
{
    CreatePatches();
}

static int GetCretureSpellPower(const H3CombatCreature *creature)
{
    switch (creature->type)
    {
    case eCreature::FAERIE_DRAGON:
        return 5 * creature->numberAlive;
    case eCreature::MASTER_GENIE:
        return 6;
    case eCreature::ENCHANTER:
        return 3;
    default:
        return 0;
    }
}

static BOOL CreateAvailableSpellsList(const H3CombatCreature *creature, std::vector<eSpell> &outList)
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

    switch (creature->type)
    {
    case eCreature::MASTER_GENIE:
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
    case eCreature::ENCHANTER:
        spellDataArray = CreatureSpellData::GetEnchantersArray();
        for (size_t i = 0; i < CreatureSpellData::ENCHANTERS_ARRAY_SIZE; i++)
        {
            const eSpell spellId = static_cast<eSpell>(spellDataArray[i].spellId);
            if (P_Spell[spellId].level > maxSpellLevel)
                continue;
            outList.push_back(spellId);
        }
        break;
    case eCreature::FAERIE_DRAGON:
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
    default:
        return FALSE;
    }

    outList.shrink_to_fit();

    return !outList.empty();
}

static eSpell GetUserSelectedSpell(const H3CombatCreature *creature)
{
    if (!CreatureSettingsManager::GetUserPoints())
        return eSpell::NONE;

    const auto &settings = CreatureSettingsManager::GetCreatureSettings(creature).At(eSettingsId::SPELL_CASTING);
    if (settings.triggerState == ALWAYS)
        return settings.spellToCast;

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
            P_CombatManager->CastSpell(setSpellByUser, pos, 1, -1, eSecSkillLevel::ADVANCED, 6);
            CreatureSettingsManager::DecreaseUserPoints(1);
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
        char ret = THISCALL_1(char, 0x0447D00, creature);
        data[0] = storedData;
        // if spell can be cast, decrease user points and return success
        if (ret)
        {
            CreatureSettingsManager::DecreaseUserPoints(1);
            return true;
        }
    }
    // retutrn default
    return THISCALL_1(char, h->GetDefaultFunc(), creature);
}

static _LHF_(BattleStack_PrepareFaerieDragonSpell)
{
    if (const auto &creature = reinterpret_cast<H3CombatCreature *>(c->edi))
    {
        const eSpell setSpellByUser = GetUserSelectedSpell(creature);
        if (setSpellByUser != eSpell::NONE)
        {
            c->eax = setSpellByUser;
            return EXEC_DEFAULT;
        }
    }

    return EXEC_DEFAULT;
}

static _LHF_(BattleStack_CastFaerieDragonSpell)
{
    if (auto *creature = reinterpret_cast<H3CombatCreature *>(c->esi))
    {
        const eSpell setSpellByUser = GetUserSelectedSpell(creature);
        if (setSpellByUser != eSpell::NONE)
        {
            creature->faerieDragonSpell = setSpellByUser;
            CreatureSettingsManager::DecreaseUserPoints(1);
            return EXEC_DEFAULT;
        }
    }

    return EXEC_DEFAULT;
}
static _LHF_(BattleStack_PhoenixResurrection)
{
    if (auto *creature = reinterpret_cast<H3CombatCreature *>(c->esi))
    {
        const auto &settings = CreatureSettingsManager::GetCreatureSettings(creature).At(eSettingsId::RESURRECTION);

        switch (settings.triggerState)
        {
        case eTriggerState::ALWAYS:             // 1
        case eTriggerState::NEVER:              // 2
            c->edx = settings.triggerState - 1; // always (0)/ never (1) succeed
            CreatureSettingsManager::DecreaseUserPoints(1);
            c->return_address = 0x04690D7;
            return NO_EXEC_DEFAULT;
        case eTriggerState::DEFAULT:
        default:
            break;
        }
    }
    return EXEC_DEFAULT;
}

static _LHF_(BattleManager_BattleStack_GetResistanceRandom)
{
    const auto &creature = reinterpret_cast<H3CombatCreature *>(c->edi);
    if (CreatureSettingsManager::GetUserPoints())
    {
        const auto &settings = CreatureSettingsManager::GetCreatureSettings(creature);

        switch (settings.abilities.resistance.triggerState)
        {
        case eTriggerState::ALWAYS:
            c->ecx = 100; // -> leads to Rand(100, 100) > [resistance_value]
            break;
        case eTriggerState::NEVER:
            c->edx = 1; // -> leads to Rand(1, 1) > [resistance_value]
            break;
        default:
            return EXEC_DEFAULT;
        }
        CreatureSettingsManager::DecreaseUserPoints(1);
    }

    return EXEC_DEFAULT;
}
static _LHF_(BattleManager_BattleStack_GetBerserkResistanceRandom)
{
    const auto &creature = *reinterpret_cast<H3CombatCreature **>(c->ebp + 0x14);
    if (CreatureSettingsManager::GetUserPoints())
    {
        const auto &settings = CreatureSettingsManager::GetCreatureSettings(creature);

        switch (settings.abilities.resistance.triggerState)
        {
        case eTriggerState::ALWAYS:
            c->edx = 1; // -> leads to Rand(1, 1) <= [resistance_value]
            break;
        case eTriggerState::NEVER:
            c->ecx = 100; // -> leads to Rand(100, 100) <= [resistance_value]
            break;
        default:
            return EXEC_DEFAULT;
        }
        CreatureSettingsManager::DecreaseUserPoints(1);
    }

    return EXEC_DEFAULT;
}
static _LHF_(BattleManager_BattleStack_GetStatusSpellResistanceRandom)
{
    const auto &creature = reinterpret_cast<H3CombatCreature *>(c->esi);
    if (CreatureSettingsManager::GetUserPoints())
    {
        const auto &settings = CreatureSettingsManager::GetCreatureSettings(creature);

        switch (settings.abilities.resistance.triggerState)
        {
        case eTriggerState::ALWAYS:
            c->edx = 1; // -> leads to Rand(1, 1) <= [resistance_value]
            break;
        case eTriggerState::NEVER:
            c->ecx = 100; // -> leads to Rand(100, 100) <= [resistance_value]
            break;
        default:
            return EXEC_DEFAULT;
        }
        CreatureSettingsManager::DecreaseUserPoints(1);
    }

    return EXEC_DEFAULT;
}
static _LHF_(BattleManager_BattleStack_GetAreaSpellResistanceRandom)
{
    const auto &creature = reinterpret_cast<H3CombatCreature *>(c->edi);
    if (CreatureSettingsManager::GetUserPoints())
    {
        const auto &settings = CreatureSettingsManager::GetCreatureSettings(creature);

        switch (settings.abilities.resistance.triggerState)
        {
        case eTriggerState::ALWAYS:
            c->ecx = 1; // -> leads to Rand(1, 1) <= [resistance_value]
            break;
        case eTriggerState::NEVER:
            c->edx = 100; // -> leads to Rand(100, 100) <= [resistance_value]
            break;
        default:
            return EXEC_DEFAULT;
        }
        CreatureSettingsManager::DecreaseUserPoints(1);
    }

    return EXEC_DEFAULT;
}
void CreatureMagicRandom::CreatePatches()
{

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
    // death ripple spell casting
    WriteLoHook(0x05A1012, BattleManager_BattleStack_GetResistanceRandom);
    // desroy undead spell casting
    WriteLoHook(0x05A120F, BattleManager_BattleStack_GetResistanceRandom);

    // berserk spell casting
    WriteLoHook(0x05A2100, BattleManager_BattleStack_GetBerserkResistanceRandom);

    // status spells casting
    WriteLoHook(0x05A6A5C, BattleManager_BattleStack_GetStatusSpellResistanceRandom);
    // Armageddon spell casting
    WriteLoHook(0x05A4F5A, BattleManager_BattleStack_GetStatusSpellResistanceRandom);

    // area spell casting
    WriteLoHook(0x05A4D80, BattleManager_BattleStack_GetAreaSpellResistanceRandom);
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

    auto &creatureSettings = CreatureSettingsManager::GetCreatureSettings(creature);
    const eSpell preselectedSpell = creatureSettings.At(eSettingsId::SPELL_CASTING).spellToCast;

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
eSpell SpellSelectionDlg::ShowSettingsDlg(H3CombatCreature *creature, const H3Msg *msg)
{

    const BOOL isPopup = msg->IsRightClick();
    std::vector<eSpell> availableSpells;
    CreateAvailableSpellsList(creature, availableSpells);
    if (availableSpells.empty())
    {
        return eSpell::NONE;
    }

    auto &dlg = SpellSelectionDlg(creature, availableSpells, isPopup); // , dlgSize.width, dlgSize.height);
    isPopup ? dlg.RMB_Show() : dlg.Start();

    return dlg.selectedSpell;
}
