#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<stdlib.h>
#include<netdb.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>
#include<string.h>
#include<iostream>
#include<arpa/inet.h>
#include<bitset>
#include"ftp_client.h"

using namespace std;

char* rbuf,*rbuf1,*wbuf,*wbuf1;
char filename[100];
char host[100];
bool bPort=1;
struct sockaddr_in servaddr;
in_addr localip;

int main(int argc,char* argv[]){

    if(argc!=2){
        printf("use:%s <hostname>\n",argv[0]);
        exit(0);
    }

    strcpy(host,argv[1]);
    int port=21;
    rbuf=(char*)malloc(MAXBUF*sizeof(char));
    rbuf1=(char*)malloc(MAXBUF*sizeof(char));
    wbuf=(char*)malloc(MAXBUF*sizeof(char));
    wbuf1=(char*)malloc(MAXBUF*sizeof(char));

    int cmdSock=cliopen(host,port);
    CmdTcp(cmdSock);
    exit(0);
    return 0;
}

int cliopen(char* host,int port){
    int sockid;
    struct hostent* ht=NULL;
    sockid=socket(AF_INET,SOCK_STREAM,0);
    if(sockid<0){
        printf("socket error.\n");
        return -1;
    }
    ///这个get 到的ip地址是网络字节序 NBO 如果是自己传入的ip地址不想调用gethost需要转为NBO
    ht=gethostbyname(host);
    if(ht==NULL)return -1;

    memset(&servaddr,0,sizeof(sockaddr_in));
    ///把第一个ip地址copy过来 都是NBO
    memcpy(&servaddr.sin_addr.s_addr,ht->h_addr,ht->h_length);
    servaddr.sin_family=AF_INET;
    servaddr.sin_port=htons(port);


    ///下面是验证hostent和sockaddr是大端还是小端
//    char* t=(char*)(ht->h_addr);
//    cout<<bitset<32>(*(int*)t)<<endl;
//    unsigned char u1=t[3];
//    printf("localip: %u.%u.%u.%u\n",(unsigned char)t[0],(unsigned char)t[1],(unsigned char)t[2]
//           ,(unsigned char)t[3]);
//    char ipbuf[20]={'\0'};
//    printf("ip:: %s\n",inet_ntoa(servaddr.sin_addr.s_addr);


//    free(ht);
    if(connect(sockid,(struct sockaddr*)&servaddr,sizeof(sockaddr))==-1)
        return -1;

    struct sockaddr_in sa;
    socklen_t scklen=sizeof(sa);
    if(getsockname(sockid,(struct sockaddr*)&sa,&scklen)!=0){
        printf("get local ip error.\n");
        close(sockid);
        return -1;
    }

//    localip=htonl(sa.sin_addr.s_addr);
//    char* t=(char*)&localip;
    printf("localip:%s\n",inet_ntoa(sa.sin_addr));
    localip=sa.sin_addr;

    return sockid;
}

int lisopen(char* host,in_addr* addr,int* dataPort){
    int dataSock=socket(AF_INET,SOCK_STREAM,0);
    if(dataSock<0){
        printf("socket error.\n");
        return -1;
    }
    struct hostent* ht=gethostbyname(host);
    struct sockaddr_in sa;
    bzero(&sa,sizeof(sockaddr_in));
    ///INADDR_ANY 是接受所有来源地址的连接请求
    sa.sin_addr.s_addr=htonl(INADDR_ANY);
    ///先随机分配一个
    sa.sin_port=htons(0);
    sa.sin_family=AF_INET;

    if(bind(dataSock,(struct sockaddr*)&sa,sizeof(sa))!=0){
        printf("bind error.\n");
        close(dataSock);
        return -1;
    }
    if(listen(dataSock,10)!=0){
        printf("listen error.\n");
        close(dataSock);
        return -1;
    }
    socklen_t scklen=sizeof(sa);
    if(getsockname(dataSock,(struct sockaddr*)&sa,&scklen)!=0){
        printf("get port error.\n");
        close(dataSock);
        return -1;
    }
    *dataPort=htons(sa.sin_port);
    *addr=localip;

//    printf("lisopen: addr:%s port:%d datasock:%d\n",inet_ntoa(*addr),*dataPort,dataSock);

    return dataSock;
}


void getFileName(char* buf,char* filename){
    sscanf(buf,"%*s%s",filename);
}

//"227 entering passive mode (127,0,0,1,4,18)"
int getPort(char* buf){
    int a[6];
    sscanf(buf,"%*[^(](%d,%d,%d,%d,%d,%d)",&a[0],&a[1],&a[2],&a[3],&a[4],&a[5]);
    bzero(host,strlen(host));
    sprintf(host,"%d.%d.%d.%d",a[0],a[1],a[2],a[3]);
    int port=(a[4]<<8)+a[5];
    return port;
}

int getRecvPort(int lisSock){
    struct sockaddr seraddr;
    socklen_t scklen=sizeof(sockaddr);
    int recvSock;
    if((recvSock=accept(lisSock,&seraddr,&scklen))==-1){
        printf("cannot accept data connection.\n");
        return -1;
    }
    return recvSock;
}

void FtpList(int dataSock){
    int nread;
    for(;;){
        if((nread=recv(dataSock,rbuf1,MAXBUF,0))<0){
            printf("recv error.\n");
        }else if(nread==0){
            break;
        }
        if(write(STDOUT_FILENO,rbuf1,nread)!=nread)
            printf("ftplist send to stdout error.\n");
    }
    if(close(dataSock)<0)
        printf("close dataSock error.\n");
}

void FtpGet(int dataSock,char* filename){
    ///TRUNC 有存在文件就清空  后面两个是创建文件的话提供的访问参数
    int fd=open(filename,O_WRONLY|O_CREAT|O_TRUNC,S_IWRITE|S_IREAD);
    int nread;
    for(;;){
        if((nread=recv(dataSock,rbuf1,MAXBUF,0))<0){
            printf("receive error.\n");
        }else if(nread==0){
            break;
        }
        if(write(fd,rbuf1,nread)!=nread)
            printf("receive error from server. write to local file error.\n");

    }
    if(close(dataSock)<0)
        printf("close dataSock error.\n");
}

void FtpPut(int dataSock,char* filename){
    int fd=open(filename,O_RDONLY);
    int nread;
    if(fd==-1){
        printf("no such file.\n");
        if(close(dataSock)<0)printf("close dataSock error.\n");
        return;
    }
    for(;;){
        if((nread=read(fd,rbuf1,MAXBUF))<0){
            printf("read file error.\n");
        }else if(nread==0)break;
        if(write(dataSock,rbuf1,nread)!=nread)
            printf("send file error.\n");
    }
    if(close(dataSock))
        printf("close dataSock error.\n");
}

void CmdTcp(int cmdSock){
    int maxfdp1,nread,nwrite,replycode,tag=0,dataSock,dataPort,recvSock;
    in_addr myaddr;
    fd_set rset;
    FD_ZERO(&rset);
    maxfdp1=cmdSock+1;
    for(;;){
        FD_SET(STDIN_FILENO,&rset);
        FD_SET(cmdSock,&rset);
        if(select(maxfdp1,&rset,NULL,NULL,NULL)<0){
            ///最后一个是时间 不设置等待时间
            printf("select error.\n");
        }
        if(FD_ISSET(STDIN_FILENO,&rset)){
            ///字节刷0的函数 跟memset差不多
            bzero(rbuf1,MAXBUF),bzero(wbuf,MAXBUF);
            ///读之多MAXBUF个字节，其实就是缓冲区大小
            if((nread=read(STDIN_FILENO,rbuf1,MAXBUF))<0)
                printf("read error from stdin.\n");


//            printf("stdin传来数据 replycode:%d\n",replycode);

            if(replycode==USERNAME){
                nwrite=sprintf(wbuf,"USER %s",rbuf1);
                if(write(cmdSock,wbuf,nwrite)!=nwrite)
                    printf("write error.\n");
            }else if(replycode==PASSWORD){
                nwrite=sprintf(wbuf,"PASS %s",rbuf1);
                if(write(cmdSock,wbuf,nwrite)!=nwrite)
                    printf("write error.\n");
            }else if(replycode==550||
                     replycode==LOGIN||
                     replycode==CLOSEDATA||
                     replycode==PATHNAME||
                     replycode==ACTIONOK){
                if(strncmp(rbuf1,"pwd",3)==0){
                    nwrite=sprintf(wbuf,"PWD\n");
                    if(write(cmdSock,wbuf,nwrite)!=nwrite)
                        printf("write error.\n");
                    continue;
                }else if(strncmp(rbuf1,"quit",4)==0){
                    nwrite=sprintf(wbuf,"QUIT\n");
                    if(write(cmdSock,wbuf,nwrite)!=nwrite)
                        printf("write error.\n");
                    if(close(cmdSock)<0)printf("close error\n");
                    break;
                }else if(strncmp(rbuf1,"cwd",3)==0){
                    ///其实ftp协议命令小写也是可以的
                    nwrite=sprintf(wbuf,"%s",rbuf1);
                    if(write(cmdSock,wbuf,nwrite)!=nwrite)
                        printf("write error\n");
                    continue;
                }else if(strncmp(rbuf1,"ls",2)==0){
                    tag=CMDLS;
                    if(bPort){
                        dataSock=lisopen(host,&myaddr,&dataPort);
                        unsigned char *t=(unsigned char*)&(localip.s_addr);
                        nwrite=sprintf(wbuf,"PORT %u,%u,%u,%u,%d,%d\n",t[0],t[1],t[2],t[3],dataPort/256,dataPort&255);
//                        printf("port send to server: %s",wbuf);
                        if(write(cmdSock,wbuf,nwrite)!=nwrite)
                            printf("write error.\n");
                    }else{
                        nwrite=sprintf(wbuf,"PASV\n");
                        if(write(cmdSock,wbuf,nwrite)!=nwrite)
                            printf("write error.\n");
                    }
                    continue;
                }else if(strncmp(rbuf1,"get",3)==0){
                    tag=CMDGET;//CMDLS;
                    if(bPort){
                        dataSock=lisopen(host,&myaddr,&dataPort);
                        unsigned char *t=(unsigned char*)&(localip.s_addr);
                        nwrite=sprintf(wbuf,"PORT %u,%u,%u,%u,%d,%d\n",t[0],t[1],t[2],t[3],dataPort/256,dataPort&255);
//                        printf("port send to server: %s",wbuf);
                        if(write(cmdSock,wbuf,nwrite)!=nwrite)
                            printf("write error.\n");
                        getFileName(rbuf1,filename);
//                        printf("filename for get:%s\n",filename);
                    }else{
                        nwrite=sprintf(wbuf,"PASV\n");
                        if(write(cmdSock,wbuf,nwrite)!=nwrite)
                            printf("write error.\n");
                        getFileName(rbuf1,filename);
//                        printf("filename for get:%s\n",filename);
                    }
                    continue;
                }else if(strncmp(rbuf1,"put",3)==0){
                    tag=CMDPUT;//CMDLS;
                    if(bPort){
                        dataSock=lisopen(host,&myaddr,&dataPort);
                        unsigned char *t=(unsigned char*)&(localip.s_addr);
                        nwrite=sprintf(wbuf,"PORT %u,%u,%u,%u,%d,%d\n",t[0],t[1],t[2],t[3],dataPort/256,dataPort&255);
//                        printf("port send to server: %s",wbuf);
                        if(write(cmdSock,wbuf,nwrite)!=nwrite)
                            printf("write error.\n");
                        getFileName(rbuf1,filename);
//                        printf("filename for put:%s\n",filename);
                    }else{
                        nwrite=sprintf(wbuf,"PASV\n");
                        getFileName(rbuf1,filename);
//                        printf("filename for put:%s\n",filename);
                        if(write(cmdSock,wbuf,nwrite)!=nwrite)
                            printf("write error.\n");
                    }
                    continue;
                }else if(strncmp(rbuf1,"pasv",4)==0){
                    bPort=false;
                    printf("using PASV.\n");
                }else if(strncmp(rbuf1,"port",4)==0){
                    bPort=true;
                    printf("using PORT.\n");
                }else{
                    printf("invalid command.\n");
                }
            }

        }

        //rbuf wbuf 都是 读写命令端口的  rbuf1 wbuf1是读写stdin stdout 数据端口的

        if(FD_ISSET(cmdSock,&rset)){
            bzero(rbuf,strlen(rbuf));
            if((nread=recv(cmdSock,rbuf,MAXBUF,0))<0)
                printf("recv error\n");
            else if(nread==0)break; ///检测到有服务器返回可读却返回0

            int numWrite=strlen(rbuf);
            if(write(STDOUT_FILENO,rbuf,numWrite)!=numWrite)
                printf("write error to stdout\n");

            ///220 对新用户的服务已准备好  550 未登录
            if(strncmp(rbuf,"220",3)==0||strncmp(rbuf,"550",3)==0){
                write(STDOUT_FILENO,"Name:",5);
                replycode=USERNAME;
            }else if(strncmp(rbuf,"331",3)==0){
                write(STDOUT_FILENO,"Password:",9);
                replycode=PASSWORD;
            }else if(strncmp(rbuf,"230",3)==0){
                replycode=LOGIN;
            }else if(strncmp(rbuf,"257",3)==0){
                replycode=PATHNAME;
            }else if(strncmp(rbuf,"226",3)==0){
                replycode=CLOSEDATA;
            }else if(strncmp(rbuf,"250",3)==0){
                replycode=ACTIONOK;
            }else if(strncmp(rbuf,"550",3)==0){
                replycode=550;
            }else if(strncmp(rbuf,"227",3)==0){
                //服务器进入被动模式  现在看看之前的tag要做什么
                int dataPort=getPort(rbuf);
//                printf("host:%s port:%d\n",host,dataPort);
                dataSock=cliopen(host,dataPort);
                if(tag==CMDLS){
                    ///先建立数据连接 再发送命令
                    nwrite=sprintf(wbuf,"list\n");
                    if(write(cmdSock,wbuf,nwrite)!=nwrite)
                        printf("write error\n");
                    FtpList(dataSock);
                }else if(tag==CMDGET){
                    nwrite=sprintf(wbuf,"RETR %s\n",filename);
                    if(write(cmdSock,wbuf,nwrite)!=nwrite)
                        printf("write error\n");
                    FtpGet(dataSock,filename);
                }else if(tag==CMDPUT){
                    nwrite=sprintf(wbuf,"STOR %s\n",filename);
                    if(write(cmdSock,wbuf,nwrite)!=nwrite)
                        printf("write error\n");
                    FtpPut(dataSock,filename);
                }

            }else if(strncmp(rbuf,"200",3)==0){
                ///服务器返回命令正常执行
                if(tag==CMDLS){
                    ///先建立数据连接 再发送命令
                    nwrite=sprintf(wbuf,"list\n");
                    if(write(cmdSock,wbuf,nwrite)!=nwrite)
                        printf("write error\n");
//                    int recvSock=getRecvPort(dataSock);
//                    FtpList(recvSock);
//                    close(dataSock);
                }else if(tag==CMDGET){
                    nwrite=sprintf(wbuf,"RETR %s\n",filename);
                    if(write(cmdSock,wbuf,nwrite)!=nwrite)
                        printf("write error\n");
//                    int recvSock=getRecvPort(dataSock);
//                    FtpGet(recvSock,filename);
//                    close(dataSock);
                }else if(tag==CMDPUT){
                    nwrite=sprintf(wbuf,"STOR %s\n",filename);
                    if(write(cmdSock,wbuf,nwrite)!=nwrite)
                        printf("write error\n");
//                    int recvSock=getRecvPort(dataSock);
//                    FtpPut(recvSock,filename);
//                    close(dataSock);
                }
            }else if(strncmp(rbuf,"150",3)==0){
                if(tag==CMDLS){
                    ///先建立数据连接 再发送命令
//                    nwrite=sprintf(wbuf,"list\n");
//                    if(write(cmdSock,wbuf,nwrite)!=nwrite)
//                        printf("write error\n");
                    int recvSock=getRecvPort(dataSock);
                    FtpList(recvSock);
                    close(dataSock);
                }else if(tag==CMDGET){
//                    nwrite=sprintf(wbuf,"RETR %s\n",filename);
//                    if(write(cmdSock,wbuf,nwrite)!=nwrite)
//                        printf("write error\n");
                    int recvSock=getRecvPort(dataSock);
                    FtpGet(recvSock,filename);
                    close(dataSock);
                }else if(tag==CMDPUT){
//                    nwrite=sprintf(wbuf,"STOR %s\n",filename);
//                    if(write(cmdSock,wbuf,nwrite)!=nwrite)
//                        printf("write error\n");
                    int recvSock=getRecvPort(dataSock);
                    FtpPut(recvSock,filename);
                    close(dataSock);
                }
            }



        }


    }




}































