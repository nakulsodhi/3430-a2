#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include "nqp_shell.h"
#include "nqp_io.h"
#include <ctype.h>
#include <stdbool.h>

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
        if (i != path->top) strcat(path_ret, "/");
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
        if (i != path->top - 1) strcat(path_ret, "/");
    }

    return path_ret;
}


char* strip_whitespace(char * token)
{
    char *end;

    // Trim leading space
    while(isspace((unsigned char)*token)) token++;

    // Trim trailing space
    end = token + strlen(token) - 1;
    while(end > token && isspace((unsigned char)*end)) end--;

    // Write new null terminator character
    end[1] = '\0';

    /* printf("whitespace stripped:%s\n", token); */
    return token;

}


void stack_push(path_t* path, char* token)
{
    path->top++;
    token = strip_whitespace(token);
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
            char** abs_path_tokens = split_str(dir, "/", &count);

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
            char** abs_path_tokens = split_str(dir, "/", &count);

            path_t* tmp = malloc(sizeof(path_t));
            memcpy(tmp, &cwd, sizeof(path_t));
            for (int i = 0; i < count; i++)
            {
                stack_push(tmp, abs_path_tokens[i]);
            }


            return tmp;
        }
}


char** split_str(const char* str, const char* delim, int* len)
{
    char** ret = NULL;
    char* tmp = strdup(str);
    int str_idx = 0;
    int token_count = 1;
    char* subtoken = NULL;
    char* saveptr = NULL;
    *len = 0;


    /* Get the number of tokens. */
    while (*str) {
        if (*delim == *str) {
            token_count++;
        }
        str++;
    }
    /* printf("token count:%d\n", token_count); */

    ret = calloc(token_count + 1, sizeof(char*)); /* Plus one for a null item */
    for (subtoken = strtok_r(tmp, delim , &saveptr);
         subtoken != NULL;
         subtoken = strtok_r(NULL, delim, &saveptr))
    {
        *len = *len + 1;
        *(ret + str_idx++) = strdup(subtoken);
        /* printf("subtoken:%s\n", subtoken); */

    }
    *(ret + str_idx) = NULL; /* Null terminate the string array */


    return ret;
}

char* join_str(char** split, int from, int len, const char* delim)
{
    int total_len = 0;
    for (int i = from; i < len; i++) total_len += 1 + strlen(split[i]);

    char* joined = malloc(total_len * sizeof(char));
    for (int i = from; i < len; i++)
    {
        strcat(joined, split[i]);
        if (i < len - 1) strcat(joined, delim);
    }

    /* printf("joined:%s\n", joined); */
    return joined;
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


int launch_command(char* command, char *envp[])
{
    int err = 0;
    int command_count = 0;
    command = strip_whitespace(command);
    char** pipeline_elements = split_str(command, "|", &command_count);
    int pipe_cur[2] = {-1};
    int pipe_prev[2] = {-1};
    int command_arg_count = 0;
    int token_count = 0;
    char temp;
    for (int i = 0; i < command_count; i++)
    {
        if (i == 0) // the first command in the pipeline
        {
            char **input_redirect_target = split_str(pipeline_elements[i], "<", &token_count);
            char **command_tokenized = split_str(pipeline_elements[i], " ", &command_arg_count);
            char *filepath = stack_join(relative_path_to_absolute(command_tokenized[0]));
            int exec_fd = nqp_open(filepath);
            if (exec_fd == NQP_FILE_NOT_FOUND) {
                return NQP_FILE_NOT_FOUND;
            }
            int mfd = memfd_create("command", 0);
            while (nqp_read(exec_fd, &temp, 1) > 0) {
                write(mfd, &temp, 1);
            }
            int input_mfd = -1;
            if  ( token_count == 2 ) /* there might be input redirection */
            {
                char *input_filepath =
                    stack_join(relative_path_to_absolute(input_redirect_target[1]));
                int input_fd = nqp_open(input_filepath);
                if (input_fd == -1)
                    return -1;
                input_mfd = memfd_create(input_filepath, 0);
                while (nqp_read(input_fd, &temp, 1) > 0) {
                    write(input_mfd, &temp, 1);
                }
                lseek(input_mfd, 0, SEEK_SET);
            }
            if (command_count > 1)
            {
                pipe(pipe_prev);
            }

            int child_pid = fork();
            if (child_pid == 0)
            {
                if (command_count > 1)
                {
                    close(pipe_prev[0]);
                    dup2(pipe_prev[1], STDOUT_FILENO);
                }
                if (token_count == 2 ) /* We do Input Redirection in these parts, pardner */
                {
                    dup2(input_mfd, STDIN_FILENO);
                    for (int i = 0; i < command_arg_count; i++)
                    {
                        if (strcmp(command_tokenized[i], "<") == 0)
                        {
                            command_tokenized[i] = NULL;
                            break;
                        }
                    }

                }

                fexecve(mfd, &command_tokenized[0], envp);
            }
        }
        else if (i == command_count - 1) //the last command in the pipeline
        {
            char **command_tokenized = split_str(pipeline_elements[i], " ", &command_arg_count);
            char *filepath = stack_join(relative_path_to_absolute(command_tokenized[0]));
            int exec_fd = nqp_open(filepath);
            if (exec_fd == NQP_FILE_NOT_FOUND) {
                return NQP_FILE_NOT_FOUND;
            }
            int mfd = memfd_create("command", 0);
            while (nqp_read(exec_fd, &temp, 1) > 0) {
                write(mfd, &temp, 1);
            }
            int child_pid = fork();
            if (child_pid == 0)
            {
                close(pipe_prev[1]);
                dup2(pipe_prev[0], STDIN_FILENO);
                fexecve(mfd, &command_tokenized[0], envp);
            }

            close(pipe_prev[0]);
            close(pipe_prev[1]);

        }
        else
        {
            char **command_tokenized = split_str(pipeline_elements[i], " ", &command_arg_count);
            char *filepath = stack_join(relative_path_to_absolute(command_tokenized[0]));
            int exec_fd = nqp_open(filepath);
            if (exec_fd == NQP_FILE_NOT_FOUND) {
                return NQP_FILE_NOT_FOUND;
            }
            int mfd = memfd_create("command", 0);
            while (nqp_read(exec_fd, &temp, 1) > 0) {
                write(mfd, &temp, 1);
            }

            pipe(pipe_cur);

            int child_pid = fork();
            if (child_pid == 0)
            {
                close(pipe_prev[1]);
                dup2(pipe_prev[0], STDIN_FILENO);
                close(pipe_cur[0]);
                dup2(pipe_cur[1], STDOUT_FILENO);
                fexecve(mfd, &command_tokenized[0], envp);
            }

            close(pipe_prev[0]);
            close(pipe_prev[1]);
            memcpy(pipe_prev, pipe_cur, sizeof(int) * 2);

        }
        wait(NULL);
    }
    return err;
}





int main( int argc, char *argv[], char *envp[] )
{
    char line_buffer[MAX_LINE_SIZE] = {0};
    char *volume_label = NULL;
    nqp_mount_error mount_error;
    int token_count = 0;
    char** command_tokenized;
    int err;



    (void) envp;

    if ( argc < 2 )
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
        char command[MAX_LINE_SIZE];
        strcpy(command, line_buffer);
        command_tokenized = split_str(command, " ", &token_count);
        if (strcmp(command_tokenized[0], "cd") == 0)
        {
            err = cd((token_count > 1) ? command_tokenized [1] : NULL);
        }
        else if (strcmp(command_tokenized[0], "pwd") == 0)
        {
            err = pwd();
        }
        else if (strcmp(command_tokenized[0], "ls") == 0)
        {
            err = ls( (token_count > 1) ? command_tokenized[1] : NULL);
        }
        else
        {
            err = launch_command(command, envp);

        }

        /* printf("%s returned with error code: %d\n", command_tokenized[0], err); */


        printf( "%s:\\ %s > ", volume_label, stack_join(&cwd) );
        free(command_tokenized);
    }

        (void) err;
    return EXIT_SUCCESS;
}
