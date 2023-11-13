#include "httprequest.h"

const std::unordered_set<std::string> HttpRequest::DEFAULT_HTML = {
    "/index", "/register", "/login", "/welcome", "/video", "/picture"
};

const std::unordered_map<std::string, int> HttpRequest::DEFAULT_HTML_TAG = {
    {"/register.html", 0}, {"/login.html", 1}
};

HttpRequest::HttpRequest() : method_(std::string()),
                             path_(std::string()),
                             version_(std::string()),
                             body_(std::string())
{
    Init();
}


void HttpRequest::Init() {
    method_ = path_ = version_ = body_ = "";
    state_ = REQUEST_LINE;
    header_.clear();
    post_.clear();
}

bool HttpRequest::IsKeepAlive() const {
    if (header_.count("Connection") > 0) {
        return header_.at("Connection") == "keep-alive" && version_ == "1.1";
    }
    return false;
}


bool HttpRequest::parse(Buffer& buff) {
    const char CRLF[] = "\r\n";
    if (buff.ReadableBytes() <= 0) {
        return false;
    }

    while (buff.ReadableBytes() && state_ != FINISH) {
        const char* lineEnd = std::search(buff.Peek(), buff.BeginWriteConst(), CRLF, CRLF + 2);

        std::string line(buff.Peek(), lineEnd);
        switch (state_) {
            case REQUEST_LINE:
                {   if (!ParseRequestLine_(line)) {
                        return false;
                    }
                    ParsePath_();
                    break;
                }
            case HEADERS:
                {   ParseHeader_(line);
                    if (buff.ReadableBytes() <= 2) {
                        state_ = FINISH;
                    }
                    break;
                }
            case BODY:
                {   ParseBody_(line);
                    break;
                }
            default:
                break;
        }
        if (lineEnd == buff.BeginWrite()) {
            buff.RetrieveAll(); /*当method为 post 时, 防止下一次解析出错*/
            break;
        }
        buff.RetrieveUntil(lineEnd + 2); // 跳过"\r\n"
    }
    // LOG_DEBUG("[%s], [%s], [%s]", method_.c_str(), path_.c_str(), version_.c_str());
    return true;
}

void HttpRequest::ParsePath_() {
    if (path_ == "/") {
        path_ = "/index2.html";
    }
    else {
        if (DEFAULT_HTML.count(path_)) {
            path_ += ".html";
        }
    }
}

bool HttpRequest::ParseRequestLine_(const std::string& line) {
    // 例子 GET path/test.html HTTP/1.1
    std::regex pattern("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    std::smatch subMatch;
    if (std::regex_match(line, subMatch, pattern)) {
        // index == 0 是整个匹配
        method_ = subMatch.str(1);
        path_ = subMatch.str(2);
        version_ = subMatch.str(3);
        state_ = HEADERS;
        return true;
    }
    // LOG_ERROR("RequestLine Error");
    return false;
}

void HttpRequest::ParseHeader_(const std::string& line) {
    // Host: www.baidu.com
    // Connection:Keep-Alive
    std::regex pattern("^([^:]*): ?(.*)$");
    std::smatch subMatch;

    if (std::regex_match(line, subMatch, pattern)) {
        header_[subMatch.str(1)] = subMatch.str(2);
    }
    else {
        // 匹配不成功时表示首部行已读完, 切换到 BODY 部分
        state_ = BODY;
    }
}

void HttpRequest::ParseBody_(const std::string& line) {
    body_ = line;
    ParsePost_();
    state_ = FINISH;
    // LOG_DEBUG("Body: %s, len: %d", body_.c_str(), body_.size());
}

void HttpRequest::ParsePost_() {
    if (method_ == "POST" && header_.at("Content-Type") == "application/x-www-form-urlencoded") {
        ParseFromUrlencoded_();
        if (DEFAULT_HTML_TAG.count(path_)) {
            // 判断是否是登陆或者注册页面
            int tag = DEFAULT_HTML_TAG.at(path_);
            // LOG_DEBUG("Tag:%d", tag);
            if (tag == 0 || tag == 1) {
                bool isLogin = (tag == 1);
                if (UserVerify(post_.at("username"), post_.at("password"), isLogin)) {
                    path_ = "/welcome.html";
                }
                else {
                    path_ = "/error.html";
                }
            }
        }
    }
}

void HttpRequest::ParseFromUrlencoded_() {
    if (body_.size() == 0) {
        return;
    }

    // 解析 urlencoded 编码的 body_
    /* urlencoded 编码规则 */
    /* 
        采用键值对方式存储, 中间用 & 分隔
        空格用 + 代替
        %后面是 16 进制数, 不能用ASCII码表示
    */
    // 例子: firstName=Mickey%26&lastName=Mouse+
    std::string key, value;
    int num = 0;
    int n = body_.size();
    int i = 0, j = 0;

    for (; i < n; ++i) {
        char ch = body_[i];
        switch (ch) {
            case '=':
            {   key = body_.substr(j, i - j);
                j = i + 1;
                break; 
            }
            case '+':
            {   body_[i] = ' ';
                break; 
            }
            case '%':
            {   num = ConverHex(body_[i + 1]) * 16 + ConverHex(body_[i + 2]);
                body_[i + 2] = num % 10 + '0';
                body_[i + 1] = num / 10 + '0';
                i += 2;
                break;
            }
            case '&':
            {   value = body_.substr(j, i - j);
                j = i + 1;
                post_[key] = value;
                // LOG_DEBUG("%s = %s", key.c_str(), value.c_str());
                break;
            }
            default:
                break;
        }
    }
    assert(j <= i);
    if (post_.count(key) == 0 && j < i) {
        value = body_.substr(j, i - j);
        post_[key] = value;
    }
}

int HttpRequest::ConverHex(char ch) {
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    return ch - '0';
}

std::string HttpRequest::path() const {
    return path_;
}

std::string& HttpRequest::path() {
    return path_;
}

std::string HttpRequest::method() const {
    return method_;
}

std::string HttpRequest::version() const {
    return version_;
}

std::string HttpRequest::GetPost(const std::string& key) const {
    if (post_.count(key)) {
        return post_.at(key);
    }
    return std::string();
}

std::string HttpRequest::GetPost(const char* key) const {
    if (post_.count(std::string(key))) {
        return post_.at(std::string(key));
    }
    return std::string();
}


bool HttpRequest::UserVerify(const std::string& name, const std::string& pwd, bool isLogin) {
    if (name == "" || pwd == "") {
        return false;
    }
    // LOG_DEBUG("Verify name:%s pwd:%s", name.c_str(), pwd.c_str());

    SqlConnRAII sql_ptr(SqlConnPool::GetInstance());

    assert(sql_ptr.HasPtr());

    bool flag = false;
    MYSQL_RES* res = nullptr;
    std::string order;

    order = "SELECT username, password FROM user WHERE username= '"; // 查询语句
    order.append(name);
    order += "'";
    // LOG_DEBUG("%s", order.c_str());
    
    if (mysql_query(sql_ptr.GetPtr(), order.c_str()) != 0) {
        return false;
    }

    res = mysql_store_result(sql_ptr.GetPtr()); // 获取结果集
    if (res == nullptr) {
        return false;
    }
    int row = mysql_num_rows(res);
    if (row == 0) {
        // 查询结果为空
        if (isLogin) {
            // LOG_DEBUG("No this user!");
            flag = false;
        }
        else {
            // 注册行为
            // LOG_DEBUG("Register!");
            order.clear();
            order = "INSERT INTO user(username, password) VALUE";
            order.append("('");
            order.append(name);
            order.append("','");
            order.append(pwd);
            order.append("')");
            // LOG_DEBUG("%s", order.c_str());
            if (mysql_query(sql_ptr.GetPtr(), order.c_str()) != 0) {
                // LOG_DEBUG("INSERT error!");
            }
            flag = true;
        }
    }
    else {
        MYSQL_ROW r = mysql_fetch_row(res); // 取出第一行记录
        // LOG_DEBUG("MYSQL ROW: %s %s", r[0], r[1]);
        std::string password(r[1]);
        if (isLogin) {
            if (pwd == password) {
                flag = true;
                // LOG_DEBUG("Login Success!");
            }
            else {
                flag = false;
                // LOG_DEBUG("pwd error!");
            }
        }
        else {
            // 注册行为
            flag = false;
            // LOG_DEBUG("user used!");
        }
    }

    mysql_free_result(res);
    // LOG_DEBUG("UserVerify Finish!");
    return flag;
}