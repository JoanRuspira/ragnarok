#include "BardAdditionalEffectsCalculator.h"


void BardAdditionalEffectsCalculator::apply_tarot_cards_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv)
{
}

void BardAdditionalEffectsCalculator::apply_great_echo_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv)
{
	status_change_start(src, bl, SC_CONFUSION, 10000, skill_lv, 0, 0, 0, skill_lv * 1000, SCSTART_NONE);
}

void BardAdditionalEffectsCalculator::apply_severe_rainstorm_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv)
{
	status_change_start(src, bl, SC_CONFUSION, 200*skill_lv, skill_lv, 0, 0, 0, 5000, SCSTART_NONE);
}
