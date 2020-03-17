#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#define PROC_NUMBER 100000

typedef struct node{
    int pid;
    char name[64];
    int nodes_len;
    struct node* children[1024];
}Node;

int compare_name(const void *n1, const void *n2) {
    Node* node1 = *(Node**)n1;
    Node* node2 = *(Node**)n2;
    int name_compare = strcmp(node1->name, node2->name);
    if (name_compare == 0)
        return node1->pid - node2->pid;
    else
        return name_compare;
}

int compare_id(const void *n1, const void *n2) {
    Node* node1 = *(Node**)n1;
    Node* node2 = *(Node**)n2;
    return node1->pid - node2->pid;
}

void printTree(Node* nodes[], int index, int layer, int show_pids, int numeric_sort) {
    for(int i = 0; i < layer; i++) {
        printf("  ");
    }
    printf("%s", nodes[index]->name);
    if(show_pids) {
        printf("(%d)", nodes[index]->pid);
    }
    printf("\n");
    if(numeric_sort){
        qsort(nodes[index]->children, nodes[index]->nodes_len, sizeof(Node*), &compare_id);
    } else {
        qsort(nodes[index]->children, nodes[index]->nodes_len, sizeof(Node*), &compare_name);
    }
    for(int i = 0; i < nodes[index]->nodes_len; i++) {
        printTree(nodes, nodes[index]->children[i]->pid, layer+1, show_pids, numeric_sort);
    }
}

int main(int argc, char *argv[]) {
    int show_pids = 0;
    int numeric_sort = 0;
    int version = 0;
    for (int i = 0; i < argc; i++) {
        assert(argv[i]);
        if(strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--show-pids") == 0) {
            show_pids = 1;
        }
        if(strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--numeric-sort") == 0) {
            numeric_sort = 1;
        }
        if(strcmp(argv[i], "-V") == 0 || strcmp(argv[i], "--version") == 0) {
            version = 1;
        }
    }
    if (version) {
        fprintf(stderr, "pstree (PSmisc) 22.21\n"
               "Copyright (C) 1993-2009 Werner Almesberger and Craig Small\n"
               "\n"
               "PSmisc comes with ABSOLUTELY NO WARRANTY.\n"
               "This is free software, and you are welcome to redistribute it under\n"
               "the terms of the GNU General Public License.\n"
               "For more information about these matters, see the files named COPYING.");
        return 0;
    }

    Node* procs[PROC_NUMBER];
    DIR *d;
    struct dirent *dir;
    d = opendir("/proc");
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            int pid = atoi(dir->d_name);
            if (pid) {
                char filename[64];
                int len = strlen(dir->d_name);
                strcpy(filename, "/proc/");
                strcpy(filename + 6, dir->d_name);
                filename[6 + len] = '/';
                strcpy(filename + 7 + len, "status");
                FILE *fp = fopen(filename, "r");
                if (fp) {
                    char* line = NULL;
                    size_t clen;
                    ssize_t read;
                    if(procs[pid] == NULL) {
                        procs[pid] = malloc(sizeof(Node));
                        procs[pid]->pid = pid;
                        procs[pid]->nodes_len = 0;
                    }
                    int tgid = -1;
                    int ppid = -1;

                    while((read = getline(&line, &clen, fp)) != -1) {

                        if(strncmp(line, "Name:", 5) == 0) {
                            line[strlen(line)-1]=0;
                            strcpy(procs[pid]->name, line + 6);
                        }
                        if(strncmp(line, "Tgid:", 5) == 0) {
                            tgid = atoi(line + 6);
                        }
                        if(strncmp(line, "PPid:", 5) == 0) {
                            ppid = atoi(line + 6);
                            if(ppid == 0) {
                                break;
                            }
                            assert(tgid > 0);
                            if (pid == tgid) {
                                if(procs[ppid] == NULL) {
                                    procs[ppid] = malloc(sizeof(Node));
                                    procs[ppid]->pid = ppid;
                                    procs[ppid]->nodes_len = 0;
                                }
                                procs[ppid]->children[procs[ppid]->nodes_len] = procs[pid];
                                procs[ppid]->nodes_len++;
                            } else {
                                procs[tgid]->children[procs[tgid]->nodes_len] = procs[pid];
                                procs[tgid]->nodes_len++;
                            }
                        }
                    }
                    fclose(fp);
                } else {
                    return 1;
                }

            }
        }
        closedir(d);
    }
    printTree(procs, 1, 0, show_pids, numeric_sort);
    assert(!argv[argc]);
    return 0;
}
