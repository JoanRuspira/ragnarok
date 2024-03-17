#include "AssassinAdditionalEffectsCalculator.h"



void AssassinAdditionalEffectsCalculator::apply_meteor_assault_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv)
{
	status_change_start(src, bl, STATUS_SLEEP, 10000, skill_lv, 0, 0, 0, skill_lv * 2000, SCSTART_NONE);
}
