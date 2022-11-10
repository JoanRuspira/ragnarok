#include "CrusaderSkillAtkRatioCalculator.h"
#include <cstring>
#include "skill.hpp"
#include "status.hpp"


int CrusaderSkillAtkRatioCalculator::calculate_skill_atk_ratio(struct block_list* src, struct block_list *target, int base_lv, int skill_id, int skill_lv, struct status_data* sstatus)
{
	switch (skill_id) {
		case CR_HOLYCROSS:
			return calculate_holy_cross_atk_ratio(skill_lv, sstatus->int_);
			break;
		
		default:
			return 0;
			break;
	}
}


int CrusaderSkillAtkRatioCalculator::calculate_holy_cross_atk_ratio(int skill_lv, int intelligence)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 125;
			break;
		case 2:
			ratio = 150;
			break;
		case 3:
			ratio = 175;
			break;
		case 4:
			ratio = 200;
			break;
		case 5:
			ratio = 225;
			break;
		}
	return ratio + (intelligence);
}

