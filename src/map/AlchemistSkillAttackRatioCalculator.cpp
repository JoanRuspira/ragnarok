#include "AlchemistSkillAttackRatioCalculator.h"

/**
 * ATK ratio calculater for Swordsman skills.
 * @param base_lv : player base level
 * @param skill_id : skill id
 * @param skill_lv : skill level
 * @param int_ : player's int
 */
int AlchemistSkillAttackRatioCalculator::calculate_skill_atk_ratio(struct block_list* src, struct block_list *target, int base_lv, int skill_id, int skill_lv, struct status_data* sstatus)
{
	switch (skill_id) {
		case HM_BASILISK_1:
		case AM_EL_ACTION:
			return calculate_action_1_attack(skill_lv);
			break;
		default:
			return 0;
			break;
	}
}

int AlchemistSkillAttackRatioCalculator::calculate_action_1_attack(int skill_lv)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 20;
			break;
		case 2:
			ratio = 230;
			break;
		case 3:
			ratio = 340;
			break;
		case 4:
			ratio = 450;
			break;
		case 5:
			ratio = 560;
			break;
		}
	return ratio;
}

