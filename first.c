#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<string.h>

struct cache{
  int  valid;
  unsigned long long int tag;
  int lruPtr;
  int fifoPtr;
};

//for order in lru
int lruOrder = 1;

//NONPREFETCH
int hit = 0;
int miss= 0;
int read = 0;
int write = 0;
//PREFETCH
int preHit = 0;
int preMiss = 0;
int preRead = 0;
int preWrite = 0;

struct cache** createCache(int numOfSets,int assocN);

//update nonprefetch cache
void updateCache(char access, char* policy, struct cache** nCache, unsigned long long int address, int setIndex, int assocN, unsigned long long int currTag);

//update prefetch cache
void updatePreCache(char access, char* policy, int prefetchSize, struct cache** nCache, unsigned long long int address, int setIndex, int assocN, unsigned long long int currTag, int blockSize, int numOfSets);

//check for hit
int checkHit(unsigned long long int address, struct cache** nCache, int setIndex, int assocN, unsigned long long int currTag);

//if miss, input currTag into cache.tag
void writeToCache(char* policy, unsigned long long int address, struct cache** nCache, int setIndex, int assocN, unsigned long long int currTag);

//check if inputs are pwr of 2
int isPwrOfTwo(int num);//0=true, 1=false

void print();

int main(int argc, char** argv){
  if(argc!= 7 ){
    printf("error\n");
    return 0;
  }
  
  FILE* fp = fopen(argv[6], "r");
  
  int cacheSize = atoi(argv[1]);
  int blockSize = atoi(argv[2]);
  char* policy = argv[3];
  int prefetchSize = atoi(argv[5]);
  
  if(isPwrOfTwo(cacheSize)== 1 || isPwrOfTwo(blockSize)== 1 ){
    printf("error\n");
    return 0;
  }
  
  int assocN;//line in sets
  int numOfSets;
  if(strcmp(argv[4], "direct") == 0){
    assocN = 1; // one line in each set
    numOfSets = cacheSize/ blockSize;
  }
  else if(strcmp(argv[4], "assoc") == 0){
    assocN = cacheSize/ blockSize;
    numOfSets = 1;//one set with all lines
  }
  else{//assoc:n
    assocN = argv[4][6] - '0';
    numOfSets = cacheSize/(blockSize * assocN);
  }
  
  int numBlockBits = log2(blockSize);
  int numSetBits = log2(numOfSets);
  int mask = (1 << numSetBits) - 1;

  char access;
  unsigned long long int address;

  struct cache** nCache = createCache(numOfSets, assocN);
  struct cache** preCache = createCache(numOfSets, assocN);

  int setIndex;
  unsigned long long int currTag;

  while(fscanf(fp, "%c %llx\n", &access, &address)!=EOF && (access != '#') ){

    setIndex = (address >> numBlockBits) & mask;
    currTag = (address >> numBlockBits) >> numSetBits;

    updateCache(access, policy, nCache, address, setIndex, assocN, currTag);

    updatePreCache(access, policy, prefetchSize, preCache, address, setIndex, assocN, currTag, blockSize, numOfSets);
  }

  free(nCache);
  free(preCache);

  print();

  fclose(fp);
  return 0;
}


struct cache** createCache(int numOfSets,int assocN){
  int row, col;
  //row=numOfSets, col=assocN
  struct cache** nCache = malloc(sizeof(struct cache) * numOfSets);

  for(row = 0; row < numOfSets; row++){
    nCache[row] = malloc(sizeof(struct cache) * assocN);
    for(col = 0; col < assocN; col++){
      nCache[row][col].tag = 0;
      nCache[row][col].valid = 0;
      nCache[row][col].lruPtr = 0;
      nCache[row][col].fifoPtr = 0;
    }
  }
  return nCache;
}

int checkHit(unsigned long long int address, struct cache** nCache, int setIndex, int assocN, unsigned long long int currTag){
  //setIndex=Row; assocN=blocks
  int found = 0;//miss:found=0; hit:found=1
  int i;
  
  for(i = 0; i < assocN; i++){
    if(nCache[setIndex][i].valid != 0){
      if(nCache[setIndex][i].tag == currTag){
	//printf("%llu %llu\n", nCache[setIndex][i].tag, currTag);
	lruOrder++;
	nCache[setIndex][i].lruPtr = lruOrder;// for lru
	found = 1;
	break;
      }
    }
  }
  return found;
}

void writeToCache(char* policy, unsigned long long int address, struct cache** nCache, int setIndex, int assocN, unsigned long long int currTag){
  //setIndex=Row; assocN=blocks
  int i, j;
  int leastLru, leastBlock;
  if(strcmp(policy, "fifo") == 0){
    for(i = 0; i < assocN; i++){
      if(nCache[setIndex][i].valid == 0){//empty block
	nCache[setIndex][i].valid = 1;
	nCache[setIndex][i].tag = currTag;
	nCache[setIndex][i].fifoPtr = 1;
	return;
	//when all val=0 are filled, which all fifoPtr=1
	//then fifoPtr++, when [i][fifo]=[i+1][fifo], b/c i before i+1
	//thus: 111->211->221->222...
	
      }
    }//for valid=0 loop

    for(j = 0; j < assocN; j++){
      if(j != assocN-1){
	if(nCache[setIndex][j].fifoPtr == nCache[setIndex][j+1].fifoPtr){
	  nCache[setIndex][j].tag = currTag;
	  nCache[setIndex][j].fifoPtr++;
	  //211: 2!=1, thus move on
	  //211: 1=1, thus change tag and make fifoptr->221

	  return;
	}
      }//end of i+1!=assocN-1
      else if(j == assocN-1){//last block
	nCache[setIndex][j].tag = currTag;
	nCache[setIndex][j].fifoPtr++;
	//221, thus change tag and make fifoPtr-> 222;

	return;
      }//end else if 1=assocN-1
    }//end for block filled loop

  }//end for if fifo


  else if( strcmp(policy, "lru") == 0){
    
    for(i = 0; i < assocN; i++){//empty blocks
      if(nCache[setIndex][i].valid == 0){
	nCache[setIndex][i].valid = 1;
	nCache[setIndex][i].tag = currTag;
	lruOrder++;
	nCache[setIndex][i].lruPtr = lruOrder;
	return;

	//global variable: lruOrder; lruOrder++ for each address
	//with lruOrder as order of tags, used as lruPtr in evict
	//use for loop for all blocks in setIndex
	//block with least lruPtr evicted
      }
    }//end loop for lru empty

    leastLru = nCache[setIndex][0].lruPtr;
    leastBlock = 0;

    for(j = 0; j < assocN; j++){//evict with lru
      if(leastLru > nCache[setIndex][j].lruPtr){
	leastLru = nCache[setIndex][j].lruPtr;
	leastBlock = j;
	//printf("%d %d\n", leastLru, leastBlock);
      }
    }//leastBlock acquired
    
    nCache[setIndex][leastBlock].tag = currTag;
    lruOrder++;
    nCache[setIndex][leastBlock].lruPtr = lruOrder;
    return;
  }//END lru if
}

void updateCache(char access, char* policy, struct cache** nCache, unsigned long long int address, int setIndex, int assocN, unsigned long long int currTag){
  
  //if(strcmp(policy, "fifo") == 0){
    if(checkHit(address, nCache, setIndex, assocN, currTag) == 1){//HIT
      if(access == 'W'){
	hit++;
	write++;
	//printf("prefetched from list: %llx HIT ,%c HIT++ WRITE++\n",currTag, access);
      }
      else if(access == 'R'){
	hit++;
      }
      //printf("prefetched from list: %llx HIT ,%c HIT++\n",currTag, access);
      return;
    }//END of if HIT

    else{ // MISS
      writeToCache(policy, address, nCache, setIndex, assocN, currTag);
      //input currTag into nCache
      if(access == 'W'){
	miss++;
	write++;
	read++;
      }
      else if(access == 'R'){
	miss++;
	read++;
      }
      return;
    }//END of else MISS
    
    //}//end if fifo
}

void updatePreCache(char access, char* policy, int prefetchSize, struct cache** nCache, unsigned long long int address, int setIndex, int assocN, unsigned long long int currTag, int blockSize, int numOfSets){
  
  int i;

  if(checkHit(address, nCache, setIndex, assocN, currTag) == 1){//HIT
    if(access == 'W'){
      preHit++;
      preWrite++;
      //printf("prefetched from list: %llx  HIT, %c HIT++ WRITE++\n", currTag, access); 
    }
    else if(access == 'R'){
      preHit++;
      //printf("prefetched from list: %llx  HIT, %c HIT++\n", currTag, access);
    }
    return;
    
  }//END IF HIT
  else{// MISS
    //for original address
    writeToCache(policy, address, nCache, setIndex, assocN, currTag);
    if(access == 'W'){
      preMiss++;
      preRead++;
      preWrite++;
      //printf("prefetched from list: %llx MISS, %c  MISS++ READ++\n", currTag, access);
    }
    else if(access == 'R'){
      preMiss++;
      preRead++;
      //printf("prefetched from list: %llx MISS, %c  MISS++ READ++\n", currTag, access);
    }
    
    //PREFETCHING
      
    unsigned long long int prefetchAddress = address;
    for(i = 0; i < prefetchSize; i++){
      prefetchAddress = prefetchAddress + blockSize;
      
      int preNumSetBits;
      int preNumBlockBits;
      int preMask;
      
      int preSetIndex;
      unsigned long long int preCurrTag;
      
      preNumSetBits = log2(numOfSets);
      preNumBlockBits = log2(blockSize);
      preMask = (1 << preNumSetBits) - 1;
      
      preSetIndex = (prefetchAddress >> preNumBlockBits) & preMask;
      preCurrTag = (prefetchAddress >> preNumBlockBits) >> preNumSetBits;
      
      int found = checkHit(prefetchAddress, nCache, preSetIndex, assocN, preCurrTag);
      if(found == 1){
	//printf("prefetched from prefetch: %llx HIT, %c\n", currTag, access);
      }
      if(found == 0){
	//if prefetch miss
	writeToCache(policy, prefetchAddress, nCache, preSetIndex, assocN, preCurrTag);
	preRead++;
	//printf("prefetched from prefetch: %llx MISS, %c preRead++\n", currTag, access);
      }//if found == 0
    }//prefetching for loop END
    
    return;
      
  }//else MISS END

}

int isPwrOfTwo(int num){
  if(num==0) return 1;
  while(num!= 1){
    if(num%2 != 0){
      return 1;
    }
    num /= 2;
  }
  return 0;
}

void print(){
  printf("no-prefetch\n");
  printf("Memory reads: %d\n", read);
  printf("Memory writes: %d\n", write);
  printf("Cache hits: %d\n", hit);
  printf("Cache misses: %d\n", miss);

  printf("with-prefetch\n");
  printf("Memory reads: %d\n", preRead);
  printf("Memory writes: %d\n", preWrite);
  printf("Cache hits: %d\n", preHit);
  printf("Cache misses: %d\n", preMiss);
}
