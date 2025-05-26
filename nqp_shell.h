#ifndef NQP_SHELL_H_
#define NQP_SHELL_H_

#define MAX_LINE_SIZE 256

/* Current Working Directory Struct */
typedef struct CURRENT_WORKING_DIRECTORY
{
    int fd;
    char path[MAX_LINE_SIZE];
} current_working_directory;


char* get_parent_path(char* path);
char** split_str(char* str, const char delim, int* len);
int pwd(void);
int ls(char* dir);
int cd(char* dir);

char* relative_path_to_absolute(char* dir);
#endif // NQP_SHELL_H_
