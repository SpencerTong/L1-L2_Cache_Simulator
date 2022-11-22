#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>

//line 203 is error 

typedef struct block{
    int valid_bit;
    unsigned int tag_bits;
    unsigned int index_bits;
    unsigned int age;
    unsigned int orig_add;
} block;


int insertWhichBlock(block* indexedBloc, int assoc){
    for (int i=0; i<assoc; i++){
        if (indexedBloc[i].valid_bit==0)
        {
            return i;
        }
    }
    return -1;
}


int main (int argc, char *argv[]){
    
    unsigned int cache_size_l1 = atoi(argv[1]);
    char* assoc_string = argv[2];
    strtok(assoc_string, ":");

    unsigned int assoc_l1 = atoi(strtok(NULL, ":"));
    char* cache_policy_l1 = argv[3];
    unsigned int block_size = atoi(argv[4]);
    unsigned int total_sets_l1 = cache_size_l1/(assoc_l1*block_size);

    unsigned int cache_size_l2 = atoi(argv[5]);
    char* assoc2_string = argv[6];
    strtok(assoc2_string, ":");

    unsigned int assoc_l2 = atoi(strtok(NULL, ":"));
    unsigned int total_sets_l2 = cache_size_l2/(assoc_l2*block_size);
        
    block** cache1 = malloc(sizeof(block*)*total_sets_l1);
    for (int i =0; i<total_sets_l1; i++){
        cache1[i] = malloc(sizeof(block)*assoc_l1);
        for (int j = 0; j<assoc_l1; j++){
            cache1[i][j].valid_bit = 0;
        }
    }

    block** cache2 = malloc(sizeof(block*)*total_sets_l2);
    for (int i =0; i<total_sets_l2; i++){
        cache2[i] = malloc(sizeof(block)*assoc_l2);
        for (int j = 0; j<assoc_l2; j++){
            cache2[i][j].valid_bit = 0;
        }
    }

    FILE *fp;
    fp = fopen(argv[8], "r");
    
    char mode;
    unsigned int address;
    int global_count = 1;
    int mem_reads = 0;
    int mem_writes = 0;
    int cache1_hit_count = 0;
    int cache1_miss_count = 0;
    int cache2_hit_count = 0;
    int cache2_miss_count = 0;
    int index_size_l1 = log2(total_sets_l1);
    int offset_size_l1 = log2(block_size);

    int index_size_l2 = log2(total_sets_l2);
    int offset_size_l2 = log2(block_size);

    while (fscanf(fp,"%c %x\n", &mode, &address)!=EOF){

        unsigned int tag_bits_l1 = address>>(index_size_l1+offset_size_l1);
        unsigned int tag_bits_l2 = address>>(index_size_l2+offset_size_l2);

        unsigned int index_bits_l1 = (address>>offset_size_l1)&(total_sets_l1-1);
        unsigned int index_bits_l2 = (address>>offset_size_l2)&(total_sets_l2-1); 

        //check if address in L1
        int cache1_hit_check = 0;
        int cache2_hit_check = 0;

        for (int i =0; i<assoc_l1; i++){

            if (cache1[index_bits_l1][i].valid_bit==1&&cache1[index_bits_l1][i].tag_bits==tag_bits_l1)
            {
                cache1_hit_count++;
                if(mode=='W')
                    {
                        mem_writes++;
                    }
                cache1_hit_check = 1;
                if (strcmp(cache_policy_l1,"lru")==0)
                    {
                        cache1[index_bits_l1][i].age = global_count;
                        global_count++;
                    }
                break;
            }
        }

        //if miss in L1, now check L2

        if (cache1_hit_check==0){
            cache1_miss_count++;
            for (int i =0; i<assoc_l2; i++){
                //hit in L2
                if (cache2[index_bits_l2][i].valid_bit==1&&cache2[index_bits_l2][i].tag_bits==tag_bits_l2){
                    cache2_hit_count++;
                    if (mode=='W')
                    {
                        mem_writes++;
                    }
                    cache2_hit_check = 1;
                    //now must add to L1
                    //check for space in L1
                    int is_space_l1 = insertWhichBlock(cache1[index_bits_l1], assoc_l1);

                    if (is_space_l1!=-1) //space in L1 for L2 block
                    {
                        cache2[index_bits_l2][i].valid_bit=0;
                        cache1[index_bits_l1][is_space_l1].valid_bit=1;
                        cache1[index_bits_l1][is_space_l1].tag_bits=tag_bits_l1;
                        cache1[index_bits_l1][is_space_l1].index_bits=index_bits_l1;
                        cache1[index_bits_l1][is_space_l1].orig_add = address;
                        cache1[index_bits_l1][is_space_l1].age = global_count;
                        global_count++;
                    } else if (is_space_l1==-1) //no space in L1, must evict for L2 block
                    {
                        cache2[index_bits_l2][i].valid_bit=0;
                        int lowest_age_l1 = INT_MAX;
                        int block_to_replace_l1;
                            for (int i =0; i<assoc_l1; i++){
                                if (cache1[index_bits_l1][i].age<lowest_age_l1){
                                    block_to_replace_l1 = i;
                                    lowest_age_l1 = cache1[index_bits_l1][i].age;
                                }
                            }
                        //calculate where evicted will go in L2 & move addr. to L1
                        unsigned int temp_addr = cache1[index_bits_l1][block_to_replace_l1].orig_add;
                        cache1[index_bits_l1][block_to_replace_l1].valid_bit=1;
                        cache1[index_bits_l1][block_to_replace_l1].tag_bits=tag_bits_l1;
                        cache1[index_bits_l1][block_to_replace_l1].index_bits=index_bits_l1;
                        cache1[index_bits_l1][block_to_replace_l1].orig_add = address;
                        cache1[index_bits_l1][block_to_replace_l1].age = global_count;
                        global_count++;

                        unsigned int recalc_tag_bits_l2 = temp_addr>>(index_size_l2+offset_size_l2);
                        unsigned int recalc_index_bits_l2 = temp_addr>>offset_size_l2&(total_sets_l2-1);

                        int is_space_l2 = insertWhichBlock(cache2[recalc_index_bits_l2], assoc_l2);

                        //placing L1 addr. into L2
                        if (is_space_l2!=-1){ //L2 has space
                            cache2[recalc_index_bits_l2][is_space_l2].valid_bit=1;
                            cache2[recalc_index_bits_l2][is_space_l2].tag_bits=recalc_tag_bits_l2;
                            cache2[recalc_index_bits_l2][is_space_l2].index_bits=recalc_index_bits_l2;
                            cache2[recalc_index_bits_l2][is_space_l2].orig_add = temp_addr;
                            cache2[recalc_index_bits_l2][is_space_l2].age = global_count;
                            global_count++;
                        } else if (is_space_l2==-1){ //L2 no space, must evict
                            int lowest_age_l2 = INT_MAX;
                            int block_to_replace_l2;
                            for (int i =0; i<assoc_l2; i++){
                                if (cache2[recalc_index_bits_l2][i].age<lowest_age_l2){
                                    block_to_replace_l2 = i;
                                    lowest_age_l2 = cache2[recalc_index_bits_l2][i].age;
                                }
                            }
                            cache2[recalc_index_bits_l2][block_to_replace_l2].valid_bit=1;
                            cache2[recalc_index_bits_l2][block_to_replace_l2].tag_bits=recalc_tag_bits_l2;
                            cache2[recalc_index_bits_l2][block_to_replace_l2].index_bits=recalc_index_bits_l2;
                            cache2[recalc_index_bits_l2][block_to_replace_l2].orig_add = temp_addr;
                            cache2[recalc_index_bits_l2][block_to_replace_l2].age = global_count;
                            global_count++;
                        }
                    }
                }
            }
        }

        
        //Address in neither cache
        if (cache1_hit_check==0&&cache2_hit_check==0)
        {
            cache2_miss_count++;
            mem_reads++;
            if (mode=='W')
            {
                mem_writes++;
            }
            //check for room in L1
            int is_space_L1 = insertWhichBlock(cache1[index_bits_l1], assoc_l1);
            if (is_space_L1!=-1){ //room in L1
                cache1[index_bits_l1][is_space_L1].valid_bit=1;
                cache1[index_bits_l1][is_space_L1].tag_bits=tag_bits_l1;
                cache1[index_bits_l1][is_space_L1].index_bits=index_bits_l1;
                cache1[index_bits_l1][is_space_L1].orig_add=address;
                cache1[index_bits_l1][is_space_L1].age = global_count;
                global_count++;
            } else if (is_space_L1==-1){ //no room in L1 to bring from memory
                int lowest_age_l1 = INT_MAX;
                int block_to_replace_l1;
                for (int i =0; i<assoc_l1; i++){
                    if (cache1[index_bits_l1][i].age<lowest_age_l1){
                        block_to_replace_l1 = i;
                        lowest_age_l1 = cache1[index_bits_l1][i].age;
                        }
                    }
                unsigned int temp_addr = cache1[index_bits_l1][block_to_replace_l1].orig_add;
                cache1[index_bits_l1][block_to_replace_l1].valid_bit=1;
                cache1[index_bits_l1][block_to_replace_l1].tag_bits=tag_bits_l1;
                cache1[index_bits_l1][block_to_replace_l1].index_bits=index_bits_l1;
                cache1[index_bits_l1][block_to_replace_l1].orig_add = address;
                cache1[index_bits_l1][block_to_replace_l1].age = global_count;
                global_count++;

                unsigned int recalc_tag_bits_l2 = temp_addr>>(index_size_l2+offset_size_l2);
                unsigned int recalc_index_bits_l2 = temp_addr>>offset_size_l2&(total_sets_l2-1);
    
                int is_space_l2 = insertWhichBlock(cache2[recalc_index_bits_l2], assoc_l2);

                //placing evicted L1 addr. into L2
                if (is_space_l2!=-1){ //L2 has space
                    cache2[recalc_index_bits_l2][is_space_l2].valid_bit=1;
                    cache2[recalc_index_bits_l2][is_space_l2].tag_bits=recalc_tag_bits_l2;
                    cache2[recalc_index_bits_l2][is_space_l2].index_bits=recalc_index_bits_l2;
                    cache2[recalc_index_bits_l2][is_space_l2].orig_add = temp_addr;
                    cache2[recalc_index_bits_l2][is_space_l2].age = global_count;
                    global_count++;
                } else if (is_space_l2==-1){ //no room in L2, must evict as well
                    int lowest_age_l2 = INT_MAX;
                    int block_to_replace_l2;
                    for (int i =0; i<assoc_l2; i++){
                        if (cache2[recalc_index_bits_l2][i].age<lowest_age_l2){
                            block_to_replace_l2 = i;
                            lowest_age_l2 = cache2[recalc_index_bits_l2][i].age;
                            }
                        }
                        cache2[recalc_index_bits_l2][block_to_replace_l2].valid_bit=1;
                        cache2[recalc_index_bits_l2][block_to_replace_l2].tag_bits=recalc_tag_bits_l2;
                        cache2[recalc_index_bits_l2][block_to_replace_l2].index_bits=recalc_index_bits_l2;
                        cache2[recalc_index_bits_l2][block_to_replace_l2].orig_add = temp_addr;
                        cache2[recalc_index_bits_l2][block_to_replace_l2].age = global_count;
                        global_count++;
                }
            }
        }
    }

    printf("memread:%d\nmemwrite:%d\nl1cachehit:%d\nl1cachemiss:%d\n", mem_reads, mem_writes, cache1_hit_count, cache1_miss_count);
    printf("l2cachehit:%d\nl2cachemiss:%d\n", cache2_hit_count, cache2_miss_count);    

    for (int i = 0; i<total_sets_l1; i++){
        free(cache1[i]);
    }
    free(cache1);

    for (int i = 0; i<total_sets_l2; i++){
        free(cache2[i]);
    }
    free(cache2);
    
    return 0;
}