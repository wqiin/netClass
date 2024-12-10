
#include "cftpsclient.h"
#include "chttpclient.h"

#include <iostream>

int main(int argc, char *argv[])
{
    const StHostInfo stHost = {std::string("hello"), std::string("515253"), std::string("127.0.0.1"), 21};
    CFTPSClient ftp(stHost, FTPMode::_EN_FTP_);


    auto bRet = ftp.isParamsValid();
    if(bRet.has_value() && *bRet){
        std::cout << "Parameters OK" << std::endl;
    }

    auto upload = ftp.upFile_async("/Users/wqiin/Desktop/vs_python.py", "/wqiin/");
    bRet = upload.get();
    if(bRet.has_value() && *bRet){
        std::cout << "async upload OK";
    }else{
        std::cout << "error message:" << ftp.getErrMsg() << std::endl;
    }


    return 0;
}
