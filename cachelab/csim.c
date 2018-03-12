//LeeKwonHyeong lkh116@snu.ac.kr
#include "cachelab.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>

typedef unsigned long long AddrBit;        //Addr is 64-bit

typedef struct{
    int s;
    int b;
    int E;
    int t;
    int ver_mode;

    int S;
    int B;
} parameter;

typedef struct{
    int hit;
    int miss;
    int evict;
    int exOfsim_count;
} count_res;

typedef struct{
    int valid;
    int when_used;
    char* block;

    AddrBit tag;
} cache_line;

typedef struct{
    cache_line* lines;
} cache_set;

typedef struct{
    cache_set* sets;
} cache;

cache initialize_cache(int num_sets, int num_lines, int num_byte){
    cache new_c;

    new_c.sets = (cache_set*)malloc(sizeof(cache_set) * num_sets);
    for (int i = 0; i < num_sets; i++){
        new_c.sets[i].lines = (cache_line*)malloc(sizeof(cache_line) * num_lines);
        for (int j = 0; j < num_lines; j++){
            new_c.sets[i].lines[j].block = (char*)malloc(sizeof(char) * num_byte);
            new_c.sets[i].lines[j].valid = 0;
            new_c.sets[i].lines[j].when_used = 0;
            new_c.sets[i].lines[j].tag = 0;
        }
    }
    return new_c;
}

void popcache(cache obcache, parameter par_in){
    for (int i = 0; i < par_in.S; i++){
        for (int j = 0; j < par_in.E; j++){
            free(obcache.sets[i].lines[j].block);
        }
        free(obcache.sets[i].lines);
    }
    free(obcache.sets);
}

int emptyLine(cache_set set, int num_lines){
    for (int lineIndex = 0; lineIndex < num_lines; lineIndex++){
        if (set.lines[lineIndex].valid == 0)
            return lineIndex;
    }
    return -1;      //there is no empty line
}

int leastUsedLine(cache_set set, int num_lines){
    int indexOfleast = 0;

    //searching lines that have min (when_used) var
    for (int i = 0; i < num_lines; i++){
        if (set.lines[i].when_used < set.lines[indexOfleast].when_used)
            indexOfleast = i;
    }

    return indexOfleast;
}

//simulation function
//return result of each line
count_res simulation(cache simcache, AddrBit addr_query, count_res count, parameter par_in){
    AddrBit query_set = (addr_query << par_in.t) >> (par_in.t + par_in.b);
    AddrBit query_tag = addr_query >> (par_in.s + par_in.b);
    int isFull = 0;
    int empty_line = 0;

    cache_set tempset = simcache.sets[query_set];

    //search if there are word that we look forward
    for (int lineIndex = 0; lineIndex < par_in.E; lineIndex++){
        if (tempset.lines[lineIndex].valid == 1){
            if (tempset.lines[lineIndex].tag == query_tag){
                count.hit++;
                tempset.lines[lineIndex].valid = 1;
                
                tempset.lines[lineIndex].when_used = count.exOfsim_count;
                count.exOfsim_count++;

                if (par_in.ver_mode == 1)
                    printf(" hit");

                return count;
            }
        }
    }

    empty_line = emptyLine(tempset, par_in.E);
    if (empty_line == -1){
        isFull = 1;
    }

    //evict
    if (isFull){
        int indexOfleast = leastUsedLine(tempset, par_in.E);
        tempset.lines[indexOfleast].tag = query_tag;
        tempset.lines[indexOfleast].valid = 1;
        count.miss++;
        count.evict++;
        
        tempset.lines[indexOfleast].when_used = count.exOfsim_count;
        count.exOfsim_count++;

        if (par_in.ver_mode == 1)
            printf(" eviction");

        return count;
    }
    //miss
    else{
        //just load in empty line
        tempset.lines[empty_line].valid = 1;
        tempset.lines[empty_line].tag = query_tag;
        count.miss++;
        
        tempset.lines[empty_line].when_used = count.exOfsim_count;
        count.exOfsim_count++;

        if (par_in.ver_mode == 1)
            printf(" miss");

        return count;
    }

    return count;
}


int main(int argc, char* argv[])
{
    char c;
    parameter par_in;
    cache mycache;
    char* tracefile;
    AddrBit addr;       //address in tracefile
    char operation;
    int offset_size;
    count_res count;
    FILE* file;
    
    count.hit = 0;
    count.miss = 0;
    count.evict = 0;
    count.exOfsim_count = 0;
    par_in.ver_mode = 0;
    
    while((c=getopt(argc,argv,"s:E:b:t:vh")) != -1){
        switch(c){
            case 's':
                par_in.s = atoi(optarg);
                break;
            case 'E':
                par_in.E = atoi(optarg);
                break;
            case 'b':
                par_in.b = atoi(optarg);
                break;
            case 't':
                tracefile = optarg;
                break;
            case 'v':
                par_in.ver_mode = 1;
                break;
            case 'h':

                break;
            default:
                break;
        }
    }
    par_in.t = 64 - (par_in.s + par_in.b);
    par_in.S = pow(2, par_in.s);
    par_in.B = pow(2, par_in.b);

    mycache = initialize_cache(par_in.S, par_in.E, par_in.B);

    file = fopen(tracefile, "r");

    //simulating line by line
    if (file != NULL){
        while (fscanf(file, " %c %llx,%d", &operation, &addr, &offset_size) == 3){
            switch(operation){
                case 'I':
                    break;
                case 'M':
                    if (par_in.ver_mode == 1)
                        printf("M %llx,%d", addr, offset_size);

                    //simulate twice
                    count = simulation(mycache, addr, count, par_in);
                    count = simulation(mycache, addr, count, par_in);

                    if (par_in.ver_mode == 1)
                        printf("\n");
                    break;
                case 'S':
                    if (par_in.ver_mode == 1)
                        printf("S %llx,%d", addr, offset_size);

                    count = simulation(mycache, addr, count, par_in);

                    if (par_in.ver_mode == 1)
                        printf("\n");
                    break;
                case 'L':
                    if (par_in.ver_mode == 1)
                        printf("L %llx,%d", addr, offset_size);

                    count = simulation(mycache, addr, count, par_in);

                    if (par_in.ver_mode == 1)
                        printf("\n");
                    break;
                default:
                    break;
            }
        }
    }
    else
        printf("file is not existing\n");

    printSummary(count.hit, count.miss, count.evict);

    popcache(mycache, par_in);
    fclose(file);
    return 0;
}