/* $Id$ */

/** @file src/file.h %File access definitions. */

#ifndef FILE_H
#define FILE_H

enum {
	FILEINFO_MAX     = 682,
	FILEINFO_INVALID = 0xFFFF,

	FILE_MAX = 20,
	FILE_INVALID = 0xFF
};

/**
 * Static information about files and their location.
 */
typedef struct FileInfo {
	const char *filename;                                   /*!< Name of the file. */
	uint32 fileSize;                                        /*!< The size of this file. */
	void *buffer;                                           /*!< In case the file is read in the memory, this is the location of the data. */
	uint32 filePosition;                                    /*!< Where in the file we currently are (doesn't have to start at zero when in PAK file). */
	uint8  parentIndex;                                     /*!< In which FileInfo this file can be found. */
	struct {
		BIT_U8 isLoaded:1;                                  /*!< File is mapped in the memory. */
		BIT_U8 inMemory:1;                                  /*!< File is loaded in alloc'd memory. */
    	BIT_U8 inPAKFile:1;                                 /*!< File can be in other PAK file. */
	} flags;                                                /*!< General flags of the FileInfo. */
} FileInfo;

extern char g_dune_data_dir[1024];
extern char g_personal_data_dir[1024];

extern void File_MakeCompleteFilename(char *buf, size_t len, const char *filename, bool is_global_data);
extern void File_Close(uint8 index);
extern uint32 File_Read(uint8 index, void *buffer, uint32 length);
extern uint32 File_Write(uint8 index, void *buffer, uint32 length);
extern uint32 File_Seek(uint8 index, uint32 position, uint8 mode);
extern uint32 File_GetSize(uint8 index);
extern void File_Delete_Personal(const char *filename);
extern void *File_ReadWholeFile(const char *filename);
extern uint32 File_ReadFile(const char *filename, void *buf);
extern void ChunkFile_Close(uint8 index);
extern uint32 ChunkFile_Seek(uint8 index, uint32 header);
extern uint32 ChunkFile_Read(uint8 index, uint32 header, void *buffer, uint32 buflen);

#define File_Exists(FILENAME)               File_Exists_Ex(FILENAME, true)
#define File_Exists_Personal(FILENAME)      File_Exists_Ex(FILENAME, false)
#define File_Open(FILENAME,MODE)            File_Open_Ex(FILENAME, true,  MODE)
#define File_Open_Personal(FILENAME,MODE)   File_Open_Ex(FILENAME, false, MODE)
#define File_ReadBlockFile(FILENAME,BUFFER,LENGTH)          File_ReadBlockFile_Ex(FILENAME, true,  BUFFER, LENGTH)
#define File_ReadBlockFile_Personal(FILENAME,BUFFER,LENGTH) File_ReadBlockFile_Ex(FILENAME, false, BUFFER, LENGTH)
#define ChunkFile_Open(FILENAME)            ChunkFile_Open_Ex(FILENAME, true)
#define ChunkFile_Open_Personal(FILENAME)   ChunkFile_Open_Ex(FILENAME, false)

extern bool File_Exists_Ex(const char *filename, bool is_global_data);
extern uint8 File_Open_Ex(const char *filename, bool is_global_data, uint8 mode);
extern uint32 File_ReadBlockFile_Ex(const char *filename, bool is_global_data, void *buffer, uint32 length);
extern uint8 ChunkFile_Open_Ex(const char *filename, bool is_global_data);

#endif /* FILE_H */
