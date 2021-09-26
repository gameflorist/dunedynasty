/* video_a5.c
 *
 * Notes:
 *
 * CPS rendered by opendune on screen 2/3 (once).
 * WSA rendered by opendune on screen 0/1 (every frame).
 * WSA loaded in screen 4/5.
 *
 * This avoids clashes with each other and mentat.
 *
 * The various textures are preserved by Allegro.
 * Other bitmaps (e.g. stored CPS files) are reloaded if necessary.
 */

#include <assert.h>

#ifdef __APPLE__
# include <OpenGL/gl.h>
#else
# ifdef __PANDORA__
#  include <GLES/gl.h>
# else
#  include <GL/gl.h>
# endif /* __PANDORA__ */
#endif /* __APPLE__ */

#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>

#ifdef ALLEGRO_WINDOWS
#include <allegro5/allegro_direct3d.h>
#endif

#include <stdio.h>
#include "../os/common.h"
#include "../os/error.h"
#include "../os/math.h"

#include "video_a5.h"

#include "../common_a5.h"
#include "../config.h"
#include "../enhancement.h"
#include "../file.h"
#include "../gfx.h"
#include "../gui/font.h"
#include "../gui/gui.h"
#include "../input/input_a5.h"
#include "../input/mouse.h"
#include "../map.h"
#include "../newui/viewport.h"
#include "../opendune.h"
#include "../scenario.h"
#include "../sprites.h"
#include "../table/widgetinfo.h"
#include "../timer/timer.h"
#include "../tools/coord.h"
#include "../tools/random_lcg.h"
#include "../wsa.h"

#ifndef MULTIPLE_WINDOW_ICONS
# if (ALLEGRO_VERSION == 5 && ALLEGRO_SUB_VERSION == 0 && ALLEGRO_WIP_VERSION >= 9) || \
     (ALLEGRO_VERSION == 5 && ALLEGRO_SUB_VERSION == 1 && ALLEGRO_WIP_VERSION >= 5)
#  define MULTIPLE_WINDOW_ICONS
# endif
#endif

#include "dune2_16x16.xpm"

#ifdef MULTIPLE_WINDOW_ICONS
#include "dune2_32x32.xpm"
/* #include "dune2_32x32a.xpm" */
#endif

#define OUTPUT_TEXTURES     false
#define ICONID_MAX          512
#define SHAPEID_MAX         640
#define FONTID_MAX          8
#define CURSOR_MAX          6

enum BitmapCopyMode {
	TRANSPARENT_COLOUR_0,
	BLACK_COLOUR_0,
	SKIP_COLOUR_0
};

typedef struct CPSStore {
	struct CPSStore *next;

	char filename[128];
	ALLEGRO_BITMAP *bmp;
} CPSStore;

typedef struct FadeInAux {
	bool fade_in;   /* false to fade out. */
	int frame;      /* 0 <= frame < height. */

	ALLEGRO_BITMAP *bmp;
	int x, y;
	int width;
	int height;

	/* Persistent random data. */
	int cols[SCREEN_WIDTH];
	int rows[SCREEN_HEIGHT];
} FadeInAux;

typedef struct IconCoord {
	int sx, sy;
	int sx32, sy32;
	int sx48, sy48;
} IconCoord;

typedef struct IconConnectivity {
	uint16 iconU;
	uint16 iconD;
	uint16 iconL;
	uint16 iconR;
} IconConnectivity;

static const struct CPSSpecialCoord {
	int cx, cy; /* coordinates in cps. */
	int tx, ty; /* coordinates in texture. */
	int w, h;
} cps_special_coord[CPS_SPECIAL_MAX] = {
	{   0,   0,   2,   2, 184, 17 }, /* CPS_MENUBAR_LEFT */
	{  -1,  -1,   2,  22, 320, 17 }, /* CPS_MENUBAR_MIDDLE */
	{ 184,   0, 189,   2, 136, 17 }, /* CPS_MENUBAR_RIGHT */
	{   0,  17,   2,  42,   8, 23 }, /* CPS_STATUSBAR_LEFT */
	{  -1,  -1,  13,  42, 425, 23 }, /* CPS_STATUSBAR_MIDDLE */
	{ 312,  17, 441,  42,   8, 23 }, /* CPS_STATUSBAR_RIGHT */
	{ 240,  40,   2,  68,  80, 17 }, /* CPS_SIDEBAR_TOP */
	{ 240,  63,   2,  88,  16, 52 }, /* CPS_SIDEBAR_MIDDLE */
	{ 240, 115,   2, 162,  80, 85 }, /* CPS_SIDEBAR_BOTTOM */
	{   8,   0, 192,  68, 304, 24 }, /* CPS_CONQUEST_EN */
	{   8,  96, 192, 118, 304, 24 }, /* CPS_CONQUEST_FR */
	{   8, 120, 192, 168, 304, 24 }, /* CPS_CONQUEST_DE */
};

static const uint8 font_palette[][8] = {
	{ 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, /* No outline. */
	{ 0x00, 0xFF, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00 }, /* Shadow. */
	{ 0x00, 0xFF, 0x0C, 0x0C, 0x00, 0x00, 0x00, 0x00 }, /* Outline. */
	{ 0x00, 0xFF, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0x00 }, /* Intro. */
};

static const struct {
	int x, y;
} cursor_focus[CURSOR_MAX] = {
	{ 0, 0 }, { 5, 0 }, { 8, 5 }, { 5, 8 }, { 0, 5 }, { 8, 8 }
};

enum GraphicsDriver g_graphics_driver;

/* Exposed for prim_a5.c. */
ALLEGRO_COLOR paltoRGB[256];

static ALLEGRO_DISPLAY *display;
static unsigned char paletteRGB[3 * 256];

static CPSStore *s_cps;
static ALLEGRO_BITMAP *scratch; /* temporary bitmap for non-speed-critical images. */
static ALLEGRO_BITMAP *interface_texture; /* cps, wsa, and fonts. */
static ALLEGRO_BITMAP *icon_texture;      /* 16x16 tiles. */
static ALLEGRO_BITMAP *icon_texture32;    /* 32x32 tiles. */
static ALLEGRO_BITMAP *icon_texture48;    /* 48x48 tiles. */
static ALLEGRO_BITMAP *shape_texture;     /* in game shapes. */
static ALLEGRO_BITMAP *mentat_texture;    /* XXX - temporary bitmap for mentats. */
static ALLEGRO_BITMAP *region_texture;    /* strategic map shapes. */
static IconCoord s_icon[ICONID_MAX][HOUSE_MAX];
static ALLEGRO_BITMAP *s_shape[SHAPEID_MAX][HOUSE_MAX];
static ALLEGRO_BITMAP *s_font[FONTID_MAX][256];
static ALLEGRO_MOUSE_CURSOR *s_cursor[CURSOR_MAX];

static ALLEGRO_BITMAP *s_minimap;
static int s_minimap_colour[MAP_SIZE_MAX * MAP_SIZE_MAX];

static bool take_screenshot = false;
static bool show_fps = false;
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
		x = 1;
		y += row_h + 1;
	}

	if (y + h - 1 >= texture_height)
		exit(1);

	*retx = x;
	*rety = y;
}

/* VideoA5_SetBitmapFlags:
 *
 * Assume you create a memory bitmap, then restore back to video bitmap.
 * Keeps old flags around for easy use.
 */
static void
VideoA5_SetBitmapFlags(int flags)
{
	static int old_flags;

	if (flags == ALLEGRO_MEMORY_BITMAP) {
		old_flags = al_get_new_bitmap_flags();
		al_set_new_bitmap_flags((old_flags & ~ALLEGRO_VIDEO_BITMAP) | ALLEGRO_MEMORY_BITMAP);
	}
	else {
		al_set_new_bitmap_flags(old_flags);
	}
}

static ALLEGRO_BITMAP *
VideoA5_ConvertToVideoBitmap(ALLEGRO_BITMAP *membmp)
{
	assert(!(al_get_new_bitmap_flags() & ALLEGRO_MEMORY_BITMAP));

	ALLEGRO_BITMAP *vidbmp = al_clone_bitmap(membmp);
	al_destroy_bitmap(membmp);
	return vidbmp;
}

static void
VideoA5_ResizeScratchBitmap(int w, int h)
{
	if (scratch == NULL) {
		scratch = al_create_bitmap(w, h);
	}
	else if ((al_get_bitmap_width(scratch) != w) || (al_get_bitmap_height(scratch) != h)) {
		al_destroy_bitmap(scratch);
		scratch = al_create_bitmap(w, h);
	}
}

static void
VideoA5_ReadPalette(const char *filename)
{
	File_ReadBlockFile(filename, paletteRGB, 3 * 256);
	Video_SetPalette(paletteRGB, 0, 256);

	/* A bit of a hack: make windtrap magic pink black. */
	paltoRGB[WINDTRAP_COLOUR] = al_map_rgb(0x00, 0x00, 0x00);
	paletteRGB[3*WINDTRAP_COLOUR + 0] = 0x00;
	paletteRGB[3*WINDTRAP_COLOUR + 1] = 0x00;
	paletteRGB[3*WINDTRAP_COLOUR + 2] = 0x00;
}

static ALLEGRO_BITMAP *
VideoA5_InitDisplayIcon(char **xpm, int w, int h, int colours)
{
	struct {
		unsigned char r, g, b;
	} map[256];

	ALLEGRO_BITMAP *icon;
	ALLEGRO_LOCKED_REGION *reg;

	icon = al_create_bitmap(w, h);
	if (icon == NULL)
		return NULL;

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
	return icon;
}

static void
VideoA5_InitWindowIcons(void)
{
	ALLEGRO_BITMAP *icon[2];

	icon[0] = VideoA5_InitDisplayIcon(dune2_16x16_xpm, 16, 16, 32);
	if (icon[0] == NULL)
		return;

#ifdef MULTIPLE_WINDOW_ICONS
	icon[1] = VideoA5_InitDisplayIcon(dune2_32x32_xpm, 32, 32, 23);
	/* icon[1] = VideoA5_InitDisplayIcon(dune2_32x32a_xpm, 32, 32, 13); */

	if (icon[1] != NULL) {
		al_set_display_icons(display, 2, icon);
		al_destroy_bitmap(icon[1]);
		al_destroy_bitmap(icon[0]);
		return;
	}
#endif

	al_set_display_icon(display, icon[0]);
	al_destroy_bitmap(icon[0]);
}

bool
VideoA5_Init(void)
{
	const int w = g_widgetProperties[WINDOWID_RENDER_TEXTURE].width;
	const int h = g_widgetProperties[WINDOWID_RENDER_TEXTURE].height;
	int display_flags = ALLEGRO_GENERATE_EXPOSE_EVENTS;

	switch (g_graphics_driver) {
		case GRAPHICS_DRIVER_OPENGL:
		default:
			display_flags |= ALLEGRO_OPENGL;
			break;

#ifdef ALLEGRO_WINDOWS
		case GRAPHICS_DRIVER_DIRECT3D:
			display_flags |= ALLEGRO_DIRECT3D;
			break;
#endif
	}

	if (g_gameConfig.windowMode == WM_FULLSCREEN) {
		display_flags |= ALLEGRO_FULLSCREEN;
	}
	else if (g_gameConfig.windowMode == WM_FULLSCREEN_WINDOW) {
		display_flags |= ALLEGRO_FULLSCREEN_WINDOW;
	}

	al_set_new_display_flags(display_flags);
	al_set_new_display_option(ALLEGRO_VSYNC, 1, ALLEGRO_SUGGEST);
	al_set_new_display_option(ALLEGRO_STENCIL_SIZE, 8, ALLEGRO_SUGGEST);
	display = al_create_display(TRUE_DISPLAY_WIDTH, TRUE_DISPLAY_HEIGHT);
	if (display == NULL) {
		Error("Could not create display!");
		return false;
	}

	al_set_window_title(display, "Dune Dynasty");
	/* al_set_new_bitmap_flags(ALLEGRO_MAG_LINEAR); */
	TRUE_DISPLAY_WIDTH = al_get_display_width(display);
	TRUE_DISPLAY_HEIGHT = al_get_display_height(display);

	VideoA5_SetBitmapFlags(ALLEGRO_MEMORY_BITMAP);
	VideoA5_InitWindowIcons();
	interface_texture = al_create_bitmap(w, h);

	VideoA5_SetBitmapFlags(ALLEGRO_VIDEO_BITMAP);
	shape_texture = al_create_bitmap(w, h);
	region_texture = al_create_bitmap(w, h);

	al_set_new_bitmap_flags(ALLEGRO_NO_PRESERVE_TEXTURE);
	s_minimap = al_create_bitmap(64, 64);

	if (interface_texture == NULL || shape_texture == NULL || region_texture == NULL || s_minimap == NULL)
		return false;

	al_register_event_source(g_a5_input_queue, al_get_display_event_source(display));

	al_init_image_addon();
	al_init_primitives_addon();

	/* Flip display in case generating the sprites takes a while. */
	al_flip_display();

	return true;
}

static void
VideoA5_UninitCPSStore(void)
{
	while (s_cps != NULL) {
		CPSStore *next = s_cps->next;

		al_destroy_bitmap(s_cps->bmp);
		free(s_cps);

		s_cps = next;
	}

	s_fadeInAux.bmp = NULL;
}

void
VideoA5_Uninit(void)
{
	for (enum HouseType houseID = HOUSE_HARKONNEN; houseID < HOUSE_MAX; houseID++) {
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

	VideoA5_UninitCPSStore();

	al_destroy_bitmap(scratch);
	scratch = NULL;

	al_destroy_bitmap(s_minimap);
	s_minimap = NULL;

	al_destroy_bitmap(interface_texture);
	interface_texture = NULL;

	al_destroy_bitmap(icon_texture);
	al_destroy_bitmap(icon_texture32);
	al_destroy_bitmap(icon_texture48);
	icon_texture = NULL;
	icon_texture32 = NULL;
	icon_texture48 = NULL;

	al_destroy_bitmap(shape_texture);
	al_destroy_bitmap(mentat_texture);
	al_destroy_bitmap(region_texture);
	shape_texture = NULL;
	mentat_texture = NULL;
	region_texture = NULL;
}

void
VideoA5_ToggleFullscreen(void)
{
	const int display_flags = al_get_display_flags(display);
	if (display_flags & ALLEGRO_FULLSCREEN)
		return;

	const bool fs = (display_flags & ALLEGRO_FULLSCREEN_WINDOW);
	const ScreenDiv *viewport = &g_screenDiv[SCREENDIV_VIEWPORT];
	const int tilex = Tile_GetPackedX(g_viewportPosition);
	const int tiley = Tile_GetPackedY(g_viewportPosition);
	const int viewport_cx = TILE_SIZE * tilex + g_viewport_scrollOffsetX + viewport->width / 2;
	const int viewport_cy = TILE_SIZE * tiley + g_viewport_scrollOffsetY + viewport->height / 2;

	al_set_display_flag(display, ALLEGRO_FULLSCREEN_WINDOW, !fs);
	TRUE_DISPLAY_WIDTH = al_get_display_width(display);
	TRUE_DISPLAY_HEIGHT = al_get_display_height(display);

	GFX_InitDefaultViewportScales(false);
	al_set_target_backbuffer(display);
	A5_InitTransform(true);
	GameLoop_TweakWidgetDimensions();
	Map_CentreViewport(viewport_cx, viewport_cy);

	/* Free CPS store when we toggle fullscreen mode, important for Direct3D. */
	VideoA5_DisplayFound();
}

void
VideoA5_ToggleFPS(void)
{
	show_fps = !show_fps;
}

void
VideoA5_CaptureScreenshot(void)
{
	take_screenshot = true;
}

static void
VideoA5_CopyBitmap(int src_stride, const unsigned char *raw, ALLEGRO_BITMAP *dest, enum BitmapCopyMode mode)
{
	ALLEGRO_LOCKED_REGION *reg;

	if (mode == SKIP_COLOUR_0) {
		reg = al_lock_bitmap(dest, ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, ALLEGRO_LOCK_READWRITE);
	}
	else {
		reg = al_lock_bitmap(dest, ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, ALLEGRO_LOCK_WRITEONLY);
	}

	if (reg == NULL)
		return;

	const int w = al_get_bitmap_width(dest);
	const int h = al_get_bitmap_height(dest);

	for (int y = 0; y < h; y++) {
		unsigned char *row = &((unsigned char *)reg->data)[reg->pitch*y];

		for (int x = 0; x < w; x++) {
			const unsigned char c = raw[src_stride * y + x];

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

static void
VideoA5_CreateWhiteMaskIndexed(unsigned char *buf,
		int stride, int sx, int sy, int dx, int dy, int w, int h, int ref)
{
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			const int src = stride * (sy + y) + (sx + x);
			const int dst = stride * (dy + y) + (dx + x);

			buf[dst] = (buf[src] == ref) ? 0xFF : 0x00;
		}
	}
}

void
VideoA5_Tick(void)
{
	static double l_last_time;
	static double l_last_fps;
	static int l_fps;

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

	if (show_fps) {
		const double curr_time = al_get_time();
		char str[16];

		/* Don't clobber the current font state. */
		int len = snprintf(str, sizeof(str), "FPS:%4.2f", l_last_fps);
		for (int i = 0; i < len; i++) {
			const unsigned char c = str[i];
			al_draw_tinted_bitmap(s_font[2][c], paltoRGB[15], 2 + 6 * i, 40, 0);
		}

		l_fps++;
		if (curr_time - l_last_time >= 0.5f) {
			l_last_fps = l_fps / (curr_time - l_last_time);
			l_last_time = al_get_time();
			l_fps = 0;
		}
	}

	/* Draw software mouse cursor for people who have trouble with hardware cursors. */
	if (!g_gameConfig.hardwareCursor && !g_mouseHidden) {
		const int size = (TRUE_DISPLAY_WIDTH >= 640) ? 32 : 16;
		const enum ScreenDivID div = A5_SaveTransform();
		const int x = g_screenDiv[div].scalex * (g_mouseX - cursor_focus[g_cursorSpriteID].x) + g_screenDiv[div].x;
		const int y = g_screenDiv[div].scaley * (g_mouseY - cursor_focus[g_cursorSpriteID].y) + g_screenDiv[div].y;

		A5_UseTransform(SCREENDIV_MAIN);

		al_draw_scaled_bitmap(s_shape[g_cursorSpriteID][HOUSE_HARKONNEN], 0, 0, 16, 16, x, y, size, size, 0);

		A5_UseTransform(div);
	}

	al_flip_display();
	al_clear_to_color(paltoRGB[0]);
}

/*--------------------------------------------------------------*/

void
Video_SetPalette(const uint8 *palette, int from, int length)
{
	const uint8 *p = palette;
	assert(from + length <= 256);

	for (int i = from; i < from + length; i++) {
		uint8 r = ((*p++) & 0x3F);
		uint8 g = ((*p++) & 0x3F);
		uint8 b = ((*p++) & 0x3F);

		r = (r << 2) | (r >> 4);
		g = (g << 2) | (g >> 4);
		b = (b << 2) | (b >> 4);

		paltoRGB[i] = al_map_rgb(r, g, b);
		paletteRGB[3*i + 0] = r;
		paletteRGB[3*i + 1] = g;
		paletteRGB[3*i + 2] = b;
	}
}

void
Video_SetClippingArea(int x, int y, int w, int h)
{
	al_set_clipping_rectangle(x, y, w, h);
}

void
Video_SetCursor(int spriteID)
{
	assert(spriteID < CURSOR_MAX);

	g_cursorSpriteID = spriteID;
	al_set_mouse_cursor(display, s_cursor[spriteID]);
}

void
Video_ShowCursor(void)
{
	if (g_gameConfig.hardwareCursor)
		al_show_mouse_cursor(display);

	g_mouseHidden = false;
}

void
Video_HideCursor(void)
{
	al_hide_mouse_cursor(display);
	g_mouseHidden = true;
}

void
Video_HideHWCursor(void)
{
	al_hide_mouse_cursor(display);
}

void
Video_WarpCursor(int x, int y)
{
	al_set_mouse_xy(display, x, y);
}

void
Video_ShadeScreen(int alpha)
{
	const enum ScreenDivID prev_transform = A5_SaveTransform();

	alpha = clamp(0x00, alpha, 0xFF);

	A5_UseTransform(SCREENDIV_MAIN);
	al_draw_filled_rectangle(0.0f, 0.0f, TRUE_DISPLAY_WIDTH, TRUE_DISPLAY_HEIGHT, al_map_rgba(0, 0, 0, alpha));

	A5_UseTransform(prev_transform);
}

void
Video_HoldBitmapDrawing(bool hold)
{
	al_hold_bitmap_drawing(hold);
}

/*--------------------------------------------------------------*/

#if 0
/* Requires read/write to texture. */
static void
VideoA5_InitDissolve_LockedBitmap(ALLEGRO_BITMAP *src, FadeInAux *aux)
{
	if ((al_get_bitmap_width(src) == aux->width) && (al_get_bitmap_height(src) == aux->height)) {
		al_destroy_bitmap(scratch);
		scratch = al_clone_bitmap(src);
	}
	else {
		ALLEGRO_BITMAP *old_target = al_get_target_bitmap();
		VideoA5_ResizeScratchBitmap(aux->width, aux->height);

		al_set_target_bitmap(scratch);
		al_draw_bitmap_region(src, aux->x, aux->y, aux->width, aux->height, 0, 0, 0);
		al_set_target_bitmap(old_target);
	}

	ALLEGRO_LOCKED_REGION *reg = al_lock_bitmap(scratch, ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, ALLEGRO_LOCK_READWRITE);

	for (int y = 0; y < aux->height; y++) {
		unsigned char *row = &((unsigned char *)reg->data)[reg->pitch*y];

		for (int x = 0; x < aux->width; x++) {
			row[reg->pixel_size*x + 3] = aux->fade_in ? 0x00 : 0xFF;
		}
	}

	al_unlock_bitmap(scratch);

	aux->bmp = NULL;
}

static void
VideoA5_DrawDissolve_LockedBitmap(const FadeInAux *aux)
{
	al_draw_bitmap(scratch, aux->x, aux->y, 0);
}

static void
VideoA5_TickDissolve_LockedBitmap(FadeInAux *aux)
{
	ALLEGRO_LOCKED_REGION *reg = al_lock_bitmap(scratch, ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, ALLEGRO_LOCK_READWRITE);

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

	al_unlock_bitmap(scratch);
}
#endif

#if 1
/* Requires OpenGL, stencil buffer. */
static void
VideoA5_InitDissolve_GLStencil(ALLEGRO_BITMAP *src, FadeInAux *aux)
{
	glClear(GL_STENCIL_BUFFER_BIT);

	aux->bmp = src;
}

static void
VideoA5_DrawDissolve_GLStencil(const FadeInAux *aux)
{
	if (aux->bmp == NULL)
		return;

	glEnable(GL_STENCIL_TEST);

	if (aux->fade_in) {
		/* Stencil is 1 where we should draw. */
		glStencilFunc(GL_EQUAL, 0x1, 0x1);
	}
	else {
		/* Stencil is 0 where we should draw. */
		glStencilFunc(GL_NOTEQUAL, 0x1, 0x1);
	}

	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	if ((al_get_bitmap_width(aux->bmp) == aux->width) && (al_get_bitmap_height(aux->bmp) == aux->height)) {
		al_draw_bitmap(aux->bmp, aux->x, aux->y, 0);
	}
	else {
		al_draw_bitmap_region(aux->bmp, aux->x, aux->y, aux->width, aux->height, aux->x, aux->y, 0);
	}

	glDisable(GL_STENCIL_TEST);
}

static void
VideoA5_TickDissolve_GLStencil(FadeInAux *aux)
{
	glEnable(GL_STENCIL_TEST);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
	glStencilFunc(GL_ALWAYS, 0x1, 0x1);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	/* 2 triangles per x coordinate. */
	ALLEGRO_VERTEX v[6 * SCREEN_WIDTH];
	assert(aux->width < SCREEN_WIDTH);
	memset(v, 0, sizeof(v));

	int count = 0;
	int j = aux->frame;
	for (int i = 0; i < aux->width; i++) {
		const int x = aux->x + aux->cols[i];
		const int y = aux->y + aux->rows[j];
		const float x1 = x;
		const float x2 = x + 1.00f;
		const float y1 = y;
		const float y2 = y + 1.00f;

		v[count].x = x1; v[count].y = y1; count++;
		v[count].x = x1; v[count].y = y2; count++;
		v[count].x = x2; v[count].y = y1; count++;
		v[count].x = x1; v[count].y = y2; count++;
		v[count].x = x2; v[count].y = y1; count++;
		v[count].x = x2; v[count].y = y2; count++;

		if (++j >= aux->height)
			j = 0;
	}

	al_draw_prim(v, NULL, NULL, 0, 6 * aux->width, ALLEGRO_PRIM_TRIANGLE_LIST);

	glDisable(GL_STENCIL_TEST);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}
#endif

#ifdef ALLEGRO_WINDOWS
/* Requires Direct3D, stencil buffer. */
static void
VideoA5_InitDissolve_D3DStencil(ALLEGRO_BITMAP *src, FadeInAux *aux)
{
	LPDIRECT3DDEVICE9 pDevice = al_get_d3d_device(display);

	if (pDevice == NULL) {
		aux->bmp = NULL;
	}
	else {
		IDirect3DDevice9_Clear(pDevice, 0, NULL, D3DCLEAR_STENCIL, 0, 0, 0);
		aux->bmp = src;
	}
}

static void
VideoA5_DrawDissolve_D3DStencil(const FadeInAux *aux)
{
	if (aux->bmp == NULL)
		return;

	LPDIRECT3DDEVICE9 pDevice = al_get_d3d_device(display);
	if (pDevice == NULL)
		return;

	IDirect3DDevice9_SetRenderState(pDevice, D3DRS_STENCILENABLE, TRUE);
	/* ?glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE); */

	if (aux->fade_in) {
		IDirect3DDevice9_SetRenderState(pDevice, D3DRS_STENCILFUNC, D3DCMP_EQUAL);
		IDirect3DDevice9_SetRenderState(pDevice, D3DRS_STENCILREF, 1);
		IDirect3DDevice9_SetRenderState(pDevice, D3DRS_STENCILMASK, 1);
	}
	else {
		IDirect3DDevice9_SetRenderState(pDevice, D3DRS_STENCILFUNC, D3DCMP_NOTEQUAL);
		IDirect3DDevice9_SetRenderState(pDevice, D3DRS_STENCILREF, 1);
		IDirect3DDevice9_SetRenderState(pDevice, D3DRS_STENCILMASK, 1);
	}

	IDirect3DDevice9_SetRenderState(pDevice, D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);
	IDirect3DDevice9_SetRenderState(pDevice, D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP);
	IDirect3DDevice9_SetRenderState(pDevice, D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);

	if ((al_get_bitmap_width(aux->bmp) == aux->width) && (al_get_bitmap_height(aux->bmp) == aux->height)) {
		al_draw_bitmap(aux->bmp, aux->x, aux->y, 0);
	}
	else {
		al_draw_bitmap_region(aux->bmp, aux->x, aux->y, aux->width, aux->height, aux->x, aux->y, 0);
	}

	IDirect3DDevice9_SetRenderState(pDevice, D3DRS_STENCILENABLE, FALSE);
}

static void
VideoA5_TickDissolve_D3DStencil(FadeInAux *aux)
{
	LPDIRECT3DDEVICE9 pDevice = al_get_d3d_device(display);
	if (pDevice == NULL)
		return;

	IDirect3DDevice9_SetRenderState(pDevice, D3DRS_STENCILENABLE, TRUE);
	/* glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE); */
	IDirect3DDevice9_SetRenderState(pDevice, D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
	IDirect3DDevice9_SetRenderState(pDevice, D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);
	IDirect3DDevice9_SetRenderState(pDevice, D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP);
	IDirect3DDevice9_SetRenderState(pDevice, D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE);

	/* 2 triangles per x coordinate. */
	ALLEGRO_VERTEX v[6 * SCREEN_WIDTH];
	assert(aux->width < SCREEN_WIDTH);
	memset(v, 0, sizeof(v));

	int count = 0;
	int j = aux->frame;
	for (int i = 0; i < aux->width; i++) {
		const int x = aux->x + aux->cols[i];
		const int y = aux->y + aux->rows[j];
		const float x1 = x;
		const float x2 = x + 1.00f;
		const float y1 = y;
		const float y2 = y + 1.00f;

		v[count].x = x1; v[count].y = y1; count++;
		v[count].x = x1; v[count].y = y2; count++;
		v[count].x = x2; v[count].y = y1; count++;
		v[count].x = x1; v[count].y = y2; count++;
		v[count].x = x2; v[count].y = y1; count++;
		v[count].x = x2; v[count].y = y2; count++;

		if (++j >= aux->height)
			j = 0;
	}

	al_draw_prim(v, NULL, NULL, 0, 6 * aux->width, ALLEGRO_PRIM_TRIANGLE_LIST);

	IDirect3DDevice9_SetRenderState(pDevice, D3DRS_STENCILENABLE, FALSE);
	/* glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); */
}
#endif

static FadeInAux *
VideoA5_InitFadeInSprite(ALLEGRO_BITMAP *src, int x, int y, int w, int h, bool fade_in)
{
	FadeInAux *aux = &s_fadeInAux;
	assert(w < SCREEN_WIDTH);
	assert(h < SCREEN_HEIGHT);

	for (int i = 0; i < w; i++) {
		aux->cols[i] = i;
	}

	for (int i = 0; i < h; i++) {
		aux->rows[i] = i;
	}

	for (int i = 0; i < w; i++) {
		const int j = Tools_RandomLCG_Range(0, w - 1);
		const int swap = aux->cols[j];

		aux->cols[j] = aux->cols[i];
		aux->cols[i] = swap;
	}

	for (int i = 0; i < h; i++) {
		const int j = Tools_RandomLCG_Range(0, h - 1);
		const int swap = aux->rows[j];

		aux->rows[j] = aux->rows[i];
		aux->rows[i] = swap;
	}

	aux->fade_in = fade_in;
	aux->frame = 0;
	aux->x = x;
	aux->y = y;
	aux->width = w;
	aux->height = h;

	switch (g_graphics_driver) {
		case GRAPHICS_DRIVER_OPENGL:
			VideoA5_InitDissolve_GLStencil(src, aux);
			break;

#ifdef ALLEGRO_WINDOWS
		case GRAPHICS_DRIVER_DIRECT3D:
			VideoA5_InitDissolve_D3DStencil(src, aux);
			break;
#endif

		default:
			/* VideoA5_InitDissolve_LockedBitmap(src, aux); */
			break;
	}

	return aux;
}

void
Video_DrawFadeIn(const FadeInAux *aux)
{
	switch (g_graphics_driver) {
		case GRAPHICS_DRIVER_OPENGL:
			VideoA5_DrawDissolve_GLStencil(aux);
			break;

#ifdef ALLEGRO_WINDOWS
		case GRAPHICS_DRIVER_DIRECT3D:
			VideoA5_DrawDissolve_D3DStencil(aux);
			break;
#endif

		default:
			/* VideoA5_DrawDissolve_LockedBitmap(aux); */
			break;
	}
}

bool
Video_TickFadeIn(FadeInAux *aux)
{
	if (aux == NULL)
		return true;

	assert(0 <= aux->frame && aux->frame <= aux->height);
	if (aux->frame >= aux->height)
		return true;

	switch (g_graphics_driver) {
		case GRAPHICS_DRIVER_OPENGL:
			VideoA5_TickDissolve_GLStencil(aux);
			break;

#ifdef ALLEGRO_WINDOWS
		case GRAPHICS_DRIVER_DIRECT3D:
			VideoA5_TickDissolve_D3DStencil(aux);
			break;
#endif

		default:
			/* VideoA5_TickDissolve_LockedBitmap(aux); */
			break;
	}

	aux->frame++;
	return false;
}

/*--------------------------------------------------------------*/

static CPSStore *
VideoA5_ExportCPS(enum SearchDirectory dir, const char *filename, unsigned char *buf)
{
	CPSStore *cps = malloc(sizeof(*cps));
	if (cps == NULL)
		return NULL;

	cps->next = NULL;
	if (dir == SEARCHDIR_CAMPAIGN_DIR) {
		snprintf(cps->filename, sizeof(cps->filename), "%s%s", g_campaign_list[g_campaign_selected].dir_name, filename);
	}
	else {
		snprintf(cps->filename, sizeof(cps->filename), "%s", filename);
	}

	cps->bmp = al_create_bitmap(SCREEN_WIDTH, SCREEN_HEIGHT);
	if (cps->bmp == NULL) {
		free(cps);
		return NULL;
	}

	bool use_benepal = false;
	if ((strncmp(filename, "MENTATM.CPS", 11) == 0) ||
	    (strncmp(filename, "MISC", 4) == 0)) {
		use_benepal = true;
	}

	VideoA5_ReadPalette(use_benepal ? "BENE.PAL" : "IBM.PAL");
	memset(buf, 0, SCREEN_WIDTH * SCREEN_HEIGHT);
	Sprites_LoadImage(dir, filename, SCREEN_1, NULL);
	VideoA5_CopyBitmap(SCREEN_WIDTH, buf, cps->bmp, BLACK_COLOUR_0);
	VideoA5_ReadPalette("IBM.PAL");

	return cps;
}

static CPSStore *
VideoA5_LoadCPS(enum SearchDirectory dir, const char *filename)
{
	char campname[1024];
	CPSStore *cps;

	if (dir == SEARCHDIR_CAMPAIGN_DIR) {
		snprintf(campname, sizeof(campname), "%s%s", g_campaign_list[g_campaign_selected].dir_name, filename);
	}
	else {
		snprintf(campname, sizeof(campname), "%s", filename);
	}

	cps = s_cps;
	while (cps != NULL) {
		if (strncmp(cps->filename, campname, sizeof(cps->filename)) == 0)
			return cps;

		cps = cps->next;
	}

	cps = VideoA5_ExportCPS(dir, filename, GFX_Screen_Get_ByIndex(SCREEN_1));
	if (cps == NULL)
		return NULL;

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

/* Draw bitmap region, but add a single pixel of padding on along each
 * side for blending purposes.
 *
 * If expand_x or expand_y is true, then take the data from around the
 * source bitmap.  Otherwise, use the edge of the source bitmap.
 */
static void
VideoA5_DrawBitmapRegion_Padded(ALLEGRO_BITMAP *src,
		const struct CPSSpecialCoord *coord, float tx, float ty, bool expand_x, bool expand_y)
{
	float sx = coord->cx;
	float sy = coord->cy;
	float w = coord->w;
	float h = coord->h;

	if (coord->cx < 1) expand_x = false;
	if (coord->cy < 1) expand_y = false;

	if (expand_x) {
		sx -= 1.0f;
		tx -= 1.0f;
		w += 2.0f;
	}

	if (expand_y) {
		sy -= 1.0f;
		ty -= 1.0f;
		h += 2.0f;
	}

	if (!expand_x) {
		al_draw_bitmap_region(src, sx, sy, 1.0f, h, tx - 1.0f, ty, 0);
		al_draw_bitmap_region(src, sx + w - 1.0f, sy, 1.0f, h, tx + w, ty, 0);
	}

	if (!expand_y) {
		al_draw_bitmap_region(src, sx, sy, w, 1.0f, tx, ty - 1.0f, 0);
		al_draw_bitmap_region(src, sx, sy + h - 1.0f, w, 1.0f, tx, ty + h, 0);
	}

	al_draw_bitmap_region(src, sx, sy, w, h, tx, ty, 0);
}

static void
VideoA5_InitCPS(void)
{
	const struct CPSSpecialCoord *coord;
	unsigned char *buf = GFX_Screen_Get_ByIndex(SCREEN_1);

	VideoA5_SetBitmapFlags(ALLEGRO_MEMORY_BITMAP);
	al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);

	CPSStore *cps_screen = VideoA5_ExportCPS(SEARCHDIR_GLOBAL_DATA_DIR, "SCREEN.CPS", buf);
	CPSStore *cps_fame = VideoA5_LoadCPS(SEARCHDIR_GLOBAL_DATA_DIR, "FAME.CPS");
	CPSStore *cps_mapmach = VideoA5_LoadCPS(SEARCHDIR_GLOBAL_DATA_DIR, "MAPMACH.CPS");
	assert(cps_screen != NULL && cps_fame != NULL && cps_mapmach != NULL);

	VideoA5_SetBitmapFlags(ALLEGRO_VIDEO_BITMAP);

	al_set_target_bitmap(interface_texture);

	for (enum CPSID cpsID = CPS_MENUBAR_LEFT; cpsID <= CPS_STATUSBAR_RIGHT; cpsID++) {
		coord = &cps_special_coord[cpsID];

		if (coord->cx < 0 || coord->cy < 0)
			continue;

		VideoA5_DrawBitmapRegion_Padded(cps_screen->bmp, coord, coord->tx, coord->ty, true, true);
	}

	coord = &cps_special_coord[CPS_STATUSBAR_MIDDLE];
	al_draw_bitmap_region(cps_screen->bmp,  8, 17 - 1, 304, coord->h + 1, coord->tx - 1, coord->ty - 1, 0);
	al_draw_bitmap_region(cps_screen->bmp, 55, 17 - 1, 123, coord->h + 1, coord->tx + 303, coord->ty - 1, 0);
	al_draw_filled_rectangle(coord->tx - 1.0f, coord->ty + coord->h, coord->tx + coord->w + 1.5f, coord->ty + coord->h + 1.0f, al_map_rgb(0, 0, 0));

	coord = &cps_special_coord[CPS_STATUSBAR_RIGHT];
	al_draw_filled_rectangle(coord->tx - 1.0f, coord->ty + coord->h, coord->tx + coord->w + 1.5f, coord->ty + coord->h + 1.0f, al_map_rgb(0, 0, 0));

	for (enum HouseType houseID = HOUSE_HARKONNEN; houseID < HOUSE_MAX; houseID++) {
		Sprites_LoadImage(SEARCHDIR_GLOBAL_DATA_DIR, "SCREEN.CPS", SCREEN_1, NULL);
		GUI_Palette_CreateRemap(houseID);
		GUI_Palette_RemapScreen(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_1, g_remap);
		VideoA5_CopyBitmap(SCREEN_WIDTH, buf, cps_screen->bmp, TRANSPARENT_COLOUR_0);

		coord = &cps_special_coord[CPS_SIDEBAR_TOP];
		VideoA5_DrawBitmapRegion_Padded(cps_screen->bmp, coord, coord->tx + 17 * houseID, coord->ty + 4 * houseID, true, false);

		coord = &cps_special_coord[CPS_SIDEBAR_MIDDLE];
		VideoA5_DrawBitmapRegion_Padded(cps_screen->bmp, coord, coord->tx + 17 * houseID, coord->ty + 4 * houseID, true, true);

		coord = &cps_special_coord[CPS_SIDEBAR_BOTTOM];
		VideoA5_DrawBitmapRegion_Padded(cps_screen->bmp, coord, coord->tx + 17 * houseID, coord->ty + 23 * houseID, true, true);
	}

	coord = &cps_special_coord[CPS_MENUBAR_MIDDLE];
	al_draw_bitmap_region(cps_fame->bmp, 134, 37, 111, coord->h + 2, coord->tx - 1, coord->ty - 1, 0);
	al_draw_bitmap_region(cps_mapmach->bmp, 55, 183, 211, coord->h, coord->tx + 110, coord->ty - 1, 0);
	al_draw_bitmap_region(cps_mapmach->bmp, 55, 198, 211, 2, coord->tx + 110, coord->ty - 1 + coord->h, ALLEGRO_FLIP_VERTICAL);

	coord = &cps_special_coord[CPS_CONQUEST_EN];
	assert(coord->tx == cps_special_coord[CPS_CONQUEST_FR].tx);
	assert(coord->tx == cps_special_coord[CPS_CONQUEST_DE].tx);
	VideoA5_DrawBitmapRegion_Padded(cps_mapmach->bmp, &cps_special_coord[CPS_CONQUEST_EN], coord->tx, cps_special_coord[CPS_CONQUEST_EN].ty, true, false);
	VideoA5_DrawBitmapRegion_Padded(cps_mapmach->bmp, &cps_special_coord[CPS_CONQUEST_FR], coord->tx, cps_special_coord[CPS_CONQUEST_FR].ty, false, false);
	VideoA5_DrawBitmapRegion_Padded(cps_mapmach->bmp, &cps_special_coord[CPS_CONQUEST_DE], coord->tx, cps_special_coord[CPS_CONQUEST_DE].ty, false, false);

	Sprites_LoadImage(SEARCHDIR_GLOBAL_DATA_DIR, "MAPMACH.CPS", SCREEN_1, NULL);
	ALLEGRO_LOCKED_REGION *reg = al_lock_bitmap(interface_texture, ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, ALLEGRO_LOCK_READWRITE);
	VideoA5_CreateWhiteMask(buf, reg, SCREEN_WIDTH, coord->cx, cps_special_coord[CPS_CONQUEST_EN].cy, coord->tx, cps_special_coord[CPS_CONQUEST_EN].ty + 30, coord->w, 20, CONQUEST_COLOUR);
	VideoA5_CreateWhiteMask(buf, reg, SCREEN_WIDTH, coord->cx, cps_special_coord[CPS_CONQUEST_FR].cy, coord->tx, cps_special_coord[CPS_CONQUEST_FR].ty + 30, coord->w, 20, CONQUEST_COLOUR);
	VideoA5_CreateWhiteMask(buf, reg, SCREEN_WIDTH, coord->cx, cps_special_coord[CPS_CONQUEST_DE].cy, coord->tx, cps_special_coord[CPS_CONQUEST_DE].ty + 30, coord->w, 20, CONQUEST_COLOUR);
	al_unlock_bitmap(interface_texture);

	/* Create cps_special_texture, free cps_screen, and convert cps_fame, cps_mapmach to video bitmaps. */
	VideoA5_FreeCPS(cps_screen);
	cps_fame->bmp = VideoA5_ConvertToVideoBitmap(cps_fame->bmp);
	cps_mapmach->bmp = VideoA5_ConvertToVideoBitmap(cps_mapmach->bmp);
}

void
VideoA5_DrawCPS(enum SearchDirectory dir, const char *filename)
{
	CPSStore *cps = VideoA5_LoadCPS(dir, filename);

	if (cps != NULL)
		al_draw_bitmap(cps->bmp, 0, 0, 0);
}

void
VideoA5_DrawCPSRegion(enum SearchDirectory dir, const char *filename, int sx, int sy, int dx, int dy, int w, int h)
{
	CPSStore *cps = VideoA5_LoadCPS(dir, filename);

	if (cps != NULL)
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

		al_draw_bitmap_region(interface_texture, sx, sy, coord->w, coord->h, x, y, 0);
		al_draw_tinted_bitmap_region(interface_texture, col, sx, sy + 30, coord->w, 20, x, y, 0);
		return;
	}

	if (CPS_SIDEBAR_TOP <= cpsID && cpsID <= CPS_SIDEBAR_BOTTOM) {
		sx += 17 * houseID;

		if (cpsID == CPS_SIDEBAR_BOTTOM) {
			sy += 23 * houseID;
		}
		else {
			sy += 4 * houseID;
		}
	}

	al_draw_bitmap_region(interface_texture, sx, sy, coord->w, coord->h, x, y, 0);
}

void
VideoA5_DrawCPSSpecialScale(enum CPSID cpsID, enum HouseType houseID, int x, int y, float scale)
{
	/* This is only used to draw interface when rendering the blur brush. */
	assert(CPS_SIDEBAR_TOP <= cpsID && cpsID <= CPS_SIDEBAR_BOTTOM);
	assert(houseID < HOUSE_MAX);

	const struct CPSSpecialCoord *coord = &cps_special_coord[cpsID];
	int sx = coord->tx + 17 * houseID;
	int sy = coord->ty;

	if (cpsID == CPS_SIDEBAR_BOTTOM) {
		sy += 23 * houseID;
	}
	else {
		sy += 4 * houseID;
	}

	al_draw_scaled_bitmap(interface_texture, sx, sy, coord->w, coord->h,
			x, y, scale * coord->w, scale * coord->h, 0);
}

FadeInAux *
Video_InitFadeInCPS(const char *filename, int x, int y, int w, int h, bool fade_in)
{
	CPSStore *cps = VideoA5_LoadCPS(SEARCHDIR_GLOBAL_DATA_DIR, filename);
	if (cps == NULL)
		return NULL;

	return VideoA5_InitFadeInSprite(cps->bmp, x, y, w, h, fade_in);
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

static IconConnectivity *
VideoA5_CreateIconConnectivities(void)
{
	IconConnectivity *connect = calloc(ICONID_MAX, sizeof(connect[0]));
	assert(connect != NULL);

	for (enum StructureType s = STRUCTURE_PALACE; s < STRUCTURE_MAX; s++) {
		const enum StructureLayout layout = g_table_structureInfo[s].layout;
		const enum IconMapEntries group = g_table_structureInfo[s].iconGroup;
		const int w = g_table_structure_layoutSize[layout].width;
		const int h = g_table_structure_layoutSize[layout].height;
		const int count = VideoA5_NumIconsInGroup(group);

		for (int idx0 = 0; idx0 < count; idx0 += w * h) {
			for (int i = 0; i < h; i++) {
				for (int j = 0; j < w; j++) {
					const uint16 iconID = g_iconMap[g_iconMap[group] + idx0 + (w * i + j)];

					/* Find icon to pad this icon with. */
					if (i > 0)   connect[iconID].iconU = g_iconMap[g_iconMap[group] + idx0 + (w * (i-1) + j)];
					if (i+1 < h) connect[iconID].iconD = g_iconMap[g_iconMap[group] + idx0 + (w * (i+1) + j)];
					if (j > 0)   connect[iconID].iconL = g_iconMap[g_iconMap[group] + idx0 + (w * i + (j-1))];
					if (j+1 < w) connect[iconID].iconR = g_iconMap[g_iconMap[group] + idx0 + (w * i + (j+1))];
				}
			}
		}
	}

	return connect;
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

			if (s_icon[iconID][houseID].sx != 0 || s_icon[iconID][houseID].sy != 0)
				continue;

			if ((idx >= num_common) || (houseID == HOUSE_HARKONNEN)) {
				VideoA5_GetNextXY(WINDOW_W, WINDOW_H, x, y, TILE_SIZE + 1, TILE_SIZE + 1, TILE_SIZE + 1, &x, &y);

				GFX_DrawSprite_(iconID, x, y, houseID);

				s_icon[iconID][houseID].sx = x;
				s_icon[iconID][houseID].sy = y;
				x += TILE_SIZE + 2;
			}
			else {
				s_icon[iconID][houseID] = s_icon[iconID][HOUSE_HARKONNEN];
			}
		}
	}

	*retx = 1;
	*rety = y + TILE_SIZE + 2;
}

static void
VideoA5_DrawIconPadding(ALLEGRO_BITMAP *membmp, IconConnectivity *connect)
{
	ALLEGRO_BITMAP *dup = al_clone_bitmap(membmp);

	if (dup == NULL)
		return;

	al_set_target_bitmap(membmp);

	for (uint16 iconID = 0; iconID < ICONID_MAX; iconID++) {
		const IconCoord *targ = &s_icon[iconID][HOUSE_HARKONNEN];

		if (targ->sx == 0 && targ->sy == 0)
			continue;

		for (enum HouseType houseID = HOUSE_HARKONNEN; houseID < HOUSE_MAX; houseID++) {
			const IconCoord *src;
			targ = &s_icon[iconID][houseID];

			if ((houseID != HOUSE_HARKONNEN) &&
					(targ->sx == s_icon[iconID][HOUSE_HARKONNEN].sx) &&
					(targ->sy == s_icon[iconID][HOUSE_HARKONNEN].sy))
				break;

			/* Pad the top of the icon with the top row of itself or the bottom row of the connected icon. */
			src = &s_icon[connect[iconID].iconU][houseID];
			if (connect[iconID].iconU == 0) {
				al_draw_bitmap_region(dup, targ->sx, targ->sy, TILE_SIZE, 1.0f,
						targ->sx, targ->sy - 1.0f, 0);
			}
			else if (src->sx != 0 && src->sy != 0) {
				al_draw_bitmap_region(dup, src->sx, src->sy + TILE_SIZE - 1.0f, TILE_SIZE, 1.0f,
						targ->sx, targ->sy - 1.0f, 0);
			}

			/* Pad the bottom. */
			src = &s_icon[connect[iconID].iconD][houseID];
			if (connect[iconID].iconD == 0) {
				al_draw_bitmap_region(dup, targ->sx, targ->sy + TILE_SIZE - 1.0f, TILE_SIZE, 1.0f,
						targ->sx, targ->sy + TILE_SIZE, 0);
			}
			else if (src->sx != 0 && src->sy != 0) {
				al_draw_bitmap_region(dup, src->sx, src->sy, TILE_SIZE, 1.0f,
						targ->sx, targ->sy + TILE_SIZE, 0);
			}

			/* Pad the left. */
			src = &s_icon[connect[iconID].iconL][houseID];
			if (connect[iconID].iconL == 0) {
				al_draw_bitmap_region(dup, targ->sx, targ->sy, 1.0f, TILE_SIZE,
						targ->sx - 1.0f, targ->sy, 0);
			}
			else if (src->sx != 0 && src->sy != 0) {
				al_draw_bitmap_region(dup, src->sx + TILE_SIZE - 1.0f, src->sy, 1.0f, TILE_SIZE,
						targ->sx - 1.0f, targ->sy, 0);
			}

			/* Pad the right. */
			src = &s_icon[connect[iconID].iconR][houseID];
			if (connect[iconID].iconR == 0) {
				al_draw_bitmap_region(dup, targ->sx + TILE_SIZE - 1.0f, targ->sy, 1.0f, TILE_SIZE,
						targ->sx + TILE_SIZE, targ->sy, 0);
			}
			else if (src->sx != 0 && src->sy != 0) {
				al_draw_bitmap_region(dup, src->sx, src->sy, 1.0f, TILE_SIZE,
						targ->sx + TILE_SIZE, targ->sy, 0);
			}
		}
	}

	al_destroy_bitmap(dup);
}

static void
VideoA5_ExportWindtrapOverlay(unsigned char *buf, uint16 iconID,
		int x, int y, int *retx, int *rety)
{
	const int WINDOW_W = g_widgetProperties[WINDOWID_RENDER_TEXTURE].width;
	const int WINDOW_H = g_widgetProperties[WINDOWID_RENDER_TEXTURE].height;
	const int idx = ICONID_MAX - (iconID - g_iconMap[g_iconMap[ICM_ICONGROUP_WINDTRAP_POWER] + 8]) - 1;

	if (s_icon[idx][HOUSE_HARKONNEN].sx != 0 || s_icon[idx][HOUSE_HARKONNEN].sy != 0)
		return;

	VideoA5_GetNextXY(WINDOW_W, WINDOW_H, x, y, TILE_SIZE, TILE_SIZE, TILE_SIZE, &x, &y);
	GFX_DrawSprite_(iconID, x, y, HOUSE_HARKONNEN);

	for (enum HouseType houseID = HOUSE_HARKONNEN; houseID < HOUSE_MAX; houseID++) {
		s_icon[idx][houseID].sx = x;
		s_icon[idx][houseID].sy = y;
	}

	VideoA5_CreateWhiteMaskIndexed(buf, WINDOW_W, x, y, x, y, TILE_SIZE, TILE_SIZE, WINDTRAP_COLOUR);

	*retx = x + TILE_SIZE + 1;
	*rety = y;
}

#if 0
static ALLEGRO_BITMAP *
VideoA5_InitExternalTiles(const char *mapfile, const char *bmpfile, int size)
{
	ALLEGRO_BITMAP *dst = NULL;
	char filename[1024];
	assert(size == 16 || size == 32 || size == 48);

	snprintf(filename, sizeof(filename), "%s/gfx/%s", g_dune_data_dir, mapfile);
	FILE *fp = fopen(filename, "r");
	if (fp == NULL)
		return NULL;

	snprintf(filename, sizeof(filename), "%s/gfx/%s", g_dune_data_dir, bmpfile);
	ALLEGRO_BITMAP *src = al_load_bitmap(filename);
	if (src == NULL)
		goto end;

	int w, h, c;
	if (fscanf(fp, "%d %d\n", &w, &h) != 2)
		goto end;

	do {
		c = fgetc(fp);
	} while (c != EOF && c != '\n');

	if (size != 16) {
		dst = al_create_bitmap(w, h);
		if (dst == NULL)
			goto end;
	}
	else {
		dst = icon_texture;
	}

	al_set_target_bitmap(dst);

	int sx = 0, sy = 0;
	int dx = 1, dy = 1;
	while (!feof(fp)) {
		int iconID;

		if (fscanf(fp, "%d", &iconID) != 1) {
			/* Skip until next line. */
			do {
				c = fgetc(fp);
			} while (c != EOF && c != '\n');

			sx = 0;
			sy += size;
			continue;
		}

		if (iconID <= 0 || iconID >= ICONID_MAX) {
			sx += size;
			continue;
		}

		for (enum HouseType houseID = HOUSE_HARKONNEN; houseID < HOUSE_MAX; houseID++) {
			IconCoord *coord = &s_icon[iconID][houseID];

			if (houseID != HOUSE_HARKONNEN &&
					coord->sx == s_icon[iconID][HOUSE_HARKONNEN].sx &&
					coord->sy == s_icon[iconID][HOUSE_HARKONNEN].sy) {
				if (size == 32) {
					coord->sx32 = s_icon[iconID][HOUSE_HARKONNEN].sx32;
					coord->sy32 = s_icon[iconID][HOUSE_HARKONNEN].sy32;
				}
				else if (size == 48) {
					coord->sx48 = s_icon[iconID][HOUSE_HARKONNEN].sx48;
					coord->sy48 = s_icon[iconID][HOUSE_HARKONNEN].sy48;
				}
			}
			else {
				if (size == 16) {
					dx = coord->sx;
					dy = coord->sy;
				}
				else {
					VideoA5_GetNextXY(w, h, dx, dy, size + 1, size + 1, size + 1, &dx, &dy);
				}

				al_draw_bitmap_region(src, sx, sy, size, size, dx, dy, 0);

				/* Remap. */
				if (houseID != HOUSE_HARKONNEN) {
					ALLEGRO_LOCKED_REGION *reg;

					reg = al_lock_bitmap_region(dst, dx, dy, size, size, ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, ALLEGRO_LOCK_READWRITE);
					assert(reg != NULL);

					for (int y = 0; y < size; y++) {
						unsigned char *row = &((unsigned char *)reg->data)[reg->pitch*y];

						for (int x = 0; x < size; x++) {
							for (int c = 144; c <= 150; c++) {
								if (memcmp(&row[reg->pixel_size * x], &paletteRGB[3*c], 3) == 0) {
									row[reg->pixel_size * x + 0] = paletteRGB[3 * (c + 16 * houseID) + 0];
									row[reg->pixel_size * x + 1] = paletteRGB[3 * (c + 16 * houseID) + 1];
									row[reg->pixel_size * x + 2] = paletteRGB[3 * (c + 16 * houseID) + 2];
									break;
								}
							}
						}
					}

					al_unlock_bitmap(dst);
				}

				if (size == 32) {
					coord->sx32 = dx;
					coord->sy32 = dy;
				}
				else if (size == 48) {
					coord->sx48 = dx;
					coord->sy48 = dy;
				}

				dx += size + 2;
			}
		}

		sx += size;
	}

end:

	fclose(fp);
	al_destroy_bitmap(src);

	return dst;
}
#endif

static void
VideoA5_CreateInvalidPlacementMask(int x1, int y1)
{
	for (enum StructureLayout layout = STRUCTURE_LAYOUT_1x1; layout <= STRUCTURE_LAYOUT_3x3; layout++) {
		const int w = g_table_structure_layoutSize[layout].width;
		const int h = g_table_structure_layoutSize[layout].height;
		const int x2 = x1 + w * TILE_SIZE - 1;
		const int y2 = y1 + h * TILE_SIZE - 1;

		Prim_Rect_i(x1, y1, x2, y2, 0xFF);
		Prim_Line(x1 + 0.5f, y1 + 0.5f, x2 + 0.5f, y2 + 0.5f, 0xFF, 0.0f);
		Prim_Line(x2 + 0.5f, y1 + 0.5f, x1 + 0.5f, y2 + 0.5f, 0xFF, 0.0f);

		x1 += w * TILE_SIZE + 1;
	}
}

static void
VideoA5_MaskDebrisTiles(ALLEGRO_BITMAP *membmp)
{
	const struct {
		int maskx, masky;
		enum IconMapEntries group;
		int idx;
	} rubble[1 + 4 + 4 + 9] = {
		{  0,  0, ICM_ICONGROUP_BASE_DEFENSE_TURRET, 1},
		{  0, 16, ICM_ICONGROUP_LIGHT_VEHICLE_FACTORY, 4 },
		{ 16, 16, ICM_ICONGROUP_LIGHT_VEHICLE_FACTORY, 5 },
		{  0, 32, ICM_ICONGROUP_LIGHT_VEHICLE_FACTORY, 6 },
		{ 16, 32, ICM_ICONGROUP_LIGHT_VEHICLE_FACTORY, 7 },
		{  0, 16, ICM_ICONGROUP_RADAR_OUTPOST, 4 },
		{ 16, 16, ICM_ICONGROUP_RADAR_OUTPOST, 5 },
		{  0, 32, ICM_ICONGROUP_RADAR_OUTPOST, 6 },
		{ 16, 32, ICM_ICONGROUP_RADAR_OUTPOST, 7 },
		{ 32,  0, ICM_ICONGROUP_HOUSE_PALACE,  9 },
		{ 48,  0, ICM_ICONGROUP_HOUSE_PALACE, 10 },
		{ 64,  0, ICM_ICONGROUP_HOUSE_PALACE, 11 },
		{ 32, 16, ICM_ICONGROUP_HOUSE_PALACE, 12 },
		{ 48, 16, ICM_ICONGROUP_HOUSE_PALACE, 13 },
		{ 64, 16, ICM_ICONGROUP_HOUSE_PALACE, 14 },
		{ 32, 32, ICM_ICONGROUP_HOUSE_PALACE, 15 },
		{ 48, 32, ICM_ICONGROUP_HOUSE_PALACE, 16 },
		{ 64, 32, ICM_ICONGROUP_HOUSE_PALACE, 17 },
	};

	char filename[1024];
	snprintf(filename, sizeof(filename), "%s/gfx/rubblemask.png", g_dune_data_dir);

	ALLEGRO_BITMAP *mask = al_load_bitmap(filename);
	if (mask == NULL)
		return;

	for (int i = 0; i < 1 + 4 + 4 + 9; i++) {
		const uint16 iconID = g_iconMap[g_iconMap[rubble[i].group] + rubble[i].idx];
		const IconCoord *coord = &s_icon[iconID][HOUSE_HARKONNEN];

		ALLEGRO_LOCKED_REGION *read = al_lock_bitmap_region(mask, rubble[i].maskx, rubble[i].masky, TILE_SIZE, TILE_SIZE, ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, ALLEGRO_LOCK_READONLY);
		ALLEGRO_LOCKED_REGION *write = al_lock_bitmap_region(membmp, coord->sx, coord->sy, TILE_SIZE, TILE_SIZE, ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, ALLEGRO_LOCK_READWRITE);

		for (int y = 0; y < TILE_SIZE; y++) {
			unsigned char *read_row = &((unsigned char *)read->data)[read->pitch*y];
			unsigned char *write_row = &((unsigned char *)write->data)[write->pitch*y];

			for (int x = 0; x < TILE_SIZE; x++) {
				/* Use read's red channel as the write's alpha. */
				write_row[write->pixel_size*x + 3] = read_row[read->pixel_size*x];
			}
		}

		al_unlock_bitmap(membmp);
		al_unlock_bitmap(mask);
	}

	al_destroy_bitmap(mask);
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

	IconConnectivity *connect = VideoA5_CreateIconConnectivities();
	int x, y;

	x = 1, y = 1;
	for (int i = 0; icon_data[i].group < ICM_ICONGROUP_EOF; i++) {
		VideoA5_ExportIconGroup(icon_data[i].group, icon_data[i].num_common, x, y, &x, &y);
	}

	/* Windtraps.  304..308 in EU v1.07, 310..314 in US v1.0. */
	for (uint16 i = 8; i <= 15; i++) {
		const uint16 iconID = g_iconMap[g_iconMap[ICM_ICONGROUP_WINDTRAP_POWER] + i];

		VideoA5_ExportWindtrapOverlay(buf, iconID, x, y, &x, &y);
	}

	/* Copy buf to memory bitmap. */
	const int WINDOW_W = g_widgetProperties[WINDOWID_RENDER_TEXTURE].width;
	const int WINDOW_H = g_widgetProperties[WINDOWID_RENDER_TEXTURE].height;

	VideoA5_SetBitmapFlags(ALLEGRO_MEMORY_BITMAP);
	icon_texture = al_create_bitmap(WINDOW_W, WINDOW_H);
	assert(icon_texture != NULL);

	al_set_target_bitmap(icon_texture);
	al_clear_to_color(al_map_rgba(0, 0, 0, 0));
	VideoA5_CopyBitmap(WINDOW_W, buf, icon_texture, TRANSPARENT_COLOUR_0);

	/* Generate pixellated invalid placement mask things. */
	VideoA5_CreateInvalidPlacementMask(1, 975);

	/* Apply rubble mask for transparent rubble. */
	VideoA5_MaskDebrisTiles(icon_texture);

#if 0
	/* Load external tile sheets. */
	VideoA5_InitExternalTiles("icons16.map", "icons16.png", 16);
	icon_texture32 = VideoA5_InitExternalTiles("icons32.map", "icons32.png", 32);
	icon_texture48 = VideoA5_InitExternalTiles("icons48.map", "icons48.png", 48);
#endif

	/* Connect neighbours for interpolation. */
	VideoA5_DrawIconPadding(icon_texture, connect);

#if OUTPUT_TEXTURES
	al_save_bitmap("icons16.png", icon_texture);
	al_save_bitmap("icons32.png", icon_texture32);
	al_save_bitmap("icons48.png", icon_texture48);
#endif

	VideoA5_SetBitmapFlags(ALLEGRO_VIDEO_BITMAP);

	const int bitmap_flags = al_get_new_bitmap_flags();
	al_set_new_bitmap_flags(bitmap_flags & ~ALLEGRO_NO_PRESERVE_TEXTURE);

	icon_texture = VideoA5_ConvertToVideoBitmap(icon_texture);
	assert(icon_texture != NULL);

	if (icon_texture32 != NULL)
		icon_texture32 = VideoA5_ConvertToVideoBitmap(icon_texture32);

	if (icon_texture48 != NULL)
		icon_texture48 = VideoA5_ConvertToVideoBitmap(icon_texture48);

	al_set_new_bitmap_flags(bitmap_flags);
	free(connect);
}

void
VideoA5_DrawIcon(uint16 iconID, enum HouseType houseID, int x, int y)
{
	assert(iconID < ICONID_MAX);
	assert(houseID < HOUSE_MAX);

	const IconCoord *coord = &s_icon[iconID][houseID];
	assert(coord->sx != 0 && coord->sy != 0);

	/* Windtraps need special overlay. */
	const bool is_windtrap = (g_iconMap[g_iconMap[ICM_ICONGROUP_WINDTRAP_POWER] + 8] <= iconID && iconID <= g_iconMap[g_iconMap[ICM_ICONGROUP_WINDTRAP_POWER] + 15]);
	const IconCoord *overlay = NULL;

	if (is_windtrap) {
		uint16 overlayID = ICONID_MAX - (iconID - g_iconMap[g_iconMap[ICM_ICONGROUP_WINDTRAP_POWER] + 8]) - 1;
		overlay = &s_icon[overlayID][HOUSE_HARKONNEN];
	}

	if (2.99f <= g_screenDiv[SCREENDIV_VIEWPORT].scalex &&
			icon_texture48 != NULL && coord->sx48 != 0 && coord->sy48 != 0) {
		al_draw_scaled_bitmap(icon_texture48, coord->sx48, coord->sy48, 48, 48, x, y, TILE_SIZE, TILE_SIZE, 0);

		if (overlay) {
			al_draw_tinted_scaled_bitmap(icon_texture48, paltoRGB[WINDTRAP_COLOUR],
					overlay->sx48, overlay->sy48, 48, 48, x, y, TILE_SIZE, TILE_SIZE, 0);
		}
	}
	else if (1.99f <= g_screenDiv[SCREENDIV_VIEWPORT].scalex && g_screenDiv[SCREENDIV_VIEWPORT].scalex <= 2.01f &&
			icon_texture32 != NULL && coord->sx32 != 0 && coord->sy32 != 0) {
		al_draw_scaled_bitmap(icon_texture32, coord->sx32, coord->sy32, 32, 32, x, y, TILE_SIZE, TILE_SIZE, 0);

		if (overlay) {
			al_draw_tinted_scaled_bitmap(icon_texture32, paltoRGB[WINDTRAP_COLOUR],
					overlay->sx32, overlay->sy32, 32, 32, x, y, TILE_SIZE, TILE_SIZE, 0);
		}
	}
	else {
		al_draw_bitmap_region(icon_texture, coord->sx, coord->sy, TILE_SIZE, TILE_SIZE, x, y, 0);

		if (overlay) {
			al_draw_tinted_bitmap_region(icon_texture, paltoRGB[WINDTRAP_COLOUR],
					overlay->sx, overlay->sy, TILE_SIZE, TILE_SIZE, x, y, 0);
		}
	}
}

void
VideoA5_DrawIconAlpha(uint16 iconID, int x, int y, unsigned char alpha)
{
	assert(iconID < ICONID_MAX);

	const IconCoord *coord = &s_icon[iconID][HOUSE_HARKONNEN];
	assert(coord->sx != 0 && coord->sy != 0);

	ALLEGRO_COLOR tint = al_map_rgba(0, 0, 0, alpha);

	al_draw_tinted_bitmap_region(icon_texture, tint,
			coord->sx, coord->sy, TILE_SIZE, TILE_SIZE, x, y, 0);
}

void
VideoA5_DrawRectCross(int x1, int y1, int w, int h, unsigned char c)
{
	/* sx[((h - 1) << 2) | w] -> int. 1 <= w, h <= 3 */
	const int sx[] = {
		0,  1,  18,   0, /* h = 1 */
		0, 51,  68, 134, /* h = 2 */
		0,  0, 101, 183, /* h = 3 */
	};

	if (enhancement_high_res_overlays || w >= 4 || h >= 4) {
		const int x2 = x1 + (w * TILE_SIZE) - 1;
		const int y2 = y1 + (h * TILE_SIZE) - 1;

		Prim_Rect_i(x1, y1, x2, y2, c);
		Prim_Line(x1 + 0.33f, y1 + 0.33f, x2 + 0.66f, y2 + 0.66f, c, 0.75f);
		Prim_Line(x2 + 0.66f, y1 + 0.33f, x1 + 0.33f, y2 + 0.66f, c, 0.75f);
	}
	else {
		const int idx = ((h - 1) << 2) | w;

		al_draw_tinted_bitmap_region(icon_texture, paltoRGB[c],
				sx[idx], 975, w * TILE_SIZE, h * TILE_SIZE, x1, y1, 0);
	}
}

/*--------------------------------------------------------------*/

static ALLEGRO_BITMAP *
VideoA5_ExportShape(enum ShapeID shapeID, int x, int y, int row_h,
		int *retx, int *rety, int *ret_row_h, unsigned char *remap)
{
	ALLEGRO_BITMAP *dest = al_get_target_bitmap();
	const int TEXTURE_W = al_get_bitmap_width(dest);
	const int TEXTURE_H = al_get_bitmap_height(dest);
	const int w = Shape_Width(shapeID);
	const int h = Shape_Height(shapeID);

	ALLEGRO_BITMAP *bmp;

	VideoA5_GetNextXY(TEXTURE_W, TEXTURE_H, x, y, w, h, row_h, &x, &y);
	GUI_DrawSprite_(SCREEN_0, g_sprites[shapeID], x, y, WINDOWID_RENDER_TEXTURE, 0x100, remap, 1);

	bmp = al_create_sub_bitmap(dest, x, y, w, h);
	assert(bmp != NULL);

	*retx = x + w + 1;
	*rety = y;
	*ret_row_h = (x <= 1) ? h : max(row_h, h);
	return bmp;
}

static void
VideoA5_InitShapeCHOAMButtons(unsigned char *buf, int y1)
{
	const struct {
		int sx, dx;
		size_t w;
	} cmd[] = {
		{ 544 +  4,   4 +  2,  8 }, /* S  from SEND ORDER. */
		{   4 + 26,   4 + 10,  6 }, /* S  from RESUME GAME. */
		{ 256 + 41,   4 + 16, 16 }, /* TA from MENTAT. */
		{   4 + 87,   4 + 32,  8 }, /* A  from RESUME GAME. */
		{ 544 +101,   4 + 40,  9 }, /* R  from SEND ORDER. */
		{ 128 + 33,   4 + 49,  2 }, /*    from BUILD THIS */
		{ 256 + 64,   4 + 51, 12 }, /* T  from MENTAT. */
		{   4 + 68,   4 + 63, 49 }, /* GAME */

		{ 128 + 96, 128 + 90, 15 }, /* S  from BUILD THIS */
		{ 128 + 23, 128 + 80, 10 }, /* U  from BUILD THIS */
		{ 352 + 16, 128 + 21, 10 }, /* P  from OPTIONS. */
		{ 544 + 66, 128 + 31, 16 }, /* R  from SEND ORDER. */
		{ 448 + 67, 128 + 47,  6 }, /* E  from INVOICE */
		{ 448 + 28, 128 + 53,  9 }, /* V  from INVOICE */
		{ 448 +  7, 128 + 62,  8 }, /* I  from INVOICE */
		{ 352 + 42, 128 + 70,  4 }, /* IO from OPTIONS */
		{ 448 + 41, 128 + 74,  6 }, /* O  from INVOICE */

		{ 128 +  2,   4 +112,  5 }, /* Fill right of START GAME */
		{ 128 +100, 128 + 12,  9 }, /* Fill left of PREVIOUS */
	};

	const int y2 = y1 + 17;
	const int h = 11;

	/* Create a "previous" button from other CHOAM buttons.
	 * The invoice button has 6 pixels of blank to the left.
	 * Avoid using French buttons.
	 */
	Sprites_InitCHOAM("BTTN.ENG", "CHOAM.ENG");
	GUI_DrawSprite_(0, g_sprites[SHAPE_RESUME_GAME + 0], 4, y1, WINDOWID_RENDER_TEXTURE, 0);
	GUI_DrawSprite_(0, g_sprites[SHAPE_RESUME_GAME + 1], 4, y2, WINDOWID_RENDER_TEXTURE, 0);
	GUI_DrawSprite_(0, g_sprites[SHAPE_BUILD_THIS + 0], 128, y1, WINDOWID_RENDER_TEXTURE, 0);
	GUI_DrawSprite_(0, g_sprites[SHAPE_BUILD_THIS + 1], 128, y2, WINDOWID_RENDER_TEXTURE, 0);
	GUI_DrawSprite_(0, g_sprites[SHAPE_MENTAT + 0], 256, y1, WINDOWID_RENDER_TEXTURE, 0);
	GUI_DrawSprite_(0, g_sprites[SHAPE_MENTAT + 1], 256, y2, WINDOWID_RENDER_TEXTURE, 0);
	GUI_DrawSprite_(0, g_sprites[SHAPE_OPTIONS + 0], 352, y1, WINDOWID_RENDER_TEXTURE, 0);
	GUI_DrawSprite_(0, g_sprites[SHAPE_OPTIONS + 1], 352, y2, WINDOWID_RENDER_TEXTURE, 0);
	GUI_DrawSprite_(0, g_sprites[SHAPE_INVOICE + 0], 448 - 6, y1, WINDOWID_RENDER_TEXTURE, 0);
	GUI_DrawSprite_(0, g_sprites[SHAPE_INVOICE + 1], 448 - 6, y2, WINDOWID_RENDER_TEXTURE, 0);
	GUI_DrawSprite_(0, g_sprites[SHAPE_SEND_ORDER + 0], 544, y1, WINDOWID_RENDER_TEXTURE, 0);
	GUI_DrawSprite_(0, g_sprites[SHAPE_SEND_ORDER + 1], 544, y2, WINDOWID_RENDER_TEXTURE, 0);

	const int WINDOW_W = g_widgetProperties[WINDOWID_RENDER_TEXTURE].width;
	unsigned char *row1 = &buf[WINDOW_W * (y1 + 2)];
	unsigned char *row2 = &buf[WINDOW_W * (y2 + 2)];

	for (int i = 0; i < h; i++) {
		for (unsigned int c = 0; c < lengthof(cmd); c++) {
			memmove(row1 + cmd[c].dx, row1 + cmd[c].sx, cmd[c].w);
			memmove(row2 + cmd[c].dx, row2 + cmd[c].sx, cmd[c].w);
		}

		row1 += WINDOW_W;
		row2 += WINDOW_W;
	}

	s_shape[SHAPE_SEND_ORDER + 0][HOUSE_HARKONNEN] = al_create_sub_bitmap(shape_texture, 4, y1, 120, 16);
	s_shape[SHAPE_SEND_ORDER + 1][HOUSE_HARKONNEN] = al_create_sub_bitmap(shape_texture, 4, y2, 120, 16);
	assert(s_shape[SHAPE_SEND_ORDER + 0][HOUSE_HARKONNEN] != NULL);
	assert(s_shape[SHAPE_SEND_ORDER + 1][HOUSE_HARKONNEN] != NULL);

	s_shape[SHAPE_RESUME_GAME + 0][HOUSE_HARKONNEN] = al_create_sub_bitmap(shape_texture, 128, y1, 120, 16);
	s_shape[SHAPE_RESUME_GAME + 1][HOUSE_HARKONNEN] = al_create_sub_bitmap(shape_texture, 128, y2, 120, 16);
	assert(s_shape[SHAPE_RESUME_GAME + 0][HOUSE_HARKONNEN] != NULL);
	assert(s_shape[SHAPE_RESUME_GAME + 1][HOUSE_HARKONNEN] != NULL);

	for (enum HouseType houseID = HOUSE_HARKONNEN + 1; houseID < HOUSE_MAX; houseID++) {
		s_shape[SHAPE_SEND_ORDER  + 0][houseID] = s_shape[SHAPE_SEND_ORDER  + 0][HOUSE_HARKONNEN];
		s_shape[SHAPE_SEND_ORDER  + 1][houseID] = s_shape[SHAPE_SEND_ORDER  + 1][HOUSE_HARKONNEN];
		s_shape[SHAPE_RESUME_GAME + 0][houseID] = s_shape[SHAPE_RESUME_GAME + 0][HOUSE_HARKONNEN];
		s_shape[SHAPE_RESUME_GAME + 1][houseID] = s_shape[SHAPE_RESUME_GAME + 1][HOUSE_HARKONNEN];
	}
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
		/*355, 372,  true */ /* CHOAM */
		{ 111, 140,  true }, /* UNITS2.SHP */
		{ 141, 150, false }, /* UNITS2.SHP: sonic tank turret, launcher turret */
		{ 151, 161, false }, /* UNITS1.SHP */
		{ 162, 167,  true }, /* UNITS1.SHP: tanks */
		{ 168, 237, false }, /* UNITS1.SHP */
		{ 238, 257,  true }, /* UNITS.SHP: quad .. mcv */
		{ 258, 282, false }, /* UNITS.SHP: rockets */
		{ 283, 300,  true }, /* UNITS.SHP: carryall .. frigate */
		{ 301, 354,  true }, /* UNITS.SHP: saboteur .. landed ornithoper */
		{ 373, 386, false }, /* MENTAT */

		{  -2,   0, false },
		{ 477, 504,  true }, /* PIECES.SHP */
		{ 505, 513, false }, /* ARROWS.SHP */

		{  -1,   0, false }
	};

	const int WINDOW_W = g_widgetProperties[WINDOWID_RENDER_TEXTURE].width;
	const int WINDOW_H = g_widgetProperties[WINDOWID_RENDER_TEXTURE].height;

	int x = 0, y = 0, row_h = 0;
	unsigned char greymap[256];

	uint8 fileID = File_Open("GRAYRMAP.TBL", FILE_MODE_READ);
	assert(fileID != FILE_INVALID);

	File_Read(fileID, greymap, 256);
	File_Close(fileID);

	/* Use colour 12 for black, colour 0 is transparent. */
	for (int i = 0; i < 256; i++) {
		if (greymap[i] == 0)
			greymap[i] = 12;
	}

	al_set_target_bitmap(shape_texture);
	al_clear_to_color(al_map_rgba(0, 0, 0, 0));

	for (int group = 0; shape_data[group].start != -1; group++) {
		if (shape_data[group].start == -2) {
			VideoA5_InitShapeCHOAMButtons(buf, y + row_h + 1);
			VideoA5_CopyBitmap(WINDOW_W, buf, shape_texture, SKIP_COLOUR_0);
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
					if (shapeID == SHAPE_RADIO_BUTTON_OFF || shapeID == SHAPE_RADIO_BUTTON_ON) {
						const uint8 backup = g_remap[RADIO_BUTTON_BACKGROUND_COLOUR];
						g_remap[RADIO_BUTTON_BACKGROUND_COLOUR] = 0;

						s_shape[shapeID][houseID] = VideoA5_ExportShape(shapeID, x, y, row_h, &x, &y, &row_h, g_remap);

						g_remap[RADIO_BUTTON_BACKGROUND_COLOUR] = backup;
					}
					else {
						s_shape[shapeID][houseID] = VideoA5_ExportShape(shapeID, x, y, row_h, &x, &y, &row_h, g_remap);
					}

					if (SHAPE_CONCRETE_SLAB <= shapeID && shapeID <= SHAPE_SANDWORM) {
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

	for (int shapeID = SHAPE_ARROW; shapeID <= SHAPE_ARROW_FINAL; shapeID++) {
		const int tintID = SHAPE_ARROW_TINT + 5 * (shapeID - SHAPE_ARROW);
		const int w = Shape_Width(shapeID);
		const int h = Shape_Height(shapeID);

		VideoA5_GetNextXY(WINDOW_W, WINDOW_H, x, y, 5 * w, h, row_h, &x, &y);
		GUI_DrawSprite_(SCREEN_0, g_sprites[shapeID], x, y, WINDOWID_RENDER_TEXTURE, 0);

		for (int i = 4; i >= 0; i--) {
			const int c = (i == 0) ? STRATEGIC_MAP_ARROW_EDGE_COLOUR : (STRATEGIC_MAP_ARROW_COLOUR + i - 1);
			assert(s_shape[tintID + i][0] == NULL);

			s_shape[tintID + i][0] = al_create_sub_bitmap(region_texture, x + i * w, y, w, h);
			assert(s_shape[tintID + i][0] != NULL);

			VideoA5_CreateWhiteMaskIndexed(buf, WINDOW_W, x, y, x + i * w, y, w, h, c);
		}

		x += 5 * w + 1;
		row_h = max(row_h, h);
	}

	VideoA5_CopyBitmap(WINDOW_W, buf, region_texture, TRANSPARENT_COLOUR_0);

#if OUTPUT_TEXTURES
	al_save_bitmap("shapes.png", shape_texture);
	al_save_bitmap("regions.png", region_texture);
#endif
}

#if 0
/* Requires read/write to texture, separate alpha blending. */
static void
VideoA5_DrawBlur_SeparateBlender(ALLEGRO_BITMAP *brush, int x, int y, int blurx)
{
	ALLEGRO_BITMAP *old_target = al_get_target_bitmap();
	const int w = al_get_bitmap_width(brush);
	const int h = al_get_bitmap_height(brush);

	VideoA5_ResizeScratchBitmap(w, h);
	al_set_target_bitmap(scratch);
	Viewport_RenderBrush(x + blurx, y);

	al_set_target_bitmap(brush);
	al_set_separate_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO, ALLEGRO_DEST_MINUS_SRC, ALLEGRO_ZERO, ALLEGRO_ONE);
	al_draw_bitmap_region(scratch, 0, 0, w, h, 0, 0, 0);

	al_set_target_bitmap(old_target);
	al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);
	al_draw_bitmap(brush, x, y, 0);
}
#endif

#if 0
/* Requires read/write to texture, DEST_MINUS_SRC blender.
 * Requires INVERTED masks (0x00000000 in the centre, 0xFFFFFFFF around).
 */
static void
VideoA5_DrawBlur_DestMinusSrc(ALLEGRO_BITMAP *brush, int x, int y, int blurx)
{
	ALLEGRO_BITMAP *old_target = al_get_target_bitmap();
	const int w = al_get_bitmap_width(brush);
	const int h = al_get_bitmap_height(brush);

	VideoA5_ResizeScratchBitmap(w, h);
	al_set_target_bitmap(scratch);
	Viewport_RenderBrush(x + blurx, y);

	al_set_blender(ALLEGRO_DEST_MINUS_SRC, ALLEGRO_ONE, ALLEGRO_ONE);
	al_draw_bitmap(brush, 0.0f, 0.0f, 0);

	al_set_target_bitmap(old_target);
	al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);
	al_draw_bitmap_region(scratch, 0.0f, 0.0f, w, h, x, y, 0);
}
#endif

#if 1
/* Requires OpenGL, stencil buffer. */
static void
VideoA5_DrawBlur_GLStencil(ALLEGRO_BITMAP *brush, int x, int y, int blurx)
{
	/* Draw brush onto stencil buffer. */
	glClear(GL_STENCIL_BUFFER_BIT);
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_STENCIL_TEST);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
	glAlphaFunc(GL_GREATER, 0.5f);
	glStencilFunc(GL_ALWAYS, 0x1, 0x1);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	al_draw_bitmap(brush, x, y, 0);

	/* Stencil is 1 where we should draw. */
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glAlphaFunc(GL_ALWAYS, 0);
	glStencilFunc(GL_EQUAL, 0x1, 0x1);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	Viewport_RenderBrush(x + blurx, y, blurx);

	glDisable(GL_ALPHA_TEST);
	glDisable(GL_STENCIL_TEST);
}
#endif

#ifdef ALLEGRO_WINDOWS
/* Requires Direct3D, stencil buffer. */
static void
VideoA5_DrawBlur_D3DStencil(ALLEGRO_BITMAP *brush, int x, int y, int blurx)
{
	LPDIRECT3DDEVICE9 pDevice = al_get_d3d_device(display);

	IDirect3DDevice9_Clear(pDevice, 0, NULL, D3DCLEAR_STENCIL, 0, 0, 0);                /* glClear(GL_STENCIL_BUFFER_BIT); */
	IDirect3DDevice9_SetRenderState(pDevice, D3DRS_ALPHATESTENABLE, TRUE);              /* glEnable(GL_ALPHA_TEST); */
	IDirect3DDevice9_SetRenderState(pDevice, D3DRS_STENCILENABLE, TRUE);                /* glEnable(GL_STENCIL_TEST); */ /* ?glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE); */
	IDirect3DDevice9_SetRenderState(pDevice, D3DRS_ALPHAFUNC, D3DCMP_GREATER);          /* glAlphaFunc(GL_GREATER, 0.5f); */
	IDirect3DDevice9_SetRenderState(pDevice, D3DRS_ALPHAREF, (DWORD)128);
	IDirect3DDevice9_SetRenderState(pDevice, D3DRS_STENCILFUNC, D3DCMP_ALWAYS);         /* glStencilFunc(GL_ALWAYS, 0x1, 0x1); */
	IDirect3DDevice9_SetRenderState(pDevice, D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);     /* glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE); */
	IDirect3DDevice9_SetRenderState(pDevice, D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP);
	IDirect3DDevice9_SetRenderState(pDevice, D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE);

	al_draw_bitmap(brush, x, y, 0);

	/* ?glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); */
	IDirect3DDevice9_SetRenderState(pDevice, D3DRS_ALPHAFUNC, D3DCMP_ALWAYS);           /* glAlphaFunc(GL_ALWAYS, 0); */
	IDirect3DDevice9_SetRenderState(pDevice, D3DRS_STENCILFUNC, D3DCMP_EQUAL);          /* glStencilFunc(GL_EQUAL, 0x1, 0x1); */
	IDirect3DDevice9_SetRenderState(pDevice, D3DRS_STENCILREF, 1);
	IDirect3DDevice9_SetRenderState(pDevice, D3DRS_STENCILMASK, 1);
	IDirect3DDevice9_SetRenderState(pDevice, D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);     /* glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP); */

	Viewport_RenderBrush(x + blurx, y, blurx);

	IDirect3DDevice9_SetRenderState(pDevice, D3DRS_ALPHATESTENABLE, FALSE);             /* glDisable(GL_ALPHA_TEST); */
	IDirect3DDevice9_SetRenderState(pDevice, D3DRS_STENCILENABLE, FALSE);               /* glDisable(GL_STENCIL_TEST); */
}
#endif

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
		const int effect = (flags >> 4) & 0x7;

		ALLEGRO_BITMAP *brush = s_shape[shapeID][houseID];

		switch (g_graphics_driver) {
			case GRAPHICS_DRIVER_OPENGL:
				VideoA5_DrawBlur_GLStencil(brush, x, y, s_variable_60[effect]);
				break;

#ifdef ALLEGRO_WINDOWS
			case GRAPHICS_DRIVER_DIRECT3D:
				VideoA5_DrawBlur_D3DStencil(brush, x, y, s_variable_60[effect]);
				break;
#endif

			default:
				/* VideoA5_DrawBlur_SeparateBlender(brush, x, y, s_variable_60[effect]); */
				/* VideoA5_DrawBlur_DestMinusSrc(brush, x, y, s_variable_60[effect]); */
				break;
		}
	}
	else if ((flags & 0x300) == 0x300) {
		/* Shadow. */
		ALLEGRO_COLOR tint = al_map_rgba(0, 0, 0, flags & 0xF0);
		al_draw_tinted_bitmap(s_shape[shapeID][houseID], tint, x, y, al_flags);
	}
	else {
		/* Normal. */
		al_draw_bitmap(s_shape[shapeID][houseID], x, y, al_flags);
	}
}

void
VideoA5_DrawShapeRotate(enum ShapeID shapeID, enum HouseType houseID, int x, int y, int orient256, int flags)
{
	ALLEGRO_BITMAP *bmp = s_shape[shapeID][houseID];
	assert(shapeID < SHAPEID_MAX);
	assert(houseID < HOUSE_MAX);
	assert(bmp != NULL);
	assert((flags & 0x300) != 0x100);
	assert((flags & 0x300) != 0x200);

	const float cx = al_get_bitmap_width(bmp) / 2.0f;
	const float cy = al_get_bitmap_height(bmp) / 2.0f;
	const float angle = 2.0f * ALLEGRO_PI * orient256 / 256.0f;
	const int al_flags = (flags & 0x3);

	if ((flags & 0x300) == 0x300) {
		ALLEGRO_COLOR tint = al_map_rgba(0, 0, 0, flags & 0xF0);
		al_draw_tinted_rotated_bitmap(bmp, tint, cx, cy, x, y, angle, al_flags);
	}
	else {
		al_draw_rotated_bitmap(bmp, cx, cy, x, y, angle, al_flags);
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
	assert(SHAPE_CONCRETE_SLAB <= shapeID && shapeID <= SHAPE_SANDWORM);
	assert(s_shape[greyID][HOUSE_HARKONNEN] != NULL);

	al_draw_bitmap(s_shape[greyID][HOUSE_HARKONNEN], x, y, flags);
}

void
VideoA5_DrawShapeGreyScale(enum ShapeID shapeID, int x, int y, int w, int h, int flags)
{
	const enum ShapeID greyID = SHAPE_CONCRETE_SLAB_GREY + (shapeID - SHAPE_CONCRETE_SLAB);
	assert(SHAPE_CONCRETE_SLAB <= shapeID && shapeID <= SHAPE_SANDWORM);

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
Video_InitFadeInShape(enum ShapeID shapeID, enum HouseType houseID, int x, int y)
{
	assert(shapeID < SHAPEID_MAX);
	assert(houseID < HOUSE_MAX);
	assert(s_shape[shapeID][houseID] != NULL);

	ALLEGRO_BITMAP *src = s_shape[shapeID][houseID];

	return VideoA5_InitFadeInSprite(src, x, y, al_get_bitmap_width(src), al_get_bitmap_height(src), true);
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

#if 0
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
#else
	return 0;
#endif
}

static void
VideoA5_ExportFont(Font *font, const uint8 *pal, int y, int *rety)
{
	const int WINDOW_W = 512;
	const int WINDOW_H = 1024;

	int x = 0;

	Font_Select(font);
	GUI_InitColors(pal, 0, 15);

	for (int c = 0; c < 256; c++) {
		if ((c < font->count) && (font->chars[c].data != NULL)) {
			/* Image width is Font_GetCharWidth(c) + 1. */
			const int w = Font_GetCharWidth(c) + 1;

			VideoA5_GetNextXY(WINDOW_W, WINDOW_H, x, y, w, font->height, font->height, &x, &y);
			GUI_DrawChar_(c, x, y);

			x += w + 1;
		}
	}

	*rety = y + font->height + 1;
}

static void
VideoA5_CreateFontCharacters(Font *font, const uint8 *pal, int y, int *rety)
{
	const int WINDOW_W = 512;
	const int WINDOW_H = 1024;
	const int fnt = VideoA5_FontIndex(font, pal);

	int x = 0;

	Font_Select(font);

	for (int c = 0; c < 256; c++) {
		if ((c < font->count) && (font->chars[c].data != NULL)) {
			/* Image width is Font_GetCharWidth(c) + 1. */
			const int w = Font_GetCharWidth(c) + 1;

			VideoA5_GetNextXY(WINDOW_W, WINDOW_H, x, y, w, font->height, font->height, &x, &y);
			s_font[fnt][c] = al_create_sub_bitmap(interface_texture, x, y, w, font->height);
			assert(s_font[fnt][c] != NULL);

			x += w + 1;
		}
	}

	*rety = y + font->height + 1;
}

static void
VideoA5_InitFonts(unsigned char *buf)
{
	int y = 512;

	if (buf != NULL) {
		const int WINDOW_W = g_widgetProperties[WINDOWID_RENDER_TEXTURE].width;

		/* Phase 1: draw the characters into interface_texture, which
		 * is a memory bitmap.
		 */
		VideoA5_ExportFont(g_fontNew6p, font_palette[0], y, &y);
		VideoA5_ExportFont(g_fontNew6p, font_palette[1], y, &y);
		VideoA5_ExportFont(g_fontNew6p, font_palette[2], y, &y);
		VideoA5_ExportFont(g_fontNew8p, font_palette[0], y, &y);
		VideoA5_ExportFont(g_fontNew8p, font_palette[1], y, &y);
		VideoA5_ExportFont(g_fontNew8p, font_palette[2], y, &y);
		VideoA5_ExportFont(g_fontIntro, font_palette[0], y, &y);
		VideoA5_ExportFont(g_fontIntro, font_palette[3], y, &y);

		VideoA5_CopyBitmap(WINDOW_W, buf, interface_texture, SKIP_COLOUR_0);
	}
	else {
		/* Phase 2: create subbitmaps for each character, after
		 * interface_texture converted into video bitmap.
		 */
		VideoA5_CreateFontCharacters(g_fontNew6p, font_palette[0], y, &y);
		VideoA5_CreateFontCharacters(g_fontNew6p, font_palette[1], y, &y);
		VideoA5_CreateFontCharacters(g_fontNew6p, font_palette[2], y, &y);
		VideoA5_CreateFontCharacters(g_fontNew8p, font_palette[0], y, &y);
		VideoA5_CreateFontCharacters(g_fontNew8p, font_palette[1], y, &y);
		VideoA5_CreateFontCharacters(g_fontNew8p, font_palette[2], y, &y);
		VideoA5_CreateFontCharacters(g_fontIntro, font_palette[0], y, &y);
		VideoA5_CreateFontCharacters(g_fontIntro, font_palette[3], y, &y);
	}
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
	const int WINDOW_W = 512;
	const int WINDOW_H = 512;

	void *wsa = WSA_LoadFile("STATIC.WSA", GFX_Screen_Get_ByIndex(SCREEN_2), GFX_Screen_GetSize_ByIndex(SCREEN_2), true);
	assert(wsa != NULL);

	const int num_frames = WSA_GetFrameCount(wsa);

	int x = 0, y = 0;

	VideoA5_SetBitmapFlags(ALLEGRO_MEMORY_BITMAP);

	ALLEGRO_BITMAP *wsacpy = al_create_bitmap(64, 64);

	VideoA5_SetBitmapFlags(ALLEGRO_VIDEO_BITMAP);

	al_set_target_bitmap(interface_texture);

	for (int frame = 0; frame < num_frames; frame++) {
		VideoA5_GetNextXY(WINDOW_W, WINDOW_H, x, y, 64, 64, 64, &x, &y);
		WSA_DisplayFrame(wsa, frame, 0, 0, SCREEN_0);

		VideoA5_CopyBitmap(SCREEN_WIDTH, buf, wsacpy, BLACK_COLOUR_0);
		al_draw_bitmap(wsacpy, 512 + x, y, 0);

		x += 64 + 1;
	}

	al_destroy_bitmap(wsacpy);
	WSA_Unload(wsa);
}

bool
VideoA5_DrawWSA(void *wsa, int frame, int sx, int sy, int dx, int dy, int w, int h)
{
	VideoA5_ResizeScratchBitmap(w, h);
	if (scratch == NULL)
		return false;

	if (wsa != NULL) {
		if (!WSA_DisplayFrame(wsa, frame, 0, 0, SCREEN_0))
			return false;
	}

	const unsigned char *buf = GFX_Screen_Get_ByIndex(SCREEN_0);

	VideoA5_CopyBitmap(SCREEN_WIDTH, &buf[SCREEN_WIDTH * sy + sx], scratch, BLACK_COLOUR_0);
	al_draw_bitmap(scratch, dx, dy, 0);

	return true;
}

void
VideoA5_DrawWSAStatic(int frame, int x, int y)
{
	const int WINDOW_W = 512;
	const int WINDOW_H = 512;
	const int tx = frame % (WINDOW_W / 65);
	const int ty = frame / (WINDOW_H / 65);
	const int sx = 512 + 65 * tx;
	const int sy = 65 * ty;
	assert(0 <= frame && frame < 21);

	al_draw_bitmap_region(interface_texture, sx, sy, 64, 64, x, y, 0);
}

/*--------------------------------------------------------------*/

/* Mode: 0 = scouted, 1 = terrain only. */
void
Video_DrawMinimap(int left, int top, int map_scale, int mode)
{
	const MapInfo *mapInfo = &g_mapInfos[map_scale];
	bool redraw = false;
	int sandworm_position[4 * 2];
	int num_sandworms = 0;

	for (int y = 0; y < mapInfo->sizeY; y++) {
		uint16 packed = Tile_PackXY(mapInfo->minX, mapInfo->minY + y);
		int i = mapInfo->sizeX * y;

		for (int x = 0; x < mapInfo->sizeX; x++, i++, packed++) {
			const Tile *t = &g_map[packed];
			int colour = 12;

			if (mode == 1) {
				uint16 type = Map_GetLandscapeTypeOriginal(packed);
				colour = g_table_landscapeInfo[type].radarColour;
			}
			else if (t->isUnveiled && g_playerHouse->flags.radarActivated) {
				Unit *u;

				if (enhancement_fog_of_war && g_mapVisible[packed].timeout <= g_timerGame) {
				}
				else if (t->hasUnit && ((u = Unit_Get_ByPackedTile(packed)) != NULL)) {
					if (u->o.type == UNIT_SANDWORM) {
						/* Really shouldn't have more than 3, but anyway. */
						if (num_sandworms < 4) {
							sandworm_position[2*num_sandworms + 0] = x;
							sandworm_position[2*num_sandworms + 1] = y;
							num_sandworms++;
						}
					}
					else {
						colour = g_table_houseInfo[Unit_GetHouseID(u)].minimapColor;
					}
				}

				if (colour == 12) {
					uint16 type = Map_GetLandscapeTypeVisible(packed);

					if (g_table_landscapeInfo[type].radarColour == 0xFFFF) {
						colour = g_table_houseInfo[t->houseID].minimapColor;
					}
					else if (enhancement_fog_of_war && g_mapVisible[packed].timeout <= g_timerGame) {
						colour = -g_table_landscapeInfo[type].radarColour;
					}
					else {
						colour = g_table_landscapeInfo[type].radarColour;
					}
				}
			}
			else if (t->hasStructure && t->houseID == g_playerHouseID) {
				colour = g_table_houseInfo[t->houseID].minimapColor;
			}

			if (s_minimap_colour[i] != colour) {
				s_minimap_colour[i] = colour;
				redraw = true;
			}
		}
	}

	if (redraw) {
		ALLEGRO_LOCKED_REGION *reg = al_lock_bitmap(s_minimap, ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, ALLEGRO_LOCK_WRITEONLY);

		for (int y = 0; y < mapInfo->sizeY; y++) {
			unsigned char *row = &((unsigned char *)reg->data)[reg->pitch*y];

			for (int x = 0; x < mapInfo->sizeX; x++) {
				if (s_minimap_colour[mapInfo->sizeX * y + x] >= 0) {
					const unsigned char c = s_minimap_colour[mapInfo->sizeX * y + x];

					row[reg->pixel_size*x + 0] = paletteRGB[3*c + 0];
					row[reg->pixel_size*x + 1] = paletteRGB[3*c + 1];
					row[reg->pixel_size*x + 2] = paletteRGB[3*c + 2];
					row[reg->pixel_size*x + 3] = 0xFF;
				}
				else {
					const unsigned char c = -s_minimap_colour[mapInfo->sizeX * y + x];

					/* Negative colour denotes darkened for fog of war. */
					row[reg->pixel_size*x + 0] = paletteRGB[3*c + 0] / 2;
					row[reg->pixel_size*x + 1] = paletteRGB[3*c + 1] / 2;
					row[reg->pixel_size*x + 2] = paletteRGB[3*c + 2] / 2;
					row[reg->pixel_size*x + 3] = 0xFF;
				}
			}
		}

		al_unlock_bitmap(s_minimap);
	}

	al_draw_scaled_bitmap(s_minimap, 0.0f, 0.0f, mapInfo->sizeX, mapInfo->sizeY,
			left, top, (map_scale + 1.0f) * mapInfo->sizeX, (map_scale + 1.0f) * mapInfo->sizeY, 0);

	/* Always redraw sandworms because they glow. */
	for (int i = 0; i < num_sandworms; i++) {
		const float x1 = left + (map_scale + 1.0f) * (sandworm_position[2*i + 0] + 0) + 0.01f;
		const float y1 = top  + (map_scale + 1.0f) * (sandworm_position[2*i + 1] + 0) + 0.01f;
		const float x2 = left + (map_scale + 1.0f) * (sandworm_position[2*i + 0] + 1) - 0.01f;
		const float y2 = top  + (map_scale + 1.0f) * (sandworm_position[2*i + 1] + 1) - 0.01f;

		al_draw_filled_rectangle(x1, y1, x2, y2, paltoRGB[0xFF]);
	}
}

/*--------------------------------------------------------------*/

static void
VideoA5_InitCursor(unsigned char *buf)
{
	/* Double-sized mouse cursors on 640x400 and above.
	 * Note that Windows cursors are always 32x32, so we can't go
	 * above 2x scaling.  Also, we can't do the shifting trick/hack.
	 */
	const int scale = (TRUE_DISPLAY_WIDTH >= 640) ? 2 : 1;
	const int sw = 16;
	const int sh = 16;

#ifdef ALLEGRO_WINDOWS
	const int dw = scale * sw;
	const int dh = scale * sh;
#else
	const int dw = scale * (sw + 8);
	const int dh = scale * (sh + 8);
#endif

	VideoA5_SetBitmapFlags(ALLEGRO_MEMORY_BITMAP);

	ALLEGRO_BITMAP *bmp = al_create_bitmap(dw, dh);
	assert(bmp != NULL);

	VideoA5_SetBitmapFlags(ALLEGRO_VIDEO_BITMAP);

	al_set_target_bitmap(bmp);

	for (int i = 0; i < CURSOR_MAX; i++) {
		ALLEGRO_BITMAP *src = s_shape[i][HOUSE_HARKONNEN];

		memset(buf, 0, SCREEN_WIDTH * sh);
		al_clear_to_color(al_map_rgba(0x00, 0x00, 0x00, 0x00));

		GUI_DrawSprite_(SCREEN_0, g_sprites[i], 0, 0, 0, 0);
		VideoA5_CopyBitmap(SCREEN_WIDTH, buf, src, TRANSPARENT_COLOUR_0);

#ifdef ALLEGRO_WINDOWS
		al_draw_scaled_bitmap(src, 0.0f, 0.0f, sw, sh, 0.0f, 0.0f, dw, dh, 0);

		s_cursor[i] = al_create_mouse_cursor(bmp, scale * cursor_focus[i].x, scale * cursor_focus[i].y);
#else
		al_draw_scaled_bitmap(src, 0.0f, 0.0f, sw, sh,
				scale * (8 - cursor_focus[i].x), scale * (8 - cursor_focus[i].y), scale * sw, scale * sh, 0);

		s_cursor[i] = al_create_mouse_cursor(bmp, scale * 8, scale * 8);
#endif
	}

	al_set_mouse_cursor(display, s_cursor[0]);
	al_destroy_bitmap(bmp);
	Mouse_SwitchHWCursor();
}

void
Video_InitMentatSprites(bool use_benepal)
{
	const int WINDOW_W = g_widgetProperties[WINDOWID_RENDER_TEXTURE].width;
	const int TEXTURE_W = 256;
	const int TEXTURE_H = 256;
	const enum WindowID old_widget = Widget_SetCurrentWidget(WINDOWID_RENDER_TEXTURE);
	const uint16 old_screen = GFX_Screen_SetActive(0);
	ALLEGRO_BITMAP *old_target = al_get_target_bitmap();

	unsigned char *buf = GFX_Screen_GetActive();
	memset(buf, 0, WINDOW_W * TEXTURE_H);

	al_destroy_bitmap(mentat_texture);
	mentat_texture = al_create_bitmap(TEXTURE_W, TEXTURE_H);
	assert(mentat_texture != NULL);

	al_set_target_bitmap(mentat_texture);

	if (use_benepal)
		VideoA5_ReadPalette("BENE.PAL");

	GUI_Palette_CreateRemap(HOUSE_HARKONNEN);

	int x = 1;
	int y = 1;
	int row_h = 0;

	for (int i = 0; i < 15; i++) {
		const enum ShapeID shapeID = SHAPE_MENTAT_EYES + i;
		al_destroy_bitmap(s_shape[shapeID][HOUSE_HARKONNEN]);

		s_shape[shapeID][HOUSE_HARKONNEN] = VideoA5_ExportShape(shapeID, x, y, row_h, &x, &y, &row_h, g_remap);
	}

	VideoA5_CopyBitmap(WINDOW_W, buf, mentat_texture, TRANSPARENT_COLOUR_0);
	VideoA5_ReadPalette("IBM.PAL");

#if OUTPUT_TEXTURES
	al_save_bitmap("mentat.png", mentat_texture);
#endif

	GFX_Screen_SetActive(old_screen);
	al_set_target_bitmap(old_target);
	Widget_SetCurrentWidget(old_widget);
}

void
VideoA5_InitSprites(void)
{
	const int WINDOW_W = g_widgetProperties[WINDOWID_RENDER_TEXTURE].width;
	const int WINDOW_H = g_widgetProperties[WINDOWID_RENDER_TEXTURE].height;
	const Screen oldScreenID = GFX_Screen_SetActive(SCREEN_0);
	const enum WindowID old_widget = Widget_SetCurrentWidget(WINDOWID_RENDER_TEXTURE);

	unsigned char *buf = GFX_Screen_GetActive();

	VideoA5_ReadPalette("IBM.PAL");

	memset(buf, 0, WINDOW_W * WINDOW_H);
	VideoA5_InitIcons(buf);

	memset(buf, 0, WINDOW_W * WINDOW_H);
	VideoA5_InitShapes(buf);
	VideoA5_InitCursor(buf);

	memset(buf, 0, WINDOW_W * WINDOW_H);
	VideoA5_InitCPS();
	memset(buf, 0, WINDOW_W * WINDOW_H);
	VideoA5_InitWSA(buf);
	memset(buf, 0, WINDOW_W * WINDOW_H);
	VideoA5_InitFonts(buf);

#if OUTPUT_TEXTURES
	al_save_bitmap("interface.png", interface_texture);
#endif

	const int bitmap_flags = al_get_new_bitmap_flags();
	al_set_new_bitmap_flags(bitmap_flags & ~ALLEGRO_NO_PRESERVE_TEXTURE);
	interface_texture = VideoA5_ConvertToVideoBitmap(interface_texture);
	al_set_new_bitmap_flags(bitmap_flags);
	VideoA5_InitFonts(NULL);

	al_set_target_backbuffer(display);
	al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);

	GFX_Screen_SetActive(oldScreenID);
	Widget_SetCurrentWidget(old_widget);
}

void
VideoA5_DisplayFound(void)
{
	VideoA5_UninitCPSStore();

	al_destroy_bitmap(scratch);
	scratch = NULL;

	memset(s_minimap_colour, 0, sizeof(s_minimap_colour));
}
