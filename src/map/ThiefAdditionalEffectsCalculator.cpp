#include "ThiefAdditionalEffectsCalculator.h"



void ThiefAdditionalEffectsCalculator::apply_envenom_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv)
{
	status_change_start(src, bl, SC_POISON, 10000, skill_lv, 0, 0, 0, skill_lv * 5000, SCSTART_NONE);
}

void ThiefAdditionalEffectsCalculator::apply_throw_stone_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv)
{
	status_change_start(src, bl, SC_STUN, 10000, skill_lv, 0, 0, 0, skill_lv * 600, SCSTART_NONE);
}


void ThiefAdditionalEffectsCalculator::apply_snatch_additional_effect(struct block_list* src, struct block_list *bl, struct map_session_data *sd, int skill_lv, int &skill)
{
	status_change_start(src, bl, SC_STUN, 10000, skill_lv, 0, 0, 0, skill_lv * 500, SCSTART_NONE);
	int zeny = 25 + ( std::rand() % ( 100 - 25 + 1 ) );
	pc_getzeny(sd,zeny,LOG_TYPE_COMMAND,NULL);
}

void ThiefAdditionalEffectsCalculator::apply_sand_attack_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv)
{
	status_change_start(src, bl, SC_STUN, 10000, skill_lv, 0, 0, 0, skill_lv * 300, SCSTART_NONE);
	status_change_start(src, bl, SC_BLIND, 10000, skill_lv, 0, 0, 0, skill_lv * 1600, SCSTART_NONE);
}
