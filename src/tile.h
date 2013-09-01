/** @file src/tile.h %Tile definitions. */

#ifndef TILE_H
#define TILE_H

extern void Tile_RefreshFogInRadius(tile32 tile, uint16 radius, bool unveil);
extern void Tile_RemoveFogInRadius(tile32 tile, uint16 radius);
extern uint16 Tile_GetTileInDirectionOf(uint16 packed_from, uint16 packed_to);
extern uint8 Tile_GetDirectionPacked(uint16 packed_from, uint16 packed_to);
extern int8 Tile_GetDirection(tile32 from, tile32 to);

#endif /* TILE_H */
