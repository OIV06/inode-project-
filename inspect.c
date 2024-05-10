#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <grp.h> // Include the necessary header file
#include <pwd.h> // Include the <pwd.h> header file
#include <time.h> // Include the <time.h> header file
// Global options structure
typedef struct {
    char *path;
    int human_readable;
    int json_format;
    char *log_file;
    int recursive;
} items;

items opts;

// Function Declarations
void parseargs(int argc, char *argv[]);
void display_help();
void inspect_file(char *file_path);
void print_json(struct stat fileInfo, char *file_path);
void print_human_readable(struct stat fileInfo, char *file_path);
char* getNumber(struct stat fileInfo);
char* getType(struct stat fileInfo);
void print_permissions(struct stat fileInfo, char *perm) ; 
char* getLinkCount(struct stat fileInfo);
char* getUid(struct stat fileInfo);
char* getGid(struct stat fileInfo);
char* getSize(struct stat fileInfo, int human_readable);


void parseargs(int argc, char *argv[]) {
    memset(&opts, 0, sizeof(items));  // Initialize options to zero

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-?") == 0 || strcmp(argv[i], "--help") == 0) {
            display_help();
            exit(EXIT_SUCCESS);
        } else if ((strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--inode") == 0) && i + 1 < argc) {
            opts.path = argv[++i];
        } else if ((strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--all") == 0) && i + 1 < argc) {
            opts.path = argv[++i];
            opts.recursive = 1;  // Default to non-recursive unless specified
        } else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--recursive") == 0) {
            opts.recursive = 1;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--human") == 0) {
            opts.human_readable = 1;
        } else if ((strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--format") == 0) && i + 1 < argc) {
            if (strcmp(argv[i+1], "json") == 0) {
                opts.json_format = 1;
            }
            i++;
        } else if ((strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--log") == 0) && i + 1 < argc) {
            opts.log_file = argv[++i];
        } else {
            fprintf(stderr, "Unrecognized option: %s\n", argv[i]);
            display_help();
            exit(EXIT_FAILURE);
        }
    }

    if (opts.path == NULL) {
        fprintf(stderr, "No file or directory path specified.\n");
        display_help();
        exit(EXIT_FAILURE);
    }
}

void display_help() {
    printf("Usage: inspect [options]\n");
    printf("Options:\n");
    printf(" -?, --help                 Display this help message.\n");
    printf(" -i, --inode <file_path>    Display detailed inode information for the specified file.\n");
    printf(" -a, --all [directory_path] Display inode information for all files in the specified directory.\n");
    printf(" -r, --recursive            List information recursively for directories.\n");
    printf(" -h, --human                Output sizes and dates in human-readable form.\n");
    printf(" -f, --format [text|json]   Specify output format, default is text.\n");
    printf(" -l, --log <log_file>       Log operations to the specified file.\n");
}
void inspect_file(char *file_path) {
    struct stat fileInfo;
    if (stat(file_path, &fileInfo) != 0) {
        fprintf(stderr, "Error getting file info for %s: %s\n", file_path, strerror(errno));
        return;
    }

    if (opts.json_format) {
        print_json(fileInfo, file_path);
    } else {
        print_human_readable(fileInfo, file_path);
    }
}
char* getNumber(struct stat fileInfo) {
    static char num[20];
    sprintf(num, "%lu", fileInfo.st_ino);
    return num;
}
char* getType(struct stat fileInfo) {
    if (S_ISREG(fileInfo.st_mode)) return "regular file";
    if (S_ISDIR(fileInfo.st_mode)) return "directory";
    if (S_ISCHR(fileInfo.st_mode)) return "character device";
    if (S_ISBLK(fileInfo.st_mode)) return "block device";
    if (S_ISFIFO(fileInfo.st_mode)) return "FIFO";
    if (S_ISLNK(fileInfo.st_mode)) return "symbolic link";
    if (S_ISSOCK(fileInfo.st_mode)) return "socket";
    return "unknown";
}

char* getLinkCount(struct stat fileInfo) {
    static char links[20];
    sprintf(links, "%lu", fileInfo.st_nlink);
    return links;
}

char* getUid(struct stat fileInfo) {
    static char uid[20];
    sprintf(uid, "%u", fileInfo.st_uid);
    return uid;
}

char* getGid(struct stat fileInfo) {
    static char gid[20];
    sprintf(gid, "%u", fileInfo.st_gid);
    return gid;
}



void print_permissions(struct stat fileInfo, char *perm) {
    char types[10] = "---------";
    mode_t mode = fileInfo.st_mode;
    if (mode & S_IRUSR) types[0] = 'r';
    if (mode & S_IWUSR) types[1] = 'w';
    if (mode & S_IXUSR) types[2] = 'x';
    if (mode & S_IRGRP) types[3] = 'r';
    if (mode & S_IWGRP) types[4] = 'w';
    if (mode & S_IXGRP) types[5] = 'x';
    if (mode & S_IROTH) types[6] = 'r';
    if (mode & S_IWOTH) types[7] = 'w';
    if (mode & S_IXOTH) types[8] = 'x';
    strcpy(perm, types);
}


char* getSize(struct stat fileInfo, int human_readable) {
    static char size[20];
    if (human_readable) {
        if (fileInfo.st_size < 1024) {
            sprintf(size, "%ld B", fileInfo.st_size);  // Bytes
        } else if (fileInfo.st_size < 1024 * 1024) {
            sprintf(size, "%.1f KB", fileInfo.st_size / 1024.0);  // Kilobytes
        } else if (fileInfo.st_size < 1024 * 1024 * 1024) {
            sprintf(size, "%.1f MB", fileInfo.st_size / (1024.0 * 1024));  // Megabytes
        } else {
            sprintf(size, "%.1f GB", fileInfo.st_size / (1024.0 * 1024 * 1024));  // Gigabytes
        }
    } else {
        sprintf(size, "%ld bytes", fileInfo.st_size);  // Non-human readable format, just bytes
    }
    return size;
}

char* format_time(time_t time) {
    struct tm *tm_info = localtime(&time);
    static char buff[20];
    strftime(buff, 20, "%Y-%m-%d %H:%M:%S", tm_info);
    return buff;
}


void print_human_readable(struct stat fileInfo, char *file_path) {
    printf("Information for %s:\n", file_path);

    char permissions[10];
    print_permissions(fileInfo, permissions); 

    printf("File Type: %s\n", getType(fileInfo));
    printf("Permissions: %s\n", permissions);
    printf("Number of Hard Links: %s\n", getLinkCount(fileInfo));
    printf("UID: %s\n", getUid(fileInfo));
    printf("GID: %s\n", getGid(fileInfo));
    printf("File Size: %s\n", getSize(fileInfo, opts.human_readable));
    printf("Last Access Time: %s\n", format_time(fileInfo.st_atime));
    printf("Last Modification Time: %s\n", format_time(fileInfo.st_mtime));
    printf("Last Status Change Time: %s\n", format_time(fileInfo.st_ctime));
}




void print_json(struct stat fileInfo, char *file_path) {
    char permissions[10];
    print_permissions(fileInfo, permissions); 

    printf("{\n");
    printf("  \"filePath\": \"%s\",\n", file_path);
    printf("  \"inode\": {\n");
    printf("    \"number\": \"%s\",\n", getNumber(fileInfo));
    printf("    \"type\": \"%s\",\n", getType(fileInfo));
    printf("    \"permissions\": \"%s\",\n", permissions);
    printf("    \"linkCount\": \"%s\",\n", getLinkCount(fileInfo));
    printf("    \"uid\": \"%s\",\n", getUid(fileInfo));
    printf("    \"gid\": \"%s\",\n", getGid(fileInfo));
    printf("    \"size\": \"%s\",\n", getSize(fileInfo, opts.human_readable));
    printf("    \"accessTime\": \"%s\",\n", format_time(fileInfo.st_atime));
    printf("    \"modificationTime\": \"%s\",\n", format_time(fileInfo.st_mtime));
    printf("    \"statusChangeTime\": \"%s\"\n", format_time(fileInfo.st_ctime));
    printf("  }\n");
    printf("}\n");
}


int main(int argc, char *argv[]) {
    struct stat fileInfo;
    parseargs(argc, argv);  // Parse command-line arguments

    if (opts.path == NULL) {
        fprintf(stderr, "No file path specified.\n");
        return 1;
    }

    if (stat(opts.path, &fileInfo) != 0) {
        fprintf(stderr, "Error getting file info for %s: %s\n", opts.path, strerror(errno));
        return 1;
    }

    if (opts.json_format) {
        print_json(fileInfo, opts.path);
    } else {
        print_human_readable(fileInfo, opts.path);
    }

    return 0;
}

// Implement other functions...
