#include <stdio.h>
int main()
{
    char x1[100] = "The 25 quick brown foxes jumped over the 27 lazy dogs 17 times.";
    int i, nwhite, nother;
    int ndigit[10];
    nwhite = nother = 0;
    for (i = 0; i < 10; ++i)
        ndigit[i] = 0;
    i = 0;
    while (x1[i] != '\0') {
        if (x1[i] >= '0' && x1[i] <= '9')
            ++ndigit[x1[i]-'0'];
        else if (x1[i] == ' ' || x1[i] == '\n' || x1[i] == '\t')
            ++nwhite;
        else
            ++nother;
        i++;
    }
    printf("digits =");
    for (i = 0; i < 10; ++i)
        printf(" %d", ndigit[i]);
    printf(", white space = %d, other = %d\n",
        nwhite, nother);
    return 0;
}