#include "filesys.h"
#include "lib.h"
#include "system_call.h"
#include "x86_desc.h"

int inode_array[64];

/**
 * filesys_init
 *  DESCRIPTION : initialize pointers relevant to the filesystem
 *  INPUTS : uint32_t filesys_addr - the address of boot block
 *  OUTPUTS : none
 *  RETURN VALUE : none
 *  SIDE EFFECTS : none
 * 
 */
void filesys_init (uint32_t filesys_addr)
{
    int i;
    boot_block_ptr = (boot_block_t*)filesys_addr;                                                   // cast filesys_addr to boot_block_t
    inode_ptr = (inode_t*) (boot_block_ptr + 1);                                                    // Pointing to the first inode block, 1 means the next block of boot block
    dentry_ptr = (dentry_t*) boot_block_ptr->dir_entries;                                           // Pointing to the first dentry
    data_block_ptr = (uint8_t*) (inode_ptr + boot_block_ptr->num_inodes);                           // Pointing to the first data block
    memset(&inode_array[0], 0, sizeof(inode_array[0]));
    for(i = 0; i < (boot_block_ptr->num_dir_entries); i++)
    {
        inode_array[dentry_ptr[i].inode] = 1;                                                       // Set it to be busy status
    }
}

/**
 * read_dentry_by_name
 *  DESCRIPTION : read the corresponding file dentry to the given
 *                dentry based on the given filename
 *  INPUTS : const uint8_t* fname - given filename
 *           dentry_t* dentry - given dentry
 *  OUTPUTS : none
 *  RETURN VALUE : 0 - successfully load the dentry
 *                 -1 - cannot find the corresponding file
 *  SIDE EFFECTS : none
 * 
 */
int32_t read_dentry_by_name (const uint8_t* fname, dentry_t* dentry)
{
    /* non-existent file */
    uint32_t name_len = strlen((char *)fname);

    if(fname == NULL) return -1;                                                                    // If the filename is NULL, return -1

    if (name_len > MAX_FILENAME_LEN) return -1;                                                                   // If the filename length is out of range, return -1

    int i;
    for(i = 0; i < (boot_block_ptr->num_dir_entries); i++){                                         // Traverse the dentries
        dentry_t* cur_dentry = &(dentry_ptr[i]);
        uint32_t dentry_len = strlen((int8_t*)(cur_dentry->file_name));
        if(dentry_len > MAX_FILENAME_LEN) dentry_len = MAX_FILENAME_LEN;
        if((name_len == dentry_len) && (!strncmp((int8_t*)fname, (int8_t*)(cur_dentry->file_name), name_len))){      // If we find the corresponding file, call read_dentry_by_index to write to the buffer
            read_dentry_by_index(i, dentry);
            return 0;                                                                               // Successfully find the file: return 0
        }
    }
    return -1;                                                                                      // Cannot find the corresponding file after traversing: return -1
}

/**
 * read_dentry_by_index
 *  DESCRIPTION : read the corresponding file dentry to the given
 *                dentry based on the given index
 *  INPUTS : uint32_t index - given index
 *           dentry_t* dentry - given dentry
 *  OUTPUTS : none
 *  RETURN VALUE : 0 - successfully load the dentry
 *                 -1 - cannot find the corresponding file
 *  SIDE EFFECTS : none
 * 
 */
int32_t read_dentry_by_index (uint32_t index, dentry_t* dentry)
{
    if (index >= boot_block_ptr->num_dir_entries) return -1;                                        // If the input index greater than the dentries number, return -1

    dentry_t* target = dentry_ptr + index;                                                          // Find the target that we need to copy from

    strncpy((char *)dentry->file_name, (char *)target->file_name, MAX_FILENAME_LEN);                // Copy the filename to dentry->file_name
    dentry->file_type = target->file_type;                                                          // Copy other fields
    dentry->inode     = target->inode;
    return 0;
}

/* read up to "length" bytes starting from position "offset" in the file with number inode*/
/**
 * read_data
 *  DESCRIPTION : read the data with "length" into buffer based on "inode"
                  and "offset" in that file 
 *  INPUTS : uint32_t inode - given inode: find the index node
             uint32_t offset - the offset in the file
             uint8_t* buf - the buffer we want to write data to
             uint32_t length - the length of the data we want to write
 *  OUTPUTS : none
 *  RETURN VALUE : bytes_copied - the number of bytes copied to the buffer
                   -1 - fail to copy data
 *  SIDE EFFECTS : none
 * 
 */
int32_t read_data (uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length) 
{
    /* invalid inode number */
    if (inode >= boot_block_ptr->num_inodes) return -1;     
    
    inode_t* target_inode = inode_ptr + inode;

    /* invalid offset*/
    if (offset > target_inode->length) return 0;                                             // successfully read 0 bytes
    /* eliminate extra length */
    if (length > target_inode->length - offset) length = target_inode->length - offset;      // if beyond end of file 
    /* tricky length */
    if (length == 0) return 0;                                                               // if reading 0 bytes                 

    uint32_t start_block_idx = offset / BLOCK_SIZE;                                          // each data block takes 4kB
    uint32_t start_block_offset = offset % BLOCK_SIZE;                                       // offset in given data block
    uint32_t bytes_copied = 0;                                                               // holding total bytes being copied

    uint32_t D = target_inode->data_blocks[start_block_idx];                                 // index within data block array
    
    /* The start address of reading*/
    uint8_t* read_ptr = data_block_ptr + BLOCK_SIZE*D + start_block_offset;                  // start reading data from this address
    uint32_t left_len = BLOCK_SIZE - start_block_offset;                                     // how many bytes still need to copy in given data block

    while (length > left_len) {

        length -= left_len;                                                                  // update length
        memcpy(buf, read_ptr, left_len);                                                     // copy left bytes into buf
        buf += left_len;                                                                     // update buf pointer
        bytes_copied += left_len;                                                            // accumulate copied bytes
        
        start_block_idx++;                                                                   // update index to data block
        D = target_inode->data_blocks[start_block_idx];                                      // update D
        read_ptr = data_block_ptr + BLOCK_SIZE*D;                                            // update read_ptr
        left_len = BLOCK_SIZE;

    }

    /* last block of data to read */
    memcpy(buf, read_ptr, length);                                          
    buf += length;
    bytes_copied += length;                                                                  // length is less than BLOCK_SIZE

    return bytes_copied;


}

/**
 * file_read
 *  DESCRIPTION : load n bytes data to the given buffer based on given fd
 *  INPUTS : int32_t fd - file descriptor / **for cp2 only**: refers to inode index here
             void* buf - the buffer that we are going to write to
             int32_t nbytes - the number of bytes that need to write
 *  OUTPUTS : none
 *  RETURN VALUE : 0 - successfully write data to the given buffer
                   -1 - fail to write data to the given buffer
 *  SIDE EFFECTS : none 
 * 
 */
int32_t file_read (int32_t fd, void* buf, int32_t nbytes)                                               
{
    pcb_t* cur_pcb_ptr = (pcb_t*)(KERNEL_STACK_START - SIZE_8KB * (cur_process+1));
    file_desc_t file_desc = cur_pcb_ptr->file_array[fd];
    
    int32_t bytes_copied = read_data(file_desc.inode, file_desc.file_position, buf, nbytes);                                                       // fd refers to inode index here, 0 means read from the start of file. **for cp2 only**
    cur_pcb_ptr->file_array[fd].file_position += bytes_copied;
    return bytes_copied;
}

/**
 * file_write
 *  DESCRIPTION : do nothing and return -1
 *  INPUTS : int32_t fd - file descriptor
             void* buf - the buffer that we are going to write to
             int32_t nbytes - the number of bytes that need to write
 *  OUTPUTS : none
 *  RETURN VALUE : -1 - always (for read-only)
 *  SIDE EFFECTS : none
 * 
 */
int32_t file_write(int32_t fd, const void* buf, int32_t nbytes)
{
    return -1;                                                                                  // always return -1(read only)
}

/**
 * file_open
 *  DESCRIPTION : check whether we can open a file
 *  INPUTS : const uint8_t* filename - the name of the file we want to open
 *  OUTPUTS : none
 *  RETURN VALUE : 0 - there exists the file that we want to open
                   -1 - there is no corresponding file
 *  SIDE EFFECTS : none
 * 
 */
int32_t file_open (const uint8_t* filename)
{
    dentry_t dentry;
    return read_dentry_by_name(filename,&dentry);                                              // Call read_dentry_by_name and use its return value
}

/**
 * file_close
 *  DESCRIPTION : do nothing and return 0
 *  INPUTS : int32_t fd - file descriptor
 *  OUTPUTS : none
 *  RETURN VALUE : 0 - always
 *  SIDE EFFECTS : none
 * 
 */
int32_t file_close (int32_t fd)
{
    return 0;                                                                                  // Always return 0
}

/**
 * dir_read
 *  DESCRIPTION : load n bytes data to the given buffer based on given fd
 *  INPUTS : int32_t fd - file descriptor / **for cp2 only**: refers to dentry index here
             void* buf - the buffer that we are going to write to
             int32_t nbytes - the number of bytes that need to write
 *  OUTPUTS : none
 *  RETURN VALUE : 0 - successfully write data to the given buffer
                   -1 - fail to write data to the given buffer
 *  SIDE EFFECTS : none 
 * 
 */
int32_t dir_read (int32_t fd, void* buf, int32_t nbytes)
{
    int ret;
    pcb_t* cur_pcb = (pcb_t*)(KERNEL_STACK_START - SIZE_8KB * (cur_process+1));
    dentry_t dentry;
    /* subsequent reads until the last is reached, at which point read should repeatedly return 0.*/
    if ((cur_pcb->file_array[fd].file_position == boot_block_ptr->num_dir_entries) || (cur_pcb->file_array[fd].file_position == MAX_FILES_NUMBER)){
        return 0;
    }
    ret = read_dentry_by_index(cur_pcb->file_array[fd].file_position, &dentry);
    if (ret == -1) return -1;                                                 
    cur_pcb->file_array[fd].file_position += 1;
    uint32_t len = MAX_FILENAME_LEN;
    if(strlen((const int8_t*)dentry.file_name) < len) len = strlen((const int8_t*)dentry.file_name);
    strncpy((int8_t*)buf, (int8_t*)dentry.file_name, len);
    return len;
}

/**
 * dir_write
 *  DESCRIPTION : do nothing and return -1
 *  INPUTS : int32_t fd - file descriptor
             void* buf - the buffer that we are going to write to
             int32_t nbytes - the number of bytes that need to write
 *  OUTPUTS : none
 *  RETURN VALUE : -1 - always (for read-only)
 *  SIDE EFFECTS : none
 * 
 */
int32_t dir_write (int32_t fd, const void* buf, int32_t nbytes)
{
    int i;
    strcpy((int8_t*)dentry_ptr[boot_block_ptr->num_dir_entries].file_name, (int8_t*)buf);
    dentry_ptr[boot_block_ptr->num_dir_entries].file_type = 2;
    for(i = 0; i < (boot_block_ptr->num_dir_entries); i++)
    {
        if(inode_array[i] == 0)
        {
            dentry_ptr[boot_block_ptr->num_dir_entries].inode = i;
            inode_array[i] = 1;
            break;
        }
    }
    if(i == boot_block_ptr->num_dir_entries)
    {
        return -1;
    }
    boot_block_ptr->num_dir_entries++;
    return 0;                                                                                  // Always return -1 (read-only)
}

/**
 * dir_open
 *  DESCRIPTION : check whether we can open a directory
 *  INPUTS : const uint8_t* filename - the name of the file we want to open
 *  OUTPUTS : none
 *  RETURN VALUE : 0 - there exists the directory that we want to open (we only use this since there is only one directory)
                   -1 - there is no corresponding directory
 *  SIDE EFFECTS : none
 * 
 */
int32_t dir_open (const uint8_t* filename)
{
    dentry_t dentry;
    return read_dentry_by_name(filename, &dentry);                                              // Call read_dentry_by_name and use its return value
}

/**
 * dir_close
 *  DESCRIPTION : do nothing and return 0
 *  INPUTS : int32_t fd - file descriptor
 *  OUTPUTS : none
 *  RETURN VALUE : 0 - always
 *  SIDE EFFECTS : none
 * 
 */
int32_t dir_close (int32_t fd)
{
    return 0;                                                                                   // Always return 0
}
