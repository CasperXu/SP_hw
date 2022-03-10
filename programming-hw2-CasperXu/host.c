#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <math.h>
#include <sys/wait.h>

int host_id,depth,lucky; 

void get_var(char *argv[]){
    for(int i=1;i<6;i=i+2){
        if(strcmp(argv[i],"-m")==0){
            host_id = atoi(argv[i+1]);
        }
        else if(strcmp(argv[i],"-d")==0){
            depth = atoi(argv[i+1]);
            argv[i+1][0]++;
        }
        else{
            lucky = atoi(argv[i+1]);
        }
    }
}
void fork_child(int rpipe[2][2],int wpipe[2][2],char *argv[]){
    int pid;
    pipe(rpipe[0]),pipe(rpipe[1]);
    pipe(wpipe[0]),pipe(wpipe[1]);
    if((pid=fork())==0){ //left child
        dup2(wpipe[0][0],0);
        dup2(rpipe[0][1],1);
        for(int i=0;i<2;i++){
            for(int j=0;j<2;j++){
                close(rpipe[i][j]);
                close(wpipe[i][j]);
            }
        }
        execv("./host",argv);
    }
    else if((pid=fork())==0){ //right child
        dup2(wpipe[1][0],0);
        dup2(rpipe[1][1],1);
        for(int i=0;i<2;i++){
            for(int j=0;j<2;j++){
                close(rpipe[i][j]);
                close(wpipe[i][j]);
            }
        }    
        execv("./host",argv);  
    }
    else{ //current process
        close(wpipe[0][0]),close(wpipe[1][0]);
        close(rpipe[0][1]),close(rpipe[1][1]);
    }
}
int find_winner(int guess[2][2]){
    if(abs(guess[0][1]-lucky)<abs(guess[1][1]-lucky)){
        return 0;
    }
    else if(abs(guess[0][1]-lucky)>abs(guess[1][1]-lucky)){
        return 1;
    }
    else{
        if(guess[0][0]>guess[1][0]){
            return 1;
        }
        else{
            return 0;
        }
    }
}
int main(int argc, char **argv){   
    get_var(argv);
    int rpi[2][2],wpi[2][2],wfi,rfi,player[8],board[13];
    FILE *rfp,*wfp,*fplr,*fplw,*fprr,*fprw,*test;
    if(depth==0){
        fork_child(rpi,wpi,argv);
        char fifo[15];
        sprintf(fifo,"fifo_%d.tmp",host_id);
        // test = fopen("test.txt","w");
        rfi = open(fifo, O_RDONLY), wfi = open("fifo_0.tmp",O_WRONLY);
        rfp = fdopen(rfi,"r");
        wfp = fdopen(wfi,"w");
        fplr = fdopen(rpi[0][0],"r");
        fplw = fdopen(wpi[0][1],"w");
        fprr = fdopen(rpi[1][0],"r");
        fprw = fdopen(wpi[1][1],"w");
    }
    else if(depth==1){
        fork_child(rpi,wpi,argv);
        fplr = fdopen(rpi[0][0],"r");
        fplw = fdopen(wpi[0][1],"w");
        fprr = fdopen(rpi[1][0],"r");
        fprw = fdopen(wpi[1][1],"w");
    }
    if(depth==0){
        while(1){
            for(int i=0;i<13;i++)
                board[i] = -1;
            for(int i=0;i<8;i++){
                fscanf(rfp,"%d",&player[i]);
                // scanf("%d",&player[i]);
                board[player[i]] = 0;
                // fprintf(test,"%d ",player[i]);
            }
            fprintf(fplw,"%d %d %d %d\n",player[0],player[1],player[2],player[3]);
            fflush(fplw);
            fprintf(fprw,"%d %d %d %d\n",player[4],player[5],player[6],player[7]);
            fflush(fprw);
            if(player[0]==-1){
                wait(NULL);
                wait(NULL);
                exit(0);
            }
            for(int t=0;t<10;t++){
                int guess[2][2];
                fscanf(fplr,"%d%d",&guess[0][0],&guess[0][1]);
                fscanf(fprr,"%d%d",&guess[1][0],&guess[1][1]);
                int res = find_winner(guess);   
                board[guess[res][0]] += 10;   
            }
            fprintf(wfp,"%d\n",host_id);
            for(int i=1;i<13;i++){
                if(board[i]!=-1){
                    fprintf(wfp,"%d %d\n",i,board[i]);
                }
            }
            fflush(wfp);
        }      
    }
    else if(depth==1){
        while(1){
            scanf("%d%d%d%d",&player[0],&player[1],&player[2],&player[3]);
            char buf[20];
            fprintf(fplw,"%d %d\n",player[0],player[1]);
            fflush(fplw);
            fprintf(fprw,"%d %d\n",player[2],player[3]);
            fflush(fprw);
            if(player[0]==-1){
                wait(NULL);
                wait(NULL);
                exit(0);
            }
            for(int t=0;t<10;t++){
                int guess[2][2];
                fscanf(fplr,"%d%d",&guess[0][0],&guess[0][1]);
                fscanf(fprr,"%d%d",&guess[1][0],&guess[1][1]);
                int res = find_winner(guess);   
                printf("%d %d\n",guess[res][0],guess[res][1]); 
            }
            fflush(stdout);
        }
    }
    else if(depth==2){
        while(1){
            int pid,rpipe[2],lpipe[2];
            char lplayer[3],rplayer[3];
            pipe(rpipe),pipe(lpipe);
            scanf("%s%s",lplayer,rplayer);
            if(atoi(lplayer)==-1){
                exit(0);
            }
            if((pid=fork())==0){ //left child
                dup2(lpipe[1],1);
                close(lpipe[1]);
                close(lpipe[0]);
                close(rpipe[1]);
                close(rpipe[0]);
                execl("./player","./player","-n",lplayer);
            }
            else if((pid=fork())==0){ //right child
                dup2(rpipe[1],1);
                close(lpipe[1]);
                close(lpipe[0]);
                close(rpipe[1]);
                close(rpipe[0]);
                execl("./player","./player","-n",rplayer);
            }
            close(rpipe[1]);
            close(lpipe[1]);
            fplr = fdopen(lpipe[0],"r");
            fprr = fdopen(rpipe[0],"r");
            for(int t=0;t<10;t++){
                int guess[2][2];
                fscanf(fplr,"%d%d",&guess[0][0],&guess[0][1]);
                fscanf(fprr,"%d%d",&guess[1][0],&guess[1][1]);
                int res = find_winner(guess);   
                printf("%d %d\n",guess[res][0],guess[res][1]);
            }
            fflush(stdout);
            wait(NULL);
            wait(NULL);
        }
    }
    exit(0);
}