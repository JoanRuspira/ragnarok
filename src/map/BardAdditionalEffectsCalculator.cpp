#include "BardAdditionalEffectsCalculator.h"


void BardAdditionalEffectsCalculator::apply_tarot_cards_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv)
{
	int card = 0;
    card = rnd() % 14 + 1;
    int effect = 0;
    effect = rnd() % 6 + 1;
    clif_specialeffect(bl, EF_DETOXICATION, AREA);

    switch (card) {
	    case 1:// THE FOOL
            status_percent_damage(src, bl, 0, 100, false);
            clif_specialeffect(bl, EF_TAROTCARD1, AREA);
            break;
        case 2:// THE MAGICIAN  
            sc_start(src, bl, SC_INCMATKRATE, 100, -50, skill_get_time2(JG_TAROTCARD, skill_lv));
            clif_specialeffect(bl, EF_TAROTCARD2, AREA);
            break;
        case 3:// THE HIGH PRIEST
            status_change_clear_buffs(bl, SCCB_BUFFS | SCCB_CHEM_PROTECT);
            clif_specialeffect(bl, EF_TAROTCARD3, AREA);
            break;
        case 4:// THE CHARIOT  
            status_change_start(src, bl, SC_FREEZE, 10000, skill_lv, 0, 0, 0, skill_lv * 2000, SCSTART_NONE);
            clif_specialeffect(bl, EF_TAROTCARD4, AREA);
            break;
        case 5:// STRENGTH   
            sc_start(src, bl, SC_INCATKRATE, 100, -50, skill_get_time2(JG_TAROTCARD, skill_lv));
            clif_specialeffect(bl, EF_TAROTCARD5, AREA);
            break;
        case 6:// THE LOVERS   
            status_change_start(src, bl, SC_BLIND, 10000, skill_lv, 0, 0, 0, skill_lv * 2000, SCSTART_NONE);
            status_change_start(src, bl, SC_CONFUSION, 10000, skill_lv, 0, 0, 0, skill_lv * 2000, SCSTART_NONE);
            clif_specialeffect(bl, EF_TAROTCARD6, AREA);
            break;
        case 7:// WHEEL OF FORTUNE
            switch (effect) {
                case 1: 
                    status_change_start(src, bl, SC_FREEZE, 10000, skill_lv, 0, 0, 0, skill_lv * 2000, SCSTART_NONE);
                    break;
                case 2:
                    status_change_start(src, bl, SC_SILENCE, 10000, skill_lv, 0, 0, 0, skill_lv * 2000, SCSTART_NONE);
                    break;
                case 3:
                    status_change_start(src, bl, SC_CURSE, 10000, skill_lv, 0, 0, 0, skill_lv * 2000, SCSTART_NONE);
                    break;
                case 4:
                    status_change_start(src, bl, SC_SLEEP, 10000, skill_lv, 0, 0, 0, skill_lv * 2000, SCSTART_NONE);
                    break;
                case 5:
                    status_change_start(src, bl, SC_STUN, 10000, skill_lv, 0, 0, 0, skill_lv * 2000, SCSTART_NONE);
                    break;
                case 6:
                    status_change_start(src, bl, SC_STONE, 10000, skill_lv, 0, 0, 0, skill_lv * 2000, SCSTART_NONE);
                    break;
            }
            clif_specialeffect(bl, EF_TAROTCARD7, AREA);
            break;
        case 8:// THE HANGED MAN
            status_change_start(src, bl, SC_SILENCE, 10000, skill_lv, 0, 0, 0, skill_lv * 2000, SCSTART_NONE);
            clif_specialeffect(bl, EF_TAROTCARD8, AREA);
            break;
        case 9:// DEATH
            status_change_start(src, bl, SC_COMA, 10000, skill_lv, 0, 0, 0, skill_lv * 2000, SCSTART_NONE);
            clif_specialeffect(bl, EF_TAROTCARD9, AREA);
            break;
        case 10:// TEMPERANCE  
            status_change_start(src, bl, SC_POISON, 10000, skill_lv, 0, 0, 0, skill_lv * 2000, SCSTART_NONE);
            clif_specialeffect(bl, EF_TAROTCARD10, AREA);
            break;
        case 11:// THE DEVIL
            status_change_start(src, bl, SC_CURSE, 10000, skill_lv, 0, 0, 0, skill_lv * 2000, SCSTART_NONE);
            clif_specialeffect(bl, EF_TAROTCARD11, AREA);
            break;
        case 12:// THE TOWER
            status_change_start(src, bl, SC_STONE, 10000, skill_lv, 0, 0, 0, skill_lv * 2000, SCSTART_NONE);
            clif_specialeffect(bl, EF_TAROTCARD12, AREA);
            break;
        case 13:// THE MOON 
            status_change_start(src, bl, SC_SLEEP, 10000, skill_lv, 0, 0, 0, skill_lv * 2000, SCSTART_NONE);
            clif_specialeffect(bl, EF_TAROTCARD13, AREA);
            break;
        case 14:// THE SUN
            status_change_start(src, bl, SC_STUN, 10000, skill_lv, 0, 0, 0, skill_lv * 2000, SCSTART_NONE);
            clif_specialeffect(bl, EF_TAROTCARD14, AREA);
            break;
    }
}
