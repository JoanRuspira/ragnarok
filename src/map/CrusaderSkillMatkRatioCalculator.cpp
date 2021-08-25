#include "CrusaderSkillMatkRatioCalculator.h"
#include <cstring>
#include "skill.hpp"
#include "status.hpp"

/**
 * MATK ratio calculater for Crusader skills.
 * @param base_lv : player base level
 * @param skill_id : skill id
 * @param skill_lv : skill level
 * @param int_ : player's int
 */
CrusaderSkillMatkRatioCalculator::CrusaderSkillMatkRatioCalculator(int base_lv, int skill_id, int skill_lv, int int_)
{
	_base_lv = base_lv;
	_skill_id = skill_id;
	_skill_lv = skill_lv;
	_int = int_;
}


CrusaderSkillMatkRatioCalculator::~CrusaderSkillMatkRatioCalculator()
{
}

int CrusaderSkillMatkRatioCalculator::calculate_skill_matk_ratio()
{
	switch (_skill_id) {
	case PA_PRESSURE:
		return calculate_pressure_matk_ratio();
		break;
	case LG_RAYOFGENESIS:
		return calculate_genesis_ray_matk_ratio();
		break;
	default:
		return 0;
		break;
	}
}

int CrusaderSkillMatkRatioCalculator::calculate_pressure_matk_ratio()
{
	return ((_base_lv + _int + 300)) * _skill_lv;
}

int CrusaderSkillMatkRatioCalculator::calculate_genesis_ray_matk_ratio()
{
	return ((_base_lv + _int + 40)) * _skill_lv;
}
