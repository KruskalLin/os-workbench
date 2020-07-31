#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <assert.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <regex.h>

#define NODE_NUM 1000
#define MAX_LEN 512

typedef struct node{
    char name[MAX_LEN];
    double time;
}Node;

Node nodes[NODE_NUM];

int readline(int fd, char* str, int len) {
    int i = 0;
    memset(str, '\0', len);
    for(i = 0; i < len; i++) {
        char c;
        fd_set read_fds, write_fds, except_fds;
        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);
        FD_ZERO(&except_fds);
        FD_SET(fd, &read_fds);

        struct timeval timeout;
        timeout.tv_sec = 100;
        timeout.tv_usec = 0;

        if(select(fd + 1, &read_fds, &write_fds, &except_fds, &timeout) == 1) {
            if (read(fd, &c, 1) == 1) {
                str[i] = c;
                if (c == '\n') {
                    str[i] = '\0';
                    return 1;
                }
            } else {
                return 0;
            }
        } else {
            return 0;
        }
    }
    return 0;
}

int cmp(const void* node1, const void* node2) {
    return (*(Node*)node1).time - (*(Node*)node2).time > 0 ? -1 : 1;
}

int main(int argc, char *argv[], char *exec_envp[]) {
    for(int i = 0; i < NODE_NUM; i++) {
        memset(nodes[i].name, '\0', MAX_LEN);
        nodes[i].time = 0;
    }

    char *exec_argv[argc + 3];
    exec_argv[0] = "strace";
    exec_argv[1] = "-T";
    for(int i = 1; i <= argc; i++)
        exec_argv[i + 1] = argv[i];
    exec_argv[argc + 2] = NULL;
    char* paths = getenv("PATH");
    char pathcopy[MAX_LEN];
    strcpy(pathcopy, paths);
    char* path = strtok(pathcopy, ":");
    char strace[50][MAX_LEN];
    memset(strace, '\0', sizeof(strace));
    int strace_num = 0;
    while(path != NULL) {
        int len = strlen(path);
        strcpy(strace[strace_num], path);
        strcpy(strace[strace_num] + len, "/strace");
        path = strtok(NULL, ":");
        strace_num++;
    }

    regex_t reg;
    regmatch_t match[3];
    if(regcomp(&reg, "(\\w+)\\(.+\\)\\s*=.+<([0-9]+\\.[0-9]+)>", REG_EXTENDED)) {
        perror("REGEX COMPILE ERROR!");
        exit(EXIT_FAILURE);
    }


    int fildes[2];
    if (pipe(fildes) != 0) {
        perror("BUILD PIPE FAIL!");
        exit(EXIT_FAILURE);
    }

    int pid = fork();
    if (pid == 0) {
        // child process
        dup2(fildes[1], fileno(stderr)); // dup2 pipe
        int fd = open("/dev/null", O_RDWR);
        dup2(fd, fileno(stdout)); // dup stdout 2 null
        while(execve(strace[strace_num - 1], exec_argv, exec_envp) == -1) {
            strace_num--;
            if(strace_num < 0)
                break;
        }
        perror("NOT REACH HERE!");
        exit(EXIT_FAILURE);
    } else {
        // parent process
        char str[MAX_LEN];
        int len = 0;
        double tot = 0.0;
        int pre = clock();
        int cnt = 1;
        while(readline(fildes[0], str, sizeof(str))) {
            if(regexec(&reg, str, 3, match, 0) != REG_NOMATCH){

                char name[MAX_LEN] = {'\0'};
                char time[MAX_LEN] = {'\0'};
                strncpy(name, str + match[1].rm_so, match[1].rm_eo - match[1].rm_so);
                strncpy(time, str + match[2].rm_so, match[2].rm_eo - match[2].rm_so);
                double t = atof(time);
                // fine name
                int flag = 1;

                for(int i = 0; i < len; i++) {
                    if(strcmp(nodes[i].name, name) == 0) {
                        nodes[i].time += t;
                        flag = 0;
                        break;
                    }
                }
                if(flag) {
                    strcpy(nodes[len].name, name);
                    nodes[len].time = t;
                    len++;
                }
                tot += t;

                int now = clock();
                if(now - pre >= 2000) {
                    qsort(nodes, len, sizeof(Node), cmp);
                    for(int i = 0; i < len && i < 5; i++) {
                        printf("%s (%d%%)\n", nodes[i].name, (int)(nodes[i].time / tot * 100));
                        fflush(stdout);
                    }
                    for (int i = 0; i < 80; i++) {
                        printf("%c",'\0');
                    }
                    fflush(stdout);

                    cnt++;
                    pre = now;
                }

                fflush(stdout);
                continue;
            }
            if(strstr(str, "exited with") != NULL){
                return 0;
            }
        }
        return 0;
    }
    return 0;
}
