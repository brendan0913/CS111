#!/usr/local/cs/bin/python3

# NAME: Brendan Rossmango
# EMAIL: brendan0913@ucla.edu
# ID: 505370692

import sys, csv
from collections import defaultdict

class Inode:
    def __init__(self, data):
        self.inode = int(data[1])
        self.file_type = str(data[2])
        self.link_count = int(data[6])
        self.file_size = int(data[10])
        self.blocks = tuple(map(int, data[12:]))

class Dirent:
    def __init__(self, data):
        self.inode = int(data[1])
        self.file_inode = int(data[3])
        self.name = str(data[6])

class Indirect:
    def __init__(self, data):
        self.level = int(data[2])
        self.referenced_block = int(data[5])

if len(sys.argv) != 2:
    sys.stderr.write('Incorrect usage: correct usage is ./lab3b [flie_system_csv]\n')
    sys.exit(1)

inconsistencies = []
bfree_list = []
ifree_list = []
inodes_list = []
dirents_list = []
indirects_list = []
csv_file = csv.reader(open(sys.argv[1]))
for data in csv_file:
    name = data[0]
    if name == 'SUPERBLOCK':
        total_blocks = int(data[1])
        total_inodes = int(data[2])
        block_size = int(data[3])
        inode_size = int(data[4])
        first_inode = int(data[7])
    elif name == 'GROUP':
        group_first_inode = int(data[8])
    elif name == 'BFREE':
        bfree_list.append(int(data[1]))
    elif name == 'IFREE':
        ifree_list.append(int(data[1]))
    elif name == 'INODE':
        inodes_list.append(Inode(data))
    elif name == 'DIRENT':
        dirents_list.append(Dirent(data))
    elif name == 'INDIRECT':
        indirects_list.append(Indirect(data))
max_reserved_block = int(group_first_inode + inode_size * total_inodes / block_size)

# inode allocation audits
allocated_inodes_list = []
for inode in inodes_list: # if in inodes summary, is allocated (mode is not 0 and was found not free on bitmap)
    inode_num = inode.inode
    allocated_inodes_list.append(inode_num)
    if inode_num in ifree_list:                                       # if allocated (in inodes_list), should not be on ifree_list
        error = 'ALLOCATED INODE ' + str(inode_num) + ' ON FREELIST'
        inconsistencies.append(error)
for inode in range(first_inode, total_inodes):
    if inode not in allocated_inodes_list and inode not in ifree_list: # if unallocated, should be in ifree_list
        error = 'UNALLOCATED INODE ' + str(inode) + ' NOT ON FREELIST'
        inconsistencies.append(error)

# block consistency audits
allocated_blocks_dict = defaultdict(lambda: []) # tracks duplicates (block is referenced more than once, length > 1) and unreferenced blocks (block is never referenced, length 0)
for inode in inodes_list:
    if inode.file_type == 'f' or inode.file_type == 'd' or (inode.file_type == 's' and inode.file_size >= 60): # symlinks with file size < 60 don't have extra fields
        for offset, block in enumerate(inode.blocks):
            indirect = ''
            if offset == 12:
                indirect = 'INDIRECT '
            elif offset == 13:
                offset = 268    # 12 + 256
                indirect = 'DOUBLE INDIRECT '
            elif offset == 14:
                offset = 65804  # 12 + 256 + 65536
                indirect = 'TRIPLE INDIRECT '
            if block < 0 or block >= total_blocks:         # invalid
                error = 'INVALID ' + indirect + 'BLOCK ' + str(block) + ' IN INODE ' + str(inode.inode) + ' AT OFFSET ' + str(offset)
                inconsistencies.append(error)
            elif block > 0 and block < max_reserved_block: # reserved
                error = 'RESERVED ' + indirect + 'BLOCK ' + str(block) + ' IN INODE ' + str(inode.inode) + ' AT OFFSET ' + str(offset)
                inconsistencies.append(error)
            elif block > 0:                                # allocated (no error)
                allocated_blocks_dict[block].append((block, indirect, inode.inode, offset))
for indirect_block in indirects_list:
    block = indirect_block.referenced_block
    if indirect_block.level == 1:
        indirect = 'INDIRECT '
    elif indirect_block.level == 2:
        indirect = 'DOUBLE INDIRECT '
    elif indirect_block.level == 3:
        indirect = 'TRIPLE INDIRECT '
    if block < 0 or block >= total_blocks:         # invalid
        error = 'INVALID ' + indirect + 'BLOCK ' + str(block) + ' IN INODE ' + str(inode.inode) + ' AT OFFSET ' + str(offset)
        inconsistencies.append(error)
    elif block > 0 and block < max_reserved_block: # reserved
        error = 'RESERVED ' + indirect + 'BLOCK ' + str(block) + ' IN INODE ' + str(inode.inode) + ' AT OFFSET ' + str(offset)
        inconsistencies.append(error)
    elif block > 0:                                # allocated (no error)
        allocated_blocks_dict[block].append((block, indirect, inode.inode, offset))
for block in range(max_reserved_block, total_blocks):
    if not allocated_blocks_dict[block] and block not in bfree_list: # unallocated (unreferenced) block not on bfree_list
        error = 'UNREFERENCED BLOCK ' + str(block)
        inconsistencies.append(error)
    elif allocated_blocks_dict[block] and block in bfree_list:       # allocated block appears on bfree_list
        error = 'ALLOCATED BLOCK ' + str(block) + ' ON FREELIST'
        inconsistencies.append(error)
    elif len(allocated_blocks_dict[block]) > 1:                      # referenced by multiple files (duplicate block)
        for dup in allocated_blocks_dict[block]: # dup = (block, indirect, inode, offset)
            block = str(dup[0])
            indirect = dup[1]
            inode = str(dup[2])
            offset = str(dup[3])
            error = 'DUPLICATE ' + indirect + 'BLOCK ' + block + ' IN INODE ' + inode + ' AT OFFSET ' + offset
            inconsistencies.append(error)

# directory consistency audits
link_counts_dict = defaultdict(lambda: 0) # if no discovered links, link count is 0
parent_dict = defaultdict(lambda: -1)     # if parent_inode not in dict, will be -1
for dirent in dirents_list:
    file_inode = dirent.file_inode
    parent_inode = dirent.inode
    name = dirent.name
    if file_inode < 1 or file_inode > total_inodes: # reference to invalid inode
        error = 'DIRECTORY INODE ' + str(parent_inode) + ' NAME ' + name + ' INVALID INODE ' + str(file_inode)
        inconsistencies.append(error)
    elif file_inode not in allocated_inodes_list:   # reference to unallocated inode
        error = 'DIRECTORY INODE ' + str(parent_inode) + ' NAME ' + name + ' UNALLOCATED INODE ' + str(file_inode)
        inconsistencies.append(error)
    else:                                           # increment discovered link count if referenced inode is valid and allocated 
        link_counts_dict[file_inode] += 1
        if name != "'.'" and name != "'..'":        # if allocated and valid (and not special), add inode as key with parent as value
            parent_dict[file_inode] = parent_inode
for inode in inodes_list:
    discovered_link_count = link_counts_dict[inode.inode]
    referenced_link_count = inode.link_count
    if discovered_link_count != referenced_link_count: # discovered link count should equal referenced link count
        error = 'INODE ' + str(inode.inode) + ' HAS ' + str(discovered_link_count) + ' LINKS BUT LINKCOUNT IS ' + str(referenced_link_count)
        inconsistencies.append(error)
for dirent in dirents_list:
    file_inode = dirent.file_inode
    parent_inode = dirent.inode
    name = dirent.name
    if name == "'.'":                  # current directory, link to itself
        if file_inode != parent_inode: # inode should be the same
            error = "DIRECTORY INODE " + str(parent_inode) + " NAME '.' LINK TO INODE " + str(file_inode) + " SHOULD BE " + str(parent_inode)
            inconsistencies.append(error)
    elif name == "'..'":
        if parent_dict[parent_inode] == -1: # parent inode key does not exist
            if file_inode != parent_inode:
                error = "DIRECTORY INODE " + str(parent_inode) + " NAME '..' LINK TO INODE " + str(file_inode) + " SHOULD BE " + str(parent_inode)
                inconsistencies.append(error)

if not inconsistencies:
    sys.exit(0)
print(*inconsistencies, sep = "\n")
sys.exit(2)