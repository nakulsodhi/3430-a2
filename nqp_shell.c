#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include "nqp_shell.h"
#include "nqp_io.h"

path_t cwd;

char* stack_join(path_t* path)
{
    char* path_ret;
    int pathsize = 1;

    if (path->top == -1) return "/";

    for (int i = 0; i <= path->top; i++)
    {

        pathsize += strlen(path->stack[i]) + 1;
    }

    path_ret = calloc(pathsize, sizeof(char));

    strcpy(path_ret, "/");

    for (int i = 0; i <= path->top; i++)
    {
        strcat(path_ret, path->stack[i]);
        strcat(path_ret, "/");
    }

    return path_ret;
}

char* path_join_parent(path_t* path)
{
    char* path_ret;
    int pathsize = 1;

    if ((path->top == -1) || (path->top == 0)) return "/";

    for (int i = 0; i <= path->top - 1; i++)
    {

        pathsize += strlen(path->stack[i]) + 1;
    }

    path_ret = calloc(pathsize, sizeof(char));

    strcpy(path_ret, "/");

    for (int i = 0; i <= path->top - 1; i++)
    {
        strcat(path_ret, path->stack[i]);
        strcat(path_ret, "/");
    }

    return path_ret;
}

void stack_push(path_t* path, char* token)
{
    path->top++;
    path->stack[path->top] = token;

}

char* stack_pop(path_t* path)
{
    return path->stack[path->top--];
}


void stack_clear(path_t* path)
{
    if (path->top == -1) return;
    while (path->top > -1)
    {
        free(path->stack[path->top]);
        path->top --;
    }
}


path_t* relative_path_to_absolute(char* dir){

    /* Still doesn't handle the case where the path is a relative path starting with .. or . */


        if (dir[0] == '/')      /* Absolute path */
        {
            path_t* tmp = malloc(sizeof(path_t));
            tmp->top = -1;

            int count = 0;
            char** abs_path_tokens = split_str(dir, '/', &count);

            for (int i = 0; i < count; i++)
            {
                stack_push(tmp, abs_path_tokens[i]);
            }
            return tmp;
        }
        else if (strcmp(dir, "..") == 0)  /* Parent path */
        {
            path_t* tmp = malloc(sizeof(path_t));
            memcpy(tmp, &cwd, sizeof(path_t));
            stack_pop(tmp);
            return tmp;
        }
        else if (strcmp(dir, ".") == 0)
        {
            return &cwd;
        }
        else                    /* Relative path */
        {
            int count = 0;
            char** abs_path_tokens = split_str(dir, '/', &count);

            path_t* tmp = malloc(sizeof(path_t));
            memcpy(tmp, &cwd, sizeof(path_t));
            for (int i = 0; i < count; i++)
            {
                stack_push(tmp, abs_path_tokens[i]);
            }


            return tmp;
        }
}



/* char* get_parent_path(char* path) */
/* { */
/*     if (strcmp(path, "/") == 0) { */
/*         return path; */
/*     } */

/*     int depth = 0; */

/*     if (depth == 1)             /\* The parent is root *\/ */
/*     { */
/*         return "/"; */

/*     } */
/*     int path_length = depth * sizeof(char); */
/*     for (int i = 0; i < depth - 1; i++) */
/*     { */
/*         path_length += strlen(stack[i]); */

/*     } */
/*     char* parent_path = calloc(path_length, sizeof(char)); */
/*     int idx = 0; */
/*     for (int i = 0; i < depth - 1; i++) */
/*     { */
/*         strcpy((parent_path + idx++), "/"); */
/*         strcpy((parent_path + idx), cwd_split[i]); */
/*         idx += strlen(cwd_split[i]); */
/*     } */


/*     return parent_path; */
/* } */

char** split_str(char* str, const char delim, int* len)
{
    char** ret = NULL;
    char* tmp = str;
    int str_idx = 0;
    int token_count = 0;
    char* subtoken;
    char* saveptr;
    *len = 0;


    /* Get the number of tokens. */
    while (*tmp) {
        if (delim == *tmp) {
            token_count++;
        }
        tmp++;
    }

    tmp = str; /* Reset the pointer to the start of the temp string */
    ret = calloc(token_count, sizeof (char*));
    for (subtoken = strtok_r(tmp, &delim , &saveptr);
         subtoken != NULL;
         subtoken = strtok_r(NULL, &delim, &saveptr))
    {
        *len = *len + 1;
        *(ret + str_idx++) = strdup(subtoken);

    }

    free(subtoken);

    return ret;
}

int pwd(void)
{
    printf("%s \n", stack_join(&cwd));
    return 0;
}


int ls(char* dir)
{
    nqp_dirent entry = {0};
    ssize_t dirents_read;
    int target_fd = -1;

    if (dir != NULL)
    {
        if (dir[0] == '/')      /* Absolute path */
        {
            target_fd = nqp_open(dir);
        }
        else if (strcmp(dir, "..") == 0)  /* Parent path */
        {
            target_fd = nqp_open(path_join_parent(&cwd));
        }
        else if (strcmp(dir, ".") == 0)
        {
            target_fd = nqp_open(stack_join(&cwd));
        }
        else                    /* Relative path */
        {
            char concat_path[strlen(stack_join(&cwd)) + strlen(dir) + 1];

            strcpy(concat_path, stack_join(&cwd));
            strcat(concat_path, dir);

            target_fd = nqp_open(concat_path);

        }

    }
    else
    {
        target_fd = nqp_open(stack_join(&cwd));
    }
    if (target_fd < 0)
    {
        return -1;
    }
    while ((dirents_read = nqp_getdents(target_fd, &entry, 1)) > 0)
    {
        printf( "%lu %s", entry.inode_number, entry.name );
        if ( entry.type == DT_DIR )
        {
            putchar('/');
        }

        putchar('\n');
        free( entry.name );
    }

    nqp_close(target_fd);

    return 0;
}


int cd(char* dir)
{
    if (dir == NULL) {
        stack_clear(&cwd);
        return 0;
    }
    else if (strcmp(dir, "/") == 0)
    {
        stack_clear(&cwd);
    }

    else
    {
        int temp_fd = -1;
        nqp_dirent tmp = {0};

        path_t* tmp_abs_path = relative_path_to_absolute(dir);
         if ((temp_fd = nqp_open(stack_join( tmp_abs_path ))) > -1)
         {
             if (nqp_getdents(temp_fd, &tmp, 1) == -1)
             {
                 /* The path is a file */
                 printf("cd: Is a file: %s \n", stack_join( tmp_abs_path ));
                 nqp_close(temp_fd);
                 return -1;
             }

             memcpy(&cwd, tmp_abs_path, sizeof(path_t));
             nqp_close(temp_fd);

         }
         else
         {
             printf("cd: Invalid Path: %s \n", stack_join( tmp_abs_path ));
             return -1;
         }
    }
    (void) dir;
    return 0;
}

int main( int argc, char *argv[], char *envp[] )
{
    char line_buffer[MAX_LINE_SIZE] = {0};
    char *volume_label = NULL;
    nqp_mount_error mount_error;
    int token_count = 0;
    char** command;
    int err;

    (void) envp;

    if ( argc != 2 )
    {
        fprintf( stderr, "Usage: ./nqp_shell volume.img\n" );
        exit( EXIT_FAILURE );
    }

    mount_error = nqp_mount( argv[1], NQP_FS_EXFAT );

    if ( mount_error != NQP_MOUNT_OK )
    {
        if ( mount_error == NQP_MOUNT_FSCK_FAIL )
        {
            fprintf( stderr, "%s is inconsistent, not mounting.\n", argv[1] );
        }

        exit( EXIT_FAILURE );
    }

    /* populate the cwd path */
    cwd.top = -1;

    volume_label = nqp_vol_label( );

    printf( "%s:\\ %s > ", volume_label, stack_join(&cwd) );
    while ( fgets( line_buffer, MAX_LINE_SIZE, stdin ) != NULL )
    {
        line_buffer[strcspn(line_buffer, "\n")] = 0; /* Strip out the newline character from the fgets output */
        command = split_str(line_buffer, ' ', &token_count);
        if (strcmp(command[0], "cd") == 0)
        {
            err = cd((token_count > 1) ? command [1] : NULL);
        }
        else if (strcmp(command[0], "pwd") == 0)
        {
            err = pwd();
        }
        else if (strcmp(command[0], "ls") == 0)
        {
            err = ls( (token_count > 1) ? command[1] : NULL);
        }
        else
        {
           path_t* tmp_filepath_stack = relative_path_to_absolute(command[0]);
           char* filepath = stack_join(tmp_filepath_stack);
           free(tmp_filepath_stack);
           int child_pid = fork();
           if (child_pid == 0)
           {
               int exec_fd = nqp_open(filepath);
               char temp;
               int mfd = memfd_create(command[0], 0);
               while (nqp_read(exec_fd, &temp, 1) > 0) {
                   write(mfd, &temp, 1);
               }
               fexecve(mfd, &command[0], envp);
           }

           wait(NULL);

        }



        printf("%s returned with error code: %d\n", command[0], err);


        printf( "%s:\\ %s > ", volume_label, stack_join(&cwd) );
        free(command);
    }

    return EXIT_SUCCESS;
}
