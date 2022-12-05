#pragma once
class BlacksmithSkillAtkRatioCalculator
{
	public:
		static int calculate_skill_atk_ratio(struct block_list* src, struct block_list *target, int base_lv, int skill_id, int skill_lv, struct status_data* sstatus);

	private:
		static int calculate_cart_brume_atk_ratio(int skill_lv, int intelligence);
		static int calculate_cart_shrapnel_atk_ratio(int skill_lv, int dex);
		static int calculate_cart_cannon_atk_ratio(int skill_lv, int dex);
		static int calculate_axe_boomerang_atk_ratio(int skill_lv);
		static int calculate_axe_tornado_atk_ratio(int skill_lv, int luk);

};

