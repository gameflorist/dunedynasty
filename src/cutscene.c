/** @file src/cutscene.c Introduction movie and cutscenes routines. */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "enum_string.h"
#include "types.h"
#include "os/common.h"
#include "os/error.h"
#include "os/math.h"
#include "os/sleep.h"

#include "cutscene.h"

#include "audio/audio.h"
#include "config.h"
#include "enhancement.h"
#include "file.h"
#include "gfx.h"
#include "gui/font.h"
#include "gui/gui.h"
#include "input/input.h"
#include "opendune.h"
#include "sprites.h"
#include "string.h"
#include "table/houseanimation.h"
#include "table/sound.h"
#include "timer/timer.h"
#include "tools/random_lcg.h"
#include "video/video.h"
#include "wsa.h"

static const HouseAnimation_Animation   *s_houseAnimation_animation = NULL;   /*!< Animation part of animation data. */
static const HouseAnimation_Subtitle    *s_houseAnimation_subtitle = NULL;    /*!< Subtitle part of animation data. */
static const HouseAnimation_SoundEffect *s_houseAnimation_soundEffect = NULL; /*!< Soundeffect part of animation data. */
static uint16 s_var_8062 = 0xFFFF; /*!< Unknown animation data. */
static uint16 s_var_8068 = 0xFFFF; /*!< Unknown animation data. */
static uint16 s_subtitleWait = 0xFFFF; /*!< Unknown animation data. */
static uint16 s_houseAnimation_currentSubtitle = 0; /*!< Current subtitle (index) part of animation. */
static uint16 s_houseAnimation_currentSoundEffect = 0; /* Current voice (index) part of animation. */
static bool s_subtitleActive = false; /* Unknown animation data. */

/** Direction of change in the #GameLoop_PalettePart_Update function. */
enum PalettePartDirection {
	PPD_STOPPED,        /*!< Not changing. */
	PPD_TO_NEW_PALETTE, /*!< Modifying towards #s_palettePartTarget */
	PPD_TO_BLACK        /*!< Modifying towards all black. */
};

static enum PalettePartDirection s_palettePartDirection;    /*!< Direction of change. @see PalettePartDirection */
static int64_t              s_paletteAnimationTimeout; /*!< Timeout value for the next palette change. */
static uint16               s_palettePartCount;        /*!< Number of steps left before the target palette is reached. */
static uint8                s_palettePartTarget[18];   /*!< Target palette part (6 colours). */
static uint8                s_palettePartCurrent[18];  /*!< Current value of the palette part (6 colours, updated each call to #GameLoop_PalettePart_Update). */
static uint8                s_palettePartChange[18];   /*!< Amount of change of each RGB colour of the palette part with each step. */

static void *s_buffer_182E = NULL;
static void *s_buffer_1832 = NULL;
static uint8 s_palette10[3 * 256 * 10];

/*--------------------------------------------------------------*/

static bool
Cutscene_InputSkipScene(void)
{
	Input_Tick(true);

	while (Input_IsInputAvailable()) {
		const int key = Input_PeekNextKey();

		if (key == SCANCODE_ESCAPE ||
			key == SCANCODE_SPACE ||
			key == MOUSE_RMB)
			return true;

		Input_GetNextKey();
	}

	return false;
}

static void
Cutscene_CopyScreen(void)
{
	Video_DrawWSA(NULL, 0, 0, 0, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	Video_Tick();
}

static uint8
Cutscene_GetPixel(uint16 x, uint16 y)
{
	if (y >= SCREEN_HEIGHT) return 0;
	if (x >= SCREEN_WIDTH) return 0;

	return *((uint8 *)GFX_Screen_GetActive() + y * SCREEN_WIDTH + x);
}

static void
Cutscene_PutPixel(uint16 x, uint16 y, uint8 colour)
{
	if (y >= SCREEN_HEIGHT) return;
	if (x >= SCREEN_WIDTH) return;

	*((uint8 *)GFX_Screen_GetActive() + y * SCREEN_WIDTH + x) = colour;
}

static void
Cutscene_DrawText(char *str, int left, int top, uint8 fg, uint8 bg)
{
	uint8 colours[2] = { bg, fg };
	char *s = str;
	int x = left;

	GUI_InitColors(colours, 0, 1);

	while (*s != '\0') {
		GUI_DrawChar_(*s, x, top);
		x += Font_GetCharWidth(*s);
		s++;
	}
}

static void
Cutscene_DrawText_Wrapper(const char *str, int x, int y)
{
	const char *s = str;

	x -= Font_GetStringWidth(str) / 2;

	while (*s != '\0') {
		GUI_DrawChar_(*s, x, y);
		x += Font_GetCharWidth(*s);
		s++;
	}
}

static void
Cutscene_Screen_FadeIn(uint16 xSrc, uint16 ySrc, uint16 xDst, uint16 yDst, uint16 width, uint16 height, Screen screenSrc, Screen screenDst)
{
	uint16 offsetsY[100];
	uint16 offsetsX[40];
	int x, y;

	height /= 2;

	for (x = 0; x < width;  x++) offsetsX[x] = x;
	for (y = 0; y < height; y++) offsetsY[y] = y;

	for (x = 0; x < width; x++) {
		uint16 index;
		uint16 temp;

		index = Tools_RandomLCG_Range(0, width - 1);

		temp = offsetsX[index];
		offsetsX[index] = offsetsX[x];
		offsetsX[x] = temp;
	}

	for (y = 0; y < height; y++) {
		uint16 index;
		uint16 temp;

		index = Tools_RandomLCG_Range(0, height - 1);

		temp = offsetsY[index];
		offsetsY[index] = offsetsY[y];
		offsetsY[y] = temp;
	}

	for (y = 0; y < height; y++) {
		uint16 y2 = y;
		for (x = 0; x < width; x++) {
			uint16 offsetX, offsetY;

			offsetX = offsetsX[x];
			offsetY = offsetsY[y2];

			GUI_Screen_Copy(xSrc + offsetX, ySrc + offsetY * 2, xDst + offsetX, yDst + offsetY * 2, 1, 2, screenSrc, screenDst);

			y2++;
			if (y2 == height) y2 = 0;
		}

		/* XXX -- This delays the system so you can in fact see the animation */
		if ((y % 4) == 0) {
			Cutscene_CopyScreen();
			Timer_Sleep(1);
		}
	}
}

static void
Cutscene_Screen_FadeIn2(int16 x, int16 y, int16 width, int16 height, Screen screenSrc, Screen screenDst, uint16 delay, bool skipNull)
{
	Screen oldScreenID;
	uint16 i;
	uint16 j;

	uint16 columns[SCREEN_WIDTH];
	uint16 rows[SCREEN_HEIGHT];

	assert(width <= SCREEN_WIDTH);
	assert(height <= SCREEN_HEIGHT);

	for (i = 0; i < width; i++)  columns[i] = i;
	for (i = 0; i < height; i++) rows[i] = i;

	for (i = 0; i < width; i++) {
		uint16 tmp;

		j = Tools_RandomLCG_Range(0, width - 1);

		tmp = columns[j];
		columns[j] = columns[i];
		columns[i] = tmp;
	}

	for (i = 0; i < height; i++) {
		uint16 tmp;

		j = Tools_RandomLCG_Range(0, height - 1);

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

			colour = Cutscene_GetPixel(curX, curY);

			GFX_Screen_SetActive(screenDst);

			if (skipNull && colour == 0) continue;

			Cutscene_PutPixel(curX, curY, colour);
		}

		Cutscene_CopyScreen();
		Timer_Sleep(delay);
	}

	GFX_Screen_SetActive(oldScreenID);
}

static void
Cutscene_SetPaletteAnimated(uint8 *palette, int16 ticksOfAnimation)
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

			Cutscene_CopyScreen();
			Timer_Sleep(delay);
		}
	} while (progress);
}

/*--------------------------------------------------------------*/

static void GameLoop_PrepareAnimation(const HouseAnimation_Animation *animation, const HouseAnimation_Subtitle *subtitle, uint16 arg_8062, const HouseAnimation_SoundEffect *soundEffect)
{
	uint8 i;
	uint8 colors[16];

	s_houseAnimation_animation   = animation;
	s_houseAnimation_subtitle    = subtitle;
	s_houseAnimation_soundEffect = soundEffect;

	s_houseAnimation_currentSubtitle    = 0;
	s_houseAnimation_currentSoundEffect = 0;

	g_fontCharOffset = 0;

	s_var_8062 = arg_8062;
	s_var_8068 = 0;
	s_subtitleWait = 0xFFFF;
	s_subtitleActive = false;

	s_palettePartDirection    = PPD_STOPPED;
	s_palettePartCount        = 0;
	s_paletteAnimationTimeout = 0;

	GFX_ClearScreen();

	File_ReadBlockFile("INTRO.PAL", g_palette1, 256 * 3);

	memcpy(g_palette_998A, g_palette1, 256 * 3);

	Font_Select(g_fontIntro);

	GFX_Screen_SetActive(SCREEN_0);

	memcpy(s_palettePartTarget, &g_palette1[(144 + s_houseAnimation_subtitle->colour * 16) * 3], 6 * 3);

	memset(&g_palette1[215 * 3], 0, 6 * 3);

	memcpy(s_palettePartCurrent, s_palettePartTarget, 6 * 3);

	memset(s_palettePartChange, 0, 6 * 3);


	colors[0] = 0;
	for (i = 0; i < 6; i++) colors[i + 1] = 215 + i;

	GUI_InitColors(colors, 0, 15);
}

static void GameLoop_FinishAnimation(void)
{
	GUI_DrawText_Wrapper(NULL, 0, 0, 0, 0, 0x1);
	GUI_DrawText_Wrapper(NULL, 0, 0, 0, 0, 0x2);

	Cutscene_SetPaletteAnimated(g_palette2, 60);

	GUI_ClearScreen(SCREEN_0);

	Input_History_Clear();

	GFX_ClearBlock(SCREEN_3);

	File_ReadBlockFile("IBM.PAL", g_palette1, 3 * 256);
	memcpy(g_palette_998A, g_palette1, 3 * 256);
	Video_SetPalette(g_palette1, 0, 256);
}

static void GameLoop_PlaySoundEffect(uint8 animation)
{
	const HouseAnimation_SoundEffect *soundEffect = &s_houseAnimation_soundEffect[s_houseAnimation_currentSoundEffect];

	if (soundEffect->animationID > animation || soundEffect->wait > s_var_8068) return;

	Audio_PlaySoundCutscene(soundEffect->voiceID);

	s_houseAnimation_currentSoundEffect++;
}

static void GameLoop_DrawText(char *string, uint16 top)
{
	char *s;
	uint8 *s2;

	s = string;
	for (s2 = (uint8 *)string; *s2 != 0; s2++) *s++ = (*s2 == 0xE1) ? 1 : *s2;
	*s = 0;

	s = string;

	while (*s != 0 && *s != '\r') s++;

	if (*s != 0) {
		*s++ = 0;
	} else {
		s = NULL;
	}

	/* GUI_DrawText_Wrapper(string, 160, top, 215, 0, 0x100); */
	Cutscene_DrawText_Wrapper(string, 160, top);

	if (s == NULL) return;

	/* GUI_DrawText_Wrapper(s, 160, top + 18, 215, 0, 0x100); */
	Cutscene_DrawText_Wrapper(s, 160, top + 18);

	s[-1] = 0xD;
}

static void GameLoop_PlaySubtitle(uint8 animation)
{
	const HouseAnimation_Subtitle *subtitle;
	uint8 i;
	uint8 colors[16];

	s_var_8068++;

	GameLoop_PlaySoundEffect(animation);

	subtitle = &s_houseAnimation_subtitle[s_houseAnimation_currentSubtitle];

	if (subtitle->stringID == 0xFFFF || subtitle->animationID > animation) return;

	if (s_subtitleActive) {
		if (s_subtitleWait == 0xFFFF) s_subtitleWait = subtitle->waitFadeout;
		if (s_subtitleWait-- != 0) return;

		s_subtitleActive = false;
		s_houseAnimation_currentSubtitle++;
		s_palettePartDirection = PPD_TO_BLACK;

		if (subtitle->paletteFadeout != 0) {
			s_palettePartCount = subtitle->paletteFadeout;

			for (i = 0; i < 18; i++) {
				s_palettePartChange[i] = s_palettePartTarget[i] / s_palettePartCount;
				if (s_palettePartChange[i] == 0) s_palettePartChange[i] = 1;
			}

			return;
		}

		memcpy(s_palettePartChange, s_palettePartTarget, 18);
		s_palettePartCount = 1;
		return;
	}

	if (s_subtitleWait == 0xFFFF) s_subtitleWait = subtitle->waitFadein;
	if (s_subtitleWait-- != 0) return;

	memcpy(s_palettePartTarget, &g_palette1[(144 + (subtitle->colour * 16)) * 3], 18);

	s_subtitleActive = true;

	/* GUI_DrawFilledRectangle(0, subtitle->top == 85 ? 0 : subtitle->top, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1, 0); */
	{
		const int top = subtitle->top == 85 ? 0 : subtitle->top;
		unsigned char *screen = GFX_Screen_Get_ByIndex(SCREEN_0);

		memset(screen + SCREEN_WIDTH * top, 0, SCREEN_WIDTH * (SCREEN_HEIGHT - top));
	}

	if (s_var_8062 != 0xFFFF && s_houseAnimation_currentSubtitle != 0 && g_gameConfig.language == LANGUAGE_ENGLISH) {
		uint16 loc06 = s_var_8062 + s_houseAnimation_currentSubtitle;

		if ((!enhancement_play_additional_voices) &&
		    (loc06 == VOICE_INTRO_DUNE || loc06 == VOICE_INTRO_THE_BUILDING_OF_A_DYNASTY)) {
			/* Newer versions lost these sounds. */
		}
		else {
			Audio_PlayVoice(loc06);
		}

		if (g_feedback[loc06].messageId != 0) {
			if ((g_feedback[loc06].messageId == 1) ||
			    (g_feedback[loc06].messageId == 2 && g_enable_subtitles)) {
				GameLoop_DrawText(String_Get_ByIndex(subtitle->stringID), subtitle->top);
			}
		}
	} else {
		if (subtitle->stringID != STR_NULL) {
			GameLoop_DrawText(String_Get_ByIndex(subtitle->stringID), subtitle->top);
		}
	}

	s_palettePartDirection = PPD_TO_NEW_PALETTE;

	if (subtitle->paletteFadein != 0) {
		s_palettePartCount = subtitle->paletteFadein;

		for (i = 0; i < 18; i++) {
			s_palettePartChange[i] = s_palettePartTarget[i] / s_palettePartCount;
			if (s_palettePartChange[i] == 0) s_palettePartChange[i] = 1;
		}
	} else {
		memcpy(s_palettePartChange, s_palettePartTarget, 18);
		s_palettePartCount = 1;
	}

	if (g_playerHouseID != HOUSE_INVALID || s_houseAnimation_currentSubtitle != 2) return;

	GUI_DrawText_Wrapper(NULL, 0, 0, 0, 0, 0x21);

	/* GUI_DrawText_Wrapper("Copyright (c) 1992 Westwood Studios, Inc.", 160, 189, 215, 0, 0x112); */
	GUI_DrawText_Wrapper(NULL, 160, 189, 215, 0, 0x112);
	Cutscene_DrawText_Wrapper("Copyright (c) 1992 Westwood Studios, Inc.", 160, 189);

	g_fontCharOffset = 0;

	colors[0] = 0;
	for (i = 0; i < 6; i++) colors[i + 1] = 215 + i;

	GUI_InitColors(colors, 0, 15);

	Font_Select(g_fontIntro);
}

/**
 * Update part of the palette one step.
 * @param finishNow Finish all steps now.
 * @return whether to redraw.
 * @note If \a finishNow, the new palette is not written to the screen.
 * @see PalettePartDirection
 */
static bool
GameLoop_PalettePart_Update(bool finishNow)
{
	Audio_Poll();

	if (s_palettePartDirection == PPD_STOPPED) return false;

	if (s_paletteAnimationTimeout >= Timer_GetTicks() && !finishNow) return false;

	s_paletteAnimationTimeout = Timer_GetTicks() + 7;
	if (--s_palettePartCount == 0 || finishNow) {
		if (s_palettePartDirection == PPD_TO_NEW_PALETTE) {
			memcpy(s_palettePartCurrent, s_palettePartTarget, 18);
		} else {
			memset(s_palettePartCurrent, 0, 18);
		}

		s_palettePartDirection = PPD_STOPPED;
	} else {
		uint8 i;

		for (i = 0; i < 18; i++) {
			if (s_palettePartDirection == PPD_TO_NEW_PALETTE) {
				s_palettePartCurrent[i] = min(s_palettePartCurrent[i] + s_palettePartChange[i], s_palettePartTarget[i]);
			} else {
				s_palettePartCurrent[i] = max(s_palettePartCurrent[i] - s_palettePartChange[i], 0);
			}
		}
	}

	if (finishNow) return true;

	memcpy(&g_palette_998A[215 * 3], s_palettePartCurrent, 18);

	GFX_SetPalette(g_palette_998A);

	return true;
}

static void GameLoop_PlayAnimation(void)
{
	const HouseAnimation_Animation *animation;
	uint8 animationMode = 0;

	animation = s_houseAnimation_animation;

	while (animation->duration != 0) {
		const uint16 posX = ((animation->flags & 0x20) == 0) ? 8 : 0;
		const uint16 posY = ((animation->flags & 0x20) == 0) ? 24 : 0;
		const uint16 mode = animation->flags & 0x3;

		uint16 loc04;
		int64_t loc10 = Timer_GetTicks() + animation->duration * 6;
		int64_t loc14 = loc10 + 30;
		int64_t loc18;
		int64_t loc1C;
		bool loc20;
		uint32 loc24;
		uint16 locdi;
		uint16 frame;
		void *wsa;

		s_var_8068 = 0;

		if (mode == 0) {
			wsa = NULL;
			frame = 0;
		} else {
			char filenameBuffer[16];

			if (mode == 3) {
				frame = animation->frameCount;
				loc20 = true;
			} else {
				frame = 0;
				loc20 = ((animation->flags & 0x40) != 0) ? true : false;
			}

			if ((animation->flags & 0x480) != 0) {
				GUI_ClearScreen(SCREEN_1);

				wsa = GFX_Screen_Get_ByIndex(SCREEN_2);

				loc24 = GFX_Screen_GetSize_ByIndex(SCREEN_2) + GFX_Screen_GetSize_ByIndex(SCREEN_3);
				loc20 = false;
			} else {
				wsa = GFX_Screen_Get_ByIndex(SCREEN_1);

				loc24 = GFX_Screen_GetSize_ByIndex(SCREEN_1) + GFX_Screen_GetSize_ByIndex(SCREEN_2) + GFX_Screen_GetSize_ByIndex(SCREEN_3);
			}

			snprintf(filenameBuffer, sizeof(filenameBuffer), "%s.WSA", animation->string);
			wsa = WSA_LoadFile(filenameBuffer, wsa, loc24, loc20);
		}

		locdi = 0;
		if ((animation->flags & 0x8) != 0) {
			loc10 -= 45;
			locdi++;
		} else {
			if ((animation->flags & 0x10) != 0) {
				loc10 -= 15;
				locdi++;
			}
		}

		if ((animation->flags & 0x4) != 0) {
			GameLoop_PlaySubtitle(animationMode);
			WSA_DisplayFrame(wsa, frame++, posX, posY, SCREEN_0);
			GameLoop_PalettePart_Update(true);
			Cutscene_CopyScreen();

			memcpy(&g_palette1[215 * 3], s_palettePartCurrent, 18);

			Cutscene_SetPaletteAnimated(g_palette1, 45);

			locdi++;
		} else {
			if ((animation->flags & 0x480) != 0) {
				GameLoop_PlaySubtitle(animationMode);
				WSA_DisplayFrame(wsa, frame++, posX, posY, SCREEN_1);
				locdi++;

				if ((animation->flags & 0x480) == 0x80) {
					Cutscene_Screen_FadeIn2(8, 24, 304, 120, SCREEN_1, SCREEN_0, 1, false);
				} else if ((animation->flags & 0x480) == 0x400) {
					Cutscene_Screen_FadeIn(1, 24, 1, 24, 38, 120, SCREEN_1, SCREEN_0);
				}
			}
		}

		loc1C = loc10 - Timer_GetTicks();
		loc18 = 0;
		loc04 = 1;

		switch (mode) {
			case 0:
				loc04 = animation->frameCount - locdi;
				loc18 = loc1C / loc04;
				break;

			case 1:
				loc04 = WSA_GetFrameCount(wsa);
				loc18 = loc1C / animation->frameCount;
				break;

			case 2:
				loc04 = WSA_GetFrameCount(wsa) - locdi;
				loc18 = loc1C / loc04;
				loc10 -= loc18;
				break;

			case 3:
				frame = animation->frameCount;
				loc04 = 1;
				loc18 = loc1C / 20;
				break;

			default:
				PrepareEnd();
				Error("Bad mode in animation #%i.\n", animationMode);
				exit(0);
		}

		while (Timer_GetTicks() < loc10) {
			const int64_t timeout = Timer_GetTicks() + loc18;

			GameLoop_PlaySubtitle(animationMode);
			WSA_DisplayFrame(wsa, frame++, posX, posY, SCREEN_0);

			if (mode == 1 && frame == loc04) {
				frame = 0;
			} else {
				if (mode == 3) frame--;
			}

			if (Cutscene_InputSkipScene()) {
				WSA_Unload(wsa);
				return;
			}

			bool redraw = true;
			do {
				if (GameLoop_PalettePart_Update(false))
					redraw = true;

				if (redraw) {
					redraw = false;
					Cutscene_CopyScreen();
				}

				Timer_Sleep(1);
			} while ((Timer_GetTicks() < timeout) && (Timer_GetTicks() < loc10));
		}

		if (mode == 2) {
			bool displayed;
			do {
				GameLoop_PlaySubtitle(animationMode);
				displayed = WSA_DisplayFrame(wsa, frame++, posX, posY, SCREEN_0);
				Cutscene_CopyScreen();
			} while (displayed);
		}

		if ((animation->flags & 0x10) != 0) {
			memset(&g_palette_998A[3 * 1], 63, 255 * 3);

			memcpy(&g_palette_998A[215 * 3], s_palettePartCurrent, 18);

			Cutscene_SetPaletteAnimated(g_palette_998A, 15);

			memcpy(g_palette_998A, g_palette1, 256 * 3);
		}

		if ((animation->flags & 0x8) != 0) {
			GameLoop_PalettePart_Update(true);

			memcpy(&g_palette_998A[215 * 3], s_palettePartCurrent, 18);

			Cutscene_SetPaletteAnimated(g_palette_998A, 45);
		}

		WSA_Unload(wsa);

		animationMode++;
		animation++;

		Timer_Sleep(loc14 - Timer_GetTicks());
	}
}

void
Cutscene_PlayAnimation(enum HouseAnimationType anim)
{
	const HouseAnimation_Animation *animation;
	const HouseAnimation_Subtitle *subtitle;
	const HouseAnimation_SoundEffect *soundEffect;
	enum MusicID musicID = MUSIC_INVALID;

	Input_History_Clear();

	animation   = g_table_houseAnimation_animation[anim];
	subtitle    = g_table_houseAnimation_subtitle[anim];
	soundEffect = g_table_houseAnimation_soundEffect[anim];

	switch (anim) {
		case HOUSEANIMATION_INTRO:
		case HOUSEANIMATION_MAX:
			/* Note: use GameLoop_GameIntroAnimation instead. */
			return;

		case HOUSEANIMATION_LEVEL4_HARKONNEN:
		case HOUSEANIMATION_LEVEL4_ATREIDES:
		case HOUSEANIMATION_LEVEL4_ORDOS:
		case HOUSEANIMATION_LEVEL8_HARKONNEN:
		case HOUSEANIMATION_LEVEL8_ATREIDES:
		case HOUSEANIMATION_LEVEL8_ORDOS:
			musicID = MUSIC_CUTSCENE;
			break;

		case HOUSEANIMATION_LEVEL9_HARKONNEN:
			Audio_LoadSampleSet(SAMPLESET_BENE_GESSERIT);
			musicID = MUSIC_END_GAME_HARKONNEN;
			break;

		case HOUSEANIMATION_LEVEL9_ATREIDES:
			Audio_LoadSampleSet(SAMPLESET_BENE_GESSERIT);
			musicID = MUSIC_END_GAME_ATREIDES;
			break;

		case HOUSEANIMATION_LEVEL9_ORDOS:
			Audio_LoadSampleSet(SAMPLESET_BENE_GESSERIT);
			musicID = MUSIC_END_GAME_ORDOS;
			break;
	}

	Video_HideCursor();

	/* Need to set the palette manually.  Normally it will be black due to fading out. */
	memset(g_palette1, 0, 256 * 3);
	Video_SetPalette(g_palette1, 0, 256);

	GameLoop_PrepareAnimation(animation, subtitle, 0xFFFF, soundEffect);
	Audio_PlayMusic(musicID);
	GameLoop_PlayAnimation();
	Audio_PlayEffect(EFFECT_FADE_OUT);
	GameLoop_FinishAnimation();

	Audio_PlayMusic(MUSIC_STOP);
	Video_ShowCursor();
}

void GameLoop_LevelEndAnimation(void)
{
	enum HouseAnimationType anim;

	switch (g_campaignID) {
		case 4:
			switch (g_playerHouseID) {
				case HOUSE_HARKONNEN:
					anim = HOUSEANIMATION_LEVEL4_HARKONNEN;
					break;

				case HOUSE_ATREIDES:
					anim = HOUSEANIMATION_LEVEL4_ATREIDES;
					break;

				case HOUSE_ORDOS:
					anim = HOUSEANIMATION_LEVEL4_ORDOS;
					break;

				default: return;
			} break;

		case 8:
			switch (g_playerHouseID) {
				case HOUSE_HARKONNEN:
					anim = HOUSEANIMATION_LEVEL8_HARKONNEN;
					break;

				case HOUSE_ATREIDES:
					anim = HOUSEANIMATION_LEVEL8_ATREIDES;
					break;

				case HOUSE_ORDOS:
					anim = HOUSEANIMATION_LEVEL8_ORDOS;
					break;

				default: return;
			}
			break;

		default: return;
	}

	Cutscene_PlayAnimation(anim);
}

static void GameCredits_SwapScreen(uint16 top, uint16 height, Screen screenID, void *buffer)
{
	uint16 *b = (uint16 *)buffer;
	uint16 *screen1 = (uint16 *)GFX_Screen_Get_ByIndex(screenID) + top * SCREEN_WIDTH / 2;
	uint16 *screen2 = (uint16 *)GFX_Screen_Get_ByIndex(SCREEN_0) + top * SCREEN_WIDTH / 2;
	uint16 count = height * SCREEN_WIDTH / 2;

	while (count-- != 0) {
		if (*b++ != *screen1++) {
			if (count == 0) return;
			b--;
			screen1--;
			*b++ = *screen1;
			*screen2 = *screen1++;
		}
		screen2++;
	}
}

static void GameCredits_Play(char *data, uint16 windowID, Screen memory, Screen screenID, uint16 delay)
{
	uint16 loc02;
	uint16 stringCount = 0;
	uint16 spriteID = 514;
	bool loc10 = false;
	uint16 spriteX;
	uint16 spriteY;
	uint16 spritePos = 0;
	struct {
		uint16 x;
		int16 y;
		char *text;
		uint8  separator;
		uint8  charHeight;
		uint8  type;
	} strings[33];
	struct {
		uint16 x;
		uint16 y;
	} positions[6];
	uint16 stage = 4;
	uint16 counter = 60;

	Widget_SetCurrentWidget(windowID);

	spriteX = g_curWidgetWidth - Sprite_GetWidth(g_sprites[spriteID]);
	spriteY = g_curWidgetHeight - Sprite_GetHeight(g_sprites[spriteID]);

	positions[0].x = spriteX;
	positions[0].y = 0;
	positions[1].x = 0;
	positions[1].y = spriteY / 2;
	positions[2].x = spriteX;
	positions[2].y = spriteY;
	positions[3].x = 0;
	positions[3].y = 0;
	positions[4].x = spriteX;
	positions[4].y = spriteY / 2;
	positions[5].x = 0;
	positions[5].y = spriteY;

	GUI_Screen_Copy(0, 0, 0, 0, SCREEN_WIDTH / 8, SCREEN_HEIGHT, SCREEN_0, memory);
	GUI_Screen_Copy(0, 0, 0, 0, SCREEN_WIDTH / 8, SCREEN_HEIGHT, memory, screenID);

	GameCredits_SwapScreen(g_curWidgetYBase, g_curWidgetHeight, memory, s_buffer_182E);

	GFX_Screen_SetActive(SCREEN_0);
	int64_t loc0C = Timer_GetTicks();

	Input_History_Clear();

	while (true) {
		Timer_Sleep(loc0C - Timer_GetTicks());
		loc0C = Timer_GetTicks() + delay;

		while ((g_curWidgetHeight / 6) + 2 > stringCount && *data != 0) {
			char *text = data;
			uint16 y;

			if (stringCount != 0) {
				y = strings[stringCount - 1].y;
				if (strings[stringCount - 1].separator != 5) y += strings[stringCount - 1].charHeight + strings[stringCount - 1].charHeight / 8;
			} else {
				y = g_curWidgetHeight;
			}

			text = data;

			data = strpbrk(data, "\x05\r");
			if (data == NULL) data = strchr(text, '\0');

			strings[stringCount].separator = *data;
			*data = '\0';
			if (strings[stringCount].separator != 0) data++;
			strings[stringCount].type = 0;

			if (*text == 3 || *text == 4) strings[stringCount].type = *text++;

			if (*text == 1) {
				text++;
				Font_Select(g_fontNew6p);
			} else if (*text == 2) {
				text++;
				Font_Select(g_fontNew8p);
			}

			strings[stringCount].charHeight = g_fontCurrent->height;

			switch (strings[stringCount].type) {
				case 3:
					strings[stringCount].x = 157 - Font_GetStringWidth(text);
					break;

				case 4:
					strings[stringCount].x = 161;
					break;

				default:
					strings[stringCount].x = 1 + (SCREEN_WIDTH - Font_GetStringWidth(text)) / 2;
					break;
			}

			strings[stringCount].y = y;
			strings[stringCount].text = text;

			stringCount++;
		}

		switch (stage) {
			case 0:
				GUI_ClearScreen(memory);

				if (spriteID == 514) GUI_ClearScreen(screenID);

				stage++;
				counter = 2;
				break;

			case 1: case 4:
				if (counter-- == 0) {
					counter = 0;
					stage++;
				}
				break;

			case 2:
				if (spriteID == 525) spriteID = 514;

				GUI_DrawSprite_(memory, g_sprites[spriteID], positions[spritePos].x, positions[spritePos].y, windowID, 0x4000);

				counter = 8;
				stage++;
				spriteID++;
				if (++spritePos > 5) spritePos = 0;;
				break;

			case 3:
				GFX_SetPalette(s_palette10 + 256 * 3 * counter);

				if (counter-- == 0) {
					stage++;
					counter = 20;
				}
				break;

			case 5:
				GFX_SetPalette(s_palette10 + 256 * 3 * counter);

				if (counter++ >= 8) stage = 0;
				break;

			default: break;
		}

		GUI_Screen_Copy(g_curWidgetXBase/8, g_curWidgetYBase, g_curWidgetXBase/8, g_curWidgetYBase, g_curWidgetWidth/8, g_curWidgetHeight, memory, screenID);

		for (loc02 = 0; loc02 < stringCount; loc02++) {
			if ((int16)strings[loc02].y < g_curWidgetHeight) {
				GFX_Screen_SetActive(screenID);

				Font_Select(g_fontNew8p);

				if (strings[loc02].charHeight != g_fontCurrent->height) Font_Select(g_fontNew6p);

				Cutscene_DrawText(strings[loc02].text, strings[loc02].x, strings[loc02].y + g_curWidgetYBase, 255, 0);

				GFX_Screen_SetActive(SCREEN_0);
			}

			strings[loc02].y--;
		}

		GameCredits_SwapScreen(g_curWidgetYBase, g_curWidgetHeight, screenID, s_buffer_182E);

		if ((int16)strings[0].y < -10) {
			strings[0].text += strlen(strings[0].text);
			*strings[0].text = strings[0].separator;
			stringCount--;
			memcpy(&strings[0], &strings[1], stringCount * sizeof(*strings));
		}

		if ((g_curWidgetHeight / 6 + 2) > stringCount) {
			if (strings[stringCount - 1].y + strings[stringCount - 1].charHeight < g_curWidgetYBase + g_curWidgetHeight) loc10 = true;
		}

		if (loc10 && stage == 0) break;

		if (Cutscene_InputSkipScene()) break;
		Cutscene_CopyScreen();
	}

	Cutscene_SetPaletteAnimated(g_palette2, 120);

	GUI_ClearScreen(SCREEN_0);
	GUI_ClearScreen(memory);
	GUI_ClearScreen(screenID);
}

static void GameCredits_LoadPalette(void)
{
	uint16 i;
	uint8 *p;

	s_buffer_182E = GFX_Screen_Get_ByIndex(SCREEN_3);
	s_buffer_1832 = (uint8 *)s_buffer_182E + SCREEN_WIDTH * g_curWidgetHeight;

	memset(g_palette2, 0, 3 * 256);

	File_ReadBlockFile("IBM.PAL", g_palette1, 256 * 3);

	/* Create 10 fadein/fadeout palettes */
	p = s_palette10;
	for (i = 0; i < 10; i++) {
		uint16 j;
		uint8 *pr = g_palette1;

		for (j = 0; j < 255 * 3; j++) *p++ = *pr++ * (9 - i) / 9;

		*p++ = 0x3F;
		*p++ = 0x3F;
		*p++ = 0x3F;
	}
}

/**
 * Shows the game credits.
 */
void
GameLoop_GameCredits(enum HouseType houseID)
{
	const uint8 colours[] = {0, 0, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	const uint16 last_widget = Widget_SetCurrentWidget(20);

	uint16 i;
	uint8 *memory;

	Video_HideCursor();

	Sprites_InitCredits();
	Sprites_LoadImage(SEARCHDIR_GLOBAL_DATA_DIR, "BIGPLAN.CPS", SCREEN_1, g_palette_998A);

	GUI_ClearScreen(SCREEN_0);

	GUI_Screen_Copy(g_curWidgetXBase/8, g_curWidgetYBase, g_curWidgetXBase/8, g_curWidgetYBase, g_curWidgetWidth/8, g_curWidgetHeight, SCREEN_1, SCREEN_0);

	Cutscene_SetPaletteAnimated(g_palette_998A, 60);

	Audio_PlayMusic(MUSIC_CREDITS);

	memory = GFX_Screen_Get_ByIndex(SCREEN_2);

	for (i = 0; i < 256; i++) {
		uint8 loc06;
		uint8 loc04;

		memory[i] = i & 0xFF;

		loc06 = i / 16;
		loc04 = i % 16;

		if (loc06 == 9 && loc04 <= 6) {
			memory[i] = (g_playerHouseID * 16) + loc04 + 144;
		}
	}

	Sprites_LoadImage(SEARCHDIR_GLOBAL_DATA_DIR, "MAPPLAN.CPS", SCREEN_1, g_palette_998A);

	GUI_Palette_RemapScreen(g_curWidgetXBase, g_curWidgetYBase, g_curWidgetWidth, g_curWidgetHeight, SCREEN_1, memory);

	/* Special treat if you win the game - Dune turns your house's colour. */
	if (houseID != HOUSE_INVALID)
		Cutscene_Screen_FadeIn2(g_curWidgetXBase, g_curWidgetYBase, g_curWidgetWidth, g_curWidgetHeight, SCREEN_1, SCREEN_0, 1, false);

	GameCredits_LoadPalette();

	GUI_InitColors(colours, 0, lengthof(colours) - 1);

	g_fontCharOffset = -1;

	GFX_SetPalette(g_palette1);

	for (;; sleepIdle()) {
		File_ReadBlockFile(String_GenerateFilename("CREDITS"), s_buffer_1832, GFX_Screen_GetSize_ByIndex(SCREEN_3));

		if ((enhancement_subtitle_override != SUBTITLE_THE_BATTLE_FOR_ARRAKIS) && (g_gameConfig.language == LANGUAGE_ENGLISH)) {
			const char *find_str = g_gameSubtitle[SUBTITLE_THE_BATTLE_FOR_ARRAKIS];
			const char *replace_str = g_gameSubtitle[enhancement_subtitle_override];
			const int find_len = strlen(find_str);
			const int replace_len = strlen(replace_str);

			char *str = s_buffer_1832;

			str = strstr(str, find_str);
			while (str != NULL) {
				memmove(str + replace_len, str + find_len, strlen(str) + 1);
				memcpy(str, replace_str, replace_len);

				str = strstr(str + replace_len, find_str);
			}
		}

		GameCredits_Play(s_buffer_1832, 20, SCREEN_1, SCREEN_2, 6);

		if (Cutscene_InputSkipScene()) break;

		Audio_PlayMusic(MUSIC_CREDITS);
	}

	Cutscene_SetPaletteAnimated(g_palette2, 60);

	Audio_PlayEffect(EFFECT_FADE_OUT);
	Audio_PlayMusic(MUSIC_STOP);

	GFX_ClearScreen();

	Widget_SetCurrentWidget(last_widget);

	Video_ShowCursor();
}

/**
 * Shows the end game "movie"
 */
void GameLoop_GameEndAnimation(void)
{
	enum HouseAnimationType anim;

	switch (g_playerHouseID) {
		case HOUSE_HARKONNEN:
			anim = HOUSEANIMATION_LEVEL9_HARKONNEN;
			break;

		default:
		case HOUSE_ATREIDES:
			anim = HOUSEANIMATION_LEVEL9_ATREIDES;
			break;

		case HOUSE_ORDOS:
			anim = HOUSEANIMATION_LEVEL9_ORDOS;
			break;
	}

	Cutscene_PlayAnimation(anim);
	GameLoop_GameCredits(g_playerHouseID);
}

/**
 * Logos at begin of intro.
 */
static void Gameloop_Logos(void)
{
	Screen oldScreenID;
	void *wsa;
	uint16 frame;

	oldScreenID = GFX_Screen_SetActive(SCREEN_0);

	GFX_SetPalette(g_palette2);
	GFX_ClearScreen();

	File_ReadBlockFile("WESTWOOD.PAL", g_palette_998A, 256 * 3);

	frame = 0;
	wsa = WSA_LoadFile("WESTWOOD.WSA", GFX_Screen_Get_ByIndex(SCREEN_1), GFX_Screen_GetSize_ByIndex(SCREEN_1) + GFX_Screen_GetSize_ByIndex(SCREEN_2) + GFX_Screen_GetSize_ByIndex(SCREEN_3), true);
	WSA_DisplayFrame(wsa, frame++, 0, 0, SCREEN_0);

	Cutscene_SetPaletteAnimated(g_palette_998A, 60);

	Audio_PlayMusic(MUSIC_LOGOS);

	const int64_t timeout1 = 360 + Timer_GetTicks();

	while (WSA_DisplayFrame(wsa, frame++, 0, 0, SCREEN_0)) {
		Cutscene_CopyScreen();

		for (int i = 0; i < 6; i++) {
			Input_Tick(true);
			Timer_Sleep(1);
		}
	}

	WSA_Unload(wsa);

	/* Voice_LoadVoices(0xFFFF); */

	while (Timer_GetTicks() < timeout1) {
		if (Input_Tick(true))
			Cutscene_CopyScreen();

		if (!Cutscene_InputSkipScene()) {
			sleepIdle();
			continue;
		}

		Cutscene_SetPaletteAnimated(g_palette2, 30);

		GUI_ClearScreen(SCREEN_0);

		GFX_Screen_SetActive(oldScreenID);
		return;
	}

	Cutscene_SetPaletteAnimated(g_palette2, 60);

	while (Audio_MusicIsPlaying()) {
		Audio_PollMusic();
		sleepIdle();
	}

	Cutscene_SetPaletteAnimated(g_palette2, 60);

	GFX_ClearScreen();

	Sprites_LoadImage(SEARCHDIR_GLOBAL_DATA_DIR, String_GenerateFilename("AND"), SCREEN_1, g_palette_998A);

	GUI_Screen_Copy(0, 0, 0, 0, SCREEN_WIDTH / 8, SCREEN_HEIGHT, SCREEN_1, SCREEN_0);

	Cutscene_SetPaletteAnimated(g_palette_998A, 30);

	for (int timeout = 0; timeout < 60; timeout++) {
		if (!Cutscene_InputSkipScene()) {
			Cutscene_CopyScreen();
			Timer_Sleep(1);
			continue;
		}

		Cutscene_SetPaletteAnimated(g_palette2, 30);

		GUI_ClearScreen(SCREEN_0);

		GFX_Screen_SetActive(oldScreenID);
		return;
	}

	Cutscene_SetPaletteAnimated(g_palette2, 30);

	GUI_ClearScreen(SCREEN_0);

	Sprites_LoadImage(SEARCHDIR_GLOBAL_DATA_DIR, "VIRGIN.CPS", SCREEN_1, g_palette_998A);

	GUI_Screen_Copy(0, 0, 0, 0, SCREEN_WIDTH / 8, SCREEN_HEIGHT, SCREEN_1, SCREEN_0);

	Cutscene_SetPaletteAnimated(g_palette_998A, 30);

	for (int timeout = 0; timeout < 180; timeout++) {
		if (Input_Tick(true))
			Cutscene_CopyScreen();

		if (!Cutscene_InputSkipScene()) {
			Timer_Sleep(1);
			continue;
		}

		break;
	}

	Cutscene_SetPaletteAnimated(g_palette2, 30);

	GUI_ClearScreen(SCREEN_0);

	GFX_Screen_SetActive(oldScreenID);
}

/**
 * The Intro.
 */
void GameLoop_GameIntroAnimation(void)
{
	const enum HouseType h = g_playerHouseID;

	GUI_ChangeSelectionType(SELECTIONTYPE_INTRO);
	Video_HideCursor();

	Audio_PlayMusic(MUSIC_STOP);
	Gameloop_Logos();

	g_playerHouseID = HOUSE_INVALID;
	Audio_LoadSampleSet(SAMPLESET_BENE_GESSERIT);

	if (!Cutscene_InputSkipScene()) {
		const HouseAnimation_Animation   *animation   = g_table_houseAnimation_animation[HOUSEANIMATION_INTRO];
		const HouseAnimation_Subtitle    *subtitle    = g_table_houseAnimation_subtitle[HOUSEANIMATION_INTRO];
		const HouseAnimation_SoundEffect *soundEffect = g_table_houseAnimation_soundEffect[HOUSEANIMATION_INTRO];

		Audio_PlayMusic(MUSIC_INTRO);

		GameLoop_PrepareAnimation(animation, subtitle, VOICE_INTRO_PRESENT, soundEffect);

		GameLoop_PlayAnimation();

		Audio_PlayEffect(EFFECT_FADE_OUT);
	}

	GameLoop_FinishAnimation();

	GUI_ChangeSelectionType(SELECTIONTYPE_MENTAT);
	Video_ShowCursor();
	Audio_PlayVoice(VOICE_STOP);
	Audio_PlayMusic(MUSIC_STOP);

	g_playerHouseID = h;
}
