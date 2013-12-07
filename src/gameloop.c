/* gameloop.c */

#include <assert.h>
#include <allegro5/allegro.h>
#include <math.h>
#include "os/common.h"
#include "os/math.h"

#include "gameloop.h"

#include "ai.h"
#include "animation.h"
#include "audio/audio.h"
#include "common_a5.h"
#include "config.h"
#include "enhancement.h"
#include "explosion.h"
#include "gui/gui.h"
#include "house.h"
#include "input/input.h"
#include "input/mouse.h"
#include "map.h"
#include "net/client.h"
#include "net/net.h"
#include "net/server.h"
#include "newui/actionpanel.h"
#include "newui/menubar.h"
#include "newui/viewport.h"
#include "opendune.h"
#include "pool/pool.h"
#include "pool/structure.h"
#include "pool/unit.h"
#include "sprites.h"
#include "structure.h"
#include "team.h"
#include "tile.h"
#include "timer/timer.h"
#include "tools/coord.h"
#include "unit.h"
#include "video/video.h"

/*--------------------------------------------------------------*/

static void
GameLoop_Client_Structure(void)
{
	if (!enhancement_fog_of_war)
		return;

	PoolFindStruct find;
	Structure *s;

	find.houseID = HOUSE_INVALID;
	find.index   = 0xFFFF;
	find.type    = 0xFFFF;

	while ((s = Structure_Find(&find)) != NULL) {
		Structure_RemoveFog(UNVEILCAUSE_STRUCTURE_VISION, s);
	}
}

static void
GameLoop_Client_Unit(void)
{
	if (!enhancement_fog_of_war)
		return;

	PoolFindStruct find;
	Unit *u;

	find.houseID = HOUSE_INVALID;
	find.index   = 0xFFFF;
	find.type    = 0xFFFF;

	while ((u = Unit_Find(&find)) != NULL) {
		const UnitInfo *ui = &g_table_unitInfo[u->o.type];

		Unit_RefreshFog(UNVEILCAUSE_UNIT_VISION, u, ui->flags.isGroundUnit);
	}
}

static void
GameLoop_Server_Logic(void)
{
	UnitAI_SquadLoop();
	GameLoop_Team();
	GameLoop_Unit();
	GameLoop_Structure();
	GameLoop_House();
	Explosion_Tick();
	Animation_Tick();
	Unit_Sort();
}

static void
GameLoop_Client_Logic(void)
{
	GameLoop_Client_Unit();
	GameLoop_Client_Structure();
	Unit_Sort();
}

/*--------------------------------------------------------------*/

/* Process input not caught by widgets, including keypad scrolling,
 * squad selection, and changing zoom levels.
 */
static void
GameLoop_Client_ProcessUnhandledInput(bool init_transform, uint16 key)
{
	const struct {
		enum Scancode code;
		int dx, dy;
	} keypad[8] = {
		{ SCANCODE_KEYPAD_1, -1,  1 },
		{ SCANCODE_KEYPAD_2,  0,  1 },
		{ SCANCODE_KEYPAD_3,  1,  1 },
		{ SCANCODE_KEYPAD_4, -1,  0 },
		{ SCANCODE_KEYPAD_6,  1,  0 },
		{ SCANCODE_KEYPAD_7, -1, -1 },
		{ SCANCODE_KEYPAD_8,  0, -1 },
		{ SCANCODE_KEYPAD_9,  1, -1 }
	};

	int dx = 0, dy = 0;

	for (unsigned int i = 0; i < lengthof(keypad); i++) {
		if ((key == keypad[i].code) ||
		    (key == 0 && Input_Test(keypad[i].code))) {
			dx += keypad[i].dx;
			dy += keypad[i].dy;
		}
	}

	if (dx != 0 || dy != 0) {
		dx = g_gameConfig.scrollSpeed * clamp(-1, dx, 1);
		dy = g_gameConfig.scrollSpeed * clamp(-1, dy, 1);
	}

	if ((fabsf(g_viewport_desiredDX) >= 4.0f) || (fabsf(g_viewport_desiredDY) >= 4.0f)) {
		dx += 0.25 * g_viewport_desiredDX;
		dy += 0.25 * g_viewport_desiredDY;
		if (fabsf(g_viewport_desiredDX) >= 4.0f) g_viewport_desiredDX *= 0.75;
		if (fabsf(g_viewport_desiredDY) >= 4.0f) g_viewport_desiredDY *= 0.75;
	}

	if (dx != 0 || dy != 0) {
		Map_MoveDirection(dx, dy);
	}

	switch (key) {
#if 0
		case SCANCODE_TAB: /* TAB, SHIFT TAB */
			Map_SelectNext(true);
			Map_SelectNext(false);
			return;
#endif

		case SCANCODE_1:
		case SCANCODE_2:
		case SCANCODE_3:
		case SCANCODE_4:
		case SCANCODE_5:
		case SCANCODE_6:
		case SCANCODE_7:
		case SCANCODE_8:
		case SCANCODE_9:
		case SCANCODE_0:
			Viewport_Hotkey(key - SCANCODE_1 + SQUADID_1);
			break;

		case SCANCODE_H:
			Viewport_Homekey();
			break;

		case SCANCODE_F5:
			Audio_DisplayMusicName();
			break;

		case SCANCODE_F6:
		case SCANCODE_F7:
			{
				const bool increase = (key == SCANCODE_F7);
				const bool adjust_current_track_only = Input_Test(SCANCODE_LSHIFT);

				Audio_AdjustMusicVolume(increase ? 0.05f : -0.05f, adjust_current_track_only);
				Audio_DisplayMusicName();
			}
			break;

		case SCANCODE_OPENBRACE:
		case SCANCODE_CLOSEBRACE:
			{
				enum ScreenDivID divID = (key == SCANCODE_OPENBRACE) ? SCREENDIV_MENUBAR : SCREENDIV_SIDEBAR;
				ScreenDiv *viewport = &g_screenDiv[SCREENDIV_VIEWPORT];
				ScreenDiv *div = &g_screenDiv[divID];
				const int oldh = viewport->height;

				div->scalex = (div->scalex >= 1.5f) ? 1.0f : 2.0f;
				div->scaley = div->scalex;
				g_factoryWindowTotal = -1;
				init_transform = false;
				A5_InitTransform(false);
				GameLoop_TweakWidgetDimensions();
				Map_MoveDirection(0, oldh - viewport->height);
			}
			break;

		case 0x80 | MOUSE_ZAXIS:
			if (g_mouseDZ == 0)
				break;

			if (g_gameConfig.holdControlToZoom) {
				if (!Input_Test(SCANCODE_LCTRL)) {
					Widget *w = GUI_Widget_Get_ByIndex(g_widgetLinkedListHead, 5);

					if ((w != NULL) && !w->flags.invisible)
						GUI_Widget_SpriteTextButton_Click(w);

					break;
				}
			}
			else {
				const WidgetProperties *w = &g_widgetProperties[WINDOWID_ACTIONPANEL_FRAME];

				if (Mouse_InRegion_Div(SCREENDIV_SIDEBAR, w->xBase, w->yBase, w->xBase + w->width - 1, w->yBase + w->height - 1))
					break;
			}
			/* Fall though. */
		case SCANCODE_MINUS:
		case SCANCODE_EQUALS:
		case SCANCODE_KEYPAD_MINUS:
		case SCANCODE_KEYPAD_PLUS:
			{
				const float scaling_factor[] = { 1.0f, 1.5f, 2.0f, 3.0f };
				ScreenDiv *viewport = &g_screenDiv[SCREENDIV_VIEWPORT];

				int curr;
				for (curr = 0; curr < (int)lengthof(scaling_factor); curr++) {
					if (viewport->scalex <= scaling_factor[curr])
						break;
				}

				const int tilex = Tile_GetPackedX(g_viewportPosition);
				const int tiley = Tile_GetPackedY(g_viewportPosition);
				int viewport_cx = g_viewport_scrollOffsetX + viewport->width / 2;
				int viewport_cy = g_viewport_scrollOffsetY + viewport->height / 2;
				int new_scale;

				/* For mouse wheel zooming in, zoom towards the cursor. */
				if ((key == (0x80 | MOUSE_ZAXIS)) && (g_mouseDZ > 0)) {
					const ScreenDiv *div = &g_screenDiv[SCREENDIV_VIEWPORT];

					if (Mouse_InRegion_Div(SCREENDIV_VIEWPORT, 0, 0, div->width, div->height)) {
						int mousex, mousey;

						new_scale = curr + 1;

						Mouse_TransformToDiv(SCREENDIV_VIEWPORT, &mousex, &mousey);
						viewport_cx = (0.50 * viewport_cx) + (0.50 * mousex);
						viewport_cy = (0.50 * viewport_cy) + (0.50 * mousey);
					}
					else {
						new_scale = curr + 1;
					}
				}
				else {
					if (key == SCANCODE_EQUALS || key == SCANCODE_KEYPAD_PLUS) {
						new_scale = curr + 1;
					}
					else {
						new_scale = curr - 1;
					}
				}

				new_scale = clamp(0, new_scale, (int)lengthof(scaling_factor) - 1);
				viewport_cx += TILE_SIZE * tilex;
				viewport_cy += TILE_SIZE * tiley;

				if (new_scale != curr) {
					viewport->scalex = scaling_factor[new_scale];
					viewport->scaley = scaling_factor[new_scale];
					init_transform = false;
					A5_InitTransform(false);
					GameLoop_TweakWidgetDimensions();
					Map_CentreViewport(viewport_cx, viewport_cy);
					break;
				}
			}
			break;

#if 0
		/* Debugging. */
		case SCANCODE_F9:
			Tile_RemoveFogInRadius(FLAG_HOUSE_ALL, UNVEILCAUSE_LONG,
					Tile_UnpackTile(Tile_PackXY(32, 32)), 64);
			break;

		case SCANCODE_F10:
			s_debugForceWin = true;
			break;
#endif

		default:
			break;
	}

	/* We may still need to translate the screen for screen shakes. */
	if (init_transform)
		A5_InitTransform(false);
}

static void
GameLoop_Client_ProcessInput(void)
{
	const bool init_transform = GFX_ScreenShake_Tick();

	if (g_gameOverlay == GAMEOVERLAY_NONE) {
		Input_Tick(false);
		A5_InitTransform(false);
		uint16 key = GUI_Widget_HandleEvents(g_widgetLinkedListHead);
		GameLoop_Client_ProcessUnhandledInput(init_transform, key);

		if (g_mousePanning)
			Video_WarpCursor(TRUE_DISPLAY_WIDTH / 2, TRUE_DISPLAY_HEIGHT / 2);
	}
	else {
		Input_Tick(true);

		if (g_gameOverlay == GAMEOVERLAY_HINT) {
			MenuBar_TickHintOverlay();
		}
		else if (g_gameOverlay == GAMEOVERLAY_MENTAT) {
			MenuBar_TickMentatOverlay();
		}
		else if (g_gameOverlay == GAMEOVERLAY_WIN
		      || g_gameOverlay == GAMEOVERLAY_LOSE) {
			MenuBar_TickWinLoseOverlay();
		}
		else {
			MenuBar_TickOptionsOverlay();
		}
	}
}

/*--------------------------------------------------------------*/

static void
GameLoop_Client_Draw(void)
{
	if (g_gameOverlay == GAMEOVERLAY_NONE) {
		GUI_DrawInterfaceAndRadar();
	}
	else if (g_gameOverlay == GAMEOVERLAY_HINT
	      || g_gameOverlay == GAMEOVERLAY_WIN
	      || g_gameOverlay == GAMEOVERLAY_LOSE) {
		GUI_DrawInterfaceAndRadar();
		MenuBar_DrawInGameOverlay();
	}
	else if (g_gameOverlay == GAMEOVERLAY_MENTAT) {
		MenuBar_DrawMentatOverlay();
	}
	else {
		GUI_DrawInterfaceAndRadar();
		MenuBar_DrawOptionsOverlay();
	}

	Video_Tick();
	A5_UseTransform(SCREENDIV_MAIN);
}

/*--------------------------------------------------------------*/

static void
GameLoop_ProcessGUITimer(void)
{
	GameLoop_Client_ProcessInput();

	const bool narrator_speaking = Audio_Poll();
	if (!narrator_speaking) {
		if (!g_enable_audio || !g_enable_music
				|| g_gameOverlay == GAMEOVERLAY_WIN
				|| g_gameOverlay == GAMEOVERLAY_LOSE) {
			g_musicInBattle = 0;
		}
		else if (g_musicInBattle > 0) {
			Audio_PlayMusic(MUSIC_RANDOM_ATTACK);
			g_musicInBattle = -1;
		}
		else if (!Audio_MusicIsPlaying()) {
			const enum MusicID musicID
				= (g_gameOverlay == GAMEOVERLAY_MENTAT)
				? g_table_houseInfo[g_playerHouseID].musicBriefing : MUSIC_RANDOM_IDLE;

			Audio_PlayMusic(musicID);
			g_musicInBattle = 0;
		}
	}
}

static void
GameLoop_ProcessGameTimer(void)
{
	static int64_t l_timerUnitStatus = 0;
	static int16 l_selectionState = -2;

	if ((g_gameOverlay == GAMEOVERLAY_NONE)
			|| (g_host_type != HOSTTYPE_NONE)) {
		const int64_t curr_ticks = Timer_GameTicks();

		if (g_timerGame != curr_ticks) {
			g_timerGame = curr_ticks;
		}
		else {
			return;
		}
	}

	if (g_selectionTypeNew != g_selectionType) {
		GUI_ChangeSelectionType(g_selectionTypeNew);
	}

	if (l_selectionState != g_selectionState) {
		Map_SetSelectionObjectPosition(0xFFFF);
		Map_SetSelectionObjectPosition(g_selectionRectanglePosition);
		l_selectionState = g_selectionState;
	}

	if ((g_gameOverlay == GAMEOVERLAY_NONE)
			&& (g_selectionType == SELECTIONTYPE_TARGET
			 || g_selectionType == SELECTIONTYPE_PLACE
			 || g_selectionType == SELECTIONTYPE_UNIT
			 || g_selectionType == SELECTIONTYPE_STRUCTURE)) {
		if (Unit_AnySelected()) {
			if (l_timerUnitStatus < g_timerGame) {
				Unit_DisplayGroupStatusText();
				l_timerUnitStatus = g_timerGame + 300;
			}

			if (g_selectionType != SELECTIONTYPE_TARGET) {
				const Unit *u = Unit_FirstSelected(NULL);
				g_selectionPosition = Tile_PackTile(Tile_Center(u->o.position));
			}
		}

		if (g_host_type != HOSTTYPE_DEDICATED_SERVER) {
			Client_SendMessages();
		}

		if (g_host_type != HOSTTYPE_DEDICATED_CLIENT) {
			Server_RecvMessages();
			GameLoop_Server_Logic();
		}
		else {
			GameLoop_Client_Logic();
		}
	}
	else if (g_host_type == HOSTTYPE_DEDICATED_SERVER
	      || g_host_type == HOSTTYPE_CLIENT_SERVER) {
		Server_RecvMessages();
		GameLoop_Server_Logic();
	}

	if (g_host_type != HOSTTYPE_DEDICATED_CLIENT) {
		GameLoop_LevelEnd();
	}
}

void
GameLoop_Loop(void)
{
	bool redraw = true;

	g_inGame = true;

	while (g_gameMode == GM_NORMAL) {
		const enum TimerType source = Timer_WaitForEvent();
		const enum NetEvent e = Client_RecvMessages();
		if (e == NETEVENT_DISCONNECT) {
			g_gameMode = GM_QUITGAME;
			break;
		}

		if (source == TIMER_GUI) {
			redraw = true;
			GameLoop_ProcessGUITimer();
		}
		else {
			GameLoop_ProcessGameTimer();
		}

		Server_SendMessages();

		if (redraw && Timer_QueueIsEmpty()) {
			redraw = false;
			GUI_PaletteAnimate();
			GameLoop_Client_Draw();
		}
	}

	g_inGame = false;
}
