#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h> 
// Global item structure
typedef struct {
    char *path;
    int humanReadable;
    int json_format;
    char *log_file;
    int recursive;
} items;

items opts;

// Function declarations
void parseargs(int argc, char *argv[]);
void displayHelp();
void inspectFile(char *filePath);
void print_json(struct stat fileInfo, char *filePath);
void printHumanRead(struct stat fileInfo, char *filePath);
void inspectDirectory(char *basePath);
void printPerm(struct stat fileInfo, char *perm);
void processDirectory(const char *dirPth, int recursive);
unsigned long getNumber(struct stat fileInfo);
char* getType(struct stat fileInfo);
unsigned int getLink(struct stat fileInfo);
unsigned int getUid(struct stat fileInfo);
unsigned int getGid(struct stat fileInfo);
char* getSize(struct stat fileInfo, int humanReadable);
void printError();
void redirect(const char* logFilePath);


void parseargs(int argc, char *argv[]) {//parsingthe arguments
     memset(&opts, 0, sizeof(items));//setting the values to zero

    for (int i = 1; i < argc; i++) {//for loop to check for arguments
           if (strcmp(argv[i], "-?") == 0 || strcmp(argv[i], "--help") == 0) {
            displayHelp();
            exit(EXIT_SUCCESS);//display help and exit
        } else if ((strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--inode") == 0) && i + 1 < argc) {
            opts.path = argv[++i];//path if i and inode
        } else if 
        ((strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--all") == 0)) {
            opts.path = (i + 1 < argc) ? argv[++i] : ".";
            opts.recursive = 0;//change back to non recursive
        } else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--recursive") == 0) {
            opts.recursive = 1;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--human") == 0) {
            opts.humanReadable = 1;
        } else if ((strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--format") == 0) && i + 1 < argc) {
            if (strcmp(argv[i+1], "json") == 0) {
                opts.json_format = 1;
            }
            i++;
        } else if ((strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--log") == 0) && i + 1 < argc) {
            opts.log_file = argv[++i];
        } else {
            fprintf(stderr, "Unrecognized option: %s\n", argv[i]);//check for unrecognized options
            displayHelp();
            exit(EXIT_FAILURE);
        }
    }

    if (opts.path && strcmp(argv[argc-2], "-a") == 0) {//extension for path a and recursive
        inspectDirectory(opts.path);
        exit(EXIT_SUCCESS);
    }

    if (opts.path == NULL) {//error message if no path is specified
        fprintf(stderr, "No file or directory path specified.\n");
        displayHelp();
        exit(EXIT_FAILURE);
    }
}

void displayHelp() {//help message 
    printf("Usage: inspect [options]\n");
    printf("Options:\n");
    printf(" -?, --help                 Display this help message.\n");
    printf(" -i, --inode <filePath>    Display detailed inode information for the specified file.\n");
    printf(" -a, --all [directory_path] Display inode information for all files in the specified directory.\n");
    printf(" -r, --recursive            List information recursively for directories.\n");
    printf(" -h, --human                Output changes sizes and dates in human-readable form.\n");
    printf(" -f, --format [text|json]   Specify output format, default is text.\n");
    printf(" -l, --log <log_file>       Log operations to the specified file.\n");
}
void inspectFile(char *filePath) {//
    struct stat fileInfo;//structure for file info
    if (stat(filePath, &fileInfo) != 0) {//
        fprintf(stderr, "Error getting file info for %s: %s\n", filePath, strerror(errno));//error message for no file info
        return;
    }

    if (opts.json_format) {//grabbing the json format
        print_json(fileInfo, filePath);
    } else {
        printHumanRead(fileInfo, filePath);//grabbing the human readable format
    }
}
unsigned long getNumber(struct stat fileInfo) {
    return fileInfo.st_ino;  
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

unsigned int getLink(struct stat fileInfo) {
    return fileInfo.st_nlink;  
}

unsigned int getUid(struct stat fileInfo) {
    return fileInfo.st_uid;
}

unsigned int getGid(struct stat fileInfo) {
    return fileInfo.st_gid;
}



void printPerm(struct stat fileInfo, char *perm) {//checking and printing permissions
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


char* getSize(struct stat fileInfo, int humanReadable) {
    static char size[64];  // Buffer to store the size string
    if (humanReadable) {
        if (fileInfo.st_size < 1024) {
            sprintf(size, "%ldB", fileInfo.st_size); // Bytes
        } else if (fileInfo.st_size < 1024 * 1024) {
            sprintf(size, "%ldK", fileInfo.st_size / 1024); // Kilobytes
        } else if (fileInfo.st_size < 1024 * 1024 * 1024) {
            sprintf(size, "%ldM", fileInfo.st_size / (1024 * 1024)); // Megabytes
        } else {
            sprintf(size, "%ldG", fileInfo.st_size / (1024 * 1024 * 1024)); // Gigabytes
        }
    } else {
        sprintf(size, "%ld", fileInfo.st_size);  // Print size in bytes only
    }
    return size;
}


char* format_time(time_t time, int humanReadable) {//making the time human readable
    static char buff[64];
    if (humanReadable) {
        struct tm *tm_info = localtime(&time);
        strftime(buff, sizeof(buff), "%Y-%m-%d %H:%M:%S", tm_info);
    } else {
        sprintf(buff, "%ld", (long)time);  // Output raw time
    }
    return buff;
}

void inspectDirectory(char *basePath) {
    if (basePath == NULL || strlen(basePath) == 0) {
        // defaulting to current directory if none specified
        basePath = ".";
    }

    processDirectory(basePath, opts.recursive);
}

void processDirectory(const char *dirPth, int recursive) {
    DIR *dir = opendir(dirPth);
    if (dir == NULL) {
        perror("error opening directory");
        return;
    }

    if (opts.json_format) {
        printf("[\n");  // Start of JSON array
    }

    struct dirent *entry;
    int firstIn = 1;  // Flag for comma placement
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", dirPth, entry->d_name); //constructing path

        struct stat fileInfo;
        if (stat(path, &fileInfo) != 0) {
            fprintf(stderr, "error getting file info for %s: %s\n", path, strerror(errno));//error message 
            continue;
        }

        // Print comma after first output
        if (opts.json_format && !firstIn) {
            printf(",\n");
        } else {
            firstIn = 0;  // reset flag afeter first output
        }

        if (opts.json_format) {
            print_json(fileInfo, path);
        } else {
            printHumanRead(fileInfo, path);
        }

        // recurse into subdirectories
        if (S_ISDIR(fileInfo.st_mode) && recursive) {
            processDirectory(path, recursive);
        }
    }

    if (opts.json_format) {
        printf("\n]"); // close json array
    }

    closedir(dir);
}

void printError() {
    fprintf(stderr, "Error redirecting output: %s\n", strerror(errno));
}
void redirect(const char* logFilePath) {//redirection for log file
    if (logFilePath != NULL) {
        
        int outFileno = open(logFilePath, O_WRONLY | O_CREAT | O_APPEND, 0666);
        if (outFileno == -1) {
            printError();
            exit(EXIT_FAILURE);
        }

    

        if (dup2(outFileno, STDOUT_FILENO) == -1) {
            printError();
            exit(EXIT_FAILURE);
        }

        // duplicate the file descriptor to stderr
        if (dup2(outFileno, STDERR_FILENO) == -1) {
            printError();
            exit(EXIT_FAILURE);
        }

        // close the original file descriptor 
        close(outFileno);
    }
}


void printHumanRead(struct stat fileInfo, char *filePath) {//making numbers to readable format
    printf("Information for %s:\n", filePath);

    char permissions[10];
    printPerm(fileInfo, permissions); 

    printf("File Type: %s\n", getType(fileInfo));
    printf("Permissions: %s\n", permissions);
    printf("Number of Hard Links: %u\n", getLink(fileInfo));
    printf("UID: %u\n", getUid(fileInfo));
    printf("GID: %u\n", getGid(fileInfo));
    printf("File Size: %s\n", getSize(fileInfo, opts.humanReadable));
printf("Last Access Time: %s\n", format_time(fileInfo.st_atime, opts.humanReadable));
printf("Last Modification Time: %s\n", format_time(fileInfo.st_mtime, opts.humanReadable));
printf("Last Status Change Time: %s\n", format_time(fileInfo.st_ctime, opts.humanReadable));

}




void print_json(struct stat fileInfo, char *filePath) {//ensuring json format
    char permissions[10];
    printPerm(fileInfo, permissions); 

    printf("{\n");
    printf("  \"filePath\": \"%s\",\n", filePath);
    printf("  \"inode\": {\n");
    printf("    \"number\": %lu,\n", getNumber(fileInfo));
    printf("    \"type\": \"%s\",\n", getType(fileInfo));
    printf("    \"permissions\": \"%s\",\n", permissions);
    printf("    \"linkCount\": %u,\n", getLink(fileInfo));
    printf("    \"uid\": %u,\n", getUid(fileInfo));
    printf("    \"gid\": %u,\n", getGid(fileInfo));
    printf("    \"size\": \"%s\",\n", getSize(fileInfo, opts.humanReadable));
    printf("    \"accessTime\": \"%s\",\n", format_time(fileInfo.st_atime, opts.humanReadable));
    printf("    \"modificationTime\": \"%s\",\n", format_time(fileInfo.st_mtime, opts.humanReadable));
    printf("    \"statusChangeTime\": \"%s\"\n", format_time(fileInfo.st_ctime, opts.humanReadable));
    printf("  }\n");
    printf("}\n");
}



int main(int argc, char *argv[]) {
    parseargs(argc, argv);  // parse arguments

    //logging if needed
    if (opts.log_file != NULL) {
        redirect(opts.log_file);
    }

    // check path
    if (opts.path == NULL) {
        fprintf(stderr, "No file or directory path specified.\n");
        displayHelp();
        exit(EXIT_FAILURE);
    }

    // check for file or directory
    struct stat fileInfo;
    if (stat(opts.path, &fileInfo) != 0) {
        fprintf(stderr, "Error getting file info for %s: %s\n", opts.path, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (S_ISDIR(fileInfo.st_mode) || opts.recursive) {
        // process directory for directory or recursive 
        processDirectory(opts.path, opts.recursive);
    } else {
        // process a single file
        if (opts.json_format) {
            print_json(fileInfo, opts.path);
        } else {
            printHumanRead(fileInfo, opts.path);
        }
    }

    return 0;// all done  
}
