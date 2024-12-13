
#include "cftpsclient.h"
#include "chttpclient.h"

#include <iostream>

int main(int argc, char *argv[])
{
    const StHostInfo stHost = {std::string("hello"), std::string("515253"), std::string("127.0.0.1"), 21, FTPMode::_EN_FTP_};
    CFTPSClient ftp(stHost);


    auto bRet = ftp.isParamsValid();
    if(bRet.has_value() && *bRet){
        std::cout << "Parameters OK" << std::endl;
    }else{
        std::cout << "error message:" << ftp.getErrMsg() << std::endl;
        return -1;
    }

    auto upload = ftp.upFile_async("/Users/wqiin/Desktop/vs_python.py", "/wqiin/");
    auto bRet1 = upload.get();
    if(bRet1.has_value() && (*bRet1).first){
        std::cout << "async upload OK" << std::endl;
    }else{
        std::cout << "error message:" << bRet1->second << std::endl;
    }

    auto download = ftp.downFile_async("/Users/wqiin/Desktop/", "/wqiin/rename_python.py");
    auto bRet2 = download.get();
    if(bRet2.has_value() && bRet2->first){
        std::cout << "async down OK" << std::endl;
    }else{
        std::cout << "error message:" << bRet2->second << std::endl;
    }


    CHTTPClient http("127.0.0.1", 80);
    auto strRet = http.getResponse("/");
    if(strRet.has_value()){
        std::cout << "message from http:" << *strRet << std::endl;
    }else{
        std::cout << "err msg from http:" <<http.getErrMsg() << std::endl;
    }

    return 0;
}
