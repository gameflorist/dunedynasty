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

/**
 * Get a pixel on the screen.
 * @param x The X-coordinate on the screen.
 * @param y The Y-coordinate on the screen.
 * @return The colour of the pixel.
 */
uint8 GFX_GetPixel(uint16 x, uint16 y)
{
	if (y >= SCREEN_HEIGHT) return 0;
	if (x >= SCREEN_WIDTH) return 0;

	return *((uint8 *)GFX_Screen_GetActive() + y * SCREEN_WIDTH + x);
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

void GUI_DrawProgressbar(uint16 current, uint16 max)
{
	static uint16 l_info[11] = { 293, 52, 24, 7, 1, 0, 0, 0, 4, 5, 8 };

	uint16 width;
	uint16 height;
	uint16 colour;

	l_info[7] = max;
	l_info[6] = current;

	if (current > max) current = max;
	if (max < 1) max = 1;

	width  = l_info[2];
	height = l_info[3];

	/* 0 = Horizontal, 1 = Vertial */
	if (l_info[5] == 0) {
		width = current * width / max;
		if (width < 1) width = 1;
	} else {
		height = current * height / max;
		if (height < 1) height = 1;
	}

	colour = l_info[8];
	if (current <= max / 2) colour = l_info[9];
	if (current <= max / 4) colour = l_info[10];

	if (current != 0 && width  == 0) width = 1;
	if (current != 0 && height == 0) height = 1;

	if (height != 0) {
		GUI_DrawBorder(l_info[0] - 1, l_info[1] - 1, l_info[2] + 2, l_info[3] + 2, 1, true);
	}

	if (width != 0) {
		GUI_DrawFilledRectangle(l_info[0], l_info[1] + l_info[3] - height, l_info[0] + width - 1, l_info[1] + l_info[3] - 1, (uint8)colour);
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
 * Draw a string to the screen using a fixed width for each char.
 *
 * @param string The string to draw.
 * @param left The most left position where to draw the string.
 * @param top The most top position where to draw the string.
 * @param fgColour The foreground colour of the text.
 * @param bgColour The background colour of the text.
 * @param charWidth The width of a char.
 */
void GUI_DrawText_Monospace(const char *string, uint16 left, uint16 top, uint8 fgColour, uint8 bgColour, uint16 charWidth)
{
	char s[2] = " ";

	while (*string != '\0') {
		*s = *string++;
		GUI_DrawText(s, left, top, fgColour, bgColour);
		left += charWidth;
	}
}

/**
 * Fade in parts of the screen from one screenbuffer to the other screenbuffer.
 * @param x The X-position in the source and destination screenbuffers.
 * @param y The Y-position in the source and destination screenbuffers.
 * @param width The width of the screen to copy.
 * @param height The height of the screen to copy.
 * @param screenSrc The ID of the source screen.
 * @param screenDst The ID of the destination screen.
 * @param delay The delay.
 * @param skipNull Wether to copy pixels with colour 0.
 */
void GUI_Screen_FadeIn2(int16 x, int16 y, int16 width, int16 height, uint16 screenSrc, uint16 screenDst, uint16 delay, bool skipNull)
{
	uint16 oldScreenID;
	uint16 i;
	uint16 j;

	uint16 columns[SCREEN_WIDTH];
	uint16 rows[SCREEN_HEIGHT];

	assert(width <= SCREEN_WIDTH);
	assert(height <= SCREEN_HEIGHT);

	if (screenDst == 0) {
		GUI_Mouse_Hide_InRegion(x, y, x + width, y + height);
	}

	for (i = 0; i < width; i++)  columns[i] = i;
	for (i = 0; i < height; i++) rows[i] = i;

	for (i = 0; i < width; i++) {
		uint16 tmp;

		j = Tools_RandomRange(0, width - 1);

		tmp = columns[j];
		columns[j] = columns[i];
		columns[i] = tmp;
	}

	for (i = 0; i < height; i++) {
		uint16 tmp;

		j = Tools_RandomRange(0, height - 1);

		tmp = rows[j];
		rows[j] = rows[i];
		rows[i] = tmp;
	}

	oldScreenID = GFX_Screen_SetActive(screenDst);

	for (j = 0; j < height; j++) {
		uint16 j2 = j;

		for (i = 0; i < width; i++) {
			uint8 colour;
			uint16 curX = x + columns[i];
			uint16 curY = y + rows[j2];

			if (++j2 >= height) j2 = 0;

			GFX_Screen_SetActive(screenSrc);

			colour = GFX_GetPixel(curX, curY);

			GFX_Screen_SetActive(screenDst);

			if (skipNull && colour == 0) continue;

			GFX_PutPixel(curX, curY, colour);
		}

		Video_Tick();
		Timer_Sleep(delay);
	}

	if (screenDst == 0) {
		GUI_Mouse_Show_InRegion();
	}

	GFX_Screen_SetActive(oldScreenID);
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
 * Draws a chess-pattern filled rectangle.
 * @param left The X-position of the rectangle.
 * @param top The Y-position of the rectangle.
 * @param width The width of the rectangle.
 * @param height The height of the rectangle.
 * @param colour The colour of the rectangle.
 */
void GUI_DrawBlockedRectangle(int16 left, int16 top, int16 width, int16 height, uint8 colour)
{
	uint8 *screen;

	if (width <= 0) return;
	if (height <= 0) return;
	if (left >= SCREEN_WIDTH) return;
	if (top >= SCREEN_HEIGHT) return;

	if (left < 0) {
		if (left + width <= 0) return;
		width += left;
		left = 0;
	}
	if (top < 0) {
		if (top + height <= 0) return;
		height += top;
		top = 0;
	}

	if (left + width >= SCREEN_WIDTH) {
		width = SCREEN_WIDTH - left;
	}
	if (top + height >= SCREEN_HEIGHT) {
		height = SCREEN_HEIGHT - top;
	}

	screen = GFX_Screen_GetActive();
	screen += top * SCREEN_WIDTH + left;

	for (; height > 0; height--) {
		int i = width;

		if ((height & 1) != (width & 1)) {
			screen++;
			i--;
		}

		for (; i > 0; i -= 2) {
			*screen = colour;
			screen += 2;
		}

		screen += SCREEN_WIDTH - width - (height & 1);
	}
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

/**
 * Set a new palette, but animate it in slowly.
 * @param palette The new palette.
 * @param ticksOfAnimation The amount of ticks it should take.
 */
void GUI_SetPaletteAnimated(uint8 *palette, int16 ticksOfAnimation)
{
	bool progress;
	int16 diffPerTick;
	int16 tickSlice;
	int16 highestDiff;
	int16 ticks;
	uint16 tickCurrent;
	uint8 data[256 * 3];
	int i;

	if (g_paletteActive == NULL || palette == NULL) return;

	memcpy(data, g_paletteActive, 256 * 3);

	highestDiff = 0;
	for (i = 0; i < 256 * 3; i++) {
		int16 diff = (int16)palette[i] - (int16)data[i];
		highestDiff = max(highestDiff, abs(diff));
	}

	ticks = ticksOfAnimation << 8;
	if (highestDiff != 0) ticks /= highestDiff;

	/* Find a nice value to change every timeslice */
	tickSlice = ticks;
	diffPerTick = 1;
	while (diffPerTick <= highestDiff && ticks < (2 << 8)) {
		ticks += tickSlice;
		diffPerTick++;
	}

	tickCurrent = 0;

	do {
		progress = false;

		tickCurrent  += (uint16)ticks;
		const int delay = tickCurrent >> 8;
		tickCurrent  &= 0xFF;

		for (i = 0; i < 256 * 3; i++) {
			int16 current = palette[i];
			int16 goal = data[i];

			if (goal == current) continue;

			if (goal < current) {
				goal = min(goal + diffPerTick, current);
				progress = true;
			}

			if (goal > current) {
				goal = max(goal - diffPerTick, current);
				progress = true;
			}

			data[i] = goal & 0xFF;
		}

		if (progress) {
			GFX_SetPalette(data);

			Video_Tick();
			Timer_Sleep(delay);
		}
	} while (progress);
}

/* gui/viewport.c */

/**
 * Get a sprite for the viewport, recolouring it when needed.
 *
 * @param spriteID The sprite to get.
 * @param houseID The House to recolour it with.
 * @return The sprite if found, otherwise NULL.
 */
static uint8 *GUI_Widget_Viewport_Draw_GetSprite(uint16 spriteID, uint8 houseID)
{
	uint8 *sprite;
	uint8 i;

	if (spriteID > 355) return NULL;

	sprite = g_sprites[spriteID];

	if (sprite == NULL) return NULL;

	if ((Sprites_GetType(sprite) & 0x1) == 0) return sprite;

	for (i = 0; i < 16; i++) {
		uint8 v = sprite[10 + i];

		if (v >= 0x90 && v <= 0x98) {
			if (v == 0xFF) break;
			v += houseID * 16;
		}

		s_paletteHouse[i] = v;
	}

	return sprite;
}

/* gui/widget.c */

/**
 * Draw a chess-pattern filled rectangle over the widget.
 *
 * @param w The widget to draw.
 * @param colour The colour of the chess pattern.
 */
static void GUI_Widget_DrawBlocked(Widget *w, uint8 colour)
{
	if (g_screenActiveID == 0) {
		GUI_Mouse_Hide_InRegion(w->offsetX, w->offsetY, w->offsetX + w->width, w->offsetY + w->height);
	}

	GUI_DrawSprite(g_screenActiveID, w->drawParameterNormal.sprite, w->offsetX, w->offsetY, w->parentID, 0);

	GUI_DrawBlockedRectangle(w->offsetX, w->offsetY, w->width, w->height, colour);

	if (g_screenActiveID == 0) {
		GUI_Mouse_Show_InRegion();
	}
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
