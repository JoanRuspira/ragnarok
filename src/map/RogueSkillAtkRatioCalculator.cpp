#include "RogueSkillAtkRatioCalculator.h"
#include <cstring>
#include "skill.hpp"
#include "status.hpp"
#include "clif.hpp"

int RogueSkillAtkRatioCalculator::calculate_skill_atk_ratio(struct block_list* src, struct block_list *target, int base_lv, int skill_id, int skill_lv, struct status_data* sstatus, bool is_hiding)
{
	switch (skill_id) {
		case AS_GRIMTOOTH:
			return calculate_grimtooth_atk_ratio(skill_lv);
			break;
		case RG_RAID:
			return calculate_phantom_menace_atk_ratio(skill_lv, is_hiding);
			break;
		case RG_BACKSTAP:
			add_back_stab_special_effects(target);
			return calculate_back_stab_atk_ratio(skill_lv);
			break;
		case GC_COUNTERSLASH:
			add_back_stab_special_effects(target);
			add_hack_and_slash_special_effects(src, target);
			return calculate_hack_and_slash_atk_ratio(skill_lv, sstatus->luk);
			break;
		case NJ_KIRIKAGE:
			add_shady_slash_special_effects(target);
			return calculate_shady_slash_atk_ratio(skill_lv);
		case RA_AIMEDBOLT:
			return calculate_quick_shot_atk_ratio(skill_lv);
		case SC_TRIANGLESHOT:
			return calculate_triangle_shot_atk_ratio(skill_lv);
		case RA_ARROWSTORM:
			return calculate_arrow_storm_atk_ratio(skill_lv, sstatus->dex);
		case LG_MOONSLASHER:
			return calculate_fatal_menace_atk_ratio(skill_lv, sstatus->luk);
		default:
			return 0;
			break;
	}
}

void RogueSkillAtkRatioCalculator::add_shady_slash_special_effects(struct block_list *target)
{
	clif_specialeffect(target, 1103, AREA);
}

void RogueSkillAtkRatioCalculator::add_back_stab_special_effects(struct block_list *target)
{
	clif_specialeffect(target, EF_STUNATTACK, AREA);
}

void RogueSkillAtkRatioCalculator::add_hack_and_slash_special_effects(struct block_list *src, struct block_list *target)
{
	clif_specialeffect(target, EF_HIT2, AREA);
	clif_specialeffect(target, EF_HIT6, AREA);
	clif_specialeffect(target, EF_HFLIMOON3, AREA);
	clif_specialeffect(target, EF_ZANGETSU, AREA); //ÃÊ½Â´Þ2.str
	clif_specialeffect(src, EF_GUMGANG7, AREA);
}


int RogueSkillAtkRatioCalculator::calculate_arrow_storm_atk_ratio(int skill_lv, int dex)
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
	return ratio + (dex/3);
}

int RogueSkillAtkRatioCalculator::calculate_triangle_shot_atk_ratio(int skill_lv)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 150;
			break;
		case 2:
			ratio = 200;
			break;
		case 3:
			ratio = 250;
			break;
		case 4:
			ratio = 300;
			break;
		case 5:
			ratio = 350;
			break;
	}
	return ratio;
}


int RogueSkillAtkRatioCalculator::calculate_quick_shot_atk_ratio(int skill_lv)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 25;
			break;
		case 2:
			ratio = 50;
			break;
		case 3:
			ratio = 75;
			break;
		case 4:
			ratio = 100;
			break;
		case 5:
			ratio = 125;
			break;
		}
	return ratio;
}


int RogueSkillAtkRatioCalculator::calculate_shady_slash_atk_ratio(int skill_lv)
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

int RogueSkillAtkRatioCalculator::calculate_hack_and_slash_atk_ratio(int skill_lv, int luck)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 3;
			break;
		case 2:
			ratio = 10;
			break;
		case 3:
			ratio = 18;
			break;
		case 4:
			ratio = 24;
			break;
		case 5:
			ratio = 30;
			break;
		}
	return ratio + (luck/8);
}

int RogueSkillAtkRatioCalculator::calculate_phantom_menace_atk_ratio(int skill_lv, bool is_hiding)
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

	if ( is_hiding ) {
		ratio += 50;
	}

	return ratio;
}

int RogueSkillAtkRatioCalculator::calculate_grimtooth_atk_ratio(int skill_lv)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = -62;
			break;
		case 2:
			ratio = -24;
			break;
		case 3:
			ratio = 14;
			break;
		case 4:
			ratio = 52;
			break;
		case 5:
			ratio = 100;
			break;
	}
	return ratio;
}

int RogueSkillAtkRatioCalculator::calculate_back_stab_atk_ratio(int skill_lv)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 150;
			break;
		case 2:
			ratio = 200;
			break;
		case 3:
			ratio = 250;
			break;
		case 4:
			ratio = 300;
			break;
		case 5:
			ratio = 350;
			break;
	}
	return ratio;
}


int RogueSkillAtkRatioCalculator::calculate_fatal_menace_atk_ratio(int skill_lv, int luck)
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
	return ratio + (luck);
}