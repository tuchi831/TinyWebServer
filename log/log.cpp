#include "log.h"

// ���캯��
Log::Log() {
    fp_ = nullptr;
    deque_ = nullptr;
    writeThread_ = nullptr;
    lineCount_ = 0;
    toDay_ = 0;
    isAsync_ = false;
}

Log::~Log() {
    while(!deque_->empty()) {
        deque_->flush();    // ���������ߣ������ʣ�µ�����
    }
    deque_->Close();    // �رն���
    writeThread_->join();   // �ȴ���ǰ�߳�������е�����
    if(fp_) {       // ��ϴ�ļ����������ر��ļ�������
        lock_guard<mutex> locker(mtx_);
        flush();        // ��ջ������е�����
        fclose(fp_);    // �ر���־�ļ�
    }
}

// �����������������ߣ���ʼд��־
void Log::flush() {
    if(isAsync_) {  // ֻ���첽��־�Ż��õ�deque
        deque_->flush();
    }
    fflush(fp_);    // ������뻺����
}

// ����ģʽ �ֲ���̬�����������ַ�������Ҫ�����ͽ���������
Log* Log::Instance() {
    static Log log;
    return &log;
}

// �첽��־��д�̺߳���
void Log::FlushLogThread() {
    Log::Instance()->AsyncWrite_();
}

// д�߳�������ִ�к���
void Log::AsyncWrite_() {
    string str = "";
    while(deque_->pop(str)) {
        lock_guard<mutex> locker(mtx_);
        fputs(str.c_str(), fp_);
    }
}

// ��ʼ����־ʵ��
void Log::init(int level, const char* path, const char* suffix, int maxQueCapacity) {
    isOpen_ = true;
    level_ = level;
    path_ = path;
    suffix_ = suffix;
    if(maxQueCapacity) {    // �첽��ʽ
        isAsync_ = true;
        if(!deque_) {   // Ϊ���򴴽�һ��
            unique_ptr<BlockQueue<std::string>> newQue(new BlockQueue<std::string>);
            // ��Ϊunique_ptr��֧����ͨ�Ŀ�����ֵ����,���Բ���move
            // ����̬������ڴ�Ȩ��deque��newDeque���ͷ�
            deque_ = move(newQue);  // ��ֵ����ֵ,�Ϳ�newDeque

            unique_ptr<thread> newThread(new thread(FlushLogThread));
            writeThread_ = move(newThread);
        }
    } else {
        isAsync_ = false;
    }

    lineCount_ = 0;
    time_t timer = time(nullptr);
    struct tm* systime = localtime(&timer);
    char fileName[LOG_NAME_LEN] = {0};
    snprintf(fileName, LOG_NAME_LEN - 1, "%s/%04d_%02d_%02d%s", 
            path_, systime->tm_year + 1900, systime->tm_mon + 1, systime->tm_mday, suffix_);
    printf("%s\n", fileName);

    toDay_ = systime->tm_mday;

   {
        lock_guard<mutex> locker(mtx_);
        buff_.RetrieveAll();
        if(fp_) {   // ���´�
            flush();
            fclose(fp_);
        }
        fp_ = fopen(fileName, "a"); // ���ļ���ȡ������д��
        
        if(fp_ == nullptr) {
            mkdir(fileName, 0777);
            
            fp_ = fopen(fileName, "a"); // ����Ŀ¼�ļ������Ȩ�ޣ�
        }
        assert(fp_ != nullptr);
    }
}

void Log::write(int level, const char *format, ...) {
    struct timeval now = {0, 0};
    gettimeofday(&now, nullptr);
    time_t tSec = now.tv_sec;
    struct tm *sysTime = localtime(&tSec);
    struct tm t = *sysTime;
    va_list vaList;

    // ��־���� ��־����  ������ǽ������������
    if (toDay_ != t.tm_mday || (lineCount_ && (lineCount_  %  MAX_LINES == 0)))
    {
        unique_lock<mutex> locker(mtx_);
        locker.unlock();
        
        char newFile[LOG_NAME_LEN];
        char tail[36] = {0};
        snprintf(tail, 36, "%04d_%02d_%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);

        if (toDay_ != t.tm_mday)    // ʱ�䲻ƥ�䣬���滻Ϊ���µ���־�ļ���
        {
            snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s%s", path_, tail, suffix_);
            toDay_ = t.tm_mday;
            lineCount_ = 0;
        }
        else {
            snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s-%d%s", path_, tail, (lineCount_  / MAX_LINES), suffix_);
        }
        
        locker.lock();
        flush();
        fclose(fp_);
        fp_ = fopen(newFile, "a");
        assert(fp_ != nullptr);
    }

    // ��buffer������һ����Ӧ����־��Ϣ
    {
        unique_lock<mutex> locker(mtx_);
        lineCount_++;
        int n = snprintf(buff_.BeginWrite(), 128, "%d-%02d-%02d %02d:%02d:%02d.%06ld ",
                    t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                    t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec);
                    
        buff_.HasWritten(n);
        AppendLogLevelTitle_(level);    

        va_start(vaList, format);
        int m = vsnprintf(buff_.BeginWrite(), buff_.WritableBytes(), format, vaList);
        va_end(vaList);

        buff_.HasWritten(m);
        buff_.Append("\n\0", 2);

        if(isAsync_ && deque_ && !deque_->full()) { // �첽��ʽ���������������У��ȴ�д�̶߳�ȡ��־��Ϣ��
            deque_->push_back(buff_.RetrieveAllToStr());
        } else {    // ͬ����ʽ��ֱ�����ļ���д����־��Ϣ��
            fputs(buff_.Peek(), fp_);   // ͬ����ֱ��д���ļ�
        }
        buff_.RetrieveAll();    // ���buff
    }
}

// �����־�ȼ�
void Log::AppendLogLevelTitle_(int level) {
    switch(level) {
    case 0:
        buff_.Append("[debug]: ", 9);
        break;
    case 1:
        buff_.Append("[info] : ", 9);
        break;
    case 2:
        buff_.Append("[warn] : ", 9);
        break;
    case 3:
        buff_.Append("[error]: ", 9);
        break;
    default:
        buff_.Append("[info] : ", 9);
        break;
    }
}

int Log::GetLevel() {
    lock_guard<mutex> locker(mtx_);
    return level_;
}

void Log::SetLevel(int level) {
    lock_guard<mutex> locker(mtx_);
    level_ = level;
}
