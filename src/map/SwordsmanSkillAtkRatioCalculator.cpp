#include "SwordsmanSkillAtkRatioCalculator.h"

/**
 * ATK ratio calculater for Swordsman skills.
 * @param base_lv : player base level
 * @param skill_id : skill id
 * @param skill_lv : skill level
 * @param int_ : player's int
 */
int SwordsmanSkillAtkRatioCalculator::calculate_skill_atk_ratio(int base_lv, int skill_id, int skill_lv)
{
	switch (skill_id) {
		case SM_BASH:
			return calculate_bash_atk_ratio(skill_lv);
			break;
		case SM_MAGNUM:
			return calculate_magnum_break_atk_ratio(skill_lv);
			break;
		case KN_SPEARSTAB:
			return calculate_spear_stab_atk_ratio(skill_lv);
			break;
		default:
			return 0;
			break;
	}
}
/**
 * Additional effect calculater for Swordsman skills.
 * @param base_lv : player base level
 * @param skill_id : skill id
 * @param skill_lv : skill level
 * @param int_ : player's int
 */
void SwordsmanSkillAtkRatioCalculator::calculate_skill_additional_effect(struct block_list* src, struct block_list *bl, int skill_id,
	int skill_lv, int flag, int skill_area_temp[8], t_tick tick)
{
	switch (skill_id) {
		case SM_BASH:
			calculate_bash_additional_effect(src, bl, skill_lv);
			break;
		case KN_SPEARSTAB:
			calculate_spear_stab_additional_effect(src, bl, skill_lv, flag, skill_area_temp, tick);
			break;
	}
}

void SwordsmanSkillAtkRatioCalculator::add_skill_special_effect(struct block_list* src, struct block_list *target, int skill_id)
{
	switch (skill_id) {
		case KN_SPEARSTAB:
			clif_specialeffect(src, EF_SPINEDBODY, AREA);
			clif_specialeffect(target, EF_BASH3D, AREA);
			break;
		}
}

int SwordsmanSkillAtkRatioCalculator::calculate_bash_atk_ratio(int skill_lv)
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

int SwordsmanSkillAtkRatioCalculator::calculate_magnum_break_atk_ratio(int skill_lv)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 0;
			break;
		case 2:
			ratio = 15;
			break;
		case 3:
			ratio = 35;
			break;
		case 4:
			ratio = 55;
			break;
		case 5:
			ratio = 75;
			break;
		}
	return ratio;
}

int SwordsmanSkillAtkRatioCalculator::calculate_spear_stab_atk_ratio(int skill_lv)
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

void SwordsmanSkillAtkRatioCalculator::calculate_bash_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv)
{
	status_change_start(src, bl, SC_STUN, 10000, skill_lv, 0, 0, 0, skill_lv * 400, SCSTART_NONE);
}

void SwordsmanSkillAtkRatioCalculator::calculate_spear_stab_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv, int flag, int skill_area_temp[8], t_tick tick)
{
	if (flag & 1) {
		if (bl->id == skill_area_temp[1]) {
			return;
		}
			
		if (skill_attack(BF_WEAPON, src, src, bl, KN_SPEARSTAB, skill_lv, tick, SD_ANIMATION))
			skill_blown(src, bl, skill_area_temp[2], -1, BLOWN_NONE);
	}
	else {
		status_change_start(src, bl, SC_BLEEDING, 10000, skill_lv, 0, 0, 0, skill_lv * 2000, SCSTART_NONE);
		int x = bl->x, y = bl->y, i, dir;
		dir = map_calc_dir(bl, src->x, src->y);
		skill_area_temp[1] = bl->id;
		skill_area_temp[2] = skill_get_blewcount(KN_SPEARSTAB, skill_lv);
		// all the enemies between the caster and the target are hit, as well as the target
		if (skill_attack(BF_WEAPON, src, src, bl, KN_SPEARSTAB, skill_lv, tick, 0))
			skill_blown(src, bl, skill_area_temp[2], -1, BLOWN_NONE);
		for (i = 0; i < 4; i++) {
			map_foreachincell(skill_area_sub, bl->m, x, y, BL_CHAR,
				src, KN_SPEARSTAB, skill_lv, tick, flag | BCT_ENEMY | 1, skill_castend_damage_id);
			x += dirx[dir];
			y += diry[dir];
		}
	}
}


