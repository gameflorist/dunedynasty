/** @file src/tile.h %Tile definitions. */

#ifndef TILE_H
#define TILE_H

extern uint16 Tile_GetDistance(tile32 from, tile32 to);
extern uint16 Tile_GetDistancePacked(uint16 packed_from, uint16 packed_to);
extern uint16 Tile_GetDistanceRoundedUp(tile32 from, tile32 to);
extern tile32 Tile_AddTileDiff(tile32 from, tile32 diff);
extern void Tile_RefreshFogInRadius(tile32 tile, uint16 radius, bool unveil);
extern void Tile_RemoveFogInRadius(tile32 tile, uint16 radius);
extern uint16 Tile_GetTileInDirectionOf(uint16 packed_from, uint16 packed_to);
extern uint8 Tile_GetDirectionPacked(uint16 packed_from, uint16 packed_to);
extern tile32 Tile_MoveByDirection(tile32 tile, int16 orientation, uint16 distance);
extern tile32 Tile_MoveByDirectionUnlimited(tile32 tile, int16 orientation, uint16 distance);
extern tile32 Tile_MoveByRandom(tile32 tile, uint16 distance, bool arg0C);
extern int8 Tile_GetDirection(tile32 from, tile32 to);
extern tile32 Tile_MoveByOrientation(tile32 position, uint8 orientation);

#endif /* TILE_H */
