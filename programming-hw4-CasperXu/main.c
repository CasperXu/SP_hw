#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#define cal(i,j) \
{ \
    int cnt=0; \
    if(i!=0&&j!=0&&src[i-1][j-1]=='O'){\
        cnt++;\
    }\
    if(i!=0&&j!=col-1&&src[i-1][j+1]=='O'){\
        cnt++;\
    }\
    if(i!=0&&src[i-1][j]=='O'){\
        cnt++;\
    }\
    if(j!=0&&src[i][j-1]=='O'){\
        cnt++;\
    }\
    if(i!=row-1&&src[i+1][j]=='O'){\
        cnt++;\
    }\
    if(j!=col-1&&src[i][j+1]=='O'){\
        cnt++;\
    }\
    if(i!=row-1&&j!=0&&src[i+1][j-1]=='O'){\
        cnt++;\
    }\
    if(i!=row-1&&j!=col-1&&src[i+1][j+1]=='O'){\
        cnt++;\
    }\
    if(src[i][j]=='O'){\
        if(cnt<2||cnt>3){\
            des[i][j] = '.';\
        }\
        else{\
            des[i][j] = 'O';\
        }\
    }\
    else{\
        if(cnt==3){\
            des[i][j] = 'O';\
        }\
        else{\
            des[i][j] = '.';\
        }\
    } \
}\

typedef struct argument{
    int start,end;
}Argument;

int row,col,flag;
char **src,**des;
void cal_row(int i){
    for(int j=0;j<col;j++){
        cal(i,j);
    }
}
void cal_col(int j){
    for(int i=0;i<row;i++){
        cal(i,j);
    }
}
void * thr_fn(void *arg){
    Argument *argument = (Argument *)arg;
    if(flag==1){
        for(int i=argument->start;i<argument->end;i++)
            cal_row(i);
    }
    else{
        for(int i=argument->start;i<argument->end;i++)
            cal_col(i);
    }
    pthread_exit((void *)0);
}
int main(int argc,char **argv){
    int quantity,epoch,seg,rest,now=0;
    quantity = atoi(argv[2]);
    pthread_t tid[quantity];
    Argument argument[quantity];
    FILE *in = fopen(argv[3],"r"),*out=fopen(argv[4],"w");
    fscanf(in,"%d%d%d",&row,&col,&epoch);
    // printf("%d %d %d\n",row,col,epoch);
    char **board1=(char **)malloc(sizeof(char *)*row);
    char **board2=(char **)malloc(sizeof(char *)*row);
    for(int i=0;i<row;i++){
        board1[i] = (char *)malloc(sizeof(char)*col);
        board2[i] = (char *)malloc(sizeof(char)*col);
    }
    src = board1;
    des = board2;
    for(int i=0;i<row;i++){
        fscanf(in,"%s",board1[i]);
    }
    if(row>col){
        flag = 1;
        seg = row/quantity;
        rest = row%quantity;
    }
    else{
        flag = 0;
        seg = col/quantity;
        rest = col%quantity;
    }
    for(int i=0;i<quantity;i++){
        argument[i].start = now;
        now+=seg;
        argument[i].end = now;
        if(rest){
            now++;
            argument[i].end = now;
            rest--;
        }
    }
    if(strcmp(argv[1],"-t")==0){
        for(int t=0;t<epoch;t++){
            for(int k=0;k<quantity;k++){
                pthread_create(&tid[k],NULL,thr_fn,&argument[k]);
            }
            for(int k=0;k<quantity;k++){
                pthread_join(tid[k],(void *)0);
            }
            char **tmp = src;
            src = des;
            des = tmp;
        }
    }
    else{
        int pid;
        for(int t=0;t<epoch;t++){
            if((pid=vfork())==0){
                for(int i=0;i<row;i++){
                    for(int j=0;j<col/2;j++){
                        cal(i,j);
                    }
                }
                _exit(0);
            }
            else{
                for(int i=0;i<row;i++){
                    for(int j=col/2;j<col;j++){
                        cal(i,j);
                    }
                }
                char **tmp = src;
                src = des;
                des = tmp;
                wait();
            }
        }     
    }
    for(int i=0;i<row;i++){
        fprintf(out,"%s%s",src[i],i==row-1?"":"\n");
    }
}