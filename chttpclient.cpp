#include "chttpclient.h"

#include "cresourceinit.h"

#include <unordered_map>

enum HTPPCode{
    EN_HTTPS_OK = 0,

    EN_HTTPS_INVALID_INPUT_ARGS,
    EN_FTTPS_RESOURCE_INIT_ERROR,

    EN_HTTPS_LAST,
};

static std::unordered_map<HTPPCode, std::string> g_mpHttpsErrMsg = {
    std::pair<HTPPCode, std::string>{EN_HTTPS_OK, std::string("No Error, everything OK")},
    std::pair<HTPPCode, std::string>{EN_HTTPS_INVALID_INPUT_ARGS, std::string("Passed an invalid parameter to the method")},
    std::pair<HTPPCode, std::string>{EN_FTTPS_RESOURCE_INIT_ERROR, std::string("Resource init error for curl")},
    std::pair<HTPPCode, std::string>{EN_HTTPS_LAST, std::string("Such the enum being meaningless")}
};


CHTTPClient::CHTTPClient(const std::string & strIp, const std::uint16_t nPort) : m_pCurl(nullptr, &curl_easy_cleanup)
{
    if(!CResourceInit::init())
        this->m_strErrMsg = CResourceInit::getErrMsg();
    else
        this->m_pCurl.reset(curl_easy_init());

    this->m_strIp = strIp;
    this->m_nPort = nPort;
}

CHTTPClient::~CHTTPClient()
{
}


CHTTPClient & CHTTPClient::setIp(const std::string & strIp)
{
    this->m_strIp = strIp;
    return *this;
}

CHTTPClient & CHTTPClient::setPort(const std::uint16_t nPort)
{
    this->m_nPort = nPort;
    return *this;
}

std::optional<std::string> CHTTPClient::getResponse(const std::string & strURL)
{
    if(strURL.empty()){
        this->m_strErrMsg = g_mpHttpsErrMsg[EN_HTTPS_INVALID_INPUT_ARGS];
        return std::nullopt;
    }

    if(!m_pCurl){
        this->m_strErrMsg = g_mpHttpsErrMsg[EN_FTTPS_RESOURCE_INIT_ERROR];
        return std::nullopt;
    }

    const std::string && strDestURL = this->getIp_Port() + std::string("/") + strURL;
    std::string strResponse;

    curl_easy_setopt(this->m_pCurl.get(), CURLOPT_URL, strDestURL.c_str());
    curl_easy_setopt(this->m_pCurl.get(), CURLOPT_WRITEFUNCTION, CHTTPClient::writeCallback);
    curl_easy_setopt(this->m_pCurl.get(), CURLOPT_WRITEDATA, &strResponse);
    curl_easy_setopt(this->m_pCurl.get(), CURLOPT_TIMEOUT_MS, 3000);//timeout for 3 secs in all
    curl_easy_setopt(this->m_pCurl.get(), CURLOPT_CONNECTTIMEOUT_MS, 1000);//connection timeout for 1 sec

    CURLcode enRet = curl_easy_perform(this->m_pCurl.get());
    if(CURLE_OK == enRet){
        return strResponse;
    }else{
        this->m_strErrMsg =std::string(curl_easy_strerror(enRet));
        return std::nullopt;
    }
}

const std::string & CHTTPClient::getErrMsg() const
{
    return this->m_strErrMsg;
}


std::string CHTTPClient::getIp_Port()
{
    const std::string && strProtocol = (this->m_enMode == _EN_HTTPS_) ? std::string("https://") : std::string("http://");
    return strProtocol + this->m_strIp + std::string(":") + std::to_string(this->m_nPort);
}

//CURL库回调函数
size_t CHTTPClient::writeCallback(void * contents, size_t size, size_t nmemb, void * pUserData){
    ((std::string*)pUserData)->append((char *)contents, size * nmemb);
    return size * nmemb;
}
