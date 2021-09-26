/* config_a5.c */

#include <assert.h>
#include <allegro5/allegro.h>
#include <ctype.h>
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
#include "table/locale.h"
#include "video/video.h"

#define CONFIG_FILENAME "dunedynasty.cfg"

typedef struct GameOption {
	const char *section;
	const char *key;

	enum OptionType {
		CONFIG_ASPECT_CORRECTION,
		CONFIG_BOOL,
		CONFIG_CAMPAIGN,
		CONFIG_FLOAT,
		CONFIG_FLOAT_05_2,
		CONFIG_FLOAT_1_6,
		CONFIG_GRAPHICS_DRIVER,
		CONFIG_HEALTH_BAR,
		CONFIG_INT,
		CONFIG_INT_0_4,
		CONFIG_INT_1_16,
		CONFIG_SMOOTH_ANIM,
		CONFIG_SOUND_EFFECTS,
		CONFIG_STRING,
		CONFIG_LANGUAGE,
		CONFIG_MUSIC_PACK,
		CONFIG_SUBTITLE,
		CONFIG_WINDOW_MODE
	} type;

	union {
		bool *_bool;
		int *_int;
		float *_float;
		char *_string;
		enum AspectRatioCorrection *_aspect_correction;
		enum GraphicsDriver *_graphics_driver;
		enum HealthBarMode *_health_bar;
		enum Language *_language;
		enum MusicSet *_music_set;
		enum SmoothUnitAnimationMode *_smooth_anim;
		enum SoundEffectSources *_sound_effects;
		enum SubtitleOverride *_subtitle;
		enum WindowMode *_window_mode;
	} d;
} GameOption;

static ALLEGRO_CONFIG *s_configFile;

GameCfg g_gameConfig = {
#ifdef __PANDORA__
	WM_FULLSCREEN,
#else
	WM_WINDOWED,
#endif

	LANGUAGE_ENGLISH,
	2,      /* gameSpeed */
	true,   /* hints */
	true,   /* autoScroll */
	true,   /* scrollAlongScreenEdge */
	4,      /* scrollSpeed */
	false,  /* leftClickOrders */
	false,  /* holdControlToZoom */
	1.0f,   /* panSensitivity */

#ifdef __PANDORA__
	false,  /* hardwareCursor */
#else
	true,   /* hardwareCursor */
#endif
};

#ifdef __PANDORA__
static int saved_screen_width  = 800;
static int saved_screen_height = 480;
#else
static int saved_screen_width  = 640;
static int saved_screen_height = 480;
#endif

/*--------------------------------------------------------------*/

static const GameOption s_game_option[] = {
	{ "game",   "language",         CONFIG_LANGUAGE,.d._language = &g_gameConfig.language },
	{ "game",   "game_speed",       CONFIG_INT_0_4, .d._int = &g_gameConfig.gameSpeed },
	{ "game",   "hints",            CONFIG_BOOL,    .d._bool = &g_gameConfig.hints },
	{ "game",   "campaign",         CONFIG_CAMPAIGN,.d._int = &g_campaign_selected },

	{ "graphics",   "driver",           CONFIG_GRAPHICS_DRIVER, .d._graphics_driver = &g_graphics_driver },
	{ "graphics",   "window_mode",      CONFIG_WINDOW_MODE,     .d._window_mode = &g_gameConfig.windowMode },
	{ "graphics",   "screen_width",     CONFIG_INT,             .d._int = &saved_screen_width },
	{ "graphics",   "screen_height",    CONFIG_INT,             .d._int = &saved_screen_height },
	{ "graphics",   "correct_aspect_ratio", CONFIG_ASPECT_CORRECTION,   .d._aspect_correction = &g_aspect_correction },
	{ "graphics",   "menubar_scale",    CONFIG_FLOAT_1_6,       .d._float = &g_screenDiv[SCREENDIV_MENUBAR].scalex },
	{ "graphics",   "sidebar_scale",    CONFIG_FLOAT_1_6,       .d._float = &g_screenDiv[SCREENDIV_SIDEBAR].scalex },
	{ "graphics",   "viewport_scale",   CONFIG_FLOAT_1_6,       .d._float = &g_screenDiv[SCREENDIV_VIEWPORT].scalex },
	{ "graphics",   "hardware_cursor",  CONFIG_BOOL,            .d._bool = &g_gameConfig.hardwareCursor },

	{ "controls",   "auto_scroll",              CONFIG_BOOL,    .d._bool = &g_gameConfig.autoScroll },
	{ "controls",   "scroll_speed",             CONFIG_INT_1_16,.d._int = &g_gameConfig.scrollSpeed },
	{ "controls",   "scroll_along_screen_edge", CONFIG_BOOL,    .d._bool = &g_gameConfig.scrollAlongScreenEdge },
	{ "controls",   "left_click_orders",        CONFIG_BOOL,    .d._bool = &g_gameConfig.leftClickOrders },
	{ "controls",   "hold_control_to_zoom",     CONFIG_BOOL,    .d._bool = &g_gameConfig.holdControlToZoom },
	{ "controls",   "pan_sensitivity",          CONFIG_FLOAT_05_2,  .d._float = &g_gameConfig.panSensitivity },

	{ "audio",  "enable_music",     CONFIG_BOOL,    .d._bool = &g_enable_music },
	{ "audio",  "enable_sounds",    CONFIG_SOUND_EFFECTS,   .d._sound_effects = &g_enable_sound_effects },
	{ "audio",  "enable_voices",    CONFIG_BOOL,    .d._bool = &g_enable_voices },
	{ "audio",  "enable_subtitles", CONFIG_BOOL,    .d._bool = &g_enable_subtitles },
	{ "audio",  "music_volume",     CONFIG_FLOAT,   .d._float = &music_volume },
	{ "audio",  "sound_volume",     CONFIG_FLOAT,   .d._float = &sound_volume },
	{ "audio",  "voice_volume",     CONFIG_FLOAT,   .d._float = &voice_volume },
	{ "audio",  "opl_mame",         CONFIG_BOOL,    .d._bool = &g_opl_mame },
	{ "audio",  "sound_font",       CONFIG_STRING,  .d._string = sound_font_path },

	{ "music",  "dune2_adlib",      CONFIG_BOOL,    .d._bool = &g_table_music_set[MUSICSET_DUNE2_ADLIB].enable },
	{ "music",  "dune2_midi",       CONFIG_BOOL,    .d._bool = &g_table_music_set[MUSICSET_DUNE2_MIDI].enable },
	{ "music",  "fluidsynth",       CONFIG_BOOL,    .d._bool = &g_table_music_set[MUSICSET_FLUIDSYNTH].enable },
	{ "music",  "fed2k_mt32",       CONFIG_BOOL,    .d._bool = &g_table_music_set[MUSICSET_SHAIWA_MT32].enable },
	{ "music",  "rcblanke_sc55",    CONFIG_BOOL,    .d._bool = &g_table_music_set[MUSICSET_RCBLANKE_SC55].enable },
	{ "music",  "d2tm_adlib",       CONFIG_BOOL,    .d._bool = &g_table_music_set[MUSICSET_D2TM_ADLIB].enable },
	{ "music",  "d2tm_mt32",        CONFIG_BOOL,    .d._bool = &g_table_music_set[MUSICSET_D2TM_MT32].enable },
	{ "music",  "d2tm_sc55",        CONFIG_BOOL,    .d._bool = &g_table_music_set[MUSICSET_D2TM_SC55].enable },
	{ "music",  "dune2_smd",        CONFIG_BOOL,    .d._bool = &g_table_music_set[MUSICSET_DUNE2_SMD].enable },
	{ "music",  "dune2000",         CONFIG_BOOL,    .d._bool = &g_table_music_set[MUSICSET_DUNE2000].enable },
	{ "music",  "default",          CONFIG_MUSIC_PACK,  .d._music_set = &default_music_pack },

	{ "enhancement",    "brutal_ai",                CONFIG_BOOL,.d._bool = &enhancement_brutal_ai },
	{ "enhancement",    "fog_of_war",               CONFIG_BOOL,.d._bool = &enhancement_fog_of_war },
	{ "enhancement",    "health_bars",              CONFIG_HEALTH_BAR,  .d._health_bar = &enhancement_draw_health_bars },
	{ "enhancement",    "hi_res_overlays",          CONFIG_BOOL,.d._bool = &enhancement_high_res_overlays },
	{ "enhancement",    "true_game_speed",          CONFIG_BOOL,.d._bool = &enhancement_true_game_speed_adjustment },
	{ "enhancement",    "infantry_squad_death_anim",CONFIG_BOOL,.d._bool = &enhancement_infantry_squad_death_animations },
	{ "enhancement",    "insatiable_sandworms",     CONFIG_BOOL,.d._bool = &enhancement_insatiable_sandworms },
	{ "enhancement",    "raise_scenario_unit_cap",  CONFIG_BOOL,.d._bool = &enhancement_raise_scenario_unit_cap },
	{ "enhancement",    "repeat_reinforcements",    CONFIG_BOOL,.d._bool = &enhancement_repeat_reinforcements },
	{ "enhancement",    "smooth_unit_animation",    CONFIG_SMOOTH_ANIM, .d._smooth_anim = &enhancement_smooth_unit_animation },
	{ "enhancement",    "subtitle_override",        CONFIG_SUBTITLE,.d._subtitle = &enhancement_subtitle_override },

	{ NULL, NULL, CONFIG_BOOL, .d._bool = NULL }
};

/*--------------------------------------------------------------*/

static ALLEGRO_CONFIG *
Config_CreateConfigFile(void)
{
	ALLEGRO_CONFIG *config = al_create_config();

	if (config != NULL) {
		al_add_config_comment(config, NULL, "# Dune Dynasty config file");
	}

	return config;
}

static void
Config_GetAspectCorrection(const char *str, enum AspectRatioCorrection *value)
{
	const char *aspect_str = strchr(str, ',');
	const char c = tolower(str[0]);

	     if (c == 'n') { g_pixel_aspect_ratio = 1.0f; *value = ASPECT_RATIO_CORRECTION_NONE; }

	/* menu or partial. */
	else if (c == 'm') { g_pixel_aspect_ratio = 1.1f; *value = ASPECT_RATIO_CORRECTION_PARTIAL; }
	else if (c == 'p') { g_pixel_aspect_ratio = 1.1f; *value = ASPECT_RATIO_CORRECTION_PARTIAL; }

	else if (c == 'f') { g_pixel_aspect_ratio = 1.2f; *value = ASPECT_RATIO_CORRECTION_FULL; }
	else if (c == 'a') { g_pixel_aspect_ratio = 1.1f; *value = ASPECT_RATIO_CORRECTION_AUTO; }

	if (aspect_str != NULL) {
		sscanf(aspect_str + 1, "%f", &g_pixel_aspect_ratio);
		g_pixel_aspect_ratio = clamp(0.5f, g_pixel_aspect_ratio, 2.0f);
	}
}

void
Config_GetCampaign(void)
{
	g_campaign_selected = CAMPAIGNID_DUNE_II;

	if (s_configFile == NULL)
		return;

	/* This needs to be read after all the campaigns have been scanned. */
	const char *str = al_get_config_value(s_configFile, "game", "campaign");
	for (int i = 0; i < g_campaign_total; i++) {
		Campaign *camp = &g_campaign_list[i];

		/* Load the previously selected campaign. */
		if ((str != NULL) && strcmp(str, camp->dir_name) == 0) {
			g_campaign_selected = i;
		}

		/* Load the campaign completion. */
		for (unsigned int h = 0; h < 3; h++) {
			const enum HouseType houseID = camp->house[h];
			char key[256];

			if (houseID == HOUSE_INVALID)
				continue;

			snprintf(key, sizeof(key), "%s%s", camp->dir_name, g_table_houseInfo[houseID].name);
			const char *value = al_get_config_value(s_configFile, "completion", key);

			if (value != NULL)
				sscanf(value, "%u", &camp->completion[h]);
		}
	}

	if (g_campaign_selected == CAMPAIGNID_SKIRMISH)
		g_campaign_selected = CAMPAIGNID_DUNE_II;
}

/* We save the current campaign completion progress at the end of
 * every campaign in case the game is killed.
 */
void
Config_SaveCampaignCompletion(void)
{
	const Campaign *camp = &g_campaign_list[g_campaign_selected];
	char filename[1024];

	if (s_configFile == NULL) {
		s_configFile = Config_CreateConfigFile();
		if (s_configFile == NULL)
			return;
	}

	for (unsigned int h = 0; h < 3; h++) {
		const enum HouseType houseID = camp->house[h];
		char key[256];
		char value[32];

		if (houseID == HOUSE_INVALID)
			continue;

		snprintf(key, sizeof(key), "%s%s", camp->dir_name, g_table_houseInfo[houseID].name);
		snprintf(value, sizeof(value), "%u", camp->completion[h]);
		al_set_config_value(s_configFile, "completion", key, value);
	}

	snprintf(filename, sizeof(filename), "%s/%s", g_personal_data_dir, CONFIG_FILENAME);
	al_save_config_file(filename, s_configFile);
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

	snprintf(str, sizeof(str), "%.2f", value);
	al_set_config_value(config, section, key, str);
}

static void
Config_GetGraphicsDriver(const char *str, enum GraphicsDriver *value)
{
	VARIABLE_NOT_USED(str);

	*value = GRAPHICS_DRIVER_OPENGL;

#ifdef ALLEGRO_WINDOWS
	if (str[0] == 'D' || str[0] == 'd')
		*value = GRAPHICS_DRIVER_DIRECT3D;
#endif
}

static void
Config_SetGraphicsDriver(ALLEGRO_CONFIG *config, const char *section, const char *key, enum GraphicsDriver graphics_driver)
{
	const char *str = "opengl";

	switch (graphics_driver) {
#ifdef ALLEGRO_WINDOWS
		case GRAPHICS_DRIVER_DIRECT3D:
			str = "direct3d";
			break;
#endif

		default:
			break;
	}

	al_set_config_value(config, section, key, str);
}

static void
Config_GetHealthBars(const char *str, enum HealthBarMode *value)
{
	const char c = tolower(str[0]);

	     if (c == 'n') *value = HEALTH_BAR_DISABLE;
	else if (c == 's') *value = HEALTH_BAR_SELECTED_UNITS;
	else if (c == 'a') *value = HEALTH_BAR_ALL_UNITS;
}

static void
Config_SetHealthBars(ALLEGRO_CONFIG *config, const char *section, const char *key, enum HealthBarMode value)
{
	const char *str[] = { "none", "selected", "all" };

	if (value > HEALTH_BAR_ALL_UNITS)
		value = HEALTH_BAR_DISABLE;

	al_set_config_value(config, section, key, str[value]);
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
Config_GetLanguage(const char *str, enum Language *value)
{
	const char c = toupper(str[0]);

	for (enum Language lang = LANGUAGE_ENGLISH; lang < LANGUAGE_MAX; lang++) {
		if (c == g_table_languageInfo[lang].name[0]) {
			*value = lang;
			return;
		}
	}
}

static void
Config_SetLanguage(ALLEGRO_CONFIG *config, const char *section, const char *key, enum Language value)
{
	if (value >= LANGUAGE_MAX)
		value = LANGUAGE_ENGLISH;

	al_set_config_value(config, section, key, g_table_languageInfo[value].name);
}

static void
Config_GetMusicPack(const char *str, enum MusicSet *value)
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
Config_SetMusicPack(ALLEGRO_CONFIG *config, const char *section, const char *key, enum MusicSet value)
{
	if (value >= NUM_MUSIC_SETS)
		value = MUSICSET_DUNE2_ADLIB;

	al_set_config_value(config, section, key, g_table_music_set[value].prefix);
}

static void
Config_SetSmoothAnimation(ALLEGRO_CONFIG *config, const char *section, const char *key, enum SmoothUnitAnimationMode value)
{
	const bool enable = (value != SMOOTH_UNIT_ANIMATION_DISABLE);

	al_set_config_value(config, section, key, enable ? "1" : "0");
}

static void
Config_GetSmoothAnimation(const char *str, enum SmoothUnitAnimationMode *value)
{
	const char c = tolower(str[0]);

	     if (c == '1' || c == 't' || c == 'y') *value = SMOOTH_UNIT_ANIMATION_ENABLE;
	else if (c == '0' || c == 'f' || c == 'n') *value = SMOOTH_UNIT_ANIMATION_DISABLE;
}

static void
Config_GetSoundEffects(const char *str, enum SoundEffectSources *value)
{
	const char c = tolower(str[0]);

	     if (c == 'n') *value = SOUNDEFFECTS_NONE;
	else if (c == 's') *value = SOUNDEFFECTS_SYNTH_ONLY;
	else if (c == 'd') *value = SOUNDEFFECTS_SAMPLES_PREFERRED;
	else if (c == 'b') *value = SOUNDEFFECTS_SYNTH_AND_SAMPLES;
}

static void
Config_SetSoundEffects(ALLEGRO_CONFIG *config, const char *section, const char *key, enum SoundEffectSources value)
{
	const char *str[] = { "none", "synth", "digital", "both" };

	if (value > SOUNDEFFECTS_SYNTH_AND_SAMPLES)
		value = SOUNDEFFECTS_SYNTH_AND_SAMPLES;

	al_set_config_value(config, section, key, str[value]);
}

static void
Config_GetSubtitle(const char *str, enum SubtitleOverride *value)
{
	const char c = tolower(str[0]);

	if (c == 'e') *value = SUBTITLE_THE_BATTLE_FOR_ARRAKIS;
	if (c == 'u') *value = SUBTITLE_THE_BUILDING_OF_UPPER_A_DYNASTY;
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

	Config_GetFloat(str, -2.0f, 2.0f, &ext->volume);

	if (ext->volume > 0.0f) {
		ext->enable |= MUSIC_WANT;
	}
	else {
		ext->volume = -ext->volume;
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
			case CONFIG_ASPECT_CORRECTION:
				Config_GetAspectCorrection(str, opt->d._aspect_correction);
				break;

			case CONFIG_BOOL:
				String_GetBool(str, opt->d._bool);
				break;

			case CONFIG_CAMPAIGN:
				/* Campaign will be read else-where. */
				break;

			case CONFIG_FLOAT:
				Config_GetFloat(str, 0.0f, 1.0f, opt->d._float);
				break;

			case CONFIG_FLOAT_05_2:
				Config_GetFloat(str, 0.5f, 2.0f, opt->d._float);
				break;

			case CONFIG_FLOAT_1_6:
				Config_GetFloat(str, 1.0f, 6.0f, opt->d._float);
				break;

			case CONFIG_GRAPHICS_DRIVER:
				Config_GetGraphicsDriver(str, opt->d._graphics_driver);
				break;

			case CONFIG_HEALTH_BAR:
				Config_GetHealthBars(str, opt->d._health_bar);
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

			case CONFIG_STRING:
				snprintf(opt->d._string, 1024, "%s", str);
				break;

			case CONFIG_LANGUAGE:
				Config_GetLanguage(str, opt->d._language);
				break;

			case CONFIG_MUSIC_PACK:
				Config_GetMusicPack(str, opt->d._music_set);
				break;

			case CONFIG_SMOOTH_ANIM:
				Config_GetSmoothAnimation(str, opt->d._smooth_anim);
				break;

			case CONFIG_SOUND_EFFECTS:
				Config_GetSoundEffects(str, opt->d._sound_effects);
				break;

			case CONFIG_SUBTITLE:
				Config_GetSubtitle(str, opt->d._subtitle);
				break;

			case CONFIG_WINDOW_MODE:
				Config_GetWindowMode(str, opt->d._window_mode);
				break;
		}
	}

	/* Music configuration. */
	for (enum MusicSet music_set = MUSICSET_DUNE2_ADLIB; music_set < NUM_MUSIC_SETS; music_set++) {
		ALLEGRO_CONFIG *config = NULL;
		char category[1024];

		if (music_set <= MUSICSET_FLUIDSYNTH) {
			char filename[1024];

			snprintf(filename, sizeof(filename), "%smusic/music.cfg", g_dune_data_dir);
			config = al_load_config_file(filename);

			snprintf(category, sizeof(category), "%s", g_table_music_set[music_set].prefix);
		}
		else if (g_table_music_set[music_set].enable || music_set == default_music_pack) {
			char filename[1024];

			snprintf(filename, sizeof(filename), "%smusic/%s/volume.cfg", g_dune_data_dir, g_table_music_set[music_set].prefix);
			config = al_load_config_file(filename);
		}

		for (enum MusicID musicID = MUSIC_LOGOS; musicID < MUSICID_MAX; musicID++) {
			MusicList *l = &g_table_music[musicID];

			for (int s = 0; s < l->length; s++) {
				MusicInfo *m = &l->song[s];

				if (m->music_set != music_set)
					continue;

				if (!g_table_music_set[music_set].enable)
					m->enable &=~MUSIC_WANT;

				if (config == NULL)
					continue;

				if (music_set <= MUSICSET_FLUIDSYNTH) {
					char key[1024];
					snprintf(key, sizeof(key), "%s/%d", m->filename, m->track);

					const char *str = al_get_config_value(config, category, key);
					bool want = (m->enable & MUSIC_WANT);
					String_GetBool(str, &want);

					if (want) {
						m->enable |= MUSIC_WANT;
					}
					else {
						m->enable &=~MUSIC_WANT;
					}
				}
				else {
					const char *key = strrchr(m->filename, '/') + 1;
					assert(key != NULL);

					Config_GetMusicVolume(config, "volume", key, m);
				}
			}
		}

		if (config != NULL && config != s_configFile)
			al_destroy_config(config);
	}

	TRUE_DISPLAY_WIDTH = saved_screen_width;
	TRUE_DISPLAY_HEIGHT = saved_screen_height;
}

void
GameOptions_Save(void)
{
	char filename[1024];

	if (s_configFile == NULL) {
		s_configFile = Config_CreateConfigFile();
		if (s_configFile == NULL)
			return;
	}

	for (int i = 0; s_game_option[i].key != NULL; i++) {
		const GameOption *opt = &s_game_option[i];

		switch (opt->type) {
			case CONFIG_BOOL:
				al_set_config_value(s_configFile, opt->section, opt->key, *(opt->d._bool) ? "1" : "0");
				break;

			case CONFIG_CAMPAIGN:
				Config_SetCampaign(s_configFile, opt->section, opt->key, *(opt->d._int));
				break;

			case CONFIG_FLOAT:
			case CONFIG_FLOAT_05_2:
			case CONFIG_FLOAT_1_6:
				Config_SetFloat(s_configFile, opt->section, opt->key, *(opt->d._float));
				break;

			case CONFIG_GRAPHICS_DRIVER:
				Config_SetGraphicsDriver(s_configFile, opt->section, opt->key, *(opt->d._graphics_driver));
				break;

			case CONFIG_HEALTH_BAR:
				Config_SetHealthBars(s_configFile, opt->section, opt->key, *(opt->d._health_bar));
				break;

			case CONFIG_INT:
			case CONFIG_INT_0_4:
			case CONFIG_INT_1_16:
				Config_SetInt(s_configFile, opt->section, opt->key, *(opt->d._int));
				break;

			case CONFIG_STRING:
				al_set_config_value(s_configFile, opt->section, opt->key, opt->d._string);
				break;

			case CONFIG_LANGUAGE:
				Config_SetLanguage(s_configFile, opt->section, opt->key, *(opt->d._language));
				break;

			case CONFIG_MUSIC_PACK:
				Config_SetMusicPack(s_configFile, opt->section, opt->key, *(opt->d._music_set));
				break;

			case CONFIG_SMOOTH_ANIM:
				Config_SetSmoothAnimation(s_configFile, opt->section, opt->key, *(opt->d._smooth_anim));
				break;

			case CONFIG_SOUND_EFFECTS:
				Config_SetSoundEffects(s_configFile, opt->section, opt->key, *(opt->d._sound_effects));
				break;

			case CONFIG_WINDOW_MODE:
				Config_SetWindowMode(s_configFile, opt->section, opt->key, *(opt->d._window_mode));
				break;

			case CONFIG_ASPECT_CORRECTION:
			case CONFIG_SUBTITLE:
				/* Not saved (hidden). */
				break;
		}
	}

	snprintf(filename, sizeof(filename), "%s/%s", g_personal_data_dir, CONFIG_FILENAME);
	al_save_config_file(filename, s_configFile);
}
