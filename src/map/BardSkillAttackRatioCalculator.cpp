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
		case SK_BA_MELODYSTRIKE:
			add_melody_strike_special_effects(target);
			return calculate_melody_strike_atk_ratio(skill_lv);
			break;
        case SK_BA_GREATECHO:
			add_great_echo_special_effects(target);
			return calculate_great_echo_atk_ratio(skill_lv);
			break;
		case SK_CL_METALLICFURY:
			add_metallic_fury_special_effects(target);
			return calculate_metallic_fury_atk_ratio(skill_lv, target);
			break;
		case SK_BA_METALLICSOUND:
			return calculate_metallic_sound_atk_ratio(skill_lv, target);
			break;
		case SK_BA_REVERBERATION:
			add_reverberation_special_effects(target);
			return calculate_reverberation_atk_ratio(skill_lv, target);
		case SK_CL_ARROWVULCAN:	
    		clif_specialeffect(target, 1355, AREA); //new_armscannon_explosion
			return calculate_arrow_vulcan_atk_ratio(skill_lv, target);
		case SK_CL_SEVERERAINSTORM:
		case SK_CL_SEVERERAINSTORM_MELEE:
			return calculate_severe_rainstorm_atk_ratio(skill_lv);
		default:
			return 0;
			break;
	}
}

int BardSkillAttackRatioCalculator::calculate_severe_rainstorm_atk_ratio(int skill_lv)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = -80;
			break;
		case 2:
			ratio = -60;
			break;
		case 3:
			ratio = -40;
			break;
		case 4:
			ratio = -20;
			break;
		case 5:
			ratio = 0;
			break;
		}
	return ratio;
}

int BardSkillAttackRatioCalculator::calculate_arrow_vulcan_normal_atk_ratio(int skill_lv)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 200;
			break;
		case 2:
			ratio = 300;
			break;
		case 3:
			ratio = 400;
			break;
		case 4:
			ratio = 500;
			break;
		case 5:
			ratio = 600;
			break;
		}
	return ratio;
}

int BardSkillAttackRatioCalculator::calculate_arrow_vulcan_confused_atk_ratio(int skill_lv)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 200;
			break;
		case 2:
			ratio = 300;
			break;
		case 3:
			ratio = 400;
			break;
		case 4:
			ratio = 500;
			break;
		case 5:
			ratio = 600;
			break;
		}
	return ratio + 100;
}


int BardSkillAttackRatioCalculator::calculate_arrow_vulcan_atk_ratio(int skill_lv, struct block_list *target)
{
	int ratio = 0;
	struct status_change *target_status;

	target_status = status_get_sc(target);

	if (target_status->data[SC_CONFUSION]) {
		return calculate_arrow_vulcan_confused_atk_ratio(skill_lv);
	}
	return calculate_arrow_vulcan_normal_atk_ratio(skill_lv);
}


int BardSkillAttackRatioCalculator::calculate_metallic_fury_atk_ratio(int skill_lv, struct block_list *target)
{
	int ratio = 0;
	struct status_change *target_status;

	target_status = status_get_sc(target);

	if (target_status->data[SC_SLEEP]) {
		return calculate_metallic_fury_sleep_atk_ratio(skill_lv);
	}
	return calculate_metallic_fury_normal_atk_ratio(skill_lv);
}


int BardSkillAttackRatioCalculator::calculate_reverberation_atk_ratio(int skill_lv, struct block_list *target)
{
	int ratio = 0;
	struct status_change *target_status;

	target_status = status_get_sc(target);

	if (target_status->data[SC_SLEEP]) {
		return calculate_reverberation_sleep_atk_ratio(skill_lv);
	}
	return calculate_reverberation_normal_atk_ratio(skill_lv);
}


int BardSkillAttackRatioCalculator::calculate_metallic_sound_atk_ratio(int skill_lv, struct block_list *target)
{
	int ratio = 0;
	struct status_change *target_status;

	target_status = status_get_sc(target);

	if (target_status->data[SC_SLEEP]) {
		return calculate_metallic_sound_sleep_atk_ratio(skill_lv);
	}
	return calculate_metallic_sound_normal_atk_ratio(skill_lv);
}

void BardSkillAttackRatioCalculator::add_metallic_fury_special_effects(struct block_list *target)
{
	clif_soundeffectall(target, "metalic.wav", 0, AREA);
    clif_specialeffect(target, 565, AREA); //moonlight_1
}

void BardSkillAttackRatioCalculator::add_reverberation_special_effects(struct block_list *target)
{
	clif_soundeffectall(target, "reverberation_jg.wav", 0, AREA);
    clif_specialeffect(target, 1366, AREA);
}
void BardSkillAttackRatioCalculator::add_tarot_cards_special_effects(struct block_list *target)
{
    clif_specialeffect(target, EF_FLOWERCAST, AREA);
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

int BardSkillAttackRatioCalculator::calculate_great_echo_atk_ratio(int skill_lv)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 200;
			break;
		case 2:
			ratio = 300;
			break;
		case 3:
			ratio = 400;
			break;
		case 4:
			ratio = 500;
			break;
		case 5:
			ratio = 600;
			break;
		}
	return ratio;
}


int BardSkillAttackRatioCalculator::calculate_metallic_sound_sleep_atk_ratio(int skill_lv)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 250;
			break;
		case 2:
			ratio = 350;
			break;
		case 3:
			ratio = 450;
			break;
		case 4:
			ratio = 550;
			break;
		case 5:
			ratio = 650;
			break;
		}
	return ratio + 100;
}

int BardSkillAttackRatioCalculator::calculate_metallic_sound_normal_atk_ratio(int skill_lv)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 250;
			break;
		case 2:
			ratio = 350;
			break;
		case 3:
			ratio = 450;
			break;
		case 4:
			ratio = 550;
			break;
		case 5:
			ratio = 650;
			break;
		}
	return ratio;
}

int BardSkillAttackRatioCalculator::calculate_tarot_cards_atk_ratio(int skill_lv)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 130;
			break;
		case 2:
			ratio = 200;
			break;
		case 3:
			ratio = 280;
			break;
		case 4:
			ratio = 400;
			break;
		case 5:
			ratio = 500;
			break;
		}
	return ratio;
}

int BardSkillAttackRatioCalculator::calculate_reverberation_normal_atk_ratio(int skill_lv)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 150;
			break;
		case 2:
			ratio = 250;
			break;
		case 3:
			ratio = 350;
			break;
		case 4:
			ratio = 450;
			break;
		case 5:
			ratio = 550;
			break;
		}
	return ratio;
}

int BardSkillAttackRatioCalculator::calculate_reverberation_sleep_atk_ratio(int skill_lv)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 150;
			break;
		case 2:
			ratio = 250;
			break;
		case 3:
			ratio = 350;
			break;
		case 4:
			ratio = 450;
			break;
		case 5:
			ratio = 550;
			break;
		}
	return ratio + 100;
}





int BardSkillAttackRatioCalculator::calculate_metallic_fury_sleep_atk_ratio(int skill_lv)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 450;
			break;
		case 2:
			ratio = 550;
			break;
		case 3:
			ratio = 650;
			break;
		case 4:
			ratio = 750;
			break;
		case 5:
			ratio = 850;
			break;
		}
	return ratio + 100;
}

int BardSkillAttackRatioCalculator::calculate_metallic_fury_normal_atk_ratio(int skill_lv)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 450;
			break;
		case 2:
			ratio = 550;
			break;
		case 3:
			ratio = 650;
			break;
		case 4:
			ratio = 750;
			break;
		case 5:
			ratio = 850;
			break;
		}
	return ratio;
}
