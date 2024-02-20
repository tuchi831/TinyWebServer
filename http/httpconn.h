#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include <sys/types.h>
#include <sys/uio.h>     // readv/writev
#include <arpa/inet.h>   // sockaddr_in
#include <stdlib.h>      // atoi()
#include <errno.h>      

#include "../log/log.h"
#include "../buffer/buffer.h"
#include "httprequest.h"
#include "httpresponse.h"
/*
���ж�д���ݲ�����httprequest �����������Լ�httpresponse��������Ӧ
*/
class HttpConn {
public:
    HttpConn();
    ~HttpConn();
    
    void init(int sockFd, const sockaddr_in& addr);
    ssize_t read(int* saveErrno);
    ssize_t write(int* saveErrno);
    void Close();
    int GetFd() const;
    int GetPort() const;
    const char* GetIP() const;
    sockaddr_in GetAddr() const;
    bool process();

    // д���ܳ���
    int ToWriteBytes() { 
        return iov_[0].iov_len + iov_[1].iov_len; 
    }

    bool IsKeepAlive() const {
        return request_.IsKeepAlive();
    }

    static bool isET;
    static const char* srcDir;
    static std::atomic<int> userCount;  // ԭ�ӣ�֧����
    
private:
   
    int fd_;
    struct  sockaddr_in addr_;

    bool isClose_;
    
    int iovCnt_;
    struct iovec iov_[2];
    
    Buffer readBuff_; // ��������
    Buffer writeBuff_; // д������

    HttpRequest request_;
    HttpResponse response_;
};

#endif
