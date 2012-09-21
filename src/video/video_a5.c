/* video_a5.c
 *
 * Notes:
 *
 * CPS rendered by opendune on screen 2/3 (once).
 * WSA rendered by opendune on screen 0/1 (every frame).
 * WSA loaded in screen 4/5.
 *
 * This avoids clashes with each other and mentat.
 */

#include <assert.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include <stdio.h>
#include "../os/math.h"

#include "video_a5.h"

#include "../common_a5.h"
#include "../config.h"
#include "../file.h"
#include "../gfx.h"
#include "../gui/font.h"
#include "../gui/gui.h"
#include "../input/input_a5.h"
#include "../input/mouse.h"
#include "../newui/viewport.h"
#include "../opendune.h"
#include "../sprites.h"
#include "../tools.h"
#include "../wsa.h"

#include "dune2_16x16.xpm"
/* #include "dune2_32x32.xpm" */
/* #include "dune2_32x32a.xpm" */

#define OUTPUT_TEXTURES     false
#define ICONID_MAX          512
#define SHAPEID_MAX         640
#define FONTID_MAX          8
#define CURSOR_MAX          6
#define CONQUEST_COLOUR     146
#define WINDTRAP_COLOUR     223
#define ARROW_COLOUR        251

enum BitmapCopyMode {
	TRANSPARENT_COLOUR_0,
	BLACK_COLOUR_0,
	SKIP_COLOUR_0
};

typedef struct CPSStore {
	struct CPSStore *next;

	char filename[16];
	ALLEGRO_BITMAP *bmp;
} CPSStore;

typedef struct FadeInAux {
	bool fade_in;   /* false to fade out. */
	int frame;      /* 0 <= frame < height. */
	int width;
	int height;

	/* Persistent random data. */
	int cols[SCREEN_WIDTH];
	int rows[SCREEN_HEIGHT];
} FadeInAux;

static const struct CPSSpecialCoord {
	int cx, cy; /* coordinates in cps. */
	int tx, ty; /* coordinates in texture. */
	int w, h;
} cps_special_coord[CPS_SPECIAL_MAX] = {
	{   0,   0,   0,   0, 184, 17 }, /* CPS_MENUBAR_LEFT */
	{  -1,  -1,   0,  18, 320, 17 }, /* CPS_MENUBAR_MIDDLE */
	{ 184,   0, 185,   0, 136, 17 }, /* CPS_MENUBAR_RIGHT */
	{   0,  17,   0,  36,   8, 23 }, /* CPS_STATUSBAR_LEFT */
	{  -1,  -1,   9,  36, 425, 23 }, /* CPS_STATUSBAR_MIDDLE */
	{ 312,  17, 435,  36,   8, 23 }, /* CPS_STATUSBAR_RIGHT */
	{ 240,  40,   0,  60,  80, 17 }, /* CPS_SIDEBAR_TOP */
	{ 240,  65,   0,  78,  16, 52 }, /* CPS_SIDEBAR_MIDDLE */
	{ 240, 117,   0, 146,  80, 83 }, /* CPS_SIDEBAR_BOTTOM */
	{   8,   0, 192,  60, 304, 24 }, /* CPS_CONQUEST_EN */
	{   8,  96, 192, 110, 304, 24 }, /* CPS_CONQUEST_FR */
	{   8, 120, 192, 160, 304, 24 }, /* CPS_CONQUEST_DE */
};

static const uint8 font_palette[][8] = {
	{ 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, /* No outline. */
	{ 0x00, 0xFF, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00 }, /* Shadow. */
	{ 0x00, 0xFF, 0x0C, 0x0C, 0x00, 0x00, 0x00, 0x00 }, /* Outline. */
	{ 0x00, 0xFF, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0x00 }, /* Intro. */
};

/* Exposed for prim_a5.c. */
ALLEGRO_COLOR paltoRGB[256];

static ALLEGRO_DISPLAY *display;
/* static ALLEGRO_DISPLAY *display2; */
static ALLEGRO_BITMAP *screen;
static unsigned char paletteRGB[3 * 256];

static CPSStore *s_cps;
static ALLEGRO_BITMAP *cps_special_texture;
static ALLEGRO_BITMAP *icon_texture;
static ALLEGRO_BITMAP *shape_texture;
static ALLEGRO_BITMAP *region_texture;
static ALLEGRO_BITMAP *font_texture;
static ALLEGRO_BITMAP *wsa_special_texture;
static ALLEGRO_BITMAP *s_icon[ICONID_MAX][HOUSE_MAX];
static ALLEGRO_BITMAP *s_shape[SHAPEID_MAX][HOUSE_MAX];
static ALLEGRO_BITMAP *s_font[FONTID_MAX][256];
static ALLEGRO_MOUSE_CURSOR *s_cursor[CURSOR_MAX];

static bool take_screenshot = false;
static FadeInAux s_fadeInAux;

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
	const int w = g_widgetProperties[WINDOWID_RENDER_TEXTURE].width;
	const int h = g_widgetProperties[WINDOWID_RENDER_TEXTURE].height;
	int display_flags = ALLEGRO_OPENGL | ALLEGRO_GENERATE_EXPOSE_EVENTS;

	if (g_gameConfig.windowMode == WM_FULLSCREEN) {
		display_flags |= ALLEGRO_FULLSCREEN;
	}
	else if (g_gameConfig.windowMode == WM_FULLSCREEN_WINDOW) {
		display_flags |= ALLEGRO_FULLSCREEN_WINDOW;
	}

	al_set_new_display_flags(display_flags);
	display = al_create_display(TRUE_DISPLAY_WIDTH, TRUE_DISPLAY_HEIGHT);
	al_set_window_title(display, "Dune Dynasty");
	/* display2 = al_create_display(SCREEN_WIDTH, SCREEN_HEIGHT); */
	if (display == NULL)
		return false;

	TRUE_DISPLAY_WIDTH = al_get_display_width(display);
	TRUE_DISPLAY_HEIGHT = al_get_display_height(display);

	VideoA5_InitDisplayIcon(dune2_16x16_xpm, 16, 16, 32);
	/* VideoA5_InitDisplayIcon(dune2_32x32_xpm, 32, 32, 23); */
	/* VideoA5_InitDisplayIcon(dune2_32x32a_xpm, 32, 32, 13); */

	screen = al_create_bitmap(SCREEN_WIDTH, SCREEN_HEIGHT);
	cps_special_texture = al_create_bitmap(w, h);
	icon_texture = al_create_bitmap(w, h);
	shape_texture = al_create_bitmap(w, h);
	region_texture = al_create_bitmap(w, h);
	font_texture = al_create_bitmap(w, h);
	wsa_special_texture = al_create_bitmap(w, h);
	if (screen == NULL || cps_special_texture == NULL || icon_texture == NULL || shape_texture == NULL || region_texture == NULL || font_texture == NULL || wsa_special_texture == NULL)
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

	while (s_cps != NULL) {
		CPSStore *next = s_cps->next;

		al_destroy_bitmap(s_cps->bmp);
		free(s_cps);

		s_cps = next;
	}

	al_destroy_bitmap(cps_special_texture);
	cps_special_texture = NULL;

	al_destroy_bitmap(icon_texture);
	icon_texture = NULL;

	al_destroy_bitmap(shape_texture);
	shape_texture = NULL;

	al_destroy_bitmap(region_texture);
	region_texture = NULL;

	al_destroy_bitmap(font_texture);
	font_texture = NULL;

	al_destroy_bitmap(wsa_special_texture);
	wsa_special_texture = NULL;

	al_destroy_bitmap(screen);
	screen = NULL;
}

void
VideoA5_ToggleFullscreen(void)
{
	const bool fs = (al_get_display_flags(display) & ALLEGRO_FULLSCREEN_WINDOW);
	const int old_width = TRUE_DISPLAY_WIDTH;

	al_set_display_flag(display, ALLEGRO_FULLSCREEN_WINDOW, !fs);
	TRUE_DISPLAY_WIDTH = al_get_display_width(display);
	TRUE_DISPLAY_HEIGHT = al_get_display_height(display);

	A5_InitTransform();
	GameLoop_TweakWidgetDimensions(old_width);
}

void
VideoA5_CaptureScreenshot(void)
{
	take_screenshot = true;
}

static void
VideoA5_CopyBitmap(const unsigned char *raw, ALLEGRO_BITMAP *dest, enum BitmapCopyMode mode)
{
	const int w = al_get_bitmap_width(dest);
	const int h = al_get_bitmap_height(dest);

	ALLEGRO_LOCKED_REGION *reg;

	if (mode == SKIP_COLOUR_0) {
		reg = al_lock_bitmap(dest, ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, ALLEGRO_LOCK_READWRITE);
	}
	else {
		reg = al_lock_bitmap(dest, ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, ALLEGRO_LOCK_WRITEONLY);
	}

	for (int y = 0; y < h; y++) {
		unsigned char *row = &((unsigned char *)reg->data)[reg->pitch*y];

		for (int x = 0; x < w; x++) {
			const unsigned char c = raw[w*y + x];

			if (c == 0) {
				if (mode != SKIP_COLOUR_0) {
					row[reg->pixel_size*x + 0] = 0x00;
					row[reg->pixel_size*x + 1] = 0x00;
					row[reg->pixel_size*x + 2] = 0x00;
					row[reg->pixel_size*x + 3] = (mode == TRANSPARENT_COLOUR_0) ? 0x00 : 0xFF;
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
#if 0
	const unsigned char *raw = GFX_Screen_Get_ByIndex(0);

	al_set_target_backbuffer(display2);
	VideoA5_CopyBitmap(raw, screen, BLACK_COLOUR_0);
	al_draw_bitmap(screen, 0, 0, 0);
	VideoA5_DrawShape(0, 0, g_mouseX, g_mouseY, 0);
	al_flip_display();

	al_set_target_backbuffer(display);
#endif

	if (take_screenshot) {
		struct tm *tm;
		time_t timep;
		char filename[1024];
		char filepath[1024];

		take_screenshot = false;

		timep = time(NULL);
		tm = localtime(&timep);

		strftime(filename, sizeof(filename), "screenshot_%Y%m%d_%H%M%S.png", tm);
		snprintf(filepath, sizeof(filepath), "%s/%s", g_personal_data_dir, filename);

		al_save_bitmap(filepath, al_get_backbuffer(display));
		fprintf(stdout, "screenshot: %s\n", filepath);
	}

	al_flip_display();
	al_clear_to_color(paltoRGB[0]);
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

void
VideoA5_ShowCursor(void)
{
	al_show_mouse_cursor(display);
}

void
VideoA5_HideCursor(void)
{
	al_hide_mouse_cursor(display);
}

/*--------------------------------------------------------------*/

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
VideoA5_ShadeScreen(int alpha)
{
	const bool prev_transform = A5_SaveTransform();

	alpha = clamp(0x00, alpha, 0xFF);

	A5_UseIdentityTransform();
	al_draw_filled_rectangle(0.0f, 0.0f, TRUE_DISPLAY_WIDTH, TRUE_DISPLAY_HEIGHT, al_map_rgba(0, 0, 0, alpha));

	A5_RestoreTransform(prev_transform);
}

/*--------------------------------------------------------------*/

static FadeInAux *
VideoA5_InitFadeInSprite(int w, int h, bool fade_in)
{
	FadeInAux *aux = &s_fadeInAux;
	assert(w < SCREEN_WIDTH);
	assert(h < SCREEN_HEIGHT);

	ALLEGRO_LOCKED_REGION *reg = al_lock_bitmap_region(screen, 0, 0, w, h, ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, ALLEGRO_LOCK_READWRITE);
	for (int y = 0; y < h; y++) {
		unsigned char *row = &((unsigned char *)reg->data)[reg->pitch*y];

		for (int x = 0; x < w; x++) {
			row[reg->pixel_size*x + 3] = fade_in ? 0x00 : 0xFF;
		}
	}
	al_unlock_bitmap(screen);

	for (int i = 0; i < w; i++) {
		aux->cols[i] = i;
	}

	for (int i = 0; i < h; i++) {
		aux->rows[i] = i;
	}

	for (int i = 0; i < w; i++) {
		const int j = Tools_RandomRange(0, w - 1);
		const int swap = aux->cols[j];

		aux->cols[j] = aux->cols[i];
		aux->cols[i] = swap;
	}

	for (int i = 0; i < h; i++) {
		const int j = Tools_RandomRange(0, h - 1);
		const int swap = aux->rows[j];

		aux->rows[j] = aux->rows[i];
		aux->rows[i] = swap;
	}

	aux->fade_in = fade_in;
	aux->frame = 0;
	aux->width = w;
	aux->height = h;
	return aux;
}

void
VideoA5_DrawFadeIn(const FadeInAux *aux, int x, int y)
{
	al_draw_bitmap_region(screen, 0, 0, aux->width, aux->height, x, y, 0);
}

bool
VideoA5_TickFadeIn(FadeInAux *aux)
{
	assert(0 <= aux->frame && aux->frame <= aux->height);

	if (aux->frame >= aux->height)
		return true;

	ALLEGRO_LOCKED_REGION *reg = al_lock_bitmap_region(screen, 0, 0, aux->width, aux->height, ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, ALLEGRO_LOCK_READWRITE);

	int j = aux->frame;
	for (int i = 0; i < aux->width; i++) {
		const int x = aux->cols[i];
		const int y = aux->rows[j];

		if (++j >= aux->height)
			j = 0;

		unsigned char *row = &((unsigned char *)reg->data)[reg->pitch*y];

		if ((row[reg->pixel_size*x + 0] == 0x00) &&
		    (row[reg->pixel_size*x + 1] == 0x00) &&
		    (row[reg->pixel_size*x + 2] == 0x00))
			continue;

		if (aux->fade_in) {
			row[reg->pixel_size*x + 3] = 0xFF;
		}
		else {
			row[reg->pixel_size*x + 3] = 0x00;
		}
	}

	al_unlock_bitmap(screen);

	aux->frame++;
	return false;
}

/*--------------------------------------------------------------*/

static CPSStore *
VideoA5_ExportCPS(const char *filename, unsigned char *buf)
{
	CPSStore *cps = malloc(sizeof(*cps));
	assert(cps != NULL);

	cps->next = NULL;
	strncpy(cps->filename, filename, sizeof(cps->filename));

	cps->bmp = al_create_bitmap(SCREEN_WIDTH, SCREEN_HEIGHT);
	assert(cps->bmp != NULL);

	bool use_benepal = false;
	if ((strncmp(cps->filename, "MENTATM.CPS", sizeof(cps->filename)) == 0) ||
	    (strncmp(cps->filename, "MISC", sizeof(cps->filename)) == 0)) {
		use_benepal = true;
	}

	VideoA5_ReadPalette(use_benepal ? "BENE.PAL" : "IBM.PAL");
	memset(buf, 0, SCREEN_WIDTH * SCREEN_HEIGHT);
	Sprites_LoadImage(filename, 2, NULL);
	VideoA5_CopyBitmap(buf, cps->bmp, BLACK_COLOUR_0);
	VideoA5_ReadPalette("IBM.PAL");

	return cps;
}

static CPSStore *
VideoA5_LoadCPS(const char *filename)
{
	CPSStore *cps;

	cps = s_cps;
	while (cps != NULL) {
		if (strncmp(cps->filename, filename, sizeof(cps->filename)) == 0)
			return cps;

		cps = cps->next;
	}

	cps = VideoA5_ExportCPS(filename, GFX_Screen_Get_ByIndex(2));
	cps->next = s_cps;
	s_cps = cps;

	return cps;
}

static void
VideoA5_FreeCPS(CPSStore *cps)
{
	al_destroy_bitmap(cps->bmp);
	free(cps);
}

static void
VideoA5_InitCPS(void)
{
	const struct CPSSpecialCoord *coord;
	unsigned char *buf = GFX_Screen_Get_ByIndex(2);
	CPSStore *cps_screen = VideoA5_ExportCPS("SCREEN.CPS", buf);
	CPSStore *cps_fame = VideoA5_LoadCPS("FAME.CPS");
	CPSStore *cps_mapmach = VideoA5_LoadCPS("MAPMACH.CPS");

	al_set_target_bitmap(cps_special_texture);

	for (enum CPSID cpsID = CPS_MENUBAR_LEFT; cpsID <= CPS_STATUSBAR_RIGHT; cpsID++) {
		coord = &cps_special_coord[cpsID];

		if (coord->cx < 0 || coord->cy < 0)
			continue;

		al_draw_bitmap_region(cps_screen->bmp, coord->cx, coord->cy, coord->w, coord->h, coord->tx, coord->ty, 0);
	}

	coord = &cps_special_coord[CPS_STATUSBAR_MIDDLE];
	al_draw_bitmap_region(cps_screen->bmp,  8, 17, 304, coord->h, coord->tx, coord->ty, 0);
	al_draw_bitmap_region(cps_screen->bmp, 55, 17, 121, coord->h, coord->tx + 304, coord->ty, 0);

	for (enum HouseType houseID = HOUSE_HARKONNEN; houseID < HOUSE_MAX; houseID++) {
		Sprites_LoadImage("SCREEN.CPS", 2, NULL);
		GUI_Palette_CreateRemap(houseID);
		GUI_Palette_RemapScreen(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 2, g_remap);
		VideoA5_CopyBitmap(buf, cps_screen->bmp, TRANSPARENT_COLOUR_0);

		coord = &cps_special_coord[CPS_SIDEBAR_TOP];
		al_draw_bitmap_region(cps_screen->bmp, coord->cx, coord->cy, coord->w, coord->h, coord->tx + 17 * houseID, coord->ty + 3 * houseID, 0);

		coord = &cps_special_coord[CPS_SIDEBAR_MIDDLE];
		al_draw_bitmap_region(cps_screen->bmp, coord->cx, coord->cy, coord->w, coord->h, coord->tx + 17 * houseID, coord->ty + 3 * houseID, 0);

		coord = &cps_special_coord[CPS_SIDEBAR_BOTTOM];
		al_draw_bitmap_region(cps_screen->bmp, coord->cx, coord->cy, coord->w, coord->h, coord->tx + 17 * houseID, coord->ty + 20 * houseID, 0);
	}

	coord = &cps_special_coord[CPS_MENUBAR_MIDDLE];
	al_draw_bitmap_region(cps_fame->bmp, 135, 37, 109, coord->h, coord->tx, coord->ty, 0);
	al_draw_bitmap_region(cps_mapmach->bmp, 55, 183, 211, coord->h, coord->tx + 109, coord->ty, 0);

	coord = &cps_special_coord[CPS_CONQUEST_EN];
	assert(coord->cx == cps_special_coord[CPS_CONQUEST_FR].cx);
	assert(coord->cx == cps_special_coord[CPS_CONQUEST_DE].cx);
	assert(coord->tx == cps_special_coord[CPS_CONQUEST_FR].tx);
	assert(coord->tx == cps_special_coord[CPS_CONQUEST_DE].tx);
	al_draw_bitmap_region(cps_mapmach->bmp, coord->cx, cps_special_coord[CPS_CONQUEST_EN].cy, coord->w, coord->h, coord->tx, cps_special_coord[CPS_CONQUEST_EN].ty, 0);
	al_draw_bitmap_region(cps_mapmach->bmp, coord->cx, cps_special_coord[CPS_CONQUEST_FR].cy, coord->w, coord->h, coord->tx, cps_special_coord[CPS_CONQUEST_FR].ty, 0);
	al_draw_bitmap_region(cps_mapmach->bmp, coord->cx, cps_special_coord[CPS_CONQUEST_DE].cy, coord->w, coord->h, coord->tx, cps_special_coord[CPS_CONQUEST_DE].ty, 0);

	Sprites_LoadImage("MAPMACH.CPS", 2, NULL);
	ALLEGRO_LOCKED_REGION *reg = al_lock_bitmap(cps_special_texture, ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, ALLEGRO_LOCK_READWRITE);
	VideoA5_CreateWhiteMask(buf, reg, SCREEN_WIDTH, coord->cx, cps_special_coord[CPS_CONQUEST_EN].cy, coord->tx, cps_special_coord[CPS_CONQUEST_EN].ty + 25, coord->w, coord->h, CONQUEST_COLOUR);
	VideoA5_CreateWhiteMask(buf, reg, SCREEN_WIDTH, coord->cx, cps_special_coord[CPS_CONQUEST_FR].cy, coord->tx, cps_special_coord[CPS_CONQUEST_FR].ty + 25, coord->w, coord->h, CONQUEST_COLOUR);
	VideoA5_CreateWhiteMask(buf, reg, SCREEN_WIDTH, coord->cx, cps_special_coord[CPS_CONQUEST_DE].cy, coord->tx, cps_special_coord[CPS_CONQUEST_DE].ty + 25, coord->w, coord->h, CONQUEST_COLOUR);
	al_unlock_bitmap(cps_special_texture);

#if OUTPUT_TEXTURES
	al_save_bitmap("cps_special.png", cps_special_texture);
#endif

	VideoA5_FreeCPS(cps_screen);
}

void
VideoA5_DrawCPS(const char *filename)
{
	CPSStore *cps = VideoA5_LoadCPS(filename);

	al_draw_bitmap(cps->bmp, 0, 0, 0);
}

void
VideoA5_DrawCPSRegion(const char *filename, int sx, int sy, int dx, int dy, int w, int h)
{
	CPSStore *cps = VideoA5_LoadCPS(filename);

	al_draw_bitmap_region(cps->bmp, sx, sy, w, h, dx, dy, 0);
}

void
VideoA5_DrawCPSSpecial(enum CPSID cpsID, enum HouseType houseID, int x, int y)
{
	const unsigned char tint[HOUSE_MAX][3] = {
		{ 0x98, 0x00, 0x00 }, { 0x28, 0x3C, 0x98 }, { 0x24, 0x98, 0x24 }, { 0x98, 0x4C, 0x04 }, { 0xA8, 0x30, 0xA8 }, { 0x98, 0x68, 0x00 }
	};
	assert(cpsID < CPS_SPECIAL_MAX);
	assert(houseID < HOUSE_MAX);

	const struct CPSSpecialCoord *coord = &cps_special_coord[cpsID];

	int sx = coord->tx;
	int sy = coord->ty;

	if (CPS_CONQUEST_EN <= cpsID && cpsID <= CPS_CONQUEST_DE) {
		const ALLEGRO_COLOR col = al_map_rgb(tint[houseID][0], tint[houseID][1], tint[houseID][2]);

		al_draw_bitmap_region(cps_special_texture, sx, sy, coord->w, coord->h, x, y, 0);
		al_draw_tinted_bitmap_region(cps_special_texture, col, sx, sy + 25, coord->w, coord->h, x, y, 0);
		return;
	}

	if (CPS_SIDEBAR_TOP <= cpsID && cpsID <= CPS_SIDEBAR_BOTTOM) {
		sx += 17 * houseID;

		if (cpsID == CPS_SIDEBAR_BOTTOM) {
			sy += 20 * houseID;
		}
		else {
			sy += 3 * houseID;
		}
	}

	al_draw_bitmap_region(cps_special_texture, sx, sy, coord->w, coord->h, x, y, 0);
}

FadeInAux *
VideoA5_InitFadeInCPS(const char *filename, int x, int y, int w, int h, bool fade_in)
{
	ALLEGRO_BITMAP *old_target = al_get_target_bitmap();
	CPSStore *cps = VideoA5_LoadCPS(filename);

	al_set_target_bitmap(screen);
	al_draw_bitmap_region(cps->bmp, x, y, w, h, 0, 0, 0);
	al_set_target_bitmap(old_target);

	return VideoA5_InitFadeInSprite(w, h, fade_in);
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
	const int WINDOW_W = g_widgetProperties[WINDOWID_RENDER_TEXTURE].width;
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
	const int WINDOW_W = g_widgetProperties[WINDOWID_RENDER_TEXTURE].width;
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

	VideoA5_CopyBitmap(buf, icon_texture, TRANSPARENT_COLOUR_0);

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
	const int WINDOW_W = g_widgetProperties[WINDOWID_RENDER_TEXTURE].width;
	const int WINDOW_H = g_widgetProperties[WINDOWID_RENDER_TEXTURE].height;
	const int w = Shape_Width(shapeID);
	const int h = Shape_Height(shapeID);

	ALLEGRO_BITMAP *bmp;

	VideoA5_GetNextXY(WINDOW_W, WINDOW_H, x, y, w, h, row_h, &x, &y);
	GUI_DrawSprite_(0, g_sprites[shapeID], x, y, WINDOWID_RENDER_TEXTURE, 0x100, remap, 1);

	bmp = al_create_sub_bitmap(al_get_target_bitmap(), x, y, w, h);
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
		/*514, 524, false */ /* CREDIT1.SHP .. CREDIT11.SHP */

		/* BENE.PAL shapes. */
		{  -3,   0, false },
		/*432, 461, false */ /* MENSHPM.SHP: Fremen, Sardaukar */
		{ 462, 476, false }, /* MENSHPM.SHP */

		{  -2,   0, false },
		{ 477, 504,  true }, /* PIECES.SHP */
		{ 505, 513, false }, /* ARROWS.SHP */

		{  -1,   0, false }
	};

	const int WINDOW_W = g_widgetProperties[WINDOWID_RENDER_TEXTURE].width;
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
		if (shape_data[group].start == -3) {
			VideoA5_CopyBitmap(buf, shape_texture, TRANSPARENT_COLOUR_0);
			VideoA5_ReadPalette("BENE.PAL");
			memset(buf, 0, WINDOW_W * WINDOW_H);
			continue;
		}
		else if (shape_data[group].start == -2) {
			VideoA5_CopyBitmap(buf, shape_texture, SKIP_COLOUR_0);
			VideoA5_ReadPalette("IBM.PAL");
			memset(buf, 0, WINDOW_W * WINDOW_H);

			al_set_target_bitmap(region_texture);
			x = 0, y = 0, row_h = 0;
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

	VideoA5_CopyBitmap(buf, region_texture, TRANSPARENT_COLOUR_0);
	memset(buf, 0, WINDOW_W * WINDOW_H);

	ALLEGRO_LOCKED_REGION *reg = al_lock_bitmap(region_texture, ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, ALLEGRO_LOCK_READWRITE);
	for (int shapeID = SHAPE_ARROW; shapeID <= SHAPE_ARROW_FINAL; shapeID++) {
		const int tintID = SHAPE_ARROW_TINT + 4 * (shapeID - SHAPE_ARROW);
		const int w = Shape_Width(shapeID);
		const int h = Shape_Height(shapeID);

		VideoA5_GetNextXY(WINDOW_W, WINDOW_H, x, y, 4 * w, h, row_h, &x, &y);
		GUI_DrawSprite_(0, g_sprites[shapeID], x, y, WINDOWID_RENDER_TEXTURE, 0);

		for (int i = 0; i < 4; i++) {
			s_shape[tintID + i][0] = al_create_sub_bitmap(region_texture, x + i * w, y, w, h);
			assert(s_shape[tintID + i][0] != NULL);

			VideoA5_CreateWhiteMask(buf, reg, WINDOW_W, x, y, x + i * w, y, w, h, ARROW_COLOUR + i);
		}

		x += 4 * w + 1;
		row_h = max(row_h, h);
	}
	al_unlock_bitmap(region_texture);

#if OUTPUT_TEXTURES
	al_save_bitmap("shapes.png", shape_texture);
	al_save_bitmap("regions.png", region_texture);
#endif
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

	if ((flags & 0x300) == 0x100) {
		/* Highlight. */
		al_draw_bitmap(s_shape[shapeID][houseID], x, y, al_flags);

		al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_ONE);
		al_draw_bitmap(s_shape[shapeID][houseID], x, y, al_flags);
		al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);
	}
	else if ((flags & 0x300) == 0x200) {
		/* Blur tile (sandworm, sonic wave). */
		const int s_variable_60[8] = {1, 3, 2, 5, 4, 3, 2, 1};
		static int effect = 0;

		effect = (effect + 1) & 0x7;

		ALLEGRO_BITMAP *old_target = al_get_target_bitmap();
		ALLEGRO_BITMAP *brush = s_shape[shapeID][houseID];

		const int w = al_get_bitmap_width(brush);
		const int h = al_get_bitmap_height(brush);

		al_set_target_bitmap(screen);
		Viewport_RenderBrush(x + s_variable_60[effect], y);

		al_set_target_bitmap(brush);
		al_set_separate_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO, ALLEGRO_DEST_MINUS_SRC, ALLEGRO_ZERO, ALLEGRO_ONE);
		al_draw_bitmap_region(screen, 0, 0, w, h, 0, 0, 0);

		al_set_target_bitmap(old_target);
		al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);
		al_draw_bitmap(brush, x, y, 0);
	}
	else if ((flags & 0x300) == 0x300) {
		/* Shadow. */
		al_draw_tinted_bitmap(s_shape[shapeID][houseID], al_map_rgba(0, 0, 0, 0x40), x, y, al_flags);
	}
	else {
		/* Normal. */
		al_draw_bitmap(s_shape[shapeID][houseID], x, y, al_flags);
	}
}

void
VideoA5_DrawShapeScale(enum ShapeID shapeID, int x, int y, int w, int h, int flags)
{
	assert(shapeID < SHAPEID_MAX);

	ALLEGRO_BITMAP *bmp = s_shape[shapeID][HOUSE_HARKONNEN];
	assert(bmp != NULL);

	al_draw_scaled_bitmap(bmp, 0, 0, al_get_bitmap_width(bmp), al_get_bitmap_height(bmp), x, y, w, h, flags);
}

void
VideoA5_DrawShapeGrey(enum ShapeID shapeID, int x, int y, int flags)
{
	const enum ShapeID greyID = SHAPE_CONCRETE_SLAB_GREY + (shapeID - SHAPE_CONCRETE_SLAB);
	assert(SHAPE_CONCRETE_SLAB <= shapeID && shapeID <= SHAPE_FREMEN);
	assert(s_shape[greyID][HOUSE_HARKONNEN] != NULL);

	al_draw_bitmap(s_shape[greyID][HOUSE_HARKONNEN], x, y, flags);
}

void
VideoA5_DrawShapeGreyScale(enum ShapeID shapeID, int x, int y, int w, int h, int flags)
{
	const enum ShapeID greyID = SHAPE_CONCRETE_SLAB_GREY + (shapeID - SHAPE_CONCRETE_SLAB);
	assert(SHAPE_CONCRETE_SLAB <= shapeID && shapeID <= SHAPE_FREMEN);

	ALLEGRO_BITMAP *bmp = s_shape[greyID][HOUSE_HARKONNEN];
	assert(bmp != NULL);

	al_draw_scaled_bitmap(bmp, 0, 0, al_get_bitmap_width(bmp), al_get_bitmap_height(bmp), x, y, w, h, flags);
}

void
VideoA5_DrawShapeTint(enum ShapeID shapeID, int x, int y, unsigned char c, int flags)
{
	assert(shapeID < SHAPEID_MAX);
	assert(s_shape[shapeID][HOUSE_HARKONNEN] != NULL);

	al_draw_tinted_bitmap(s_shape[shapeID][HOUSE_HARKONNEN], paltoRGB[c], x, y, flags);
}

FadeInAux *
VideoA5_InitFadeInShape(enum ShapeID shapeID, enum HouseType houseID)
{
	assert(shapeID < SHAPEID_MAX);
	assert(houseID < HOUSE_MAX);
	assert(s_shape[shapeID][houseID] != NULL);

	ALLEGRO_BITMAP *old_target = al_get_target_bitmap();
	ALLEGRO_BITMAP *bmp = s_shape[shapeID][houseID];

	al_set_target_bitmap(screen);
	al_clear_to_color(al_map_rgba(0x00, 0x00, 0x00, 0x00));
	al_draw_bitmap(bmp, 0, 0, 0);
	al_set_target_bitmap(old_target);

	return VideoA5_InitFadeInSprite(al_get_bitmap_width(bmp), al_get_bitmap_height(bmp), true);
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
	const int WINDOW_W = g_widgetProperties[WINDOWID_RENDER_TEXTURE].width;
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

	VideoA5_CopyBitmap(buf, font_texture, TRANSPARENT_COLOUR_0);

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
VideoA5_InitWSA(unsigned char *buf)
{
	const int WINDOW_W = g_widgetProperties[WINDOWID_RENDER_TEXTURE].width;
	const int WINDOW_H = g_widgetProperties[WINDOWID_RENDER_TEXTURE].height;

	void *wsa = WSA_LoadFile("STATIC.WSA", GFX_Screen_Get_ByIndex(5), GFX_Screen_GetSize_ByIndex(5), true);
	assert(wsa != NULL);

	const int num_frames = WSA_GetFrameCount(wsa);

	int x = 0, y = 0;

	al_set_target_bitmap(wsa_special_texture);

	for (int frame = 0; frame < num_frames; frame++) {
		VideoA5_GetNextXY(WINDOW_W, WINDOW_H, x, y, 64, 64, 64, &x, &y);
		WSA_DisplayFrame(wsa, frame, 0, 0, 0);

		VideoA5_CopyBitmap(buf, screen, BLACK_COLOUR_0);
		al_draw_bitmap_region(screen, 0, 0, 64, 64, x, y, 0);

		x += 64 + 1;
	}

#if OUTPUT_TEXTURES
	al_save_bitmap("wsa_special.png", wsa_special_texture);
#endif

	WSA_Unload(wsa);
}

bool
VideoA5_DrawWSA(void *wsa, int frame, int sx, int sy, int dx, int dy, int w, int h)
{
	if (wsa != NULL) {
		if (!WSA_DisplayFrame(wsa, frame, 0, 0, 0))
			return false;
	}

	const unsigned char *buf = GFX_Screen_Get_ByIndex(0);

	VideoA5_CopyBitmap(buf, screen, BLACK_COLOUR_0);
	al_draw_bitmap_region(screen, sx, sy, w, h, dx, dy, 0);

	return true;
}

void
VideoA5_DrawWSAStatic(int frame, int x, int y)
{
	const int WINDOW_W = g_widgetProperties[WINDOWID_RENDER_TEXTURE].width;
	const int WINDOW_H = g_widgetProperties[WINDOWID_RENDER_TEXTURE].height;
	const int tx = frame % (WINDOW_W / 65);
	const int ty = frame / (WINDOW_H / 65);
	const int sx = 65 * tx;
	const int sy = 65 * ty;
	assert(0 <= frame && frame < 21);

	al_draw_bitmap_region(wsa_special_texture, sx, sy, 64, 64, x, y, 0);
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
	const int WINDOW_W = g_widgetProperties[WINDOWID_RENDER_TEXTURE].width;
	const int WINDOW_H = g_widgetProperties[WINDOWID_RENDER_TEXTURE].height;
	const uint16 old_screen = GFX_Screen_SetActive(0);
	const enum WindowID old_widget = Widget_SetCurrentWidget(WINDOWID_RENDER_TEXTURE);

	unsigned char *buf = GFX_Screen_GetActive();

	VideoA5_ReadPalette("IBM.PAL");

	VideoA5_InitCPS();

	memset(buf, 0, WINDOW_W * WINDOW_H);
	VideoA5_InitIcons(buf);

	memset(buf, 0, WINDOW_W * WINDOW_H);
	VideoA5_InitShapes(buf);

	memset(buf, 0, WINDOW_W * WINDOW_H);
	VideoA5_InitFonts(buf);

	memset(buf, 0, WINDOW_W * WINDOW_H);
	VideoA5_InitWSA(buf);

	VideoA5_InitCursor();

	al_set_target_backbuffer(display);
	al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);

	GFX_Screen_SetActive(old_screen);
	Widget_SetCurrentWidget(old_widget);
}
