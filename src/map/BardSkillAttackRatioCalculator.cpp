#include "BardSkillAttackRatioCalculator.h"

/**
 * ATK ratio calculater for Swordsman skills.
 * @param base_lv : player base level
 * @param skill_id : skill id
 * @param skill_lv : skill level
 * @param int_ : player's int
 */
int BardSkillAttackRatioCalculator::calculate_skill_atk_ratio(struct block_list* src, struct block_list *target, int base_lv, int skill_id, int skill_lv, struct status_data* sstatus)
{
	switch (skill_id) {
		case BA_MUSICALSTRIKE:
			add_melody_strike_special_effects(target);
			return calculate_melody_strike_atk_ratio(skill_lv);
			break;
        case WM_GREAT_ECHO:
			add_great_echo_special_effects(target);
			return calculate_melody_strike_atk_ratio(skill_lv);
			break;
		default:
			return 0;
			break;
	}
}

void BardSkillAttackRatioCalculator::add_melody_strike_special_effects(struct block_list *target)
{
    clif_specialeffect(target, 1220, AREA);
}

void BardSkillAttackRatioCalculator::add_great_echo_special_effects(struct block_list *target)
{
    clif_specialeffect(target, 1371, AREA);
}

int BardSkillAttackRatioCalculator::calculate_melody_strike_atk_ratio(int skill_lv)
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