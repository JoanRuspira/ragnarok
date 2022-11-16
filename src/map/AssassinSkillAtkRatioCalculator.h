#pragma once
class AssassinSkillAtkRatioCalculator
{
	public:
		static int calculate_skill_atk_ratio(struct block_list* src, struct block_list *target, int base_lv, int skill_id, int skill_lv, struct status_data* sstatus, bool is_hiding);

	private:
		static int calculate_venom_dust_atk_ratio(int skill_lv, int intelligence);
		static void add_sonic_blow_especial_effects(struct block_list *target);
};

