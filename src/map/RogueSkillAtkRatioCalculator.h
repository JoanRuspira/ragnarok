#pragma once
class RogueSkillAtkRatioCalculator
{
	public:
		static int calculate_skill_atk_ratio(struct block_list* src, struct block_list *target, int base_lv, int skill_id, int skill_lv, struct status_data* sstatus, bool is_hiding);

	private:
		static int calculate_grimtooth_atk_ratio(int skill_lv);
		static int calculate_back_stab_atk_ratio(int skill_lv);
		static int calculate_phantom_menace_atk_ratio(int skill_lv, bool is_hiding);
		static int calculate_shady_slash_atk_ratio(int skill_lv);
		static int calculate_hack_and_slash_atk_ratio(int skill_lv, int luck);
		static int calculate_quick_shot_atk_ratio(int skill_lv);
		static int calculate_triangle_shot_atk_ratio(int skill_lv);
		static int calculate_arrow_storm_atk_ratio(int skill_lv, int dex);
		static void add_back_stab_special_effects(struct block_list *target);
		static void add_shady_slash_special_effects(struct block_list *target);
		static void add_hack_and_slash_special_effects(struct block_list *src, struct block_list *target);
};

