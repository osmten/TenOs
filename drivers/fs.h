#ifndef FAT12_H
#define FAT12_H

// #include <stdint.h>
#include "../cpu/types.h"
#include "../kernel/util.h"
#include "ata.h"
#include <stdint.h>


// BPB Structure (matches boot sector)
typedef struct {
    uint8_t  jump[3];               // 0x00: Jump instruction
    uint8_t  oem_name[8];           // 0x03: OEM name
    uint16_t bytes_per_sector;      // 0x0B: 512
    uint8_t  sectors_per_cluster;   // 0x0D: 1
    uint16_t reserved_sectors;      // 0x0E: 112
    uint8_t  num_fats;              // 0x10: 2
    uint16_t root_entries;          // 0x11: 512
    uint16_t total_sectors_small;   // 0x13: 0
    uint8_t  media_type;            // 0x15: 0xF8
    uint16_t sectors_per_fat;       // 0x16: 12
    uint16_t sectors_per_track;     // 0x18: 63
    uint16_t num_heads;             // 0x1A: 16
    uint32_t hidden_sectors;        // 0x1C: 0
    uint32_t total_sectors_large;   // 0x20: 4194304
    uint8_t  drive_number;          // 0x24: 0x80
    uint8_t  reserved;              // 0x25: 0
    uint8_t  boot_signature;        // 0x26: 0x29
    uint32_t volume_serial;         // 0x27: Random
    uint8_t  volume_label[11];      // 0x2B: "MY OS DISK "
    uint8_t  filesystem_type[8];    // 0x36: "FAT12   "
} __attribute__((packed)) BPB;

// Directory Entry
typedef struct {
    char  filename[8];           // Filename (padded with spaces)
    char  extension[3];          // Extension
    uint8_t  attributes;            // File attributes
    uint8_t  reserved;              // Reserved
    uint8_t  creation_time_ms;      // Creation time (ms)
    uint16_t creation_time;         // Creation time
    uint16_t creation_date;         // Creation date
    uint16_t last_access_date;      // Last access date
    uint16_t first_cluster_high;    // High word of first cluster (FAT32)
    uint16_t last_mod_time;         // Last modification time
    uint16_t last_mod_date;         // Last modification date
    uint16_t first_cluster;         // First cluster of file
    uint32_t file_size;             // File size in bytes
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
    uint32_t fat_start;             // FAT start sector
    uint32_t root_start;            // Root directory start sector
    uint32_t root_size;             // Root directory size in sectors
    uint32_t data_start;            // Data area start sector
    uint16_t sectors_per_fat;
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t root_entries;
    uint8_t  *fat_buffer;           // Cached FAT
} FAT12_MountInfo;

// File handle
typedef struct {
    char     name[256];
    uint16_t first_cluster;
    uint16_t current_cluster;
    uint32_t file_size;
    uint32_t position;
    uint8_t  attributes;
    int      eof;
} FAT12_File;

// Function prototypes
int fat12_init(void);
FAT12_File* fat12_open(const char *filename);
int fat12_read(FAT12_File *file, uint8_t *buffer, uint32_t size);
int fat12_close(FAT12_File *file);
int fat12_create(const char *filename, uint32_t size);
int fat12_write(FAT12_File *file, const uint8_t *buffer, uint32_t size);
int fat12_list_root(void);

#endif