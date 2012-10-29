/* config_a5.c */

#include <assert.h>
#include <allegro5/allegro.h>
#include <stdio.h>
#include "buildcfg.h"
#include "os/math.h"
#include "os/strings.h"

#include "config.h"

#include "audio/audio.h"
#include "enhancement.h"
#include "file.h"
#include "gfx.h"
#include "opendune.h"
#include "scenario.h"
#include "string.h"

#define CONFIG_FILENAME "dunedynasty.cfg"

typedef struct GameOption {
	const char *section;
	const char *key;

	enum OptionType {
		CONFIG_BOOL,
		CONFIG_CAMPAIGN,
		CONFIG_FLOAT,
		CONFIG_FLOAT_1_3,
		CONFIG_INT,
		CONFIG_INT_0_4,
		CONFIG_INT_1_16,
		CONFIG_LANGUAGE,
		CONFIG_MUSIC_PACK,
		CONFIG_STRING,
		CONFIG_WINDOW
	} type;

	union {
		bool *_bool;
		unsigned int *_uint;
		int *_int;
		float *_float;
		char *_string;
		enum WindowMode *_window;
	} d;
} GameOption;

static ALLEGRO_CONFIG *s_configFile;

GameCfg g_gameConfig = {
	WM_WINDOWED,
	LANGUAGE_ENGLISH,
	2,      /* gameSpeed */
	true,   /* hints */
	true,   /* autoScroll */
	4,      /* scrollSpeed */
	false,  /* leftClickOrders */
	false,  /* holdControlToZoom */
};

static int saved_screen_width = 640;
static int saved_screen_height = 480;

/*--------------------------------------------------------------*/

static const GameOption s_game_option[] = {
	{ "game",   "screen_width",     CONFIG_INT,     .d._int = &saved_screen_width },
	{ "game",   "screen_height",    CONFIG_INT,     .d._int = &saved_screen_height },
	{ "game",   "window_mode",      CONFIG_WINDOW,  .d._window = &g_gameConfig.windowMode },
	{ "game",   "menubar_scale",    CONFIG_FLOAT_1_3,   .d._float = &g_screenDiv[SCREENDIV_MENUBAR].scale },
	{ "game",   "sidebar_scale",    CONFIG_FLOAT_1_3,   .d._float = &g_screenDiv[SCREENDIV_SIDEBAR].scale },
	{ "game",   "viewport_scale",   CONFIG_FLOAT_1_3,   .d._float = &g_screenDiv[SCREENDIV_VIEWPORT].scale },
	{ "game",   "language",         CONFIG_LANGUAGE,.d._uint = &g_gameConfig.language },
	{ "game",   "game_speed",       CONFIG_INT_0_4, .d._int = &g_gameConfig.gameSpeed },
	{ "game",   "hints",            CONFIG_BOOL,    .d._bool = &g_gameConfig.hints },
	{ "game",   "campaign",         CONFIG_CAMPAIGN,.d._int = &g_campaign_selected },
	{ "game",   "auto_scroll",      CONFIG_BOOL,    .d._bool = &g_gameConfig.autoScroll },
	{ "game",   "scroll_speed",     CONFIG_INT_1_16,.d._int = &g_gameConfig.scrollSpeed },
	{ "game",   "scroll_along_screen_edge", CONFIG_BOOL,.d._bool = &enhancement_scroll_along_screen_edge },
	{ "game",   "left_click_orders",        CONFIG_BOOL,.d._bool = &g_gameConfig.leftClickOrders },
	{ "game",   "hold_control_to_zoom",     CONFIG_BOOL,.d._bool = &g_gameConfig.holdControlToZoom },

	{ "audio",  "enable_music",     CONFIG_BOOL,    .d._bool = &g_enable_music },
	{ "audio",  "enable_effects",   CONFIG_BOOL,    .d._bool = &g_enable_effects },
	{ "audio",  "enable_sbsounds",  CONFIG_BOOL,    .d._bool = &g_enable_sounds },
	{ "audio",  "enable_voices",    CONFIG_BOOL,    .d._bool = &g_enable_voices },
	{ "audio",  "enable_subtitles", CONFIG_BOOL,    .d._bool = &g_enable_subtitles },
	{ "audio",  "music_volume",     CONFIG_FLOAT,   .d._float = &music_volume },
	{ "audio",  "sound_volume",     CONFIG_FLOAT,   .d._float = &sound_volume },
	{ "audio",  "voice_volume",     CONFIG_FLOAT,   .d._float = &voice_volume },
	{ "audio",  "opl_mame",         CONFIG_BOOL,    .d._bool = &g_opl_mame },
	{ "audio",  "sound_font",       CONFIG_STRING,  .d._string = sound_font_path },

	{ "music",  "dune2_adlib",      CONFIG_BOOL,    .d._bool = &g_table_music_set[MUSICSET_DUNE2_ADLIB].enable },
	{ "music",  "dune2_c55",        CONFIG_BOOL,    .d._bool = &g_table_music_set[MUSICSET_DUNE2_C55].enable },
	{ "music",  "fed2k_mt32",       CONFIG_BOOL,    .d._bool = &g_table_music_set[MUSICSET_FED2K_MT32].enable },
	{ "music",  "d2tm_adlib",       CONFIG_BOOL,    .d._bool = &g_table_music_set[MUSICSET_D2TM_ADLIB].enable },
	{ "music",  "d2tm_mt32",        CONFIG_BOOL,    .d._bool = &g_table_music_set[MUSICSET_D2TM_MT32].enable },
	{ "music",  "d2tm_sc55",        CONFIG_BOOL,    .d._bool = &g_table_music_set[MUSICSET_D2TM_SC55].enable },
	{ "music",  "dune2_smd",        CONFIG_BOOL,    .d._bool = &g_table_music_set[MUSICSET_DUNE2_SMD].enable },
	{ "music",  "dune2000",         CONFIG_BOOL,    .d._bool = &g_table_music_set[MUSICSET_DUNE2000].enable },
	{ "music",  "default",          CONFIG_MUSIC_PACK,  .d._uint = &default_music_pack },

	{ "enhancement",    "brutal_ai",                CONFIG_BOOL,.d._bool = &enhancement_brutal_ai },
	{ "enhancement",    "insatiable_sandworms",     CONFIG_BOOL,.d._bool = &enhancement_insatiable_sandworms },
	{ "enhancement",    "raise_scenario_unit_cap",  CONFIG_BOOL,.d._bool = &enhancement_raise_scenario_unit_cap },
	{ "enhancement",    "repeat_reinforcements",    CONFIG_BOOL,.d._bool = &enhancement_repeat_reinforcements },

	{ NULL, NULL, CONFIG_BOOL, .d._bool = NULL }
};

/*--------------------------------------------------------------*/

static void
Config_GetBool(const char *str, bool *value)
{
	if (str[0] == '1' || str[0] == 't' || str[0] == 'T' || str[0] == 'y' || str[0] == 'Y') {
		*value = true;
	}
	else {
		*value = false;
	}
}

static void
Config_SetBool(ALLEGRO_CONFIG *config, const char *section, const char *key, bool value)
{
	const char *str = (value == true) ? "1" : "0";

	al_set_config_value(config, section, key, str);
}

void
Config_GetCampaign(void)
{
	g_campaign_selected = 0;

	if (s_configFile == NULL)
		return;

	/* This needs to be read after all the campaigns are scanned. */
	const char *str = al_get_config_value(s_configFile, "game", "campaign");
	if (str == NULL)
		return;

	for (int i = 0; i < g_campaign_total; i++) {
		if (strcmp(str, g_campaign_list[i].dir_name) == 0) {
			g_campaign_selected = i;
			return;
		}
	}
}

static void
Config_SetCampaign(ALLEGRO_CONFIG *config, const char *section, const char *key, int value)
{
	al_set_config_value(config, section, key, g_campaign_list[value].dir_name);
}

static void
Config_GetFloat(const char *str, float min_val, float max_val, float *value)
{
	*value = clamp(min_val, atof(str), max_val);
}

static void
Config_SetFloat(ALLEGRO_CONFIG *config, const char *section, const char *key, float value)
{
	char str[16];

	snprintf(str, sizeof(str), "%f", value);
	al_set_config_value(config, section, key, str);
}

static void
Config_GetInt(const char *str, int min_val, int max_val, int *value)
{
	*value = clamp(min_val, atoi(str), max_val);
}

static void
Config_SetInt(ALLEGRO_CONFIG *config, const char *section, const char *key, int value)
{
	char str[16];

	snprintf(str, sizeof(str), "%d", value);
	al_set_config_value(config, section, key, str);
}

static void
Config_GetLanguage(const char *str, unsigned int *value)
{
	for (unsigned int lang = LANGUAGE_ENGLISH; lang < LANGUAGE_MAX; lang++) {
		const char c_upper = g_languageSuffixes[lang][0];
		const char c_lower = c_upper - 'A' + 'a';

		if (str[0] == c_upper || str[0] == c_lower) {
			*value = lang;
			return;
		}
	}
}

static void
Config_SetLanguage(ALLEGRO_CONFIG *config, const char *section, const char *key, unsigned int value)
{
	if (value >= LANGUAGE_MAX)
		value = LANGUAGE_ENGLISH;

	al_set_config_value(config, section, key, g_languageSuffixes[value]);
}

static void
Config_GetMusicPack(const char *str, unsigned int *value)
{
	for (enum MusicSet set = MUSICSET_DUNE2_ADLIB; set < NUM_MUSIC_SETS; set++) {
		if (strcasecmp(str, g_table_music_set[set].prefix) == 0) {
			*value = set;
			return;
		}
	}

	*value = MUSICSET_DUNE2_ADLIB;
}

static void
Config_SetMusicPack(ALLEGRO_CONFIG *config, const char *section, const char *key, unsigned int value)
{
	if (value >= NUM_MUSIC_SETS)
		value = MUSICSET_DUNE2_ADLIB;

	al_set_config_value(config, section, key, g_table_music_set[value].prefix);
}

static void
Config_GetWindowMode(const char *str, enum WindowMode *value)
{
	/* Anything that's not 'fullscreen': win, window, windowed, etc. */
	if (str[0] != 'f' && str[0] != 'F') {
		*value = WM_WINDOWED;
		return;
	}

	*value = WM_FULLSCREEN;

	/* Anything with 'w': fsw, fullscreen_window, etc. */
	while (*str != '\0') {
		if (*str == 'w' || *str == 'W') {
			*value = WM_FULLSCREEN_WINDOW;
			return;
		}

		if (*str == '#' || *str == ';')
			break;

		str++;
	}
}

static void
Config_SetWindowMode(ALLEGRO_CONFIG *config, const char *section, const char *key, enum WindowMode value)
{
	const char *str[] = { "windowed", "fullscreen", "fullscreenwindow" };

	if (value > WM_FULLSCREEN_WINDOW)
		value = WM_WINDOWED;

	al_set_config_value(config, section, key, str[value]);
}

static void
Config_GetMusicVolume(ALLEGRO_CONFIG *config, const char *category, const char *key, MusicInfo *ext)
{
	const char *str = al_get_config_value(config, category, key);
	if (str == NULL)
		return;

	Config_GetFloat(str, 0.0f, 2.0f, &ext->volume);

	if (ext->volume > 0.0f) {
		ext->enable |= MUSIC_WANT;
	}
	else {
		ext->enable &=~MUSIC_WANT;
	}
}

/*--------------------------------------------------------------*/

static void
ConfigA5_InitDataDirectoriesAndLoadConfigFile(void)
{
	ALLEGRO_PATH *dune_data_path = al_get_standard_path(ALLEGRO_RESOURCES_PATH);
	ALLEGRO_PATH *user_data_path = al_get_standard_path(ALLEGRO_USER_DATA_PATH);
	ALLEGRO_PATH *user_settings_path = al_get_standard_path(ALLEGRO_USER_SETTINGS_PATH);
	const char *dune_data_cstr = al_path_cstr(dune_data_path, ALLEGRO_NATIVE_PATH_SEP);
	const char *user_data_cstr = al_path_cstr(user_data_path, ALLEGRO_NATIVE_PATH_SEP);
	const char *user_settings_cstr = al_path_cstr(user_settings_path, ALLEGRO_NATIVE_PATH_SEP);
	char filename[1024];
	FILE *fp;

	snprintf(g_dune_data_dir, sizeof(g_dune_data_dir), "%s", dune_data_cstr);
	snprintf(g_personal_data_dir, sizeof(g_personal_data_dir), "%s", dune_data_cstr);

	/* Find global data directory.  Test we can read DUNE.PAK. */

	/* 1. Try current executable directory/data/DUNE.PAK. */
	fp = File_Open_CaseInsensitive(SEARCHDIR_GLOBAL_DATA_DIR, "DUNE.PAK", "rb");

	/* 2. Try ~/.local/share/dunedynasty/data/DUNE.PAK. */
	if (fp == NULL) {
		snprintf(g_dune_data_dir, sizeof(g_dune_data_dir), "%s", user_data_cstr);
		fp = File_Open_CaseInsensitive(SEARCHDIR_GLOBAL_DATA_DIR, "DUNE.PAK", "rb");
	}

	/* 3. If /something/bin/dunedynasty, try /something/share/dunedynasty/data/DUNE.PAK. */
	if (fp == NULL) {
		if (strcmp(al_get_path_tail(dune_data_path), "bin") == 0) {
			al_replace_path_component(dune_data_path, -1, "share/dunedynasty");
			dune_data_cstr = al_path_cstr(dune_data_path, ALLEGRO_NATIVE_PATH_SEP);
			snprintf(g_dune_data_dir, sizeof(g_dune_data_dir), "%s", dune_data_cstr);
			fp = File_Open_CaseInsensitive(SEARCHDIR_GLOBAL_DATA_DIR, "DUNE.PAK", "rb");
		}
	}

	/* 4. Try DUNE_DATA_DIR/data/DUNE.PAK. */
	if (fp == NULL) {
		snprintf(g_dune_data_dir, sizeof(g_dune_data_dir), DUNE_DATA_DIR);
	}
	else {
		fclose(fp);
	}

	/* Find personal directory, and create subdirectories. */

	/* 1. Try current executable directory/dunedynasty.cfg. */
	snprintf(filename, sizeof(filename), "%s/%s", g_personal_data_dir, CONFIG_FILENAME);
	s_configFile = al_load_config_file(filename);

	/* 2. Try ~/.config/dunedynasty/dunedynasty.cfg. */
	if (s_configFile == NULL) {
		snprintf(g_personal_data_dir, sizeof(g_personal_data_dir), "%s", user_settings_cstr);
		snprintf(filename, sizeof(filename), "%s/%s", g_personal_data_dir, CONFIG_FILENAME);
		s_configFile = al_load_config_file(filename);
	}

	if (!al_make_directory(g_personal_data_dir)) {
		fprintf(stderr, "Could not create %s!\n", filename);
	}

	al_destroy_path(dune_data_path);
	al_destroy_path(user_data_path);
	al_destroy_path(user_settings_path);
	fprintf(stdout, "Dune data directory: %s\n", g_dune_data_dir);
	fprintf(stdout, "Personal data directory: %s\n", g_personal_data_dir);
}

void
GameOptions_Load(void)
{
	ConfigA5_InitDataDirectoriesAndLoadConfigFile();
	if (s_configFile == NULL)
		return;

	for (int i = 0; s_game_option[i].key != NULL; i++) {
		const GameOption *opt = &s_game_option[i];

		const char *str = al_get_config_value(s_configFile, opt->section, opt->key);
		if (str == NULL)
			continue;

		switch (opt->type) {
			case CONFIG_BOOL:
				Config_GetBool(str, opt->d._bool);
				break;

			case CONFIG_CAMPAIGN:
				/* Campaign will be read else-where. */
				break;

			case CONFIG_FLOAT:
				Config_GetFloat(str, 0.0f, 1.0f, opt->d._float);
				break;

			case CONFIG_FLOAT_1_3:
				Config_GetFloat(str, 1.0f, 3.0f, opt->d._float);
				break;

			case CONFIG_INT:
				Config_GetInt(str, 0, INT_MAX, opt->d._int);
				break;

			case CONFIG_INT_0_4:
				Config_GetInt(str, 0, 4, opt->d._int);
				break;

			case CONFIG_INT_1_16:
				Config_GetInt(str, 1, 16, opt->d._int);
				break;

			case CONFIG_LANGUAGE:
				Config_GetLanguage(str, opt->d._uint);
				break;

			case CONFIG_MUSIC_PACK:
				Config_GetMusicPack(str, opt->d._uint);
				break;

				break;

			case CONFIG_STRING:
				snprintf(opt->d._string, 1024, "%s", str);
				break;

			case CONFIG_WINDOW:
				Config_GetWindowMode(str, opt->d._window);
				break;
		}
	}

	/* Music configuration. */
	for (enum MusicID musicID = MUSIC_LOGOS; musicID < MUSICID_MAX; musicID++) {
		MusicInfo *m = &g_table_music[musicID];

		if (!g_table_music_set[m->music_set].enable) {
			m->enable &=~MUSIC_WANT;
			continue;
		}

		char category[1024];
		snprintf(category, sizeof(category), "music/%s", g_table_music_set[m->music_set].prefix);

		if (m->music_set <= MUSICSET_DUNE2_C55) {
			char key[1024];
			snprintf(key, sizeof(key), "%s_%d", m->filename, m->track);

			const char *str = al_get_config_value(s_configFile, category, key);
			if (str != NULL) {
				bool want;

				Config_GetBool(str, &want);

				if (want) {
					m->enable |= MUSIC_WANT;
				}
				else {
					m->enable &=~MUSIC_WANT;
				}
			}
		}
		else {
			const char *key = strrchr(m->filename, '/') + 1;
			assert(key != NULL);

			Config_GetMusicVolume(s_configFile, category, key, m);
		}
	}

	TRUE_DISPLAY_WIDTH = saved_screen_width;
	TRUE_DISPLAY_HEIGHT = saved_screen_height;
}

void
GameOptions_Save(void)
{
	char filename[1024];

	if (s_configFile == NULL) {
		s_configFile = al_create_config();
		if (s_configFile == NULL)
			return;

		al_add_config_comment(s_configFile, NULL, "# Dune Dynasty config file");
	}

	for (int i = 0; s_game_option[i].key != NULL; i++) {
		const GameOption *opt = &s_game_option[i];

		switch (opt->type) {
			case CONFIG_BOOL:
				Config_SetBool(s_configFile, opt->section, opt->key, *(opt->d._bool));
				break;

			case CONFIG_CAMPAIGN:
				Config_SetCampaign(s_configFile, opt->section, opt->key, *(opt->d._int));
				break;

			case CONFIG_FLOAT:
			case CONFIG_FLOAT_1_3:
				Config_SetFloat(s_configFile, opt->section, opt->key, *(opt->d._float));
				break;

			case CONFIG_INT:
			case CONFIG_INT_0_4:
			case CONFIG_INT_1_16:
				Config_SetInt(s_configFile, opt->section, opt->key, *(opt->d._int));
				break;

			case CONFIG_LANGUAGE:
				Config_SetLanguage(s_configFile, opt->section, opt->key, *(opt->d._uint));
				break;

			case CONFIG_MUSIC_PACK:
				Config_SetMusicPack(s_configFile, opt->section, opt->key, *(opt->d._uint));
				break;

			case CONFIG_STRING:
				al_set_config_value(s_configFile, opt->section, opt->key, opt->d._string);
				break;

			case CONFIG_WINDOW:
				Config_SetWindowMode(s_configFile, opt->section, opt->key, *(opt->d._window));
				break;
		}
	}

	snprintf(filename, sizeof(filename), "%s/%s", g_personal_data_dir, CONFIG_FILENAME);
	al_save_config_file(filename, s_configFile);
}
