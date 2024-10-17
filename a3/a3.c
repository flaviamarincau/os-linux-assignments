#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdint.h>
#include <sys/types.h>


#define REQ_PIPE_NAME "REQ_PIPE_37024"
#define RESP_PIPE_NAME "RESP_PIPE_37024"
#define CREATE_SHM_NO_BYTES 3594660
#define CREATE_SHM_FILE_NAME "/zK6HDGQI"
#define BUFFER_SIZE 256
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

void handle_error(const char* message) {
    perror(message);
    exit(EXIT_FAILURE);
}

//string field
void write_string_field(int fd, const char* str) {
    if (write(fd, str, strlen(str)) != (ssize_t)strlen(str) ||
        write(fd, "#", 1) != 1) {
        handle_error("ERROR\ncannot write string field to the response pipe");
    }
}

//number field
void write_number_field(int fd, unsigned int number) {
    if (write(fd, &number, sizeof(number)) != sizeof(number)) {
        handle_error("ERROR\ncannot write number field to the response pipe");
    }
}

int main() {
    char buffer[BUFFER_SIZE];

    if (mkfifo(RESP_PIPE_NAME, 0644) == -1) {
        handle_error("ERROR\ncannot create the response pipe");
    }

    int req_fd = open(REQ_PIPE_NAME, O_RDONLY);
    if (req_fd == -1) {
        handle_error("ERROR\ncannot open the request pipe");
    }

    int resp_fd = open(RESP_PIPE_NAME, O_WRONLY);
    if (resp_fd == -1) {
        handle_error("ERROR\ncannot open the response pipe");
    }
    write_string_field(resp_fd, "CONNECT");
    printf("SUCCESS\n");

    void* shm_ptr = NULL;
    unsigned int shm_size = 0;
    int shm_fd;
    void* file_ptr = NULL;
    unsigned int file_size = 0;
    char* file_name_start = 0;
    int file_fd;

    while (1) {
        ssize_t count = read(req_fd, buffer, BUFFER_SIZE - 1);
        if (count == -1) {
            handle_error("ERROR\ncannot read from the request pipe");
        } else if (count == 0) {
            break;
        }

        buffer[count] = '\0';

        //"ECHO"
        if (strcmp(buffer, "ECHO#") == 0) {
            write_string_field(resp_fd, "ECHO");
            write_string_field(resp_fd, "VARIANT");
            write_number_field(resp_fd, 37024);
        }
        //"EXIT"
        else if (strcmp(buffer, "EXIT#") == 0) {
            close(req_fd);
            close(resp_fd);
            unlink(RESP_PIPE_NAME);
            return 0;
        }
        //"CREATE_SHM"
        else if (strncmp(buffer, "CREATE_SHM#", 11) == 0) {
            shm_size = sizeof(char)*CREATE_SHM_NO_BYTES;
            shm_fd = shm_open(CREATE_SHM_FILE_NAME, O_CREAT | O_RDWR, 0664);
            if (shm_fd == -1 || ftruncate(shm_fd, shm_size) == -1 || 
                (shm_ptr = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == MAP_FAILED) {
                write_string_field(resp_fd, "CREATE_SHM");
                write_string_field(resp_fd, "ERROR");
            } else {
                write_string_field(resp_fd, "CREATE_SHM");
                write_string_field(resp_fd, "SUCCESS");
            }
        }
        //"WRITE_TO_SHM"
        else if (strncmp(buffer, "WRITE_TO_SHM#", 13) == 0) {
            unsigned int offset = *(unsigned int*)(buffer + 13);  
            unsigned int value = *(unsigned int*)(buffer + 17);   
            if (offset + sizeof(value) > shm_size) {
                write_string_field(resp_fd, "WRITE_TO_SHM");
                write_string_field(resp_fd, "ERROR");
            } else {
                memcpy(shm_ptr + offset, &value, sizeof(value));
                write_string_field(resp_fd, "WRITE_TO_SHM");
                write_string_field(resp_fd, "SUCCESS");
            }
        }
        //"MAP_FILE"
        else if (strncmp(buffer, "MAP_FILE#", 9) == 0) {
            file_name_start = buffer + 9;
            char* file_name_end = strchr(file_name_start, '#'); 
            if (file_name_end) {
                *file_name_end = '\0'; 
                file_fd = open(file_name_start, O_RDONLY);
                struct stat st;
                if (file_fd == -1 || fstat(file_fd, &st) == -1 || 
                    (file_ptr = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, file_fd, 0)) == MAP_FAILED) {
                    write_string_field(resp_fd, "MAP_FILE");
                    write_string_field(resp_fd, "ERROR");
                } else {
                    file_size = st.st_size;
                    write_string_field(resp_fd, "MAP_FILE");
                    write_string_field(resp_fd, "SUCCESS");
                }
            } else {
                write_string_field(resp_fd, "MAP_FILE");
                write_string_field(resp_fd, "ERROR");
            }
        }
         //"READ_FROM_FILE_OFFSET"
        else if (strncmp(buffer, "READ_FROM_FILE_OFFSET#", 22) == 0) {
            unsigned int offset = *(unsigned int*)(buffer + 22);  
            unsigned int no_of_bytes = *(unsigned int*)(buffer + 26);  
            if (shm_ptr == NULL || file_ptr == NULL || offset + no_of_bytes > file_size || no_of_bytes > shm_size) {
                write_string_field(resp_fd, "READ_FROM_FILE_OFFSET");
                write_string_field(resp_fd, "ERROR");
            } else {
                memcpy(shm_ptr, (char*)file_ptr + offset, no_of_bytes);
                write_string_field(resp_fd, "READ_FROM_FILE_OFFSET");
                write_string_field(resp_fd, "SUCCESS");

            }
        }
     else if (strncmp(buffer, "READ_FROM_FILE_SECTION#", 23) == 0) {
    unsigned int section_no = *(unsigned int*)(buffer + 23);    
    unsigned int offset = *(unsigned int*)(buffer + 27);       
    unsigned int no_of_bytes = *(unsigned int*)(buffer + 31);  

    if (file_ptr == NULL) {
        write_string_field(resp_fd, "READ_FROM_FILE_SECTION");
        write_string_field(resp_fd, "ERROR");
        return 0;
    }

    uint16_t header_size;
    uint8_t nr_sections;
    memcpy(&header_size, (char*)file_ptr + file_size - MAGIC_SIZE - HEADER_SIZE, HEADER_SIZE);
    memcpy(&nr_sections, (char*)file_ptr + file_size - header_size + VERSION, NO_OF_SECTIONS);

    if (section_no == 0 || section_no > nr_sections) {
        write_string_field(resp_fd, "READ_FROM_FILE_SECTION");
        write_string_field(resp_fd, "ERROR");
        return 0;
    }

    off_t section_header_offset = file_size - header_size + VERSION + NO_OF_SECTIONS + ((section_no - 1) * SECTION_HEADER);
    uint32_t section_offset, section_size;
    memcpy(&section_offset, (char*)file_ptr + section_header_offset + SECT_OFFSET, sizeof(uint32_t));
    memcpy(&section_size, (char*)file_ptr + section_header_offset + SECT_SIZE, sizeof(uint32_t));

    if (offset + no_of_bytes > section_size || no_of_bytes > shm_size) {
        write_string_field(resp_fd, "READ_FROM_FILE_SECTION");
        write_string_field(resp_fd, "ERROR");
        return 0;
    } else {
        char* section_start = (char*)file_ptr + section_offset + offset;
        char* section_end = section_start + no_of_bytes;

        if (section_end > (char*)file_ptr + file_size) {
            write_string_field(resp_fd, "READ_FROM_FILE_SECTION");
            write_string_field(resp_fd, "ERROR");
            return 0;
        } else {
            memcpy(shm_ptr, section_start, no_of_bytes);
            write_string_field(resp_fd, "READ_FROM_FILE_SECTION");
            write_string_field(resp_fd, "SUCCESS");
        }
    }
}
      else if (strncmp(buffer, "READ_FROM_LOGICAL_SPACE_OFFSET#", 31) == 0) {
        unsigned int logical_offset = *(unsigned int*)(buffer + 31);         
        unsigned int no_of_bytes = *(unsigned int*)(buffer + 35);  
        logical_offset++;
        no_of_bytes++;
        file_fd = open(file_name_start, O_RDONLY);
        if(file_fd == -1) {
        write_string_field(resp_fd, "READ_FROM_LOGICAL_SPACE_OFFSET");
        write_string_field(resp_fd, "ERROR");
       }
       int size = lseek(file_fd, 0, SEEK_END); //find out the size of the file
       lseek(file_fd, 0, SEEK_SET);
       file_ptr = mmap(NULL, size, PROT_READ, MAP_PRIVATE, file_fd, 0);
         if(file_ptr == MAP_FAILED) {
          write_string_field(resp_fd, "READ_FROM_LOGICAL_SPACE_OFFSET");
          write_string_field(resp_fd, "ERROR");
         }
      }

    }
    close(req_fd);
    close(resp_fd);
    return 0;
}