#pragma once
class KnightSkillAtkRatioCalculator
{
private:
	int _base_lv;
	int _skill_id;
	int _skill_lv;
	int _int;
	int _weapon_element;

public:
	KnightSkillAtkRatioCalculator(int base_lv, int skill_id, int skill_lv, int int_, int _weapon_element);
	int calculate_skill_atk_ratio();

private:
	int calculate_spear_boomerang_atk_ratio();
	int calculate_brandish_spear_atk_ratio();
	int calculate_pierce_atk_ratio();
	int calculate_hundred_spear_atk_ratio();
	int calculate_bowling_bash_atk_ratio();
	int calculate_charge_attack_atk_ratio();
	int calculate_head_crush_atk_ratio();
	int calculate_spiral_pierce_atk_ratio();
	int calculate_shield_chain_atk_ratio();
	int calculate_wind_cutter_atk_ratio();
	int calculate_sonic_wave_atk_ratio();
	int calculate_ignition_break_atk_ratio();
};
