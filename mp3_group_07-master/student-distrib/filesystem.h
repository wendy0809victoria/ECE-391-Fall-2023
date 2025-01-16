/*filesystem.h - Defines used for the Read-only File System
 */
#ifndef FILESYSTEM_H
#define FILESYSTEM_H
#include "types.h"

#define BLOCK_SIZE 4096 // the file system memory is divided into 4KB blocks
// the struct for directory entry
typedef struct dentry_t 
{
    uint8_t file_name[32]; // file name up to 32 B (character), zero-padded, but not necessarily including a terminal EOS or 0-byte
    uint32_t file_type; // file type (0, 1, 2), 4B
    uint32_t inode_num; // index node number
    uint8_t reserved[24]; // reserved 24B
} dentry_t;

// the struct for the boot block
typedef struct boot_block_t 
{
    uint32_t num_dir_entries; // number of directory entries
    uint32_t num_inodes; // number of index nodes
    uint32_t num_data_blocks; // number of data blocks
    uint8_t reserved[52]; // reserved 52B
    dentry_t dir_entries[63]; // directory entries that can hold up to 63 files, (4096 - 64) / 64 = 63
} boot_block_t;

// the struct for the index node
typedef struct inode_t 
{
    uint32_t length; // length of the file in bytes
    uint32_t data_block_num[1023]; // data block numbers, 4096 / 4 = 1024, 1024 - 1 = 1023
} inode_t;

// the struct for the data block
typedef struct data_block_t 
{
    uint8_t data[BLOCK_SIZE]; // data block
} data_block_t;

// three functions used by the file system
// find the directory entry by file name, and copy the entry to the dentry
int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry);
// find the directory entry by index, and copy the entry to the dentry
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry);
// read up to length bytes starting from position offset in the file with inode number inode 
// and returning the number of bytes read and placed in the buffer
int32_t read_data (uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length);

// int32_t read_dirt(uint8_t* buf,uint32_t index);
// four functions used for files
int32_t file_open(const uint8_t* filename);
int32_t file_close(int32_t fd);
int32_t file_read(int32_t fd, void* buf, int32_t nbytes);
int32_t file_write(int32_t fd, const void* buf, int32_t nbytes);
// four functions used for directories
int32_t dir_open(const uint8_t* filename);
int32_t dir_close(int32_t fd);
int32_t dir_read(int32_t fd, void* buf, int32_t nbytes);
int32_t dir_write(int32_t fd, const void* buf, int32_t nbytes);

// init the file system
void file_system_init(uint32_t fs_start_addr);

// get the file size by inode number
int32_t get_length(uint32_t inode_num);

#endif
