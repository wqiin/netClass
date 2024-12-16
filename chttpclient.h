#ifndef CHTTPCLIENT_H
#define CHTTPCLIENT_H

#include "curl/curl.h"

#include <string>
#include <optional>
#include <memory>

enum HTTPMode{
    _EN_HTTP_ = 0,
    _EN_HTTPS_,

    //Do NOT Use the below
    _EN_INVALID_HTTP_MODE_LAST_,
};

class CHTTPClient
{
private:
    std::string m_strErrMsg = "";//error message of the last operation
    std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> m_pCurl;
    HTTPMode m_enMode = HTTPMode::_EN_HTTP_;

    //the remote host information
    std::string m_strIp = "";
    std::uint16_t m_nPort = 80;

public:
    CHTTPClient(const std::string & strIp, const std::uint16_t nPort = 80, const HTTPMode enMode = _EN_HTTP_);
    ~CHTTPClient() = default;

    //copy constructor and assignment operator prohibited
    CHTTPClient(const CHTTPClient & ) = delete;
    CHTTPClient(const CHTTPClient && ) = delete;
    CHTTPClient & operator=(const CHTTPClient &) = delete;
    CHTTPClient & operator=(const CHTTPClient &&) = delete;

    CHTTPClient & setIp(const std::string & strIp);
    CHTTPClient & setPort(const std::uint16_t nPort);


    std::optional<std::string> getResponse(const std::string & strURL);

    const std::string & getErrMsg() const;

private:
    std::string getIp_Port();

private:
    static size_t readRespCallback(void * contents, size_t size, size_t nmemb, void * pUserData);
};

#endif // CHTTPCLIENT_H
