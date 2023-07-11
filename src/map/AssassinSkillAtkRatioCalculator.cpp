#include "AssassinSkillAtkRatioCalculator.h"
#include <cstring>
#include "skill.hpp"
#include "status.hpp"
#include "clif.hpp"

int AssassinSkillAtkRatioCalculator::calculate_skill_atk_ratio(struct block_list* src, struct block_list *target, int base_lv, int skill_id, int skill_lv, struct status_data* sstatus, int rolling_cutter_counters)
{
	switch (skill_id) {
		case AS_SONICBLOW:
			add_sonic_blow_especial_effects(target);
			return calculate_sonic_blow_atk_ratio(skill_lv);
			break;
		case SO_CLOUD_KILL:
			return calculate_venom_dust_atk_ratio(skill_lv, sstatus->int_);
			break;
		case GC_ROLLINGCUTTER:
			return calculate_rolling_cutter_atk_ratio(skill_lv, sstatus->luk);
			break;
		case GC_CROSSRIPPERSLASHER:
			return calculate_cross_ripper_slasher_atk_ratio(skill_lv, rolling_cutter_counters);
			break;
		case AS_SPLASHER:
			add_venom_splasher_especial_effects(target);
			return calculate_venom_splasher_atk_ratio(skill_lv, sstatus->int_);
			break;
		case NJ_KUNAI:
			add_throw_kunai_especial_effects(target);
			return calculate_throw_kunai_atk_ratio(skill_lv, sstatus->dex);
			break;
		case ASC_METEORASSAULT:
			// add_meteor_assault_special_effects(src);
			return calculate_meteor_assault_atk_ratio(skill_lv, sstatus->dex);
			break;
		case ASC_BREAKER:
			return calculate_soul_destroyer_atk_ratio(skill_lv, sstatus->dex);
			break;
		case GC_DARKCROW:
			add_dark_claw_special_effects(target);
			return calculate_dark_claw_atk_ratio(skill_lv);
			break;
		case DUMMY_CROSSIMPACT:
		case GC_CROSSIMPACT:
			add_cross_impact_special_effects(target);
			return calculate_cross_impact_atk_ratio(skill_lv, sstatus->luk);
			break;
		case NJ_SYURIKEN:
			add_throw_shuriken_especial_effects(target);
			return calculate_throw_shuriken_atk_ratio(skill_lv, sstatus->dex);
			break;
		default:
			return 0;
			break;
	}
}

// void AssassinSkillAtkRatioCalculator::add_meteor_assault_special_effects(struct block_list *target)
// {
// 	// clif_specialeffect(target, EF_ZANGETSU, AREA);
// }

void AssassinSkillAtkRatioCalculator::add_throw_kunai_especial_effects(struct block_list *target)
{
	clif_specialeffect(target, EF_TRIPLEATTACK4, AREA);
}

void AssassinSkillAtkRatioCalculator::add_throw_shuriken_especial_effects(struct block_list *target)
{
	clif_specialeffect(target, EF_SONIC_CLAW, AREA);
}

void AssassinSkillAtkRatioCalculator::add_sonic_blow_especial_effects(struct block_list *target)
{
	clif_specialeffect(target, EF_SONICBLOW, AREA);
	clif_specialeffect(target, 1354, AREA); //new_armscannon_11_clock_down
	clif_specialeffect(target, 1354, AREA); //new_armscannon_11_clock_down
	clif_specialeffect(target, 1354, AREA); //new_armscannon_11_clock_down
	clif_specialeffect(target, 1354, AREA); //new_armscannon_11_clock_down
}

void AssassinSkillAtkRatioCalculator::add_venom_splasher_especial_effects(struct block_list *target)
{
	clif_specialeffect(target, EF_INVENOM, AREA);
	clif_specialeffect(target, EF_VENOMDUST, AREA);
}


void AssassinSkillAtkRatioCalculator::add_dark_claw_special_effects(struct block_list *target)
{
	clif_specialeffect(target, 1349, AREA); //new_armscannon_01_up

}

void AssassinSkillAtkRatioCalculator::add_cross_impact_special_effects(struct block_list *target)
{
	clif_specialeffect(target, EF_SONICBLOW, AREA);
	clif_specialeffect(target, 1354, AREA); //new_armscannon_11_clock_down
	clif_specialeffect(target, 1354, AREA); //new_armscannon_11_clock_down
	clif_specialeffect(target, 1354, AREA); //new_armscannon_11_clock_down
	clif_specialeffect(target, 1354, AREA); //new_armscannon_11_clock_down
}

int AssassinSkillAtkRatioCalculator::calculate_cross_impact_atk_ratio(int skill_lv, int luck)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 100;
			break;
		case 2:
			ratio = 200;
			break;
		case 3:
			ratio = 300;
			break;
		case 4:
			ratio = 400;
			break;
		case 5:
			ratio = 500;
			break;
	}
	return ratio + (luck/3);
}

int AssassinSkillAtkRatioCalculator::calculate_dark_claw_atk_ratio(int skill_lv)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 100;
			break;
		case 2:
			ratio = 200;
			break;
		case 3:
			ratio = 300;
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


int AssassinSkillAtkRatioCalculator::calculate_cross_ripper_slasher_atk_ratio(int skill_lv, int rolling_cutter_counters)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 75;
			break;
		case 2:
			ratio = 100;
			break;
		case 3:
			ratio = 125;
			break;
		case 4:
			ratio = 150;
			break;
		case 5:
			ratio = 175;
			break;
	}
	return ratio + (20 * rolling_cutter_counters);
}

int AssassinSkillAtkRatioCalculator::calculate_rolling_cutter_atk_ratio(int skill_lv, int luck)
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
	return ratio + (luck/3);
}

int AssassinSkillAtkRatioCalculator::calculate_sonic_blow_atk_ratio(int skill_lv)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 200;
			break;
		case 2:
			ratio = 250;
			break;
		case 3:
			ratio = 300;
			break;
		case 4:
			ratio = 350;
			break;
		case 5:
			ratio = 400;
			break;
	}
	return ratio;
}

int AssassinSkillAtkRatioCalculator::calculate_venom_dust_atk_ratio(int skill_lv, int intelligence)
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
	return ratio + (intelligence/3);
}

int AssassinSkillAtkRatioCalculator::calculate_meteor_assault_atk_ratio(int skill_lv, int dex)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 100;
			break;
		case 2:
			ratio = 200;
			break;
		case 3:
			ratio = 300;
			break;
		case 4:
			ratio = 400;
			break;
		case 5:
			ratio = 500;
			break;
		}
	return ratio + (dex/3);
}

int AssassinSkillAtkRatioCalculator::calculate_venom_splasher_atk_ratio(int skill_lv, int intelligence)
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

int AssassinSkillAtkRatioCalculator::calculate_throw_kunai_atk_ratio(int skill_lv, int dex)
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
	return ratio + (dex/3);
}

int AssassinSkillAtkRatioCalculator::calculate_throw_shuriken_atk_ratio(int skill_lv, int dex)
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
	return ratio + (dex/3);
}

int AssassinSkillAtkRatioCalculator::calculate_soul_destroyer_atk_ratio(int skill_lv, int dex)
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
	return ratio + (dex/3);
}