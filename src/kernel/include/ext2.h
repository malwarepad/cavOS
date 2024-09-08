#include "system.h"
#include "types.h"
#include "vfs.h"

#ifndef EXT2_H
#define EXT2_H

// & 0xF000
#define EXT2_S_IFSOCK 0xC000
#define EXT2_S_IFLNK 0xA000
#define EXT2_S_IFREG 0x8000
#define EXT2_S_IFBLK 0x6000
#define EXT2_S_IFDIR 0x4000
#define EXT2_S_IFCHR 0x2000
#define EXT2_S_IFIFO 0x1000

// I think they took a bit of inspiration from FAT*
#define EXT2_ROOT_INODE 2

// Required feature Flags
#define EXT2_R_F_COMPRESSION 0x0001
#define EXT2_R_F_TYPE_FIELD 0x0002
#define EXT2_R_F_JOURNAL_REPLAY 0x0004
#define EXT2_R_F_JOURNAL_DEVICE 0x0008

// FileSystem State
#define EXT2_FS_S_CLEAN 1
#define EXT2_FS_S_ERRORS 2

// FileSystem Error handling
#define EXT2_FS_E_IGNORE 1
#define EXT2_FS_E_REMOUNT_RO 2
#define EXT2_FS_E_KPANIC 3

typedef struct Ext2SuperblockExtended {
  uint32_t first_inode;
  uint16_t inode_size;
  uint16_t superblock_group;
  uint32_t optional_feature;
  uint32_t required_feature;
  uint32_t readonly_feature;
  char     fs_id[16];
  char     vol_name[16];
  char     last_mount_path[64];
  uint32_t compression_method;
  uint8_t  file_pre_alloc_blocks;
  uint8_t  dir_pre_alloc_blocks;
  uint16_t unused1;
  char     journal_id[16];
  uint32_t journal_inode;
  uint32_t journal_device;
  uint32_t orphan_head;

  char reserved[1024 - 236];
} Ext2SuperblockExtended;

typedef struct Ext2Superblock {
  uint32_t total_inodes;
  uint32_t total_blocks;
  uint32_t su_blocks;
  uint32_t free_blocks;
  uint32_t free_inodes;
  uint32_t superblock_idx;
  uint32_t log2block_size;
  uint32_t log2frag_size;
  uint32_t blocks_per_group;
  uint32_t frags_per_group;
  uint32_t inodes_per_group;

  uint32_t mtime;
  uint32_t wtime;

  uint16_t mount_count;
  uint16_t mount_allowed_count;
  uint16_t ext2_magic;
  uint16_t fs_state;
  uint16_t err;
  uint16_t minor;

  uint32_t last_check;
  uint32_t interval;
  uint32_t os_id;
  uint32_t major;

  uint16_t r_userid;
  uint16_t r_groupid;

  Ext2SuperblockExtended extended;
} __attribute__((packed)) Ext2Superblock;

typedef struct Ext2BlockGroup {
  uint32_t block_bitmap;
  uint32_t inode_bitmap;
  uint32_t inode_table;
  uint16_t free_blocks;
  uint16_t free_inodes;
  uint16_t num_dirs;
  uint16_t padding;
  uint8_t  reserved[12];
} __attribute__((packed)) Ext2BlockGroup;

#define EXT2_DIRECT_BLOCKS 12
typedef struct Ext2Inode {
  uint16_t permission;
  uint16_t userid;
  uint32_t size;
  uint32_t atime;
  uint32_t ctime;
  uint32_t mtime;
  uint32_t dtime;
  uint16_t gid;
  uint16_t hard_links;
  uint32_t num_sectors;
  uint32_t flags;
  uint32_t os_specific1;
  uint32_t blocks[EXT2_DIRECT_BLOCKS + 3];
  uint32_t generation;
  uint32_t file_acl;
  union {
    uint32_t dir_acl;
    uint32_t size_high;
  };
  uint32_t f_block_addr;
  char     os_specific2[12];
} __attribute__((packed)) Ext2Inode;

typedef struct Ext2Directory {
  uint32_t inode;
  uint16_t size;
  uint8_t  filenameLength;
  uint8_t  type; // we required this, although there's a feature bit
  char     filename[0];
} Ext2Directory;

#define EXT2_MAX_CONSEC_DIRALLOC 32
#define EXT2_MAX_CONSEC_BLOCK 32
#define EXT2_MAX_CONSEC_INODE 32

typedef struct Ext2 {
  // various offsets
  size_t offsetBase;
  size_t offsetSuperblock;
  size_t offsetBGDT;

  // sizes
  size_t blockSize;
  size_t inodeSize;
  size_t inodeSizeRounded;

  // cnts
  size_t blockGroups;

  size_t blockGD;

  // max block size is 8KiB and bgdts are forced to be 1 block long soooooo
  Ext2BlockGroup *bgdts; // regular old array
  Ext2Superblock  superblock;

  // special locks for ext2Dir(De)Allocate
  uint32_t dirOperations[EXT2_MAX_CONSEC_DIRALLOC];
  Spinlock LOCK_DIRALLOC_GLOBAL;

  // special locks for ext2Blocks*
  uint32_t blockOperations[EXT2_MAX_CONSEC_BLOCK];
  Spinlock LOCK_BLOCK_GLOBAL;

  // special locks for ext2Inode*
  uint32_t inodeOperations[EXT2_MAX_CONSEC_INODE];
  Spinlock LOCK_BLOCK_INODE;

  // regular Spinlock[] array for every bgdt item
  Spinlock *LOCKS_BLOCK_BITMAP;
  Spinlock *LOCKS_INODE_BITMAP;

  // bgdt & superblock global write locks
  Spinlock LOCK_BGDT_WRITE;
  Spinlock LOCK_SUPERBLOCK_WRITE;
} Ext2;

typedef struct Ext2LookupControl {
  // uint8_t  blockPtr;
  // ^ 0 -> singly, 1 -> doubly, 2 -> triply

  uint32_t *tmp1;
  size_t    tmp1Block;

  uint32_t *tmp2;
  size_t    tmp2Block;
} Ext2LookupControl;

typedef struct Ext2OpenFd {
  Ext2LookupControl lookup;
  // ^ serves as our inodeCurr somewhat

  // size_t   blockNum;
  uint64_t ptr;

  uint32_t  inodeNum;
  Ext2Inode inode;
} Ext2OpenFd;

#define EXT2_PTR(a) ((Ext2 *)(a))
#define EXT2_DIR_PTR(a) ((Ext2OpenFd *)(a))

#define BLOCK_TO_LBA(ext2, group, block)                                       \
  ((ext2)->offsetBase +                                                        \
   ((group) * (ext2)->superblock.blocks_per_group * (ext2)->blockSize) /       \
       SECTOR_SIZE +                                                           \
   ((block) * (ext2)->blockSize) / SECTOR_SIZE)

#define INODE_TO_BLOCK_GROUP(ext2, inode)                                      \
  (((inode) - 1) / (ext2)->superblock.inodes_per_group)

#define INODE_TO_INDEX(ext2, inode)                                            \
  (((inode) - 1) % (ext2)->superblock.inodes_per_group)

// ext2_controller.c
bool   ext2Mount(MountPoint *mount);
int    ext2Open(char *filename, int flags, int mode, OpenFile *fd,
                char **symlinkResolve);
bool   ext2Close(OpenFile *fd);
int    ext2Read(OpenFile *fd, uint8_t *buff, size_t limit);
bool   ext2Stat(MountPoint *mnt, char *filename, struct stat *target,
                char **symlinkResolve);
bool   ext2Lstat(MountPoint *mnt, char *filename, struct stat *target,
                 char **symlinkResolve);
int    ext2StatFd(OpenFile *fd, struct stat *target);
size_t ext2Seek(OpenFile *fd, size_t target, long int offset, int whence);
size_t ext2GetFilesize(OpenFile *fd);
int    ext2Readlink(Ext2 *ext2, char *path, char *buf, int size,
                    char **symlinkResolve);

// ext2_create.c
int ext2Mkdir(MountPoint *mnt, char *dirname, uint32_t mode,
              char **symlinkResolve);
int ext2Touch(MountPoint *mnt, char *filename, uint32_t mode,
              char **symlinkResolve);

// ext2_util.c
void ext2BlockFetchInit(Ext2 *ext2, Ext2LookupControl *control);
void ext2BlockFetchCleanup(Ext2LookupControl *control);

int ext2DirLock(uint32_t *opArr, Spinlock *lock, int constant,
                uint32_t inodeNum);

uint32_t  ext2BlockFetch(Ext2 *ext2, Ext2Inode *ino, Ext2LookupControl *control,
                         size_t curr);
uint32_t *ext2BlockChain(Ext2 *ext2, Ext2OpenFd *fd, size_t curr,
                         size_t blocks);

void     ext2BlockAssign(Ext2 *ext2, Ext2Inode *ino, uint32_t inodeNum,
                         Ext2LookupControl *control, size_t curr, uint32_t val);
uint32_t ext2BlockFind(Ext2 *ext2, int groupSuggestion, uint32_t amnt);
uint32_t ext2BlockFindL(Ext2 *ext2, int group, uint32_t amnt);

// ext2_traverse.c
uint32_t ext2Traverse(Ext2 *ext2, size_t initInode, char *search,
                      size_t searchLength);
uint32_t ext2TraversePath(Ext2 *ext2, char *path, size_t initInode, bool follow,
                          char **symlinkResolve);

// ext2_inode.c
Ext2Inode *ext2InodeFetch(Ext2 *ext2, size_t inode);
void       ext2InodeModifyM(Ext2 *ext2, size_t inode, Ext2Inode *target);

uint32_t ext2InodeFindL(Ext2 *ext2, int group);
uint32_t ext2InodeFind(Ext2 *ext2, int groupSuggestion);

// ext2_dirs.c
bool ext2DirAllocate(Ext2 *ext2, uint32_t inodeNum, Ext2Inode *parentDirInode,
                     char *filename, uint8_t filenameLen, uint8_t type,
                     uint32_t inode);
int  ext2Getdents64(OpenFile *file, struct linux_dirent64 *start,
                    unsigned int hardlimit);

void ext2BgdtPushM(Ext2 *ext2);
void ext2SuperblockPushM(Ext2 *ext2);

// finale
VfsHandlers ext2Handlers;

#endif