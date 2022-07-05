#include "ArcherAdditionalEffectsCalculator.h"



void ArcherAdditionalEffectsCalculator::apply_tranquilizer_shot_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv)
{
	status_change_start(src, bl, SC_SLEEP, 10000, skill_lv, 0, 0, 0, skill_lv * 2000, SCSTART_NONE);
}

void ArcherAdditionalEffectsCalculator::apply_paralyzing_shot_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv)
{
	status_change_start(src, bl, SC_PARALYSIS, 10000, skill_lv, 0, 0, 0, skill_lv * 2000, SCSTART_NONE);
}
