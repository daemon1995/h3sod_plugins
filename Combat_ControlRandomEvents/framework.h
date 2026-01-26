#pragma once

#define WIN32_LEAN_AND_MEAN // Исключите редко используемые компоненты из
                            // заголовков Windows
// Файлы заголовков Windows
#include <windows.h>
#define NLOHMAN_JSON
#include "..\headers\header.h"
#include "enums.h"

#include "CombatCreatureSettings.h"
#include "CombatCreatureSettingsDlg.h"
#include "CreatureAttackRandom.h"
#include "CreatureMoraleRandom.h"
#include "CreatureSettingsManager.h"
