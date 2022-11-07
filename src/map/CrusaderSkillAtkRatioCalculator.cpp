#include "CrusaderSkillAtkRatioCalculator.h"
#include <cstring>
#include "skill.hpp"
#include "status.hpp"

/**
 * ATK ratio calculater for Crusader skills.
 * @param base_lv : player base level
 * @param skill_id : skill id
 * @param skill_lv : skill level
 * @param int_ : player's int
 */
CrusaderSkillAtkRatioCalculator::CrusaderSkillAtkRatioCalculator(int base_lv, int skill_id, int skill_lv, int int_)
{
	_base_lv = base_lv;
	_skill_id = skill_id;
	_skill_lv = skill_lv;
	_int = int_;
}

CrusaderSkillAtkRatioCalculator::~CrusaderSkillAtkRatioCalculator()
{
}

int CrusaderSkillAtkRatioCalculator::calculate_skill_atk_ratio()
{
	switch (_skill_id) {
	case CR_HOLYCROSS:
		return calculate_holy_cross_atk_ratio();
		break;
	// case CR_SHIELDCHARGE:
	// 	return calculate_shield_charge_atk_ratio();
	// 	break;
	// case CR_SHIELDBOOMERANG:
	// 	return calculate_shield_boomerang_atk_ratio();
	// 	break;
	// case KN_SPEARBOOMERANG:
	// 	return calculate_spear_boomerang_atk_ratio();
	// 	break;
	// case KN_BRANDISHSPEAR:
	// 	return calculate_brandish_spear_atk_ratio();
	// 	break;
	// case KN_PIERCE:
	// 	return calculate_pierce_atk_ratio();
	// 	break;
	// case PA_SHIELDCHAIN:
	// 	return calculate_shield_chain_atk_ratio();
	// 	break;
	// case PA_SACRIFICE:
	// 	return calculate_sacrifice_atk_ratio();
	// 	break;
	// case RK_HUNDREDSPEAR:
	// 	return calculate_hundred_spear_atk_ratio();
	// 	break;
	default:
		return 0;
		break;
	}
}


int CrusaderSkillAtkRatioCalculator::calculate_holy_cross_atk_ratio()
{
	return ((_base_lv + _int) / 4) * _skill_lv;
}

int CrusaderSkillAtkRatioCalculator::calculate_shield_charge_atk_ratio()
{
	return (60 + (_base_lv / 5)) * _skill_lv;
}

int CrusaderSkillAtkRatioCalculator::calculate_shield_boomerang_atk_ratio()
{
	return (400 + (_base_lv / 5)) * _skill_lv;
}

int CrusaderSkillAtkRatioCalculator::calculate_shield_chain_atk_ratio()
{
	return 400 + _base_lv + 40 * _skill_lv;
}

int CrusaderSkillAtkRatioCalculator::calculate_sacrifice_atk_ratio()
{
	return (300 + _base_lv * _skill_lv) * 5;
}

int CrusaderSkillAtkRatioCalculator::calculate_spear_boomerang_atk_ratio()
{
	return (100 + (_base_lv / 5)) * _skill_lv;
}

int CrusaderSkillAtkRatioCalculator::calculate_brandish_spear_atk_ratio()
{
	return (_base_lv / 6) * _skill_lv;
}

int CrusaderSkillAtkRatioCalculator::calculate_pierce_atk_ratio()
{
	return (10 + (_base_lv / 10)) * _skill_lv;
}

int CrusaderSkillAtkRatioCalculator::calculate_hundred_spear_atk_ratio()
{
	return 550 + ((_base_lv + 20) * _skill_lv);
}
//sd->inventory.u.items_inventory[index].refine
