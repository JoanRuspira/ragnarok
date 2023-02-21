#include "SageSkillAttackRatioCalculator.h"

/**
 * ATK ratio calculater for Swordsman skills.
 * @param base_lv : player base level
 * @param skill_id : skill id
 * @param skill_lv : skill level
 * @param int_ : player's int
 */
int SageSkillAttackRatioCalculator::calculate_skill_atk_ratio(struct block_list* src, struct block_list *target, int base_lv, int skill_id, int skill_lv, struct status_data* sstatus)
{
	switch (skill_id) {
		case SO_EARTHGRAVE:
			return calculate_earth_spikes_atk_ratio(skill_lv);
			break;
        case WZ_VERMILION:
			return calculate_vermillion_atk_ratio(skill_lv);
			break;
		default:
			return 0;
			break;
	}
}

int SageSkillAttackRatioCalculator::calculate_earth_spikes_atk_ratio(int skill_lv)
{
	return skill_lv*100;
}

int SageSkillAttackRatioCalculator::calculate_vermillion_atk_ratio(int skill_lv)
{
	return skill_lv*100;
}