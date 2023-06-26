#include "log.h"



Log::Log(): lineCount_(0),
            isAsync_(false),
            writeThread_(nullptr),
            deque_(nullptr),
            toDay_(0),
            fp_(nullptr)
{ }

Log::~Log() {
    if (writeThread_ && writeThread_->joinable()) {
        while (!deque_->empty()) {
            deque_->flush();
        }
        deque_->Close();
        writeThread_->join();
    }
    if (fp_) {
        std::lock_guard<std::mutex> lk(mtx_);
        flushAll();
        fclose(fp_);
    }
}

int Log::GetLevel() {
    std::lock_guard<std::mutex> lk(mtx_);
    return level_;
}

void Log::Setlevel(int level) {
    std::lock_guard<std::mutex> lk(mtx_);
    level_ = level;
}

void Log::init(int level, const char* path, const char* suffix, int maxQueueSize) {
    isOpen_ = true;
    level_ = level;
    if (maxQueueSize > 0) {
        isAsync_ = true;
        if (!deque_) {
            std::unique_ptr<BlockQueue<std::string>> newQueue(new BlockQueue<std::string>(maxQueueSize));
            deque_ = std::move(newQueue);

            std::unique_ptr<std::thread> newThread(new std::thread(FlushLogThread));
            writeThread_ = std::move(newThread);
        }
    }
    else {
        // 立即刷盘不需要 缓冲队列
        isAsync_ = false;
    }

    lineCount_ = 0;
    // 内置定时器
    time_t timer = time(nullptr); // 返回当前时间
    // 转换为日期时间
    struct tm* sysTime = localtime(&timer); // 把timer的时间赋给 tm 结构体
    path_ = path;
    suffix_ = suffix;
    // 以当前日期时间组织日志文件名
    char fileName[LOG_NAME_LEN];
    memset(fileName, 0, sizeof(fileName));
    snprintf(fileName, LOG_NAME_LEN - 1, "%s/%04u-%02u-%02u%s",
            path_, sysTime->tm_year + 1900, sysTime->tm_mon + 1, sysTime->tm_mday, suffix_);
    
    toDay_ = sysTime->tm_mday;

    {
        std::lock_guard<std::mutex> lk(mtx_);
        buff_.RetrieveAll();
        if (fp_) {
            flushAll();
            fclose(fp_);
        }

        fp_ = fopen(fileName, "a");
        if (fp_ == nullptr) {
            mkdir(path_, 0777);
            fp_ = fopen(fileName, "a");
        }
        assert(fp_ != nullptr);
    }
}

void Log::write(int level, const char* format, ...) {
    // 获取当天的时间: 时分秒
    struct timeval now = {0, 0};
    gettimeofday(&now, nullptr);

    time_t tSec = now.tv_sec;
    struct tm* sysTime = localtime(&tSec);
    
    va_list vaList; // 参数列表

    if (toDay_ != sysTime->tm_mday || (lineCount_ && (lineCount_ % MAX_LINES == 0))) {
        // 日期不对应 或者 当前文件满

        // 新建文件
        char newFile[LOG_NAME_LEN];
        memset(newFile, 0, sizeof(newFile));
        char tail[36];
        memset(tail, 0, sizeof(tail));
        snprintf(tail, 36, "%04u-%02u-%02u", sysTime->tm_year + 1900, sysTime->tm_mon + 1, sysTime->tm_mday);

        if (toDay_ != sysTime->tm_mday) {
            // 新的日期
            snprintf(newFile, LOG_NAME_LEN - 1, "%s/%s%s", path_, tail, suffix_);
            toDay_ = sysTime->tm_mday;
            lineCount_ = 0;    
        }
        else {
            // 日期不变但文件满
            snprintf(newFile, LOG_NAME_LEN - 1, "%s/%s_%d%s", path_, tail, (lineCount_ / MAX_LINES), suffix_);
        }

        {
            std::unique_lock<std::mutex> lk(mtx_);
            // flushAll();
            flush();
            fclose(fp_);
            fp_ = fopen(newFile, "a");
            assert(fp_ != nullptr);
        }
    }

    {
        std::unique_lock<std::mutex> lk(mtx_);
        // 写文件
        ++lineCount_;
        int n = snprintf(buff_.BeginWrite(), 128, "%04u-%02u-%02u %02u:%02u:%02u.%06ld",
                        sysTime->tm_year + 1900, sysTime->tm_mon + 1, sysTime->tm_mday, 
                        sysTime->tm_hour, sysTime->tm_min, sysTime->tm_sec, now.tv_usec);

        buff_.HasWritten(n); // 先写入时间
        AppendLogLevelTitle_(level);

        // 格式化提取参数
        va_start(vaList, format);
        int m = vsnprintf(buff_.BeginWrite(), buff_.WritableBytes(), format, vaList);
        va_end(vaList);

        buff_.HasWritten(m);
        buff_.Append("\n\0", 2);

        if (isAsync_ && deque_ && !deque_->full()) {
            deque_->push_back(buff_.RetrieveAllToStr());
        }
        else {
            // 非异步
            fputs(buff_.Peek(), fp_);
        }
        buff_.RetrieveAll();
    }

}

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

void Log::flush() {
    if (isAsync_) {
        deque_->flush();
    }

    fflush(fp_);
}

void Log::flushAll() {
    // 主线程先做刷盘
    if (isAsync_) {
        std::vector<std::string> tmp;
        tmp = deque_->GetAllData();

        for (int i = 0; i < tmp.size(); ++i) {
            fputs(tmp[i].c_str(), fp_);
        }
    }

    fflush(fp_);
}

void Log::AsyncWrite_() {
    std::string str;
    while (deque_->pop(str)) {
        std::lock_guard<std::mutex> lk(mtx_);
        fputs(str.c_str(), fp_);
    }
}

void Log::FlushLogThread() {
    Log::GetInstance()->AsyncWrite_();
}