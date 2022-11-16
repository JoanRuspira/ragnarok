#include "AssassinSkillAtkRatioCalculator.h"
#include <cstring>
#include "skill.hpp"
#include "status.hpp"
#include "clif.hpp"

int AssassinSkillAtkRatioCalculator::calculate_skill_atk_ratio(struct block_list* src, struct block_list *target, int base_lv, int skill_id, int skill_lv, struct status_data* sstatus, bool is_hiding)
{
	switch (skill_id) {
		case AS_SONICBLOW:
			add_sonic_blow_especial_effects(target);
			return 1;
			break;
		case SO_CLOUD_KILL:
			return calculate_venom_dust_atk_ratio(skill_lv, sstatus->int_);
			break;
		default:
			return 0;
			break;
	}
}

void AssassinSkillAtkRatioCalculator::add_sonic_blow_especial_effects(struct block_list *target)
{
	//clif_specialeffect(target, EF_SONICBLOW2, AREA);
	clif_specialeffect(target, EF_SONICBLOW, AREA);
	//clif_specialeffect(target, EF_SONICBLOWHIT, AREA);
}

int AssassinSkillAtkRatioCalculator::calculate_venom_dust_atk_ratio(int skill_lv, int intelligence)
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
