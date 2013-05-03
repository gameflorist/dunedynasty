/** @file src/file.c %File access routines. */

/* Use Allegro to create directories. */
#include <allegro5/allegro.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "multichar.h"
#include "types.h"
#include "os/endian.h"
#include "os/error.h"
#include "os/file.h"
#include "os/math.h"
#include "os/strings.h"

#include "file.h"

#include "scenario.h"

#define HASH_SIZE 4093

#define DUNE2_CAMPAIGN_PREFIX   "campaign"
#define DUNE2_DATA_PREFIX       "data"
#define DUNE2_SAVE_PREFIX       "save"

/**
 * Static information about opened files.
 */
typedef struct File {
	FILE *fp;
	uint32 size;
	uint32 start;
	uint32 position;
} File;

static File s_file[FILE_MAX];
static FileInfo s_hash_file[HASH_SIZE];

char g_dune_data_dir[1024];
char g_personal_data_dir[1024];

extern const FileInfo g_table_fileInfo[FILEINFO_MAX];

/*--------------------------------------------------------------*/
/* Simple Hash table. */

static unsigned int
FileHash_djb2(const char *str)
{
	unsigned long hash = 5381;
	char c;

	while ((c = *str) != '\0') {
		if ('A' <= c && c <= 'Z')
			c += 'a' - 'A';

		str++;
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
	}

	return hash % HASH_SIZE;
}

FileInfo *
FileHash_Store(const char *key)
{
	unsigned int idx = FileHash_djb2(key);

	while (s_hash_file[idx].filename != NULL)
		idx = (idx + 1) % HASH_SIZE;

	return &s_hash_file[idx];
}

unsigned int
FileHash_FindIndex(const char *key)
{
	unsigned int idx = FileHash_djb2(key);

	while (s_hash_file[idx].filename != NULL) {
		if (strcasecmp(key, s_hash_file[idx].filename) == 0)
			return idx;

		idx = (idx + 1) % HASH_SIZE;
	}

	return -1;
}

static FileInfo *
FileHash_Find(const char *key)
{
	const unsigned int idx = FileHash_FindIndex(key);

	return (idx < HASH_SIZE) ? &s_hash_file[idx] : NULL;
}

void
FileHash_Init(void)
{
	for (unsigned int i = 0; i < HASH_SIZE; i++) {
		s_hash_file[i].filename = NULL;
	}

	for (unsigned int i = 0; i < FILEINFO_MAX; i++) {
		FileInfo *fi = FileHash_Store(g_table_fileInfo[i].filename);

		memcpy(fi, &g_table_fileInfo[i], sizeof(g_table_fileInfo[i]));

		fi->parentIndex = FileHash_FindIndex(g_table_fileInfo[g_table_fileInfo[i].parentIndex].filename);
	}
}

/*--------------------------------------------------------------*/

/**
 * Read a uint32 value from a little endian file.
 */
bool fread_le_uint32(uint32 *value, FILE *stream)
{
	uint8 buffer[4];
	if (value == NULL) return false;
	if (fread(buffer, 1, 4, stream) != 4) return false;
	*value = READ_LE_UINT32(buffer);
	return true;
}

/**
 * Read a uint16 value from a little endian file.
 */
bool fread_le_uint16(uint16 *value, FILE *stream)
{
	uint8 buffer[2];
	if (value == NULL) return false;
	if (fread(buffer, 1, 2, stream) != 2) return false;
	*value = READ_LE_UINT16(buffer);
	return true;
}

/**
 * Write a uint32 value from a little endian file.
 */
bool fwrite_le_uint32(uint32 value, FILE *stream)
{
	if (putc(value & 0xff, stream) == EOF) return false;
	if (putc((value >> 8) & 0xff, stream) == EOF) return false;
	if (putc((value >> 16) & 0xff, stream) == EOF) return false;
	if (putc((value >> 24) & 0xff, stream) == EOF) return false;
	return true;
}

/**
 * Write a uint16 value from a little endian file.
 */
bool fwrite_le_uint16(uint16 value, FILE *stream)
{
	if (putc(value & 0xff, stream) == EOF) return false;
	if (putc((value >> 8) & 0xff, stream) == EOF) return false;
	return true;
}

/*--------------------------------------------------------------*/

void
File_MakeCompleteFilename(char *buf, size_t len, enum SearchDirectory dir, const char *filename, bool convert_to_lowercase)
{
	int i = 0;

	if (dir == SEARCHDIR_GLOBAL_DATA_DIR) {
		i = snprintf(buf, len, "%s/%s/", g_dune_data_dir, DUNE2_DATA_PREFIX);
	}
	else if (dir == SEARCHDIR_CAMPAIGN_DIR) {
		if (strchr(filename, '/') == NULL) { /* Dune II */
			i = snprintf(buf, len, "%s/%s/", g_dune_data_dir, DUNE2_DATA_PREFIX);
		}
		else {
			i = snprintf(buf, len, "%s/%s/", g_dune_data_dir, DUNE2_CAMPAIGN_PREFIX);
		}
	}
	else if (dir == SEARCHDIR_PERSONAL_DATA_DIR) {
		i = snprintf(buf, len, "%s/%s/%s", g_personal_data_dir, DUNE2_SAVE_PREFIX, g_campaign_list[g_campaign_selected].dir_name);
	}

	strncpy(buf + i, filename, len - i);
	buf[len - 1] = '\0';

	if (convert_to_lowercase) {
		for (int j = len - 2; j >= i; j--) {
			if (buf[j] == '/' || buf[j] == '\\')
				break;

			if ('A' <= buf[j] && buf[j] <= 'Z')
				buf[j] = buf[j] + 'a' - 'A';
		}
	}
}

FILE *
File_Open_CaseInsensitive(enum SearchDirectory dir, const char *filename, const char *mode)
{
	char buf[1024];
	FILE *fp;

	/* Create directories. */
	if (dir == SEARCHDIR_PERSONAL_DATA_DIR && mode[0] == 'w') {
		File_MakeCompleteFilename(buf, sizeof(buf), dir, "", false);
		if (!al_make_directory(buf))
			return NULL;
	}

	File_MakeCompleteFilename(buf, sizeof(buf), dir, filename, false);
	fp = fopen(buf, mode);
	if (fp != NULL)
		return fp;

#ifndef ALLEGRO_WINDOWS
	/* Attempt lower-case on case-sensitive filesystems. */
	File_MakeCompleteFilename(buf, sizeof(buf), dir, filename, true);
	fp = fopen(buf, mode);
#endif

	return fp;
}

#if 0
/**
 * Find the FileInfo index for the given filename.
 *
 * @param filename The filename to get the index for.
 * @return The index or 0xFFFF if not found.
 */
static uint16 FileInfo_FindIndex_ByName(const char *filename)
{
	uint16 index;

	for (index = 0; index < FILEINFO_MAX; index++) {
		if (!strcasecmp(g_table_fileInfo[index].filename, filename)) return index;
	}

	return FILEINFO_INVALID;
}
#endif

/**
 * Internal function to truly open a file.
 *
 * @param filename The name of the file to open.
 * @param mode The mode to open the file in. Bit 1 means reading, bit 2 means writing.
 * @return An index value refering to the opened file, or FILE_INVALID.
 */
static uint8
_File_OpenInDir(enum SearchDirectory dir, const char *filename, uint8 mode)
{
	const char *mode_str = (mode == FILE_MODE_WRITE) ? "wb" : ((mode == FILE_MODE_READ_WRITE) ? "wb+" : "rb");

	const char *pakName;
	uint8 fileIndex;

	if ((mode & FILE_MODE_READ_WRITE) == 0) return FILE_INVALID;

	/* Find a free spot in our limited array */
	for (fileIndex = 0; fileIndex < FILE_MAX; fileIndex++) {
		if (s_file[fileIndex].fp == NULL) break;
	}
	if (fileIndex == FILE_MAX) return FILE_INVALID;

	/* Check if we can find the file outside any PAK file */
	s_file[fileIndex].fp = File_Open_CaseInsensitive(dir, filename, mode_str);
	if (s_file[fileIndex].fp != NULL) {
		s_file[fileIndex].start    = 0;
		s_file[fileIndex].position = 0;
		s_file[fileIndex].size     = 0;

		/* We can only check the size of the file if we are reading (or appending) */
		if ((mode & FILE_MODE_READ) != 0) {
			fseek(s_file[fileIndex].fp, 0, SEEK_END);
			s_file[fileIndex].size = ftell(s_file[fileIndex].fp);
			fseek(s_file[fileIndex].fp, 0, SEEK_SET);
		}

		return fileIndex;
	}

	/* We never allow writing of files inside PAKs */
	if ((mode & FILE_MODE_WRITE) != 0) return FILE_INVALID;

	/* Check if the file could be inside any of our PAK files */
	FileInfo *fileInfoIndex = FileHash_Find(filename);
	if (fileInfoIndex == NULL) return FILE_INVALID;

	/* If the file is not inside another PAK, then the file doesn't exist (as it wasn't in the directory either) */
	if (!fileInfoIndex->flags.inPAKFile) return FILE_INVALID;

	pakName = s_hash_file[fileInfoIndex->parentIndex].filename;
	s_file[fileIndex].fp = File_Open_CaseInsensitive(dir, pakName, "rb");
	if (s_file[fileIndex].fp == NULL) return FILE_INVALID;

	/* If this file is not yet read from the PAK, read the complete index
	 *  of the PAK and index all files */
	if (!fileInfoIndex->flags.isLoaded) {
		FileInfo *pakIndexLast = NULL;

		while (true) {
			char pakFilename[1024];
			uint32 pakPosition;
			uint16 i;

			if (fread(&pakPosition, sizeof(uint32), 1, s_file[fileIndex].fp) != 1) {
				fclose(s_file[fileIndex].fp);
				s_file[fileIndex].fp = NULL;
				return FILE_INVALID;
			}
			if (pakPosition == 0) break;

			/* Add campaign directory (and slash) to filename. */
			if ((dir == SEARCHDIR_CAMPAIGN_DIR) && (g_campaign_selected != CAMPAIGNID_DUNE_II)) {
				i = snprintf(pakFilename, sizeof(pakFilename), "%s", g_campaign_list[g_campaign_selected].dir_name);
			}
			else {
				i = 0;
			}

			/* Read the name of the file inside the PAK */
			for (; i < sizeof(pakFilename); i++) {
				if (fread(&pakFilename[i], 1, 1, s_file[fileIndex].fp) != 1) {
					fclose(s_file[fileIndex].fp);
					s_file[fileIndex].fp = NULL;
					return FILE_INVALID;
				}
				if (pakFilename[i] == '\0') break;

				/* We always work in lowercase */
				if (pakFilename[i] >= 'A' && pakFilename[i] <= 'Z') pakFilename[i] += 32;
			}
			if (i == sizeof(pakFilename)) {
				fclose(s_file[fileIndex].fp);
				s_file[fileIndex].fp = NULL;
				return FILE_INVALID;
			}

			/* Check if we expected this file in this PAK */
			FileInfo *pakIndex = FileHash_Find(pakFilename);
			if (pakIndex == NULL) continue;
			if (pakIndex->parentIndex != fileInfoIndex->parentIndex) continue;

			/* Update the information of the file */
			pakIndex->flags.isLoaded = true;
			pakIndex->filePosition = pakPosition;
			if (pakIndexLast != NULL)
				pakIndexLast->fileSize = pakPosition - pakIndexLast->filePosition;

			pakIndexLast = pakIndex;
		}

		/* Make sure we set the right size of the last entry */
		if (pakIndexLast != NULL) {
			fseek(s_file[fileIndex].fp, 0, SEEK_END);
			pakIndexLast->fileSize = ftell(s_file[fileIndex].fp) - pakIndexLast->filePosition;
		}
	}

	/* Check if the file is inside the PAK file */
	if (!fileInfoIndex->flags.isLoaded) {
		fclose(s_file[fileIndex].fp);
		s_file[fileIndex].fp = NULL;
		return FILE_INVALID;
	}

	s_file[fileIndex].start    = fileInfoIndex->filePosition;
	s_file[fileIndex].position = 0;
	s_file[fileIndex].size     = fileInfoIndex->fileSize;

	/* Go to the start of the file now */
	fseek(s_file[fileIndex].fp, s_file[fileIndex].start, SEEK_SET);
	return fileIndex;
}

static uint8
_File_Open(enum SearchDirectory dir, const char *filename, uint8 mode)
{
	uint8 ret;

	/* Try campaign file. */
	if (dir == SEARCHDIR_CAMPAIGN_DIR) {
		if (g_campaign_selected != CAMPAIGNID_DUNE_II) {
			char buf[1024];

			snprintf(buf, sizeof(buf), "%s%s", g_campaign_list[g_campaign_selected].dir_name, filename);
			ret = _File_OpenInDir(dir, buf, mode);
			if (ret != FILE_INVALID)
				return ret;
		}

		dir = SEARCHDIR_GLOBAL_DATA_DIR;
	}

	ret = _File_OpenInDir(dir, filename, mode);
	return ret;
}

/**
 * Check if a file exists either in a PAK or on the disk.
 *
 * @param filename The filename to check for.
 * @return True if and only if the file can be found.
 */
bool
File_Exists_Ex(enum SearchDirectory dir, const char *filename)
{
	uint8 index;

	index = _File_Open(dir, filename, FILE_MODE_READ);
	if (index == FILE_INVALID) {
		return false;
	}
	File_Close(index);

	return true;
}

/**
 * Open a file for reading/writing/appending.
 *
 * @param filename The name of the file to open.
 * @param mode The mode to open the file in. Bit 1 means reading, bit 2 means writing.
 * @return An index value refering to the opened file, or FILE_INVALID.
 */
uint8
File_Open_Ex(enum SearchDirectory dir, const char *filename, uint8 mode)
{
	uint8 res;

	res = _File_Open(dir, filename, mode);

	if (res == FILE_INVALID) {
		Error("Unable to open file '%s'.\n", filename);
		exit(1);
	}

	return res;
}

/**
 * Close an opened file.
 *
 * @param index The index given by File_Open() of the file.
 */
void File_Close(uint8 index)
{
	if (index >= FILE_MAX) return;
	if (s_file[index].fp == NULL) return;

	fclose(s_file[index].fp);
	s_file[index].fp = NULL;
}

/**
 * Read bytes from a file into a buffer.
 *
 * @param index The index given by File_Open() of the file.
 * @param buffer The buffer to read into.
 * @param length The amount of bytes to read.
 * @return The amount of bytes truly read, or 0 if there was a failure.
 */
uint32 File_Read(uint8 index, void *buffer, uint32 length)
{
	if (index >= FILE_MAX) return 0;
	if (s_file[index].fp == NULL) return 0;
	if (s_file[index].position >= s_file[index].size) return 0;
	if (length == 0) return 0;

	if (length > s_file[index].size - s_file[index].position) length = s_file[index].size - s_file[index].position;

	if (fread(buffer, length, 1, s_file[index].fp) != 1) {
		Error("Read error\n");
		File_Close(index);

		length = 0;
	}

	s_file[index].position += length;
	return length;
}

/**
 * Read a 16bit unsigned from the file (written on disk in Little endian)
 *
 * @param index The index given by File_Open() of the file.
 * @return The integer read.
 */
uint16 File_Read_LE16(uint8 index)
{
	uint8 buffer[2];
	File_Read(index, buffer, sizeof(buffer));
	return READ_LE_UINT16(buffer);
}

/**
 * Read a 32bit unsigned from the file (written on disk in Little endian)
 *
 * @param index The index given by File_Open() of the file.
 * @return The integer read.
 */
uint32 File_Read_LE32(uint8 index)
{
	uint8 buffer[4];
	File_Read(index, buffer, sizeof(buffer));
	return READ_LE_UINT32(buffer);
}

/**
 * Write bytes from a buffer to a file.
 *
 * @param index The index given by File_Open() of the file.
 * @param buffer The buffer to write from.
 * @param length The amount of bytes to write.
 * @return The amount of bytes truly written, or 0 if there was a failure.
 */
uint32 File_Write(uint8 index, void *buffer, uint32 length)
{
	if (index >= FILE_MAX) return 0;
	if (s_file[index].fp == NULL) return 0;

	if (fwrite(buffer, length, 1, s_file[index].fp) != 1) {
		Error("Write error\n");
		File_Close(index);

		length = 0;
	}

	s_file[index].position += length;
	if (s_file[index].position > s_file[index].size) s_file[index].size = s_file[index].position;
	return length;
}

/**
 * Seek inside a file.
 *
 * @param index The index given by File_Open() of the file.
 * @param position Position to fix to.
 * @param mode Mode of seeking. 0 = SEEK_SET, 1 = SEEK_CUR, 2 = SEEK_END.
 * @return The new position inside the file, relative from the start.
 */
uint32 File_Seek(uint8 index, uint32 position, uint8 mode)
{
	if (index >= FILE_MAX) return 0;
	if (s_file[index].fp == NULL) return 0;
	if (mode > 2) { File_Close(index); return 0; }

	switch (mode) {
		case 0:
			fseek(s_file[index].fp, s_file[index].start + position, SEEK_SET);
			s_file[index].position = position;
			break;
		case 1:
			fseek(s_file[index].fp, (int32)position, SEEK_CUR);
			s_file[index].position += (int32)position;
			break;
		case 2:
			fseek(s_file[index].fp, s_file[index].start + s_file[index].size - position, SEEK_SET);
			s_file[index].position = s_file[index].size - position;
			break;
	}

	return s_file[index].position;
}

/**
 * Get the size of a file.
 *
 * @param index The index given by File_Open() of the file.
 * @return The size of the file.
 */
uint32 File_GetSize(uint8 index)
{
	if (index >= FILE_MAX) return 0;
	if (s_file[index].fp == NULL) return 0;

	return s_file[index].size;
}

/**
 * Delete a file from the disk.
 *
 * @param filename The filename to remove.
 */
void
File_Delete_Personal(const char *filename)
{
	char filenameComplete[1024];

	File_MakeCompleteFilename(filenameComplete, sizeof(filenameComplete), SEARCHDIR_PERSONAL_DATA_DIR, filename, false);
	if (unlink(filenameComplete) == 0)
		return;

	File_MakeCompleteFilename(filenameComplete, sizeof(filenameComplete), SEARCHDIR_PERSONAL_DATA_DIR, filename, true);
	unlink(filenameComplete);
}

/**
 * Reads length bytes from filename into buffer.
 *
 * @param filename Then name of the file to read.
 * @param buffer The buffer to read into.
 * @param length The amount of bytes to read.
 * @return The amount of bytes truly read, or 0 if there was a failure.
 */
uint32
File_ReadBlockFile_Ex(enum SearchDirectory dir, const char *filename, void *buffer, uint32 length)
{
	uint8 index;

	index = File_Open_Ex(dir, filename, FILE_MODE_READ);
	length = File_Read(index, buffer, length);
	File_Close(index);
	return length;
}

/**
 * Reads the whole file in the memory.
 *
 * @param filename The name of the file to open.
 * @param mallocFlags The type of memory to allocate.
 * @return The pointer to allocated memory where the file has been read.
 */
void *
File_ReadWholeFile_Ex(enum SearchDirectory dir, const char *filename)
{
	uint8 index;
	uint32 length;
	void *buffer;

	index = File_Open_Ex(dir, filename, FILE_MODE_READ);
	length = File_GetSize(index);

	buffer = malloc(length + 1);
	File_Read(index, buffer, length);

	/* In case of text-files it can be very important to have a \0 at the end */
	((char *)buffer)[length] = '\0';

	File_Close(index);

	return buffer;
}

/**
 * Reads the whole file in the memory. The file should contain little endian
 * 16bits unsigned integers. It is converted to host byte ordering if needed.
 *
 * @param filename The name of the file to open.
 * @param mallocFlags The type of memory to allocate.
 * @return The pointer to allocated memory where the file has been read.
 */
uint16 *File_ReadWholeFileLE16(const char *filename)
{
	uint8 index;
	uint32 count;
	uint16 *buffer;
#if __BYTE_ORDER == __BIG_ENDIAN
	uint32 i;
#endif

	index = File_Open(filename, FILE_MODE_READ);
	count = File_GetSize(index) / sizeof(uint16);

	buffer = malloc(count * sizeof(uint16));
	if (File_Read(index, buffer, count * sizeof(uint16)) != count * sizeof(uint16)) {
		free(buffer);
		return NULL;
	}

	File_Close(index);

#if __BYTE_ORDER == __BIG_ENDIAN
	for(i = 0; i < count; i++) {
		buffer[i] = LETOH16(buffer[i]);
	}
#endif

	return buffer;
}

/**
 * Reads the whole file into buffer.
 *
 * @param filename The name of the file to open.
 * @param buf The buffer to read into.
 * @return The length of the file.
 */
uint32
File_ReadFile_Ex(enum SearchDirectory dir, const char *filename, void *buf)
{
	uint8 index;
	uint32 length;

	index = File_Open_Ex(dir, filename, FILE_MODE_READ);
	length = File_Seek(index, 0, 2);
	File_Seek(index, 0, 0);
	File_Read(index, buf, length);
	File_Close(index);

	return length;
}

/**
 * Open a chunk file (starting with FORM) for reading.
 *
 * @param filename The name of the file to open.
 * @return An index value refering to the opened file, or FILE_INVALID.
 */
uint8
ChunkFile_Open_Ex(enum SearchDirectory dir, const char *filename)
{
	uint8 index;
	uint32 header;

	/* XXX: what is with this? */
	/* index = File_Open_Ex(dir, filename, FILE_MODE_READ); */
	/* File_Close(index); */

	index = File_Open_Ex(dir, filename, FILE_MODE_READ);

	File_Read(index, &header, 4);

	if (header != HTOBE32(CC_FORM)) {
		File_Close(index);
		return FILE_INVALID;
	}

	File_Seek(index, 4, 1);

	return index;
}

/**
 * Close an opened chunk file.
 *
 * @param index The index given by ChunkFile_Open() of the file.
 */
void ChunkFile_Close(uint8 index)
{
	if (index == FILE_INVALID) return;

	File_Close(index);
}

/**
 * Seek to the given chunk inside a chunk file.
 *
 * @param index The index given by ChunkFile_Open() of the file.
 * @param chunk The chunk to seek to.
 * @return The length of the chunk (0 if not found).
 */
uint32 ChunkFile_Seek(uint8 index, uint32 chunk)
{
	uint32 value = 0;
	uint32 length = 0;
	bool first = true;

	while (true) {
		if (File_Read(index, &value, 4) != 4 && !first) return 0;

		if (value == 0 && File_Read(index, &value, 4) != 4 && !first) return 0;

		if (File_Read(index, &length, 4) != 4 && !first) return 0;

		length = HTOBE32(length);

		if (value == chunk) {
			File_Seek(index, -8, 1);
			return length;
		}

		if (first) {
			File_Seek(index, 12, 0);
			first = false;
			continue;
		}

		length += 1;
		length &= 0xFFFFFFFE;
		File_Seek(index, length, 1);
	}
}

/**
 * Read bytes from a chunk file into a buffer.
 *
 * @param index The index given by ChunkFile_Open() of the file.
 * @param chunk The chunk to read from.
 * @param buffer The buffer to read into.
 * @param length The amount of bytes to read.
 * @return The amount of bytes truly read, or 0 if there was a failure.
 */
uint32 ChunkFile_Read(uint8 index, uint32 chunk, void *buffer, uint32 buflen)
{
	uint32 value = 0;
	uint32 length = 0;
	bool first = true;

	while (true) {
		if (File_Read(index, &value, 4) != 4 && !first) return 0;

		if (value == 0 && File_Read(index, &value, 4) != 4 && !first) return 0;

		if (File_Read(index, &length, 4) != 4 && !first) return 0;

		length = HTOBE32(length);

		if (value == chunk) {
			buflen = min(buflen, length);

			File_Read(index, buffer, buflen);

			length += 1;
			length &= 0xFFFFFFFE;

			if (buflen < length) File_Seek(index, length - buflen, 1);

			return buflen;
		}

		if (first) {
			File_Seek(index, 12, 0);
			first = false;
			continue;
		}

		length += 1;
		length &= 0xFFFFFFFE;
		File_Seek(index, length, 1);
	}
}
