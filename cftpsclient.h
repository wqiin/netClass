#ifndef CFTPSCLIENT_H
#define CFTPSCLIENT_H

/*
 * CFTPSClient is NOT a thread-safe class, when a single object being used in various thread, it is
 * essential to avoide the occurance of the race condition.
 *
 *
 * Created by zhangys on 2024-11-25
 */

#include "curl/curl.h"

#include <optional>
#include <vector>
#include <string>
#include <cstdint>


//remote host service and user information
typedef struct STHostInfo
{
    std::string m_strUserName = "";
    std::string m_strPassword = "";
    std::string m_strIp = "";
    std::uint16_t m_nPort = 0;
}StHostInfo;


//async file transfermation

class CFTPSClient
{
private:
    StHostInfo m_stParams;//ftp connection parameters
    std::int64_t m_nTimeCost = 0;
    std::string m_strErrMsg = "";//error message of the last operation
    CURL * m_pCurl = nullptr;//curl session handle

public:
    CFTPSClient(const StHostInfo & stInfo);
    CFTPSClient(const std::string & strUserName,
                const std::string & strPassword,
                const std::string & strIP,
                const std::uint16_t nPort);
    ~CFTPSClient();

    //copy constructor and assignment operator prohibited
    CFTPSClient(const CFTPSClient & ) = delete;
    CFTPSClient(const CFTPSClient && ) = delete;
    void operator=(const CFTPSClient &) = delete;
    void operator=(const CFTPSClient &&) = delete;


    /*************************************************************************
     *  setting the remote service parameters and user logging parameters    *
    *************************************************************************/
    CFTPSClient & setUserName(const std::string & strUserName);
    CFTPSClient & setPassword(const std::string & strPassword);
    CFTPSClient & setPort(const std::uint16_t nPort);
    CFTPSClient & setIP(const std::string & strIP);
    const StHostInfo & getParams() const;


    //to verify the parameters valid or not, return true when parameters valid,otherwise return false
    std::optional<bool> isParamsValid();

    bool newSession();

    /**************************************
     * directory operation functionalities*
    **************************************/
    //make the given remote directory, return true on success, otherwise return false
    std::optional<bool> makeDir(const std::string & strRemoteDir);

    //remove the given remote directory, return true on success, otherwise return false
    std::optional<bool> removeDir(const std::string & strRemoteDir);

    //list the filename on given directory, the filename would be put into the returned vector
    std::optional<std::vector<std::string>> listDir(const std::string & strRemoteDir);

    //change the working directory, return true on success, otherwise return false
    std::optional<bool> cd(const std::string & strRemoteDir);

    //get the current working directory, return as a string
    std::optional<std::string> pwd();


    /**************************************
     * file operation functionalities     *
    **************************************/

    //make a given file in the remote,  return true on success, otherwise return false
    std::optional<bool> makeFile(const std::string & strRemoteFile);

    //renam the given remote file, return true on success, otherwise return false
    std::optional<bool> renameFile(const std::string & strOldName, const std::string & newName);

    //remove the given remote file, return true on success otherwise return false
    std::optional<bool> removeFile(const std::string & strRemoteFile);

    //upload the given local file to the remote path, return true on success, otherwise return false
    std::optional<bool> upFile(const std::string & strLocalFile, const std::string & strRemotePath);

    //download the given remote file to the local path, return true on success, otherwise return false
    std::optional<bool> downFile(const std::string & strRemoteFile, const std::string & strLocalPath);

    //copy the given remote file into the given remote directory
    std::optional<bool> copyFile(const std::string & strRemoteFile, const std::string & strRemotePath);

    //get modification time of the given remote file, return true on success, otherwise return false
    std::optional<bool> geFiletModifiedTime(const std::string & strRemoteFile);

    //get the file size of the given remote file
    std::optional<std::int64_t> getFileSize(const std::string & strRemoteFileName);


    /**************************************
     * general functionalities            *
    **************************************/
    //get the progress of file upload or download as a percentage value
    std::optional<double> getProcess();

    //get time cost of the last functionality calling, return a positive valid value or a negative invalid value, (unit:ms)
    std::int64_t getTimeCost() const;

    //get the error message according to the error code
    const std::string & getErrMsg() const;

    //mage file upload or download
    //async file transportation

private:
    //upload read callback, being used to read the local file to upload
    static size_t upReadCallback(void * ptr, size_t size, size_t nmemb, void * stream);

    //download write callback, being used to write the received data into the destination local file
    static size_t downWriteCallback(void * ptr, size_t size, size_t nmemb, void * stream);

    //callback functionality used to receive the returned info from the remote
    static size_t readResponseDataCallback(void * ptr, size_t size, size_t nmemb, void * stream);

    //ignore the data from the remote, do nothing
    static size_t throw_away(void * ptr, size_t size, size_t nmemb, void * data);

    //the get the progress of the file transportation
    static int progressCallback(void* p, double dltotal, double dlnow, double uptotal, double upnow);

public:
    //craete the missing directory og the given file
    static bool createDirectory(const std::string & strFile);

private:
    std::string getIp_Port();
    std::string getUser_Pwd();
};

#endif // CFTPSCLIENT_H
