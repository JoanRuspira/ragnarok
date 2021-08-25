#include "ArcherSkillAtkRatioCalculator.h"
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
int ArcherSkillAtkRatioCalculator::calculate_skill_atk_ratio(int base_lv, int skill_id, int skill_lv)
{
	switch (skill_id) {
		case AC_DOUBLE:
			return calculate_double_strafe_atk_ratio(base_lv, skill_lv);
			break;
		case AC_SHOWER:
			return calculate_arrow_shower_atk_ratio(base_lv, skill_lv);
			break;
		default:
			return 0;
			break;
	}
}

int ArcherSkillAtkRatioCalculator::calculate_double_strafe_atk_ratio(int base_lv, int skill_lv)
{
	return (75 * skill_lv) + (base_lv);
}

int ArcherSkillAtkRatioCalculator::calculate_arrow_shower_atk_ratio(int base_lv, int skill_lv)
{
	return (100 * skill_lv) + (base_lv);
}
