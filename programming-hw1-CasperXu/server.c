#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include <stdbool.h>

#define ERR_EXIT(a) do { perror(a); exit(1); } while(0)
#define MAX_FD 1024
#define MAX_ID 22

typedef struct {
    char hostname[512];  // server's hostname
    unsigned short port;  // port to listen
    int listen_fd;  // fd to wait for a new connection
} server;

typedef struct {
    char host[512];  // client's host
    int conn_fd;  // fd to talk with client
    char buf[512];  // data sent by/to client
    size_t buf_len;  // bytes used by buf
    // you don't need to change this.
    int id;
    int wait_for_write;  // used by handle_read to know if the header is read or not.
} request;

typedef struct {
    int id;          //902001-902020
    int AZ;          
    int BNT;         
    int Moderna;     
}registerRecord;

server svr;  // server
request* requestP = NULL;  // point to a list of requests
int maxfd=1024;  // size of open file descriptor table, size of request list

const char* accept_read_header = "ACCEPT_FROM_READ";
const char* accept_write_header = "ACCEPT_FROM_WRITE";

static void init_server(unsigned short port);
// initailize a server, exit for error

static void init_request(request* reqP);
// initailize a request instance

static void free_request(request* reqP);
// free resources used by a request instance



int handle_read(request* reqP) {
    int r;
    char buf[512];

    // Read in request from client
    r = read(reqP->conn_fd, buf, sizeof(buf));
    if (r < 0) return -1;
    if (r == 0) return 0;
    char* p1 = strstr(buf, "\015\012");
    int newline_len = 2;
    if (p1 == NULL) {
       p1 = strstr(buf, "\012");
        if (p1 == NULL) {
            ERR_EXIT("this really should not happen...");
        }
    }
    size_t len = p1 - buf + 1;
    memmove(reqP->buf, buf, len);
    reqP->buf[len - 1] = '\0';
    reqP->buf_len = len-1;
    return 1;
}


int main(int argc, char** argv) {

    // Parse args.
    if (argc != 2) {
        fprintf(stderr, "usage: %s [port]\n", argv[0]);
        exit(1);
    }

    struct sockaddr_in cliaddr;  // used by accept()
    int clilen;

    int conn_fd;  // fd for a new connection with client
    int file_fd;  // fd for file that we open for reading
    char buf[512]; 
    int buf_len;

    int record = open("registerRecord",O_RDWR);
    int curr_maxfd = 2;
    // Initialize server
    init_server((unsigned short) atoi(argv[1]));

    bool local_lock[MAX_ID] = {false};
    int stage[1024]={0};

    struct pollfd pollsvr;
    pollsvr.fd = svr.listen_fd;
    pollsvr.events = POLLIN;
    fd_set readset,masterset;
    struct timeval timeout;
    timeout.tv_usec = 50;
    timeout.tv_sec = 0;
    FD_ZERO(&masterset);
    FD_ZERO(&readset);
    // Loop for handling connections
    fprintf(stderr, "\nstarting on %.80s, port %d, fd %d, maxconn %d...\n", svr.hostname, svr.port, svr.listen_fd, maxfd);

    while (1) {
        // TODO: Add IO multiplexing
        // Check new connection
        if(poll(&pollsvr,1,50)){
            clilen = sizeof(cliaddr);
            conn_fd = accept(svr.listen_fd, (struct sockaddr*)&cliaddr, (socklen_t*)&clilen);
            if (conn_fd < 0) {
                if (errno == EINTR || errno == EAGAIN) continue;  // try again
                if (errno == ENFILE) {
                    (void) fprintf(stderr, "out of file descriptor table ... (maxconn %d)\n", maxfd);
                    continue;
                }
                ERR_EXIT("accept");
            }
            FD_SET(conn_fd,&masterset);
            if(curr_maxfd<conn_fd){
                curr_maxfd = conn_fd;
            }
            requestP[conn_fd].conn_fd = conn_fd;
            strcpy(requestP[conn_fd].host, inet_ntoa(cliaddr.sin_addr));
            fprintf(stderr, "getting a new request... fd %d from %s\n", conn_fd, requestP[conn_fd].host);
            write(requestP[conn_fd].conn_fd,"Please enter your id (to check your preference order):\n",56);
        }
        readset = masterset;
        if(select(1024,&readset,NULL,NULL,&timeout)>0){
            for(int i=0;i<=curr_maxfd;i++){
                if(FD_ISSET(i,&readset)&&stage[i]==0){
                    int ret = handle_read(&requestP[i]); // parse data from client to requestP[conn_fd].buf
                    fprintf(stderr, "ret = %d\n", ret);
                    if (ret < 0) {
                        fprintf(stderr, "bad request from %s\n", requestP[i].host);
                        continue;
                    }

                    requestP[i].id = atoi(requestP[i].buf);
                    if(!(requestP[i].id>=902001&&requestP[i].id<=902020&&strlen(requestP[i].buf)==6)){
                        write(requestP[i].conn_fd,"[Error] Operation failed. Please try again.\n",45);
                        FD_CLR(i,&masterset);
                        close(requestP[i].conn_fd);
                        free_request(&requestP[i]);
                        continue;
                    }
                    
                    struct flock *lock=(struct flock *)malloc(sizeof(struct flock));
                    int pos = requestP[i].id - 902001;
                    registerRecord r;
                    lock->l_type = F_WRLCK;
                    lock->l_start = pos*sizeof(registerRecord);
                    lock->l_whence = SEEK_SET;
                    lock->l_len = sizeof(registerRecord);
                    lock->l_pid = getpid();
                    if(fcntl(record,F_SETLK,lock)<0||local_lock[pos]==true){
                        write(requestP[i].conn_fd,"Locked.\n",9);
                        close(requestP[i].conn_fd);
                        FD_CLR(i,&masterset);
                        free_request(&requestP[i]);
                        continue;
                    }
                    local_lock[pos] = true;
                    pread(record,&r,sizeof(registerRecord),pos*sizeof(registerRecord));
                    write(requestP[i].conn_fd,"Your preference order is ",26);
                    for(int j=1;j<=3;j++){
                        if(r.AZ==j)
                            write(requestP[i].conn_fd,"AZ",3);
                        else if(r.Moderna==j)
                            write(requestP[i].conn_fd,"Moderna",8);   
                        else if(r.BNT==j)
                            write(requestP[i].conn_fd,"BNT",4);
                        if(j!=3)
                            write(requestP[i].conn_fd," > ",4);
                    }
                    write(requestP[i].conn_fd,".\n",3);
            #ifdef READ_SERVER      
                    fprintf(stderr, "%s", requestP[i].buf);
                    sprintf(buf,"%s : %s",accept_read_header,requestP[i].buf);
                    FD_CLR(i,&masterset);
                    close(requestP[i].conn_fd);
                    free_request(&requestP[i]);
                    lock->l_type = F_UNLCK;
                    lock->l_start = pos*sizeof(registerRecord);
                    lock->l_whence = SEEK_SET;
                    lock->l_len = sizeof(registerRecord);
                    lock->l_pid = getpid();
                    fcntl(record,F_SETLK,lock);
                    local_lock[pos] = false;
                    stage[i] = 0;

                    // write(requestP[conn_fd].conn_fd, buf, strlen(buf));    
            #elif defined WRITE_SERVER
                    fprintf(stderr, "%s", requestP[i].buf);
                    sprintf(buf,"%s : %s",accept_write_header,requestP[i].buf);

                    write(requestP[i].conn_fd,"Please input your preference order respectively(AZ,BNT,Moderna):\n",65);
                    stage[i] = 1;
                }
                else if(FD_ISSET(i,&readset)&&stage[i]==1){
                    struct flock *lock=(struct flock *)malloc(sizeof(struct flock)); 
                    int ret = handle_read(&requestP[i]),pos = requestP[i].id-902001;
                    fprintf(stderr, "ret = %d\n", ret);
                    if (ret < 0) {
                        fprintf(stderr, "bad request from %s\n", requestP[i].host);
                        lock->l_type = F_UNLCK;
                        lock->l_start = pos*sizeof(registerRecord);
                        lock->l_whence = SEEK_SET;
                        lock->l_len = sizeof(registerRecord);
                        lock->l_pid = getpid();
                        fcntl(record,F_SETLK,lock);
                        local_lock[pos] = false;
                        stage[i] = 0;
                        continue;
                    }
                    if(strlen(requestP[i].buf)!=5||requestP[i].buf[1]!=' '||requestP[i].buf[3]!=' '){
                        write(requestP[i].conn_fd,"[Error] Operation failed. Please try again.\n",45);
                        close(requestP[i].conn_fd);
                        free_request(&requestP[i]);
                        FD_CLR(i,&masterset);
                        lock->l_type = F_UNLCK;
                        lock->l_start = pos*sizeof(registerRecord);
                        lock->l_whence = SEEK_SET;
                        lock->l_len = sizeof(registerRecord);
                        lock->l_pid = getpid();
                        fcntl(record,F_SETLK,lock);
                        local_lock[pos] = false;
                        stage[i] = 0;
                        continue;            
                    }
                    int order[3];
                    registerRecord rw;
                    rw.id = requestP[i].id; 
                    order[0]=requestP[i].buf[0]-'1';
                    order[1]=requestP[i].buf[2]-'1';
                    order[2]=requestP[i].buf[4]-'1';
                    if((1<<order[0])+(1<<order[1])+(1<<order[2])==7){
                        char idbuf[20];
                        sprintf(idbuf,"%d",requestP[i].id);
                        write(requestP[i].conn_fd,"Preference order for ",22);
                        write(requestP[i].conn_fd,idbuf,strlen(idbuf));
                        write(requestP[i].conn_fd," modified successed, new preference order is ",46);
                        for(int j=0;j<3;j++){
                            if(order[0]==j){
                                rw.AZ = j+1;
                            }
                            else if(order[1]==j){
                                rw.BNT = j+1;
                            }
                            else{
                                rw.Moderna = j+1;
                            }
                        }
                        for(int j=1;j<=3;j++){
                            if(rw.AZ==j)
                                write(requestP[i].conn_fd,"AZ",3);
                            else if(rw.BNT==j)
                                write(requestP[i].conn_fd,"BNT",4);
                            else
                                write(requestP[i].conn_fd,"Moderna",8);
                            if(j!=3)
                                write(requestP[i].conn_fd," > ",4);
                        }           
                        write(requestP[i].conn_fd,".\n",3); 
                        pwrite(record,&rw,sizeof(registerRecord),pos*sizeof(registerRecord));
                    }
                    else{
                        write(requestP[i].conn_fd,"[Error] Operation failed. Please try again.\n",45);
                        close(requestP[i].conn_fd);
                        free_request(&requestP[i]);
                        FD_CLR(i,&masterset);
                        local_lock[pos] = false;
                        stage[i] = 0;
                        lock->l_type = F_UNLCK;
                        lock->l_start = pos*sizeof(registerRecord);
                        lock->l_whence = SEEK_SET;
                        lock->l_len = sizeof(registerRecord);
                        lock->l_pid = getpid();
                        fcntl(record,F_SETLK,lock);
                        continue;  
                    }
                    FD_CLR(i,&masterset);
                    close(requestP[i].conn_fd);
                    free_request(&requestP[i]);
                    lock->l_type = F_UNLCK;
                    lock->l_start = pos*sizeof(registerRecord);
                    lock->l_whence = SEEK_SET;
                    lock->l_len = sizeof(registerRecord);
                    lock->l_pid = getpid();
                    fcntl(record,F_SETLK,lock);
                    local_lock[pos] = false;
                    stage[i] = 0;
                    // write(requestP[conn_fd].conn_fd, buf, strlen(buf));    
            #endif
                }
            }
        }
    }
    free(requestP);
    return 0;
}

// ======================================================================================================
// You don't need to know how the following codes are working
#include <fcntl.h>

static void init_request(request* reqP) {
    reqP->conn_fd = -1;
    reqP->buf_len = 0;
    reqP->id = 0;
}

static void free_request(request* reqP) {
    /*if (reqP->filename != NULL) {
        free(reqP->filename);
        reqP->filename = NULL;
    }*/
    init_request(reqP);
}

static void init_server(unsigned short port) {
    struct sockaddr_in servaddr;
    int tmp;

    gethostname(svr.hostname, sizeof(svr.hostname));
    svr.port = port;

    svr.listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (svr.listen_fd < 0) ERR_EXIT("socket");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    tmp = 1;
    if (setsockopt(svr.listen_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&tmp, sizeof(tmp)) < 0) {
        ERR_EXIT("setsockopt");
    }
    if (bind(svr.listen_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        ERR_EXIT("bind");
    }
    if (listen(svr.listen_fd, 1024) < 0) {
        ERR_EXIT("listen");
    }

    // Get file descripter table size and initialize request table
    maxfd = getdtablesize();
    requestP = (request*) malloc(sizeof(request) * maxfd);
    if (requestP == NULL) {
        ERR_EXIT("out of memory allocating all requests");
    }
    for (int i = 0; i < maxfd; i++) {
        init_request(&requestP[i]);
    }
    requestP[svr.listen_fd].conn_fd = svr.listen_fd;
    strcpy(requestP[svr.listen_fd].host, svr.hostname);

    return 0;
}
