/* config_a5.c */

#include <assert.h>
#include <allegro5/allegro.h>
#include <stdio.h>
#include "os/math.h"

#include "config.h"

#include "audio/audio.h"
#include "gfx.h"

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
	} type;

	union {
		bool *_bool;
		int *_int;
		float *_float;
	} d;
} GameOption;

static ALLEGRO_CONFIG *s_configFile;

DuneCfg g_config;

GameCfg g_gameConfig = {
	2,      /* gameSpeed */
	true,   /* hints */
	true,   /* autoScroll */
	5,      /* autoScrollDelay */
};

/*--------------------------------------------------------------*/

static const GameOption s_game_option[] = {
	{ "game",   "screen_width",     CONFIG_INT,     .d._int = &TRUE_DISPLAY_WIDTH },
	{ "game",   "screen_height",    CONFIG_INT,     .d._int = &TRUE_DISPLAY_HEIGHT },
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

/*--------------------------------------------------------------*/

void
GameOptions_Load(void)
{
	s_configFile = al_load_config_file(CONFIG_FILENAME);
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
		}
	}
}

void
GameOptions_Save(void)
{
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
		}
	}

	al_save_config_file(CONFIG_FILENAME, s_configFile);
}
