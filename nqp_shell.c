#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include "nqp_shell.h"
#include "nqp_io.h"


char cwd[MAX_LINE_SIZE];

char* relative_path_to_absolute(char* dir){

    /* Still doesn't handle the case where the path is a relative path starting with .. or . */


        if (dir[1] == '/')      /* Absolute path */
        {
            return dir;
        }
        else if (strcmp(dir, "..") == 0)  /* Parent path */
        {
            return get_parent_path(dir);
        }
        else if (strcmp(dir, ".") == 0)
        {
            return cwd;
        }
        else                    /* Relative path */
        {
            char* concat_path = calloc(strlen(cwd) + strlen(dir) + 1, sizeof(char));

            strcpy(concat_path, cwd);
            strcat(concat_path, dir);

            return concat_path;
        }
}



char* get_parent_path(char* path)
{
    if (strcmp(path, "/") == 0) {
        return path;
    }

    int depth = 0;
    char** cwd_split = split_str(cwd, '/', &depth);

    if (depth == 1)             /* The parent is root */
    {
        return "/";

    }
    int path_length = depth * sizeof(char);
    for (int i = 0; i < depth - 1; i++)
    {
        path_length += strlen(cwd_split[i]);

    }
    char* parent_path = calloc(path_length, sizeof(char));
    int idx = 0;
    for (int i = 0; i < depth - 1; i++)
    {
        strcpy((parent_path + idx++), "/");
        strcpy((parent_path + idx), cwd_split[i]);
        idx += strlen(cwd_split[i]);
    }


    return parent_path;
}

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
        printf("token:%s\n", subtoken);

    }

    free(subtoken);

    return ret;
}

int pwd(void)
{
    printf("%s \n", cwd);
    return 0;
}


int ls(char* dir)
{
    nqp_dirent entry = {0};
    ssize_t dirents_read;
    int target_fd = -1;

    if (dir != NULL)
    {
        if (dir[1] == '/')      /* Absolute path */
        {
            target_fd = nqp_open(dir);
        }
        else if (strcmp(dir, "..") == 0)  /* Parent path */
        {
            target_fd = nqp_open(get_parent_path(dir));
        }
        else if (strcmp(dir, ".") == 0)
        {
            target_fd = nqp_open(cwd);
        }
        else                    /* Relative path */
        {
            char concat_path[strlen(cwd) + strlen(dir) + 1];

            strcpy(concat_path, cwd);
            strcat(concat_path, dir);

            target_fd = nqp_open(concat_path);

        }

    }
    else
    {
        target_fd = nqp_open(cwd);
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
        strcpy(cwd, "/");
        return 0;
    }
    else if (strcmp(dir, "/") == 0)
    {
        strcpy(cwd, "/");
    }
    else
    {
        int temp_fd = -1;
        nqp_dirent tmp = {0};
        char* abs_path = relative_path_to_absolute(dir);
         if ((temp_fd = nqp_open(abs_path)) > -1)
         {
             if (nqp_getdents(temp_fd, &tmp, 1) == -1)
             {
                 /* The path is a file */
                 printf("cd: Is a file: %s \n", abs_path);
                 nqp_close(temp_fd);
                 return -1;
             }

             strcpy(cwd, abs_path);
             if (cwd[strlen(cwd) - 1] != '/') strcat(cwd, "/");
             nqp_close(temp_fd);

         }
         else
         {
             printf("cd: Invalid Path: %s \n", abs_path);
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

    strcpy(cwd, "/");

    volume_label = nqp_vol_label( );

    printf( "%s:\\ %s > ", volume_label, cwd );
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
           char filepath[MAX_LINE_SIZE];
           strcpy(filepath, cwd);
           strcat(filepath, command[0]);
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


        printf( "%s:\\ %s > ", volume_label, cwd );
        free(command);
    }

    return EXIT_SUCCESS;
}
