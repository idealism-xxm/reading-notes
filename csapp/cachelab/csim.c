#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include "cachelab.h"

// 每次内存操作是否打印相关信息
bool verbose = false;
// 缓存相关参数 （书中 P425 有具体讲解）
int s, S, E, b, B;
// trace 文件路径
char tracefile_path[1003];
// 统计数据
int hits = 0, misses = 0, evictions = 0;
// 时间戳（用读取的行数模拟）
int timestamp = 0;

// 打印帮助信息
static void usage(char *name) {
    printf("Usage: %s [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n", name);
    printf("   -h     Optional help flag that prints usage info\n");
    printf("   -v     Optional verbose flag that displays trace info\n");
    printf("   -s <s>: Number of set index bits (S = 2 ^ s is the number of sets)\n");
    printf("   -E <E>: Associativity (number of lines per set)\n");
    printf("   -b <b>: Number of block bits (B = 2 ^ b is the block size)\n");
    printf("   -t <tracefile>: Name of the valgrind trace to replay\n\n");
    printf("Examples:\n");
    printf("   linux>  %s -s 4 -E 1 -b 4 -t traces/yi.trace\n", name);
    printf("   linux>  %s -v -s 8 -E 2 -b 4 -t traces/yi.trace\n", name);
    exit(0);
}

// 处理调用参数
void handle_args(int argc, char *argv[]) {
    // 接收 getopt 的返回值，表明当前处理的 flag
    int opt;

    // getopt 第三个参数表明所有选项，不可省略的选项字符后要跟冒号
    while(-1 != (opt = (getopt(argc, argv, "hvs:E:b:t:")))) {
        switch(opt) {
            case 'h':
                usage(argv[0]);
                break;
            case 'v':
                verbose = true;
                break;
            case 's':
                s = atoi(optarg);
                break;
            case 'E':
                E = atoi(optarg);
                break;
            case 'b':
                b = atoi(optarg);
                break;
            case 't':
                strcpy(tracefile_path, optarg);
                break;
            default:
                usage(argv[0]);
                break;
        }
    }
}

typedef struct {
    // 有效位
    bool valid;
    // 标记
    int tag;
    // 上次访问时间
    int timestamp;
    // 由于本实验不需要存取数据，所以缓存块省略了
} CacheLine;

// 缓存的分布就类似一个二分数组
// 第一维用于定位在哪一组
// 第二维用于定位该组中的哪一行
CacheLine** cache;

// 初始化缓存
void init_cache() {
    int i, j;
    // 总共 2 ^ s 组
    S = 1 << s;
    // 每个缓存块供 2 ^ b 字节
    B = 1 << b;

    // 先分配 S 个缓存组的空间
    cache = (CacheLine**) malloc(sizeof(CacheLine*) * S);
    for (i = 0; i < S; i++) {
        // 再分配每个缓存组中的缓存行空间
        cache[i] = (CacheLine*) malloc(sizeof(CacheLine) * E);
        // 然后初始化每个缓存行的数据
        for (j = 0; j < E; j++) {
            cache[i][j].valid = false;
        }
    }
}

// 由于没有数据块，所以我们可以把 L 和 S 统一处理
// 0: 命中
// 1: 未命中未驱逐
// 2: 未命中已驱逐
int load_or_store(int address) {
    int j, info = 0;
    // 获取该地址对应的组下标
    int set_index = (address >> b) & (S - 1);
    // 获取该地址对应的行 tag
    int line_tag = address >> (s + b);
    // 假设可能失败且该组已满，这样我们可以一次循环处理出来结果
    int line_index = -1;
    // 同时假设可能失败且该组未满，这样我们可以一次循环处理出来结果
    int invalid_line = -1;
    for (j = 0; j < E; j++) {
        if (!cache[set_index][j].valid) {
            // 如果最后发现未命中缓存，那么该缓存行将存储这个数据
            invalid_line = j;
            continue;
        }

        // 如果是我们要找的地址，那么 命中 + 1 ，更新访问时间
        if (cache[set_index][j].tag == line_tag) {
            hits++;
            cache[set_index][j].timestamp = timestamp;
            return 0;
        }
        // 如果不是我们要找的地址，那么找到最早访问的行
        if (line_index == -1 || cache[set_index][line_index].timestamp > cache[set_index][j].timestamp) {
            line_index = j;
        }
    }

    // 此时没有命中
    misses++;
    info++;
    if (invalid_line == -1) {
        // 如果没有可用的行，则需要驱逐
        evictions++;
        info++;
        invalid_line = line_index;
    }
    // 将缓存放入该行，并更新对应的信息
    cache[set_index][invalid_line].valid = true;
    cache[set_index][invalid_line].tag = line_tag;
    cache[set_index][invalid_line].timestamp = timestamp;
    return info;
}

// 打印调试信息
void print_verbose(char operation, int address, int size, bool miss, bool hit, bool evict) {
    if (!verbose) {
        return;
    }

    printf("%c %x,%d", operation, address, size);
    if (miss) {
        printf(" miss");
    }
    if (evict) {
        printf(" eviction");
    }
    if (hit) {
        printf(" hit");
    }
    printf("\n");
}

// 处理 trace 文件的数据
void handle_trace() {
    // 用于存储每次内存操作的信息
    int info;
    char operation;
    int address, size;
    // 获取文件句柄
    FILE* fp = fopen(tracefile_path, "r"); // 读取文件名
    if(fp == NULL) {
        printf("open file [%s] error", tracefile_path);
        exit(-1);
    }

    // 读取每一行数据，并执行对应的内存操作
    while(fscanf(fp, " %c %x,%d\n", &operation, &address, &size) > 0) {
        // 由于题目说明使用的数据不会跨域两个缓存块，所以不用管 size 即可
        switch(operation) {
            case 'L':
            case 'S':
                info = load_or_store(address);
                print_verbose(operation, address, size, info >= 1, info == 0, info == 2);
                break;
            case 'M':
                // 先执行 Load
                info = load_or_store(address);
                // 再执行 Store （第二次必定命中，不记录结果）
                load_or_store(address);
                print_verbose(operation, address, size, info >= 1, true, info == 2);
        }
        // 模拟时间流动
        timestamp++;
    }

    // 关闭文件
    fclose(fp);
    // 释放所有分配的内存
    for(int i = 0; i < S; ++i) {
        free(cache[i]);
    }
    free(cache);
}

int main(int argc, char *argv[]) {
    // 处理参数
    handle_args(argc, argv);
    // 初始化缓存
    init_cache();
    // 处理 trace 文件的数据
    handle_trace();
    // 打印统计信息
    printSummary(hits, misses, evictions);
    return 0;
}
