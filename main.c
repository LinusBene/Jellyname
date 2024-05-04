#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "stdlib.h"
#include "unistd.h"
#include "dirent.h"
#include <sys/stat.h>

void print_help() {
    printf("Usage: jellyname [options] targetName directory\n");
    printf("Options:\n");
    printf("  -h Print this help message\n");
    printf("  -v Print version information\n");
    printf("  -y yes to all\n");
    printf("  -r recursive calls to subdirectories\n");
}

int is_video_file(const char *filename) {
    const char *video_extensions[] = {
        ".mp4", ".avi", ".mkv", ".mov", ".wmv", ".flv", ".webm", ".m4v"
    };
    int num_video_extensions = sizeof(video_extensions) / sizeof(video_extensions[0]);

    const char *extension = strrchr(filename, '.');
    if (extension != NULL) {
        for (int i = 0; i < num_video_extensions; ++i) {
            if (strcasecmp(extension, video_extensions[i]) == 0) {
                return 1; 
            }
        }
    }
    return 0; 
}

uint8_t getNumber(const char* str, uint16_t* res) {
    
    if(str[0] < '0' || str[0] > '9') 
        return EXIT_FAILURE;

    uint8_t i;
    for(i = 0; str[i] != '\0'; i++) {
        if(str[i] < '0' || str[i] > '9') {
            break;
        }
    }

    char numbers[i];
    strncpy(numbers, str, i);
    *res = (uint16_t) atoi (numbers);
    return EXIT_SUCCESS;
}

uint8_t userConfirm(const char* msg) {
    char res = '0';
    printf("%s\n", msg);
    while(tolower(res) != 'n' && tolower(res) != 'y') {
        scanf("%c", &res);
    }

    return res == 'y' ? 1 : 0;
}

char* getNewName(const char* currentName, const char* pattern) {

    const char *extension = strrchr(currentName, '.');

    uint16_t season = 0xFFFF;
    uint16_t episode = 0xFFFF;
    for(int i = 0; currentName[i] != '\0'; i++) {
        if(currentName[i] == 'S' && season == 0xFFFF) {
            if(getNumber(&currentName[i+1], &season) == EXIT_SUCCESS) {
                if(episode != 0xFFFF)
                    break;
            }
        }else if (currentName[i] == 'E' && episode == 0xFFFF) {
            if(getNumber(&currentName[i+1], &episode) == EXIT_SUCCESS) {
                if(season != 0xFFFF)
                    break;
            }
        }
    }

    if(season == 0xFFFF || episode == 0xFFFF)
        return "null";

    const size_t resLength = strlen(extension) + strlen(currentName) + 9;
    char* res = (char*) malloc(resLength * sizeof(char));

    if (res == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    sprintf(res, "%s S%02dE%02d%s", pattern, season, episode, extension);
    return res;
}

int renameFiles(const char* pattern, const char* dirTarget, uint8_t recursiveFlag, uint8_t approvedFlag) {
    DIR *dir; 

    struct dirent *entry;
    struct stat statbuf;

    dir = opendir(dirTarget);
    if(dir == NULL) {
        fprintf(stderr, "Unable to open directory\n");
        return EXIT_FAILURE;
    }

    size_t dirPathLength = strlen(dirTarget);
    
    char filePath[dirPathLength + 150]; //set max file name length to 150
    strcpy(filePath, dirTarget);

    // appending '/' to end if dir path
    if(dirTarget[dirPathLength - 1] != '/'){
        filePath[dirPathLength] = '/';
        filePath[dirPathLength+1] = '\0';
        dirPathLength = dirPathLength + 1;
    }

    char newFilePath[dirPathLength + 150]; //set max file name length to 150
    strcpy(newFilePath, filePath);
    
    while ((entry = readdir(dir)) != NULL) {
        // Ignore "." and ".." entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        strcpy(filePath + dirPathLength, entry->d_name);

        // Get file information
        if (lstat(filePath, &statbuf) == -1) {
            fprintf(stderr, "Error getting file information\n");
            continue;
        }

        if (S_ISREG(statbuf.st_mode)) {
            if(is_video_file(entry->d_name)){
                char *newFileName = getNewName(entry->d_name, pattern);

                if(strcmp(newFileName, "null") == 0){
                    continue;
                }

                strcpy(&newFilePath[dirPathLength], newFileName);
                uint8_t approved = approvedFlag;
                
                if(!approved) {
                    char * temp = "Rename '%s' to '%s'? [y/n]?\t";
                    char msg[strlen(temp) + strlen(entry->d_name) + strlen(newFileName)];
                    sprintf(msg, temp, entry->d_name, newFileName);
                    approved = userConfirm(msg);
                }

                if(approved && strcmp(newFileName, "null") != 0){
                    int res = rename(filePath, newFilePath);
                    if(res == -1) {
                        fprintf(stderr, "Error renaming the file\n");
                    }
                }

                free(newFileName);
            }
        }else if (recursiveFlag && S_ISDIR(statbuf.st_mode)) {
            renameFiles(pattern, filePath, recursiveFlag, approvedFlag);
        }
    }

    // Close the directory
    closedir(dir);
    
    return EXIT_SUCCESS;
}

int main(int argc, char * argv[]) {
    int opt;
    uint8_t approvedFlag = 0;
    uint8_t recursiveFlag = 0;
    uint8_t optionsSet = 0;

    while ((opt = getopt(argc, argv, "hvyr")) != -1) {
        switch (opt) {
            case 'h':
                print_help();
                return 0;
            case 'v':
                printf("Version 1.0\n");
                return 0;
            case 'y':
                approvedFlag = 1;
                optionsSet = optionsSet + 1;
                break;
            case 'r':
                optionsSet = optionsSet + 1;
                recursiveFlag = 1;
                break;
            case '?':
                fprintf(stderr, "Invalid option: -%c\n", optopt);
                print_help();
                return 1;
            default:
                // Should not reach here
                abort();
        }
    }

    char* pattern = argv[1 + optionsSet];
    char* dirTarget = argv[2 + optionsSet];

    if(argc - 1 - optionsSet != 2) {
        fprintf(stderr, "Exactly two arguments are required\n");
        print_help();
    }

    return renameFiles(pattern, dirTarget, recursiveFlag, approvedFlag);
}
