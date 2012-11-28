/** @file src/enhancement.c
 *
 * Enhancements to the original game.  In all cases, true indicates
 * deviation from the original game.
 */

#include "enhancement.h"

/**
 * Enhancements that do not have a separate entry.
 */
bool g_dune2_enhanced = true;

/**
 * In the original game, the AI is allowed to place structures
 * on top of units, and is not penalised for lack of concrete.
 */
bool enhancement_ai_respects_structure_placement = true;

/**
 * Various AI changes to make the game tougher.  Includes double
 * production rate, half cost, flanking attacks, etc.
 */
bool enhancement_brutal_ai = false;

/**
 * Draw structure and unit health bars in directly in the viewport.
 * Toggle in game with back-quote.
 */
bool enhancement_draw_health_bars = true;

/**
 * Fix the often incorrect warning message regarding the direction of
 * incoming enemies in the early missions.
 */
bool enhancement_fix_enemy_approach_direction_warning = true;

/**
 * In the original game, once a unit starts to wobble, it will never
 * stop wobbling.
 */
bool enhancement_fix_everlasting_unit_wobble = true;

/**
 * Make sonic blasts' distance and units' relative firing rates more
 * consistent across game speeds, and make infantry attack structures.
 */
bool enhancement_fix_firing_rates_and_ranges = true;

/**
 * Too many tile colours are remapped with house colours, causing
 * graphical issues with the IX building.
 */
bool enhancement_fix_ix_colour_remapping = true;

/**
 * In the original game, Ordos get siege tanks one level too late
 * (affects both players and AI), which serves as a difficulty level.
 */
bool enhancement_fix_ordos_siege_tank_tech = true;

/**
 * Fix typos in the scenarios, including AI unit caps, AI teams,
 * reinforcements, and Atreides WOR facilities.
 */
bool enhancement_fix_scenario_typos = true;

/**
 * When a unit enters a structure, the last tile the unit was on
 * becomes selected rather than the entire structure.
 */
bool enhancement_fix_selection_after_entering_structure = true;

/**
 * Fix some of the typos and formatting issues from the original game,
 * including the silo, windtrap, and outpost text.
 */
bool enhancement_fix_typos = true;

/**
 * In the original game, fog is drawn underneath units, at the same
 * time as other overlays, making units suddenly appear and disappear.
 */
bool enhancement_fog_covers_units = true;

/**
 * Add non-permanent scouting.
 */
bool enhancement_fog_of_war = false;

/**
 * Dune 2 likes to search the tiles surrounding the one you clicked
 * for an appropriate target, which can be annoying.
 */
bool enhancement_i_mean_where_i_clicked = true;

/**
 * Use the infantry squad death animations like in the Sega Mega Drive
 * version of Dune II.
 */
bool enhancement_infantry_squad_death_animations = true;

/**
 * In the original game, sandworms disappear after eating a set number
 * of units.
 */
bool enhancement_insatiable_sandworms = false;

/**
 * Saboteurs are masters of stealth; make them visible only in the
 * minimap like in v1.0 (due to a bug).
 */
bool enhancement_invisible_saboteurs = false;

/**
 * The original selection cursor can be a little obstructive with
 * multiple units selected.
 */
bool enhancement_new_selection_cursor = true;

/**
 * Do not pause game play when playing the radar activation and
 * deactivation animation sequences.
 */
bool enhancement_nonblocking_radar_animation = true;

/**
 * Normally non-Ordos deviators (e.g. captured Ordos heavy factories)
 * will still turn units to Ordos (maybe it's the gas?).
 */
bool enhancement_nonordos_deviation = true;

/**
 * Enable some extra sounds and voices.  This restores the original
 * "The Building of a Dynasty" voice!
 */
bool enhancement_play_additional_voices = true;

/**
 * Dune 2 usually limits the player to 25 units, and the CPU to 20
 * units per house.  Allow larger armies if possible.
 */
bool enhancement_raise_scenario_unit_cap = false;

/**
 * The original game ignores all structures' health fields specified
 * in the scenario, giving them all full health instead.
 */
bool enhancement_read_scenario_structure_health = false;

/**
 * The repair cost formula is different in v1.0 and v1.07, and caused
 * palace repairs to become free in v1.07 due to a flooring bug.
 */
enum RepairCostFormula enhancement_repair_cost_formula;
enum RepairCostFormula enhancement_repair_cost_formula_default = REPAIR_COST_v107_HIGH_HP_FIX;

/**
 * A mistake in reading the scenario script causes reinforcements to
 * only be sent once.
 */
bool enhancement_repeat_reinforcements = true;

/**
 * Enable the security question, accept any answer (default), or skip
 * it entirely.
 */
enum SecurityQuestionMode enhancement_security_question = SECURITY_QUESTION_ACCEPT_ALL;

/**
 * Render units (and bullets) as if they move every frame, and rotate
 * top-down units to arbitrary angles.
 */
enum SmoothUnitAnimationMode enhancement_smooth_unit_animation = SMOOTH_UNIT_ANIMATION_ENABLE;

/**
 * Make soldiers entering structures do more damage, thus turning them
 * into half-decent engineers.
 */
bool enhancement_soldier_engineers = false;

/**
 * Use the Fremen and Sardaukar portaits for troopers and trooper
 * squads.
 */
bool enhancement_special_trooper_portaits = true;

/**
 * Prevent structures built completely on concrete slabs from
 * degrading, as this seems in contrast with the idea of slabs.
 */
bool enhancement_structures_on_concrete_do_not_degrade = true;

/**
 * Choose to override the EU version's "The Battle for Arrakis"
 * subtitle with "The Building of a Dynasty".
 */
enum SubtitleOverride enhancement_subtitle_override = SUBTITLE_THE_BUILDING_OF_LOWER_A_DYNASTY;

/**
 * Make saboteurs only detonate at the target if on sabotage command
 * or right-clicked on a structure, unit, or wall.
 */
bool enhancement_targetted_sabotage = true;
