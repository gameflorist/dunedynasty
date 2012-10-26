/** @file src/enhancement.h Enhancements to the original game. */

#ifndef ENHANCEMENT_H
#define ENHANCEMENT_H

#include <stdbool.h>

enum SecurityQuestionMode {
	SECURITY_QUESTION_ENABLE,
	SECURITY_QUESTION_ACCEPT_ALL,
	SECURITY_QUESTION_SKIP
};

enum SmoothUnitAnimationMode {
	SMOOTH_UNIT_ANIMATION_DISABLE,
	SMOOTH_UNIT_ANIMATION_TRANSLATION_ONLY,
	SMOOTH_UNIT_ANIMATION_ENABLE
};

extern bool g_dune2_enhanced;
extern bool enhancement_brutal_ai;
extern bool enhancement_draw_health_bars;
extern bool enhancement_fix_enemy_approach_direction_warning;
extern bool enhancement_fix_everlasting_unit_wobble;
extern bool enhancement_fix_firing_rates_and_ranges;
extern bool enhancement_fix_ix_colour_remapping;
extern bool enhancement_fix_ordos_siege_tank_tech;
extern bool enhancement_fix_scenario_typos;
extern bool enhancement_fix_selection_after_entering_structure;
extern bool enhancement_fix_typos;
extern bool enhancement_fog_covers_units;
extern bool enhancement_i_mean_where_i_clicked;
extern bool enhancement_infantry_squad_death_animations;
extern bool enhancement_insatiable_sandworms;
extern bool enhancement_invisible_saboteurs;
extern bool enhancement_new_selection_cursor;
extern bool enhancement_nonblocking_radar_animation;
extern bool enhancement_nonordos_deviation;
extern bool enhancement_play_additional_voices;
extern bool enhancement_raise_scenario_unit_cap;
extern bool enhancement_read_scenario_structure_health;
extern bool enhancement_repeat_reinforcements;
extern bool enhancement_scroll_along_screen_edge;
extern enum SecurityQuestionMode enhancement_security_question;
extern enum SmoothUnitAnimationMode enhancement_smooth_unit_animation;
extern bool enhancement_soldier_engineers;
extern bool enhancement_special_trooper_portaits;
extern bool enhancement_structures_on_concrete_do_not_degrade;

#endif
