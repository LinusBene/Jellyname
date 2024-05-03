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

uint16_t getNumber(const char* str) {
    
    uint8_t i;
    for(i = 0; str[i] != '\0'; i++) {
        if(str[i + 1] < '0' && str[i + 1] > '9' && str[i + 2] < '0' && str[i + 2] > '9') {
            break; 
        }
    }

    char numbers[i];
    strncpy(numbers, str, i);

    return (uint16_t) atoi(numbers);
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

    int16_t season = -1;
    int16_t episode = -1;
    for(int i = 0; currentName[i] != '\0'; i++) {
        if(currentName[i] == 'S') {
            season = getNumber(&currentName[i + 1]);
        }else if (currentName[i] == 'E') {
            episode = getNumber(&currentName[i + 1]);
        }
    }

    if(season == -1 || episode == -1)
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

int main(int argc, char * argv[]) {

    int opt;
    uint8_t approvedFlag = 0;

    while ((opt = getopt(argc, argv, "hvy")) != -1) {
        switch (opt) {
            case 'h':
                print_help();
                return 0;
            case 'v':
                printf("Version 1.0\n");
                return 0;
            case 'y':
                approvedFlag = 1;
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
    char* pattern = approvedFlag ? argv[2] : argv [1];
    char* dirTarget = approvedFlag ? argv[3] : argv [2];

    if(argc < 3 || argc > 4) {
        fprintf(stderr, "Exactly two arguments are required\n");
        print_help();
    }

    DIR *dir; 

    struct dirent *entry;
    struct stat statbuf;

    dir = opendir(dirTarget);
    if(dir == NULL) {
        fprintf(stderr, "Unable to open directory\n");
        return EXIT_FAILURE;
    }

    while ((entry = readdir(dir)) != NULL) {
        // Ignore "." and ".." entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        // Get file information
        if (lstat(entry->d_name, &statbuf) == -1) {
            fprintf(stderr, "Error getting file information\n");
            continue;
        }

        if (S_ISREG(statbuf.st_mode)) {
            if(is_video_file(entry->d_name)){
                char* newFileName = getNewName(entry->d_name, pattern);
                uint8_t approved = approvedFlag;
                
                if(!approved) {
                    char * temp = "Rename '%s' to '%s'? [Y/n]?\t";
                    char msg[strlen(temp) + strlen(entry->d_name) + strlen(newFileName)];
                    sprintf(msg, temp, entry->d_name, newFileName);
                    approved = userConfirm(msg);
                }

                if(approved){
                    rename(entry->d_name, newFileName);
                }

                free(newFileName);
            }
        }
    }

    // Close the directory
    closedir(dir);

    return EXIT_SUCCESS;
}
