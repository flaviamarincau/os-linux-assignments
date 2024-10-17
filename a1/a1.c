#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#define MAX_PATH_LENGTH 1024
#define MAGIC_SIZE 4
#define HEADER_SIZE 2
#define SECT_SIZE 4
#define SECT_OFFSET 4
#define SECT_TYPE 4
#define SECT_NAME 14
#define SECTION_HEADER 26
#define NO_OF_SECTIONS 1
#define VERSION 1


int has_execute_permission(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        return st.st_mode & S_IXUSR;
    }
    return 0; 
}

void list_directory_simple(const char *path, const char *name_starts_with, int has_perm_execute) {
    DIR *dir = NULL;
    struct dirent *entry = NULL;

    dir = opendir(path);
    if (dir == NULL) {
        perror("Could not open directory");
        return;
    }

    printf("SUCCESS\n");

    while ((entry = readdir(dir)) != NULL) {
        char full_path[MAX_PATH_LENGTH];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        if (entry->d_name == NULL) continue; 
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        if (has_perm_execute && has_execute_permission(full_path) == 0)
            continue;
        if (name_starts_with && strncmp(entry->d_name, name_starts_with, strlen(name_starts_with)) != 0)
            continue;

        printf("%s\n", full_path);
    }
    closedir(dir);
}

void list_directory_recursive(const char *path, const char *name_starts_with, int has_perm_execute) {
    DIR *dir = NULL;
    struct dirent *entry = NULL;
    struct stat statbuf;
    static int ok_success = 0;

    dir = opendir(path);
    if (dir == NULL) {
        perror("Could not open directory");
        return;
    }

    if (ok_success == 0) { // i need to print it only once in the beginning
        printf("SUCCESS\n");
        ok_success = 1;
    }

    while ((entry = readdir(dir)) != NULL) {
        char full_path[MAX_PATH_LENGTH] = {0};
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue; // avoid infinite loop
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        if (stat(full_path, &statbuf) != 0 || !(S_ISDIR(statbuf.st_mode) || S_ISREG(statbuf.st_mode)))
            continue;
        if (has_perm_execute && has_execute_permission(full_path) == 0)
            continue;
        if (name_starts_with && strncmp(entry->d_name, name_starts_with, strlen(name_starts_with)) != 0)
            continue;
        printf("%s\n", full_path);
        if (S_ISDIR(statbuf.st_mode))
            list_directory_recursive(full_path, name_starts_with, has_perm_execute);
    }

    closedir(dir);
}

void parse_sf_file(const char *sf_path) {
    int sf = open(sf_path, O_RDONLY);
    if (sf == -1) {
        perror("Error in opening the sf\n");
        return;
    }

    //MAGIC FIELD CORRECT
    char magic[MAGIC_SIZE + 1];
    if (lseek(sf, -MAGIC_SIZE, SEEK_END) == -1 || read(sf, magic, MAGIC_SIZE) != MAGIC_SIZE) {
        printf("ERROR\nfailed to read magic\n");
        close(sf);
        return;
    }
    magic[MAGIC_SIZE] = '\0';

    if (strcmp(magic, "z0Mb") != 0) {
        printf("ERROR\nwrong magic\n");
        close(sf);
        return;
    }

    //HEADER SIZE CORRECT
    uint16_t header_size;
    if (lseek(sf, -(HEADER_SIZE+MAGIC_SIZE), SEEK_END) == -1 || read(sf, &header_size, sizeof(header_size)) != sizeof(header_size)) {

        printf("ERROR\nfailed to read header_size\n");
        close(sf);
        return;
    }
  
     //VERSION
    uint8_t version;
    if (lseek(sf, -(header_size), SEEK_END) == -1 || read(sf, &version, VERSION) != VERSION) {
        printf("ERROR\nfailed to read version\n");
        close(sf);
        return;
    }
    if (version < 24 || version > 110) {
        printf("ERROR\nwrong version\n");
        close(sf);
        return;
    }
  
     //NR SECTIONS
    uint8_t nr_sections;
    if (lseek(sf, -(header_size-VERSION), SEEK_END) == -1 || read(sf, &nr_sections, NO_OF_SECTIONS) != NO_OF_SECTIONS) {
        printf("ERROR\nfailed to read section\n");
         close(sf);
        return;
    }
    if (nr_sections!=2){
    if(nr_sections<4 || nr_sections>19) {
        printf("ERROR\nwrong sect_nr\n");
        close(sf);
        return;
    }
    }
     //SECTIONS
    char sect_name[nr_sections][SECT_NAME + 1];
    uint32_t sect_type[nr_sections];
    uint32_t sect_offset[nr_sections];
    uint32_t sect_size[nr_sections];
    for (int i = 0; i < nr_sections; i++) {
        if (lseek(sf, -header_size + (VERSION +NO_OF_SECTIONS) + (i * SECTION_HEADER), SEEK_END) == -1) {
            printf("ERROR\nfailed to read sect_name\n");
            close(sf);
            return;
        }
        if (read(sf, sect_name[i], SECT_NAME) != SECT_NAME) {
            printf("ERROR\nfailed to read sect_name\n");
            close(sf);
            return;
        }
        sect_name[i][SECT_NAME] = '\0';

        if (read(sf, &sect_type[i], SECT_TYPE) != SECT_TYPE) {
            printf("ERROR\nfailed to read sect_type\n");
            close(sf);
            return;
        }
        if (!(sect_type[i] == 27 || sect_type[i] == 31 || sect_type[i] == 41 || sect_type[i] == 75 || sect_type[i] == 94 || sect_type[i] == 85 || sect_type[i] == 82)) {
            printf("ERROR\nwrong sect_types\n");
            close(sf);
            return;
        }
        if (read(sf, &sect_offset[i], SECT_OFFSET) != SECT_OFFSET) {
            printf("ERROR\nfailed to read sect_offset\n");
            close(sf);
            return;
        }

        if (read(sf, &sect_size[i], SECT_SIZE) != SECT_SIZE) {
            printf("ERROR\nfailed to read sect_size\n");
            close(sf);
            return;
        }

    }
    printf("SUCCESS\n");
    printf("version=%d\nnr_sections=%d\n", version, nr_sections);
    for (int i = 0; i < nr_sections; i++) {
        printf("section%d: %s %d %d\n", i+1, sect_name[i], sect_type[i], sect_size[i]);
    }

    close(sf);
}

void extract_line_from_sf(const char *sf_path, int section_number_extract, int line_number) {
    int sf = open(sf_path, O_RDONLY);
    if (sf == -1) {
        perror("Error in opening the sf\n");
        return;
    }

    //MAGIC FIELD CORRECT
    char magic[MAGIC_SIZE + 1];
    if (lseek(sf, -MAGIC_SIZE, SEEK_END) == -1 || read(sf, magic, MAGIC_SIZE) != MAGIC_SIZE) {
        printf("ERROR\nfailed to read magic\n");
        close(sf);
        return;
    }
    magic[MAGIC_SIZE] = '\0';

    if (strcmp(magic, "z0Mb") != 0) {
        printf("ERROR\nwrong magic\n");
        close(sf);
        return;
    }

    //HEADER SIZE CORRECT
    uint16_t header_size;
    if (lseek(sf, -(HEADER_SIZE+MAGIC_SIZE), SEEK_END) == -1 || read(sf, &header_size, sizeof(header_size)) != sizeof(header_size)) {

        printf("ERROR\nfailed to read header_size\n");
        close(sf);
        return;
    }
  
     //VERSION
    uint8_t version;
    if (lseek(sf, -(header_size), SEEK_END) == -1 || read(sf, &version, VERSION) != VERSION) {
        printf("ERROR\nfailed to read version\n");
        close(sf);
        return;
    }
    if (version < 24 || version > 110) {
        printf("ERROR\nwrong version\n");
        close(sf);
        return;
    }
  
     //NR SECTIONS
    uint8_t nr_sections;
    if (lseek(sf, -(header_size-VERSION), SEEK_END) == -1 || read(sf, &nr_sections, NO_OF_SECTIONS) != NO_OF_SECTIONS) {
        printf("ERROR\nfailed to read section\n");
         close(sf);
        return;
    }
    if (nr_sections!=2){
    if(nr_sections<4 || nr_sections>19) {
        printf("ERROR\nwrong sect_nr\n");
        close(sf);
        return;
    }
    }
     //SECTIONS
    char sect_name[SECT_NAME + 1];
    uint32_t sect_type;
    uint32_t sect_offset;
    uint32_t sect_size;
        if (lseek(sf, -header_size + (VERSION +NO_OF_SECTIONS) + ((section_number_extract-1) * SECTION_HEADER), SEEK_END) == -1) {
            printf("ERROR\nfailed to read sect_name\n");
            close(sf);
            return;
        }
        if (read(sf, sect_name, SECT_NAME) != SECT_NAME) {
            printf("ERROR\nfailed to read sect_name\n");
            close(sf);
            return;
        }
        sect_name[SECT_NAME] = '\0';

        if (read(sf, &sect_type, SECT_TYPE) != SECT_TYPE) {
            printf("ERROR\nfailed to read sect_type\n");
            close(sf);
            return;
        }
        if (!(sect_type== 27 || sect_type == 31 || sect_type == 41 || sect_type == 75 || sect_type == 94 || sect_type== 85 || sect_type == 82)) {
            printf("ERROR\nwrong sect_types\n");
            close(sf);
            return;
        }
        if (read(sf, &sect_offset, SECT_OFFSET) != SECT_OFFSET) {
            printf("ERROR\nfailed to read sect_offset\n");
            close(sf);
            return;
        }

        if (read(sf, &sect_size, SECT_SIZE) != SECT_SIZE) {
            printf("ERROR\nfailed to read sect_size\n");
            close(sf);
            return;
        }

    
  //HERE THE EXTRACT ACTUALLY BEGINS  
int sf_extract = open(sf_path, O_RDONLY);
if(sf_extract == -1) {
    printf("ERROR\nfailed to open sf\n");
    close(sf);
    return;
}
if (lseek(sf_extract, sect_offset, SEEK_SET) == -1) {
printf("ERROR\nfailed to read sect_offset\n");
 close(sf_extract);
        return;
}

char *line = (char *)malloc(sect_size + 1);
if (read(sf_extract, line, sect_size) != (ssize_t)sect_size) {
    printf("ERROR\nfailed to read line\n");
    free(line);
    close(sf_extract);
    close(sf);
    return;
}
line[sect_size] = '\0';


char *next_line = strrchr(line, '\n');
for (int i = 1; i < line_number && next_line != NULL; i++) {
    *next_line = '\0'; 
    next_line = strrchr(line, '\n');
}

if (next_line != NULL) {
    printf("SUCCESS\n%s\n", next_line + 1); // +1 to skip the newline character
} else if (line_number == 1) {
    printf("SUCCESS\n%s\n", line); 
} else {
    printf("ERROR\nLine not found in section\n");
}

free(line);
close(sf_extract);
close(sf);

}


int main(int argc, char **argv) {
    if (argc >= 2 && strcmp(argv[1], "variant") == 0) {
        printf("37024\n");
        return 0;
    }

    if (argc > 1) {
        char *path = NULL;
        char *name_starts_with = NULL;
        int recursive = 0;
        int has_perm_execute = 0;
        int parse = 0;
        int extract = 0;
        int section_number = 0;
        int line_number = 0;

        for (int i = 1; i < argc; i++) {
            if (strncmp(argv[i], "path=", 5) == 0) {
                path = strchr(argv[i], '=') + 1; 
            } else if (strncmp(argv[i], "name_starts_with=", 17) == 0) {
                name_starts_with = strchr(argv[i], '=') + 1; 
            } else if (strcmp(argv[i], "recursive") == 0) {
                recursive = 1; 
            } else if (strcmp(argv[i], "has_perm_execute") == 0) {
                has_perm_execute = 1; 
            } else if (strcmp(argv[i], "parse") == 0) {
                parse = 1;
            }
            else if (strcmp(argv[i], "extract") == 0) {
                extract = 1;
            } else if (strncmp(argv[i], "section=", 8) == 0) {
                section_number = atoi(strchr(argv[i], '=') + 1);
            } else if (strncmp(argv[i], "line=", 5) == 0) {
                line_number = atoi(strchr(argv[i], '=') + 1);
            }
        }

        if (parse) {
            if (path) {
                parse_sf_file(path);
                return 0;
            } else {
                printf("ERROR\nInvalid command\n");
                return 1;
            }
        }

         if (extract) {
            if (path && section_number > 0 && line_number > 0) {
                extract_line_from_sf(path, section_number, line_number);
                return 0;
            } else {
                printf("ERROR\nInvalid command\n");
                return 1;
            }
        }

        if (path) {
            if (recursive != 0) {
                list_directory_recursive(path, name_starts_with, has_perm_execute);
            } else {
                list_directory_simple(path, name_starts_with, has_perm_execute);
            }

            return 0;
        }
    }

    printf("ERROR\nInvalid command\n");
    return 1;
}
