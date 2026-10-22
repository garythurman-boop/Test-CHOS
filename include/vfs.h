#ifndef CHOS_VFS_H
#define CHOS_VFS_H
#include "filesystem.h"
struct process; struct exception_frame;
void vfs_mount(struct chfs *);
int64_t file_syscall(struct process *,struct exception_frame *);
#endif
