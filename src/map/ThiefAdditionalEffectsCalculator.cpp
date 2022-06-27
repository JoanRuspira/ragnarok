#include "ThiefAdditionalEffectsCalculator.h"



void ThiefAdditionalEffectsCalculator::calculate_envenom_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv)
{
	status_change_start(src, bl, SC_POISON, 10000, skill_lv, 0, 0, 0, skill_lv * 5000, SCSTART_NONE);
}
