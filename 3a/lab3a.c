// NAME: Brendan Rossmango
// EMAIL: brendan0913@ucla.edu
// ID: 505370692

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include "ext2_fs.h"

// Referenced https://www.geeksforgeeks.org/strftime-function-in-c/#:~:text=strftime()%20is%20a%20function,hold%20the%20time%20and%20date
void format_gmt(char* str_time, u_int32_t t){
    time_t raw_time = t;
    struct tm* time = gmtime(&raw_time);
                         /* mm/dd/yy hh:mm:ss */
    strftime(str_time, 18, "%m/%d/%y %H:%M:%S", time);
}

void indirect_block_references_summary(int fd, u_int32_t block_size, u_int32_t curr_inode, u_int32_t indirect_block, u_int32_t offset, int level){
    u_int32_t num_blocks = block_size / 4;
    u_int32_t blocks[num_blocks];
    if (pread(fd, blocks, block_size, 1024 + block_size * (indirect_block - 1)) < 0){
        fprintf(stderr, "Error: pread of indirect block reference of inode %d failed\n", curr_inode);
        exit(2);
    }
    u_int32_t i = 0;
    while (i < num_blocks){
        u_int32_t block = blocks[i];
        if (block){ /* Print non-zero block pointers
                        1     2  3  4  5  6 */
            printf("INDIRECT,%d,%d,%d,%d,%d\n",
                curr_inode,     // 2. I-node number of owning file
                level,          // 3. single indirect level for scanned block
                offset + i,     // 4. logical block offset 
                indirect_block, // 5. block number of the indirect block being scanned
                block);         // 6. block number of referenced block
            // Recursively scan by levels
            if (level == 2){
                indirect_block_references_summary(fd, block_size, curr_inode, block, offset, 1);
                offset += 256; // double indirect block size is 256
            }
            else if (level == 3) {
                indirect_block_references_summary(fd, block_size, curr_inode, block, offset, 2);
                offset += 65536; // triple indirect block size is 65536
            }
        }
        i++;
	}
}

void directory_entries_summary(int fd, u_int32_t block_size, u_int32_t curr_inode, u_int32_t block){
    struct ext2_dir_entry dir_entry;
    u_int32_t i = 0;
    // For each data block in given directory inode (curr_inode)
    while (i < block_size){
        if (pread(fd, &dir_entry, sizeof(dir_entry), block * block_size + i) < 0){
            fprintf(stderr, "Error: pread of directory entry %d failed\n", i);
            exit(2);
        }
        u_int16_t entry_length = dir_entry.rec_len;
        u_int32_t i_node = dir_entry.inode;
        if (i_node){ /* If valid directory entry
                       1    2  3  4  5  6   7 */
            printf("DIRENT,%d,%d,%d,%d,%d,'%s'\n",
                curr_inode,         // 2. parent inode number (the I-node number of the directory that contains this entry)
                i,                  // 3. byte offset of entry within directory
                i_node,             // 4. inode number of referenced file
                entry_length,       // 5. entry length
                dir_entry.name_len, // 6. name length
                dir_entry.name);    // 7. name
        }
        i += entry_length;
    }
}

void inodes_summary(int fd, u_int32_t block_size, u_int32_t prev_inode, u_int32_t curr_inode, u_int32_t inode_table){
    struct ext2_inode inode;
    if (pread(fd, &inode, sizeof(inode), inode_table * block_size + prev_inode * sizeof(inode)) < 0){
        fprintf(stderr, "Error: pread of allocated inode entry %d failed\n", curr_inode);
        exit(2);
    }
    u_int16_t mode = inode.i_mode, links_count = inode.i_links_count;
    if (mode && links_count){ // If allocated (non-zero mode and non-zero link count)
        // Time format "mm/dd/yy hh:mm:ss" is 17 characters long
        char ctime[18], mtime[18], atime[18];
        format_gmt(ctime, inode.i_ctime);
        format_gmt(mtime, inode.i_mtime);
        format_gmt(atime, inode.i_atime);
        
        char file_type = '?';
        // Macros defined in <sys/stat.h>
        if (S_ISDIR(mode)) file_type = 'd';      // directory
        else if (S_ISREG(mode)) file_type = 'f'; // regular file
        else if (S_ISLNK(mode)) file_type = 's'; // symlink

        /* First 12 fields 
                  1    2  3  4  5  6  7  8  9 10 11 12 */
        printf("INODE,%d,%c,%o,%d,%d,%d,%s,%s,%s,%d,%d",
            curr_inode,      // 2. inode number
            file_type,       // 3. file type
            mode & 0xFFF,    // 4. mode (low order 12-bits)
            inode.i_uid,     // 5. owner
            inode.i_gid,     // 6. group
            links_count,     // 7. link count
            ctime,           // 8. time of last I-node change (creation time)
            mtime,           // 9. modification time
            atime,           // 10. time of last access (access time)
            inode.i_size,    // 11. file size
            inode.i_blocks); // 12. number of (512 byte) blocks of disk space taken up by this file
        int i;
        // Next EXT2_N_BLOCKS (15) fields
        // If file is symbolic link and file length is less than 60 bytes, no need to print the 15 block pointers
        if (file_type == 'f' || file_type == 'd' || (file_type == 's' && inode.i_size >= 60)){
            for (i = 0; i < 15; i++){
                printf(",%d",inode.i_block[i]);
            }
        }
        printf("\n"); // After printing the 15 fields, print new line
        // For each directory inode (given as curr_inode), scan all EXT2_NDIR_BLOCKS (12) data blocks
        if (file_type == 'd'){
            for (i = 0; i < 12; i++){
                u_int32_t b = inode.i_block[i];
                if (b) directory_entries_summary(fd, block_size, curr_inode, b);
            }
        }
        if (file_type == 's') return;
        // For each file or directory i-node (given as curr_inode), scan single indirect blocks, recursively scan double and triple indirect blocks
        u_int32_t single_indirect = inode.i_block[EXT2_IND_BLOCK];
        u_int32_t double_indirect = inode.i_block[EXT2_DIND_BLOCK];
        u_int32_t triple_indirect = inode.i_block[EXT2_TIND_BLOCK];
        u_int32_t offset = 12;
        if (single_indirect) // single indirect block, offset 12 (after 12 block pointers), level 1
	        indirect_block_references_summary(fd, block_size, curr_inode, single_indirect, offset, 1);
        offset += 256;
	    if (double_indirect) // double indirect block, offset 12 + 256 (double indirect block is 256), level 2
	        indirect_block_references_summary(fd, block_size, curr_inode, double_indirect, offset, 2);
        offset += 65536;
	    if (triple_indirect) // triple indirect block, offset 268 + 65536 (triple indirect block is 65536), level 3
	        indirect_block_references_summary(fd, block_size, curr_inode, triple_indirect, offset, 3);
    }
}

void free_inode_entries_summary(int fd, u_int32_t block_size, u_int32_t inodes_count, u_int32_t inode_bitmap, u_int32_t inode_table){
    u_int32_t inode_size = inodes_count / 8; // Assume a single group (inodes_count == inodes_per_group)
    char inode_bytes[inode_size];
    if (pread(fd, inode_bytes, inode_size, inode_bitmap * block_size) < 0){
        fprintf(stderr, "Error: pread of inode entries failed\n");
        exit(2);
    }
    // Start at first inode
    u_int32_t curr_inode = 1, i = 0;
    while (i < inode_size){
        char c = inode_bytes[i];
        u_int32_t j;
        for (j = 0; j < 8; j++){
            if (c & 1) // 1: inode is allocated
                inodes_summary(fd, block_size, curr_inode - 1, curr_inode, inode_table);
            else // 0: inode is free
                printf("IFREE,%d\n", curr_inode);
            curr_inode++;
            c >>= 1;
        }
        i++;
    }
}

void free_block_entries_summary(int fd, u_int32_t block_size, u_int32_t first_data_block, u_int32_t block_bitmap){
    char block_bytes[block_size];
    if (pread(fd, block_bytes, block_size, block_bitmap * block_size) < 0){
        fprintf(stderr, "Error: pread of block entries failed\n");
        exit(2);
    }
    // Start at first data block (found from superblock analysis)
    u_int32_t free_block = first_data_block, i = 0;
    while (i < block_size){
        char c = block_bytes[i];
        u_int32_t j;
        for (j = 0; j < 8; j++){
            if (!(c & 1)) // 0: block is free
                printf("BFREE,%d\n", free_block);
            free_block++;
            c >>= 1;
        }
        i++;
    }
}

void group_summary(int fd, u_int32_t blocks_count, u_int32_t inodes_count, u_int32_t block_size, u_int32_t first_data_block){
    struct ext2_group_desc group;
    if (pread(fd, &group, sizeof(group), 1024 + block_size) < 0){
        fprintf(stderr, "Error: pread of group failed\n");
        exit(2);
    }
    u_int32_t block_bitmap = group.bg_block_bitmap, inode_bitmap = group.bg_inode_bitmap, inode_table = group.bg_inode_table;
    /* Assume single group --> group number is 0
              1   2  3  4  5  6  7  8  9 */
    printf("GROUP,0,%d,%d,%d,%d,%d,%d,%d\n",
        blocks_count,               // 3. total number of blocks in group
        inodes_count,               // 4. total number of i-nodes in group
        group.bg_free_blocks_count, // 5. number of free blocks
        group.bg_free_inodes_count, // 6. number of free i-nodes
        block_bitmap,               // 7. free block bitmap
        inode_bitmap,               // 8. free i-node bitmap
        inode_table);               // 9. first block of i-nodes
    free_block_entries_summary(fd, block_size, first_data_block, block_bitmap);
    free_inode_entries_summary(fd, block_size, inodes_count, inode_bitmap, inode_table);
}

void superblock_summary(int fd){
    struct ext2_super_block sb;
    if (pread(fd, &sb, sizeof(sb), 1024) < 0){
        fprintf(stderr, "Error: pread of superblock failed\n");
        exit(2);
    }
    if (sb.s_magic != EXT2_SUPER_MAGIC){
        fprintf(stderr, "Error: bad file system\n");
        exit(2);
    }
    u_int32_t blocks_count = sb.s_blocks_count, inodes_count = sb.s_inodes_count;
    u_int32_t block_size = 1024 << sb.s_log_block_size;
    u_int32_t first_data_block = sb.s_first_data_block;
    /*           1      2  3  4  5  6  7  8 */
    printf("SUPERBLOCK,%d,%d,%d,%d,%d,%d,%d\n",
        blocks_count,          // 2. total number of blocks
        inodes_count,          // 3. total number of i-nodes
        block_size,            // 4. block size 
        sb.s_inode_size,       // 5. i-node size
        sb.s_blocks_per_group, // 6. blocks per group
        sb.s_inodes_per_group, // 7. i-nodes per group
        sb.s_first_ino);       // 8. first non-reserved i-node
    group_summary(fd, blocks_count, inodes_count, block_size, first_data_block);
}

int main(int argc, char **argv){
    if (argc != 2){
        fprintf(stderr, "Incorrect usage: correct usage is ./lab3a [file_system_img]\n");
        exit(1);
    }
    int fd = -1;
    if ((fd = open(argv[1], O_RDONLY)) < 0){
        fprintf(stderr, "Error: Could not open image file\n");
        exit(1);
    }
    /* Prints superblock summary, which also prints group summary (assuming a single group) after assigning values
     * that other functions use (blocks_count, inodes_count, block_size, first_data_block). In addition to group 
     * summary, prints free block and I-node entries summary. If the I-node is allocated, prints the I-node
     * summary, which prints the following:
     * If the filetype of the I-node is a directory, prints directory entries summary. If single, double, 
     * triple blocks point to blocks, prints indirect block references summary.
     */
    superblock_summary(fd);
    exit(0);
}