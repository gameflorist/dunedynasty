/* config_a5.c */

#include <assert.h>
#include <allegro5/allegro.h>
#include <stdio.h>
#include "buildcfg.h"
#include "os/math.h"

#include "config.h"

#include "audio/audio.h"
#include "enhancement.h"
#include "file.h"
#include "gfx.h"
#include "opendune.h"
#include "string.h"

#define CONFIG_FILENAME "dunedynasty.cfg"

typedef struct GameOption {
	const char *section;
	const char *key;

	enum OptionType {
		CONFIG_BOOL,
		CONFIG_FLOAT,
		CONFIG_INT,
		CONFIG_INT_0_4,
		CONFIG_INT_1_10,
		CONFIG_LANGUAGE,
		CONFIG_STRING,
		CONFIG_WINDOW
	} type;

	union {
		bool *_bool;
		unsigned int *_language;
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
	5,      /* autoScrollDelay */
};

/*--------------------------------------------------------------*/

static const GameOption s_game_option[] = {
	{ "game",   "screen_width",     CONFIG_INT,     .d._int = &TRUE_DISPLAY_WIDTH },
	{ "game",   "screen_height",    CONFIG_INT,     .d._int = &TRUE_DISPLAY_HEIGHT },
	{ "game",   "window_mode",      CONFIG_WINDOW,  .d._window = &g_gameConfig.windowMode },
	{ "game",   "language",         CONFIG_LANGUAGE,.d._language = &g_gameConfig.language },
	{ "game",   "game_speed",       CONFIG_INT_0_4, .d._int = &g_gameConfig.gameSpeed },
	{ "game",   "hints",            CONFIG_BOOL,    .d._bool = &g_gameConfig.hints },
	{ "game",   "auto_scroll",      CONFIG_BOOL,    .d._bool = &g_gameConfig.autoScroll },
	{ "game",   "auto_scroll_delay",CONFIG_INT_1_10,.d._int = &g_gameConfig.autoScrollDelay },

	{ "audio",  "enable_music",     CONFIG_BOOL,    .d._bool = &g_enable_music },
	{ "audio",  "enable_effects",   CONFIG_BOOL,    .d._bool = &g_enable_effects },
	{ "audio",  "enable_sound",     CONFIG_BOOL,    .d._bool = &g_enable_sounds },
	{ "audio",  "enable_voice",     CONFIG_BOOL,    .d._bool = &g_enable_voices },
	{ "audio",  "enable_subtitles", CONFIG_BOOL,    .d._bool = &g_enable_subtitles },
	{ "audio",  "music_volume",     CONFIG_FLOAT,   .d._float = &music_volume },
	{ "audio",  "sound_volume",     CONFIG_FLOAT,   .d._float = &sound_volume },
	{ "audio",  "voice_volume",     CONFIG_FLOAT,   .d._float = &voice_volume },
	{ "audio",  "sound_font",       CONFIG_STRING,  .d._string = sound_font_path },

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
Config_GetMusicVolume(ALLEGRO_CONFIG *config, const char *category, const char *key, bool enable, ExtMusicInfo *ext)
{
	if (!ext->enable)
		return;

	const char *str = al_get_config_value(config, category, key);
	if (str == NULL)
		return;

	Config_GetFloat(str, 0.0f, 2.0f, &ext->volume);

	if (ext->volume <= 0.0f)
		ext->enable = false;

	if (!enable)
		ext->enable = false;
}

/*--------------------------------------------------------------*/

static void
ConfigA5_InitDataDirectories(void)
{
	ALLEGRO_PATH *dune_data_path = al_get_standard_path(ALLEGRO_RESOURCES_PATH);
	ALLEGRO_PATH *user_data_path = al_get_standard_path(ALLEGRO_USER_SETTINGS_PATH);
	const char *data_path_cstr = al_path_cstr(dune_data_path, ALLEGRO_NATIVE_PATH_SEP);
	const char *user_path_cstr = al_path_cstr(user_data_path, ALLEGRO_NATIVE_PATH_SEP);
	char filename[1024];

	/* Find global data directory. */
	snprintf(g_dune_data_dir, sizeof(g_dune_data_dir), data_path_cstr);
	File_MakeCompleteFilename(filename, sizeof(filename), "dune.pak", true);
	if (!al_filename_exists(filename)) {
		snprintf(g_dune_data_dir, sizeof(g_dune_data_dir), DUNE_DATA_DIR);
	}

	/* Find personal directory, and create subdirectories. */
	snprintf(g_personal_data_dir, sizeof(g_personal_data_dir), user_path_cstr);
	File_MakeCompleteFilename(filename, sizeof(filename), "", false);
	if (!al_make_directory(filename)) {
		fprintf(stderr, "Could not create %s!\n", filename);
	}

	al_destroy_path(dune_data_path);
	al_destroy_path(user_data_path);
	fprintf(stdout, "Dune data directory: %s\n", g_dune_data_dir);
	fprintf(stdout, "Personal data directory: %s\n", g_personal_data_dir);
}

void
GameOptions_Load(void)
{
	char filename[1024];

	ConfigA5_InitDataDirectories();

	snprintf(filename, sizeof(filename), "%s/%s", g_personal_data_dir, CONFIG_FILENAME);
	s_configFile = al_load_config_file(filename);
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

			case CONFIG_FLOAT:
				Config_GetFloat(str, 0.0f, 1.0f, opt->d._float);
				break;

			case CONFIG_INT:
				Config_GetInt(str, 0, INT_MAX, opt->d._int);
				break;

			case CONFIG_INT_0_4:
				Config_GetInt(str, 0, 4, opt->d._int);
				break;

			case CONFIG_INT_1_10:
				Config_GetInt(str, 1, 10, opt->d._int);
				break;

			case CONFIG_LANGUAGE:
				Config_GetLanguage(str, opt->d._language);
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
	for (enum MusicSet music_set = MUSICSET_DUNE2_ADLIB; music_set < NUM_MUSIC_SETS; music_set++) {
		char category[1024];
		bool enable_set = true;

		const char *str = al_get_config_value(s_configFile, "music", g_music_set_prefix[music_set]);
		if (str != NULL)
			Config_GetBool(str, &enable_set);

		snprintf(category, sizeof(category), "music/%s", g_music_set_prefix[music_set]);

		for (enum MusicID musicID = MUSIC_LOSE_ORDOS; musicID < MUSICID_MAX; musicID++) {
			MusicInfoGlob glob[NUM_MUSIC_SETS];
			MusicInfo *m = &g_table_music[musicID];

			Audio_GlobMusicInfo(m, glob);

			if (music_set <= MUSICSET_DUNE2_C55) {
				MidiFileInfo *mid = glob[music_set].mid;

				if (!enable_set) {
					mid->enable = false;
				}
				else {
					char key[1024];

					snprintf(key, sizeof(key), "%s_%d", mid->filename, mid->track);
					str = al_get_config_value(s_configFile, category, key);
					if (str != NULL)
						Config_GetBool(str, &mid->enable);
				}
			}
			else {
				ExtMusicInfo *ext = glob[music_set].ext;
				const char *key = ext->filename + strlen(g_music_set_prefix[music_set]) + 1;

				Config_GetMusicVolume(s_configFile, category, key, enable_set, ext);
			}
		}
	}
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

			case CONFIG_FLOAT:
				Config_SetFloat(s_configFile, opt->section, opt->key, *(opt->d._float));
				break;

			case CONFIG_INT:
			case CONFIG_INT_0_4:
			case CONFIG_INT_1_10:
				Config_SetInt(s_configFile, opt->section, opt->key, *(opt->d._int));
				break;

			case CONFIG_LANGUAGE:
				Config_SetLanguage(s_configFile, opt->section, opt->key, *(opt->d._language));
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
