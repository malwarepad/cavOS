#include <ext2.h>
#include <malloc.h>
#include <string.h>
#include <system.h>
#include <timer.h>
#include <util.h>

int ext2Mkdir(MountPoint *mnt, char *dirname, uint32_t mode,
              char **symlinkResolve) {
  Ext2 *ext2 = EXT2_PTR(mnt->fsInfo);

  // dirname will be sanitized anyways
  int len = strlength(dirname);
  int lastSlash = 0;
  for (int i = 0; i < len; i++) {
    if (dirname[i] == '/')
      lastSlash = i;
  }

  uint32_t inode = 0;
  if (lastSlash > 0) {
    char *parent = malloc(lastSlash);
    memcpy(parent, dirname, lastSlash);
    parent[lastSlash] = '\0';
    inode =
        ext2TraversePath(ext2, parent, EXT2_ROOT_INODE, true, symlinkResolve);
    free(parent);
  } else // if it's trying to open / just set the inode directly
    inode = 2;

  if (!inode)
    return -ENOENT;

  // main scope
  int ret = 0;

  // various checks
  char *name = &dirname[lastSlash + 1];
  int   nameLen = strlength(name);

  if (!nameLen) // going for /
    return -EEXIST;

  Ext2Inode *inodeContents = ext2InodeFetch(ext2, inode);
  if (!(inodeContents->permission & S_IFDIR)) {
    ret = -ENOTDIR;
    goto cleanup;
  }

  if (ext2Traverse(ext2, inode, name, nameLen)) {
    ret = -EEXIST;
    goto cleanup;
  }

  size_t time = timerBootUnix + timerTicks / 1000;

  // prepare what we want to write to that inode
  Ext2Inode newInode = {0};
  newInode.permission = S_IFDIR | mode;
  newInode.userid = 0;
  newInode.size = 0; // will get increased by . & .. entries
  newInode.atime = time;
  newInode.ctime = time;
  newInode.mtime = time;
  newInode.dtime = 0; // not yet :p
  newInode.gid = 0;
  newInode.hard_links = 1;
  // newInode.blocks = ; // should be zero'd
  newInode.num_sectors = newInode.size / SECTOR_SIZE;
  newInode.generation = 0;
  newInode.file_acl = 0;
  newInode.dir_acl = 0;
  newInode.f_block_addr = 0;
  newInode.size_high = 0;

  // assign an inode
  uint32_t group = INODE_TO_BLOCK_GROUP(ext2, inode);
  uint32_t newInodeNum = ext2InodeFind(ext2, group);

  // write to it
  ext2InodeModifyM(ext2, newInodeNum, &newInode);

  // create the . and .. entries as children
  ext2DirAllocate(ext2, newInodeNum, &newInode, ".", 1, 2, newInodeNum);
  ext2DirAllocate(ext2, newInodeNum, &newInode, "..", 2, 2, inode);

  // finally, assign it to the parent
  ext2DirAllocate(ext2, inode, inodeContents, name, nameLen, 2, newInodeNum);

cleanup:
  free(inodeContents);
  return ret;
}

int ext2Touch(MountPoint *mnt, char *filename, uint32_t mode,
              char **symlinkResolve) {
  Ext2 *ext2 = EXT2_PTR(mnt->fsInfo);

  // dirname will be sanitized anyways
  int len = strlength(filename);
  int lastSlash = 0;
  for (int i = 0; i < len; i++) {
    if (filename[i] == '/')
      lastSlash = i;
  }

  uint32_t inode = 0;
  if (lastSlash > 0) {
    char *parent = malloc(lastSlash + 1);
    memcpy(parent, filename, lastSlash);
    parent[lastSlash] = '\0';
    inode =
        ext2TraversePath(ext2, parent, EXT2_ROOT_INODE, true, symlinkResolve);
    free(parent);
  } else // if it's trying to open / just set the inode directly
    inode = 2;

  if (!inode)
    return -ENOENT;

  // main scope
  int ret = 0;

  // various checks
  char *name = &filename[lastSlash + 1];
  int   nameLen = strlength(name);

  if (!nameLen) // going for /
    return -EISDIR;

  Ext2Inode *inodeContents = ext2InodeFetch(ext2, inode);
  if (!(inodeContents->permission & S_IFDIR)) {
    ret = -ENOTDIR;
    goto cleanup;
  }

  if (ext2Traverse(ext2, inode, name, nameLen)) {
    ret = -EEXIST;
    goto cleanup;
  }

  size_t time = timerBootUnix + timerTicks / 1000;

  // prepare what we want to write to that inode
  Ext2Inode newInode = {0};
  newInode.permission = S_IFREG | mode;
  newInode.userid = 0;
  newInode.size = 0;
  newInode.atime = time;
  newInode.ctime = time;
  newInode.mtime = time;
  newInode.dtime = 0; // not yet :p
  newInode.gid = 0;
  newInode.hard_links = 1;
  // newInode.blocks = ; // should be zero'd
  newInode.num_sectors = newInode.size / SECTOR_SIZE;
  newInode.generation = 0;
  newInode.file_acl = 0;
  newInode.dir_acl = 0;
  newInode.f_block_addr = 0;
  newInode.size_high = 0;

  // assign an inode
  uint32_t group = INODE_TO_BLOCK_GROUP(ext2, inode);
  uint32_t newInodeNum = ext2InodeFind(ext2, group);

  // write to it
  ext2InodeModifyM(ext2, newInodeNum, &newInode);

  // finally, assign it to the parent
  ext2DirAllocate(ext2, inode, inodeContents, name, nameLen, 1, newInodeNum);

cleanup:
  free(inodeContents);
  return ret;
}
