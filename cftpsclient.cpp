#include "cftpsclient.h"

#include "curl/curl.h"

#include <sstream>
#include <cstdio>   //for tempfile()
#include <filesystem>
#include <unordered_map>

enum FTPSCode
{
    EN_FTPS_OK = 0,

    EN_FTPS_CONNECT_TIMEOUT,
    EN_FTPS_TRANSIT_TIMEOUT,
    EN_FTPS_PERMISSION_DENIED,
    EN_FTPS_INVALID_CONNECT_ARGS,

    EN_FTPS_RESOURCE_INIT_ERROR,
    EN_FTPS_INVALID_INPUT_ARGS,

    EN_FTPS_FAILED_TO_OPEN_LOCAL_FILE,
    EN_FTPS_FAILED_TO_CREATE_TMP_FILE,

    EN_FTPS_FAILED_TO_CREATE_LOCAL_DIR,

    EN_FTPS_UNKNOWN_ERROR,

    EN_FTPS_LAST,   //not using
};

//error code being mapped into the error message
static std::unordered_map<FTPSCode, std::string> g_mpFtpsErrMsg = {
    std::pair<FTPSCode, std::string>{EN_FTPS_OK, std::string("No Error, everything OK")},
    std::pair<FTPSCode, std::string>{EN_FTPS_CONNECT_TIMEOUT, std::string("Timeout when connecting the Remote")},
    std::pair<FTPSCode, std::string>{EN_FTPS_TRANSIT_TIMEOUT, std::string("timeout when file transfermation")},
    std::pair<FTPSCode, std::string>{EN_FTPS_PERMISSION_DENIED, std::string("Permission deined")},
    std::pair<FTPSCode, std::string>{EN_FTPS_INVALID_CONNECT_ARGS, std::string("Invalid remote connecting parameters")},
    std::pair<FTPSCode, std::string>{EN_FTPS_RESOURCE_INIT_ERROR, std::string("Failed to initialized curl")},
    std::pair<FTPSCode, std::string>{EN_FTPS_INVALID_INPUT_ARGS, std::string("Invalid parameters input")},
    std::pair<FTPSCode, std::string>{EN_FTPS_FAILED_TO_OPEN_LOCAL_FILE, std::string("Failed to open the given local file")},
    std::pair<FTPSCode, std::string>{EN_FTPS_UNKNOWN_ERROR, std::string("")},
    std::pair<FTPSCode, std::string>{EN_FTPS_LAST, std::string("Such the enum is meaningless")},
    std::pair<FTPSCode, std::string>{EN_FTPS_FAILED_TO_CREATE_TMP_FILE, std::string("Failed to create a temperoary file by calling tmpfile")},
    };

namespace fs = std::filesystem;

CFTPSClient::CFTPSClient(const StHostInfo & stInfo)
{
    m_nTimeCost = 0;
    m_stParams = stInfo;

    curl_global_init(CURL_GLOBAL_DEFAULT);//curl global initialization
    m_pCurl = curl_easy_init();
}

CFTPSClient::CFTPSClient(const std::string & strUserName, const std::string & strPassword, const std::string & strIP, const std::uint16_t nPort)
{
    m_nTimeCost = 0;
    m_stParams.m_strUserName = strUserName;
    m_stParams.m_nPort = nPort;
    m_stParams.m_strIp = strIP;
    m_stParams.m_strPassword = strPassword;

    curl_global_init(CURL_GLOBAL_DEFAULT);//curl global initialization
    m_pCurl = curl_easy_init();
}

CFTPSClient::~CFTPSClient()
{
    if(this->m_pCurl){
        curl_easy_cleanup(m_pCurl);
        m_pCurl = nullptr;
    }

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
std::optional<bool> CFTPSClient::isParamsValid()
{
    if(m_stParams.m_strIp.empty() || m_stParams.m_strUserName.empty()){
        this->m_strErrMsg = g_mpFtpsErrMsg[EN_FTPS_INVALID_INPUT_ARGS];
        return std::nullopt;
    }

    if(!m_pCurl){
        this->m_strErrMsg = g_mpFtpsErrMsg[EN_FTPS_RESOURCE_INIT_ERROR];
        return std::nullopt;
    }

    const std::string && strURL = this->getIp_Port();
    const std::string && strUserPwd = this->getUser_Pwd();

    //setting libcurl optional
    curl_easy_setopt(m_pCurl, CURLOPT_URL, strURL.c_str());// FTP URL
    curl_easy_setopt(m_pCurl, CURLOPT_USERPWD, strUserPwd.c_str());
    curl_easy_setopt(m_pCurl, CURLOPT_NOBODY, 1L);//just for connection test
    curl_easy_setopt(m_pCurl, CURLOPT_CONNECT_ONLY, 0L);// perform a complete ftp session
    curl_easy_setopt(m_pCurl, CURLOPT_CONNECTTIMEOUT_MS, 5000);//timeout for 5 seconds

    //perform the ftp request
    CURLcode enRet = curl_easy_perform(m_pCurl);

    if (enRet == CURLE_OK) {
        return true;
    } else {
        m_strErrMsg = std::string(curl_easy_strerror(enRet));
        return false;
    }
}


bool CFTPSClient::newSession()
{
    if(m_pCurl){
        curl_easy_cleanup(m_pCurl);
    }

    m_pCurl = curl_easy_init();
}

//get the error message according to the error code
const std::string & CFTPSClient::getErrMsg() const
{
    return this->m_strErrMsg;
}

//make the given remote directory
std::optional<bool> CFTPSClient::makeDir(const std::string & strRemoteDir)
{
    if(strRemoteDir.empty()){
        this->m_strErrMsg = g_mpFtpsErrMsg[EN_FTPS_INVALID_INPUT_ARGS];
        return std::nullopt;
    }

    if(!m_pCurl){
        this->m_strErrMsg = g_mpFtpsErrMsg[EN_FTPS_RESOURCE_INIT_ERROR];
        return std::nullopt;
    }

    const std::string && strURL = this->getIp_Port() + strRemoteDir + std::string("/");
    const std::string && strUserPwd = this->getUser_Pwd();
    CURLcode enRet = CURL_LAST;

    curl_easy_setopt(m_pCurl, CURLOPT_URL, strURL.c_str());
    curl_easy_setopt(m_pCurl, CURLOPT_USERPWD, strUserPwd.c_str());
    curl_easy_setopt(m_pCurl, CURLOPT_FTP_CREATE_MISSING_DIRS, 1L);

    enRet = curl_easy_perform(m_pCurl);

    if(CURLE_OK == enRet){
        return true;
    }else{
        m_strErrMsg = std::string(curl_easy_strerror(enRet));
        return false;
    }
}

//remove the given remote directory
std::optional<bool> CFTPSClient::removeDir(const std::string & strRemoteDir)
{
    if(strRemoteDir.empty()){
        this->m_strErrMsg = g_mpFtpsErrMsg[EN_FTPS_INVALID_INPUT_ARGS];
        return std::nullopt;
    }

    if(!m_pCurl){
        this->m_strErrMsg = g_mpFtpsErrMsg[EN_FTPS_RESOURCE_INIT_ERROR];
        return std::nullopt;
    }

    const std::string && strURL = this->getIp_Port() + strRemoteDir + std::string("/");
    const std::string && strUserPwd = this->getUser_Pwd();
    CURLcode enRet = CURL_LAST;

    curl_easy_setopt(m_pCurl, CURLOPT_URL, strURL.c_str());
    curl_easy_setopt(m_pCurl, CURLOPT_USERPWD, strUserPwd.c_str());

    struct curl_slist * pCommands = nullptr;
    const std::string && rmdCommand = std::string("RMD ") + strRemoteDir;
    pCommands = curl_slist_append(pCommands, rmdCommand.c_str());
    curl_easy_setopt(m_pCurl, CURLOPT_QUOTE, pCommands);

    enRet = curl_easy_perform(m_pCurl);
    curl_slist_free_all(pCommands);

    if(CURLE_OK == enRet){
        return true;
    }else{
        m_strErrMsg = std::string(curl_easy_strerror(enRet));
        return false;
    }
}

//list the filename on given directory, the filename would be put into the returned vector
std::optional<std::vector<std::string>> CFTPSClient::listDir(const std::string & strRemoteDir)
{
    if(strRemoteDir.empty()){
        this->m_strErrMsg = g_mpFtpsErrMsg[EN_FTPS_INVALID_INPUT_ARGS];
        return std::nullopt;
    }

    if(!m_pCurl){
        this->m_strErrMsg = g_mpFtpsErrMsg[EN_FTPS_RESOURCE_INIT_ERROR];
        return std::nullopt;
    }

    CURLcode enRet = CURL_LAST;
    const std::string && strURL = this->getIp_Port() + strRemoteDir + std::string("/");
    const std::string && strUserPwd = this->getUser_Pwd();
    std::string strRetInfo;

    curl_easy_setopt(m_pCurl, CURLOPT_URL, strURL.c_str());
    curl_easy_setopt(m_pCurl, CURLOPT_USERPWD, strUserPwd.c_str());
    curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION, CFTPSClient::readResponseDataCallback);
    curl_easy_setopt(m_pCurl, CURLOPT_DIRLISTONLY, 1L);
    curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA, &strRetInfo);

    enRet = curl_easy_perform(m_pCurl);

    if(CURLE_OK == enRet){
        std::istringstream stream(strRetInfo);
        std::string strLine;
        std::vector<std::string> vecFiles;
        vecFiles.clear();

        while(std::getline(stream, strLine)){
            if(!strLine.empty()){
                vecFiles.push_back(strLine);
            }
        }

        return vecFiles;
    }else{
        m_strErrMsg = std::string(curl_easy_strerror(enRet));
        return std::nullopt;
    }
}

//change the working directory, return true on success, otherwise return false
std::optional<bool> CFTPSClient::cd(const std::string & strRemoteDir)
{
    if(strRemoteDir.empty()){
        this->m_strErrMsg = g_mpFtpsErrMsg[EN_FTPS_INVALID_INPUT_ARGS];
        return std::nullopt;
    }

    if(!m_pCurl){
        this->m_strErrMsg = g_mpFtpsErrMsg[EN_FTPS_RESOURCE_INIT_ERROR];
        return std::nullopt;
    }

    const std::string && strURL = this->getIp_Port() + strRemoteDir + std::string("/");
    const std::string && strCMD = std::string("CWD ") + strRemoteDir;
    const std::string && strUserPwd = this->getUser_Pwd();

    curl_easy_setopt(m_pCurl, CURLOPT_URL, strURL.c_str());
    curl_easy_setopt(m_pCurl, CURLOPT_USERPWD, strUserPwd.c_str());

    struct curl_slist* pHeaderList = nullptr;
    pHeaderList = curl_slist_append(pHeaderList, strCMD.c_str());
    curl_easy_setopt(m_pCurl, CURLOPT_QUOTE, pHeaderList);

    //perform the request
    CURLcode enRet = curl_easy_perform(m_pCurl);
    curl_slist_free_all(pHeaderList);

    if(CURLE_OK == enRet){
        return true;
    }else{
        m_strErrMsg = std::string(curl_easy_strerror(enRet));
        return false;
    }
}

//get the current working directory
std::optional<std::string> CFTPSClient::pwd()
{
    if(!m_pCurl){
        this->m_strErrMsg = g_mpFtpsErrMsg[EN_FTPS_RESOURCE_INIT_ERROR];
        return std::nullopt;
    }

    CURLcode enRet = CURL_LAST;
    std::string && strURL = this->getIp_Port();
    std::string && strUserPwd = this->getUser_Pwd();
    std::string strRemoteDir;

    curl_easy_setopt(m_pCurl, CURLOPT_URL, strURL.c_str());
    curl_easy_setopt(m_pCurl, CURLOPT_USERPWD, strUserPwd.c_str());

    //setting the write callback function
    curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION, readResponseDataCallback);
    curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA, &strRemoteDir);

    struct curl_slist* pHeaderList = nullptr;
    pHeaderList = curl_slist_append(pHeaderList, "PWD");
    curl_easy_setopt(m_pCurl, CURLOPT_QUOTE, pHeaderList);

    enRet = curl_easy_perform(m_pCurl);
    curl_slist_free_all(pHeaderList);

    if(CURLE_OK == enRet){
        return strRemoteDir;
    }else{
        m_strErrMsg = std::string(curl_easy_strerror(enRet));
        return std::nullopt;
    }
}

//make a given file in the remote,  return true on success, otherwise return false
std::optional<bool> CFTPSClient::makeFile(const std::string & strRemoteFile)
{
    if(strRemoteFile.empty()){
        this->m_strErrMsg = g_mpFtpsErrMsg[EN_FTPS_INVALID_INPUT_ARGS];
        return std::nullopt;
    }

    if(!m_pCurl){
        this->m_strErrMsg = g_mpFtpsErrMsg[EN_FTPS_RESOURCE_INIT_ERROR];
        return std::nullopt;
    }

    const std::string && strURL = this->getIp_Port() + strRemoteFile;
    const std::string && strUserPwd = this->getUser_Pwd();
    CURLcode enRet = CURL_LAST;

    curl_easy_setopt(m_pCurl, CURLOPT_URL, strURL.c_str());
    curl_easy_setopt(m_pCurl, CURLOPT_USERPWD, strUserPwd.c_str());

    FILE * fpTemp = tmpfile();
    if(!fpTemp){
        m_strErrMsg = std::string("Failed to create a temp file by calling tmpfile");
        return EN_FTPS_FAILED_TO_CREATE_TMP_FILE;
    }

    curl_easy_setopt(m_pCurl, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(m_pCurl, CURLOPT_READDATA, fpTemp);

    enRet = curl_easy_perform(m_pCurl);
    fclose(fpTemp);

    if(CURLE_OK == enRet){
        return true;
    }else{
        m_strErrMsg = std::string(curl_easy_strerror(enRet));
        return false;
    }
}

//renam the given remote file, return true on success, otherwise return false
std::optional<bool> CFTPSClient::renameFile(const std::string & strOldName, const std::string & strNewName)
{
    if(strOldName.empty() || strNewName.empty()){
        this->m_strErrMsg = g_mpFtpsErrMsg[EN_FTPS_INVALID_INPUT_ARGS];
        return std::nullopt;
    }

    if(!m_pCurl){
        this->m_strErrMsg = g_mpFtpsErrMsg[EN_FTPS_RESOURCE_INIT_ERROR];
        return std::nullopt;
    }

    CURLcode enRet = CURL_LAST;
    const std::string && strURL = this->getIp_Port() + strOldName;
    const std::string && strUserPwd = this->getUser_Pwd();

    curl_easy_setopt(m_pCurl, CURLOPT_URL, strURL.c_str());
    curl_easy_setopt(m_pCurl, CURLOPT_USERPWD, strUserPwd.c_str());
    curl_easy_setopt(m_pCurl, CURLOPT_NOBODY, 1);

    struct curl_slist * pHeader_list = nullptr;
    std::string && strTempCmd = std::string("RNFR ") + strOldName;
    pHeader_list = curl_slist_append(pHeader_list, strTempCmd.c_str());

    strTempCmd = std::string("RNTO ") + strNewName;
    pHeader_list = curl_slist_append(pHeader_list, strTempCmd.c_str());

    curl_easy_setopt(m_pCurl, CURLOPT_POSTQUOTE, pHeader_list);
    enRet = curl_easy_perform(m_pCurl);
    curl_slist_free_all(pHeader_list);

    if (CURLE_OK == enRet){
        return true;
    }else{
        m_strErrMsg = std::string(curl_easy_strerror(enRet));
        return false;
    }
}

//remove the given remote file, return true on success otherwise return false
std::optional<bool> CFTPSClient::removeFile(const std::string & strRemoteFile)
{
    if(strRemoteFile.empty()){
        this->m_strErrMsg = g_mpFtpsErrMsg[EN_FTPS_INVALID_INPUT_ARGS];
        return std::nullopt;
    }

    if(!m_pCurl){
        this->m_strErrMsg = g_mpFtpsErrMsg[EN_FTPS_RESOURCE_INIT_ERROR];
        return std::nullopt;
    }

    CURLcode enRet = CURL_LAST;
    const std::string && strURL = this->getIp_Port() + strRemoteFile;
    const std::string && strUserPwd = this->getUser_Pwd();

    curl_easy_setopt(m_pCurl, CURLOPT_URL, strURL.c_str());
    curl_easy_setopt(m_pCurl, CURLOPT_USERPWD, strUserPwd.c_str());
    curl_easy_setopt(m_pCurl, CURLOPT_NOBODY, 1);
    curl_easy_setopt(m_pCurl, CURLOPT_CUSTOMREQUEST, "DELE");

    enRet = curl_easy_perform(m_pCurl);

    if(CURLE_OK == enRet){
        return true;
    }else{
        m_strErrMsg = std::string(curl_easy_strerror(enRet));
        return false;
    }
}

//upload the given local file to the remote path, return true on success, otherwise return false
std::optional<bool> CFTPSClient::upFile(const std::string & strLocalFile, const std::string & strRemotePath)
{
    if(strLocalFile.empty() || strRemotePath.empty()){
        this->m_strErrMsg = g_mpFtpsErrMsg[EN_FTPS_INVALID_INPUT_ARGS];
        return std::nullopt;
    }

    if(!m_pCurl){
        this->m_strErrMsg = g_mpFtpsErrMsg[EN_FTPS_RESOURCE_INIT_ERROR];
        return std::nullopt;
    }

    FILE * fp = fopen(strLocalFile.c_str(), "rb");
    if (!fp){
        this->m_strErrMsg = g_mpFtpsErrMsg[EN_FTPS_FAILED_TO_OPEN_LOCAL_FILE];
        return std::nullopt;
    }

    fseek(fp, 0L, SEEK_END);
    size_t nFileSize = ftell(fp);//get the size of the file to upload
    fseek(fp, 0L, SEEK_SET);

    const std::string && strURL = this->getIp_Port() + strRemotePath;
    const std::string && strUserPwd = this->getUser_Pwd();
    CURLcode enRet = CURL_LAST;

    curl_easy_setopt(m_pCurl, CURLOPT_URL, strURL.c_str());
    curl_easy_setopt(m_pCurl, CURLOPT_USERPWD, strUserPwd.c_str());
    curl_easy_setopt(m_pCurl, CURLOPT_READFUNCTION, CFTPSClient::upReadCallback);
    curl_easy_setopt(m_pCurl, CURLOPT_READDATA, fp);
    curl_easy_setopt(m_pCurl, CURLOPT_FTP_CREATE_MISSING_DIRS, 1);
    curl_easy_setopt(m_pCurl, CURLOPT_UPLOAD, 1);
    curl_easy_setopt(m_pCurl, CURLOPT_INFILESIZE, nFileSize);

    enRet = curl_easy_perform(m_pCurl);
    fclose(fp);

    if (CURLE_OK == enRet){
        return true;
    }else{
        m_strErrMsg = std::string(curl_easy_strerror(enRet));
        return false;
    }
}

//download the given remote file to the local file, return true on success, otherwise return false
std::optional<bool> CFTPSClient::downFile(const std::string & strRemoteFile, const std::string & strLocalFile)
{
    if(strRemoteFile.empty() || strLocalFile.empty()){
        this->m_strErrMsg = g_mpFtpsErrMsg[EN_FTPS_INVALID_INPUT_ARGS];
        return std::nullopt;
    }

    if(!m_pCurl){
        this->m_strErrMsg = g_mpFtpsErrMsg[EN_FTPS_RESOURCE_INIT_ERROR];
        return std::nullopt;
    }

    //create the local file path if not existing
    if(!createDirectory(strLocalFile)){
        this->m_strErrMsg = g_mpFtpsErrMsg[EN_FTPS_FAILED_TO_CREATE_LOCAL_DIR];
        return std::nullopt;
    }

    FILE * fp = fopen(strLocalFile.c_str(), "wb+");
    if(!fp){
        this->m_strErrMsg = g_mpFtpsErrMsg[EN_FTPS_FAILED_TO_OPEN_LOCAL_FILE];
        return std::nullopt;
    }

    const std::string && strURL = this->getIp_Port() + strRemoteFile;
    const std::string && strUserPwd = this->getUser_Pwd();
    CURLcode enRet = CURL_LAST;

    curl_easy_setopt(m_pCurl, CURLOPT_URL, strURL.c_str());
    curl_easy_setopt(m_pCurl, CURLOPT_USERPWD, strUserPwd.c_str());
    curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION, CFTPSClient::downWriteCallback);
    curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA, fp);

    enRet = curl_easy_perform(m_pCurl);
    fclose(fp);

    if (CURLE_OK == enRet){
        return true;
    }else{
        m_strErrMsg = std::string(curl_easy_strerror(enRet));
        return false;
    }
}

//copy the given remote file into the given remote directory
std::optional<bool> CFTPSClient::copyFile(const std::string & strRemoteSourceFile, const std::string & strRemoteDestFile)
{
    //usually way to do:
    //first, download the given remote file
    //second, uplaod the file download to the given remote path
    //third, delete the local temp file

    //some ftp sever support SITE COPY command which allows copy the source to the destination in the remote
    if(strRemoteSourceFile.empty() || strRemoteDestFile.empty()){
        this->m_strErrMsg = g_mpFtpsErrMsg[EN_FTPS_INVALID_INPUT_ARGS];
        return std::nullopt;
    }

    if(!m_pCurl){
        this->m_strErrMsg = g_mpFtpsErrMsg[EN_FTPS_RESOURCE_INIT_ERROR];
        return std::nullopt;
    }

    CURLcode enRet = CURL_LAST;
    const std::string && strURL = this->getIp_Port() + strRemoteSourceFile;
    const std::string && strUserPwd = this->getUser_Pwd();
    const std::string strCmd = std::string("SITE COPY ") + strRemoteSourceFile + std::string(" ") + strRemoteDestFile;

    curl_easy_setopt(m_pCurl, CURLOPT_URL, strURL.c_str());
    curl_easy_setopt(m_pCurl, CURLOPT_USERPWD, strUserPwd.c_str());

    struct curl_slist * pHeaderList = nullptr;
    pHeaderList = curl_slist_append(pHeaderList, strCmd.c_str());
    curl_easy_setopt(m_pCurl, CURLOPT_QUOTE, pHeaderList);

    enRet = curl_easy_perform(m_pCurl);
    curl_slist_free_all(pHeaderList);

    if (CURLE_OK == enRet){
        return true;
    }else{
        m_strErrMsg = std::string(curl_easy_strerror(enRet));
        return false;
    }
}

//get modification time of the given remote file, return true on success, otherwise return false
std::optional<bool> CFTPSClient::geFiletModifiedTime(const std::string & strRemoteFile)
{
    return std::nullopt;
}

//get the file size of the given remote file
std::optional<std::int64_t> CFTPSClient::getFileSize(const std::string & strRemoteFile)
{
    if(strRemoteFile.empty()){
        this->m_strErrMsg = g_mpFtpsErrMsg[EN_FTPS_INVALID_INPUT_ARGS];
        return std::nullopt;
    }

    if(!m_pCurl){
        this->m_strErrMsg = g_mpFtpsErrMsg[EN_FTPS_RESOURCE_INIT_ERROR];
        return std::nullopt;
    }

    CURLcode enRet = CURL_LAST;
    const std::string && strURL = this->getIp_Port() + strRemoteFile;
    const std::string && strUserPwd = this->getUser_Pwd();

    curl_easy_setopt(m_pCurl, CURLOPT_URL, strURL.c_str());
    curl_easy_setopt(m_pCurl, CURLOPT_USERPWD, strUserPwd.c_str());
    curl_easy_setopt(m_pCurl, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION, CFTPSClient::throw_away);
    enRet = curl_easy_perform(m_pCurl);

    if (CURLE_OK == enRet){
        double fFileSize = 0.0f;
        enRet = curl_easy_getinfo(m_pCurl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &fFileSize);
        if(CURLE_OK == enRet){
            std::int64_t nFileSize = (std::int64_t)fFileSize;
            return nFileSize;
        }else{
            m_strErrMsg = std::string(curl_easy_strerror(enRet));
            return std::nullopt;
        }
    }else{
        m_strErrMsg = std::string(curl_easy_strerror(enRet));
        return std::nullopt;
    }
}

//get the progress of file upload or download as a percentage value
std::optional<double> CFTPSClient::CFTPSClient::getProcess()
{
    if(!m_pCurl){
        m_strErrMsg = std::string("Failed to initialize network resource library");
        return std::nullopt;
    }

    double fProgressRet = 0.0;

    // 设置进度回调函数
    curl_easy_setopt(m_pCurl, CURLOPT_PROGRESSFUNCTION, progressCallback);
    curl_easy_setopt(m_pCurl, CURLOPT_PROGRESSDATA, &fProgressRet);  // 这里可以传递上下文数据
    return fProgressRet;
}

//get time cost of the last functionality calling, return a positive valid value or a negative invalid value, (unit:ms)
std::int64_t CFTPSClient::getTimeCost() const
{
    return -1;
}


std::string CFTPSClient::getIp_Port()
{
    return std::string("ftp://") + m_stParams.m_strIp + std::string(":") + std::to_string(m_stParams.m_nPort);
}

std::string CFTPSClient::getUser_Pwd()
{
    return m_stParams.m_strUserName + std::string(":") + m_stParams.m_strPassword;
}

 /************************************************************
 ************************callback functionality***************
 ************************************************************/
//upload read callback, being used to read the local file to upload
size_t CFTPSClient::upReadCallback(void * ptr, size_t size, size_t nmemb, void * stream)
{
    FILE * fp = static_cast<FILE*>(stream);
    if(NULL == fp)
        return 0;
    else
        return (curl_off_t)fread(ptr, size, nmemb, fp);
}

//download write callback, being used to write the received data into the destination local file
size_t CFTPSClient::downWriteCallback(void * ptr, size_t size, size_t nmemb, void * stream)
{
    FILE * fp = static_cast<FILE *>(stream);

    if(NULL == fp)
        return 0;
    else
        return fwrite(ptr, size, nmemb, fp);
}

//callback functionality used to receive the returned info from the remote
size_t CFTPSClient::readResponseDataCallback(void * ptr, size_t size, size_t nmemb, void * stream)
{
    std::string * pStr = static_cast<std::string * >(stream);
    *pStr += std::string(static_cast<char *>(ptr), size * nmemb);
    return size * nmemb;
}

//ignore the data from the remote, do nothings
size_t CFTPSClient::throw_away(void * ptr, size_t size, size_t nmemb, void * data)
{
    (void)ptr;(void)data;
    return (size_t)(size * nmemb);
}

//the get the progress of the file transportation
int CFTPSClient::progressCallback(void* pUserData, double dltotal, double dlnow, double uptotal, double upnow)
{
    double * pProgress = static_cast<double*>(pUserData);

    //download progress
    if (dltotal > 0 && pProgress) {
        *pProgress = (dlnow / dltotal) * 100;
    }

    //upload progress
    if (uptotal > 0 && pProgress) {
        *pProgress = (upnow / uptotal) * 100;
    }

    return 0;  // 0 meaning the continuity of transportantion
}

//craete the missing directory of the given file
bool CFTPSClient::createDirectory(const std::string & strFile)
{
    fs::path parentDir = fs::path(strFile).parent_path();
    if(!fs::exists(parentDir)){
        return fs::create_directories(parentDir);
    }

    return true;
}

