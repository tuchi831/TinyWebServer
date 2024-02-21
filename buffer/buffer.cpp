#include "buffer.h"

// ��д�±��ʼ����vector<char>��ʼ��
Buffer::Buffer(int initBuffSize) : buffer_(initBuffSize), readPos_(0), writePos_(0)  {}  

// ��д��������buffer��С - д�±�
size_t Buffer::WritableBytes() const {
    return buffer_.size() - writePos_;
}

// �ɶ���������д�±� - ���±�
size_t Buffer::ReadableBytes() const {
    return writePos_ - readPos_;
}

// ��Ԥ���ռ䣺�Ѿ������ľ�û���ˣ����ڶ��±�
size_t Buffer::PrependableBytes() const {
    return readPos_;
}

const char* Buffer::Peek() const {
    
    return &buffer_[readPos_];
}

// ȷ����д�ĳ���
void Buffer::EnsureWriteable(size_t len) {
    if(len > WritableBytes()) {
        MakeSpace_(len);
    }
    assert(len <= WritableBytes());
}

// �ƶ�д�±꣬��Append��ʹ��
void Buffer::HasWritten(size_t len) {
    writePos_ += len;
}

// ��ȡlen���ȣ��ƶ����±�
void Buffer::Retrieve(size_t len) {
    readPos_ += len;
}

// ��ȡ��endλ��
void Buffer::RetrieveUntil(const char* end) {
    assert(Peek() <= end );
    Retrieve(end - Peek()); // endָ�� - ��ָ�� ����
}

// ȡ���������ݣ�buffer���㣬��д�±����,�ڱ�ĺ����л��õ�
void Buffer::RetrieveAll() {
    bzero(&buffer_[0], buffer_.size()); // ����ԭ������
    readPos_ = writePos_ = 0;
}

// ȡ��ʣ��ɶ���str
std::string Buffer::RetrieveAllToStr() {
    std::string str(Peek(), ReadableBytes());
    RetrieveAll();
    return str;
}

// дָ���λ��
const char* Buffer::BeginWriteConst() const {
    return &buffer_[writePos_];
}

char* Buffer::BeginWrite() {
    return &buffer_[writePos_];
}

// ���str��������
void Buffer::Append(const char* str, size_t len) {
    assert(str);
    EnsureWriteable(len);   // ȷ����д�ĳ���
    std::copy(str, str + len, BeginWrite());    // ��str�ŵ�д�±꿪ʼ�ĵط�
    HasWritten(len);    // �ƶ�д�±�
}

void Buffer::Append(const std::string& str) {
    Append(str.c_str(), str.size());
}

void Append(const void* data, size_t len) {
    Append(static_cast<const char*>(data), len);
}

// ��buffer�еĶ��±�ĵط��ŵ���buffer�е�д�±�λ��
void Append(const Buffer& buff) {
    Append(buff.Peek(), buff.ReadableBytes());
}

// ��fd�����ݶ�������������writable��λ��
ssize_t Buffer::ReadFd(int fd, int* Errno) {
    char buff[65535];   // ջ��
    struct iovec iov[2];
    size_t writeable = WritableBytes(); // �ȼ�¼��д����
    // ��ɢ���� ��֤����ȫ������
    iov[0].iov_base = BeginWrite();
    iov[0].iov_len = writeable;
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);

    ssize_t len = readv(fd, iov, 2);
    if(len < 0) {
        *Errno = errno;
    } else if(static_cast<size_t>(len) <= writeable) {   // ��lenС��writable��˵��д����������len
        writePos_ += len;   // ֱ���ƶ�д�±�
    } else {    
        writePos_ = buffer_.size(); // д��д����,�±��Ƶ����
        Append(buff, static_cast<size_t>(len - writeable)); // ʣ��ĳ���
    }
    return len;
}

// ��buffer�пɶ�������д��fd��
ssize_t Buffer::WriteFd(int fd, int* Errno) {
    ssize_t len = write(fd, Peek(), ReadableBytes());
    if(len < 0) {
        *Errno = errno;
        return len;
    } 
    Retrieve(len);
    return len;
}

char* Buffer::BeginPtr_() {
    return &buffer_[0];
}

const char* Buffer::BeginPtr_() const{
    return &buffer_[0];
}

// ��չ�ռ�
void Buffer::MakeSpace_(size_t len) {
    if(WritableBytes() + PrependableBytes() < len) {
        buffer_.resize(writePos_ + len + 1);
    } else {
        size_t readable = ReadableBytes();
        std::copy(BeginPtr_() + readPos_, BeginPtr_() + writePos_, BeginPtr_());
        readPos_ = 0;
        writePos_ = readable;
        assert(readable == ReadableBytes());
    }
}
