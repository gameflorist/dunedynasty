/* video_a5.c */

#include <assert.h>
#include <allegro5/allegro.h>

#include "video_a5.h"

#include "../gfx.h"
#include "../input/input_a5.h"

static ALLEGRO_DISPLAY *display;
static ALLEGRO_BITMAP *screen;
static ALLEGRO_COLOR paltoRGB[256];
static unsigned char paletteRGB[3 * 256];

bool
VideoA5_Init(void)
{
	display = al_create_display(SCREEN_WIDTH, SCREEN_HEIGHT);
	if (display == NULL)
		return false;

	screen = al_create_bitmap(SCREEN_WIDTH, SCREEN_HEIGHT);
	if (screen == NULL)
		return false;

	al_hide_mouse_cursor(display);

	return true;
}

void
VideoA5_Uninit(void)
{
	al_destroy_bitmap(screen);
	screen = NULL;
}

void
VideoA5_Tick(void)
{
	InputA5_Tick();

	const unsigned char *raw = GFX_Screen_Get_ByIndex(0);
	ALLEGRO_LOCKED_REGION *reg;

	reg = al_lock_bitmap(screen, ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, ALLEGRO_LOCK_WRITEONLY);

	for (int y = 0; y < al_get_bitmap_height(screen); y++) {
		unsigned char *row = &((unsigned char *)reg->data)[reg->pitch*y];

		for (int x = 0; x < al_get_bitmap_width(screen); x++) {
			const unsigned char c = raw[SCREEN_WIDTH*y + x];

			row[reg->pixel_size*x + 0] = paletteRGB[3*c + 0];
			row[reg->pixel_size*x + 1] = paletteRGB[3*c + 1];
			row[reg->pixel_size*x + 2] = paletteRGB[3*c + 2];
			row[reg->pixel_size*x + 3] = 0xff;
		}
	}

	al_unlock_bitmap(screen);

	al_set_target_backbuffer(display);
	al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);
	al_draw_bitmap(screen, 0, 0, 0);
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
