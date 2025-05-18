/** @file src/enhancement.c
 *
 * Enhancements to the original game.  In all cases, true indicates
 * deviation from the original game.
 */

#include "enhancement.h"

/**
 * Enhancements that do not have a separate entry.
 */
bool const g_dune2_enhanced = true;

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
 * In Dune II, construction automatically goes on hold when you run
 * out of funds.  This behaviour does not work well with build queues.
 */
bool enhancement_construction_does_not_pause = true;

/**
 * Draw structure and unit health bars in directly in the viewport.
 * Toggle in game with back-quote.
 */
enum HealthBarMode enhancement_draw_health_bars = HEALTH_BAR_SELECTED_UNITS;

/**
 * [OpenDUNE bug] Someone messed up the calculation regarding the
 * direction of incoming enemies in the early missions.
 */
bool const enhancement_fix_enemy_approach_direction_warning = true;

/**
 * In the original game, once a unit starts to wobble, it will never
 * stop wobbling.
 */
bool const enhancement_fix_everlasting_unit_wobble = true;

/**
 * Make sonic blasts' distance and units' relative firing rates more
 * consistent across game speeds, and make infantry attack structures.
 */
bool const enhancement_fix_firing_logic = true;

/**
 * Too many tile colours are remapped with house colours, causing
 * graphical issues with the IX building.
 */
bool const enhancement_fix_ix_colour_remapping = true;

/**
 * When a unit enters a structure, the last tile the unit was on
 * becomes selected rather than the entire structure.
 */
bool const enhancement_fix_selection_after_entering_structure = true;

/**
 * Fix some of the typos and formatting issues from the original game,
 * including the silo, windtrap, and outpost text.
 */
bool const enhancement_fix_typos = true;

/**
 * Show info about active/standby/maximum units in outpost text.
 * Standby units are either:
 * - in production
 * - ordered in factory
 * - outstanding reinforcements
 * - usually 1 backup-harvester
 * This is always enabled in multiplayer.
 */
bool enhancement_show_outpost_unit_info = false;

/**
 * Reduce building time of walls to 0.
 */
bool enhancement_instant_walls = false;

/**
 * In the original game, fog is drawn underneath units, at the same
 * time as other overlays, making units suddenly appear and disappear.
 */
bool enhancement_fog_covers_units = true;

/**
 * Add non-permanent scouting.
 */
bool enhancement_fog_of_war = false;
bool enhancement_fog_of_war_backup = false;

/**
 * High-resolution overlays includes a new selection cursor, thinner
 * health bar borders, and invalid building crosses.
 */
bool enhancement_high_res_overlays = true;

/**
 * Dune II likes to search the tiles surrounding the one you clicked
 * for an appropriate target, which can be annoying.
 */
bool enhancement_i_mean_where_i_clicked = true;

/**
 * [SMD] Show infantry squad and trooper squad death animations when
 * they reduce down from a squad to a single unit.
 */
bool enhancement_infantry_squad_death_animations = true;

/**
 * In the original game, sandworms disappear after eating a set number
 * of units.
 */
bool enhancement_insatiable_sandworms = false;

/**
 * [v1.0] Saboteurs are masters of stealth; make them visible only in
 * the minimap.
 */
bool enhancement_invisible_saboteurs = false;

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
 * In the Sega Mega Drive version of Dune II, units will not return to
 * guard after catching up to the leader.  They will therefore
 * continue to follow once the leader moves again.  However, they will
 * not attack any enemies that come into range.
 */
bool enhancement_permanent_follow_mode = false;

/**
 * Enable some extra sounds and voices.  This restores the original
 * "The Building of a Dynasty" voice!
 */
bool enhancement_play_additional_voices = true;

/**
 * Dune II has 2 types of unit caps:
 * - A unit cap defined per house by the scenario (usually 25 for the player, and 20 for the CPU)
 * - A hard-coded overall cap via the UNIT_INDEX_MAX constant (default 102)
 * - And a hard-coded cap per unit/group of units via the indexStart and indexEnd properties of UnitTypes:
 *     - Caryalls and Ornithopters: 11
 *     - Frigates: 1
 *     - Projectiles: 4 (bullets, rockets, missiles, sonic blasts)
 *     - Sandworms: 3
 *     - Saboteurs: 3
 *     - All others (ground units): 80
 * This sets the scenario unit cap for all houses to 50, raises UNIT_INDEX_MAX to 322
 * and indexEnd of all ground units (incl. saboteurs) to allow a total of 300.
 * This will always be active with skirmish/multiplayer.
 */
bool enhancement_raise_unit_cap = false;

/**
 * Dune II limits total structures on a map to about 70.
 * This raises that limit by 100.
 */
bool enhancement_raise_structure_cap = false;

/**
 * A mistake in reading the scenario script causes reinforcements to
 * only be sent once.
 */
bool enhancement_repeat_reinforcements = true;

/**
 * Enable the security question, pre-answer the question (default),
 * or skip it entirely.
 */
enum SecurityQuestionMode enhancement_security_question = SECURITY_QUESTION_ANSWER_GIVEN;

/**
 * Render units (and bullets) as if they move every frame, and rotate
 * top-down units to arbitrary angles.
 */
enum SmoothUnitAnimationMode enhancement_smooth_unit_animation = SMOOTH_UNIT_ANIMATION_ENABLE;
enum SmoothUnitAnimationMode enhancement_smooth_unit_animation_backup = SMOOTH_UNIT_ANIMATION_ENABLE;

/**
 * [OpenDUNE bug] Make soldiers entering structures do more damage,
 * thus turning them into half-decent engineers.
 */
bool enhancement_soldier_engineers = false;

/**
 * Prevent structures built completely on concrete slabs from
 * degrading, as this seems in contrast with the idea of slabs.
 */
bool enhancement_structures_on_concrete_do_not_degrade = true;

/**
 * [v1.0] Override the EU "The Battle for Arrakis" subtitle with the
 * original subtitle or "The Building of a Dynasty".
 */
enum SubtitleOverride enhancement_subtitle_override = SUBTITLE_THE_BUILDING_OF_LOWER_A_DYNASTY;

/**
 * Make saboteurs only detonate at the target if on sabotage command
 * or right-clicked on a structure, unit, or wall.
 */
bool enhancement_targetted_sabotage = true;

/**
 * Dune II's game speed implementation doesn't affect scripts and
 * other things.  This also fixes the sonic tank range bug.
 */
bool enhancement_true_game_speed_adjustment = true;

/**
 * Dune II's unit movement speeds lose precision, meaning that fast
 * units (e.g. trikes) often cannot out run slower units (e.g. quads).
 */
bool enhancement_true_unit_movement_speed = false;

/**
 * In Dune II, attack damage is heavily dependent on the direction:
 * U=Target, F=Attacker making full damage, H=Attacker making half damage
 * ---F---
 * -HHFHH-
 * -HHFHH-
 * HHHUHHH
 * -HFFFH-
 * -HFFFH-
 * ---F---
 * It is unknown, if this is an unintended bug or a hidden feature.
 * 
 * More info:
 * https://forum.dune2k.com/topic/19996-info-dune-2-half-damage-bugtip-plus-question-on-fremensarduakar/
 * http://nyerguds.arsaneus-design.com/manuals/Dune%20II/Dune%20II%20-%20Insider's%20Guide.pdf (Page 279)
 * 
 */
bool enhancement_attack_dir_consistency = false;

/**
 * Skip introduction video
 */
bool enhancement_skip_introduction = false;

/**
 * Increase fog uncover radius of trikes and quads from 2 to 4 tiles.
 */
bool enhancement_extend_sight_range = false;

/**
 * Draw trajectory and target of movement/attack-order of selected units.
 * (like in Tibarian Sun).
 */
bool enhancement_draw_target_lines = false;

/**
 * This enhancement extends the spice sensing ranges in these cases:
 * - Harvesters searching for spice (from about 3-4 to 64)
 * - Carryalls searching for spice (from 20 to 64)
 * The extended search will only work for unveiled tiles visible to the house.
 */
bool enhancement_extend_spice_sensor = false;

/*--------------------------------------------------------------*/
/* Tweaks for campaigns. */

/**
 * [Dune II only] Fix typos in the original scenarios (e.g. unit caps,
 * reinforcements, Atreides WOR facilities).  Fix your own scenarios!
 */
bool enhancement_fix_scenario_typos;

/**
 * [Dune II only] The original game ignores the structure health
 * percentage specified in the scenarios.  Fix your scenarios!
 */
bool enhancement_read_scenario_structure_health;

/**
 * [Skirmish / MP] The original game delays Ordos siege tanks by one
 * level, leading to unfairness in skirmish mode.
 */
bool enhancement_undelay_ordos_siege_tank_tech;

/**
 * [Dune 2 eXtended] Make infantry fire mini-rockets when attacking
 * from long range (2+ tiles), like troopers.
 */
bool enhancement_infantry_mini_rockets;

/**
 * [v1.0] Repairs are far more expensive in v1.0.  In addition, a
 * flooring bug in the v1.07 formula lead to free repairs for palaces.
 */
enum RepairCostFormula enhancement_repair_cost_formula;

/**
 * [SMD] Use the Fremen and Sardaukar portaits for troopers and
 * trooper squads.
 */
bool enhancement_special_trooper_portaits;
