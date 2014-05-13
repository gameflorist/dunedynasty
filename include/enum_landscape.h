#ifndef ENUM_LANDSCAPE_H
#define ENUM_LANDSCAPE_H

enum LandscapeType {
	LST_NORMAL_SAND         =  0, /* Flat sand. */
	LST_PARTIAL_ROCK        =  1, /* Edge of a rocky area (mostly sand). */
	LST_ENTIRELY_DUNE       =  2, /* Entirely sand dunes. */
	LST_PARTIAL_DUNE        =  3, /* Partial sand dunes. */
	LST_ENTIRELY_ROCK       =  4, /* Centre part of rocky area. */
	LST_MOSTLY_ROCK         =  5, /* Edge of a rocky area (mostly rocky). */
	LST_ENTIRELY_MOUNTAIN   =  6, /* Centre part of the mountain. */
	LST_PARTIAL_MOUNTAIN    =  7, /* Edge of a mountain. */
	LST_SPICE               =  8, /* Sand with spice. */
	LST_THICK_SPICE         =  9, /* Sand with thick spice. */
	LST_CONCRETE_SLAB       = 10, /* Concrete slab. */
	LST_WALL                = 11, /* Wall. */
	LST_STRUCTURE           = 12, /* Structure. */
	LST_DESTROYED_WALL      = 13, /* Destroyed wall. */
	LST_BLOOM_FIELD         = 14, /* Bloom field. */

	LST_MAX                 = 15
};

#endif
