#ifndef VFS_H
#define VFS_H

#define buildvfs_fd int
#define buildvfs_fd_invalid (-1)
#define buildvfs_kfd int
#define buildvfs_kfd_invalid (-1)

#define buildvfs_exists(fn) (access(fn, F_OK) == 0)

static inline void buildvfs_fputstrptr(FILE *fp, const char *str) {
	fwrite(str, 1, strlen(str), fp);
}

#define BUILDVFS_FIND_REC CACHE1D_FIND_REC
#define BUILDVFS_FIND_FILE CACHE1D_FIND_FILE
#define BUILDVFS_FIND_DIR CACHE1D_FIND_DIR
#define BUILDVFS_SOURCE_GRP CACHE1D_SOURCE_GRP

#define klistaddentry(rec, name, type, source) (-1) // not supported
#define kfileparent(origfp) ((char*)NULL) // only for GRP files

#endif
