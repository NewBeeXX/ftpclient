#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>


#define MAXBUF 1024
#define STDIN_FILENO 1
#define STDOUT_FILENO 0


#define USERNAME 220
#define PASSWORD 331
#define LOGIN 230
#define PATHNAME 257
#define CLOSEDATA 226
#define ACTIONOK 250


char *rbuf,*rbuf1,*wbuf,*wbuf1;


char filename[100];
char *host;


struct sockaddr_in servaddr;


int cliopen(char *host,int port);
int strtosrv(char *str);
int ftp_get(int sck,char *pDownloadFileName);
int ftp_put(int sck,char *pUploadFileName_s);
void cmd_tcp(int sockfd);


int main(int argc,char *argv[]) {
    int fd;
    if(0 != argc -2) {
        printf("%s\n","missing <hostname>");
        exit(0);


    }
    host = argv[1];
    int port = 21;

    rbuf = (char *)malloc(MAXBUF*sizeof(char));
    rbuf1 = (char *)malloc(MAXBUF*sizeof(char));
    wbuf = (char *)malloc(MAXBUF*sizeof(char));
    wbuf1 = (char *)malloc(MAXBUF*sizeof(char));

    fd = cliopen(host,port);
    cmd_tcp(fd);
    exit(0);
}






int cliopen(char *host,int port) {
    int control_sock;
    struct hostent *ht = NULL;
    control_sock = socket(AF_INET,SOCK_STREAM,0);
    if(control_sock < 0) {
        printf("socket error\n");
        return -1;
    }
    ht = gethostbyname(host);
    if(!ht) {
        return -1;
    }
    //memcpy(&servaddr.sin_addr.s_addr,ht->h_addr,ht->h_length);

    memset(&servaddr,0,sizeof(struct sockaddr_in));
    memcpy(&servaddr.sin_addr.s_addr,ht->h_addr,ht->h_length);
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);

    if(connect(control_sock,(struct sockaddr*)&servaddr,sizeof(struct sockaddr)) == -1) {
        return -1;
    }
    return control_sock;
}


int s(char *str,char *s2) {
    //char s1[100];

    return sscanf(str," get %s",s2) == 1;

}


int st(char *str,char *s1) {
    return sscanf(str," put %s",s1) == 1;
}


int strtosrv(char *str) {
    int addr[6];
    //printf("%s\n",str);
    sscanf(str,"%*[^(](%d,%d,%d,%d,%d,%d)",&addr[0],&addr[1],&addr[2],&addr[3],&addr[4],&addr[5]);
    bzero(host,strlen(host));
    sprintf(host,"%d.%d.%d.%d",addr[0],addr[1],addr[2],addr[3]);
    int port = addr[4]*256 + addr[5];
    return port;
}


void ftp_list(int sockfd) {
    int nread;
    for(;;) {
        if((nread = recv(sockfd,rbuf1,MAXBUF,0)) < 0) {
            printf("recv error\n");
        } else if(nread == 0) {
            //printf("over\n");
            break;
        }
        if(write(STDOUT_FILENO,rbuf1,nread) != nread)
            printf("send error to stdout\n");
        /*else
            printf("read something\n");*/
    }
    if(close(sockfd) < 0)
        printf("close error\n");
}


int ftp_get(int sck,char *pDownloadFileName) {
    int handle = open(pDownloadFileName,O_WRONLY | O_CREAT | O_TRUNC, S_IREAD| S_IWRITE);
    int nread;
    printf("%d\n",handle);
    /*if(handle == -1)
        return -1;*/
    for(;;) {
        if((nread = recv(sck,rbuf1,MAXBUF,0)) < 0) {
            printf("receive error\n");
        } else if(nread == 0) {
            printf("over\n");
            break;
        }
        //   printf("%s\n",rbuf1);
        if(write(handle,rbuf1,nread) != nread)
            printf("receive error from server!");
        if(write(STDOUT_FILENO,rbuf1,nread) != nread)
            printf("receive error from server!");
    }
    if(close(sck) < 0)
        printf("close error\n");
}


int ftp_put(int sck,char *pUploadFileName_s) {
    //int c_sock;
    int handle = open(pUploadFileName_s,O_RDWR);
    int nread;
    if(handle == -1)
        return -1;
    //ftp_type(c_sock,"I");
    for(;;) {
        if((nread = read(handle,rbuf1,MAXBUF)) < 0) {
            printf("read error!");
        } else if(nread == 0)
            break;
        if(write(STDOUT_FILENO,rbuf1,nread) != nread)
            printf("send error!");
        if(write(sck,rbuf1,nread) != nread)
            printf("send error!");
    }
    if(close(sck) < 0)
        printf("close error\n");
}




void cmd_tcp(int sockfd) {
    int maxfdp1,nread,nwrite,fd,replycode,tag=0,data_sock;
    int port;
    char *pathname;
    fd_set rset;
    FD_ZERO(&rset);
    maxfdp1 = sockfd + 1;
    for(;;) {
        FD_SET(STDIN_FILENO,&rset);
        FD_SET(sockfd,&rset);
        if(select(maxfdp1,&rset,NULL,NULL,NULL)<0) {
            printf("select error\n");
        }
        if(FD_ISSET(STDIN_FILENO,&rset)) {
            //nwrite = 0;
            //nread = 0;
            bzero(wbuf,MAXBUF);          //zero
            bzero(rbuf1,MAXBUF);
            if((nread = read(STDIN_FILENO,rbuf1,MAXBUF)) <0)
                printf("read error from stdin\n");
            nwrite = nread + 5;
            //printf("%d\n",nread);
            if(replycode == USERNAME) {
                sprintf(wbuf,"USER %s",rbuf1);

                if(write(sockfd,wbuf,nwrite) != nwrite) {
                    printf("write error\n");
                }
                //printf("%s\n",wbuf);
                //memset(rbuf1,0,sizeof(rbuf1));
                //memset(wbuf,0,sizeof(wbuf));
                //printf("1:%s\n",wbuf);
            }


            if(replycode == PASSWORD) {
                //printf("%s\n",rbuf1);
                sprintf(wbuf,"PASS %s",rbuf1);
                if(write(sockfd,wbuf,nwrite) != nwrite)
                    printf("write error\n");
                //bzero(rbuf,sizeof(rbuf));
                //printf("%s\n",wbuf);
                //printf("2:%s\n",wbuf);
            }
            if(replycode == 550 || replycode == LOGIN || replycode == CLOSEDATA || replycode == PATHNAME || replycode == ACTIONOK) {
                if(strncmp(rbuf1,"pwd",3) == 0) {
                    //printf("%s\n",rbuf1);
                    sprintf(wbuf,"%s","PWD\n");
                    write(sockfd,wbuf,4);
                    continue;
                }
                if(strncmp(rbuf1,"quit",4) == 0) {
                    sprintf(wbuf,"%s","QUIT\n");
                    write(sockfd,wbuf,5);
                    //close(sockfd);  
                    if(close(sockfd) <0)
                        printf("close error\n");
                    break;
                }
                if(strncmp(rbuf1,"cwd",3) == 0) {
                    //sprintf(wbuf,"%s","PASV\n");
                    sprintf(wbuf,"%s",rbuf1);
                    write(sockfd,wbuf,nread);

                    //sprintf(wbuf1,"%s","CWD\n");

                    continue;
                }
                if(strncmp(rbuf1,"ls",2) == 0) {
                    tag = 2;
                    //printf("%s\n",rbuf1);
                    sprintf(wbuf,"%s","PASV\n");
                    //printf("%s\n",wbuf);
                    write(sockfd,wbuf,5);
                    //read
                    //sprintf(wbuf1,"%s","LIST -al\n");
                    nwrite = 0;
                    //write(sockfd,wbuf1,nwrite);
                    //ftp_list(sockfd);
                    continue;
                }
                if(strncmp(rbuf1,"get",3) == 0) {
                    tag = 1;
                    sprintf(wbuf,"%s","PASV\n");
                    //printf("%s\n",s(rbuf1));
                    //char filename[100];
                    s(rbuf1,filename);
                    printf("%s\n",filename);
                    write(sockfd,wbuf,5);
                    continue;
                }
                if(strncmp(rbuf1,"put",3) == 0) {
                    tag = 3;
                    sprintf(wbuf,"%s","PASV\n");
                    st(rbuf1,filename);
                    printf("%s\n",filename);
                    write(sockfd,wbuf,5);
                    continue;
                }
            }
            /*if(close(sockfd) <0)
               printf("close error\n");*/
        }
        if(FD_ISSET(sockfd,&rset)) {
            bzero(rbuf,strlen(rbuf));
            if((nread = recv(sockfd,rbuf,MAXBUF,0)) <0)
                printf("recv error\n");
            else if(nread == 0)
                break;

            if(strncmp(rbuf,"220",3) ==0 || strncmp(rbuf,"530",3)==0) {
                /*if(write(STDOUT_FILENO,rbuf,nread) != nread)
                    printf("write error to stdout\n");*/
                strcat(rbuf,"your name:");
                //printf("%s\n",rbuf);
                nread += 12;
                /*if(write(STDOUT_FILENO,rbuf,nread) != nread)
                    printf("write error to stdout\n");*/
                replycode = USERNAME;
            }
            if(strncmp(rbuf,"331",3) == 0) {
                /*if(write(STDOUT_FILENO,rbuf,nread) != nread)
                    printf("write error to stdout\n")*/;
                strcat(rbuf,"your password:");
                nread += 16;
                /*if(write(STDOUT_FILENO,rbuf,nread) != nread)
                    printf("write error to stdout\n");*/
                replycode = PASSWORD;
            }
            if(strncmp(rbuf,"230",3) == 0) {
                /*if(write(STDOUT_FILENO,rbuf,nread) != nread)
                    printf("write error to stdout\n");*/
                replycode = LOGIN;
            }
            if(strncmp(rbuf,"257",3) == 0) {
                /*if(write(STDOUT_FILENO,rbuf,nread) != nread)
                    printf("write error to stdout\n");*/
                replycode = PATHNAME;
            }
            if(strncmp(rbuf,"226",3) == 0) {
                /*if(write(STDOUT_FILENO,rbuf,nread) != nread)
                    printf("write error to stdout\n");*/
                replycode = CLOSEDATA;
            }
            if(strncmp(rbuf,"250",3) == 0) {
                /*if(write(STDOUT_FILENO,rbuf,nread) != nread)
                    printf("write error to stdout\n");*/
                replycode = ACTIONOK;
            }
            if(strncmp(rbuf,"550",3) == 0) {
                replycode = 550;
            }
            /*if(strncmp(rbuf,"150",3) == 0)
            {
               if(write(STDOUT_FILENO,rbuf,nread) != nread)
                   printf("write error to stdout\n");
            }*/
            //fprintf(stderr,"%d\n",1);
            if(strncmp(rbuf,"227",3) == 0) {
                //printf("%d\n",1);
                /*if(write(STDOUT_FILENO,rbuf,nread) != nread)
                   printf("write error to stdout\n");*/
                int port1 = strtosrv(rbuf);
                printf("%d\n",port1);
                printf("%s\n",host);
                data_sock = cliopen(host,port1);



//bzero(rbuf,sizeof(rbuf));
                //printf("%d\n",fd);
                //if(strncmp(rbuf1,"ls",2) == 0)
                if(tag == 2) {
                    write(sockfd,"list\n",strlen("list\n"));
                    ftp_list(data_sock);
                    /*if(write(STDOUT_FILENO,rbuf,nread) != nread)
                        printf("write error to stdout\n");*/

                }
                //else if(strncmp(rbuf1,"get",3) == 0)
                else if(tag == 1) {
                    //sprintf(wbuf,"%s","RETR\n");
                    //printf("%s\n",wbuf);
                    //int str = strlen(filename);
                    //printf("%d\n",str);
                    sprintf(wbuf,"RETR %s\n",filename);
                    printf("%s\n",wbuf);
                    //int p = 5 + str + 1;


                    printf("%d\n",write(sockfd,wbuf,strlen(wbuf)));
                    //printf("%d\n",p);
                    ftp_get(data_sock,filename);
                } else if(tag == 3) {
                    sprintf(wbuf,"STOR %s\n",filename);
                    printf("%s\n",wbuf);
                    write(sockfd,wbuf,strlen(wbuf));
                    ftp_put(data_sock,filename);
                }
                nwrite = 0;
            }
            /*if(strncmp(rbuf,"150",3) == 0)
            {
                if(write(STDOUT_FILENO,rbuf,nread) != nread)
                    printf("write error to stdout\n");
            }*/
            //printf("%s\n",rbuf);
            if(write(STDOUT_FILENO,rbuf,nread) != nread)
                printf("write error to stdout\n");
            /*else
                printf("%d\n",-1);*/
        }
    }
}
