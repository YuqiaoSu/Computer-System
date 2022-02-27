#include <stdio.h>
#define IN   1  /* inside a word */
#define OUT  0  /* outside a word */

int main(){
    char x1[100] = "The quick brown fox jumped over the lazy dog.\n";
    int i, nl, nw, nc,state;
    nl = nw = nc = 0;
    state = OUT;
    char *x1ptr = x1;
    while(*(x1ptr + i) != '\0'){
        nc++;
        if(*(x1ptr + i) == '\n'){
            nl++;
        }
        if(*(x1ptr + i) == ' ' || *(x1ptr + i) == '\n' || *(x1ptr + i) == '\t'){
              state= OUT;
        }
        else if (state == OUT){
            state = IN;
            nw++;
        }
        i++;
    }
    printf("%d %d %d\n", nl, nw, nc);
    return 0;


}