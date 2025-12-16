#ifndef FAT12_H
#define FAT12_H

#include <lib/lib.h>
#include <drivers/ata.h>
#include <mm/memory.h>

// BPB Structure (matches boot sector)
typedef struct {
    u8  jump[3];               // 0x00: Jump instruction
    u8  oem_name[8];           // 0x03: OEM name
    u16 bytes_per_sector;      // 0x0B: 512
    u8  sectors_per_cluster;   // 0x0D: 1
    u16 reserved_sectors;      // 0x0E: 112
    u8  num_fats;              // 0x10: 2
    u16 root_entries;          // 0x11: 512
    u16 total_sectors_small;   // 0x13: 0
    u8  media_type;            // 0x15: 0xF8
    u16 sectors_per_fat;       // 0x16: 12
    u16 sectors_per_track;     // 0x18: 63
    u16 num_heads;             // 0x1A: 16
    u32 hidden_sectors;        // 0x1C: 0
    u32 total_sectors_large;   // 0x20: 4194304
    u8  drive_number;          // 0x24: 0x80
    u8  reserved;              // 0x25: 0
    u8  boot_signature;        // 0x26: 0x29
    u32 volume_serial;         // 0x27: Random
    u8  volume_label[11];      // 0x2B: "MY OS DISK "
    u8  filesystem_type[8];    // 0x36: "FAT12   "
} __attribute__((packed)) BPB;

// Directory Entry
typedef struct {
    char  filename[8];           // Filename (padded with spaces)
    char  extension[3];          // Extension
    u8  attributes;            // File attributes
    u8  reserved;              // Reserved
    u8  creation_time_ms;      // Creation time (ms)
    u16 creation_time;         // Creation time
    u16 creation_date;         // Creation date
    u16 last_access_date;      // Last access date
    u16 first_cluster_high;    // High word of first cluster (FAT32)
    u16 last_mod_time;         // Last modification time
    u16 last_mod_date;         // Last modification date
    u16 first_cluster;         // First cluster of file
    u32 file_size;             // File size in bytes
} __attribute__((packed)) DirectoryEntry;

// File attributes
#define ATTR_READ_ONLY  0x01
#define ATTR_HIDDEN     0x02
#define ATTR_SYSTEM     0x04
#define ATTR_VOLUME_ID  0x08
#define ATTR_DIRECTORY  0x10
#define ATTR_ARCHIVE    0x20

// FAT12 special values
#define FAT12_FREE      0x000
#define FAT12_RESERVED  0x001
#define FAT12_BAD       0xFF7
#define FAT12_EOF_MIN   0xFF8
#define FAT12_EOF       0xFFF

// Mount information
typedef struct {
    u32 fat_start;             // FAT start sector
    u32 root_start;            // Root directory start sector
    u32 root_size;             // Root directory size in sectors
    u32 data_start;            // Data area start sector
    u16 sectors_per_fat;
    u16 bytes_per_sector;
    u8  sectors_per_cluster;
    u16 root_entries;
    u8  *fat_buffer;           // Cached FAT
} FAT12_MountInfo;

// File handle
typedef struct {
    char     name[256];
    u16 first_cluster;
    u16 current_cluster;
    u32 file_size;
    u32 position;
    u8  attributes;
    int      eof;
} FAT12_File;

// Function prototypes
int fat12_init(void);
FAT12_File* fat12_open(const char *filename);
int fat12_read(FAT12_File *file, u8 *buffer, u32 size);
int fat12_close(FAT12_File *file);
int fat12_create(const char *filename, u32 size);
int fat12_write(FAT12_File *file, const u8 *buffer, u32 size);
int fat12_list_root(void);

#endif