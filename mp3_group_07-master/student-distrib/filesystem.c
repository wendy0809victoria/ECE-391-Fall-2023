#include "filesystem.h"
#include "lib.h"
#include "system_calls.h"

static boot_block_t* boot_block_ptr;
static inode_t* inode_ptr;
static data_block_t* data_block_ptr;
int32_t file_idx = 0;

/*
* read_dentry_by_name
*   DESCRIPTION: Find the directory entry by file name, and copy the entry to the dentry
*   INPUTS: fname - the file name
*           dentry - the directory entry
*   OUTPUTS: none
*   RETURN VALUE: 0 for success, -1 for failure
*   SIDE EFFECTS: copy the found entry to the dentry
*/
int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry)
{
    int i;
    // check if the fname or the dentry is valid, and the length of the file name is less than 32
    if (fname == NULL || dentry == NULL || strlen((int8_t*)fname) > 32)
    {
        // return -1 for invalid arguments
        return -1;
    }
    // loop through the directory entries
    for (i = 0; i < boot_block_ptr->num_dir_entries; i++)
    {
        // check if the file name matches
        if (strncmp((int8_t*)fname, (int8_t*)boot_block_ptr->dir_entries[i].file_name, 32) == 0)
        {
            // copy the directory entry to the dentry
            memcpy(dentry, &boot_block_ptr->dir_entries[i], sizeof(dentry_t));
            return 0;
        }
    }
    // if the file name is non-existent, return -1
    return -1;
}

/*
* read_dentry_by_index
*   DESCRIPTION: Find the directory entry by index, and copy the entry to the dentry
*   INPUTS: index - the index of the directory entry
*           dentry - the directory entry
*   OUTPUTS: none
*   RETURN VALUE: 0 for success, -1 for failure
*   SIDE EFFECTS: copy the found entry to the dentry
*/
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry)
{
    // check if the index or the dentry is valid
    if (index >= boot_block_ptr->num_dir_entries || dentry == NULL)
    {
        // return -1 for invalid arguments
        return -1;
    }
    // copy the directory entry to the dentry
    memcpy(dentry, &(boot_block_ptr->dir_entries[index]), sizeof(dentry_t));
    return 0;
}
/*
* read_data
*   DESCRIPTION: Read up to length bytes starting from position offset in the file with inode number inode
*   INPUTS: inode - the inode number
*           offset - the offset of the file
*           buf - the buffer to read
*           length - the length of the file
*   OUTPUTS: none
*   RETURN VALUE: the number of bytes read and placed in the buffer
*   SIDE EFFECTS: copy the data block to the buf
*/
int32_t read_data (uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length)
{
    uint32_t bytes_read = 0;
    uint32_t start_data_block;
    uint32_t start_offset;
    uint32_t end_data_block;
    uint32_t end_offset;
    uint32_t i;
    // check if the inode or the buf is valid
    if (inode >= boot_block_ptr->num_inodes || buf == NULL)
    {
        // return -1 for invalid arguments
        return -1;
    }
    if (offset >= inode_ptr[inode].length)
    {
        // return 0 for end of file
        return 0;
    }
    if (offset + length > inode_ptr[inode].length)
    {
        // read the remaining bytes if the length is greater than the remaining bytes
        length = inode_ptr[inode].length - offset;
    }
    start_data_block = offset / BLOCK_SIZE;
    start_offset = offset % BLOCK_SIZE;
    end_data_block = (offset + length - 1) / BLOCK_SIZE;
    end_offset = (offset + length - 1) % BLOCK_SIZE;
    // read the data block by block
    for (i = start_data_block; i <= end_data_block; i++)
    {
        // calculate the number of bytes to copy
        uint32_t bytes_to_copy = (i == start_data_block && i == end_data_block) ?
                                end_offset - start_offset + 1 :
                                (i == start_data_block) ?
                                BLOCK_SIZE - start_offset :
                                (i == end_data_block) ?
                                end_offset + 1 :
                                BLOCK_SIZE;
        // copy the data block to the buf
        memcpy(buf + bytes_read, &((data_block_ptr[(inode_ptr[inode].data_block_num)[i]].data)[start_offset]), bytes_to_copy);
        bytes_read += bytes_to_copy;
        // reset the start offset to 0 after the first data block
        start_offset = 0;
    }
    return bytes_read;
}

/*
* file_system_init
*   DESCRIPTION: Initialize the file system
*   INPUTS: fs_start_addr - the starting address of the file system
*   OUTPUTS: none
*   RETURN VALUE: none
*   SIDE EFFECTS: init the boot_block_ptr, inode_ptr, and data_block_ptr
*/
void file_system_init(uint32_t fs_start_addr)
{
    boot_block_ptr = (boot_block_t*)fs_start_addr;
    // + 1 - skip the boot block to get the index node ,move the ptr forward by the size of 1 boot block
    inode_ptr = (inode_t*)(boot_block_ptr + 1);
    // + boot_block_ptr->num_inodes - skip the boot block and the index nodes to get the data block
    data_block_ptr = (data_block_t*)(inode_ptr + boot_block_ptr->num_inodes);
}
/*
* file_open
*   DESCRIPTION: Open the file
*   INPUTS: filename - the file name
*   OUTPUTS: none
*   RETURN VALUE: 0 for success, -1 for failure
*   SIDE EFFECTS: none
*/
int32_t file_open(const uint8_t* filename)
{
    dentry_t dentry;
    // check if the file name is valid
    if (filename == NULL)
    {
        // return -1 for invalid arguments
        return -1;
    }
    // read the directory entry by the file name
    if (read_dentry_by_name(filename, &dentry) == -1 || dentry.file_type != 2)
    {
        // return -1 for failure to read the directory entry, or the file is not a regular file
        return -1;
    }
    return 0;
}

/*
* file_close
*   DESCRIPTION: Close the file
*   INPUTS: fd - file descriptor
*   OUTPUTS: none
*   RETURN VALUE: 0 for success, -1 for failure
*   SIDE EFFECTS: none
*/
int32_t file_close(int32_t fd)
{
    return 0;
}
/*
* file_read
*   DESCRIPTION: Read from the file
*   INPUTS: fd - file descriptor
*           buf - buffer to copy
*           nbytes - number of bytes to read
*   OUTPUTS: none
*   RETURN VALUE: -1 for failure, Otherwise, return the number of bytes read
*   SIDE EFFECTS: copy the file to the buf
*/
int32_t file_read(int32_t fd, void* buf, int32_t nbytes)
{
    int32_t bytes_read;
    if (fd < 0 || fd > 7 || buf == NULL || nbytes < 0)
    {
        // return -1 for invalid arguments
        return -1;
    }
    // get the current pcb
    pcb* pcb_ptr = get_cur_pcb_ptr();
    bytes_read = read_data(pcb_ptr->file_descriptor_array[fd].inode, pcb_ptr->file_descriptor_array[fd].file_position, buf, nbytes);
    // increment the file position by the number of bytes read, if the number of bytes read is greater than 0
    // if (bytes_read > 0)
    // {
        // pcb_ptr->file_descriptor_array[fd].file_position += bytes_read;
    // }
    return bytes_read;
}

/*
* file_write
*   DESCRIPTION: Write to the file
*   INPUTS: fd - file descriptor
*           buf - buffer to write
*           nbytes - number of bytes to write
*   OUTPUTS: none
*   RETURN VALUE: 0 for success, -1 for failure
*   SIDE EFFECTS: none
*/
int32_t file_write(int32_t fd, const void* buf, int32_t nbytes)
{
    // always return -1 since the file system is read-only
    return -1;
}
/*
* dir_open
*   DESCRIPTION: Open the directory
*   INPUTS: filename - the file name
*   OUTPUTS: none
*   RETURN VALUE: 0 for success, -1 for failure
*   SIDE EFFECTS: none
*/
int32_t dir_open(const uint8_t* filename)
{
    dentry_t dentry;
    // check if the file name is valid
    if (filename == NULL)
    {
        // return -1 for invalid arguments
        return -1;
    }
    // read the directory entry by the file name
    if (read_dentry_by_name(filename, &dentry) == -1 || dentry.file_type != 1)
    {
        // return -1 for failure to read the directory entry, or the file is not a directory
        return -1;
    }
    return 0;
}
/*
* dir_close
*   DESCRIPTION: Close the directory
*   INPUTS: fd - file descriptor
*   OUTPUTS: none
*   RETURN VALUE: 0 for success, -1 for failure
*   SIDE EFFECTS: none
*/
int32_t dir_close(int32_t fd)
{
    return 0;
}
/*
* dir_read
*   DESCRIPTION: Read from the directory
*   INPUTS: fd - file descriptor
*           buf - buffer to copy
*           nbytes - number of bytes to read
*   OUTPUTS: none
*   RETURN VALUE: 0 for end of directory, Otherwise, return the number of bytes read
*   SIDE EFFECTS: copy the directory entry to the buf
*/
int32_t dir_read(int32_t fd, void* buf, int32_t nbytes)
{
    dentry_t dentry;
    uint8_t* buffer = (uint8_t*)buf;
    if (fd < 0 || fd > 7 || buf == NULL || nbytes < 0)
    {
        // return -1 for invalid arguments
        return -1;
    }
    // If file index does not exceed the number of directory entries
    if (file_idx < boot_block_ptr->num_dir_entries) 
    {
        
        // Get current dentry
        dentry = boot_block_ptr->dir_entries[file_idx];

        // Copy file name into buffer
        memcpy(buffer, &(dentry.file_name), 32);
        // Increase the file idx for next search
        file_idx++;
    } 
    else 
    {
        // If file index exceeds the number of directory entries, end search in current directory
        file_idx = 0;       // 0 - Reset file index for next directory's search
        return 0;           
    }
    // 32 - maximum length of a file name
    return 32;
}

/*
* dir_write
*   DESCRIPTION: Write to the directory
*   INPUTS: fd - file descriptor
*           buf - buffer to write
*           nbytes - number of bytes to write
*   OUTPUTS: none
*   RETURN VALUE: 0 for success, -1 for failure
*   SIDE EFFECTS: none
*/
int32_t dir_write(int32_t fd, const void* buf, int32_t nbytes)
{
    // always return -1 since the file system is read-only
    return -1;
}

/*
* get_length
*   DESCRIPTION: Get the file size by inode number
*   INPUTS: inode - the inode number
*   OUTPUTS: none
*   RETURN VALUE: the length of the file
*   SIDE EFFECTS: none
*/
int32_t get_length(uint32_t inode)
{
    // check if the inode is valid
    if (inode >= boot_block_ptr->num_inodes)
    {
        return -1;
    }
    return inode_ptr[inode].length;
}
