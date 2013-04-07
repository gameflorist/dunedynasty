/** @file src/enhancement.h Enhancements to the original game. */

#ifndef ENHANCEMENT_H
#define ENHANCEMENT_H

#include <stdbool.h>

enum HealthBarMode {
	HEALTH_BAR_DISABLE,
	HEALTH_BAR_SELECTED_UNITS,
	HEALTH_BAR_ALL_UNITS,

	NUM_HEALTH_BAR_MODES
};

enum RepairCostFormula {
	REPAIR_COST_v107,
	REPAIR_COST_v100,
	REPAIR_COST_OPENDUNE,
	REPAIR_COST_v107_HIGH_HP_FIX
};

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

enum SubtitleOverride {
	SUBTITLE_THE_BATTLE_FOR_ARRAKIS,            /* EU subtitle. */
	SUBTITLE_THE_BUILDING_OF_UPPER_A_DYNASTY,   /* US subtitle. */
	SUBTITLE_THE_BUILDING_OF_LOWER_A_DYNASTY
};

extern bool const g_dune2_enhanced;
extern bool const enhancement_fix_enemy_approach_direction_warning;
extern bool const enhancement_fix_everlasting_unit_wobble;
extern bool const enhancement_fix_firing_logic;
extern bool const enhancement_fix_ix_colour_remapping;
extern bool const enhancement_fix_selection_after_entering_structure;
extern bool const enhancement_fix_typos;

extern bool enhancement_ai_respects_structure_placement;
extern bool enhancement_brutal_ai;
extern bool enhancement_construction_does_not_pause;
extern enum HealthBarMode enhancement_draw_health_bars;
extern bool enhancement_fog_covers_units;
extern bool enhancement_fog_of_war;
extern bool enhancement_high_res_overlays;
extern bool enhancement_i_mean_where_i_clicked;
extern bool enhancement_infantry_squad_death_animations;
extern bool enhancement_insatiable_sandworms;
extern bool enhancement_invisible_saboteurs;
extern bool enhancement_nonblocking_radar_animation;
extern bool enhancement_nonordos_deviation;
extern bool enhancement_permanent_follow_mode;
extern bool enhancement_play_additional_voices;
extern bool enhancement_raise_scenario_unit_cap;
extern bool enhancement_repeat_reinforcements;
extern enum SecurityQuestionMode enhancement_security_question;
extern enum SmoothUnitAnimationMode enhancement_smooth_unit_animation;
extern bool enhancement_soldier_engineers;
extern bool enhancement_structures_on_concrete_do_not_degrade;
extern enum SubtitleOverride enhancement_subtitle_override;
extern bool enhancement_targetted_sabotage;
extern bool enhancement_true_game_speed_adjustment;

extern bool enhancement_fix_scenario_typos;
extern bool enhancement_read_scenario_structure_health;
extern bool enhancement_undelay_ordos_siege_tank_tech;
extern bool enhancement_infantry_mini_rockets;
extern enum RepairCostFormula enhancement_repair_cost_formula;
extern bool enhancement_special_trooper_portaits;

#endif
