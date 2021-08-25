#pragma once
class CrusaderSkillAtkRatioCalculator
{
private:
	int _base_lv;
	int _skill_id;
	int _skill_lv;
	int _int;

public:
	CrusaderSkillAtkRatioCalculator(int base_lv, int skill_id, int skill_lv, int int_);
	~CrusaderSkillAtkRatioCalculator();
	int calculate_skill_atk_ratio();

private:
	int calculate_holy_cross_atk_ratio();
	int calculate_shield_charge_atk_ratio();
	int calculate_shield_boomerang_atk_ratio();
	int calculate_shield_chain_atk_ratio();
	int calculate_sacrifice_atk_ratio();
	int calculate_spear_boomerang_atk_ratio();
	int calculate_brandish_spear_atk_ratio();
	int calculate_pierce_atk_ratio();
	int calculate_hundred_spear_atk_ratio();
};
