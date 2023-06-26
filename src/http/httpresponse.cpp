#include "httpresponse.h"



const std::unordered_map<std::string, std::string> HttpResponse::SUFFIX_TYPE = {
    {".html", "text/html"},
    {".xml",  "text/xml"},
    {".xhtml","application/xhtml+xml"},
    {".txt",  "text/plain"},
    {".rtf",  "application/rtf"},
    {".pdf",  "application/pdf"},
    {".word", "application/nsword"},
    {".png",  "image/png"},
    {".gif",  "image/gif"},
    {".jpg",  "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".au",   "audio/basic"},
    {".mpeg", "video/mpeg"},
    {".mpg",  "video/mpeg"},
    {".avi",  "video/x-msvideo"},
    {".gz",   "application/x-gzip"},
    {".tar",  "application/x-tar"},
    {".css",  "text/css "},
    {".js",   "text/javascript "}
};

const std::unordered_map<int, std::string> HttpResponse::CODE_STATUS = {
    {200, "OK"},
    {400, "Bad Request"},
    {403, "Forbidden"},
    {404, "Not Found"}
};

const std::unordered_map<int, std::string> HttpResponse::CODE_PATH = {
    {400, "/400.html"},
    {403, "/403.html"},
    {404, "/404.html"}
};


HttpResponse::HttpResponse() : code_(-1),
                               isKeepAlive_(false),
                               mmFile_(nullptr)
{
    memset(&mmFileStat_, 0, sizeof(mmFileStat_));
}

HttpResponse::~HttpResponse() {
    UnmapFile();
}


void HttpResponse::Init(const std::string& srcDir, std::string& path, bool isKeepAlive, int code) {
    assert(srcDir != "");
    if (mmFile_) {
        UnmapFile();
    }
    code_ = code;
    isKeepAlive_ = isKeepAlive;
    path_ = path;
    srcDir_ = srcDir;
    mmFile_ = nullptr;
    memset(&mmFileStat_, 0, sizeof(mmFileStat_));
}

void HttpResponse::MakeResponse(Buffer& buff) {
    // 判断请求的文件
    // 文件错误 或 该文件的类型是文件夹
    if (stat((srcDir_ + path_).c_str(), &mmFileStat_) < 0 || S_ISDIR(mmFileStat_.st_mode)) {
        code_ = 404;
    }
    else if (!(mmFileStat_.st_mode & S_IROTH)) {
        // 其他用户可读(Forbidden)
        code_ = 403;
    }
    else if (code_ == -1) {
        code_ = 200;
    }

    ErrorHtml_();
    AddStateLine_(buff);
    AddHeader_(buff);
    AddContent_(buff);
}


void HttpResponse::ErrorHtml_() {
    if (CODE_PATH.count(code_)) {
        path_ = CODE_PATH.at(code_);
        stat((srcDir_ + path_).c_str(), &mmFileStat_);
    }
}

void HttpResponse::AddStateLine_(Buffer& buff) {
    std::string status;
    // 状态码映射状态信息
    if (CODE_STATUS.count(code_)) {
        status = CODE_STATUS.at(code_);
    }
    else {
        code_ = 400;
        status = CODE_STATUS.at(400);
    }
    // 添加状态行
    buff.Append("HTTP/1.1 " + std::to_string(code_) + " " + status + "\r\n");
}

void HttpResponse::AddHeader_(Buffer& buff) {
    buff.Append("Connection: ");
    if (isKeepAlive_) {
        buff.Append("keep-alive\r\n");
        buff.Append("keep-alive: max=6, timeout=60\r\n");
    }
    else {
        buff.Append("close\r\n");
    }
    buff.Append("Content-type: " + GetFileType_() + "\r\n");
}

void HttpResponse::AddContent_(Buffer& buff) {
    // 以只读方式打开
    int srcFd = open((srcDir_ + path_).c_str(), O_RDONLY);
    if (srcFd < 0) {
        ErrorContent(buff, "File NotFound!");
        return;
    }

    /*
        将文件映射到内存提高文件的访问速度
        MAP_PRIVATE 建立一个写入时拷贝的私有映射
    */
    LOG_DEBUG("Content file path: %s", (srcDir_ + path_).c_str());
    // 把 文件 fd 的 offset 偏移量开始的 length 字节 映射到 虚拟空间 start 地址开始的 lenth 字节
    int* mmRet = (int*)mmap(NULL, mmFileStat_.st_size, PROT_READ, MAP_PRIVATE, srcFd, 0);
    if (*mmRet == -1) {
        ErrorContent(buff, "File NotFound!");
        return;
    }
    mmFile_ = (char*)mmRet; // 记录映射的首地址
    close(srcFd);
    buff.Append("Content-length: " + std::to_string(mmFileStat_.st_size) + "\r\n\r\n");

}

std::string HttpResponse::GetFileType_() {
    std::string::size_type idx = path_.find_last_of('.');

    if (idx == std::string::npos) {
        return "text/plain";
    }
    std::string suffix = path_.substr(idx);
    if (SUFFIX_TYPE.count(suffix)) {
        return SUFFIX_TYPE.at(suffix);
    }
    return "text/plain";
}

void HttpResponse::ErrorContent(Buffer& buff, std::string message) {
    std::string body;
    std::string status;
    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";

    if (CODE_STATUS.count(code_)) {
        status = CODE_STATUS.at(code_);
    }
    else {
        status = "Bad Request";
    }

    body += std::to_string(code_) + " : " + status + "\n";
    body += "<p>" + message + "</p>";
    body += "<hr><em>TinyWebServer</em></body></html>";

    buff.Append("Content-length: " + std::to_string(body.size()) + "\r\n\r\n");
    buff.Append(body);
}

char* HttpResponse::File() {
    return mmFile_;
}

size_t HttpResponse::FileLen() const {
    return mmFileStat_.st_size;
}

void HttpResponse::UnmapFile() {
    if (mmFile_) {
        munmap(mmFile_, mmFileStat_.st_size);
        mmFile_ = nullptr;
    }
}