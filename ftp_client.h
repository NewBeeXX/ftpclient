#ifndef FTP_CLIENT_H_INCLUDED
#define FTP_CLIENT_H_INCLUDED


#define MAXBUF 1024

#define CMDLS 2
#define CMDGET 1
#define CMDPUT 3

#define USERNAME 220
#define PASSWORD 331
#define LOGIN 230
#define PATHNAME 257
#define CLOSEDATA 226
#define ACTIONOK 250

int cliopen(char* host,int port);
int getPort(char* buf);
void getFileName(char* buf,char* filename);
void FtpGet(int sck,char* pDownLoadFileName);
void FtpPut(int sck,char* pUploadFileName);
void FtpList(int sck);
void CmdTcp(int sockfd);
int lisopen(char* host,in_addr* addr,int* dataPort);
#endif // FTP_CLIENT_H_INCLUDED
