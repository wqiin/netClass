#include "cftpsclient.h"

#include "curl/curl.h"

#include <mutex>
#include <sstream>

//used to ensure the curl initialization resource being called only once
static std::once_flag g_ftpsOnceFlag_init;
static std::once_flag g_ftpsOnceFlag_destory;

CFTPSClient::CFTPSClient(const StHostInfo & stInfo)
{
    m_nTimeCost = 0;
    m_stParams = stInfo;

    std::call_once(g_ftpsOnceFlag_init, [](){
        curl_global_init(CURL_GLOBAL_DEFAULT);//curl global initialization
    });
}

CFTPSClient::CFTPSClient(const std::string & strUserName, const std::string & strPassword, const std::string & strIP, const std::uint16_t nPort)
{
    m_nTimeCost = 0;
    m_stParams.m_strUserName = strUserName;
    m_stParams.m_nPort = nPort;
    m_stParams.m_strIp = strIP;
    m_stParams.m_strPassword = strPassword;

    std::call_once(g_ftpsOnceFlag_init, [](){
        curl_global_init(CURL_GLOBAL_DEFAULT);//curl global initialization
    });

    //curl_global_init(CURL_GLOBAL_DEFAULT);//curl global initialization
}

CFTPSClient::~CFTPSClient()
{
    //std::call_once(g_ftpsOnceFlag_destory, [](){
    //    curl_global_cleanup();//free curl global resource
    //});
    //curl_global_cleanup();//free curl global resource
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

//make the given remote directory, return true on success, otherwise return false
std::optional<bool> CFTPSClient::mkDir(const std::string & strRemoteDir)
{
    if(strRemoteDir.empty())
        return std::nullopt;

    std::string strURL = std::string("ftp://") + m_stParams.m_strIp + std::string(":") + std::to_string(m_stParams.m_nPort) + strRemoteDir + std::string("/");
    CURLcode enRet = CURL_LAST;
    CURL * curl = curl_easy_init();

    if(curl){
        curl_easy_setopt(curl, CURLOPT_URL, strURL.c_str());
        curl_easy_setopt(curl, CURLOPT_USERNAME, m_stParams.m_strUserName.c_str());
        curl_easy_setopt(curl, CURLOPT_PASSWORD, m_stParams.m_strPassword.c_str());
        curl_easy_setopt(curl, CURLOPT_FTP_CREATE_MISSING_DIRS, 1L);

        enRet = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }

    if(CURLE_OK == enRet){
        return true;
    }else{
        m_strErrMsg = std::string(curl_easy_strerror(enRet));
        return false;
    }

    return std::nullopt;
}

//list the filename on given directory, the filename would be put into the returned vector
std::optional<std::vector<std::string>> CFTPSClient::lstDir(const std::string & strRemoteDir)
{
    if(strRemoteDir.empty())
        return std::nullopt;

    CURLcode enRet = CURL_LAST;
    CURL * curl = curl_easy_init();
    std::string strURL = std::string("ftp://") + m_stParams.m_strIp + std::string(":") + std::to_string(m_stParams.m_nPort) + strRemoteDir + std::string("/");
    std::string strRetIInfo;

    if(curl){
        curl_easy_setopt(curl, CURLOPT_URL, strURL.c_str());
        curl_easy_setopt(curl, CURLOPT_USERNAME, m_stParams.m_strUserName.c_str());
        curl_easy_setopt(curl, CURLOPT_PASSWORD, m_stParams.m_strPassword.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CFTPSClient::readResponseDataCallback);
        // curl_easy_setopt(curl, CURLOPT_FTPLISTONLY, 1L);
        curl_easy_setopt(curl, CURLOPT_DIRLISTONLY, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &strRetIInfo);

        enRet = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }

    if(CURLE_OK == enRet){
        std::vector<std::string> vecFileName;
        std::istringstream stream(strRetIInfo);
        std::string strLine;

        while(std::getline(stream, strLine)){
            if(!strLine.empty()){
                vecFileName.push_back(strLine);
            }
        }

        return vecFileName;
    }else{
        m_strErrMsg = std::string(curl_easy_strerror(enRet));
        return std::nullopt;
    }
}

//remove the given remote directory, return true on success, otherwise return false
std::optional<bool> CFTPSClient::rmDir(const std::string & strRemoteDir)
{
    if(strRemoteDir.empty())
        return std::nullopt;

    std::string strURL = std::string("ftp://") + m_stParams.m_strIp + std::string(":") + std::to_string(m_stParams.m_nPort) + strRemoteDir + std::string("/");
    CURLcode enRet = CURL_LAST;
    CURL * curl = curl_easy_init();
    if(curl){
        curl_easy_setopt(curl, CURLOPT_URL, strURL.c_str());
        curl_easy_setopt(curl, CURLOPT_USERNAME, m_stParams.m_strUserName.c_str());
        curl_easy_setopt(curl, CURLOPT_PASSWORD, m_stParams.m_strPassword.c_str());
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "RMD");
        //curl_easy_setopt(curl, CURLOPT_QUOTE, curl_slist_append(nullptr, ("RMD " + strRemoteDir).c_str()));

        enRet = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }

    if(CURLE_OK == enRet){
        return true;
    }else{
        m_strErrMsg = std::string(curl_easy_strerror(enRet));
        return false;
    }
}


//change the working directory, return true on success, otherwise return false
std::optional<bool> CFTPSClient::cd(const std::string & strRemoteDir)
{
    if(strRemoteDir.empty())
        return std::nullopt;

    CURL * curl = curl_easy_init();
    if(!curl)
        return std::nullopt;

    std::string strURL = std::string("ftp://") + m_stParams.m_strIp + std::string(":") + std::to_string(m_stParams.m_nPort) + strRemoteDir + std::string("/");

    // 构造 CWD 命令
    std::string strCMD = "CWD " + strRemoteDir;

    // 设置 QUOTE 命令
    struct curl_slist* header_list = nullptr;
    header_list = curl_slist_append(header_list, strCMD.c_str());
    curl_easy_setopt(curl, CURLOPT_QUOTE, header_list);

    // 设置 FTP 服务器 URL 和用户凭据
    curl_easy_setopt(curl, CURLOPT_URL, strURL.c_str());
    curl_easy_setopt(curl, CURLOPT_USERNAME, m_stParams.m_strUserName.c_str());
    curl_easy_setopt(curl, CURLOPT_PASSWORD, m_stParams.m_strPassword.c_str());

    // 执行请求
    CURLcode enRet = curl_easy_perform(curl);
    curl_slist_free_all(header_list);
    curl_easy_cleanup(curl);

    if(CURLE_OK == enRet){
        return true;
    }else{
        m_strErrMsg = std::string(curl_easy_strerror(enRet));
        return false;
    }

    return std::nullopt;
}


//get the current working directory, return as a string
std::optional<std::string> CFTPSClient::pwd()
{
    return std::nullopt;
}

//renam the given remote file, return true on success, otherwise return false
std::optional<bool> CFTPSClient::renameFile(const std::string & strOldName, const std::string & newName)
{
    return std::nullopt;
}

//remove the given remote file, return true on success otherwise return false
std::optional<bool> CFTPSClient::removeFile(const std::string & strFileName)
{
    return std::nullopt;
}

//upload the given local file to the remote path, return true on success, otherwise return false
std::optional<bool> CFTPSClient::upFile(const std::string & strLocalFile, const std::string & strRemotePath)
{
    return std::nullopt;
}

//download the given remote file to the local path, return true on success, otherwise return false
std::optional<bool> CFTPSClient::downFile(const std::string & strRemoteFile, const std::string & strLocalPath)
{
    return std::nullopt;
}

//copy the given remote file into the given remote directory
std::optional<bool> CFTPSClient::copyFile(const std::string & strRemoteFile, const std::string & strRemotePath)
{
    return std::nullopt;
}

//get modification time of the given remote file, return true on success, otherwise return false
std::optional<bool> CFTPSClient::geFiletModifiedTime(const std::string & strRemoteFile)
{
    return std::nullopt;
}

//get the file size of the given remote file
std::optional<std::int64_t> CFTPSClient::getFileSize(const std::string & strFileName)
{
    return std::nullopt;
}

//get the progress of file upload or download as a percentage value
std::optional<double> CFTPSClient::CFTPSClient::getProcess() const
{
    return std::nullopt;
}

//get time cost of the last functionality calling, return a positive valid value or a negative invalid value, (unit:ms)
std::int64_t CFTPSClient::getTimeCost() const
{
    return -1;
}

//get the error message according to the error code
std::string CFTPSClient::getLastErrMsg(const FTPSCode enCode)
{
    return std::string("");
}

//callback functionality used to receive the returned info from the remote
size_t CFTPSClient::readResponseDataCallback(void * ptr, size_t size, size_t nmemb, void * stream)
{
    std::string * pStr = static_cast<std::string * >(stream);
    *pStr += std::string(static_cast<char *>(ptr), size * nmemb);
    return size * nmemb;
}

