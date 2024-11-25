#include "cftpsclient.h"

#include "curl/curl.h"

CFTPSClient::CFTPSClient(const StHostInfo & stInfo)
{
    m_nTimeCost = 0;
    m_stParams = stInfo;
    curl_global_init(CURL_GLOBAL_DEFAULT);//curl global initialization
}

CFTPSClient::CFTPSClient(const std::string & strUserName, const std::string & strPassword, const std::string & strIP, const std::uint16_t nPort)
{
    m_nTimeCost = 0;
    m_stParams.m_strUserName = strUserName;
    m_stParams.m_nPort = nPort;
    m_stParams.m_strIp = strIP;
    m_stParams.m_strPassword = strPassword;

    curl_global_init(CURL_GLOBAL_DEFAULT);//curl global initialization
}

CFTPSClient::~CFTPSClient()
{
    curl_global_cleanup();//free curl global resource
}

CFTPSClient & CFTPSClient::setUserName(const std::string & strUserName)
{
    m_stParams.m_strUserName = strUserName;
    return *this;
}

CFTPSClient & CFTPSClient::setPassword(const std::string & strPassword)
{
    m_stParams.m_strPassword = strPassword;
    return *this;
}

CFTPSClient & CFTPSClient::setPort(const std::uint16_t nPort)
{
    m_stParams.m_nPort = nPort;
    return *this;
}

CFTPSClient & CFTPSClient::setIP(const std::string & strIP)
{
    m_stParams.m_strIp = strIP;
    return *this;
}

const StHostInfo & CFTPSClient::getParams() const
{
    return this->m_stParams;
}

//to verify the parameters valid or not, return true when parameters valid,otherwise return false
FTPSCode CFTPSClient::isParamsValid()
{
    if(m_stParams.m_strIp.empty() || m_stParams.m_strUserName.empty())
        return EN_INVALID_CONNECT_PARAMETERS;

    CURL * curl = curl_easy_init();
    if(!curl){
        return EN_RESOURCE_INIT_ERROR;
    }

    std::string strFtpUrl = "ftp://" + m_stParams.m_strIp + ":" + std::to_string(m_stParams.m_nPort);

    //setting libcurl optional
    curl_easy_setopt(curl, CURLOPT_URL, strFtpUrl.c_str());// FTP URL
    curl_easy_setopt(curl, CURLOPT_USERNAME, m_stParams.m_strUserName.c_str());//ftp usernmae
    curl_easy_setopt(curl, CURLOPT_PASSWORD, m_stParams.m_strPassword.c_str());//ftp password
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);//just for connection test
    curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, 0L);// perform a complete ftp session
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 5000);//timeout for 5 seconds

    //perform the ftp request
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);//close the ftp conncetion session

    if (res == CURLE_OK) {
        return EN_FTPS_OK;
    } else {
        m_strErrMsg = std::string(curl_easy_strerror(res));
        return EN_FTPS_UNKNOWN_ERROR;
    }
}

//get the error message of the last functionality callings
const std::string & CFTPSClient::getLastErrMsg() const
{
    return m_strErrMsg;
}

//list the filename on given directory, the filename would be put into the returned vector
std::optional<std::vector<std::string>>  CFTPSClient::lstDir(const std::string & strDirectory)
{
    return std::nullopt;
}
