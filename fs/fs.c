#include "fs.h"

// Disk I/O functions (you need to implement these)

static FAT12_MountInfo mount_info;
static u8 sector_buffer[512];

// Initialize FAT12 filesystem
int fat12_init(void) {
    // Read boot sector (sector 0)
    read_sector(0, sector_buffer);
    BPB *bpb = (BPB*)sector_buffer;
    
    // Verify filesystem
    if (memcmp(bpb->filesystem_type, "FAT12   ", 8) != 0) {
        return -1; // Not FAT12
    }
    
    // Store mount information
    mount_info.bytes_per_sector = bpb->bytes_per_sector;          // 512
    mount_info.sectors_per_cluster = bpb->sectors_per_cluster;    // 1
    mount_info.sectors_per_fat = bpb->sectors_per_fat;            // 12
    mount_info.root_entries = bpb->root_entries;                  // 512
    
    // Calculate filesystem layout
    mount_info.fat_start = bpb->reserved_sectors;                 // 112
    
    mount_info.root_start = mount_info.fat_start + 
                           (bpb->num_fats * mount_info.sectors_per_fat);
    // root_start = 112 + (2 × 12) = 136
    
    mount_info.root_size = (bpb->root_entries * 32) / bpb->bytes_per_sector;
    // root_size = (512 × 32) / 512 = 32 sectors
    
    mount_info.data_start = mount_info.root_start + mount_info.root_size;
    // data_start = 136 + 32 = 168
    
    // Allocate and load FAT into memory
    mount_info.fat_buffer =  (u8*)pmmngr_alloc_block();//malloc(mount_info.sectors_per_fat * 512);
    if (!mount_info.fat_buffer) {
        return -1;
    }
    
    // Load entire FAT
    for (u32 i = 0; i < mount_info.sectors_per_fat; i++) {
        read_sector(mount_info.fat_start + i, 
                   mount_info.fat_buffer + (i * 512));
    }
    
    kprint("FAT12 initialized:\n");
    // printf("  FAT start: %d\n", mount_info.fat_start);
    // printf("  Root start: %d\n", mount_info.root_start);
    // printf("  Data start: %d\n", mount_info.data_start);
    
    return 0;
}

// Convert filename to DOS 8.3 format
static void to_dos_filename(const char *input, char *output) {
    memory_set(output, ' ', 11);
    
    int i = 0, j = 0;
    
    // Copy name part (up to 8 chars)
    while (input[i] && input[i] != '.' && j < 8) {
        output[j++] = (input[i] >= 'a' && input[i] <= 'z') ? 
                      input[i] - 32 : input[i];  // Convert to uppercase
        i++;
    }
    
    // Find extension
    while (input[i] && input[i] != '.') i++;
    
    if (input[i] == '.') {
        i++; // Skip dot
        j = 8; // Start of extension
        while (input[i] && j < 11) {
            output[j++] = (input[i] >= 'a' && input[i] <= 'z') ? 
                          input[i] - 32 : input[i];
            i++;
        }
    }
}

// Open file from root directory
FAT12_File* fat12_open(const char *filename) {
    char dos_name[11];
    to_dos_filename(filename, dos_name);
    
    // Search root directory
    for (u32 sector = 0; sector < mount_info.root_size; sector++) {
        
        read_sector(mount_info.root_start + sector, sector_buffer);
        
        DirectoryEntry *entries = (DirectoryEntry*)sector_buffer;
        
        // Check all 16 entries per sector (512 / 32 = 16)
        for (int i = 0; i < 16; i++) {
            
            // End of directory?
            if (entries[i].filename[0] == 0x00) {
                return 0;  // Not found
            }
            
            // Deleted entry?
            if (entries[i].filename[0] == 0xE5) {
                continue;
            }

            // Compare filenames
            if (memcmp(entries[i].filename, dos_name, 11) == 0) {
                // Found it!
                kprint("FILE FOUND\n");
                FAT12_File *file = (FAT12_File*)pmmngr_alloc_block();//malloc(sizeof(FAT12_File));
                if (!file) return 0;
                
                kprint("FILE POINTER FOUNd\n");
                // strncpy(file->name, filename, 256);
                memory_copy(file->name, filename, 256);
                file->first_cluster = entries[i].first_cluster;
                file->current_cluster = entries[i].first_cluster;
                file->file_size = entries[i].file_size;
                file->position = 0;
                file->attributes = entries[i].attributes;
                file->eof = 0;
                
                return file;
            }
        }
    }
    
    return 0;  // Not found
}

// Get next cluster from FAT12
static u16 get_next_cluster(u16 cluster) {
    // FAT12: each entry is 12 bits (1.5 bytes)
    u32 fat_offset = cluster + (cluster / 2);  // cluster * 1.5
    
    // Read 16-bit value
    u16 next_cluster = *(u16*)(mount_info.fat_buffer + fat_offset);
    
    // Extract 12 bits
    if (cluster & 1) {
        // Odd cluster: high 12 bits
        next_cluster >>= 4;
    } else {
        // Even cluster: low 12 bits
        next_cluster &= 0x0FFF;
    }
    
    return next_cluster;
}

// Set cluster value in FAT12
static void set_cluster_value(u16 cluster, u16 value) {
    u32 fat_offset = cluster + (cluster / 2);
    u16 *fat_entry = (u16*)(mount_info.fat_buffer + fat_offset);
    
    if (cluster & 1) {
        // Odd cluster
        *fat_entry = (*fat_entry & 0x000F) | (value << 4);
    } else {
        // Even cluster
        *fat_entry = (*fat_entry & 0xF000) | (value & 0x0FFF);
    }
}

// Write FAT back to disk
static void sync_fat(void) {
    for (u32 i = 0; i < mount_info.sectors_per_fat; i++) {
        // Write to both FATs
        write_sector(mount_info.fat_start + i, 
                    mount_info.fat_buffer + (i * 512));
        write_sector(mount_info.fat_start + mount_info.sectors_per_fat + i,
                    mount_info.fat_buffer + (i * 512));
    }
}

// Read from file
int fat12_read(FAT12_File *file, u8 *buffer, u32 size) {
    if (!file || file->eof) return 0;
    
    u32 bytes_read = 0;
    u32 bytes_remaining = file->file_size - file->position;

    kprint("FAT12 READ\n");
    
    if (size > bytes_remaining) {
        size = bytes_remaining;
    }
    
    while (size > 0 && file->current_cluster < FAT12_EOF_MIN) {
        // Convert cluster to LBA
        u32 lba = mount_info.data_start + 
                      (file->current_cluster - 2) * mount_info.sectors_per_cluster;
        
        // Read cluster
        read_sector(lba, sector_buffer);
        
        // Copy data
        u32 bytes_to_copy = (size > 512) ? 512 : size;
        memory_copy(buffer, sector_buffer, bytes_to_copy);


        // for (int i = 0; i < 16; i++)
    	// {
		//     kprint(sector_buffer[i]);
	    // }

        kprint("\n");
        
        buffer += bytes_to_copy;
        size -= bytes_to_copy;
        bytes_read += bytes_to_copy;
        file->position += bytes_to_copy;
        
        // Get next cluster
        file->current_cluster = get_next_cluster(file->current_cluster);
    }
    
    if (file->current_cluster >= FAT12_EOF_MIN) {
        file->eof = 1;
    }
    
    return bytes_read;
}

// Find free cluster
static u16 find_free_cluster(void) {
    for (u16 cluster = 2; cluster < 4084; cluster++) {
        u16 value = get_next_cluster(cluster);
        if (value == FAT12_FREE) {
            return cluster;
        }
    }
    return 0;  // No free clusters
}

// Find free directory entry
static DirectoryEntry* find_free_dir_entry(u32 *sector_out, u32 *index_out) {
    for (u32 sector = 0; sector < mount_info.root_size; sector++) {
        read_sector(mount_info.root_start + sector, sector_buffer);
        DirectoryEntry *entries = (DirectoryEntry*)sector_buffer;
        
        for (int i = 0; i < 16; i++) {
            if (entries[i].filename[0] == 0x00 || entries[i].filename[0] == 0xE5) {
                *sector_out = sector;
                *index_out = i;
                return &entries[i];
            }
        }
    }
    return 0;  // Root directory full
}

// Create new file
int fat12_create(const char *filename, u32 size) {
    char dos_name[11] = {0};
    to_dos_filename(filename, dos_name);
    
    // Check if file already exists
    FAT12_File *existing = fat12_open(filename);
    if (existing) {
        // free(existing);
        return -1;  // File exists
    }
    kprint("\nCreated FileName : ");
    kprint(dos_name);
    kprint("\n");

    
    // Find free directory entry
    u32 dir_sector, dir_index;
    DirectoryEntry *entry = find_free_dir_entry(&dir_sector, &dir_index);
    if (!entry) {
        return -1;  // Root directory full
    }
    
    // Allocate clusters for file
    u32 clusters_needed = (size + 511) / 512;  // Round up
    u16 first_cluster = 0;
    u16 prev_cluster = 0;
    
    for (u32 i = 0; i < clusters_needed; i++) {
        u16 cluster = find_free_cluster();
        if (cluster == 0) {
            return -1;  // Disk full
        }
        
        if (i == 0) {
            first_cluster = cluster;
        } else {
            set_cluster_value(prev_cluster, cluster);
        }
        
        set_cluster_value(cluster, (i == clusters_needed - 1) ? FAT12_EOF : 0);
        prev_cluster = cluster;
    }
    
    // Write FAT
    sync_fat();
    
    // Create directory entry
    read_sector(mount_info.root_start + dir_sector, sector_buffer);
    DirectoryEntry *entries = (DirectoryEntry*)sector_buffer;
    
    memory_copy(entries[dir_index].filename, dos_name, 11);
    entries[dir_index].attributes = ATTR_ARCHIVE;
    entries[dir_index].first_cluster = first_cluster;
    entries[dir_index].file_size = size;

    // Write directory entry
    write_sector(mount_info.root_start + dir_sector, sector_buffer);
    
    return 0;  // Success
}

// Write to file
int fat12_write(FAT12_File *file, const u8 *buffer, u32 size) {
    if (!file) return -1;
    
    u32 bytes_written = 0;
    file->current_cluster = file->first_cluster;
    
    while (size > 0 && file->current_cluster < FAT12_EOF_MIN) {
        // Convert cluster to LBA
        u32 lba = mount_info.data_start + 
                      (file->current_cluster - 2) * mount_info.sectors_per_cluster;
        
        // Copy data
        u32 bytes_to_write = (size > 512) ? 512 : size;
        memory_set(sector_buffer, 0, 512);
        memory_copy(sector_buffer, buffer, bytes_to_write);
        
        // Write sector
        write_sector(lba, sector_buffer);
        
        buffer += bytes_to_write;
        size -= bytes_to_write;
        bytes_written += bytes_to_write;
        
        // Next cluster
        file->current_cluster = get_next_cluster(file->current_cluster);
    }
    
    return bytes_written;
}

// List root directory
int fat12_list_root(void) {
    kprint("Root directory contents:\n");
    kprint("Name                   Size\n");
    // printf("------------------------\n");
    
    int count = 0;
    
    for (u32 sector = 0; sector < mount_info.root_size; sector++) {
        read_sector(mount_info.root_start + sector, sector_buffer);
        DirectoryEntry *entries = (DirectoryEntry*)sector_buffer;
        
        for (int i = 0; i < 16; i++) {
            // End of directory?
            if (entries[i].filename[0] == 0x00) {
                goto end_list;
            }
            
            // Deleted entry?
            if (entries[i].filename[0] == 0xE5) {
                continue;
            }
            
            // Skip volume label and directory entries for now
            if (entries[i].attributes & (ATTR_VOLUME_ID | ATTR_DIRECTORY)) {
                continue;
            }
            
            // Print filename
            char name[13];
            int j;
            for (j = 0; j < 8 && entries[i].filename[j] != ' '; j++) {
                name[j] = entries[i].filename[j];
            }
            
            // Add dot if extension exists
            if (entries[i].extension[0] != ' ') {
                name[j++] = '.';
                for (int k = 0; k < 3 && entries[i].extension[k] != ' '; k++) {
                    name[j++] = entries[i].extension[k];
                }
            }
            name[j] = '\0';
            
            // printf("%-11s %10d\n", name, entries[i].file_size);
            kprint(name);
            kprint("                    ");
            kprint(entries[i].file_size);
            kprint("\n");
            count++;
        }
    }
    
end_list:
    // printf("\nTotal files: %d\n", count);
    return count;
}

// Close file
int fat12_close(FAT12_File *file) {
    if (file) {
        // free(file);
        return 0;
    }
    return -1;
}