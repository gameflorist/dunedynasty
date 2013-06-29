/** @file src/gui/widget.h %Widget definitions. */

#ifndef GUI_WIDGET_H
#define GUI_WIDGET_H

#include "types.h"
#include "../gfx.h"

/**
 * Types of DrawMode available in the game.
 */
enum DrawMode {
	DRAW_MODE_NONE                = 0,                      /*!< Draw nothing. */
	DRAW_MODE_SPRITE              = 1,                      /*!< Draw a sprite. */
	DRAW_MODE_TEXT                = 2,                      /*!< Draw text. */
	DRAW_MODE_UNKNOWN3            = 3,
	DRAW_MODE_CUSTOM_PROC         = 4,                      /*!< Draw via a custom defined function. */
	DRAW_MODE_WIRED_RECTANGLE     = 5,                      /*!< Draw a wired rectangle. */
	DRAW_MODE_XORFILLED_RECTANGLE = 6,                      /*!< Draw a filled rectangle using xor. */

	DRAW_MODE_MAX             = 7
};

enum WindowID {
	WINDOWID_MODAL_MESSAGE      = 1,
	WINDOWID_VIEWPORT           = 2,
	WINDOWID_MINIMAP            = 3,
	WINDOWID_CREDITS            = 5,
	WINDOWID_ACTIONPANEL_FRAME  = 6,
	WINDOWID_STATUSBAR          = 7,
	WINDOWID_MENTAT_PICTURE     = 8,
	WINDOWID_MENTAT_EDIT_BOX    = 9,
	WINDOWID_STARPORT_INVOICE   = 11,
	WINDOWID_MAINMENU_FRAME     = 13,
	WINDOWID_MAINMENU_ITEM      = 21,
	WINDOWID_RENDER_TEXTURE     = 22,
	WINDOWID_MAX                = 23
};

struct Widget;

/**
 * The parameter for a given DrawMode.
 */
typedef union WidgetDrawParameter {
	uint16 unknown;                                         /*!< Parameter for DRAW_MODE_UNKNOWN3. */
	uint16 sprite;                                          /*!< Parameter for Shape_DrawRemap. */
	const char *text;                                       /*!< Parameter for DRAW_MODE_TEXT. */
	void (*proc)(struct Widget *);                          /*!< Parameter for DRAW_MODE_CUSTOM_PROC. */
} WidgetDrawParameter;

typedef bool (ClickProc)(struct Widget *);

/**
 * A Widget as stored in the memory.
 */
typedef struct Widget {
	struct Widget *next;                                    /*!< Next widget in the list. */
	uint16 index;                                           /*!< Index of the widget. */
	uint16 shortcut;                                        /*!< What key triggers this widget. */
	uint16 shortcut2;                                       /*!< What key (also) triggers this widget. */
	uint8  drawModeNormal;                                  /*!< Draw mode when normal. */
	uint8  drawModeSelected;                                /*!< Draw mode when selected. */
	uint8  drawModeDown;                                    /*!< Draw mode when down. */
	struct {
		BIT_U8 requiresClick:1;                             /*!< Requires click. */
		BIT_U8 notused1:1;
		BIT_U8 clickAsHover:1;                              /*!< Click as hover. */
		BIT_U8 invisible:1;                                 /*!< Widget is invisible. */
		BIT_U8 greyWhenInvisible:1;                         /*!< Make the widget grey out when made invisible, instead of making it invisible. */
		BIT_U8 noClickCascade:1;                            /*!< Don't cascade the click event to any other widgets. */
		BIT_U8 loseSelect:1;                                /*!< Lose select when leave. */
		BIT_U8 notused2:1;
		BIT_U8 buttonFilterLeft:4;                          /*!< Left button filter. */
		BIT_U8 buttonFilterRight:4;                         /*!< Right button filter. */
	} flags;                                                /*!< General flags of the Widget. */
	WidgetDrawParameter drawParameterNormal;                /*!< Draw parameter when normal. */
	WidgetDrawParameter drawParameterSelected;              /*!< Draw parameter when selected. */
	WidgetDrawParameter drawParameterDown;                  /*!< Draw parameter when down. */
	uint16 parentID;                                        /*!< Parent window we are nested in. */
	 int16 offsetX;                                         /*!< X position from parent we are at, in pixels. */
	 int16 offsetY;                                         /*!< Y position from parent we are at, in pixels. */
	uint16 width;                                           /*!< Width of widget in pixels. */
	uint16 height;                                          /*!< Height of widget in pixels. */
	uint8  fgColourNormal;                                  /*!< Foreground colour for draw proc when normal. */
	uint8  bgColourNormal;                                  /*!< Background colour for draw proc when normal. */
	uint8  fgColourSelected;                                /*!< Foreground colour for draw proc when selected. */
	uint8  bgColourSelected;                                /*!< Background colour for draw proc when selected. */
	uint8  fgColourDown;                                    /*!< Foreground colour for draw proc when down. */
	uint8  bgColourDown;                                    /*!< Background colour for draw proc when down. */
	struct {
		BIT_U8 selected:1;                                  /*!< Selected. */
		BIT_U8 hover1:1;                                    /*!< Hover. */
		BIT_U8 hover2:1;                                    /*!< Hover. */
		BIT_U8 selectedLast:1;                              /*!< Last Selected. */
		BIT_U8 hover1Last:1;                                /*!< Last Hover. */
		BIT_U8 hover2Last:1;                                /*!< Last Hover. */
		BIT_U8 notused:1;
		BIT_U8 keySelected:1;                               /*!< Key Selected. */
		BIT_U8 buttonState:8;                               /*!< Button state. */
	} state;                                                /*!< State of the Widget. */
	ClickProc *clickProc;                                   /*!< Function to execute when widget is pressed. */
	void *data;                                             /*!< If non-NULL, it points to WidgetScrollbar or HallOfFameData belonging to this widget. */
	uint16 stringID;                                        /*!< Strings to print on the widget. Index above 0xFFF2 are special. */

	enum ScreenDivID div;
} Widget;

typedef void (ScrollbarDrawProc)(Widget *);

/**
 * Scrollbar information as stored in the memory.
 */
typedef struct WidgetScrollbar {
	Widget *parent;                                         /*!< Parent widget we belong to. */
	uint16 size;                                            /*!< Size (in pixels) of the scrollbar. */
	uint16 position;                                        /*!< Current position of the scrollbar. */
	uint16 scrollMax;                                       /*!< Maximum position of the scrollbar cursor. */
	uint16 scrollPageSize;                                  /*!< Amount of elements to scroll per page. */
	uint16 scrollPosition;                                  /*!< Current position of the scrollbar cursor. */
	uint8  pressed;                                         /*!< If non-zero, the scrollbar is currently pressed. */
	uint8  dirty;                                           /*!< If non-zero, the scrollbar is dirty (requires repaint). */
	uint16 pressedPosition;                                 /*!< Position where we clicked on the scrollbar when pressed. */
	ScrollbarDrawProc *drawProc;                            /*!< Draw proc (called on every draw). Can be null. */

	int itemHeight;
} WidgetScrollbar;

/**
 * Static information per WidgetClick type.
 */
typedef struct WindowDesc {
	uint16 index;                                           /*!< Index of the Window. */
	int16 stringID;                                         /*!< String for the Window. */
	bool   addArrows;                                       /*!< If true, arrows are added to the Window. */
	uint8  widgetCount;                                     /*!< Amount of widgets following. */
	struct {
		uint16 stringID;                                    /*!< String of the Widget. */
		uint16 offsetX;                                     /*!< Offset in X-position of the Widget (relative to Window). */
		uint16 offsetY;                                     /*!< Offset in Y-position of the Widget (relative to Window). */
		uint16 width;                                       /*!< Width of the Widget. */
		uint16 height;                                      /*!< Height of the Widget. */
		uint16 labelStringId;                               /*!< Label of the Widget. */
		uint16 shortcut2;                                   /*!< The shortcut to trigger the Widget. */
	} widgets[7];                                           /*!< The Widgets belonging to the Window. */
} WindowDesc;

/** Widget properties. */
typedef struct WidgetProperties {
	uint16 xBase;                                           /*!< Horizontal base coordinate. */
	uint16 yBase;                                           /*!< Vertical base coordinate. */
	uint16 width;                                           /*!< Width of the widget. */
	uint16 height;                                          /*!< Height of the widget. */
	uint8  fgColourBlink;                                   /*!< Foreground colour for 'blink'. */
	uint8  fgColourNormal;                                  /*!< Foreground colour for 'normal'. */
	uint8  fgColourSelected;                                /*!< Foreground colour when 'selected' */
} WidgetProperties;

extern WindowDesc g_optionsWindowDesc;
extern WindowDesc g_gameControlWindowDesc;
extern WindowDesc g_yesNoWindowDesc;
extern WindowDesc g_saveLoadWindowDesc;
extern WindowDesc g_savegameNameWindowDesc;

extern uint8 g_paletteActive[3 * 256];
extern uint8 g_palette1[3 * 256];
extern uint8 g_palette2[3 * 256];
extern uint8 g_paletteMapping1[256];
extern uint8 g_paletteMapping2[256];

extern Widget *g_widgetLinkedListHead;
extern Widget *g_widgetLinkedListTail;
extern Widget *g_widgetInvoiceTail;
extern Widget *g_widgetMentatFirst;
extern Widget *g_widgetMentatTail;
extern Widget *g_widgetMentatScrollUp;
extern Widget *g_widgetMentatScrollDown;
extern Widget *g_widgetMentatScrollbar;

extern WidgetProperties g_widgetProperties[WINDOWID_MAX];
extern uint16 g_curWidgetIndex;
extern uint16 g_curWidgetXBase;
extern uint16 g_curWidgetYBase;
extern uint16 g_curWidgetWidth;
extern uint16 g_curWidgetHeight;
extern uint8  g_curWidgetFGColourBlink;
extern uint8  g_curWidgetFGColourNormal;

extern Widget g_table_windowWidgets[9];


extern Widget *GUI_Widget_GetNext(Widget *w);
extern Widget *GUI_Widget_Get_ByIndex(Widget *w, uint16 index);
extern uint16  GUI_Widget_HandleEvents(Widget *w);
extern void    GUI_Widget_MakeInvisible(Widget *w);
extern void    GUI_Widget_MakeVisible(Widget *w);
extern void    GUI_Widget_Draw(Widget *w);
extern uint8   GUI_Widget_GetShortcut(uint8 c);
extern void GUI_Widget_SetShortcuts(Widget *w);
extern Widget *GUI_Widget_Allocate(uint16 index, uint16 shortcut, uint16 offsetX, uint16 offsetY, uint16 spriteID, uint16 stringID);
extern void    GUI_Widget_MakeNormal(Widget *w, bool clickProc);
extern void    GUI_Widget_MakeSelected(Widget *w, bool clickProc);
extern Widget *GUI_Widget_Link(Widget *w1, Widget *w2);
extern Widget *GUI_Widget_Insert(Widget *w1, Widget *w2);
extern uint16 Widget_SetCurrentWidget(uint16 index);
extern uint16 Widget_SetAndPaintCurrentWidget(uint16 index);
extern void Widget_PaintCurrentWidget(void);

/* viewport.c */
extern void GUI_Widget_Viewport_Draw(bool arg06, bool arg08, bool drawToMainScreen);
extern void GUI_Widget_Viewport_RedrawMap(void);

/* widget_click.c */
extern bool GUI_Widget_SpriteTextButton_Click(Widget *w);
extern bool GUI_Widget_TextButton_Click(Widget *w);
extern bool GUI_Widget_Name_Click(Widget *w);
extern bool GUI_Widget_Cancel_Click(Widget *w);
extern bool GUI_Widget_Picture_Click(Widget *w);
extern bool GUI_Widget_RepairUpgrade_Click(Widget *w);
extern void GUI_Window_Create(WindowDesc *desc);
extern int GUI_Widget_HOF_ClearList_Click(Widget *w);

/* widget_draw.c */
extern void GUI_Widget_TextButton_Draw(Widget *w);
extern void GUI_Widget_SpriteButton_Draw(Widget *w);
extern void GUI_Widget_SpriteTextButton_Draw(Widget *w);
extern void GUI_Widget_TextButton2_Draw(Widget *w);
extern void GUI_Widget_Scrollbar_Draw(Widget *w);
extern void GUI_Widget_ActionPanel_Draw(bool forceDraw);
extern void GUI_Widget_DrawBorder(uint16 widgetIndex, uint16 borderType, bool pressed);
extern void GUI_Widget_DrawWindow(const WindowDesc *desc);
extern void GUI_Widget_DrawAll(Widget *w);
extern void GUI_Widget_Savegame_Draw(uint16 key);

#endif /* GUI_WIDGET_H */
