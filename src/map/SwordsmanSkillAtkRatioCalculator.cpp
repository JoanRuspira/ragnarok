#include "SwordsmanSkillAtkRatioCalculator.h"
#include <cstring>
#include "skill.hpp"
#include "status.hpp"

/**
 * ATK ratio calculater for Swordsman skills.
 * @param base_lv : player base level
 * @param skill_id : skill id
 * @param skill_lv : skill level
 * @param int_ : player's int
 */
SwordsmanSkillAtkRatioCalculator::SwordsmanSkillAtkRatioCalculator(int base_lv, int skill_id, int skill_lv, int int_, int weapon_element)
{
	_base_lv = base_lv;
	_skill_id = skill_id;
	_skill_lv = skill_lv;
	_int = int_;
	_weapon_element = weapon_element;
}


int SwordsmanSkillAtkRatioCalculator::calculate_skill_atk_ratio()
{
	switch (_skill_id) {
	case SM_BASH:
		return calculate_bash_atk_ratio();
		break;
	case SM_MAGNUM:
		return calculate_magnum_break_atk_ratio();
		break;
	default:
		return 0;
		break;
	}
}

int SwordsmanSkillAtkRatioCalculator::calculate_bash_atk_ratio()
{
	return 0;
	//return (37 + (_base_lv / 5)) * _skill_lv;
}

int SwordsmanSkillAtkRatioCalculator::calculate_magnum_break_atk_ratio()
{
	return 0;
	//return (_base_lv / 12) * _skill_lv;
}
