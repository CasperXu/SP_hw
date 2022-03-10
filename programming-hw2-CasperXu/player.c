#include <stdio.h>
#include <stdlib.h>

int main(int argc,char **argv){
    int id = atoi(argv[2]);
    for(int r=1;r<=10;r++){
        int guess;
        /* initialize random seed: */
        srand((id+r)*323);
        /* generate guess between 1 and 1000: */
        guess = rand() % 1001;  
        printf("%d %d\n",id,guess);
        fflush(stdout);
    }
    exit(0);
}