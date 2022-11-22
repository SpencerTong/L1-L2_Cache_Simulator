#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>

typedef struct block{
    int valid_bit;
    unsigned int tag_bits;
    unsigned int index_bits;
    unsigned int age;
} block;


int main (int argc, char *argv[]){

    unsigned int cache_size = atoi(argv[1]);
    char* assoc_string = argv[2];
    strtok(assoc_string, ":");

    unsigned int assoc = atoi(strtok(NULL, ":"));
    char* cache_policy = argv[3];
    unsigned int block_size = atoi(argv[4]);
    unsigned int total_sets = cache_size/(assoc*block_size);
    
    //2d array, total_sets which each hold assoc block
    //block is a struct
    

    block** cache = malloc(sizeof(block*)*total_sets);
    for (int i =0; i<total_sets; i++){
        cache[i] = malloc(sizeof(block)*assoc);
        for (int j = 0; j<assoc; j++){
            cache[i][j].age = 0;
            cache[i][j].index_bits = 0;
            cache[i][j].tag_bits = 0;
            cache[i][j].valid_bit = 0;
        }
    }

    FILE *fp;
    fp = fopen(argv[5], "r");
    
    char mode;
    unsigned int address;
    int global_count = 1; 
    int mem_reads = 0;
    int mem_writes = 0;
    int cache_hit = 0;
    int cache_miss = 0;

    int index_size = log2(total_sets);
    int offset_size = log2(block_size);

    while (fscanf(fp,"%c %x\n", &mode, &address)!=EOF){

        unsigned int tag_bits = address>>(index_size+offset_size);
        unsigned int index_bits = (address>>(offset_size))&(total_sets-1);
        int cache_miss_check = 1;
        int is_space = -1;
            for (int i = 0; i<assoc; i++){
                
                if (cache[index_bits][i].valid_bit==0){ //check if space in set
                    is_space = i;
                }

                if (cache[index_bits][i].valid_bit==1){ //cache hit
                    if (cache[index_bits][i].tag_bits==tag_bits){
                        cache_hit++;
                        if(mode=='W'){
                            mem_writes++;
                        }
                        cache_miss_check = 0;
                        if (strcmp(cache_policy,"lru")==0){
                            cache[index_bits][i].age = global_count;
                            global_count++;
                        }
                        break;
                    }
                } //----------------------------------------
            }
            if (cache_miss_check==1){ //cache miss 
                    cache_miss++;
                    mem_reads++;
                    if(mode=='W'){
                            mem_writes++;
                    }
                    if (is_space!=-1){ //room in block
                        cache[index_bits][is_space].valid_bit = 1;
                        cache[index_bits][is_space].tag_bits = tag_bits;
                        cache[index_bits][is_space].index_bits = index_bits;
                        cache[index_bits][is_space].age = global_count;
                        global_count++;
                    } else { // must replace (miss and no space)
                        int lowest_age = INT_MAX;
                        int block_to_replace;
                            for (int i =0; i<assoc; i++){
                                if (cache[index_bits][i].age<lowest_age){
                                    block_to_replace = i;
                                    lowest_age = cache[index_bits][i].age;
                                }
                            }
                        cache[index_bits][block_to_replace].index_bits = index_bits;
                        cache[index_bits][block_to_replace].tag_bits = tag_bits;
                        cache[index_bits][block_to_replace].age = global_count;
                        global_count++;
                        
                    }
            }

            
    }
    
    printf("memread:%d\nmemwrite:%d\ncachehit:%d\ncachemiss:%d\n", mem_reads, mem_writes, cache_hit, cache_miss);
    
    for (int i = 0; i<total_sets; i++){
        free(cache[i]);
    }
    free(cache);

    return 0;
}
