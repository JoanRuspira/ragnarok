#include "MonkSkillAttackRatioCalculator.h"

/**
 * ATK ratio calculater for Swordsman skills.
 * @param base_lv : player base level
 * @param skill_id : skill id
 * @param skill_lv : skill level
 * @param int_ : player's int
 */
int MonkSkillAttackRatioCalculator::calculate_skill_atk_ratio(struct block_list* src, struct block_list *target, int base_lv, int skill_id, int skill_lv, struct status_data* sstatus, bool revealed_hidden_enemy, map_session_data *sd)
{
	switch (skill_id) {
		case MO_FINGEROFFENSIVE:
			return calculate_throw_spirit_sphere_atk_ratio(skill_lv);
        case MO_INVESTIGATE:
            return calculate_occult_impact(skill_lv, status_get_def(target));
		case SR_EARTHSHAKER:
			return calculate_ground_shaker_atk_ratio(skill_lv, sstatus->str);
		case MO_TRIPLEATTACK:
			return calculate_raging_triple_blow_atk_ratio(skill_lv);
		case MO_CHAINCOMBO:
			{
				bool is_using_mace = false;
				if (sd && sd->status.weapon == W_MACE || sd->status.weapon == W_2HMACE) {
					is_using_mace = true;
				}
				return calculate_chain_combo_atk_ratio(skill_lv, is_using_mace);
			}
		case SR_WINDMILL:
			return calculate_circular_fists_atk_ratio(skill_lv, revealed_hidden_enemy);
		default:
			return 0;
			break;
	}
}


int MonkSkillAttackRatioCalculator::calculate_circular_fists_atk_ratio(int skill_lv, bool revealed_hidden_enemy)
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
	return ratio;
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


int MonkSkillAttackRatioCalculator::calculate_ground_shaker_atk_ratio(int skill_lv, int str)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 100;
			break;
		case 2:
			ratio = 250;
			break;
		case 3:
			ratio = 300;
			break;
		case 4:
			ratio = 450;
			break;
		case 5:
			ratio = 500;
			break;
	}
	return ratio + str/3;
}


int MonkSkillAttackRatioCalculator::calculate_raging_triple_blow_atk_ratio(int skill_lv)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 0;
			break;
		case 2:
			ratio = 75;
			break;
		case 3:
			ratio = 175;
			break;
		case 4:
			ratio = 275;
			break;
		case 5:
			ratio = 375;
			break;
	}
	return ratio;
}


int MonkSkillAttackRatioCalculator::calculate_chain_combo_atk_ratio(int skill_lv, bool is_using_mace)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 50;
			break;
		case 2:
			ratio = 200;
			break;
		case 3:
			ratio = 250;
			break;
		case 4:
			ratio = 350;
			break;
		case 5:
			ratio = 450;
			break;
		}
	if (is_using_mace){
		ratio += 150;
	}
	return ratio;
}

