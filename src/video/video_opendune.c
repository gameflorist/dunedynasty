/* video_opendune.c */

/* gfx.c */

/**
 * Put a pixel on the screen.
 * @param x The X-coordinate on the screen.
 * @param y The Y-coordinate on the screen.
 * @param colour The colour of the pixel to put on the screen.
 */
void GFX_PutPixel(uint16 x, uint16 y, uint8 colour)
{
	if (y >= SCREEN_HEIGHT) return;
	if (x >= SCREEN_WIDTH) return;

	*((uint8 *)GFX_Screen_GetActive() + y * SCREEN_WIDTH + x) = colour;
}

uint16 GFX_GetSize(int16 width, int16 height)
{
	if (width < 1) width = 1;
	if (width > SCREEN_WIDTH) width = SCREEN_WIDTH;
	if (height < 1) height = 1;
	if (height > SCREEN_HEIGHT) height = SCREEN_HEIGHT;

	return width * height;
}

/* gui/gui.c */

MSVC_PACKED_BEGIN
typedef struct ClippingArea {
	/* 0000(2)   */ PACK uint16 left;                       /*!< ?? */
	/* 0002(2)   */ PACK uint16 top;                        /*!< ?? */
	/* 0004(2)   */ PACK uint16 right;                      /*!< ?? */
	/* 0006(2)   */ PACK uint16 bottom;                     /*!< ?? */
} GCC_PACKED ClippingArea;
MSVC_PACKED_END
assert_compile(sizeof(ClippingArea) == 0x08);

static ClippingArea g_clipping = { 0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1 };

static uint16 s_mouseSpriteLeft;
static uint16 s_mouseSpriteTop;
static uint16 s_mouseSpriteWidth;
static uint16 s_mouseSpriteHeight;

static uint16 g_mouseSpriteHotspotX;
static uint16 g_mouseSpriteHotspotY;
static uint16 g_mouseWidth;
static uint16 g_mouseHeight;

/**
 * Draw a wired rectangle.
 * @param left The left position of the rectangle.
 * @param top The top position of the rectangle.
 * @param right The right position of the rectangle.
 * @param bottom The bottom position of the rectangle.
 * @param colour The colour of the rectangle.
 */
void GUI_DrawWiredRectangle(uint16 left, uint16 top, uint16 right, uint16 bottom, uint8 colour)
{
	GUI_DrawLine(left, top, right, top, colour);
	GUI_DrawLine(left, bottom, right, bottom, colour);
	GUI_DrawLine(left, top, left, bottom, colour);
	GUI_DrawLine(right, top, right, bottom, colour);
}

/**
 * Draw a filled rectangle.
 * @param left The left position of the rectangle.
 * @param top The top position of the rectangle.
 * @param right The right position of the rectangle.
 * @param bottom The bottom position of the rectangle.
 * @param colour The colour of the rectangle.
 */
void GUI_DrawFilledRectangle(int16 left, int16 top, int16 right, int16 bottom, uint8 colour)
{
	uint16 x;
	uint16 y;
	uint16 height;
	uint16 width;

	uint8 *screen = GFX_Screen_GetActive();

	if (left >= SCREEN_WIDTH) return;
	if (left < 0) left = 0;

	if (top >= SCREEN_HEIGHT) return;
	if (top < 0) top = 0;

	if (right >= SCREEN_WIDTH) right = SCREEN_WIDTH - 1;
	if (right < 0) right = 0;

	if (bottom >= SCREEN_HEIGHT) bottom = SCREEN_HEIGHT - 1;
	if (bottom < 0) bottom = 0;

	if (left > right) return;
	if (top > bottom) return;

	screen += left + top * SCREEN_WIDTH;
	width = right - left + 1;
	height = bottom - top + 1;
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			*screen++ = colour;
		}
		screen += SCREEN_WIDTH - width;
	}
}

/**
 * Get how the given point must be clipped.
 * @param x The X-coordinate of the point.
 * @param y The Y-coordinate of the point.
 * @return A bitset.
 */
static uint16 GetNeededClipping(int16 x, int16 y)
{
	uint16 flags = 0;

	if (y < g_clipping.top)    flags |= 0x1;
	if (y > g_clipping.bottom) flags |= 0x2;
	if (x < g_clipping.left)   flags |= 0x4;
	if (x > g_clipping.right)  flags |= 0x8;

	return flags;
}

/**
 * Applies top clipping to a line.
 * @param x1 Pointer to the X-coordinate of the begin of the line.
 * @param y1 Pointer to the Y-coordinate of the begin of the line.
 * @param x2 The X-coordinate of the end of the line.
 * @param y2 The Y-coordinate of the end of the line.
 */
static void ClipTop(int16 *x1, int16 *y1, int16 x2, int16 y2)
{
	*x1 += (x2 - *x1) * (g_clipping.top - *y1) / (y2 - *y1);
	*y1 = g_clipping.top;
}

/**
 * Applies bottom clipping to a line.
 * @param x1 Pointer to the X-coordinate of the begin of the line.
 * @param y1 Pointer to the Y-coordinate of the begin of the line.
 * @param x2 The X-coordinate of the end of the line.
 * @param y2 The Y-coordinate of the end of the line.
 */
static void ClipBottom(int16 *x1, int16 *y1, int16 x2, int16 y2)
{
	*x1 += (x2 - *x1) * (*y1 - g_clipping.bottom) / (*y1 - y2);
	*y1 = g_clipping.bottom;
}

/**
 * Applies left clipping to a line.
 * @param x1 Pointer to the X-coordinate of the begin of the line.
 * @param y1 Pointer to the Y-coordinate of the begin of the line.
 * @param x2 The X-coordinate of the end of the line.
 * @param y2 The Y-coordinate of the end of the line.
 */
static void ClipLeft(int16 *x1, int16 *y1, int16 x2, int16 y2)
{
	*y1 += (y2 - *y1) * (g_clipping.left - *x1) / (x2 - *x1);
	*x1 = g_clipping.left;
}

/**
 * Applies right clipping to a line.
 * @param x1 Pointer to the X-coordinate of the begin of the line.
 * @param y1 Pointer to the Y-coordinate of the begin of the line.
 * @param x2 The X-coordinate of the end of the line.
 * @param y2 The Y-coordinate of the end of the line.
 */
static void ClipRight(int16 *x1, int16 *y1, int16 x2, int16 y2)
{
	*y1 += (y2 - *y1) * (*x1 - g_clipping.right) / (*x1 - x2);
	*x1 = g_clipping.right;
}

/**
 * Draws a line from (x1, y1) to (x2, y2) using given colour.
 * @param x1 The X-coordinate of the begin of the line.
 * @param y1 The Y-coordinate of the begin of the line.
 * @param x2 The X-coordinate of the end of the line.
 * @param y2 The Y-coordinate of the end of the line.
 * @param colour The colour to use to draw the line.
 */
void GUI_DrawLine(int16 x1, int16 y1, int16 x2, int16 y2, uint8 colour)
{
	uint8 *screen = GFX_Screen_GetActive();
	int16 increment = 1;

	if (x1 < g_clipping.left || x1 > g_clipping.right || y1 < g_clipping.top || y1 > g_clipping.bottom || x2 < g_clipping.left || x2 > g_clipping.right || y2 < g_clipping.top || y2 > g_clipping.bottom) {
		while (true) {
			uint16 clip1 = GetNeededClipping(x1, y1);
			uint16 clip2 = GetNeededClipping(x2, y2);

			if (clip1 == 0 && clip2 == 0) break;
			if ((clip1 & clip2) != 0) return;

			switch (clip1) {
				case 1: case 9:  ClipTop(&x1, &y1, x2, y2); break;
				case 2: case 6:  ClipBottom(&x1, &y1, x2, y2); break;
				case 4: case 5:  ClipLeft(&x1, &y1, x2, y2); break;
				case 8: case 10: ClipRight(&x1, &y1, x2, y2); break;
				default:
					switch (clip2) {
						case 1: case 9:  ClipTop(&x2, &y2, x1, y1); break;
						case 2: case 6:  ClipBottom(&x2, &y2, x1, y1); break;
						case 4: case 5:  ClipLeft(&x2, &y2, x1, y1); break;
						case 8: case 10: ClipRight(&x2, &y2, x1, y1); break;
						default: break;
					}
			}
		}
	}

	y2 -= y1;

	if (y2 == 0) {
		if (x1 >= x2) {
			int16 x = x1;
			x1 = x2;
			x2 = x;
		}

		x2 -= x1 - 1;

		screen += y1 * SCREEN_WIDTH + x1;

		memset(screen, colour, x2);
		return;
	}

	if (y2 < 0) {
		int16 x = x1;
		x1 = x2;
		x2 = x;
		y2 = -y2;
		y1 -= y2;
	}

	screen += y1 * SCREEN_WIDTH;

	x2 -= x1;
	if (x2 == 0) {
		screen += x1;

		while (y2-- != 0) {
			*screen = colour;
			screen += SCREEN_WIDTH;
		}

		return;
	}

	if (x2 < 0) {
		x2 = -x2;
		increment = -1;
	}

	if (x2 < y2) {
		int16 full = y2;
		int16 half = y2 / 2;
		screen += x1;
		while (true) {
			*screen = colour;
			if (y2-- == 0) return;
			screen += SCREEN_WIDTH;
			half -= x2;
			if (half < 0) {
				half += full;
				screen += increment;
			}
		}
	} else {
		int16 full = x2;
		int16 half = x2 / 2;
		screen += x1;
		while (true) {
			*screen = colour;
			if (x2-- == 0) return;
			screen += increment;
			half -= y2;
			if (half < 0) {
				half += full;
				screen += SCREEN_WIDTH;
			}
		}
	}
}

/**
 * Sets the clipping area.
 * @param left The left clipping.
 * @param top The top clipping.
 * @param right The right clipping.
 * @param bottom The bottom clipping.
 */
void GUI_SetClippingArea(uint16 left, uint16 top, uint16 right, uint16 bottom)
{
	g_clipping.left   = left;
	g_clipping.top    = top;
	g_clipping.right  = right;
	g_clipping.bottom = bottom;
}

/**
 * Show the mouse on the screen. Copy the screen behind the mouse in a safe
 *  buffer.
 */
void GUI_Mouse_Show(void)
{
	int left, top;

	if (g_var_7097 == 1) return;
	if (g_mouseHiddenDepth == 0 || --g_mouseHiddenDepth != 0) return;

	left = g_mouseX - g_mouseSpriteHotspotX;
	top  = g_mouseY - g_mouseSpriteHotspotY;

	s_mouseSpriteLeft = (left < 0) ? 0 : (left >> 3);
	s_mouseSpriteTop = (top < 0) ? 0 : top;

	s_mouseSpriteWidth = g_mouseWidth;
	if ((left >> 3) + g_mouseWidth >= SCREEN_WIDTH / 8) s_mouseSpriteWidth -= (left >> 3) + g_mouseWidth - SCREEN_WIDTH / 8;

	s_mouseSpriteHeight = g_mouseHeight;
	if (top + g_mouseHeight >= SCREEN_HEIGHT) s_mouseSpriteHeight -= top + g_mouseHeight - SCREEN_HEIGHT;

	if (g_mouseSpriteBuffer != NULL) {
		GFX_CopyToBuffer(s_mouseSpriteLeft * 8, s_mouseSpriteTop, s_mouseSpriteWidth * 8, s_mouseSpriteHeight, g_mouseSpriteBuffer);
	}

	GUI_DrawSprite(0, g_mouseSprite, left, top, 0, 0);
}

/**
 * Hide the mouse from the screen. Do this by copying the mouse buffer back to
 *  the screen.
 */
void GUI_Mouse_Hide(void)
{
	if (g_var_7097 == 1) return;

	if (g_mouseHiddenDepth == 0 && s_mouseSpriteWidth != 0) {
		if (g_mouseSpriteBuffer != NULL) {
			GFX_CopyFromBuffer(s_mouseSpriteLeft * 8, s_mouseSpriteTop, s_mouseSpriteWidth * 8, s_mouseSpriteHeight, g_mouseSpriteBuffer);
		}

		s_mouseSpriteWidth = 0;
	}

	g_mouseHiddenDepth++;
}

/**
 * The safe version of GUI_Mouse_Hide(). It waits for a mouselock before doing
 *  anything.
 */
void GUI_Mouse_Hide_Safe(void)
{
	while (g_mouseLock != 0) msleep(0);
	g_mouseLock++;

	if (g_var_7097 == 1) {
		g_mouseLock--;
		return;
	}

	GUI_Mouse_Hide();

	g_mouseLock--;
}

/**
 * The safe version of GUI_Mouse_Show(). It waits for a mouselock before doing
 *  anything.
 */
void GUI_Mouse_Show_Safe(void)
{
	while (g_mouseLock != 0) msleep(0);
	g_mouseLock++;

	if (g_var_7097 == 1) {
		g_mouseLock--;
		return;
	}

	GUI_Mouse_Show();

	g_mouseLock--;
}

/**
 * Show the mouse if needed. Should be used in combination with
 *  #GUI_Mouse_Hide_InRegion().
 */
void GUI_Mouse_Show_InRegion(void)
{
	uint8 counter;

	while (g_mouseLock != 0) msleep(0);
	g_mouseLock++;

	counter = g_regionFlags & 0xFF;
	if (counter == 0 || --counter != 0) {
		g_regionFlags = (g_regionFlags & 0xFF00) | (counter & 0xFF);
		g_mouseLock--;
		return;
	}

	if ((g_regionFlags & 0x4000) != 0) {
		GUI_Mouse_Show();
	}

	g_regionFlags = 0;
	g_mouseLock--;
}

/**
 * Hide the mouse when it is inside the specified region. Works with
 *  #GUI_Mouse_Show_InRegion(), which only calls #GUI_Mouse_Show() when
 *  mouse was really hidden.
 */
void GUI_Mouse_Hide_InRegion(uint16 left, uint16 top, uint16 right, uint16 bottom)
{
	int minx, miny;
	int maxx, maxy;

	minx = left - ((g_mouseWidth - 1) << 3) + g_mouseSpriteHotspotX;
	if (minx < 0) minx = 0;

	miny = top - g_mouseHeight + g_mouseSpriteHotspotY;
	if (miny < 0) miny = 0;

	maxx = right + g_mouseSpriteHotspotX;
	if (maxx > SCREEN_WIDTH - 1) maxx = SCREEN_WIDTH - 1;

	maxy = bottom + g_mouseSpriteHotspotY;
	if (maxy > SCREEN_HEIGHT - 1) maxy = SCREEN_HEIGHT - 1;

	while (g_mouseLock != 0) msleep(0);
	g_mouseLock++;

	if (g_regionFlags == 0) {
		g_regionMinX = minx;
		g_regionMinY = miny;
		g_regionMaxX = maxx;
		g_regionMaxY = maxy;
	}

	if (minx > g_regionMinX) g_regionMinX = minx;
	if (miny > g_regionMinY) g_regionMinY = miny;
	if (maxx < g_regionMaxX) g_regionMaxX = maxx;
	if (maxy < g_regionMaxY) g_regionMaxY = maxy;

	if ((g_regionFlags & 0x4000) == 0 &&
	     g_mouseX >= g_regionMinX &&
	     g_mouseX <= g_regionMaxX &&
	     g_mouseY >= g_regionMinY &&
	     g_mouseY <= g_regionMaxY) {
		GUI_Mouse_Hide();

		g_regionFlags |= 0x4000;
	}

	g_regionFlags |= 0x8000;
	g_regionFlags = (g_regionFlags & 0xFF00) | (((g_regionFlags & 0x00FF) + 1) & 0xFF);

	g_mouseLock--;
}

/**
 * Show the mouse if needed. Should be used in combination with
 *  GUI_Mouse_Hide_InWidget().
 */
void GUI_Mouse_Show_InWidget(void)
{
	GUI_Mouse_Show_InRegion();
}

/**
 * Hide the mouse when it is inside the specified widget. Works with
 *  #GUI_Mouse_Show_InWidget(), which only calls #GUI_Mouse_Show() when
 *  mouse was really hidden.
 * @param widgetIndex The index of the widget to check on.
 */
void GUI_Mouse_Hide_InWidget(uint16 widgetIndex)
{
	uint16 left, top;
	uint16 width, height;

	left   = g_widgetProperties[widgetIndex].xBase << 3;
	top    = g_widgetProperties[widgetIndex].yBase;
	width  = g_widgetProperties[widgetIndex].width << 3;
	height = g_widgetProperties[widgetIndex].height;

	GUI_Mouse_Hide_InRegion(left, top, left + width - 1, top + height - 1);
}

/**
 * Set the mouse to the given position on the screen.
 *
 * @param x The new X-position of the mouse.
 * @param y The new Y-position of the mouse.
 */
void GUI_Mouse_SetPosition(uint16 x, uint16 y)
{
	while (g_mouseLock != 0) msleep(0);
	g_mouseLock++;

	if (x < g_mouseRegionLeft)   x = g_mouseRegionLeft;
	if (x > g_mouseRegionRight)  x = g_mouseRegionRight;
	if (y < g_mouseRegionTop)    y = g_mouseRegionTop;
	if (y > g_mouseRegionBottom) y = g_mouseRegionBottom;

	g_mouseX = x;
	g_mouseY = y;

	Video_Mouse_SetPosition(x, y);

	if (g_mouseX != g_mousePrevX || g_mouseY != g_mousePrevY) {
		GUI_Mouse_Hide();
		GUI_Mouse_Show();
	}

	g_mouseLock--;
}

/* sprites.c */

void Sprites_SetMouseSprite(uint16 hotSpotX, uint16 hotSpotY, uint8 *sprite)
{
	uint16 size;

	if (sprite == NULL || g_var_7097 != 0) return;

	while (g_mouseLock != 0) msleep(0);

	g_mouseLock++;

	GUI_Mouse_Hide();

	size = GFX_GetSize(*(uint16 *)(sprite + 3) + 16, sprite[5]);

	if (s_mouseSpriteBufferSize < size) {
		g_mouseSpriteBuffer = realloc(g_mouseSpriteBuffer, size);
		s_mouseSpriteBufferSize = size;
	}

	size = *(uint16 *)(sprite + 8) + 10;
	if ((*(uint16 *)sprite & 0x1) != 0) size += 16;

	if (s_mouseSpriteSize < size) {
		g_mouseSprite = realloc(g_mouseSprite, size);
		s_mouseSpriteSize = size;
	}

	if ((*(uint16 *)sprite & 0x2) != 0) {
		memcpy(g_mouseSprite, sprite, *(uint16 *)(sprite + 6) * 2);
	} else {
		uint8 *dst = (uint8 *)g_mouseSprite;
		uint8 *buf = g_spriteBuffer;
		uint16 flags = *(uint16 *)sprite | 0x2;

		*(uint16 *)dst = flags;
		dst += 2;
		sprite += 2;

		memcpy(dst, sprite, 6);
		dst += 6;
		sprite += 6;

		size = *(uint16 *)sprite;
		*(uint16 *)dst = size;
		dst += 2;
		sprite += 2;

		if ((flags & 0x1) != 0) {
			memcpy(dst, sprite, 16);
			dst += 16;
			sprite += 16;
		}

		Format80_Decode(buf, sprite, size);

		memcpy(dst, buf, size);
	}

	g_mouseSpriteHotspotX = hotSpotX;
	g_mouseSpriteHotspotY = hotSpotY;

	sprite = g_mouseSprite;
	g_mouseHeight = sprite[5];
	g_mouseWidth = (*(uint16 *)(sprite + 3) >> 3) + 2;

	GUI_Mouse_Show();

	g_mouseLock--;
}
