/*
    请求报文解析
*/

#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <regex>
#include <errno.h>
#include <mysql/mysql.h>

#include "../buffer/buffer.h"
#include "../pool/sqlconnpool.h"
#include "../pool/sqlconnRAII.h"

class HttpRequest {
public:
    enum PARSE_STATE {
        REQUEST_LINE,
        HEADERS,
        BODY,
        FINISH
    };

    enum HTTP_CODE {
        NO_REQUEST = 0,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURSE,
        FORBIDDENT_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION
    };

    HttpRequest();

    ~HttpRequest() = default;

    /// @brief 初始化函数
    void Init();

    /// @brief 解析请求报文
    /// @param buff 请求报文
    /// @return 是否解析成功
    bool parse(Buffer& buff);

    /// @brief 获取请求报文路径
    /// @return 路径字符串
    std::string path() const;
    std::string& path();
    /// @brief 获取请求报文方法
    /// @return 方法字符串
    std::string method() const;
    /// @brief 获取请求报文HTTP版本
    /// @return 版本号
    std::string version() const;
    /// @brief 获取 POST 方法 解析出来的字符串
    /// @param key 键
    /// @return 值
    std::string GetPost(const std::string& key) const;
    std::string GetPost(const char* key) const;

    /// @brief 请求是否是 keep-alive 的
    /// @return true-yes, false-no
    bool IsKeepAlive() const;

private:
    /// @brief 解析请求行
    /// @param line 请求行字符串
    /// @return 请求行是否正确
    bool ParseRequestLine_(const std::string& line);
    /// @brief 解析首部行
    /// @param line 首部行字符串
    void ParseHeader_(const std::string& line);
    /// @brief 解析正文
    /// @param line 正文字符串 
    void ParseBody_(const std::string& line);

    /// @brief 解析路径
    void ParsePath_();
    /// @brief 解析方法为 POST 的 BODY
    void ParsePost_();
    /// @brief 解析 urlencoded 编码的键值对
    void ParseFromUrlencoded_();

    /// @brief 身份密码验证
    /// @param name 用户名
    /// @param pwd 密码
    /// @param isLogin 登陆选项(注册/登陆)
    /// @return 是否成功
    static bool UserVerify(const std::string& name, const std::string& pwd, bool isLogin);

    PARSE_STATE state_;
    std::string method_;
    std::string path_;
    std::string version_;
    std::string body_;
    std::unordered_map<std::string, std::string> header_;
    std::unordered_map<std::string, std::string> post_;

    static const std::unordered_set<std::string> DEFAULT_HTML;
    static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;
    // 十六进制转换为 十进制
    static int ConverHex(char ch);
};


#endif