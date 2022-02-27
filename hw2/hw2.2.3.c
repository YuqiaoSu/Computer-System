
#include <stdio.h>
int main()
{
    char x1[100] = "The 25 quick brown foxes jumped over the 27 lazy dogs 17 times.";
    int  nwhite, nother;
    int ndigit[10];
    nwhite = nother = 0;
    char *x1ptr = x1;
    int *digitptr = ndigit;
    
    for (int i = 0; i < 10; ++i) {
        *digitptr = 0;
        digitptr++;
    }
    digitptr = digitptr - 10;
    while (*x1ptr != '\0') {
        if (*x1ptr >= '0' && *x1ptr<= '9')
            ++ndigit[*x1ptr-'0'];
        else if (*x1ptr == ' ' || *x1ptr == '\n' ||*x1ptr == '\t')
            ++nwhite;
        else
            ++nother;
        x1ptr++;
    }
    printf("digits =");
    for (int i = 0; i < 10; ++i) {
        printf(" %d", *digitptr);
        digitptr++;
    }
    printf(", white space = %d, other = %d\n",
        nwhite, nother);
    return 0;
}