#pragma once

#include <stdint.h>
#include <sys/types.h>

#define MAX_OPEN_FILES     16
#define NQP_FILE_NOT_FOUND -1

typedef enum NQP_FS_TYPE
{
    NQP_FS_EXFAT,

    // NQP_FS_TYPES should always be last
    NQP_FS_TYPES
} nqp_fs_type;

typedef enum NQP_DIRECTORY_ENTRY_TYPE
{
    DT_REG,  // a regular file
    DT_DIR,  // a directory
} nqp_dtype;

typedef struct NQP_DIRECTORY_ENTRY
{
    uint64_t  inode_number; // the unique identifier for this entry
    size_t    name_len;     // the number of characters in the name
    char*     name;         // the actual name
    size_t    file_size;    // the total number of bytes in the file
    nqp_dtype type;         // the type of file that this points at
} nqp_dirent;

typedef enum NQP_SEEK_WHENCE
{
    NQP_SEEK_SET,
    NQP_SEEK_CUR,
    NQP_SEEK_END
} nqp_seek_whence;

typedef enum NQP_MOUNT_ERROR
{
    NQP_MOUNT_OK = 0,              // no error.

    NQP_MOUNT_UNSUPPORTED_FS = -1, // this file system is not supported by the
                                   // implementation.

    NQP_MOUNT_FSCK_FAIL = -2,      // the file system's super block did not pass
                                   // the basic file system check.

    NQP_MOUNT_INVAL = -3,          // an invalid argment was passed.
} nqp_mount_error;

/**
 * "Mount" a file system.
 *
 * This function must be called before interacting with any other nqp_*
 * functions (they will all use the "mounted" file system).
 *
 * This function does a basic file system check on the super block of the file 
 * system being mounted.
 *
 * Parameters:
 *  * source: The file containing the file system to mount. Must not be NULL.
 *  * fs_type: The type of the file system. Must be a value from nqp_fs_type.
 * Return: An appropriate error response as defined in nqp_mount_error.
 */
nqp_mount_error nqp_mount( const char *source, nqp_fs_type fs_type );

/**
 * "Unmount" the mounted file system.
 *
 * This function should be called to flush any changes to the file system's 
 * volume (there shouldn't be! All operations are read only.)
 *
 * Return: NQP_INVAL on error (e.g., there is no fs currently mounted) or
 *         NQP_OK on success.
 */
nqp_mount_error nqp_unmount( void );
/**
 * Get the volume label for the mounted file system.
 *
 * Return: NULL on error, or the volume label for the mounted file system.
 *         Caller is responsible for free()-ing the returned pointer.
 */
char *nqp_vol_label( void );
/**
 * Open the file at pathname in the "mounted" file system.
 *
 * Parameters:
 *  * pathname: The path of the file or directory in the file system that
 *              should be opened.  Must not be NULL.
 * Return: -1 on error, or a nonnegative integer on success. The nonnegative
 *         integer is a file descriptor.
 */
int nqp_open( const char *pathname );

/**
 * Close the file referred to by the descriptor.
 *
 * Parameters:
 *  * fd: The file descriptor to close. Must be a nonnegative integer.
 * Return: -1 on error or 0 on success.
 */
int nqp_close( int fd );

/**
 * Read from a file desriptor.
 *
 * Parameters:
 *  * fd: The file descriptor to read from. Must be a nonnegative integer. The
 *        file descriptor should refer to a file, not a directory.
 *  * buffer: The buffer to read data into. Must not be NULL.
 *  * count: The number of bytes to read into the buffer.
 * Return: The number of bytes read, 0 at the end of the file, or -1 on error.
 */
ssize_t nqp_read( const int fd, void *buffer, const size_t count );

/**
 * Get the directory entries for a directory. Similar to read()ing a file, you
 * may need to call this function repeatedly to get all directory entries.
 *
 * The name field of struct NQP_DIRECTORY_ENTRY is dynamically allocated in
 * this function, the caller is responsible for free()-ing the name after use.
 *
 * Parameters:
 *  * fd: The file descriptor to read from. Must be a nonnegative integer. The
 *        file descriptor should refer to a directory, not a file.
 *  * dirp: the buffer into which the directory entries will be written. The
 *          buffer must not be NULL.
 *  * count: the number of instances of struct NQP_DIRECTORY_ENTRY to read 
 *           (e.g., to read NQP_DIRECTORY_ENTRY, you would pass 1). Must be
 *           greater than zero.
 * Return: The total number of struct NQP_DIRECTORY_ENTRY read into the buffer,
 *         0 at the end of the directory, or -1 on error.
 */
ssize_t nqp_getdents( const int fd, nqp_dirent *dirp, const size_t count );

/**
 * Reposition the read/write file offset.
 * 
 * An offset that's past the end of the file has the same effect as setting the
 * offset to the end of the file.
 * 
 * An offset that results in a value lower than zero when working backwards 
 * with NQP_SEEK_END has the effect of setting the current offset to zero (this
 * is the same as using NQP_SEEK_SET and 0).
 *
 * Parameters:
 *  * fd: the file descriptor to change the offset for. Must be a nonnegative
 *        integer.
 *  * offset: the offset (in bytes). Offsets relative to the end of the file
 *            should be passed as a positive value (i.e., to seek to one byte
 *            before the end of the file, you should pass 1 to this function).
 *  * whence: relative to what should the offset be.
 * Return: the resulting offet into the file, or -1 on error.
 */
off_t nqp_lseek( const int fd, const off_t offset, const nqp_seek_whence whence );
