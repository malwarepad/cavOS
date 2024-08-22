#include <fakefs.h>
#include <linked_list.h>
#include <string.h>
#include <task.h>

FakefsFile *fakefsAddFile(Fakefs *fakefs, FakefsFile **under, char *filename,
                          char *symlink, uint16_t filetype,
                          VfsHandlers *handlers) {
  FakefsFile *file =
      (FakefsFile *)LinkedListAllocate((void **)under, sizeof(FakefsFile));

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

FakefsFile fakefsRoot = {.filename = "/",
                         .filenameLength = 1,
                         .next = 0,
                         .inner = 0,
                         .filetype = S_IFDIR | S_IRUSR | S_IWUSR | S_IXUSR,
                         .symlink = 0,
                         .symlinkLength = 0,
                         .handlers = &fakefsHandlers,
                         .size = 3620};

FakefsFile *fakefsTraversePath(FakefsFile *start, char *path) {
  FakefsFile *fakefs = start;
  size_t      len = strlength(path);

  if (len == 1) { // meaning it's trying to open /
    return &fakefsRoot;
  }

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

  FakefsFile *file = fakefsTraversePath(fakefs->fakefs->firstFile, filename);
  if (!file) {
    // debugf("! %s\n", filename);
    return false;
  }
  target->handlers = file->handlers;

  if (file->handlers->open) {
    // if a specific open handler is in place
    if (!file->handlers->open(filename, target))
      return false;
  }

  return true;
}

bool fakefsStat(MountPoint *mnt, char *filename, struct stat *target) {
  FakefsOverlay *fakefs = (FakefsOverlay *)mnt->fsInfo;
  FakefsFile    *file = fakefsTraversePath(fakefs->fakefs->firstFile, filename);
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
                              .getdents64 = 0}; // <- todo
