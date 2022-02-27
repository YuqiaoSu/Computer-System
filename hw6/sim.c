#include <stdio.h>

typedef struct {
  int cacheSize;
  int blockSize;
} Cache;

struct Line{
  int index;
  int tag;
  int valid;
};


struct Line* current;
struct Line* head;
struct Line* tail;
int inputs[] = { 0, 16, 32, 48, 64, 80, 96, 112, 128, 144, 160, 176, 192, 208, 224, 240, 
256, 272, 288, 304, 320, 0, 16, 32, 48, 64, 284, 12, 164, 324, 156, 152, 284, 60, 156, 44, 204, 228, 164 };

// int inputs[] = {0,8,16,32};
//Initialize the cache table by setting everything to default

 void print_table(int count, struct Line *sets,int n){
   printf("\nCache contents (for each cache row index):");
   int i;
  for(i = 0; i < count; i++){ 
    int valid = (sets + i) -> valid;
    int tag = (sets + i) ->tag;
    if(valid){
      printf("\n%d: Valid: True; ",i);
    }
    else{
      printf("\n%d: Valid: false; ",i);
    }
    if(tag == -1){
      if(n == 1){
        printf("Tag: - (Set #:  0)");
      }
      else if( n == 0){
      printf("Tag: - (Set #:  %d)", i);
      }
      else{
        printf("Tag: - (Set #:  %d)", i / n);
      }
    }
    else{
      if(n ==1){
        printf("Tag: %d - (Set #:  0)",tag);
      }
      else if (n == 0){
      printf("Tag: %d (Set #:  %d)", tag, i);
      }
      else {
        printf("Tag: %d (Set #:  %d)", tag, i / n);
      }
    }
  }
 }
//8 byte cache line = 3 offset bits
//128 bytes: 128/8=16 sets  = 2^4    4 index bits
//input: 0-7 first set 0, offset 0-7
//input: 8-15 second set 1, offset 0
//Tag = input address/total size
void direct_map_simulator(Cache *simulator,struct Line *sets){
  int cacheSize = simulator -> cacheSize;//128
  int cacheBlock = simulator -> blockSize;//8
  int count = cacheSize / cacheBlock;//16


  int input_len = sizeof(inputs) / sizeof(int);
  int i,j;
  for(i = 0; i < input_len; i++){
    int input = inputs[i];
    int offset = input % cacheBlock;//0-7
    int index = (input % cacheSize) / cacheBlock;//0-15
   int tag = input / cacheSize;

   struct Line* temp = sets + index;

     int setTag = temp -> tag;
     if(setTag == tag){
       printf("\n%d:HIT  (Tag/Set#/Offset: %d/%d/%d)",input,tag,index,offset);
       temp -> valid = 1;
     }
     else{
       printf("\n%d:MISS  (Tag/Set#/Offset: %d/%d/%d)",input,tag,index,offset);
       temp -> tag = tag;
       temp -> valid = 1;
     }
  }
  
   print_table(count,&sets[0],0);




}


//64, 8,
void fully_assoc_simulator(Cache *simulator,struct Line const *sets){
  int cacheSize = simulator -> cacheSize;//128
  int cacheBlock = simulator -> blockSize;//8

  int count = cacheSize / cacheBlock;
  int input_len = sizeof(inputs) / sizeof(int);
    // number of bits in each part

  int i,j,setTag,valid;
  struct Line* temp;
    for(i = 0; i < input_len; i++){

      int input = inputs[i];
      int offset = input % cacheBlock;//0-7
      int tag = input / cacheBlock;
      int hit = 0;
      for(j = 0; j < count; j++){
         temp = sets + j;
        if(temp -> tag == tag){
          hit = 1;
          printf("\n%d:HIT  (Tag/Set#/Offset: %d/%d/%d)",input,tag,0,offset);
          break;
        }
    }

    if(hit == 0){
        current -> tag = tag;
        current -> valid = 1;
        current -> index = 0;
        current++;
        printf("\n%d:MISS  (Tag/Set#/Offset: %d/%d/%d)",input,tag,0,offset);
        if (current > tail) {
			  	current = head;
			  }
    }
    

  }
   print_table(count,&sets[0],1);
}

void n_Way_Associative (Cache *simulator,struct Line* sets,int n){

  int cacheSize = simulator -> cacheSize;//128
  int cacheBlock = simulator -> blockSize;//8

  int count = cacheSize / cacheBlock;
  int input_len = sizeof(inputs) / sizeof(int);

  int set = count / n;
  int sets_ptr[set];
  int i,j,ptr;
  for (i = 0; i < set; i++) {
		sets_ptr[i] = 0;
	}

  for (i = 0; i < input_len; i++) {

		int input = inputs[i];
		int tag = input / (cacheBlock * set);
		int sets_index = input / cacheBlock % set;
    int offset = input % cacheBlock;
		int hit = 0;

		for (j = sets_index * n; j < (sets_index + 1) * n; j++) {
			struct Line* temp = sets + j;
      int setTag = temp -> tag ;
			if (setTag == tag) {
				hit = 1;
         printf("\n%d:HIT  (Tag/Set#/Offset: %d/%d/%d)",input,tag,sets_index,offset);
				break;
			}
      else if (temp->valid == 0) {
				break;
			}
		}

		if (hit == 0) {
			int ptr = sets_ptr[sets_index];
			struct Line* out = sets + sets_index * n + ptr;
			out->tag = tag;
			out->valid = 1;
			ptr++;
			sets_ptr[sets_index] = ptr;
       printf("\n%d:MISS  (Tag/Set#/Offset: %d/%d/%d)",input,tag,sets_index,offset);
			if (ptr == n) {
				sets_ptr[sets_index] = 0;
			}
    }
  }
    print_table(count,&sets[0],n);

}


void initializeCache(int count, struct Line* sets){

  int i;
	for (i = 0; i < count; i++) {
    (sets + i)->index = 0;
    (sets + i)->tag = -1;
    (sets + i)->valid = 0;
  }
}

int main(){
/*struct that holds the parameters governing current simulation*/
  Cache simulator;

    int totalByte;
    int blockByte;
    int isDirect;
    int isAssoc;
    printf("Enter total bytes: ");
    scanf("%d", &totalByte);  
    simulator. cacheSize = totalByte;
    printf("Enter block bytes:");
    scanf("%d", &blockByte);  
    simulator.blockSize = blockByte;
    printf("Please modify global array input");

    int count = totalByte / blockByte;
    struct Line sets[count];
    initializeCache(count, &sets);
    head = &sets[0];
	  current = head;
	  tail = &sets[count - 1];

    printf("is direct-mapped?(1 for yes, 0 for no)");
    scanf("%d", &isDirect);
    if(isDirect){
      direct_map_simulator(&simulator,&sets[0]);
    }
    else{
    printf("is fully-associative?(1 for yes, 0 for no)");
    scanf("%d", &isAssoc);  
    if(isAssoc){
    fully_assoc_simulator(&simulator,&sets[0]);}
    else{
      int n;
      printf("n of n-Set-Associative cache?: ");
      scanf("%d", &n);
      n_Way_Associative(&simulator,&sets[0],n);
    }

    }
  return 0;
}
