﻿/*
===========================================================================

  Copyright (c) 2010-2015 Darkstar Dev Teams

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see http://www.gnu.org/licenses/

  This file is part of DarkStar-server source code.

===========================================================================
*/

#include "../../common/utils.h"

#include <math.h>

#include "battleutils.h"
#include "../grades.h"
#include "../trait.h"
#include "mobutils.h"
#include "petutils.h"
#include "zoneutils.h"
#include "../lua/luautils.h"
#include "../mob_modifier.h"
#include "../mob_spell_list.h"
#include "../spell.h"
#include "../items/item_weapon.h"
#include "../status_effect_container.h"
#include "../mob_spell_container.h"
#include <vector>
#include "../packets/action.h"

namespace mobutils
{
	ModsMap_t mobFamilyModsList;
	ModsMap_t mobPoolModsList;
	ModsMap_t mobSpawnModsList;

/************************************************************************
*																		*
*  Расчет базовой величины оружия монстров								*
*																		*
************************************************************************/

uint16 GetWeaponDamage(CMobEntity* PMob)
{
    uint16 lvl = PMob->GetMLevel();
    uint8 bonus = 0;

    if (lvl >= 75)
    {
        bonus = 3;
    }
    else if (lvl >= 60)
    {
        bonus = 2;
    }
    else if (lvl >= 50)
    {
        bonus = 1;
    }

    uint16 damage = lvl + bonus;

    damage *= (float)PMob->m_dmgMult / 100.0f;

    if (PMob->getMobMod(MOBMOD_WEAPON_BONUS) != 0)
    {
        damage *= (float)PMob->getMobMod(MOBMOD_WEAPON_BONUS) / 100.0f;
    }

    return damage;
}

uint16 GetMagicEvasion(CMobEntity* PMob)
{
    uint8 mEvaRank = 3;

    return GetBase(PMob, mEvaRank);
}

uint16 GetEvasion(CMobEntity* PMob)
{
    uint8 evaRank = PMob->evaRank;

    // Mob evasion is based on job
    // but occasionally war mobs
    // might have a different rank
    switch (PMob->GetMJob())
    {
        case JOB_THF:
        case JOB_NIN:
            evaRank = 1;
        break;
        case JOB_MNK:
        case JOB_DNC:
        case JOB_SAM:
        case JOB_PUP:
        case JOB_RUN:
            evaRank = 2;
        break;
        case JOB_RDM:
        case JOB_BRD:
        case JOB_GEO:
        case JOB_COR:
            evaRank = 4;
        break;
        case JOB_WHM:
        case JOB_SCH:
        case JOB_RNG:
        case JOB_SMN:
        case JOB_BLM:
            evaRank = 5;
        break;
    }

    return GetBase(PMob, evaRank);
}

/************************************************************************
*																		*
*  Базовое значение для расчера характеристик							*
*  (на название не хватило фантазии)									*
*																		*
************************************************************************/

uint16 GetBaseToRank(uint8 rank, uint16 lvl)
{
	switch (rank)
	{
		case 1: return (5+((lvl-1)*50)/100); // A
		case 2: return (4+((lvl-1)*45)/100); // B
		case 3: return (4+((lvl-1)*40)/100); // C
		case 4: return (3+((lvl-1)*35)/100); // D
		case 5: return (3+((lvl-1)*30)/100); // E
		case 6: return (2+((lvl-1)*25)/100); // F
		case 7: return (2+((lvl-1)*20)/100); // G
	}
	return 0;
}

/************************************************************************
*																		*
*  Базовое значение для расчерта защиты и уклонения						*
*  (на название не хватило фантазии)									*
*																		*
************************************************************************/

uint16 GetBase(CMobEntity * PMob, uint8 rank)
 {
 	uint8 lvl = PMob->GetMLevel();
 	if(lvl > 50){
 		switch(rank){
 			case 1: // A
 				return (float)153+(lvl-50)*5.0;
 			case 2: // B
 				return (float)147+(lvl-50)*4.9;
 			case 3: // C
 				return (float)136+(lvl-50)*4.8;
 			case 4: // D
 				return (float)126+(lvl-50)*4.7;
 			case 5: // E
 				return (float)116+(lvl-50)*4.5;
 			case 6: // F
 				return (float)106+(lvl-50)*4.4;
 			case 7: // G
 				return (float)96+(lvl-50)*4.3;
 		}
 	} else {
 		switch(rank){
 			case 1:
 				return (float)6+(lvl-1)*3.0;
 			case 2:
 				return (float)5+(lvl-1)*2.9;
 			case 3:
 				return (float)5+(lvl-1)*2.8;
 			case 4:
 				return (float)4+(lvl-1)*2.7;
 			case 5:
 				return (float)4+(lvl-1)*2.5;
 			case 6:
 				return (float)3+(lvl-1)*2.4;
 			case 7:
 				return (float)3+(lvl-1)*2.3;
 		}
 	}

        ShowError("Mobutils::GetBase rank (%d) is out of bounds for mob (%u) \n", rank, PMob->id);
	return 0;
}

/************************************************************************
*                                                                       *
*  Расчет атрибутов (характеристик) монстра                             *
*                                                                       *
************************************************************************/

void CalculateStats(CMobEntity * PMob)
{
    // remove all to keep mods in sync
    PMob->StatusEffectContainer->KillAllStatusEffect();
    PMob->restoreModifiers();
    PMob->restoreMobModifiers();

    bool isNM = PMob->m_Type & MOBTYPE_NOTORIOUS;
    JOBTYPE mJob = PMob->GetMJob();
    JOBTYPE sJob = PMob->GetSJob();
    uint8 mLvl = PMob->GetMLevel();
    ZONETYPE zoneType = PMob->loc.zone->GetType();

    if(PMob->HPmodifier == 0)
    {
        float hpScale = PMob->HPscale;

        if (PMob->getMobMod(MOBMOD_HP_SCALE) != 0)
        {
            hpScale = (float)PMob->getMobMod(MOBMOD_HP_SCALE) / 100.0f;
        }

        float growth = 1.06;
        float petGrowth = 0.75;
        float base = 18.0;

        //give hp boost every 10 levels after 25
        //special boosts at 25 and 50
        if(mLvl > 75)
        {
            growth = 1.28;
            petGrowth = 1.03;
        }
        else if(mLvl > 65)
        {
            growth = 1.27;
            petGrowth = 1.02;
        }
        else if(mLvl > 55)
        {
            growth = 1.25;
            petGrowth = 0.99;
        }
        else if(mLvl > 50)
        {
            growth = 1.21;
            petGrowth = 0.96;
        }
        else if(mLvl > 45)
        {
            growth = 1.17;
            petGrowth = 0.95;
        }
        else if(mLvl > 35)
        {
            growth = 1.14;
            petGrowth = 0.92;
        }
        else if(mLvl > 25)
        {
            growth = 1.1;
            petGrowth = 0.82;
        }

        // pets have lower health
        if(PMob->PMaster != nullptr)
        {
            growth = petGrowth;
        }


        PMob->health.maxhp = (int16)(base * pow(mLvl, growth) * hpScale);

        if(isNM)
        {
            PMob->health.maxhp *= 2.0;
            if(mLvl > 75){
                PMob->health.maxhp *= 2.5;
            }
        }

    }
    else
    {
        PMob->health.maxhp = PMob->HPmodifier;
    }

    if(isNM)
    {
        PMob->health.maxhp *= map_config.nm_hp_multiplier;
    }
    else
    {
        PMob->health.maxhp *= map_config.mob_hp_multiplier;
    }

    bool hasMp = false;

    switch(mJob){
    case JOB_PLD:
    case JOB_WHM:
    case JOB_BLM:
    case JOB_RDM:
    case JOB_DRK:
    case JOB_BLU:
    case JOB_SCH:
    case JOB_SMN:
        hasMp = true;
        break;
    }

    switch(sJob){
    case JOB_PLD:
    case JOB_WHM:
    case JOB_BLM:
    case JOB_RDM:
    case JOB_DRK:
    case JOB_BLU:
    case JOB_SCH:
    case JOB_SMN:
        hasMp = true;
        break;
    }

    if(PMob->getMobMod(MOBMOD_MP_BASE))
    {
        hasMp = true;
    }

    if(hasMp)
    {
        float scale = PMob->MPscale;

        if(PMob->getMobMod(MOBMOD_MP_BASE))
        {
            scale = (float)PMob->getMobMod(MOBMOD_MP_BASE) / 100.0f;
        }

        if(PMob->MPmodifier == 0)
        {
            PMob->health.maxmp = (int16)(18.2 * pow(mLvl,1.1075) * scale) + 10;
            if(isNM)
            {
                PMob->health.maxmp *= 1.5;
                if(mLvl>75)
                {
                    PMob->health.maxmp *= 1.5;
                }
            }
        }
        else
        {
            PMob->health.maxmp = PMob->MPmodifier;
        }

        if(isNM)
        {
            PMob->health.maxhp *= map_config.nm_mp_multiplier;
        }
        else
        {
            PMob->health.maxhp *= map_config.mob_mp_multiplier;
        }
    }

    PMob->UpdateHealth();

    PMob->health.tp = 0;
    PMob->health.hp = PMob->GetMaxHP();
    PMob->health.mp = PMob->GetMaxMP();

    PMob->m_Weapons[SLOT_MAIN]->setDamage(GetWeaponDamage(PMob));

    //reduce weapon delay of MNK
    if(PMob->GetMJob()==JOB_MNK){
        PMob->m_Weapons[SLOT_MAIN]->resetDelay();
    }

    uint16 fSTR = GetBaseToRank(PMob->strRank, mLvl);
    uint16 fDEX = GetBaseToRank(PMob->dexRank, mLvl);
    uint16 fVIT = GetBaseToRank(PMob->vitRank, mLvl);
    uint16 fAGI = GetBaseToRank(PMob->agiRank, mLvl);
    uint16 fINT = GetBaseToRank(PMob->intRank, mLvl);
    uint16 fMND = GetBaseToRank(PMob->mndRank, mLvl);
    uint16 fCHR = GetBaseToRank(PMob->chrRank, mLvl);

    uint16 mSTR = GetBaseToRank(grade::GetJobGrade(PMob->GetMJob(),2), mLvl);
    uint16 mDEX = GetBaseToRank(grade::GetJobGrade(PMob->GetMJob(),3), mLvl);
    uint16 mVIT = GetBaseToRank(grade::GetJobGrade(PMob->GetMJob(),4), mLvl);
    uint16 mAGI = GetBaseToRank(grade::GetJobGrade(PMob->GetMJob(),5), mLvl);
    uint16 mINT = GetBaseToRank(grade::GetJobGrade(PMob->GetMJob(),6), mLvl);
    uint16 mMND = GetBaseToRank(grade::GetJobGrade(PMob->GetMJob(),7), mLvl);
    uint16 mCHR = GetBaseToRank(grade::GetJobGrade(PMob->GetMJob(),8), mLvl);

    uint16 sSTR = GetBaseToRank(grade::GetJobGrade(PMob->GetSJob(),2), PMob->GetSLevel());
    uint16 sDEX = GetBaseToRank(grade::GetJobGrade(PMob->GetSJob(),3), PMob->GetSLevel());
    uint16 sVIT = GetBaseToRank(grade::GetJobGrade(PMob->GetSJob(),4), PMob->GetSLevel());
    uint16 sAGI = GetBaseToRank(grade::GetJobGrade(PMob->GetSJob(),5), PMob->GetSLevel());
    uint16 sINT = GetBaseToRank(grade::GetJobGrade(PMob->GetSJob(),6), PMob->GetSLevel());
    uint16 sMND = GetBaseToRank(grade::GetJobGrade(PMob->GetSJob(),7), PMob->GetSLevel());
    uint16 sCHR = GetBaseToRank(grade::GetJobGrade(PMob->GetSJob(),8), PMob->GetSLevel());

    if(PMob->GetSLevel() > 15)
    {
        sSTR /= 2;
        sDEX /= 2;
        sAGI /= 2;
        sINT /= 2;
        sMND /= 2;
        sCHR /= 2;
        sVIT /= 2;
    }
    else
    {
        sSTR = 0;
        sDEX = 0;
        sAGI = 0;
        sINT = 0;
        sMND = 0;
        sCHR = 0;
        sVIT = 0;
    }

    PMob->stats.STR = fSTR + mSTR + sSTR;
    PMob->stats.DEX = fDEX + mDEX + sDEX;
    PMob->stats.VIT = fVIT + mVIT + sVIT;
    PMob->stats.AGI = fAGI + mAGI + sAGI;
    PMob->stats.INT = fINT + mINT + sINT;
    PMob->stats.MND = fMND + mMND + sMND;
    PMob->stats.CHR = fCHR + mCHR + sCHR;

    if(isNM)
    {
        PMob->stats.STR *= (1.5 * map_config.nm_stat_multiplier);
        PMob->stats.DEX *= (1.5 * map_config.nm_stat_multiplier);
        PMob->stats.VIT *= (1.5 * map_config.nm_stat_multiplier);
        PMob->stats.AGI *= (1.5 * map_config.nm_stat_multiplier);
        PMob->stats.INT *= (1.5 * map_config.nm_stat_multiplier);
        PMob->stats.MND *= (1.5 * map_config.nm_stat_multiplier);
        PMob->stats.CHR *= (1.5 * map_config.nm_stat_multiplier);
    }
    else
    {
        PMob->stats.STR *= map_config.mob_stat_multiplier;
        PMob->stats.DEX *= map_config.mob_stat_multiplier;
        PMob->stats.VIT *= map_config.mob_stat_multiplier;
        PMob->stats.AGI *= map_config.mob_stat_multiplier;
        PMob->stats.INT *= map_config.mob_stat_multiplier;
        PMob->stats.MND *= map_config.mob_stat_multiplier;
        PMob->stats.CHR *= map_config.mob_stat_multiplier;
    }

    // special case, give spell list to my pet
    if(PMob->getMobMod(MOBMOD_PET_SPELL_LIST) && PMob->PPet != nullptr)
    {
        // Stubborn_Dredvodd
        CMobEntity* PPet = (CMobEntity*)PMob->PPet;

        // give pet spell list
        PPet->m_SpellListContainer = mobSpellList::GetMobSpellList(PMob->getMobMod(MOBMOD_PET_SPELL_LIST));
    }

    if(PMob->getMobMod(MOBMOD_SPELL_LIST))
    {
        PMob->m_SpellListContainer = mobSpellList::GetMobSpellList(PMob->getMobMod(MOBMOD_SPELL_LIST));
    }

    // cap all stats for mLvl / job
    for (int i=SKILL_DIV; i <=SKILL_BLU; i++)
    {
        uint16 maxSkill = battleutils::GetMaxSkill((SKILLTYPE)i,PMob->GetMJob(),mLvl > 99 ? 99 : mLvl);
        if (maxSkill != 0)
            {
            PMob->WorkingSkills.skill[i] = maxSkill;
        }
        else //if the mob is WAR/BLM and can cast spell
        {
            // set skill as high as main level, so their spells won't get resisted
            uint16 maxSubSkill = battleutils::GetMaxSkill((SKILLTYPE)i,PMob->GetSJob(),mLvl > 99 ? 99 : mLvl);

            if (maxSubSkill != 0)
            {
                PMob->WorkingSkills.skill[i] = maxSubSkill;
            }
        }
    }

    PMob->addModifier(MOD_DEF, GetBase(PMob,PMob->defRank));
    PMob->addModifier(MOD_EVA, GetEvasion(PMob));
    PMob->addModifier(MOD_ATT, GetBase(PMob,PMob->attRank));
    PMob->addModifier(MOD_ACC, GetBase(PMob,PMob->accRank));

    //natural magic evasion
    PMob->addModifier(MOD_MEVA, GetMagicEvasion(PMob));

    // add traits for sub and main
    battleutils::AddTraits(PMob, traits::GetTraits(mJob), mLvl);
    battleutils::AddTraits(PMob, traits::GetTraits(PMob->GetSJob()), PMob->GetSLevel());

    SetupJob(PMob);
    SetupRoaming(PMob);

    // All beastmen drop gil
    if (PMob->m_EcoSystem == SYSTEM_BEASTMEN)
    {
        PMob->defaultMobMod(MOBMOD_GIL_BONUS, 100);
    }

    if (PMob->PMaster != nullptr)
    {
        SetupPetSkills(PMob);
    }

    PMob->m_Behaviour |= PMob->getMobMod(MOBMOD_BEHAVIOR);

    if(zoneType == ZONETYPE_DUNGEON)
    {
        SetupDungeonMob(PMob);
    }
    else if(zoneType == ZONETYPE_BATTLEFIELD)
    {
        SetupBattlefieldMob(PMob);
    }
    else if(zoneType == ZONETYPE_DYNAMIS)
    {
        SetupDynamisMob(PMob);
    }

    if(PMob->m_Type & MOBTYPE_NOTORIOUS)
    {
        SetupNMMob(PMob);
    }

    if(PMob->m_Type & MOBTYPE_EVENT)
    {
        SetupEventMob(PMob);
    }

    if(PMob->m_Family == 335)
    {
        SetupMaat(PMob);
    }

    if (PMob->CanStealGil())
    {
        PMob->ResetGilPurse();
    }
}

void SetupJob(CMobEntity* PMob)
{
    JOBTYPE mJob = PMob->GetMJob();

    switch(mJob)
    {
        case JOB_THF:
            // thfs drop more gil
            if (PMob->m_EcoSystem == SYSTEM_BEASTMEN)
            {
                // 50% bonus
                PMob->defaultMobMod(MOBMOD_GIL_BONUS, 150);
            }
            break;
        case JOB_DRG:
            // drg can use 2 hour multiple times
            PMob->setMobMod(MOBMOD_2HOUR_MULTI, 1);

            // only drgs in 3rd expansion calls wyvern as non-NM
            // include fomors
            if(!(PMob->m_Type & MOBTYPE_NOTORIOUS) && PMob->loc.zone->GetContinentID() == THE_ARADJIAH_CONTINENT || PMob->m_Family == 115)
            {
                // 20 min recast
                PMob->defaultMobMod(MOBMOD_SPECIAL_SKILL, 476);
                PMob->defaultMobMod(MOBMOD_SPECIAL_COOL, 720);
            }
            break;
        case JOB_RNG:

            // giga
            if(PMob->m_Family >= 126 && PMob->m_Family <= 130 || PMob->m_Family == 328)
            {
                // only used while at range
                PMob->defaultMobMod(MOBMOD_SPECIAL_COOL, 12);
                // catapult
                PMob->defaultMobMod(MOBMOD_SPECIAL_SKILL, 658);
            }
            else
            {
                // all other rangers
                PMob->defaultMobMod(MOBMOD_SPECIAL_COOL, 12);
                PMob->defaultMobMod(MOBMOD_SPECIAL_SKILL, 272);
                PMob->defaultMobMod(MOBMOD_STANDBACK_COOL, 8);
            }

            PMob->defaultMobMod(MOBMOD_HP_STANDBACK, 70);

            break;
        case JOB_NIN:
            PMob->defaultMobMod(MOBMOD_SPECIAL_COOL, 9);
            PMob->defaultMobMod(MOBMOD_SPECIAL_SKILL, 272);
            PMob->defaultMobMod(MOBMOD_MAGIC_COOL, 35);
            PMob->defaultMobMod(MOBMOD_BUFF_CHANCE, 20);
            PMob->defaultMobMod(MOBMOD_MAGIC_DELAY, 7);

            PMob->defaultMobMod(MOBMOD_HP_STANDBACK, 70);
            break;
        case JOB_BST:
            PMob->defaultMobMod(MOBMOD_SPECIAL_COOL, 70);
            PMob->defaultMobMod(MOBMOD_SPECIAL_SKILL, 1017);
            break;
        case JOB_PUP:
            PMob->defaultMobMod(MOBMOD_SPECIAL_SKILL, 1901);
            PMob->defaultMobMod(MOBMOD_SPECIAL_COOL, 720);
            break;
        case JOB_BLM:
            PMob->defaultMobMod(MOBMOD_STANDBACK_COOL, 16);
            PMob->defaultMobMod(MOBMOD_MAGIC_COOL, 35);
            PMob->defaultMobMod(MOBMOD_GA_CHANCE, 40);
            PMob->defaultMobMod(MOBMOD_BUFF_CHANCE, 15);


            PMob->defaultMobMod(MOBMOD_HP_STANDBACK, 70);
            break;
        case JOB_PLD:
            PMob->defaultMobMod(MOBMOD_MAGIC_DELAY, 7);
            break;
        case JOB_DRK:
            PMob->defaultMobMod(MOBMOD_MAGIC_DELAY, 7);
            break;
        case JOB_WHM:
            PMob->defaultMobMod(MOBMOD_MAGIC_COOL, 35);
            PMob->defaultMobMod(MOBMOD_MAGIC_DELAY, 10);
            break;
        case JOB_BRD:
            PMob->defaultMobMod(MOBMOD_MAGIC_COOL, 35);
            PMob->defaultMobMod(MOBMOD_GA_CHANCE, 25);
            PMob->defaultMobMod(MOBMOD_BUFF_CHANCE, 60);
            PMob->defaultMobMod(MOBMOD_MAGIC_DELAY, 10);
        case JOB_BLU:
            PMob->defaultMobMod(MOBMOD_MAGIC_COOL, 35);
        case JOB_RDM:
            PMob->defaultMobMod(MOBMOD_MAGIC_COOL, 35);
            PMob->defaultMobMod(MOBMOD_GA_CHANCE, 15);
            PMob->defaultMobMod(MOBMOD_BUFF_CHANCE, 40);
            PMob->defaultMobMod(MOBMOD_MAGIC_DELAY, 10);
        case JOB_SMN:
            PMob->defaultMobMod(MOBMOD_MAGIC_COOL, 70);
            // smn only has "buffs"
            PMob->defaultMobMod(MOBMOD_BUFF_CHANCE, 100);
    }

}

void SetupRoaming(CMobEntity* PMob)
{
    uint16 distance = 10;
    uint16 turns = 1;
    uint16 cool = 20;
    uint16 rate = 15;

    switch(PMob->m_EcoSystem)
    {
        case SYSTEM_BEASTMEN:
            distance = 20;
            turns = 5;
            cool = 45;
            break;
    }

    // default mob roaming mods
    PMob->defaultMobMod(MOBMOD_ROAM_DISTANCE, distance);
    PMob->defaultMobMod(MOBMOD_ROAM_TURNS, turns);
    PMob->defaultMobMod(MOBMOD_ROAM_COOL, cool);
    PMob->defaultMobMod(MOBMOD_ROAM_RATE, rate);

    if(PMob->m_roamFlags & ROAMFLAG_AMBUSH)
    {
        PMob->setMobMod(MOBMOD_SPECIAL_SKILL, 278);
        PMob->setMobMod(MOBMOD_SPECIAL_COOL, 1);
        PMob->m_specialFlags |= SPECIALFLAG_HIDDEN;
        // always stay close to spawn
        PMob->m_maxRoamDistance = 2.0f;
        PMob->setMobMod(MOBMOD_ROAM_DISTANCE, 5);
        PMob->setMobMod(MOBMOD_ROAM_TURNS, 1);
    }

}

void SetupPetSkills(CMobEntity* PMob)
{
    int16 skillListId = 0;
    // same mob can spawn as different families
    // can't set this from the database
    switch(PMob->m_Family)
    {
        case 383: // ifrit
            skillListId = 715;
            break;
        case 388: // titan
            skillListId = 716;
            break;
        case 384: // levi
            skillListId = 717;
            break;
        case 382: //garuda
            skillListId = 718;
            break;
        case 387: // shiva
            skillListId = 719;
            break;
        case 386: // ramuh
            skillListId = 720;
            break;
        case 379: // carbuncle
            skillListId = 721;
            break;
    }

    if (skillListId != 0)
    {
        PMob->setMobMod(MOBMOD_SKILL_LIST, skillListId);
    }
}

void SetupDynamisMob(CMobEntity* PMob)
{
    JOBTYPE mJob = PMob->GetMJob();

    // no gil drop and no mugging!
    PMob->setMobMod(MOBMOD_GIL_MAX, -1);
    PMob->setMobMod(MOBMOD_MUG_GIL, -1);
    PMob->setMobMod(MOBMOD_2HOUR_PROC, 80);

    // used for dynamis stat-spawned mobs
    PMob->m_StatPoppedMobs = false;

    // dynamis mobs have true sight
    if(PMob->m_Aggro & AGGRO_DETECT_SIGHT)
    {
        PMob->m_Aggro |= AGGRO_DETECT_TRUESIGHT;
    }

    if(PMob->m_Aggro & AGGRO_DETECT_HEARING)
    {
        PMob->m_Aggro |= AGGRO_DETECT_TRUEHEARING;
    }

    // Hydra's and beastmen can 2 hour
    if(PMob->m_EcoSystem == SYSTEM_BEASTMEN ||
            PMob->m_EcoSystem == SYSTEM_UNDEAD)
    {
        PMob->setMobMod(MOBMOD_MAIN_2HOUR, 1);
    }

    // boost dynamis mobs weapon damage
    PMob->setMobMod(MOBMOD_WEAPON_BONUS, 135);
    PMob->m_Weapons[SLOT_MAIN]->setDamage(GetWeaponDamage(PMob));

    // never despawn
    PMob->SetDespawnTime(0s);
    PMob->setMobMod(MOBMOD_NO_DESPAWN, 1);

    // do not roam around
    PMob->m_roamFlags |= ROAMFLAG_EVENT;
    PMob->m_maxRoamDistance = 0.5f;

    // job resist traits are much more powerful in dynamis
    // according to wiki
    for(auto&& PTrait : PMob->TraitList)
    {
        uint16 type = PTrait->getMod();

        if(type >= 240 && type <= 255)
        {
            // give mob a total of x4 the regular rate
            PMob->addModifier(type, PTrait->getValue() * 3);
        }
    }
}

void SetupBattlefieldMob(CMobEntity* PMob)
{
    PMob->setMobMod(MOBMOD_NO_DESPAWN, 1);

    // Battlefield mobs don't drop gil
    PMob->setMobMod(MOBMOD_GIL_MAX, -1);
    PMob->setMobMod(MOBMOD_MUG_GIL, -1);

    // never despawn
    PMob->SetDespawnTime(0s);
    // do not roam around
    PMob->m_roamFlags |= ROAMFLAG_EVENT;
    PMob->m_maxRoamDistance = 0.5f;

    if((PMob->m_bcnmID != 864) && (PMob->m_bcnmID != 704) && (PMob->m_bcnmID != 706))
    {
        // bcnmID 864 (desires of emptiness), 704 (darkness named), and 706 (waking dreams) don't superlink
        // force all mobs in same instance to superlink
        // plus one in case id is zero
        PMob->setMobMod(MOBMOD_SUPERLINK, PMob->m_battlefieldID);
    }

}

void SetupDungeonMob(CMobEntity* PMob)
{
}

void SetupEventMob(CMobEntity* PMob)
{
    // event mob types will always have custom roaming
    PMob->m_roamFlags |= ROAMFLAG_EVENT;
    PMob->m_maxRoamDistance = 0.5f; // always go back to spawn

    PMob->setMobMod(MOBMOD_NO_DESPAWN, 1);
}

void SetupNMMob(CMobEntity* PMob)
{
    JOBTYPE mJob = PMob->GetMJob();
    uint8 mLvl = PMob->GetMLevel();

    PMob->setMobMod(MOBMOD_NO_DESPAWN, 1);
    // enmity range is larger
    PMob->m_enmityRange = 28;

    // NMs cure earlier
    PMob->defaultMobMod(MOBMOD_HP_HEAL_CHANCE, 50);
    PMob->defaultMobMod(MOBMOD_HEAL_CHANCE, 40);

    // give a gil bonus if accurate value was not set
    if (PMob->getMobMod(MOBMOD_GIL_MAX) == 0)
    {
        PMob->defaultMobMod(MOBMOD_GIL_BONUS, 100);
    }

    if(mLvl >= 25)
    {
        if(mJob == JOB_NIN)
        {
            PMob->setMobMod(MOBMOD_DUAL_WIELD, 1);
        }

        if(mJob == JOB_WHM)
        {
            // whm nms have stronger regen effect
            PMob->addModifier(MOD_REGEN, mLvl/4);
        }

        // add two hours
        if(PMob->m_EcoSystem == SYSTEM_BEASTMEN ||
                PMob->m_EcoSystem == SYSTEM_HUMANOID)
        {
            PMob->defaultMobMod(MOBMOD_MAIN_2HOUR, 1);
        }
    }

}

void SetupMaat(CMobEntity* PMob)
{
    // Should swing normally when not a mnk
    PMob->m_Weapons[SLOT_MAIN]->setDelay((240*1000)/60);

    switch(PMob->GetMJob()){
        case JOB_NIN:
            PMob->setMobMod(MOBMOD_DUAL_WIELD, 1);
            PMob->m_Weapons[SLOT_MAIN]->resetDelay();
            PMob->setMobMod(MOBMOD_SPECIAL_SKILL, 0);
            break;
        case JOB_DRK:
        case JOB_PLD:
            // Give shield bash
            PMob->setMobMod(MOBMOD_SPECIAL_SKILL, 1036);
            PMob->setMobMod(MOBMOD_SPECIAL_COOL, 50);
            PMob->setMobMod(MOBMOD_SPECIAL_DELAY, 40);
            break;
        case JOB_BST:
            // Call beast skill
            PMob->setMobMod(MOBMOD_SPECIAL_SKILL, 1017);
            PMob->setMobMod(MOBMOD_SPECIAL_COOL, 50);
            break;
    }
}

void RecalculateSpellContainer(CMobEntity* PMob)
{
	// clear spell list
	PMob->SpellContainer->ClearSpells();

	//insert the rest of the spells
	for (std::vector<MobSpell_t>::iterator it = PMob->m_SpellListContainer->m_spellList.begin(); it != PMob->m_SpellListContainer->m_spellList.end(); ++it)
	{
		if (PMob->GetMLevel() >= (*it).min_level && PMob->GetMLevel() <= (*it).max_level)
		{
			PMob->SpellContainer->AddSpell((*it).spellId);
		}
	}
}

/* Gets the available spells for the specified monster.
 */
void GetAvailableSpells(CMobEntity* PMob) {
	//make sure the mob actually has a spell list
	if (PMob->m_SpellListContainer == nullptr)
	{
		return;
	}

	// catch all non-defaulted spell chances
        PMob->defaultMobMod(MOBMOD_MAGIC_COOL, 35);
	PMob->defaultMobMod(MOBMOD_GA_CHANCE, 35);
	PMob->defaultMobMod(MOBMOD_NA_CHANCE, 40);
	PMob->defaultMobMod(MOBMOD_BUFF_CHANCE, 35);
	PMob->defaultMobMod(MOBMOD_HEAL_CHANCE, 40);
	PMob->defaultMobMod(MOBMOD_HP_HEAL_CHANCE, 40);

	RecalculateSpellContainer(PMob);

	// make sure mob has mp to cast spells
	if(PMob->health.maxmp == 0 && PMob->SpellContainer != nullptr && PMob->SpellContainer->HasMPSpells())
	{
		ShowError("mobutils::CalculateStats Mob (%u) has no mp for casting spells!\n", PMob->id);
	}
}

void SetSpellList(CMobEntity* PMob, uint16 spellList)
{
	PMob->m_SpellListContainer = mobSpellList::GetMobSpellList(spellList);
	RecalculateSpellContainer(PMob);
}

void InitializeMob(CMobEntity* PMob, CZone* PZone)
{
    // add special mob mods

    // this only has to be added once
    AddCustomMods(PMob);

    PMob->m_Immunity |= PMob->getMobMod(MOBMOD_IMMUNITY);

    PMob->defaultMobMod(MOBMOD_SKILL_LIST, PMob->m_MobSkillList);
    PMob->defaultMobMod(MOBMOD_LINK_RADIUS, 10);
    PMob->defaultMobMod(MOBMOD_TP_USE_CHANCE, 30);
    PMob->defaultMobMod(MOBMOD_2HOUR_PROC, 60);
    PMob->defaultMobMod(MOBMOD_SIGHT_RANGE, CMobEntity::sight_range);
    PMob->defaultMobMod(MOBMOD_SOUND_RANGE, CMobEntity::sound_range);


    // Killer Effect
    switch (PMob->m_EcoSystem)
      {
        case SYSTEM_AMORPH:   PMob->addModifier(MOD_BIRD_KILLER,     5); break;
        case SYSTEM_AQUAN:    PMob->addModifier(MOD_AMORPH_KILLER,   5); break;
        case SYSTEM_ARCANA:   PMob->addModifier(MOD_UNDEAD_KILLER,   5); break;
        case SYSTEM_BEAST:    PMob->addModifier(MOD_LIZARD_KILLER,   5); break;
        case SYSTEM_BIRD:     PMob->addModifier(MOD_AQUAN_KILLER,    5); break;
        case SYSTEM_DEMON:    PMob->addModifier(MOD_DRAGON_KILLER,   5); break;
        case SYSTEM_DRAGON:   PMob->addModifier(MOD_DEMON_KILLER,    5); break;
        case SYSTEM_LIZARD:   PMob->addModifier(MOD_VERMIN_KILLER,   5); break;
        case SYSTEM_LUMINION: PMob->addModifier(MOD_LUMORIAN_KILLER, 5); break;
        case SYSTEM_LUMORIAN: PMob->addModifier(MOD_LUMINION_KILLER, 5); break;
        case SYSTEM_PLANTOID: PMob->addModifier(MOD_BEAST_KILLER,    5); break;
        case SYSTEM_UNDEAD:   PMob->addModifier(MOD_ARCANA_KILLER,   5); break;
        case SYSTEM_VERMIN:   PMob->addModifier(MOD_PLANTOID_KILLER, 5); break;
      }

    if (PMob->m_maxLevel == 0 && PMob->m_minLevel == 0)
    {
        if (PMob->getZone() >= 1 && PMob->getZone() <= 252)
        {
            ShowError("Mob %s level is 0! zoneid %d, poolid %d\n", PMob->GetName(), PMob->getZone(), PMob->m_Pool);
        }
    }

}

/*
Loads up custom mob mods from mob_pool_mods and mob_family_mods table. This will allow you to customize
a mobs regen rate, magic defense, triple attack rate from a table instead of hardcoding it.

Usage:

	Evil weapons have a magic defense boost. So pop that into mob_family_mods table.
	Goblin Diggers have a vermin killer trait, so find its poolid and put it in mod_pool_mods table.

*/
void LoadCustomMods()
{

	// load family mods
	const int8 QueryFamilyMods[] = "SELECT familyid, modid, value, is_mob_mod FROM mob_family_mods;";

    int32 ret = Sql_Query(SqlHandle, QueryFamilyMods);

	if(ret != SQL_ERROR && Sql_NumRows(SqlHandle) != 0)
	{
		while(Sql_NextRow(SqlHandle) == SQL_SUCCESS)
		{
			ModsList_t* familyMods = GetMobFamilyMods(Sql_GetUIntData(SqlHandle,0), true);

			CModifier* mod = new CModifier(Sql_GetUIntData(SqlHandle,1));
			mod->setModAmount(Sql_GetIntData(SqlHandle,2));

			int8 isMobMod = Sql_GetIntData(SqlHandle,3);
			if(isMobMod == 1)
			{
				familyMods->mobMods.push_back(mod);
			}
			else
			{
				familyMods->mods.push_back(mod);
			}
		}
	}

	// load pool mods
	const int8 QueryPoolMods[] = "SELECT poolid, modid, value, is_mob_mod FROM mob_pool_mods;";

    ret = Sql_Query(SqlHandle, QueryPoolMods);

	if(ret != SQL_ERROR && Sql_NumRows(SqlHandle) != 0)
	{
		while(Sql_NextRow(SqlHandle) == SQL_SUCCESS)
		{
			uint16 pool = Sql_GetUIntData(SqlHandle,0);
			ModsList_t* poolMods = GetMobPoolMods(pool, true);

			uint16 id = Sql_GetUIntData(SqlHandle,1);


			CModifier* mod = new CModifier(id);
			mod->setModAmount(Sql_GetUIntData(SqlHandle,2));

			int8 isMobMod = Sql_GetIntData(SqlHandle,3);
			if(isMobMod == 1)
			{
				poolMods->mobMods.push_back(mod);
			}
			else
			{
				poolMods->mods.push_back(mod);
			}
		}
	}

	// load spawn mods
	const int8 QuerySpawnMods[] = "SELECT mobid, modid, value, is_mob_mod FROM mob_spawn_mods;";

    ret = Sql_Query(SqlHandle, QuerySpawnMods);

	if(ret != SQL_ERROR && Sql_NumRows(SqlHandle) != 0)
	{
		while(Sql_NextRow(SqlHandle) == SQL_SUCCESS)
		{
			ModsList_t* spawnMods = GetMobSpawnMods(Sql_GetUIntData(SqlHandle,0), true);

			CModifier* mod = new CModifier(Sql_GetUIntData(SqlHandle,1));
			mod->setModAmount(Sql_GetUIntData(SqlHandle,2));

			int8 isMobMod = Sql_GetIntData(SqlHandle,3);
			if(isMobMod == 1)
			{
				spawnMods->mobMods.push_back(mod);
			}
			else
			{
				spawnMods->mods.push_back(mod);
			}
		}
	}
}

ModsList_t* GetMobFamilyMods(uint16 familyId, bool create)
{
	if(mobFamilyModsList[familyId])
	{
		return mobFamilyModsList[familyId];
	}

	if(create)
	{
		// create new one
		ModsList_t* mods = new ModsList_t;
		mods->id = familyId;

		mobFamilyModsList[familyId] = mods;

		return mods;
	}

	return nullptr;
}

ModsList_t* GetMobPoolMods(uint32 poolId, bool create)
{
	if(mobPoolModsList[poolId])
	{
		return mobPoolModsList[poolId];
	}

	if(create)
	{
		// create new one
		ModsList_t* mods = new ModsList_t;
		mods->id = poolId;

		mobPoolModsList[poolId] = mods;

		return mods;
	}

	return nullptr;
}

ModsList_t* GetMobSpawnMods(uint32 mobId, bool create)
{
	if(mobSpawnModsList[mobId])
	{
		return mobSpawnModsList[mobId];
	}

	if(create)
	{
		// create new one
		ModsList_t* mods = new ModsList_t;
		mods->id = mobId;

		mobSpawnModsList[mobId] = mods;

		return mods;
	}

	return nullptr;
}

void AddCustomMods(CMobEntity* PMob)
{

	// find my families custom mods
	ModsList_t* PFamilyMods = GetMobFamilyMods(PMob->m_Family);

	if(PFamilyMods != nullptr)
	{
		// add them
		for(std::vector<CModifier*>::iterator it = PFamilyMods->mods.begin(); it != PFamilyMods->mods.end() ; ++it)
		{
			PMob->addModifier((*it)->getModID(), (*it)->getModAmount());
		}

		for(std::vector<CModifier*>::iterator it = PFamilyMods->mobMods.begin(); it != PFamilyMods->mobMods.end() ; ++it)
		{
			PMob->setMobMod((*it)->getModID(), (*it)->getModAmount());
		}
	}

	// find my pools custom mods
	ModsList_t* PPoolMods = GetMobPoolMods(PMob->m_Pool);

	if(PPoolMods != nullptr)
	{
		// add them
		for(std::vector<CModifier*>::iterator it = PPoolMods->mods.begin(); it != PPoolMods->mods.end() ; ++it)
		{
			PMob->addModifier((*it)->getModID(), (*it)->getModAmount());
		}

		for(std::vector<CModifier*>::iterator it = PPoolMods->mobMods.begin(); it != PPoolMods->mobMods.end() ; ++it)
		{
			PMob->setMobMod((*it)->getModID(), (*it)->getModAmount());
		}
	}

	// find my pools custom mods
	ModsList_t* PSpawnMods = GetMobSpawnMods(PMob->id);

	if(PSpawnMods != nullptr)
	{
		// add them
		for(std::vector<CModifier*>::iterator it = PSpawnMods->mods.begin(); it != PSpawnMods->mods.end() ; ++it)
		{
			PMob->addModifier((*it)->getModID(), (*it)->getModAmount());
		}

		for(std::vector<CModifier*>::iterator it = PSpawnMods->mobMods.begin(); it != PSpawnMods->mobMods.end() ; ++it)
		{
			PMob->setMobMod((*it)->getModID(), (*it)->getModAmount());
		}
	}
}

void InitializeMaat(CMobEntity* PMob, JOBTYPE job)
{
    //set job based on characters job
    PMob->SetMJob(job);

    // give him a spell list based on job
    uint16 spellList = 0;

    switch(job){
        case JOB_WHM:
            spellList = 1;
            break;
        case JOB_BLM:
            spellList = 2;
            break;
        case JOB_RDM:
            spellList = 3;
            break;
        case JOB_PLD:
            spellList = 4;
            break;
        case JOB_DRK:
            spellList = 5;
            break;
        case JOB_BRD:
            spellList = 6;
            break;
        case JOB_NIN:
            spellList = 7;
            break;
        case JOB_BLU:
            spellList = 8;
            break;
        case JOB_SMN:
            spellList = 141;
            break;
    }

    PMob->m_SpellListContainer = mobSpellList::GetMobSpellList(spellList);

    PMob->m_DropID = 4485; //Give Maat his stealable Warp Scroll 
}

CMobEntity* InstantiateAlly(uint32 groupid, uint16 zoneID, CInstance* instance)
{
	const int8* Query =
		"SELECT zoneid, name, \
		respawntime, spawntype, dropid, mob_groups.HP, mob_groups.MP, minLevel, maxLevel, \
		modelid, mJob, sJob, cmbSkill, cmbDmgMult, cmbDelay, behavior, links, mobType, immunity, \
		systemid, mobsize, speed, \
		STR, DEX, VIT, AGI, `INT`, MND, CHR, EVA, DEF, \
		Slash, Pierce, H2H, Impact, \
		Fire, Ice, Wind, Earth, Lightning, Water, Light, Dark, Element, \
		mob_pools.familyid, name_prefix, flags, animationsub, \
		(mob_family_system.HP / 100), (mob_family_system.MP / 100), hasSpellScript, spellList, ATT, ACC, mob_groups.poolid, \
		allegiance, namevis, aggro, mob_pools.skill_list_id \
		FROM mob_groups INNER JOIN mob_pools ON mob_groups.poolid = mob_pools.poolid \
		INNER JOIN mob_family_system ON mob_pools.familyid = mob_family_system.familyid \
		WHERE mob_groups.groupid = %u";

	int32 ret = Sql_Query(SqlHandle, Query, groupid);

	CMobEntity* PMob = nullptr;

	if (ret != SQL_ERROR && Sql_NumRows(SqlHandle) != 0)
	{
		if (Sql_NextRow(SqlHandle) == SQL_SUCCESS)
		{
			PMob = new CMobEntity;
            PMob->PInstance = instance;

			PMob->name.insert(0, Sql_GetData(SqlHandle, 1));

			PMob->m_RespawnTime = Sql_GetUIntData(SqlHandle, 2) * 1000;
			PMob->m_SpawnType = (SPAWNTYPE)Sql_GetUIntData(SqlHandle, 3);
			PMob->m_DropID = Sql_GetUIntData(SqlHandle, 4);

			PMob->HPmodifier = (uint32)Sql_GetIntData(SqlHandle, 5);
			PMob->MPmodifier = (uint32)Sql_GetIntData(SqlHandle, 6);

			PMob->m_minLevel = (uint8)Sql_GetIntData(SqlHandle, 7);
			PMob->m_maxLevel = (uint8)Sql_GetIntData(SqlHandle, 8);

			memcpy(&PMob->look, Sql_GetData(SqlHandle, 9), 23);

			PMob->SetMJob(Sql_GetIntData(SqlHandle, 10));
			PMob->SetSJob(Sql_GetIntData(SqlHandle, 11));

			PMob->m_Weapons[SLOT_MAIN]->setMaxHit(1);
			PMob->m_Weapons[SLOT_MAIN]->setSkillType(Sql_GetIntData(SqlHandle, 12));
			PMob->m_dmgMult = Sql_GetUIntData(SqlHandle, 13);
			PMob->m_Weapons[SLOT_MAIN]->setDelay((Sql_GetIntData(SqlHandle, 14) * 1000) / 60);
			PMob->m_Weapons[SLOT_MAIN]->setBaseDelay((Sql_GetIntData(SqlHandle, 14) * 1000) / 60);

			PMob->m_Behaviour = (uint16)Sql_GetIntData(SqlHandle, 15);
			PMob->m_Link = (uint8)Sql_GetIntData(SqlHandle, 16);
			PMob->m_Type = (uint8)Sql_GetIntData(SqlHandle, 17);
			PMob->m_Immunity = (IMMUNITY)Sql_GetIntData(SqlHandle, 18);
			PMob->m_EcoSystem = (ECOSYSTEM)Sql_GetIntData(SqlHandle, 19);
			PMob->m_ModelSize += (uint8)Sql_GetIntData(SqlHandle, 10);

			PMob->speed = (uint8)Sql_GetIntData(SqlHandle, 21);
			PMob->speedsub = (uint8)Sql_GetIntData(SqlHandle, 21);

			/*if(PMob->speed != 0)
			{
			PMob->speed += map_config.speed_mod;
			// whats this for?
			PMob->speedsub += map_config.speed_mod;
			}*/

			PMob->strRank = (uint8)Sql_GetIntData(SqlHandle, 22);
			PMob->dexRank = (uint8)Sql_GetIntData(SqlHandle, 23);
			PMob->vitRank = (uint8)Sql_GetIntData(SqlHandle, 24);
			PMob->agiRank = (uint8)Sql_GetIntData(SqlHandle, 25);
			PMob->intRank = (uint8)Sql_GetIntData(SqlHandle, 26);
			PMob->mndRank = (uint8)Sql_GetIntData(SqlHandle, 27);
			PMob->chrRank = (uint8)Sql_GetIntData(SqlHandle, 28);
			PMob->evaRank = (uint8)Sql_GetIntData(SqlHandle, 29);
			PMob->defRank = (uint8)Sql_GetIntData(SqlHandle, 30);
			PMob->attRank = (uint8)Sql_GetIntData(SqlHandle, 52);
			PMob->accRank = (uint8)Sql_GetIntData(SqlHandle, 53);

			PMob->setModifier(MOD_SLASHRES, (uint16)(Sql_GetFloatData(SqlHandle, 31) * 1000));
			PMob->setModifier(MOD_PIERCERES, (uint16)(Sql_GetFloatData(SqlHandle, 32) * 1000));
			PMob->setModifier(MOD_HTHRES, (uint16)(Sql_GetFloatData(SqlHandle, 33) * 1000));
			PMob->setModifier(MOD_IMPACTRES, (uint16)(Sql_GetFloatData(SqlHandle, 34) * 1000));

			PMob->setModifier(MOD_FIRERES, (int16)((Sql_GetFloatData(SqlHandle, 35) - 1) * -100)); // These are stored as floating percentages
			PMob->setModifier(MOD_ICERES, (int16)((Sql_GetFloatData(SqlHandle, 36) - 1) * -100)); // and need to be adjusted into modifier units.
			PMob->setModifier(MOD_WINDRES, (int16)((Sql_GetFloatData(SqlHandle, 37) - 1) * -100)); // Higher RES = lower damage.
			PMob->setModifier(MOD_EARTHRES, (int16)((Sql_GetFloatData(SqlHandle, 38) - 1) * -100)); // Negatives signify lower resist chance.
			PMob->setModifier(MOD_THUNDERRES, (int16)((Sql_GetFloatData(SqlHandle, 39) - 1) * -100)); // Positives signify increased resist chance.
			PMob->setModifier(MOD_WATERRES, (int16)((Sql_GetFloatData(SqlHandle, 40) - 1) * -100));
			PMob->setModifier(MOD_LIGHTRES, (int16)((Sql_GetFloatData(SqlHandle, 41) - 1) * -100));
			PMob->setModifier(MOD_DARKRES, (int16)((Sql_GetFloatData(SqlHandle, 42) - 1) * -100));

			PMob->m_Element = (uint8)Sql_GetIntData(SqlHandle, 43);
			PMob->m_Family = (uint16)Sql_GetIntData(SqlHandle, 44);
			PMob->m_name_prefix = (uint8)Sql_GetIntData(SqlHandle, 45);
			PMob->m_flags = (uint32)Sql_GetIntData(SqlHandle, 46);

			//Special sub animation for Mob (yovra, jailer of love, phuabo)
			// yovra 1: en hauteur, 2: en bas, 3: en haut
			// phuabo 1: sous l'eau, 2: sort de l'eau, 3: rentre dans l'eau
			PMob->animationsub = (uint32)Sql_GetIntData(SqlHandle, 47);

			// Setup HP / MP Stat Percentage Boost
			PMob->HPscale = Sql_GetFloatData(SqlHandle, 48);
			PMob->MPscale = Sql_GetFloatData(SqlHandle, 49);

			// Check if we should be looking up scripts for this mob
			PMob->m_HasSpellScript = (uint8)Sql_GetIntData(SqlHandle, 50);

			PMob->m_SpellListContainer = mobSpellList::GetMobSpellList(Sql_GetIntData(SqlHandle, 51));

			PMob->m_Pool = Sql_GetUIntData(SqlHandle, 54);

			PMob->allegiance = Sql_GetUIntData(SqlHandle, 55);
			PMob->namevis = Sql_GetUIntData(SqlHandle, 56);
			PMob->m_Aggro = Sql_GetUIntData(SqlHandle, 57);
			PMob->m_MobSkillList = Sql_GetUIntData(SqlHandle, 58);

			// must be here first to define mobmods
			mobutils::InitializeMob(PMob, zoneutils::GetZone(zoneID));

			zoneutils::GetZone(zoneID)->InsertPET(PMob);

			luautils::OnMobInitialize(PMob);

			PMob->saveModifiers();
			PMob->saveMobModifiers();
		}
	}
	return PMob;
}

void WeaknessTrigger(CBaseEntity* PTarget, WeaknessType level)
{
    uint16 animationID = 0;
    switch (level)
    {
    case WeaknessType::RED:
        animationID = 1806;
        break;
    case WeaknessType::YELLOW:
        animationID = 1807;
        break;
    case WeaknessType::BLUE:
        animationID = 1808;
        break;
    case WeaknessType::WHITE:
        animationID = 1946;
        break;
    }
    action_t action;
    action.actiontype = ACTION_MOBABILITY_FINISH;
    action.id = PTarget->id;
    actionList_t& list = action.getNewActionList();
    list.ActionTargetID = PTarget->id;
    actionTarget_t& target = list.getNewActionTarget();
    target.animation = animationID;
    target.param = 2582;
    PTarget->loc.zone->PushPacket(PTarget, CHAR_INRANGE, new CActionPacket(action));
}

}; // namespace mobutils
