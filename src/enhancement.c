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
 * In the original game, once a unit starts to wobble, it will never
 * stop wobbling.
 */
bool enhancement_fix_everlasting_unit_wobble = true;

/**
 * Too many tile colours are remapped with house colours, causing
 * graphical issues with the IX building.
 */
bool enhancement_fix_ix_colour_remapping = true;

/**
 * The original game ignores all structures' health fields specified
 * in the scenario, giving them all full health instead.
 */
bool enhancement_read_scenario_structure_health = false;

/**
 * A mistake in reading the scenario script causes reinforcements to
 * only be sent once.
 */
bool enhancement_repeat_reinforcements = true;

/**
 * Prevent structures built completely on concrete slabs from
 * degrading, as this seems in contrast with the idea of slabs.
 */
bool enhancement_structures_on_concrete_do_not_degrade = true;
