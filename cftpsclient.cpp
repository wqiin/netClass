#include "cftpsclient.h"

#include <sstream>
#include <cstdio>   //for tempfile()
#include <filesystem>
#include <unordered_map>
#include <cctype>
#include <iostream>
#include <sstream> // for std::ostringstream
#include <iomanip> // for std::put_time
#include <ctime>   // for std::tm
#include <thread>
#include <utility>

namespace fs = std::filesystem;
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

    EN_FTPS_LAST,   //not using
};

//error code being mapped into the error message
static std::unordered_map<FTPSCode, std::string> g_mpFtpsErrMsg = {
    std::make_pair(EN_FTPS_OK, std::string("No Error, everything OK")),
    std::make_pair(EN_FTPS_CONNECT_TIMEOUT, std::string("Timeout when connecting the Remote")),
    std::make_pair(EN_FTPS_TRANSIT_TIMEOUT, std::string("timeout when file transfermation")),
    std::make_pair(EN_FTPS_PERMISSION_DENIED, std::string("Permission deined")),
    std::make_pair(EN_FTPS_INVALID_CONNECT_ARGS, std::string("Invalid remote connecting parameters")),
    std::make_pair(EN_FTPS_RESOURCE_INIT_ERROR, std::string("Failed to initialized curl")),
    std::make_pair(EN_FTPS_INVALID_INPUT_ARGS, std::string("Invalid parameters input")),
    std::make_pair(EN_FTPS_FAILED_TO_OPEN_LOCAL_FILE, std::string("Failed to open the given local file")),
    std::make_pair(EN_FTPS_LAST, std::string("Such the enum is meaningless")),
    std::make_pair(EN_FTPS_FAILED_TO_CREATE_TMP_FILE, std::string("Failed to create a temperoary file by calling tmpfile")),
};



CFTPSClient::CFTPSClient(const StHostInfo & stInfo, FTPMode enMode)
{
    this->m_nTimeCost = 0;
    this->m_stParams = stInfo;
    this->m_enMode = enMode;

    curl_global_init(CURL_GLOBAL_DEFAULT);//curl global initialization
    this->m_pCurl = curl_easy_init();
}

CFTPSClient::~CFTPSClient()
{
    if(this->m_pCurl){
        curl_easy_cleanup(this->m_pCurl);
        this->m_pCurl = nullptr;
    }

    curl_global_cleanup();//free curl global resource
}

CFTPSClient & CFTPSClient::setUserName(const std::string & strUserName)
{
    this->m_stParams.m_strUserName = strUserName;
    return *this;
}

CFTPSClient & CFTPSClient::setPassword(const std::string & strPassword)
{
    this->m_stParams.m_strPassword = strPassword;
    return *this;
}

CFTPSClient & CFTPSClient::setPort(const std::uint16_t nPort)
{
    this->m_stParams.m_nPort = nPort;
    return *this;
}

CFTPSClient & CFTPSClient::setIP(const std::string & strIP)
{
    this->m_stParams.m_strIp = strIP;
    return *this;
}

CFTPSClient & CFTPSClient::setMode(const FTPMode enMode)
{
    this->m_enMode = enMode;
    return *this;
}

const StHostInfo & CFTPSClient::getParams() const
{
    return this->m_stParams;
}

CURL * CFTPSClient::getHandle()
{
    return this->m_pCurl;
}

//to verify the parameters valid or not, return true when parameters valid,otherwise return false
std::optional<bool> CFTPSClient::isParamsValid()
{
    if(m_stParams.m_strIp.empty() || m_stParams.m_strUserName.empty()){
        this->m_strErrMsg = g_mpFtpsErrMsg[EN_FTPS_INVALID_INPUT_ARGS];
        return std::nullopt;
    }

    if(!this->m_pCurl){
        this->m_strErrMsg = g_mpFtpsErrMsg[EN_FTPS_RESOURCE_INIT_ERROR];
        return std::nullopt;
    }

    const std::string && strURL = this->getIp_Port();
    const std::string && strUserPwd = this->getUser_Pwd();

    //setting libcurl optional
    curl_easy_setopt(this->m_pCurl, CURLOPT_URL, strURL.c_str());// FTP URL
    curl_easy_setopt(this->m_pCurl, CURLOPT_USERPWD, strUserPwd.c_str());
    curl_easy_setopt(this->m_pCurl, CURLOPT_NOBODY, 1L);//just for connection test
    curl_easy_setopt(this->m_pCurl, CURLOPT_CONNECT_ONLY, 0L);// perform a complete ftp session
    curl_easy_setopt(this->m_pCurl, CURLOPT_CONNECTTIMEOUT_MS, 5000);//timeout for 5 seconds
    curl_easy_setopt(this->m_pCurl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(this->m_pCurl, CURLOPT_SSL_VERIFYHOST, 0L);

    //perform the ftp request
    CURLcode enRet = curl_easy_perform(this->m_pCurl);
    if (enRet == CURLE_OK) {
        return true;
    } else {
        this->m_strErrMsg = std::string(curl_easy_strerror(enRet));
        return false;
    }
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

    if(!this->m_pCurl){
        this->m_strErrMsg = g_mpFtpsErrMsg[EN_FTPS_RESOURCE_INIT_ERROR];
        return std::nullopt;
    }

    const std::string && strURL = this->getIp_Port() + strRemoteDir + std::string("/");
    const std::string && strUserPwd = this->getUser_Pwd();

    curl_easy_setopt(this->m_pCurl, CURLOPT_URL, strURL.c_str());
    curl_easy_setopt(this->m_pCurl, CURLOPT_USERPWD, strUserPwd.c_str());
    curl_easy_setopt(this->m_pCurl, CURLOPT_FTP_CREATE_MISSING_DIRS, 1L);
    curl_easy_setopt(this->m_pCurl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(this->m_pCurl, CURLOPT_SSL_VERIFYHOST, 0L);

    CURLcode enRet = curl_easy_perform(this->m_pCurl);
    if(CURLE_OK == enRet){
        return true;
    }else{
        this->m_strErrMsg = std::string(curl_easy_strerror(enRet));
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

    if(!this->m_pCurl){
        this->m_strErrMsg = g_mpFtpsErrMsg[EN_FTPS_RESOURCE_INIT_ERROR];
        return std::nullopt;
    }

    const std::string && strURL = this->getIp_Port() + strRemoteDir + std::string("/");
    const std::string && strUserPwd = this->getUser_Pwd();

    curl_easy_setopt(this->m_pCurl, CURLOPT_URL, strURL.c_str());
    curl_easy_setopt(this->m_pCurl, CURLOPT_USERPWD, strUserPwd.c_str());
    curl_easy_setopt(this->m_pCurl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(this->m_pCurl, CURLOPT_SSL_VERIFYHOST, 0L);

    struct curl_slist * pCommands = nullptr;
    const std::string && strCmd = std::string("RMD ") + strRemoteDir;
    pCommands = curl_slist_append(pCommands, strCmd.c_str());
    curl_easy_setopt(this->m_pCurl, CURLOPT_QUOTE, pCommands);

    CURLcode enRet = curl_easy_perform(this->m_pCurl);
    curl_slist_free_all(pCommands);
    if(CURLE_OK == enRet || CURLE_REMOTE_ACCESS_DENIED == enRet){
        return true;
    }else{
        this->m_strErrMsg = std::string(curl_easy_strerror(enRet));
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

    if(!this->m_pCurl){
        this->m_strErrMsg = g_mpFtpsErrMsg[EN_FTPS_RESOURCE_INIT_ERROR];
        return std::nullopt;
    }

    const std::string && strURL = this->getIp_Port() + strRemoteDir + std::string("/");
    const std::string && strUserPwd = this->getUser_Pwd();
    std::string strRetInfo;

    curl_easy_setopt(this->m_pCurl, CURLOPT_URL, strURL.c_str());
    curl_easy_setopt(this->m_pCurl, CURLOPT_USERPWD, strUserPwd.c_str());
    curl_easy_setopt(this->m_pCurl, CURLOPT_WRITEFUNCTION, CFTPSClient::readResponseDataCallback);
    curl_easy_setopt(this->m_pCurl, CURLOPT_DIRLISTONLY, 1L);
    curl_easy_setopt(this->m_pCurl, CURLOPT_WRITEDATA, &strRetInfo);
    curl_easy_setopt(this->m_pCurl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(this->m_pCurl, CURLOPT_SSL_VERIFYHOST, 0L);

    CURLcode enRet = curl_easy_perform(this->m_pCurl);
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
        this->m_strErrMsg = std::string(curl_easy_strerror(enRet));
        return std::nullopt;
    }
}

//list the filename in detail on given directory, the filename would be put into the returned vector
std::optional<std::vector<StFile>> CFTPSClient::listDir_detailed(const std::string & strRemoteDir)
{
    if(!this->m_pCurl){
        this->m_strErrMsg = g_mpFtpsErrMsg[EN_FTPS_RESOURCE_INIT_ERROR];
        return std::nullopt;
    }

    const std::string && strURL = this->getIp_Port() + strRemoteDir + std::string("/");
    const std::string && strUserPwd = this->getUser_Pwd();
    const std::string && strCmd = std::string("LIST ") + strRemoteDir;
    std::string strRemoteDirInfo;

    curl_easy_setopt(this->m_pCurl, CURLOPT_URL, strURL.c_str());
    curl_easy_setopt(this->m_pCurl, CURLOPT_USERPWD, strUserPwd.c_str());
    curl_easy_setopt(this->m_pCurl, CURLOPT_CUSTOMREQUEST, strCmd.c_str());
    curl_easy_setopt(this->m_pCurl, CURLOPT_WRITEFUNCTION, readResponseDataCallback);
    curl_easy_setopt(this->m_pCurl, CURLOPT_WRITEDATA, &strRemoteDirInfo);
    curl_easy_setopt(this->m_pCurl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(this->m_pCurl, CURLOPT_SSL_VERIFYHOST, 0L);

    CURLcode enRet = curl_easy_perform(this->m_pCurl);
    if(CURLE_OK == enRet){
        std::vector<StFile> vecFileInfo;
        std::istringstream stream(strRemoteDirInfo);
        std::string line;

        while (std::getline(stream, line)) {
            std::istringstream lineStream(line);
            StFile stInfo;

            //parsing a returned line
            lineStream >> stInfo.strPermission
                    >> stInfo.nLinkCount
                    >> stInfo.strOwner
                    >> stInfo.strGroup
                    >> stInfo.nSize
                    >> stInfo.strMonth
                    >> stInfo.nDay
                    >> stInfo.strTime;

            //parsing the file name
            std::getline(lineStream, stInfo.strFileName);
            if (!stInfo.strFileName.empty() && stInfo.strFileName[0] == ' ')
                stInfo.strFileName = stInfo.strFileName.substr(1); //trimming off the space

            vecFileInfo.push_back(stInfo);
        }

        return vecFileInfo;
    }else{
        this->m_strErrMsg = std::string(curl_easy_strerror(enRet));
        return std::nullopt;
    }

    return std::nullopt;
}

//change the working directory, return true on success, otherwise return false
std::optional<bool> CFTPSClient::cd(const std::string & strRemoteDir)
{
    if(strRemoteDir.empty()){
        this->m_strErrMsg = g_mpFtpsErrMsg[EN_FTPS_INVALID_INPUT_ARGS];
        return std::nullopt;
    }

    if(!this->m_pCurl){
        this->m_strErrMsg = g_mpFtpsErrMsg[EN_FTPS_RESOURCE_INIT_ERROR];
        return std::nullopt;
    }

    const std::string && strURL = this->getIp_Port() + strRemoteDir + std::string("/");
    const std::string && strCMD = std::string("CWD ") + strRemoteDir;
    const std::string && strUserPwd = this->getUser_Pwd();

    curl_easy_setopt(this->m_pCurl, CURLOPT_URL, strURL.c_str());
    curl_easy_setopt(this->m_pCurl, CURLOPT_USERPWD, strUserPwd.c_str());
    curl_easy_setopt(this->m_pCurl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(this->m_pCurl, CURLOPT_SSL_VERIFYHOST, 0L);

    struct curl_slist* pHeaderList = nullptr;
    pHeaderList = curl_slist_append(pHeaderList, strCMD.c_str());
    curl_easy_setopt(this->m_pCurl, CURLOPT_QUOTE, pHeaderList);

    //perform the request
    CURLcode enRet = curl_easy_perform(this->m_pCurl);
    curl_slist_free_all(pHeaderList);
    if(CURLE_OK == enRet){
        return true;
    }else{
        this->m_strErrMsg = std::string(curl_easy_strerror(enRet));
        return false;
    }
}

//get the current working directory
std::optional<std::string> CFTPSClient::pwd()
{
    if(!this->m_pCurl){
        this->m_strErrMsg = g_mpFtpsErrMsg[EN_FTPS_RESOURCE_INIT_ERROR];
        return std::nullopt;
    }

    const std::string && strURL = this->getIp_Port();
    const std::string && strUserPwd = this->getUser_Pwd();
    std::string strRetHeaderInfo;

    curl_easy_setopt(this->m_pCurl, CURLOPT_URL, strURL.c_str());
    curl_easy_setopt(this->m_pCurl, CURLOPT_USERPWD, strUserPwd.c_str());
    curl_easy_setopt(this->m_pCurl, CURLOPT_HEADERFUNCTION, readResponseDataCallback);
    curl_easy_setopt(this->m_pCurl, CURLOPT_HEADERDATA, &strRetHeaderInfo);
    curl_easy_setopt(this->m_pCurl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(this->m_pCurl, CURLOPT_SSL_VERIFYHOST, 0L);

    struct curl_slist *pHeaderList = nullptr;
    pHeaderList = curl_slist_append(pHeaderList, "PWD");
    curl_easy_setopt(this->m_pCurl, CURLOPT_QUOTE, pHeaderList);

    CURLcode enRet = curl_easy_perform(this->m_pCurl);
    curl_slist_free_all(pHeaderList);
    if(CURLE_OK == enRet){
        const std::string strStartTag = "257 \"";
        const std::string strEndTag = "\"";
        std::string strWorkingDir;
        size_t nStartPos = strRetHeaderInfo.find(strStartTag);
        if (nStartPos != std::string::npos) {
            size_t nEndPos = strRetHeaderInfo.find(strEndTag, nStartPos + strStartTag.length());//find the first char '"'
            if (nEndPos != std::string::npos) {
                strWorkingDir = strRetHeaderInfo.substr(nStartPos + strStartTag.length(), nEndPos - nStartPos - strStartTag.length());
            }
        }

        return strWorkingDir;
    }else{
        this->m_strErrMsg = std::string(curl_easy_strerror(enRet));
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

    if(!this->m_pCurl){
        this->m_strErrMsg = g_mpFtpsErrMsg[EN_FTPS_RESOURCE_INIT_ERROR];
        return std::nullopt;
    }

    //when remote parent directory NOT existing, make it
    const std::string && strParentDir = fs::path(strRemoteFile).parent_path().string();
    auto bRet = this->makeDir(strParentDir);
    if(!bRet.has_value() || true != *bRet)
        return std::nullopt;

    const std::string && strURL = this->getIp_Port() + strRemoteFile;
    const std::string && strUserPwd = this->getUser_Pwd();

    curl_easy_setopt(this->m_pCurl, CURLOPT_URL, strURL.c_str());
    curl_easy_setopt(this->m_pCurl, CURLOPT_USERPWD, strUserPwd.c_str());

    FILE * fpTemp = tmpfile();
    if(!fpTemp){
        m_strErrMsg = std::string("Failed to create a temp file by calling tmpfile");
        return EN_FTPS_FAILED_TO_CREATE_TMP_FILE;
    }

    curl_easy_setopt(this->m_pCurl, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(this->m_pCurl, CURLOPT_READDATA, fpTemp);
    curl_easy_setopt(this->m_pCurl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(this->m_pCurl, CURLOPT_SSL_VERIFYHOST, 0L);

    CURLcode enRet = curl_easy_perform(this->m_pCurl);
    fclose(fpTemp);
    if(CURLE_OK == enRet){
        return true;
    }else{
        this->m_strErrMsg = std::string(curl_easy_strerror(enRet));
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

    if(!this->m_pCurl){
        this->m_strErrMsg = g_mpFtpsErrMsg[EN_FTPS_RESOURCE_INIT_ERROR];
        return std::nullopt;
    }

    const std::string && strURL = this->getIp_Port() + strOldName;
    const std::string && strUserPwd = this->getUser_Pwd();

    curl_easy_setopt(this->m_pCurl, CURLOPT_URL, strURL.c_str());
    curl_easy_setopt(this->m_pCurl, CURLOPT_USERPWD, strUserPwd.c_str());
    curl_easy_setopt(this->m_pCurl, CURLOPT_NOBODY, 1);
    curl_easy_setopt(this->m_pCurl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(this->m_pCurl, CURLOPT_SSL_VERIFYHOST, 0L);

    struct curl_slist * pHeader_list = nullptr;
    std::string && strTempCmd = std::string("RNFR ") + strOldName;
    pHeader_list = curl_slist_append(pHeader_list, strTempCmd.c_str());

    strTempCmd = std::string("RNTO ") + strNewName;
    pHeader_list = curl_slist_append(pHeader_list, strTempCmd.c_str());
    curl_easy_setopt(this->m_pCurl, CURLOPT_POSTQUOTE, pHeader_list);

    CURLcode enRet = curl_easy_perform(this->m_pCurl);
    curl_slist_free_all(pHeader_list);
    if (CURLE_OK == enRet){
        return true;
    }else{
        this->m_strErrMsg = std::string(curl_easy_strerror(enRet));
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

    if(!this->m_pCurl){
        this->m_strErrMsg = g_mpFtpsErrMsg[EN_FTPS_RESOURCE_INIT_ERROR];
        return std::nullopt;
    }

    const std::string && strURL = this->getIp_Port() + strRemoteFile;
    const std::string && strUserPwd = this->getUser_Pwd();

    curl_easy_setopt(this->m_pCurl, CURLOPT_URL, strURL.c_str());
    curl_easy_setopt(this->m_pCurl, CURLOPT_USERPWD, strUserPwd.c_str());
    curl_easy_setopt(this->m_pCurl, CURLOPT_NOBODY, 1);
    curl_easy_setopt(this->m_pCurl, CURLOPT_CUSTOMREQUEST, "DELE");
    curl_easy_setopt(this->m_pCurl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(this->m_pCurl, CURLOPT_SSL_VERIFYHOST, 0L);

    CURLcode enRet = curl_easy_perform(this->m_pCurl);
    if(CURLE_OK == enRet){
        return true;
    }else{
        this->m_strErrMsg = std::string(curl_easy_strerror(enRet));
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

    if(!this->m_pCurl){
        this->m_strErrMsg = g_mpFtpsErrMsg[EN_FTPS_RESOURCE_INIT_ERROR];
        return std::nullopt;
    }

    FILE * fp = fopen(strLocalFile.c_str(), "rb");
    if (!fp){
        this->m_strErrMsg = g_mpFtpsErrMsg[EN_FTPS_FAILED_TO_OPEN_LOCAL_FILE];
        return std::nullopt;
    }

    //when the remote file passed as a directory, set the upload filename as the local filename
    std::string strRemotePathTemp = strRemotePath;
    if('/' == strRemotePathTemp.back()){
        fs::path fpFileName = fs::path(strLocalFile);
        strRemotePathTemp += fpFileName.filename().string();
    }

    //get the size of the file to upload
    fseek(fp, 0L, SEEK_END);
    size_t nFileSize = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    this->m_fProgress = 0.0f;
    const std::string && strURL = this->getIp_Port() + strRemotePathTemp;
    const std::string && strUserPwd = this->getUser_Pwd();

    curl_easy_setopt(this->m_pCurl, CURLOPT_URL, strURL.c_str());
    curl_easy_setopt(this->m_pCurl, CURLOPT_USERPWD, strUserPwd.c_str());
    curl_easy_setopt(this->m_pCurl, CURLOPT_READFUNCTION, CFTPSClient::upReadCallback);
    curl_easy_setopt(this->m_pCurl, CURLOPT_READDATA, fp);
    curl_easy_setopt(this->m_pCurl, CURLOPT_FTP_CREATE_MISSING_DIRS, 1);
    curl_easy_setopt(this->m_pCurl, CURLOPT_UPLOAD, 1);
    curl_easy_setopt(this->m_pCurl, CURLOPT_INFILESIZE, nFileSize);
    curl_easy_setopt(this->m_pCurl, CURLOPT_PROGRESSFUNCTION, CFTPSClient::progressCallback);
    curl_easy_setopt(this->m_pCurl, CURLOPT_PROGRESSDATA, &this->m_fProgress);  // passing user data into the callback function
    curl_easy_setopt(this->m_pCurl, CURLOPT_NOPROGRESS, 0L); // enable progress callback
    curl_easy_setopt(this->m_pCurl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(this->m_pCurl, CURLOPT_SSL_VERIFYHOST, 0L);

    CURLcode enRet = curl_easy_perform(this->m_pCurl);
    fclose(fp);
    if (CURLE_OK == enRet){
        return true;
    }else{
        this->m_strErrMsg = std::string(curl_easy_strerror(enRet));
        return false;
    }
}

//async file upload, return true on success, otherwise return false and relative error message
std::future<std::optional<std::pair<bool, std::string>>> CFTPSClient::upFile_async(const std::string & strLocalFile, const std::string & strRemotePath)
{
    std::promise<std::optional<std::pair<bool, std::string>>> promise;
    std::future<std::optional<std::pair<bool, std::string>>> future = promise.get_future();

    //async file upload, note strLocalFile and strRemotePath can NOT captured by reference
    auto async_upFile = [this, strLocalFile, strRemotePath, promiseUpload = std::move(promise)]() mutable ->void{
        try{
            std::optional<std::pair<bool, std::string>> bRet;
            if(strLocalFile.empty() || strRemotePath.empty()){
                bRet = std::make_pair(false, g_mpFtpsErrMsg[EN_FTPS_INVALID_INPUT_ARGS]);
                promiseUpload.set_value(bRet);
                return ;
            }

            CURL * curl = curl_easy_init();
            if(!curl){
                bRet = std::make_pair(false, g_mpFtpsErrMsg[EN_FTPS_RESOURCE_INIT_ERROR]);
                promiseUpload.set_value(bRet);
                return ;
            }

            //open the local file to upload
            FILE * fp = fopen(strLocalFile.c_str(), "rb");
            if (!fp){
                bRet = std::make_pair(false, g_mpFtpsErrMsg[EN_FTPS_FAILED_TO_OPEN_LOCAL_FILE]);
                curl_easy_cleanup(curl);
                promiseUpload.set_value(bRet);
                return ;
            }

            //when the remote file passed as a directory, set the upload filename as the local filename
            std::string strRemotePathTemp = strRemotePath;
            if('/' == strRemotePathTemp.back()){
                fs::path fpFileName = fs::path(strLocalFile);
                strRemotePathTemp += fpFileName.filename().string();
            }

            //get the size of the file to upload
            fseek(fp, 0L, SEEK_END);
            size_t nFileSize = ftell(fp);
            fseek(fp, 0L, SEEK_SET);

            const std::string && strURL = this->getIp_Port() + strRemotePathTemp;
            const std::string && strUserPwd = this->getUser_Pwd();

            curl_easy_setopt(curl, CURLOPT_URL, strURL.c_str());
            curl_easy_setopt(curl, CURLOPT_USERPWD, strUserPwd.c_str());
            curl_easy_setopt(curl, CURLOPT_READFUNCTION, CFTPSClient::upReadCallback);
            curl_easy_setopt(curl, CURLOPT_READDATA, fp);
            curl_easy_setopt(curl, CURLOPT_FTP_CREATE_MISSING_DIRS, 1);
            curl_easy_setopt(curl, CURLOPT_UPLOAD, 1);
            curl_easy_setopt(curl, CURLOPT_INFILESIZE, nFileSize);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

            //perform the file upload request
            CURLcode enRet = curl_easy_perform(curl);
            curl_easy_cleanup(curl);
            fclose(fp);

            if (CURLE_OK == enRet){
                bRet = std::make_pair(true, std::string());
            }else{
                bRet = std::make_pair(false, std::string(curl_easy_strerror(enRet)));
            }

            promiseUpload.set_value(bRet);
        }catch(...){
            promiseUpload.set_exception(std::current_exception());
        }
    };

    std::thread(std::move(async_upFile)).detach();
    return future;
}

//download the given remote file to the local file, return true on success, otherwise return false
std::optional<bool> CFTPSClient::downFile(const std::string & strRemoteFile, const std::string & strLocalFile)
{
    if(strRemoteFile.empty() || strLocalFile.empty()){
        this->m_strErrMsg = g_mpFtpsErrMsg[EN_FTPS_INVALID_INPUT_ARGS];
        return std::nullopt;
    }

    if(!this->m_pCurl){
        this->m_strErrMsg = g_mpFtpsErrMsg[EN_FTPS_RESOURCE_INIT_ERROR];
        return std::nullopt;
    }

    //create the local file path if not existing
    if(!createDirectory(strLocalFile)){
        this->m_strErrMsg = g_mpFtpsErrMsg[EN_FTPS_FAILED_TO_CREATE_LOCAL_DIR];
        return std::nullopt;
    }

    //when the local file name passed as a directory, set the donw file name as the remote filename
    std::string strLocalFileTemp = strLocalFile;
    if('/' == strLocalFileTemp.back()){
        fs::path fpFileName = fs::path(strRemoteFile);
        strLocalFileTemp += fpFileName.filename().string();
    }

    FILE * fp = fopen(strLocalFileTemp.c_str(), "wb+");
    if(!fp){
        this->m_strErrMsg = g_mpFtpsErrMsg[EN_FTPS_FAILED_TO_OPEN_LOCAL_FILE];
        return std::nullopt;
    }

    this->m_fProgress = 0.0f;
    const std::string && strURL = this->getIp_Port() + strRemoteFile;
    const std::string && strUserPwd = this->getUser_Pwd();

    curl_easy_setopt(this->m_pCurl, CURLOPT_URL, strURL.c_str());
    curl_easy_setopt(this->m_pCurl, CURLOPT_USERPWD, strUserPwd.c_str());
    curl_easy_setopt(this->m_pCurl, CURLOPT_WRITEFUNCTION, CFTPSClient::downWriteCallback);
    curl_easy_setopt(this->m_pCurl, CURLOPT_PROGRESSFUNCTION, CFTPSClient::progressCallback);
    curl_easy_setopt(this->m_pCurl, CURLOPT_PROGRESSDATA, &this->m_fProgress);  // passing user data into the callback function
    curl_easy_setopt(this->m_pCurl, CURLOPT_NOPROGRESS, 0L); // enable progress callback
    curl_easy_setopt(this->m_pCurl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(this->m_pCurl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(this->m_pCurl, CURLOPT_SSL_VERIFYHOST, 0L);

    CURLcode enRet = curl_easy_perform(this->m_pCurl);
    fclose(fp);
    if (CURLE_OK == enRet){
        return true;
    }else{
        this->m_strErrMsg = std::string(curl_easy_strerror(enRet));
        return false;
    }
}

//async file download, return true on success, otherwise return false
std::future<std::optional<std::pair<bool, std::string>>> CFTPSClient::downFile_async(const std::string & strLocalFile, const std::string & strRemoteFile)
{
    std::promise<std::optional<std::pair<bool, std::string>>> promise;
    std::future<std::optional<std::pair<bool, std::string>>> future = promise.get_future();

    //async file download, NOTE: strLocalFile and strRemotePath can NOT captured by reference
    auto async_downFile = [this, strRemoteFile, strLocalFile, promiseDownload = std::move(promise)]() mutable ->void{
        try{
            std::optional<std::pair<bool, std::string>> bRet;
            if(strRemoteFile.empty() || strLocalFile.empty()){
                bRet = std::make_pair(false, g_mpFtpsErrMsg[EN_FTPS_INVALID_INPUT_ARGS]);
                promiseDownload.set_value(bRet);
                return ;
            }

            CURL * curl = curl_easy_init();
            if(!curl){
                bRet = std::make_pair(false, g_mpFtpsErrMsg[EN_FTPS_RESOURCE_INIT_ERROR]);
                promiseDownload.set_value(bRet);
                return ;
            }

            //create the local file path if not existing
            if(!CFTPSClient::createDirectory(strLocalFile)){
                bRet = std::make_pair(false, g_mpFtpsErrMsg[EN_FTPS_FAILED_TO_CREATE_LOCAL_DIR]);
                promiseDownload.set_value(bRet);
                return ;
            }

            //when the local file name passed as a directory, set the donw file name as the remote filename
            std::string strLocalFileTemp = strLocalFile;
            if('/' == strLocalFileTemp.back()){
                fs::path fpFileName = fs::path(strRemoteFile);
                strLocalFileTemp += fpFileName.filename().string();
            }

            FILE * fp = fopen(strLocalFileTemp.c_str(), "wb+");
            if(!fp){
                bRet = std::make_pair(false, g_mpFtpsErrMsg[EN_FTPS_FAILED_TO_OPEN_LOCAL_FILE]);
                promiseDownload.set_value(bRet);
                return ;
            }

            const std::string && strURL = this->getIp_Port() + strRemoteFile;
            const std::string && strUserPwd = this->getUser_Pwd();

            curl_easy_setopt(curl, CURLOPT_URL, strURL.c_str());
            curl_easy_setopt(curl, CURLOPT_USERPWD, strUserPwd.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CFTPSClient::downWriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

            //perform the download request
            CURLcode enRet = curl_easy_perform(curl);
            fclose(fp);
            curl_easy_cleanup(curl);

            if (CURLE_OK == enRet){
                bRet = std::make_pair(true, std::string());
            }else{
                bRet = std::make_pair(false, std::string(curl_easy_strerror(enRet)));
            }

            promiseDownload.set_value(bRet);
        }catch(...){
            promiseDownload.set_exception(std::current_exception());
        }
    };

    std::thread(std::move(async_downFile)).detach();
    return future;
}

//get the given remote file content, return as a std::string
std::optional<std::string> CFTPSClient::catFile(const std::string & strRemoteFile)
{
    if(strRemoteFile.empty()){
        this->m_strErrMsg = g_mpFtpsErrMsg[EN_FTPS_INVALID_INPUT_ARGS];
        return std::nullopt;
    }

    if(!this->m_pCurl){
        this->m_strErrMsg = g_mpFtpsErrMsg[EN_FTPS_RESOURCE_INIT_ERROR];
        return std::nullopt;
    }

    const std::string && strURL = this->getIp_Port() + strRemoteFile;
    const std::string && strUserPwd = this->getUser_Pwd();
    const std::string && strCmd = std::string("RETR ") + strRemoteFile;//MDTM file modification time，RETR  to download file content
    CURLcode enRet = CURL_LAST;
    std::string strRetFileInfo;

    curl_easy_setopt(this->m_pCurl, CURLOPT_URL, strURL.c_str());
    curl_easy_setopt(this->m_pCurl, CURLOPT_USERPWD, strUserPwd.c_str());
    curl_easy_setopt(this->m_pCurl, CURLOPT_CUSTOMREQUEST, strCmd.c_str());
    curl_easy_setopt(this->m_pCurl, CURLOPT_WRITEFUNCTION, CFTPSClient::readResponseDataCallback);
    curl_easy_setopt(this->m_pCurl, CURLOPT_WRITEDATA, &strRetFileInfo);
    curl_easy_setopt(this->m_pCurl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(this->m_pCurl, CURLOPT_SSL_VERIFYHOST, 0L);

    enRet = curl_easy_perform(this->m_pCurl);
    if(CURLE_OK == enRet){
        std::cout << strRetFileInfo;
        return strRetFileInfo;
    }else{
        this->m_strErrMsg = std::string(curl_easy_strerror(enRet));
        return std::nullopt;
    }
}

//get the file size of the given remote file
std::optional<std::int64_t> CFTPSClient::getFileSize(const std::string & strRemoteFile)
{
    if(strRemoteFile.empty()){
        this->m_strErrMsg = g_mpFtpsErrMsg[EN_FTPS_INVALID_INPUT_ARGS];
        return std::nullopt;
    }

    if(!this->m_pCurl){
        this->m_strErrMsg = g_mpFtpsErrMsg[EN_FTPS_RESOURCE_INIT_ERROR];
        return std::nullopt;
    }

    CURLcode enRet = CURL_LAST;
    const std::string && strURL = this->getIp_Port() + strRemoteFile;
    const std::string && strUserPwd = this->getUser_Pwd();
    std::string strRetFileInfo;

    curl_easy_setopt(this->m_pCurl, CURLOPT_URL, strURL.c_str());
    curl_easy_setopt(this->m_pCurl, CURLOPT_USERPWD, strUserPwd.c_str());
    curl_easy_setopt(this->m_pCurl, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(this->m_pCurl, CURLOPT_WRITEFUNCTION, CFTPSClient::throwAwayReturnData);
    curl_easy_setopt(this->m_pCurl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(this->m_pCurl, CURLOPT_SSL_VERIFYHOST, 0L);

    enRet = curl_easy_perform(this->m_pCurl);
    if (CURLE_OK == enRet){
        double fFileSize = 0.0f;
        enRet = curl_easy_getinfo(this->m_pCurl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &fFileSize);
        if(CURLE_OK == enRet){
            std::int64_t nFileSize = (std::int64_t)fFileSize;
            return nFileSize;
        }else{
            this->m_strErrMsg = std::string(curl_easy_strerror(enRet));
            return std::nullopt;
        }
    }else{
        this->m_strErrMsg = std::string(curl_easy_strerror(enRet));
        return std::nullopt;
    }
}

//get the file size of the given remote file, having the same function with the getFileSize
std::optional<std::uint64_t> CFTPSClient::getSize(const std::string & strRemoteFile)
{
    if(strRemoteFile.empty()){
        this->m_strErrMsg = g_mpFtpsErrMsg[EN_FTPS_INVALID_INPUT_ARGS];
        return std::nullopt;
    }

    if(!this->m_pCurl){
        this->m_strErrMsg = g_mpFtpsErrMsg[EN_FTPS_RESOURCE_INIT_ERROR];
        return std::nullopt;
    }

    CURLcode enRet = CURL_LAST;
    const std::string && strURL = this->getIp_Port() + strRemoteFile;
    const std::string && strUserPwd = this->getUser_Pwd();
    const std::string && strCmd = "SIZE " + strRemoteFile;
    std::string strRetFileInfo;

    curl_easy_setopt(this->m_pCurl, CURLOPT_URL, strURL.c_str());
    curl_easy_setopt(this->m_pCurl, CURLOPT_USERPWD, strUserPwd.c_str());
    curl_easy_setopt(this->m_pCurl, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(this->m_pCurl, CURLOPT_CUSTOMREQUEST, strCmd.c_str());
    curl_easy_setopt(this->m_pCurl, CURLOPT_WRITEFUNCTION, CFTPSClient::readResponseDataCallback);
    curl_easy_setopt(this->m_pCurl, CURLOPT_WRITEDATA, &strRetFileInfo);
    curl_easy_setopt(this->m_pCurl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(this->m_pCurl, CURLOPT_SSL_VERIFYHOST, 0L);

    enRet = curl_easy_perform(this->m_pCurl);
    if (CURLE_OK == enRet){
        auto getLength = [](const std::string & strInput)->std::optional<std::uint64_t> {
            const std::string & strPrefix = "Content-Length:";
            size_t nPos = strInput.find(strPrefix);

            if (nPos == std::string::npos) {
                return std::nullopt;
            }

            nPos += strPrefix.length();
            while (nPos < strInput.length() && std::isspace(strInput[nPos])) {
                nPos++;
            }

            std::string strNumber;
            while (nPos < strInput.length() && std::isdigit(strInput[nPos])) {
                strNumber += strInput[nPos];
                nPos++;
            }

            return strNumber.empty() ? std::nullopt : std::optional<std::uint64_t>{std::stoull(strNumber)};
        };

        return getLength(strRetFileInfo);
    }else{
        this->m_strErrMsg = std::string(curl_easy_strerror(enRet));
        return std::nullopt;
    }
}

 //get the last file modification time, return the timestamp and time stirng with format 'yyyy-MM-dd hh:mm:ss' on sccuess
std::optional<std::pair<std::int64_t, std::string>> CFTPSClient::getFileModificationTime(const std::string & strRemmoteFile)
{
    if(strRemmoteFile.empty() || '/' == strRemmoteFile.back()){
        this->m_strErrMsg = g_mpFtpsErrMsg[EN_FTPS_INVALID_INPUT_ARGS];
        return std::nullopt;
    }

    if(!this->m_pCurl){
        this->m_strErrMsg = g_mpFtpsErrMsg[EN_FTPS_RESOURCE_INIT_ERROR];
        return std::nullopt;
    }

    CURLcode enRet = CURL_LAST;
    const std::string && strURL = this->getIp_Port() + strRemmoteFile;
    const std::string && strUserPwd = this->getUser_Pwd();

    curl_easy_setopt(this->m_pCurl, CURLOPT_URL, strURL.c_str());
    curl_easy_setopt(this->m_pCurl, CURLOPT_USERPWD, strUserPwd.c_str());
    curl_easy_setopt(this->m_pCurl, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(this->m_pCurl, CURLOPT_FILETIME, 1L);
    curl_easy_setopt(this->m_pCurl, CURLOPT_WRITEFUNCTION, CFTPSClient::throwAwayReturnData);// disable debug info printing
    curl_easy_setopt(this->m_pCurl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(this->m_pCurl, CURLOPT_SSL_VERIFYHOST, 0L);

    enRet = curl_easy_perform(this->m_pCurl);
    if(enRet == CURLE_OK) {
        std::int64_t nTimeStamp = -1;
        enRet = curl_easy_getinfo(this->m_pCurl, CURLINFO_FILETIME, &nTimeStamp);
        if(enRet == CURLE_OK && nTimeStamp != -1) {
            std::time_t nTime = nTimeStamp;
            std::tm stUTCTime = *std::gmtime(&nTime);

            //convert utc time into a string with format 'yyyy-MM-dd hh:mm:s'
            std::ostringstream oss;
            oss << std::put_time(&stUTCTime, "%Y-%m-%d %H:%M:%S");
            return std::pair<std::uint64_t, std::string>{nTimeStamp, oss.str()};
        } else {
            this->m_strErrMsg = std::string(curl_easy_strerror(enRet));
            return std::nullopt;
        }
    } else {
        this->m_strErrMsg = std::string(curl_easy_strerror(enRet));
        return std::nullopt;
    }
}


//get the progress of file upload or download as a percentage value
std::optional<double> CFTPSClient::CFTPSClient::getProcess()
{
    if(!this->m_pCurl){
        this->m_strErrMsg = std::string("Failed to initialize network resource library");
        return std::nullopt;
    }

    return this->m_fProgress;
}

std::string CFTPSClient::getIp_Port()
{
    const std::string && strProtocol = (m_enMode == _EN_FTPS_) ? std::string("ftps://") : std::string("ftp://");
    return strProtocol + this->m_stParams.m_strIp + std::string(":") + std::to_string(this->m_stParams.m_nPort);
}

std::string CFTPSClient::getUser_Pwd()
{
    return this->m_stParams.m_strUserName + std::string(":") + this->m_stParams.m_strPassword;
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
size_t CFTPSClient::throwAwayReturnData(void * ptr, size_t size, size_t nmemb, void * data)
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

