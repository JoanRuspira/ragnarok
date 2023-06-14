#include "SwordsmanSkillAtkRatioCalculator.h"

/**
 * ATK ratio calculater for Swordsman skills.
 * @param base_lv : player base level
 * @param skill_id : skill id
 * @param skill_lv : skill level
 * @param int_ : player's int
 */
int SwordsmanSkillAtkRatioCalculator::calculate_skill_atk_ratio(struct block_list* src, struct block_list *target, int base_lv, int skill_id, int skill_lv, struct status_data* sstatus)
{
	switch (skill_id) {
		case SM_BASH:
			add_bash_special_effects(target);
			return calculate_bash_atk_ratio(skill_lv);
			break;
		case SM_MAGNUM:
			add_magnum_break_special_effects(src);
			return calculate_magnum_break_atk_ratio(skill_lv, sstatus->int_);
			break;
		case KN_SPEARSTAB:
			add_spear_stab_special_effects(src, target);
			return calculate_spear_stab_atk_ratio(skill_lv, target);
			break;
		case LK_HEADCRUSH:
			add_traumatic_blow_special_effects(src, target);
			calculate_traumatic_blow_atk_ratio(skill_lv);
			break;
		case LK_JOINTBEAT:
			add_lightning_strike_special_effects(target);
			calculate_lightning_strike_atk_ratio(skill_lv, sstatus->int_);
			break;
		default:
			return 0;
			break;
	}
}

void SwordsmanSkillAtkRatioCalculator::add_spear_stab_special_effects(struct block_list* src, struct block_list *target)
{
	clif_specialeffect(src, EF_BASH3D, AREA);
}

void SwordsmanSkillAtkRatioCalculator::add_magnum_break_special_effects(struct block_list* src)
{
	clif_specialeffect(src, 1239, AREA);
}

void SwordsmanSkillAtkRatioCalculator::add_lightning_strike_special_effects(struct block_list *target)
{
	clif_specialeffect(target, EF_WINDHIT, AREA);
}

void SwordsmanSkillAtkRatioCalculator::add_traumatic_blow_special_effects(struct block_list* src, struct block_list *target)
{
	clif_specialeffect(target, EF_MADNESS_RED, AREA);
	clif_specialeffect(target, EF_CRITICALWOUND, AREA);
}

void SwordsmanSkillAtkRatioCalculator::add_bash_special_effects(struct block_list *target)
{
	clif_specialeffect(target, EF_STUNATTACK, AREA);
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

int SwordsmanSkillAtkRatioCalculator::calculate_magnum_break_atk_ratio(int skill_lv, int intelligence)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 50 + (intelligence/3);
			break;
		case 2:
			ratio = 100 + (intelligence/3);
			break;
		case 3:
			ratio = 150 + (intelligence/3);
			break;
		case 4:
			ratio = 200 + (intelligence/3);
			break;
		case 5:
			ratio = 250 + (intelligence/3);
			break;
		}
	return ratio;
}


int SwordsmanSkillAtkRatioCalculator::calculate_spear_stab_atk_ratio(int skill_lv, struct block_list *target)
{
	int ratio = 0;
	struct status_change *target_status;

	target_status = status_get_sc(target);

	if (target_status->data[SC_BLEEDING]) {
		return calculate_spear_stab_bleeding_atk_ratio(skill_lv);
	}
	return calculate_spear_stab_normal_atk_ratio(skill_lv);
}

int SwordsmanSkillAtkRatioCalculator::calculate_spear_stab_normal_atk_ratio(int skill_lv)
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


int SwordsmanSkillAtkRatioCalculator::calculate_spear_stab_bleeding_atk_ratio(int skill_lv)
{
	int ratio = 0;
	
	switch (skill_lv) {
		case 1:
			ratio = 80;
			break;
		case 2:
			ratio = 150;
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
	return ratio;
}



int SwordsmanSkillAtkRatioCalculator::calculate_traumatic_blow_atk_ratio(int skill_lv)
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




int SwordsmanSkillAtkRatioCalculator::calculate_lightning_strike_atk_ratio(int skill_lv, int intelligence)
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
	return ratio + (intelligence/3);
}
