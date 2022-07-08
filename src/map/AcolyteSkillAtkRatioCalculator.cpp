#include "AcolyteSkillAtkRatioCalculator.h"

/**
 * ATK ratio calculater for Swordsman skills.
 * @param base_lv : player base level
 * @param skill_id : skill id
 * @param skill_lv : skill level
 * @param int_ : player's int
 */
int AcolyteSkillAtkRatioCalculator::calculate_skill_atk_ratio(struct block_list* src, struct block_list *target, int base_lv, int skill_id, int skill_lv)
{
	switch (skill_id) {
		case MG_NAPALMBEAT:
			add_sacred_wave_special_effects(target);
			return calculate_sacred_wave_atk_ratio(skill_lv);
			break;
		default:
			return 0;
			break;
	}
}

void AcolyteSkillAtkRatioCalculator::add_sacred_wave_special_effects(struct block_list *target)
{
    clif_specialeffect(target, EF_MOCHI, AREA);
	clif_specialeffect(target, EF_RK_LUXANIMA, AREA);
}


int AcolyteSkillAtkRatioCalculator::calculate_sacred_wave_atk_ratio(int skill_lv)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 30;
			break;
		case 2:
			ratio = 100;
			break;
		case 3:
			ratio = 180;
			break;
		case 4:
			ratio = 240;
			break;
		case 5:
			ratio = 300;
			break;
		}
	return ratio;
}
