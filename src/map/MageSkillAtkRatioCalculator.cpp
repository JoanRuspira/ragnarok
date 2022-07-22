#include "MageSkillAttackRatioCalculator.h"
#include <cstring>
#include "skill.hpp"
#include "status.hpp"

/**
 * ATK ratio calculater for Mage skills.
 * @param base_lv : player base level
 * @param skill_id : skill id
 * @param skill_lv : skill level
 * @param int_ : player's int
 */
int MageSkillAtkRatioCalculator::calculate_skill_atk_ratio(int base_lv, int skill_id, int skill_lv)
{
	switch (skill_id) {
		case WZ_EARTHSPIKE:
		case MG_LIGHTNINGBOLT:
		case MG_COLDBOLT:
		case MG_FIREBOLT:
			return calculate_bolt_attack(skill_lv);
			break;
		default:
			return 0;
			break;
	}
}

int MageSkillAtkRatioCalculator::calculate_bolt_attack(int skill_lv)
{
	int ratio = 40;
	return ratio;
}

