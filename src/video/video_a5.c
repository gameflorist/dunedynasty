/* video_a5.c */

#include <assert.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>

#include "video_a5.h"

#include "../common_a5.h"
#include "../file.h"
#include "../gfx.h"
#include "../gui/gui.h"
#include "../gui/widget.h"
#include "../house.h"
#include "../input/input_a5.h"
#include "../sprites.h"

#define OUTPUT_TEXTURES     false
#define ICONID_MAX          512

static ALLEGRO_DISPLAY *display;
static ALLEGRO_DISPLAY *display2;
static ALLEGRO_BITMAP *screen;
static ALLEGRO_COLOR paltoRGB[256];
static unsigned char paletteRGB[3 * 256];

static ALLEGRO_BITMAP *icon_texture;

bool
VideoA5_Init(void)
{
	const int w = g_widgetProperties[WINDOWID_RENDER_TEXTURE].width*8;
	const int h = g_widgetProperties[WINDOWID_RENDER_TEXTURE].height;

	display = al_create_display(SCREEN_WIDTH, SCREEN_HEIGHT);
	display2 = al_create_display(SCREEN_WIDTH, SCREEN_HEIGHT);
	if (display == NULL || display2 == NULL)
		return false;

	screen = al_create_bitmap(SCREEN_WIDTH, SCREEN_HEIGHT);
	icon_texture = al_create_bitmap(w, h);
	if (screen == NULL || icon_texture == NULL)
		return false;

	al_register_event_source(g_a5_input_queue, al_get_display_event_source(display));
	al_hide_mouse_cursor(display);

	al_init_image_addon();

	return true;
}

void
VideoA5_Uninit(void)
{
	al_destroy_bitmap(icon_texture);
	icon_texture = NULL;

	al_destroy_bitmap(screen);
	screen = NULL;
}

static void
VideoA5_CopyBitmap(const unsigned char *raw, ALLEGRO_BITMAP *dest, bool transparent)
{
	const int w = al_get_bitmap_width(dest);
	const int h = al_get_bitmap_height(dest);

	ALLEGRO_LOCKED_REGION *reg;

	reg = al_lock_bitmap(dest, ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, ALLEGRO_LOCK_WRITEONLY);

	for (int y = 0; y < h; y++) {
		unsigned char *row = &((unsigned char *)reg->data)[reg->pitch*y];

		for (int x = 0; x < w; x++) {
			const unsigned char c = raw[w*y + x];

			if (transparent && (c == 0)) {
				row[reg->pixel_size*x + 0] = 0x00;
				row[reg->pixel_size*x + 1] = 0x00;
				row[reg->pixel_size*x + 2] = 0x00;
				row[reg->pixel_size*x + 3] = 0x00;
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

void
VideoA5_Tick(void)
{
	const unsigned char *raw = GFX_Screen_Get_ByIndex(0);

	al_set_target_backbuffer(display2);
	VideoA5_CopyBitmap(raw, screen, false);
	al_draw_bitmap(screen, 0, 0, 0);
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

			if ((idx >= num_common) || (houseID == HOUSE_HARKONNEN)) {
				if (x + TILE_SIZE - 1 >= WINDOW_W) {
					x = 0;
					y += TILE_SIZE + 1;

					if (y + TILE_SIZE - 1 >= WINDOW_H)
						exit(1);
				}

				GFX_DrawSprite(iconID, x, y, houseID);

				x += TILE_SIZE + 1;
			}
		}
	}

	*retx = x;
	*rety = y;
}

void
VideoA5_InitSprites(void)
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

	const int w = g_widgetProperties[WINDOWID_RENDER_TEXTURE].width*8;
	const int h = g_widgetProperties[WINDOWID_RENDER_TEXTURE].height;
	const uint16 old_screen = GFX_Screen_SetActive(0);
	const enum WindowID old_widget = Widget_SetCurrentWidget(WINDOWID_RENDER_TEXTURE);

	unsigned char *buf = GFX_Screen_GetActive();
	int x = 0, y = 0;

	al_set_target_bitmap(icon_texture);
	al_clear_to_color(al_map_rgba(0, 0, 0, 0));
	memset(buf, 0, w*h);

	File_ReadBlockFile("IBM.PAL", paletteRGB, 3 * 256);
	VideoA5_SetPalette(paletteRGB, 0, 256);

	for (int i = 0; icon_data[i].group < ICM_ICONGROUP_EOF; i++) {
		VideoA5_ExportIconGroup(icon_data[i].group, icon_data[i].num_common,
				x, y, &x, &y);
	}

	VideoA5_CopyBitmap(buf, icon_texture, true);

#if OUTPUT_TEXTURES
	al_save_bitmap("icons.png", icon_texture);
#endif

	GFX_Screen_SetActive(old_screen);
	Widget_SetCurrentWidget(old_widget);
}
