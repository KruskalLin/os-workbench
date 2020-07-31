#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>
#include <regex.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>

struct fat_header {
    uint8_t BS_jmpBoot[3];
    uint8_t BS_OEMName[8];
    uint16_t BPB_BytsPerSec;
    uint8_t BPB_SecPerClus;
    uint16_t BPB_RsvdSecCnt;
    uint8_t BPB_NumFATs;
    uint16_t BPB_RootEntCnt;
    uint16_t BPB_TotSec16;
    uint8_t BPB_Media;
    uint16_t BPB_FATSz16;
    uint16_t BPB_SecPerTrk;
    uint16_t BPB_NumHeads;
    uint32_t BPB_HiddSec;
    uint32_t BPB_TotSec32;
    uint32_t BPB_FATSz32;
    uint16_t BPB_ExtFlags;
    uint16_t BPB_FSVer;
    uint32_t BPB_RootClus;
    uint16_t BPB_FSInfo;
    uint16_t BPB_BkBootSec;
    uint8_t BPB_Reserved[12];
    uint8_t BS_DrvNum;
    uint8_t BS_Reserved1;
    uint8_t BS_BootSig;
    uint32_t BS_VolID;
    uint8_t BS_VolLab[11];
    uint8_t BS_FilSysType[8];
    uint8_t padding[420];
    uint16_t signature;
} __attribute__((packed));

struct fat_short_entry {
    uint8_t DIR_Name[11];
    uint8_t DIR_Attr;
    uint8_t DIR_NTRes;
    uint8_t DIR_CrtTimeTenth;
    uint16_t DIR_CrtTime;
    uint16_t DIR_CrtDate;
    uint16_t DIR_LstAccDate;
    uint16_t DIR_FstClusHI;
    uint16_t DIR_WrtTime;
    uint16_t DIR_WrtDate;
    uint16_t DIR_FstClusLO;
    uint32_t DIR_FileSize;
}__attribute__((packed));

struct fat_long_entry {
    uint8_t LDIR_Ord;
    uint16_t LDIR_Name1[5];
    uint8_t LDIR_Attr;
    uint8_t LDIR_Type;
    uint8_t LDIR_Chksum;
    uint16_t LDIR_Name2[6];
    uint16_t LDIR_FstClusLO;
    uint16_t LDIR_Name3[2];
}__attribute__((packed));

struct bmp_header {
    uint8_t signature[2];
    uint32_t file_sz;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset;
}__attribute__((packed));

struct bmp_information_header {
    uint32_t header_sz;
    uint32_t width;
    uint32_t height;
    uint16_t nplanes;
    uint16_t bits_pp;
    uint32_t compress_type;
    uint32_t img_sz;
    uint32_t hres;
    uint32_t vres;
    uint32_t ncolors;
    uint32_t nimpcolors;
}__attribute__((packed));

unsigned int gradient(char *before, char *current, int len) {
    unsigned int sum = 0;
    for (int i = 0; i < len; i++) {
        if(current[i] > before[i])
            sum += (current[i] - before[i]);
        else
            sum += (before[i] - current[i]);
    }
    return sum;
}

int main(int argc, char *argv[]) {
    assert(sizeof(struct fat_header) == 512);
    int fd = open(argv[1], O_RDONLY);
    struct stat fstate;
    fstat(fd, &fstate);
    struct fat_header *disk = (struct fat_header *) mmap(NULL, fstate.st_size, PROT_READ, MAP_SHARED, fd, 0);
    assert(disk->signature == 0xaa55);

    unsigned int tot_sec = disk->BPB_TotSec32;
    unsigned int offset_sec = disk->BPB_RsvdSecCnt + disk->BPB_NumFATs * disk->BPB_FATSz32;
    unsigned int data_sec = tot_sec - offset_sec;
    unsigned int count_of_clusters = data_sec / disk->BPB_SecPerClus;
    unsigned int cluster_sz = disk->BPB_BytsPerSec * disk->BPB_SecPerClus;
    unsigned int eq_short_entry_sz = cluster_sz / sizeof(struct fat_short_entry);
    uintptr_t data_start_ptr = (uintptr_t) disk + offset_sec * disk->BPB_BytsPerSec;
    unsigned int* data_fat_map = (unsigned int*)((uintptr_t)disk + disk->BPB_RsvdSecCnt * disk->BPB_BytsPerSec);
    uintptr_t end = (uintptr_t) disk + tot_sec * disk->BPB_BytsPerSec;
    char* used = malloc(count_of_clusters + 2);
    memset(used, 0, sizeof(count_of_clusters + 2));
    // we only need entries to get names
    char results[500][500];
    memset(results, '\0', sizeof(results));
    unsigned int first_cluster[500];
    memset(first_cluster, -1, sizeof(first_cluster));
    int num = 0;
    for (int i = 0; i < count_of_clusters; i++) {
        uintptr_t address = data_start_ptr + i * cluster_sz;
        struct fat_short_entry *short_entry = (struct fat_short_entry *) address;
        for (int j = 0; j < eq_short_entry_sz; j++) {
            struct fat_short_entry *short_entry = (struct fat_short_entry *) (address + j * sizeof(struct fat_short_entry));
            if (strncmp((char *) short_entry->DIR_Name + 8, "BMP", 3) == 0) {
                // short entry
                if ((unsigned short) short_entry->DIR_Name[0] == 0xE5 || short_entry->DIR_FileSize == 0) {
                    continue;
                }
                unsigned int cluster_index = ((unsigned int) short_entry->DIR_FstClusLO | (((unsigned int) (short_entry->DIR_FstClusHI)) << 16)) - 2;
                first_cluster[num] = cluster_index;
                uintptr_t bmp_fp = data_start_ptr + cluster_index * cluster_sz;
                struct bmp_header *bmp_file_header = (struct bmp_header *) bmp_fp;
                if (bmp_fp >= end || strncmp((char *) bmp_file_header->signature, "BM", 2) != 0)
                    continue;

                used[cluster_index] = 1;

                char filename[500];
                memset(filename, '\0', sizeof(filename));
                int flag = 1;
                int index = 0;
                for (int k = j - 1; k >= 0; k--) {
                    struct fat_long_entry *long_entry = (struct fat_long_entry *) (address + k * sizeof(struct fat_short_entry));
                    if (long_entry->LDIR_Attr == 15 && long_entry->LDIR_Type == 0 && long_entry->LDIR_FstClusLO == 0) {
                        // long entry
                        flag = 0;
                        for (int l = 0; l < 5; l++) {
                            if (long_entry->LDIR_Name1[l] == 0xFFFF)
                                continue;
                            filename[index++] = (char) long_entry->LDIR_Name1[l];
                        }
                        for (int l = 0; l < 6; l++) {
                            if (long_entry->LDIR_Name2[l] == 0xFFFF)
                                continue;
                            filename[index++] = (char) long_entry->LDIR_Name2[l];
                        }
                        for (int l = 0; l < 2; l++) {
                            if (long_entry->LDIR_Name3[l] == 0xFFFF)
                                continue;
                            filename[index++] = (char) long_entry->LDIR_Name3[l];
                        }
                    } else {
                        break;
                    }
                }
                if (flag) {
                    for (int l = 0; l < 11; l++) {
                        filename[index++] = (char) short_entry->DIR_Name[l]; // TODO
                    }
                }
                strcpy(results[num], filename);
                num++;
            }
        }
    }
    unsigned int align = cluster_sz - sizeof(struct bmp_header) - sizeof(struct bmp_information_header);
    for(int i = 0; i < num; i++) {
        char temp_filename[512] = "/tmp/";
        strcat(temp_filename, results[i]);
        remove(temp_filename);
        uintptr_t bmp_fp = data_start_ptr + first_cluster[i] * cluster_sz;
        struct bmp_header *bmp_file_header = (struct bmp_header *) bmp_fp;
        FILE *bmp_temp_file = fopen(temp_filename, "a");
        fwrite((void *) bmp_file_header, sizeof(struct bmp_header), 1, bmp_temp_file);
        struct bmp_information_header *bmp_information_file_header = (struct bmp_information_header *) (bmp_file_header + 1);
        fwrite((void *) bmp_information_file_header, sizeof(struct bmp_information_header), 1, bmp_temp_file);
        uintptr_t img_start = bmp_fp + bmp_file_header->offset;

        if(bmp_information_file_header->img_sz > align) {
            fwrite((void *) img_start, align, 1, bmp_temp_file);
            int height = bmp_information_file_header->height;
            int width = bmp_information_file_header->img_sz / height;
            if(width >= 4096 || width < 0)
                continue;
            int img_sz = bmp_information_file_header->img_sz;
            img_sz -= align;
            char before_line[4096];
            char current_line[4096];
            int before_pos = cluster_sz - align % width - width;
            int current_pos = width - align % width;
            if (before_pos < 0) {
                uintptr_t img_current = img_start + align;
                while (img_sz >= cluster_sz) {
                    fwrite((void *) img_current, cluster_sz, 1, bmp_temp_file);
                    img_current = img_current + cluster_sz;
                    img_sz -= cluster_sz;
                }
                if (img_sz > 0) {
                    fwrite((void *) img_current, img_sz, 1, bmp_temp_file);
                }
            } else {
                memcpy(before_line, (void *) (img_start + before_pos), width);
                uintptr_t img_current = img_start + align;
                while (img_sz >= cluster_sz) {
                    memcpy(current_line, (void *) (img_current + current_pos), 4096);
                    unsigned int g = 0;
                    for (int v = 0; v < width; v++) {
                        if(current_line[v] > before_line[v])
                            g += (current_line[v] - before_line[v]);
                        else
                            g += (before_line[v] - current_line[v]);
                    }
                    if (1) {
                        unsigned int gmin = g;
                        unsigned int ind = 0;
                        for (unsigned int p = 2; p < count_of_clusters / 50 + 1; p++) {
                            if (used[p])
                                continue;
                            uintptr_t current_address = data_start_ptr + p * cluster_sz;
                            if(current_address + current_pos + 4096 > end)
                                break;
                            char* unused = malloc(4096);
                            memcpy(unused, (void*)(current_address + current_pos), 4096);
                            unsigned int g_ = 0;
                            for (int v_ = 0; v_ < width; v_++) {
                                if(unused[v_] > before_line[v_])
                                    g_ += (unused[v_] - before_line[v_]);
                                else
                                    g_ += (before_line[v_] - unused[v_]);
                            }
                            if (g_ < gmin) {
                                gmin = g_;
//                                img_current = (uintptr_t)current_address;
                                ind = p;
                            }
                            free(unused);
                        }
                        if(gmin != g) {
                            used[ind] = 1;
                        }
                    } else {
                        used[(img_current - data_start_ptr) / cluster_sz] = 1;
                    }
                    if(img_current + before_pos + width > end)
                        break;
                    fwrite((void *) img_current, cluster_sz, 1, bmp_temp_file);
                    before_pos = cluster_sz - (cluster_sz - current_pos) % width - width;
                    current_pos = width - (cluster_sz - current_pos) % width;
                    memcpy(before_line, (void *) (img_current + before_pos), 4096);
                    img_current = img_current + cluster_sz;
                    img_sz -= cluster_sz;
                }
                if (img_sz > 0) {
                    fwrite((void *) img_current, img_sz, 1, bmp_temp_file);
                }
            }
        } else {
            fwrite((void *) img_start, bmp_information_file_header->img_sz, 1, bmp_temp_file);
        }
        fclose(bmp_temp_file);
        char buf[512];
        char temp_call[512] = "sha1sum /tmp/";
        strcat(temp_call, results[i]);
        FILE *fp = popen(temp_call, "r");
        fscanf(fp, "%s", buf);
        pclose(fp);
        printf("%s %s\n", buf, results[i]);
    }

    free(used);
}

