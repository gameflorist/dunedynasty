/* audio.c */

#include <assert.h>
#include <stdio.h>

#include "audio.h"

#include "audio_a5.h"
#include "../config.h"
#include "../file.h"
#include "../opendune.h"

static enum HouseType s_curr_sample_set = HOUSE_INVALID;

static char
Audio_GetSamplePrefix(enum HouseType houseID)
{
	switch (g_config.language) {
		case LANGUAGE_FRENCH:
			return 'F';

		case LANGUAGE_GERMAN:
			return 'G';

		default:
			if (houseID < HOUSE_MAX)
				return g_table_houseInfo[houseID].prefixChar;
			break;
	}

	return 'Z';
}

static void
Audio_LoadSample(const char *filename, enum SampleID sampleID)
{
	if (filename == NULL || !File_Exists(filename))
		return;

	const uint8 file_index = File_Open(filename, 1);
	const uint32 file_size = File_GetSize(file_index);

	AudioA5_StoreSample(sampleID, file_index, file_size);
	File_Close(file_index);
}

static void
Audio_LoadSampleForHouse(enum HouseType houseID, enum SampleID sampleID)
{
	const SoundData *s = &g_table_voices[sampleID];
	const char *filename;
	char buf[16];

	/* [+-/?]FILENAME. */
	filename = s->string + 1;
	switch (s->string[0]) {
		case '+':
			/* +: common to all houses. */
			if (s_curr_sample_set != HOUSE_INVALID)
				return;

			/* +%c: common to all houses, substitue with language prefix. */
			if (s->string[1] == '%') {
				char prefix = Audio_GetSamplePrefix(HOUSE_INVALID);
				snprintf(buf, sizeof(buf), s->string + 1, prefix);
				filename = buf;
			}
			break;

		case '-':
			/* -: common to all houses. */
			if (s_curr_sample_set != HOUSE_INVALID)
				return;
			break;

		case '/':
			/* /: mercenary only. */
			if (houseID != HOUSE_MERCENARY)
				return;
			break;

		case '?':
			/* ?%c: load as required, substitute with house or language prefix. */
			if (s->string[1] == '%') {
				char prefix = Audio_GetSamplePrefix(houseID);
				snprintf(buf, sizeof(buf), s->string + 1, prefix);
				filename = buf;
			}
			break;

		case '%':
			/* %c: substitute with house or language prefix. */
			{
				char prefix = Audio_GetSamplePrefix(houseID);
				snprintf(buf, sizeof(buf), s->string, prefix);
				filename = buf;
			}
			break;

		default:
			return;
	}

	Audio_LoadSample(filename, sampleID);
}

void
Audio_LoadSampleSet(enum HouseType houseID)
{
	if (s_curr_sample_set == houseID)
		return;

	for (enum SampleID sampleID = 0; sampleID < SAMPLEID_MAX; sampleID++) {
		Audio_LoadSampleForHouse(houseID, sampleID);
	}

	s_curr_sample_set = houseID;
}
