#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>
#include <regex.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

static char line[4096];

#if defined(__i386__)
    #define COMPILE_OPTION "-m32"
#elif defined(__x86_64__)
    #define COMPILE_OPTION "-m64"
#endif

void copy(const char* source, const char* target) {
    FILE* f_source = fopen(source, "r");
    FILE* f_target = fopen(target, "w");
    char buf[4096] = {'\0'};
    int len;
    while((len = fread(buf, 1, sizeof(buf), f_source)) > 0) {
        fwrite(buf, 1, len, f_target);
    }
    fclose(f_source);
    fclose(f_target);
}

int main(int argc, char *argv[], char *exec_envp[]) {

    regex_t reg;
    regmatch_t match;
    if(regcomp(&reg, "int\\s+\\w+\\(.*\\)\\s*\\{.*\\}", REG_EXTENDED)) {
        perror("REGEX COMPILE ERROR!");
        exit(EXIT_FAILURE);
    }
    char* temp_name = "/tmp/crepl.c";
    char* temp_backup_name = "/tmp/crepl_backup.c";
    char* temp_so = "/tmp/crepl.so";
    char* gcc_argv[]={"gcc","-fPIC", COMPILE_OPTION, "-Werror", "-shared", temp_name, "-o", temp_so, NULL};

    remove(temp_name);
    remove(temp_backup_name);
    creat(temp_name, 0666);
    creat(temp_backup_name, 0666);
    int cnt = 0;
    while (1) {
        printf("crepl> ");
        fflush(stdout);
        memset(line, '\0', sizeof(line));
        if (!fgets(line, sizeof(line), stdin)) {
            break;
        }
        int len = strlen(line);
        if(len >= 2) {
            line[len - 1] = '\0';
            int flag = 0;
            FILE *f = fopen(temp_name, "a");
            if(regexec(&reg, line, 1, &match, 0) != REG_NOMATCH) {
                fprintf(f, "%s\n", line);
            } else{
                fprintf(f, "int wrapper_%d(){ return %s;}\n", cnt, line);
                flag = 1;
            }
            fclose(f);
            int pid = fork();
            int status;
            if(pid == 0) {
                int fd = open("/dev/null", O_RDWR);
                dup2(fd, fileno(stderr));
                execvp("gcc", gcc_argv);
            } else {
                wait(&status);
                if(status != 0){
                    printf("Compile Error.\n");
                    remove(temp_name);
                    copy(temp_backup_name, temp_name);
                    continue;
                }
                FILE *f_backup = fopen(temp_backup_name, "a");
                if(flag) {
                    fprintf(f_backup, "int wrapper_%d(){ return %s;}\n", cnt, line);
                    void *handle = dlopen(temp_so, RTLD_LAZY|RTLD_GLOBAL);
                    char func_name[20];
                    sprintf(func_name, "wrapper_%d", cnt);
                    int (*wrapper)()= dlsym(handle, func_name);
                    printf("(%s) == %d\n", line, wrapper());
                    dlclose(handle);
                    cnt++;
                } else {
                    fprintf(f_backup, "%s\n", line);
                    printf("Added: %s\n", line);
                }
                fclose(f_backup);
            }
        }
    }
}