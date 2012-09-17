#ifndef TABLE_WIDGETINFO_H
#define TABLE_WIDGETINFO_H

#include "types.h"
#include "../gui/widget.h"

enum GameWidgetType {
	GAME_WIDGET_MENTAT              = 0,
	GAME_WIDGET_OPTIONS             = 1,
	GAME_WIDGET_NAME                = 2,
	GAME_WIDGET_PICTURE             = 3,
	GAME_WIDGET_REPAIR_UPGRADE      = 4,
	GAME_WIDGET_BUILD_PLACE         = 5,
	GAME_WIDGET_CANCEL              = 6,
	GAME_WIDGET_UNIT_COMMAND_1      = 7,
	GAME_WIDGET_UNIT_COMMAND_2      = 8,
	GAME_WIDGET_UNIT_COMMAND_3      = 9,
	GAME_WIDGET_UNIT_COMMAND_4      = 10,
	GAME_WIDGET_SCROLL_UP           = 11,
	GAME_WIDGET_SCROLL_RIGHT        = 12,
	GAME_WIDGET_SCROLL_LEFT         = 13,
	GAME_WIDGET_SCROLL_DOWN         = 14,
	GAME_WIDGET_VIEWPORT            = 15,
	GAME_WIDGET_MINIMAP             = 16,
	GAME_WIDGET_VIEWPORT_FALLBACK   = 17,

	GAME_WIDGET_MAX                 = 18,
	GAME_WIDGET_INVALID             = 0xFFFF
};

typedef struct WidgetInfo {
	int16 index;            /*!< ?? */
	ClickProc *clickProc;   /*!< Function to execute when widget is pressed. */
	int16 shortcut;         /*!< ?? */
	uint16 flags;           /*!< ?? */
	int16  spriteID;        /*!< ?? */
	uint16 offsetX;         /*!< ?? */
	uint16 offsetY;         /*!< ?? */
	uint16 width;           /*!< ?? */
	uint16 height;          /*!< ?? */
	uint16 stringID;        /*!< ?? */

	enum ScreenDivID div;
} WidgetInfo;

extern WidgetInfo g_table_gameWidgetInfo[GAME_WIDGET_MAX + 1];

#endif
