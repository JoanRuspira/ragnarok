#include "KnightSkillAtkRatioCalculator.h"
#include <cstring>
#include "skill.hpp"
#include "status.hpp"

/**
 * ATK ratio calculater for Knight skills.
 * @param base_lv : player base level
 * @param skill_id : skill id
 * @param skill_lv : skill level
 * @param int_ : player's int
 * @param weapon_element : equiped weapon element
 */
KnightSkillAtkRatioCalculator::KnightSkillAtkRatioCalculator(int base_lv, int skill_id, int skill_lv, int int_, int weapon_element)
{
	_base_lv = base_lv;
	_skill_id = skill_id;
	_skill_lv = skill_lv;
	_int = int_;
	_weapon_element = weapon_element;
}


int KnightSkillAtkRatioCalculator::calculate_skill_atk_ratio()
{
	switch (_skill_id) {
	case KN_BOWLINGBASH:
		return calculate_bowling_bash_atk_ratio();
		break;
	case KN_CHARGEATK:
		return calculate_charge_attack_atk_ratio();
		break;
	case KN_SPEARBOOMERANG:
		return calculate_spear_boomerang_atk_ratio();
		break;
	case KN_BRANDISHSPEAR:
		return calculate_brandish_spear_atk_ratio();
		break;
	case KN_PIERCE:
		return calculate_pierce_atk_ratio();
		break;
	case LK_HEADCRUSH:
		return calculate_head_crush_atk_ratio();
		break;
	case LK_SPIRALPIERCE:
		return calculate_spiral_pierce_atk_ratio();
		break;
	case RK_HUNDREDSPEAR:
		return calculate_hundred_spear_atk_ratio();
		break;
	case PA_SHIELDCHAIN:
		return calculate_shield_chain_atk_ratio();
		break;
	case RK_WINDCUTTER:
		return calculate_wind_cutter_atk_ratio();
		break;
	case RK_SONICWAVE:
		return calculate_sonic_wave_atk_ratio();
		break;
	case RK_IGNITIONBREAK:
		return calculate_ignition_break_atk_ratio();
		break;
	default:
		return 0;
		break;
	}
}

int KnightSkillAtkRatioCalculator::calculate_spear_boomerang_atk_ratio()
{
	return (100 + (_base_lv / 5)) * _skill_lv;
}

int KnightSkillAtkRatioCalculator::calculate_brandish_spear_atk_ratio()
{
	return (_base_lv / 6) * _skill_lv;
}

int KnightSkillAtkRatioCalculator::calculate_pierce_atk_ratio()
{
	return (10 + (_base_lv / 10)) * _skill_lv;
}

int KnightSkillAtkRatioCalculator::calculate_hundred_spear_atk_ratio()
{
	return 550 + ((_base_lv + 20) * _skill_lv);
}

int KnightSkillAtkRatioCalculator::calculate_bowling_bash_atk_ratio()
{
	return -20 + (_base_lv / 3) * _skill_lv;
}

int KnightSkillAtkRatioCalculator::calculate_charge_attack_atk_ratio()
{
	return (10 * _base_lv) * 3;
}

int KnightSkillAtkRatioCalculator::calculate_head_crush_atk_ratio()
{
	return (_base_lv / 4) * _skill_lv;
}

int KnightSkillAtkRatioCalculator::calculate_spiral_pierce_atk_ratio()
{
	return 55 + (_base_lv / 3) * _skill_lv;
}

int KnightSkillAtkRatioCalculator::calculate_shield_chain_atk_ratio()
{
	return 400 + _base_lv + 40 * _skill_lv;
}

int KnightSkillAtkRatioCalculator::calculate_wind_cutter_atk_ratio()
{
	return 100 + ((_base_lv + _int) / 1.5 * _skill_lv);
}

int KnightSkillAtkRatioCalculator::calculate_sonic_wave_atk_ratio()
{
	return 200 + ((_base_lv + _int) / 20 * _skill_lv);
}

int KnightSkillAtkRatioCalculator::calculate_ignition_break_atk_ratio()
{
	int skillratio = ((_base_lv + _int) + 60)* _skill_lv;
	skillratio = skillratio * _base_lv / 100;
	// Elemental check, 1.5x damage if your weapon element is fire.
	if (_weapon_element == ELE_FIRE) {
		skillratio += 100 * _skill_lv;
	}
	return skillratio;
}

//sstatus->rhw.ele
		//case RK_DRAGONBREATH:
		//	{
		//		int damagevalue = ((50 + status_get_lv(src) + sstatus->int_) * 4) * skill_lv; //status_get_max_sp(src)

		//		if(status_get_lv(src) > 100)
		//			damagevalue = damagevalue * status_get_lv(src) / 150;
		//		if(sd)
		//			damagevalue = damagevalue * (100 + 5 * (pc_checkskill(sd,RK_DRAGONTRAINING) - 1)) / 100;
		//		ATK_ADD(wd->damage, wd->damage2, damagevalue);
		//		ATK_ADD(wd->weaponAtk, wd->weaponAtk2, damagevalue);
		//		wd->flag |= BF_LONG;
		//	}
		//	break;
