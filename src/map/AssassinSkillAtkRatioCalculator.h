#pragma once
class AssassinSkillAtkRatioCalculator
{
	public:
		static int calculate_skill_atk_ratio(struct block_list* src, struct block_list *target, int base_lv, int skill_id, int skill_lv, struct status_data* sstatus, int rolling_cutter_counters);

	private:
		static int calculate_venom_dust_atk_ratio(int skill_lv, int intelligence);
		static int calculate_venom_splasher_atk_ratio(int skill_lv, int intelligence);
		static int calculate_sonic_blow_atk_ratio(int skill_lv);
		static int calculate_throw_kunai_atk_ratio(int skill_lv, int dex);
		static int calculate_throw_shuriken_atk_ratio(int skill_lv, int dex);
		static int calculate_soul_destroyer_atk_ratio(int skill_lv, int dex);
		static int calculate_rolling_cutter_atk_ratio(int skill_lv, int luck);
		static int calculate_dark_claw_atk_ratio(int skill_lv);
		static int calculate_cross_impact_atk_ratio(int skill_lv, int luck);
		static int calculate_cross_ripper_slasher_atk_ratio(int skill_lv, int rolling_cutter_counters);
		static int calculate_meteor_assault_atk_ratio(int skill_lv, int dex);
		static int calculate_poison_buster_atk_ratio(int skill_lv, int intelligence);
		static void add_sonic_blow_especial_effects(struct block_list *target);
		static void add_rolling_cutter_especial_effects(struct block_list *target);
		static void add_venom_splasher_especial_effects(struct block_list *target);
		static void add_throw_kunai_especial_effects(struct block_list *target);
		static void add_throw_shuriken_especial_effects(struct block_list *target);
		static void add_meteor_assault_special_effects(struct block_list *src);
		static void add_dark_claw_special_effects(struct block_list *target);
		static void add_cross_impact_special_effects(struct block_list *target);

};

