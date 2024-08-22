#include <dents.h>
#include <fakefs.h>
#include <linked_list.h>
#include <malloc.h>
#include <string.h>
#include <task.h>

FakefsFile *fakefsAddFile(Fakefs *fakefs, FakefsFile *under, char *filename,
                          char *symlink, uint16_t filetype,
                          VfsHandlers *handlers) {
  FakefsFile *file = (FakefsFile *)LinkedListAllocate((void **)(&under->inner),
                                                      sizeof(FakefsFile));

  file->filename = filename;
  file->filenameLength = strlength(filename);
  file->filetype = filetype;
  file->inode = ++fakefs->lastInode;
  file->handlers = handlers;

  if (symlink) {
    file->symlink = symlink;
    file->symlinkLength = strlength(symlink);
  }

  return file;
}

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
FakefsFile *fakefsTraverse(FakefsFile *start, char *search,
                           size_t searchLength) {
  FakefsFile *browse = start;
  while (browse) {
    if (memcmp(search, browse->filename,
               MAX(browse->filenameLength, searchLength)) == 0)
      break;
    browse = browse->next;
  }

  return browse;
}

void fakefsSetupRoot(FakefsFile **ptr) {
  FakefsFile fakefsRoot = {.filename = "/",
                           .filenameLength = 1,
                           .next = 0,
                           .inner = 0,
                           .filetype = S_IFDIR | S_IRUSR | S_IWUSR | S_IXUSR,
                           .symlink = 0,
                           .symlinkLength = 0,
                           .handlers = &fakefsRootHandlers,
                           .size = 3620};

  *ptr = (FakefsFile *)malloc(sizeof(FakefsFile));
  memcpy(*ptr, &fakefsRoot, sizeof(FakefsFile));
}

FakefsFile *fakefsTraversePath(FakefsFile *start, char *path) {
  FakefsFile *fakefs = start->inner;
  size_t      len = strlength(path);

  if (len == 1) // meaning it's trying to open /
    return start;

  int lastslash = 0;
  for (int i = 1; i < len; i++) { // 1 to skip /[...]
    bool last = i == (len - 1);

    if (path[i] == '/' || last) {
      size_t length = i - lastslash - 1;
      if (last) // no need to remove trailing /
        length += 1;

      FakefsFile *res = fakefsTraverse(fakefs, path + lastslash + 1, length);

      // return fail or last's success
      if (!res || i == (len - 1))
        return res;

      fakefs = res->inner;
      lastslash = i;
    }
  }

  // will never be reached but whatever
  return 0;
}

bool fakefsOpen(char *filename, OpenFile *target) {
  MountPoint    *mnt = target->mountPoint;
  FakefsOverlay *fakefs = (FakefsOverlay *)mnt->fsInfo;

  FakefsFile *file = fakefsTraversePath(fakefs->fakefs->rootFile, filename);
  if (!file) {
    // debugf("! %s\n", filename);
    return false;
  }
  target->handlers = file->handlers;

  if (file->filetype | S_IFDIR) {
    // if it's a directory yk
    int len = strlength(filename) + 1;
    target->dirname = (char *)malloc(len);
    memcpy(target->dirname, filename, len);
  }

  if (file->handlers->open) {
    // if a specific open handler is in place
    if (!file->handlers->open(filename, target))
      return false;
  }

  return true;
}

bool fakefsStat(MountPoint *mnt, char *filename, struct stat *target) {
  FakefsOverlay *fakefs = (FakefsOverlay *)mnt->fsInfo;
  FakefsFile    *file = fakefsTraversePath(fakefs->fakefs->rootFile, filename);
  if (!file)
    return false;

  target->st_dev = 69;          // haha
  target->st_ino = file->inode; // could work
  target->st_mode = file->filetype;
  target->st_nlink = 1;
  target->st_uid = 0;
  target->st_gid = 0;
  target->st_rdev = 0;
  target->st_blksize = 0x1000;

  target->st_size = file->size;
  target->st_blocks =
      (DivRoundUp(target->st_size, target->st_blksize) * target->st_blksize) /
      512;

  target->st_atime = 0;
  target->st_mtime = 0;
  target->st_ctime = 0;

  return true;
}

bool fakefsLstat(MountPoint *mnt, char *filename, struct stat *target) {
  // todo (when we got actual symlinks lol)
  return fakefsStat(mnt, filename, target);
}

VfsHandlers fakefsHandlers = {.open = fakefsOpen,
                              .close = 0,
                              .duplicate = 0,
                              .ioctl = 0,
                              .mmap = 0,
                              .read = 0,
                              .stat = 0,
                              .write = 0,
                              .getdents64 = 0};

int fakefsGetDents64(OpenFile *fd, void *task, struct linux_dirent64 *start,
                     unsigned int hardlimit) {
  FakefsOverlay *fakefs = (FakefsOverlay *)fd->mountPoint->fsInfo;

  FakefsFile *weAt = fakefsTraversePath(fakefs->fakefs->rootFile, fd->dirname);

  if (!fd->tmp1)
    fd->tmp1 = (size_t)weAt->inner;

  struct linux_dirent64 *dirp = (struct linux_dirent64 *)start;
  int                    allocatedlimit = 0;

  while (fd->tmp1 != (size_t)(-1)) {
    FakefsFile *current = (FakefsFile *)fd->tmp1;
    DENTS_RES   res =
        dentsAdd(start, &dirp, &allocatedlimit, hardlimit, current->filename,
                 current->filenameLength, current->inode, 0); // todo: type

    if (res == DENTS_NO_SPACE) {
      allocatedlimit = -EINVAL;
      goto cleanup;
    } else if (res == DENTS_RETURN)
      goto cleanup;

    fd->tmp1 = (size_t)current->next;
    if (!fd->tmp1)
      fd->tmp1 = (size_t)(-1);
  }

cleanup:
  return allocatedlimit;
}

VfsHandlers fakefsRootHandlers = {.open = 0,
                                  .close = 0,
                                  .duplicate = 0,
                                  .ioctl = 0,
                                  .mmap = 0,
                                  .read = 0,
                                  .stat = 0,
                                  .write = 0,
                                  .getdents64 = fakefsGetDents64};
