#ifndef NQP_SHELL_H_
#define NQP_SHELL_H_

#define MAX_LINE_SIZE 256


typedef struct PATH_T {
    char* stack[1024];
    int top;
} path_t;



char* get_parent_path(char* path);
char** split_str(const char* str, const char* delim, int* len);
int pwd(void);
int ls(char* dir);
int cd(char* dir);

path_t* relative_path_to_absolute(char* dir);
#endif // NQP_SHELL_H_
