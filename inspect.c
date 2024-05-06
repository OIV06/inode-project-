#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

int main(int argc, char *argv[]) {
    struct stat fileInfo;
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file_path>\n", argv[0]);
        return 1;
    }

    if (stat(argv[1], &fileInfo) != 0) {
        fprintf(stderr, "Error getting file info for %s: %s\n", argv[1], strerror(errno));
        return 1;
    }

    printf("Information for %s:\n", argv[1]);
    printf("File Inode: %lu\n", fileInfo.st_ino);
    printf("File Type: ");
    if (S_ISREG(fileInfo.st_mode))
        printf("regular file\n");
    else if (S_ISDIR(fileInfo.st_mode))
        printf("directory\n");
    else if (S_ISCHR(fileInfo.st_mode))
        printf("character device\n");
    else if (S_ISBLK(fileInfo.st_mode))
        printf("block device\n");
    else if (S_ISFIFO(fileInfo.st_mode))
        printf("FIFO (named pipe)\n");
    else if (S_ISLNK(fileInfo.st_mode))
        printf("symbolic link\n");
    else if (S_ISSOCK(fileInfo.st_mode))
        printf("socket\n");
    else
        printf("unknown?\n");

    printf("Number of Hard Links: %lu\n", fileInfo.st_nlink);
    printf("File Size: %lu bytes\n", fileInfo.st_size);
    printf("Last Access Time: %ld\n", fileInfo.st_atime);
    printf("Last Modification Time: %ld\n", fileInfo.st_mtime);
    printf("Last Status Change Time: %ld\n", fileInfo.st_ctime);

    return 0;
}
