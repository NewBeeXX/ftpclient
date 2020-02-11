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
#include"ftp_client.h"

using namespace std;

char* rbuf,*rbuf1,*wbuf,*wbuf1;
char filename[100];
char host[100];
bool bPort=false;
struct sockaddr_in servaddr;

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
        printf("socket error\n");
        return -1;
    }
    ///���get ����ip��ַ�������ֽ��� NBO ������Լ������ip��ַ�������gethost��ҪתΪNBO
    ht=gethostbyname(host);
    if(ht==NULL)return -1;
    memset(&servaddr,0,sizeof(sockaddr_in));
    ///�ѵ�һ��ip��ַcopy���� ����NBO
    memcpy(&servaddr.sin_addr.s_addr,ht->h_addr,ht->h_length);
    servaddr.sin_family=AF_INET;
    servaddr.sin_port=htons(port);
//    free(ht);
    if(connect(sockid,(struct sockaddr*)&servaddr,sizeof(sockaddr))==-1)
        return -1;
    return sockid;
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

void FtpList(int dataSock){
    int nread;
    for(;;){
        if((nread=recv(dataSock,rbuf1,MAXBUF,0))<0){
            printf("recv error\n");
        }else if(nread==0){
            break;
        }
        if(write(STDOUT_FILENO,rbuf1,nread)!=nread)
            printf("ftplist send to stdout error\n");
    }
    if(close(dataSock)<0)
        printf("close dataSock error\n");
}

void FtpGet(int dataSock,char* filename){
    ///TRUNC �д����ļ������  ���������Ǵ����ļ��Ļ��ṩ�ķ��ʲ���
    int fd=open(filename,O_WRONLY|O_CREAT|O_TRUNC,S_IWRITE|S_IREAD);
    int nread;
    for(;;){
        if((nread=recv(dataSock,rbuf1,MAXBUF,0))<0){
            printf("receive error\n");
        }else if(nread==0){
            break;
        }
        if(write(fd,rbuf1,nread)!=nread)
            printf("receive error from server. write to local file error.\n");

    }
    if(close(dataSock)<0)
        printf("close dataSock error\n");
}

void FtpPut(int dataSock,char* filename){
    int fd=open(filename,O_RDONLY);
    int nread;
    if(fd==-1){
        printf("no such file.\n");
        if(close(dataSock)<0)printf("close dataSock error\n");
        return;
    }
    for(;;){
        if((nread=read(fd,rbuf1,MAXBUF))<0){
            printf("read file error\n");
        }else if(nread==0)break;
        if(write(dataSock,rbuf1,nread)!=nread)
            printf("send file error\n");
    }
    if(close(dataSock))
        printf("close dataSock error\n");
}

void CmdTcp(int cmdSock){
    int maxfdp1,nread,nwrite,replycode,tag=0,dataSock;
    fd_set rset;
    FD_ZERO(&rset);
    maxfdp1=cmdSock+1;
    for(;;){
        FD_SET(STDIN_FILENO,&rset);
        FD_SET(cmdSock,&rset);
        if(select(maxfdp1,&rset,NULL,NULL,NULL)<0){
            ///���һ����ʱ�� �����õȴ�ʱ��
            printf("select error\n");
        }
        if(FD_ISSET(STDIN_FILENO,&rset)){
            ///�ֽ�ˢ0�ĺ��� ��memset���
            bzero(rbuf1,MAXBUF),bzero(wbuf,MAXBUF);
            ///��֮��MAXBUF���ֽڣ���ʵ���ǻ�������С
            if((nread=read(STDIN_FILENO,rbuf1,MAXBUF))<0)
                printf("read error from stdin\n");


//            printf("stdin�������� replycode:%d\n",replycode);

            if(replycode==USERNAME){
                nwrite=sprintf(wbuf,"USER %s",rbuf1);
                if(write(cmdSock,wbuf,nwrite)!=nwrite)
                    printf("write error\n");
            }else if(replycode==PASSWORD){
                nwrite=sprintf(wbuf,"PASS %s",rbuf1);
                if(write(cmdSock,wbuf,nwrite)!=nwrite)
                    printf("write error\n");
            }else if(replycode==550||
                     replycode==LOGIN||
                     replycode==CLOSEDATA||
                     replycode==PATHNAME||
                     replycode==ACTIONOK){
                if(strncmp(rbuf1,"pwd",3)==0){
                    nwrite=sprintf(wbuf,"PWD\n");
                    if(write(cmdSock,wbuf,nwrite)!=nwrite)
                        printf("write error\n");
                    continue;
                }else if(strncmp(rbuf1,"quit",4)==0){
                    nwrite=sprintf(wbuf,"QUIT\n");
                    if(write(cmdSock,wbuf,nwrite)!=nwrite)
                        printf("write error\n");
                    if(close(cmdSock)<0)printf("close error\n");
                    break;
                }else if(strncmp(rbuf1,"cwd",3)==0){
                    ///��ʵftpЭ������СдҲ�ǿ��Ե�
                    nwrite=sprintf(wbuf,"%s",rbuf1);
                    if(write(cmdSock,wbuf,nwrite)!=nwrite)
                        printf("write error\n");
                    continue;
                }else if(strncmp(rbuf1,"ls",2)==0){
                    tag=CMDLS;
                    if(bPort){

                    }else{
                        nwrite=sprintf(wbuf,"PASV\n");
                        if(write(cmdSock,wbuf,nwrite)!=nwrite)
                            printf("write error\n");
                    }
                    continue;
                }else if(strncmp(rbuf1,"get",3)==0){
                    tag=CMDGET;
                    if(bPort){

                    }else{
                        nwrite=sprintf(wbuf,"PASV\n");
                        getFileName(rbuf1,filename);
                        printf("filename for get:%s\n",filename);
                        if(write(cmdSock,wbuf,nwrite)!=nwrite)
                            printf("write error\n");
                    }

                    continue;
                }else if(strncmp(rbuf1,"put",3)==0){
                    tag=CMDPUT;
                    if(bPort){

                    }else{
                        nwrite=sprintf(wbuf,"PASV\n");
                        getFileName(rbuf1,filename);
                        printf("filename for put:%s\n",filename);
                        if(write(cmdSock,wbuf,nwrite)!=nwrite)
                            printf("write error\n");
                    }

                    continue;
                }else{
                    printf("invalid command.\n");
                }
            }

        }

        //rbuf wbuf ���� ��д����˿ڵ�  rbuf1 wbuf1�Ƕ�дstdin stdout ���ݶ˿ڵ�

        if(FD_ISSET(cmdSock,&rset)){
            bzero(rbuf,strlen(rbuf));
            if((nread=recv(cmdSock,rbuf,MAXBUF,0))<0)
                printf("recv error\n");
            else if(nread==0)break; ///��⵽�з��������ؿɶ�ȴ����0

            ///220 �����û��ķ�����׼����  550 δ��¼
            if(strncmp(rbuf,"220",3)==0||strncmp(rbuf,"550",3)==0){
                strcat(rbuf,"Name:");
                replycode=USERNAME;
            }else if(strncmp(rbuf,"331",3)==0){
                strcat(rbuf,"Password:");
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
                //���������뱻��ģʽ  ���ڿ���֮ǰ��tagҪ��ʲô
                int dataPort=getPort(rbuf);
                printf("host:%s port:%d\n",host,dataPort);
                dataSock=cliopen(host,dataPort);
                if(tag==CMDLS){
                    ///�Ƚ����������� �ٷ�������
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
                ///����������ģʽ��������
            }

            int numWrite=strlen(rbuf);
            if(write(STDOUT_FILENO,rbuf,numWrite)!=numWrite)
                printf("write error to stdout\n");

        }


    }




}































