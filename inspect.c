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
    
    // Try to get file info using stat
    if (stat(file_path, &fileInfo) != 0) {
        fprintf(stderr, "Error getting file info for %s: %s\n", file_path, strerror(errno));
        return;
    }
    
    // Depending on the output format, call the appropriate function
    if (opts.json_format) {
        print_json(fileInfo, file_path);
    } else {
        print_human_readable(fileInfo, file_path);
    }
    
    // If the -a or --all flag is set, we need to handle directory listing
    if (S_ISDIR(fileInfo.st_mode) && opts.recursive) {
        

        DIR *dir;
        struct dirent *entry;
        
        if (!(dir = opendir(file_path))) {
            fprintf(stderr, "Failed to open directory %s\n", file_path);
            return;
        }
        
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;
            
            char path[1024];
          

            int len = snprintf(path, sizeof(path)-1, "%s/%s", file_path, entry->d_name);
            path[len] = 0;
            
            inspect_file(path);  // Recursively call inspect_file
        }
        
        closedir(dir);
    }
}

void print_permissions(mode_t mode, char *perm) {
    char types[10] = "---------";
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
char* format_time(time_t time) {
    struct tm *tm_info = localtime(&time);
    static char buff[20];
    strftime(buff, 20, "%Y-%m-%d %H:%M:%S", tm_info);
    return buff;
}


void print_human_readable(struct stat fileInfo, char *file_path) {
    printf("Information for %s:\n", file_path);
    
    // File Type
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
    
    // Permissions
    printf("Permissions: ");
    char perm[10];
    print_permissions(fileInfo.st_mode, perm);
    printf("%s\n", perm);
    
    // Link Count
    printf("Number of Hard Links: %lu\n", fileInfo.st_nlink);
    
    // UID and GID
    struct passwd *pw = getpwuid(fileInfo.st_uid);
    struct group *gr = getgrgid(fileInfo.st_gid);
    printf("Owner: %s (UID = %u)\n", pw ? pw->pw_name : "Unknown", fileInfo.st_uid);
    printf("Group: %s (GID = %u)\n", gr ? gr->gr_name : "Unknown", fileInfo.st_gid);
    
    // File Size
    printf("File Size: ");
    if (opts.human_readable) {
        if (fileInfo.st_size < 1024)
            printf("%ld bytes\n", fileInfo.st_size);
        else if (fileInfo.st_size < 1024 * 1024)
            printf("%.1f KB\n", fileInfo.st_size / 1024.0);
        else
            printf("%.1f MB\n", fileInfo.st_size / (1024.0 * 1024));
    } else {
        printf("%ld bytes\n", fileInfo.st_size);
    }
    
    // Timestamps
    printf("Last Access Time: %s\n", format_time(fileInfo.st_atime));
    printf("Last Modification Time: %s\n", format_time(fileInfo.st_mtime));
    printf("Last Status Change Time: %s\n", format_time(fileInfo.st_ctime));
}

void print_json(struct stat fileInfo, char *file_path) {
    struct passwd *pw = getpwuid(fileInfo.st_uid);
    struct group *gr = getgrgid(fileInfo.st_gid);
    char perm[10];
    print_permissions(fileInfo.st_mode, perm);

    // Ensure file_path is sanitized for JSON to prevent injection issues.
    printf("{\n");
    printf("  \"filePath\": \"%s\",\n", file_path);
    printf("  \"inode\": {\n");
    printf("    \"number\": %lu,\n", fileInfo.st_ino);
    printf("    \"type\": \"%s\",\n",
           S_ISDIR(fileInfo.st_mode) ? "directory" :
           S_ISREG(fileInfo.st_mode) ? "regular file" :
           S_ISCHR(fileInfo.st_mode) ? "character device" :
           S_ISBLK(fileInfo.st_mode) ? "block device" :
           S_ISFIFO(fileInfo.st_mode) ? "FIFO" :
           S_ISLNK(fileInfo.st_mode) ? "symbolic link" :
           S_ISSOCK(fileInfo.st_mode) ? "socket" : "unknown");
    printf("    \"permissions\": \"%s\",\n", perm);
    printf("    \"linkCount\": %lu,\n", fileInfo.st_nlink);
    printf("    \"uid\": %u,\n", fileInfo.st_uid);
    printf("    \"owner\": \"%s\",\n", (pw && pw->pw_name) ? pw->pw_name : "Unknown");
    printf("    \"gid\": %u,\n", fileInfo.st_gid);
    printf("    \"group\": \"%s\",\n", (gr && gr->gr_name) ? gr->gr_name : "Unknown");
    printf("    \"size\": \"%s\",\n",
           opts.human_readable ?
           (fileInfo.st_size < 1024 ? "1K" :
            (fileInfo.st_size < 1024 * 1024 ? "1M" : "1G")) : "bytes");
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
