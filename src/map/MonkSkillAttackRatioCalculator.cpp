#include "MonkSkillAttackRatioCalculator.h"

/**
 * ATK ratio calculater for Swordsman skills.
 * @param base_lv : player base level
 * @param skill_id : skill id
 * @param skill_lv : skill level
 * @param int_ : player's int
 */
int MonkSkillAttackRatioCalculator::calculate_skill_atk_ratio(struct block_list* src, struct block_list *target, int base_lv, int skill_id, int skill_lv, struct status_data* sstatus, bool revealed_hidden_enemy)
{
	switch (skill_id) {
		case MO_FINGEROFFENSIVE:
			return calculate_throw_spirit_sphere_atk_ratio(skill_lv);
        case MO_INVESTIGATE:
            return calculate_occult_impact(skill_lv, status_get_def(target));
		case SR_EARTHSHAKER:
			return calculate_ground_shaker_atk_ratio(skill_lv, sstatus->str, revealed_hidden_enemy);
		default:
			return 0;
			break;
	}
}


int MonkSkillAttackRatioCalculator::calculate_throw_spirit_sphere_atk_ratio(int skill_lv)
{
	return 50;
}

int MonkSkillAttackRatioCalculator::calculate_occult_impact(int skill_lv, defType defence)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 30 + (defence);
			break;
		case 2:
			ratio = 100 + (defence);
			break;
		case 3:
			ratio = 180 + (defence *2);
			break;
		case 4:
			ratio = 240 + (defence *2);
			break;
		case 5:
			ratio = 300 + (defence *3);
			break;
	}
	return ratio;
}


int MonkSkillAttackRatioCalculator::calculate_ground_shaker_atk_ratio(int skill_lv, int str, bool revealed_hidden_enemy)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 100;
			break;
		case 2:
			ratio = 150;
			break;
		case 3:
			ratio = 200;
			break;
		case 4:
			ratio = 250;
			break;
		case 5:
			ratio = 300;
			break;
	}
	if (revealed_hidden_enemy) {
		ratio += 200;
	}
	return ratio + str/4;
}