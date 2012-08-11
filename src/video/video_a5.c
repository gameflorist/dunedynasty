/* video_a5.c */

#include <assert.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include <stdio.h>
#include "../os/math.h"

#include "video_a5.h"

#include "../common_a5.h"
#include "../file.h"
#include "../gfx.h"
#include "../gui/font.h"
#include "../gui/gui.h"
#include "../input/input_a5.h"
#include "../input/mouse.h"
#include "../sprites.h"

#include "dune2_16x16.xpm"
/* #include "dune2_32x32.xpm" */
/* #include "dune2_32x32a.xpm" */

#define OUTPUT_TEXTURES     false
#define ICONID_MAX          512
#define SHAPEID_MAX         640
#define FONTID_MAX          8
#define CURSOR_MAX          6
#define WINDTRAP_COLOUR     223

static const uint8 font_palette[][8] = {
	{ 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, /* No outline. */
	{ 0x00, 0xFF, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00 }, /* Shadow. */
	{ 0x00, 0xFF, 0x0C, 0x0C, 0x00, 0x00, 0x00, 0x00 }, /* Outline. */
	{ 0x00, 0xFF, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0x00 }, /* Intro. */
};

static ALLEGRO_DISPLAY *display;
static ALLEGRO_DISPLAY *display2;
static ALLEGRO_BITMAP *screen;
static ALLEGRO_COLOR paltoRGB[256];
static unsigned char paletteRGB[3 * 256];

static ALLEGRO_BITMAP *icon_texture;
static ALLEGRO_BITMAP *shape_texture;
static ALLEGRO_BITMAP *font_texture;
static ALLEGRO_BITMAP *s_icon[ICONID_MAX][HOUSE_MAX];
static ALLEGRO_BITMAP *s_shape[SHAPEID_MAX][HOUSE_MAX];
static ALLEGRO_BITMAP *s_font[FONTID_MAX][256];
static ALLEGRO_MOUSE_CURSOR *s_cursor[CURSOR_MAX];

/* VideoA5_GetNextXY:
 *
 * Returns (x, y) if the sprite will fit into the texture at (x, y).
 * Otherwise, returns (0, y + row_h + 1), where row_h is the maximum
 * height of any sprite in the same row.
 */
static void
VideoA5_GetNextXY(int texture_width, int texture_height,
		int x, int y, int w, int h, int row_h, int *retx, int *rety)
{
	if (x + w - 1 >= texture_width) {
		x = 0;
		y += row_h + 1;
	}

	if (y + h - 1 >= texture_height)
		exit(1);

	*retx = x;
	*rety = y;
}

static void
VideoA5_ReadPalette(const char *filename)
{
	File_ReadBlockFile(filename, paletteRGB, 3 * 256);
	VideoA5_SetPalette(paletteRGB, 0, 256);
}

static void
VideoA5_InitDisplayIcon(char **xpm, int w, int h, int colours)
{
	struct {
		unsigned char r, g, b;
	} map[256];

	ALLEGRO_BITMAP *icon;
	ALLEGRO_LOCKED_REGION *reg;

	icon = al_create_bitmap(w, h);
	if (icon == NULL)
		return;

	for (int ln = 2; ln < 1 + colours; ln++) {
		unsigned char sym;
		unsigned int c;

		sscanf(xpm[ln], "%c c #%x", &sym, &c);

		map[sym].r = (c >> 16);
		map[sym].g = (c >> 8) & 0xFF;
		map[sym].b = c & 0xFF;
	}

	reg = al_lock_bitmap(icon, ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, ALLEGRO_LOCK_WRITEONLY);

	for (int y = 0; y < h; y++) {
		unsigned char *row = &((unsigned char *)reg->data)[reg->pitch*y];

		for (int x = 0; x < w; x++) {
			const unsigned char c = xpm[1 + colours + y][x];

			if (c == ' ') {
				row[reg->pixel_size*x + 0] = 0x00;
				row[reg->pixel_size*x + 1] = 0x00;
				row[reg->pixel_size*x + 2] = 0x00;
				row[reg->pixel_size*x + 3] = 0x00;
			}
			else {
				row[reg->pixel_size*x + 0] = map[c].r;
				row[reg->pixel_size*x + 1] = map[c].g;
				row[reg->pixel_size*x + 2] = map[c].b;
				row[reg->pixel_size*x + 3] = 0xFF;
			}
		}
	}

	al_unlock_bitmap(icon);

	al_set_display_icon(display, icon);
	al_destroy_bitmap(icon);
}

bool
VideoA5_Init(void)
{
	const int w = g_widgetProperties[WINDOWID_RENDER_TEXTURE].width*8;
	const int h = g_widgetProperties[WINDOWID_RENDER_TEXTURE].height;

	display = al_create_display(TRUE_DISPLAY_WIDTH, TRUE_DISPLAY_HEIGHT);
	display2 = al_create_display(SCREEN_WIDTH, SCREEN_HEIGHT);
	if (display == NULL || display2 == NULL)
		return false;

	VideoA5_InitDisplayIcon(dune2_16x16_xpm, 16, 16, 32);
	/* VideoA5_InitDisplayIcon(dune2_32x32_xpm, 32, 32, 23); */
	/* VideoA5_InitDisplayIcon(dune2_32x32a_xpm, 32, 32, 13); */

	screen = al_create_bitmap(SCREEN_WIDTH, SCREEN_HEIGHT);
	icon_texture = al_create_bitmap(w, h);
	shape_texture = al_create_bitmap(w, h);
	font_texture = al_create_bitmap(w, h);
	if (screen == NULL || icon_texture == NULL || shape_texture == NULL || font_texture == NULL)
		return false;

	al_register_event_source(g_a5_input_queue, al_get_display_event_source(display));

	al_init_image_addon();
	al_init_primitives_addon();

	return true;
}

void
VideoA5_Uninit(void)
{
	for (enum HouseType houseID = HOUSE_HARKONNEN; houseID < HOUSE_MAX; houseID++) {
		for (uint16 iconID = 0; iconID < ICONID_MAX; iconID++) {
			if (s_icon[iconID][houseID] != NULL) {
				/* Don't destroy in the case of shared sub-bitmaps. */
				if ((houseID + 1 == HOUSE_MAX) || (s_icon[iconID][houseID] != s_icon[iconID][houseID + 1]))
					al_destroy_bitmap(s_icon[iconID][houseID]);

				s_icon[iconID][houseID] = NULL;
			}
		}

		for (enum ShapeID shapeID = 0; shapeID < SHAPEID_MAX; shapeID++) {
			if (s_shape[shapeID][houseID] != NULL) {
				if ((houseID + 1 == HOUSE_MAX) || (s_shape[shapeID][houseID] != s_shape[shapeID][houseID + 1]))
					al_destroy_bitmap(s_shape[shapeID][houseID]);

				s_shape[shapeID][houseID] = NULL;
			}
		}
	}

	for (int fnt = 0; fnt < FONTID_MAX; fnt++) {
		for (int c = 0; c < 256; c++) {
			al_destroy_bitmap(s_font[fnt][c]);
			s_font[fnt][c] = NULL;
		}
	}

	for (int i = 0; i < CURSOR_MAX; i++) {
		al_destroy_mouse_cursor(s_cursor[i]);
		s_cursor[i] = NULL;
	}

	al_destroy_bitmap(icon_texture);
	icon_texture = NULL;

	al_destroy_bitmap(shape_texture);
	shape_texture = NULL;

	al_destroy_bitmap(font_texture);
	font_texture = NULL;

	al_destroy_bitmap(screen);
	screen = NULL;
}

static void
VideoA5_CopyBitmap(const unsigned char *raw, ALLEGRO_BITMAP *dest, bool writeonly)
{
	const int w = al_get_bitmap_width(dest);
	const int h = al_get_bitmap_height(dest);

	ALLEGRO_LOCKED_REGION *reg;

	if (writeonly) {
		reg = al_lock_bitmap(dest, ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, ALLEGRO_LOCK_WRITEONLY);
	}
	else {
		reg = al_lock_bitmap(dest, ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, ALLEGRO_LOCK_READWRITE);
	}

	for (int y = 0; y < h; y++) {
		unsigned char *row = &((unsigned char *)reg->data)[reg->pitch*y];

		for (int x = 0; x < w; x++) {
			const unsigned char c = raw[w*y + x];

			if (c == 0) {
				if (writeonly) {
					row[reg->pixel_size*x + 0] = 0x00;
					row[reg->pixel_size*x + 1] = 0x00;
					row[reg->pixel_size*x + 2] = 0x00;
					row[reg->pixel_size*x + 3] = 0x00;
				}
			}
			else {
				row[reg->pixel_size*x + 0] = paletteRGB[3*c + 0];
				row[reg->pixel_size*x + 1] = paletteRGB[3*c + 1];
				row[reg->pixel_size*x + 2] = paletteRGB[3*c + 2];
				row[reg->pixel_size*x + 3] = 0xFF;
			}
		}
	}

	al_unlock_bitmap(dest);
}

static void
VideoA5_CreateWhiteMask(unsigned char *src, ALLEGRO_LOCKED_REGION *reg,
		int src_stride, int sx, int sy, int dx, int dy, int w, int h, int ref)
{
	for (int y = 0; y < h; y++) {
		unsigned char *row = &((unsigned char *)reg->data)[reg->pitch * (dy + y)];

		for (int x = 0; x < w; x++) {
			const unsigned char c = src[src_stride * (sy + y) + (sx + x)];

			if (c == ref) {
				row[reg->pixel_size * (dx + x) + 0] = 0xFF;
				row[reg->pixel_size * (dx + x) + 1] = 0xFF;
				row[reg->pixel_size * (dx + x) + 2] = 0xFF;
				row[reg->pixel_size * (dx + x) + 3] = 0xFF;
			}
			else {
				row[reg->pixel_size * (dx + x) + 0] = 0x00;
				row[reg->pixel_size * (dx + x) + 1] = 0x00;
				row[reg->pixel_size * (dx + x) + 2] = 0x00;
				row[reg->pixel_size * (dx + x) + 3] = 0x00;
			}
		}
	}
}

void
VideoA5_Tick(void)
{
	const unsigned char *raw = GFX_Screen_Get_ByIndex(0);

	al_set_target_backbuffer(display2);
	VideoA5_CopyBitmap(raw, screen, false);
	al_draw_bitmap(screen, 0, 0, 0);
	VideoA5_DrawShape(0, 0, g_mouseX, g_mouseY, 0);
	al_flip_display();

	InputA5_Tick();

	al_set_target_backbuffer(display);
	al_flip_display();
}

void
VideoA5_SetPalette(const uint8 *palette, int from, int length)
{
	const uint8 *p = palette;
	assert(from + length <= 256);

	for (int i = from; i < from + length; i++) {
		const uint8 r = ((*p++) & 0x3F) * 4;
		const uint8 g = ((*p++) & 0x3F) * 4;
		const uint8 b = ((*p++) & 0x3F) * 4;

		paltoRGB[i] = al_map_rgb(r, g, b);
		paletteRGB[3*i + 0] = r;
		paletteRGB[3*i + 1] = g;
		paletteRGB[3*i + 2] = b;
	}
}

void
VideoA5_SetClippingArea(int x, int y, int w, int h)
{
	al_set_clipping_rectangle(x, y, w, h);
}

void
VideoA5_SetCursor(int spriteID)
{
	assert(spriteID < CURSOR_MAX);

	g_cursorSpriteID = spriteID;
	al_set_mouse_cursor(display, s_cursor[spriteID]);
}

/*--------------------------------------------------------------*/

void
VideoA5_PutPixel(int x, int y, uint8 c)
{
	al_draw_pixel(x + 0.5f, y + 0.5f, paltoRGB[c]);
}

void
VideoA5_DrawLine(int x1, int y1, int x2, int y2, uint8 c)
{
	if (y1 == y2) {
		al_draw_line(x1, y1 + 0.5f, x2 + 0.99f, y2 + 0.5f, paltoRGB[c], 1.0f);
	}
	else if (x1 == x2) {
		al_draw_line(x1 + 0.5f, y1, x2 + 0.5f, y2 + 0.99f, paltoRGB[c], 1.0f);
	}
	else {
		al_draw_line(x1 + 0.5f, y1 + 0.5f, x2 + 0.5f, y2 + 0.5f, paltoRGB[c], 1.0f);
	}
}

void
VideoA5_DrawRectangle(int x1, int y1, int x2, int y2, uint8 c)
{
	al_draw_rectangle(x1 + 0.01f, y1 + 0.01f, x2 + 0.99f, y2 + 0.99f, paltoRGB[c], 1.0f);
}

void
VideoA5_DrawFilledRectangle(int x1, int y1, int x2, int y2, uint8 c)
{
	al_draw_filled_rectangle(x1 + 0.01f, y1 + 0.01f, x2 + 0.99f, y2 + 0.99f, paltoRGB[c]);
}

/*--------------------------------------------------------------*/

static int
VideoA5_NumIconsInGroup(enum IconMapEntries group)
{
	if (!(0 < group && group < ICM_ICONGROUP_EOF))
		return 0;

	/* group == ICM_ICONGROUP_RADAR_OUTPOST. */
	if (group + 1 == ICM_ICONGROUP_EOF) {
		return 24;
	}
	else {
		return g_iconMap[group + 1] - g_iconMap[group];
	}
}

static void
VideoA5_ExportIconGroup(enum IconMapEntries group, int num_common,
		int x, int y, int *retx, int *rety)
{
	const int WINDOW_W = g_widgetProperties[WINDOWID_RENDER_TEXTURE].width*8;
	const int WINDOW_H = g_widgetProperties[WINDOWID_RENDER_TEXTURE].height;
	const int num = VideoA5_NumIconsInGroup(group);

	if (num_common < 0)
		num_common = num;

	for (enum HouseType houseID = HOUSE_HARKONNEN; houseID < HOUSE_MAX; houseID++) {
		GUI_Palette_CreateRemap(houseID);

		for (int idx = 0; idx < num; idx++) {
			const uint16 iconID = g_iconMap[g_iconMap[group] + idx];
			assert(iconID < ICONID_MAX);

			if (s_icon[iconID][houseID] != NULL)
				continue;

			if ((idx >= num_common) || (houseID == HOUSE_HARKONNEN)) {
				VideoA5_GetNextXY(WINDOW_W, WINDOW_H, x, y, TILE_SIZE, TILE_SIZE, TILE_SIZE, &x, &y);
				GFX_DrawSprite_(iconID, x, y, houseID);

				s_icon[iconID][houseID] = al_create_sub_bitmap(icon_texture, x, y, TILE_SIZE, TILE_SIZE);
				assert(s_icon[iconID][houseID] != NULL);

				x += TILE_SIZE + 1;
			}
			else {
				s_icon[iconID][houseID] = s_icon[iconID][HOUSE_HARKONNEN];
			}
		}
	}

	*retx = x;
	*rety = y;
}

static void
VideoA5_ExportWindtrapOverlay(unsigned char *buf, uint16 iconID,
		int x, int y, int *retx, int *rety)
{
	const int WINDOW_W = g_widgetProperties[WINDOWID_RENDER_TEXTURE].width*8;
	const int WINDOW_H = g_widgetProperties[WINDOWID_RENDER_TEXTURE].height;
	const int idx = ICONID_MAX - (iconID - 304) - 1;
	assert(304 <= iconID && iconID <= 308);
	assert(s_icon[idx][HOUSE_HARKONNEN] == NULL);

	VideoA5_GetNextXY(WINDOW_W, WINDOW_H, x, y, TILE_SIZE, TILE_SIZE, TILE_SIZE, &x, &y);
	GFX_DrawSprite_(iconID, x, y, HOUSE_HARKONNEN);

	s_icon[idx][HOUSE_HARKONNEN] = al_create_sub_bitmap(icon_texture, x, y, TILE_SIZE, TILE_SIZE);
	assert(s_icon[idx][HOUSE_HARKONNEN] != NULL);

	ALLEGRO_LOCKED_REGION *reg = al_lock_bitmap(s_icon[idx][HOUSE_HARKONNEN], ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, ALLEGRO_LOCK_WRITEONLY);
	VideoA5_CreateWhiteMask(buf, reg, WINDOW_W, x, y, 0, 0, TILE_SIZE, TILE_SIZE, WINDTRAP_COLOUR);
	al_unlock_bitmap(s_icon[idx][HOUSE_HARKONNEN]);

	*retx = x + TILE_SIZE + 1;
	*rety = y;
}

static void
VideoA5_InitIcons(unsigned char *buf)
{
	const struct {
		int num_common;
		enum IconMapEntries group;
	} icon_data[] = {
		{ -1, ICM_ICONGROUP_ROCK_CRATERS },
		{ -1, ICM_ICONGROUP_SAND_CRATERS },
		{ -1, ICM_ICONGROUP_FLY_MACHINES_CRASH },
		{ -1, ICM_ICONGROUP_SAND_TRACKS },
		{ -1, ICM_ICONGROUP_WALLS },
		{ -1, ICM_ICONGROUP_FOG_OF_WAR },
		{ -1, ICM_ICONGROUP_CONCRETE_SLAB },
		{ -1, ICM_ICONGROUP_LANDSCAPE },
		{ -1, ICM_ICONGROUP_SPICE_BLOOM },
		{ -1, ICM_ICONGROUP_BASE_DEFENSE_TURRET },
		{ -1, ICM_ICONGROUP_BASE_ROCKET_TURRET },

		{  0, ICM_ICONGROUP_SAND_DEAD_BODIES },
		{ 18, ICM_ICONGROUP_HOUSE_PALACE },
		{  8, ICM_ICONGROUP_LIGHT_VEHICLE_FACTORY },
		{ 12, ICM_ICONGROUP_HEAVY_VEHICLE_FACTORY },
		{ 12, ICM_ICONGROUP_HI_TECH_FACTORY },
		{  8, ICM_ICONGROUP_IX_RESEARCH },
		{  8, ICM_ICONGROUP_WOR_TROOPER_FACILITY },
		{  8, ICM_ICONGROUP_CONSTRUCTION_YARD },
		{  8, ICM_ICONGROUP_INFANTRY_BARRACKS },
		{  8, ICM_ICONGROUP_WINDTRAP_POWER },
		{ 18, ICM_ICONGROUP_STARPORT_FACILITY },
		{ 12, ICM_ICONGROUP_SPICE_REFINERY },
		{ 12, ICM_ICONGROUP_VEHICLE_REPAIR_CENTRE },
		{  8, ICM_ICONGROUP_SPICE_STORAGE_SILO },
		{  8, ICM_ICONGROUP_RADAR_OUTPOST },

		{  0, ICM_ICONGROUP_EOF }
	};

	int x = 0, y = 0;

	al_set_target_bitmap(icon_texture);
	al_clear_to_color(al_map_rgba(0, 0, 0, 0));

	for (int i = 0; icon_data[i].group < ICM_ICONGROUP_EOF; i++) {
		VideoA5_ExportIconGroup(icon_data[i].group, icon_data[i].num_common,
				x, y, &x, &y);
	}

	VideoA5_CopyBitmap(buf, icon_texture, true);

	/* Windtraps. */
	for (uint16 iconID = 304; iconID <= 308; iconID++) {
		VideoA5_ExportWindtrapOverlay(buf, iconID, x, y, &x, &y);
	}

#if OUTPUT_TEXTURES
	al_save_bitmap("icons.png", icon_texture);
#endif
}

void
VideoA5_DrawIcon(uint16 iconID, enum HouseType houseID, int x, int y)
{
	assert(iconID < ICONID_MAX);
	assert(houseID < HOUSE_MAX);
	assert(s_icon[iconID][houseID] != NULL);

	al_draw_bitmap(s_icon[iconID][houseID], x, y, 0);

	/* Windtraps. */
	if (304 <= iconID && iconID <= 308) {
		const int idx = ICONID_MAX - (iconID - 304) - 1;
		assert(s_icon[idx][HOUSE_HARKONNEN] != NULL);

		al_draw_tinted_bitmap(s_icon[idx][HOUSE_HARKONNEN], paltoRGB[WINDTRAP_COLOUR], x, y, 0);
	}
}

/*--------------------------------------------------------------*/

static ALLEGRO_BITMAP *
VideoA5_ExportShape(enum ShapeID shapeID, int x, int y, int row_h,
		int *retx, int *rety, int *ret_row_h, unsigned char *remap)
{
	const int WINDOW_W = g_widgetProperties[WINDOWID_RENDER_TEXTURE].width*8;
	const int WINDOW_H = g_widgetProperties[WINDOWID_RENDER_TEXTURE].height;
	const int w = Shape_Width(shapeID);
	const int h = Shape_Height(shapeID);

	ALLEGRO_BITMAP *bmp;

	VideoA5_GetNextXY(WINDOW_W, WINDOW_H, x, y, w, h, row_h, &x, &y);
	GUI_DrawSprite_(0, g_sprites[shapeID], x, y, WINDOWID_RENDER_TEXTURE, 0x100, remap, 1);

	bmp = al_create_sub_bitmap(shape_texture, x, y, w, h);
	assert(bmp != NULL);

	*retx = x + w + 1;
	*rety = y;
	*ret_row_h = max(row_h, h);
	return bmp;
}

static void
VideoA5_InitShapes(unsigned char *buf)
{
	/* Check Sprites_Init. */
	const struct {
		int start, end;
		bool remap;
	} shape_data[] = {
		{   0,   6, false }, /* MOUSE.SHP */
		{  12, 110, false }, /* SHAPES.SHP */
		{   7,  11,  true }, /* BTTN */
		{ 355, 372,  true }, /* CHOAM */
		{ 111, 150,  true }, /* UNITS2.SHP */
		{ 151, 161, false }, /* UNITS1.SHP */
		{ 162, 167,  true }, /* UNITS1.SHP: tanks */
		{ 168, 237, false }, /* UNITS1.SHP */
		{ 238, 257,  true }, /* UNITS.SHP: quad .. mcv */
		{ 258, 282, false }, /* UNITS.SHP: rockets */
		{ 283, 354,  true }, /* UNITS.SHP: carry-all .. landed ornithoper */
		{ 373, 386, false }, /* MENTAT */
		{ 387, 401, false }, /* MENSHPH.SHP */
		{ 402, 416, false }, /* MENSHPA.SHP */
		{ 417, 431, false }, /* MENSHPO.SHP */
		/*477, 504,  true */ /* PIECES.SHP */
		/*505, 513, false */ /* ARROWS.SHP */
		/*514, 524, false */ /* CREDIT1.SHP .. CREDIT11.SHP */

		/* BENE.PAL shapes. */
		{  -2,   0, false },
		/*432, 461, false */ /* MENSHPM.SHP: Fremen, Sardaukar */
		{ 462, 476, false }, /* MENSHPM.SHP */

		{  -1,   0, false }
	};

	const int WINDOW_W = g_widgetProperties[WINDOWID_RENDER_TEXTURE].width*8;
	const int WINDOW_H = g_widgetProperties[WINDOWID_RENDER_TEXTURE].height;

	int x = 0, y = 0, row_h = 0;
	unsigned char greymap[256];

	uint8 fileID = File_Open("GRAYRMAP.TBL", 1);
	assert(fileID != FILE_INVALID);

	File_Read(fileID, greymap, 256);
	File_Close(fileID);

	al_set_target_bitmap(shape_texture);
	al_clear_to_color(al_map_rgba(0, 0, 0, 0));

	for (int group = 0; shape_data[group].start != -1; group++) {
		if (shape_data[group].start == -2) {
			VideoA5_CopyBitmap(buf, shape_texture, true);
			VideoA5_ReadPalette("BENE.PAL");
			memset(buf, 0, WINDOW_W * WINDOW_H);
			continue;
		}

		for (enum HouseType houseID = HOUSE_HARKONNEN; houseID < HOUSE_MAX; houseID++) {
			GUI_Palette_CreateRemap(houseID);

			for (uint16 shapeID = shape_data[group].start; shapeID <= shape_data[group].end; shapeID++) {
				assert(shapeID < SHAPEID_MAX);

				if ((shape_data[group].remap) || (houseID == HOUSE_HARKONNEN)) {
					s_shape[shapeID][houseID] = VideoA5_ExportShape(shapeID, x, y, row_h, &x, &y, &row_h, g_remap);

					if (SHAPE_CONCRETE_SLAB <= shapeID && shapeID <= SHAPE_FREMEN) {
						const enum ShapeID greyID = SHAPE_CONCRETE_SLAB_GREY + (shapeID - SHAPE_CONCRETE_SLAB);

						s_shape[greyID][houseID] = VideoA5_ExportShape(shapeID, x, y, row_h, &x, &y, &row_h, greymap);
					}
				}
				else {
					s_shape[shapeID][houseID] = s_shape[shapeID][HOUSE_HARKONNEN];
				}
			}
		}
	}

	VideoA5_CopyBitmap(buf, shape_texture, false);

#if OUTPUT_TEXTURES
	al_save_bitmap("shapes.png", shape_texture);
#endif

	VideoA5_ReadPalette("IBM.PAL");
}

void
VideoA5_DrawShape(enum ShapeID shapeID, enum HouseType houseID, int x, int y, int flags)
{
	assert(shapeID < SHAPEID_MAX);
	assert(houseID < HOUSE_MAX);
	assert(s_shape[shapeID][houseID] != NULL);

	int al_flags = 0;

	if (flags & 0x01) al_flags |= ALLEGRO_FLIP_HORIZONTAL;
	if (flags & 0x02) al_flags |= ALLEGRO_FLIP_VERTICAL;

	al_draw_bitmap(s_shape[shapeID][houseID], x, y, al_flags);
}

void
VideoA5_DrawShapeGrey(enum ShapeID shapeID, int x, int y, int flags)
{
	const enum ShapeID greyID = SHAPE_CONCRETE_SLAB_GREY + (shapeID - SHAPE_CONCRETE_SLAB);
	assert(SHAPE_CONCRETE_SLAB <= shapeID && shapeID <= SHAPE_FREMEN);

	VideoA5_DrawShape(greyID, HOUSE_HARKONNEN, x, y, flags);
}

void
VideoA5_DrawShapeTint(enum ShapeID shapeID, int x, int y, unsigned char c, int flags)
{
	assert(s_shape[shapeID][HOUSE_HARKONNEN] != NULL);

	int al_flags = 0;

	if (flags & 0x01) al_flags |= ALLEGRO_FLIP_HORIZONTAL;
	if (flags & 0x02) al_flags |= ALLEGRO_FLIP_VERTICAL;

	al_draw_tinted_bitmap(s_shape[shapeID][HOUSE_HARKONNEN], paltoRGB[c], x, y, al_flags);
}

/*--------------------------------------------------------------*/

static int
VideoA5_FontIndex(const Font *font, const uint8 *pal)
{
	if (font == g_fontNew6p) {
		/* 4 colour font. */
		if (memcmp(pal+2, font_palette[0]+2, 2) == 0) return 0;
		if (memcmp(pal+2, font_palette[1]+2, 2) == 0) return 1;
		if (memcmp(pal+2, font_palette[2]+2, 2) == 0) return 2;
	}
	else if (font == g_fontNew8p) {
		/* 4 colour font. */
		if (memcmp(pal+2, font_palette[0]+2, 2) == 0) return 3;
		if (memcmp(pal+2, font_palette[1]+2, 2) == 0) return 4;
		if (memcmp(pal+2, font_palette[2]+2, 2) == 0) return 5;
	}
	else {
		/* 7 colour font. */
		assert(font == g_fontIntro);
		if (memcmp(pal+2, font_palette[0]+2, 5) == 0) return 6;
		if (memcmp(pal+2, font_palette[3]+2, 5) == 0) return 7;
	}

	{
		const char *name =
			(font == g_fontNew6p) ? "New6p" :
			(font == g_fontNew8p) ? "New8p" : "Intro";

		const int end = (font == g_fontIntro) ? 7 : 4;

		fprintf(stderr, "Untreated palette for font g_font%s\n  { 0x00, 0xFF", name);

		for (int i = 2; i < end; i++)
			fprintf(stderr, ", 0x%02x", pal[i]);

		for (int i = end; i < 8; i++)
			fprintf(stderr, ", 0x00");

		fprintf(stderr, " },\n");
		exit(1);
	}
}

static void
VideoA5_ExportFont(Font *font, const uint8 *pal, int y, int *rety)
{
	const int WINDOW_W = g_widgetProperties[WINDOWID_RENDER_TEXTURE].width*8;
	const int WINDOW_H = g_widgetProperties[WINDOWID_RENDER_TEXTURE].height;
	const int fnt = VideoA5_FontIndex(font, pal);

	int x = 0;

	Font_Select(font);
	GUI_InitColors(pal, 0, 15);

	for (int c = 0; c < 256; c++) {
		if ((c < font->count) && (font->chars[c].data != NULL)) {
			/* Image width is Font_GetCharWidth(c) + 1. */
			const int w = Font_GetCharWidth(c) + 1;

			VideoA5_GetNextXY(WINDOW_W, WINDOW_H, x, y, w, font->height, font->height, &x, &y);
			GUI_DrawChar_(c, x, y);

			s_font[fnt][c] = al_create_sub_bitmap(font_texture, x, y, w, font->height);
			assert(s_font[fnt][c] != NULL);

			x += w + 1;
		}
	}

	*rety = y + font->height + 1;
}

static void
VideoA5_InitFonts(unsigned char *buf)
{
	int y = 0;

	VideoA5_ExportFont(g_fontNew6p, font_palette[0], y, &y);
	VideoA5_ExportFont(g_fontNew6p, font_palette[1], y, &y);
	VideoA5_ExportFont(g_fontNew6p, font_palette[2], y, &y);
	VideoA5_ExportFont(g_fontNew8p, font_palette[0], y, &y);
	VideoA5_ExportFont(g_fontNew8p, font_palette[1], y, &y);
	VideoA5_ExportFont(g_fontNew8p, font_palette[2], y, &y);
	VideoA5_ExportFont(g_fontIntro, font_palette[0], y, &y);
	VideoA5_ExportFont(g_fontIntro, font_palette[3], y, &y);

	VideoA5_CopyBitmap(buf, font_texture, true);

#if OUTPUT_TEXTURES
	al_save_bitmap("fonts.png", font_texture);
#endif
}

void
VideoA5_DrawChar(unsigned char c, const uint8 *pal, int x, int y)
{
	const int fnt = VideoA5_FontIndex(g_fontCurrent, pal);
	const ALLEGRO_COLOR fg = paltoRGB[pal[1]];

	if (s_font[fnt][c] != NULL)
		al_draw_tinted_bitmap(s_font[fnt][c], fg, x, y, 0);
}

/*--------------------------------------------------------------*/

static void
VideoA5_InitCursor(void)
{
	/* From gui/viewport.c */
	const struct {
		int x, y;
	} focus[CURSOR_MAX] = {
		{ 0, 0 }, { 5, 0 }, { 8, 5 }, { 5, 8 }, { 0, 5 }, { 8, 8 }
	};

	for (int i = 0; i < CURSOR_MAX; i++) {
		s_cursor[i] = al_create_mouse_cursor(s_shape[i][HOUSE_HARKONNEN], focus[i].x, focus[i].y);
	}

	al_set_mouse_cursor(display, s_cursor[0]);
}

void
VideoA5_InitSprites(void)
{
	const int WINDOW_W = g_widgetProperties[WINDOWID_RENDER_TEXTURE].width*8;
	const int WINDOW_H = g_widgetProperties[WINDOWID_RENDER_TEXTURE].height;
	const uint16 old_screen = GFX_Screen_SetActive(0);
	const enum WindowID old_widget = Widget_SetCurrentWidget(WINDOWID_RENDER_TEXTURE);

	unsigned char *buf = GFX_Screen_GetActive();

	VideoA5_ReadPalette("IBM.PAL");

	memset(buf, 0, WINDOW_W * WINDOW_H);
	VideoA5_InitIcons(buf);

	memset(buf, 0, WINDOW_W * WINDOW_H);
	VideoA5_InitShapes(buf);

	memset(buf, 0, WINDOW_W * WINDOW_H);
	VideoA5_InitFonts(buf);

	VideoA5_InitCursor();

	al_set_target_backbuffer(display);

	GFX_Screen_SetActive(old_screen);
	Widget_SetCurrentWidget(old_widget);
}
