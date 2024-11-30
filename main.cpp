
#include "cftpsclient.h"
#include "chttpclient.h"

#include <iostream>

int main(int argc, char *argv[])
{
    CFTPSClient ftp(std::string("hello"), std::string("515253"), std::string("127.0.0.1"), 21);
/*
    if(ftp.isParamsValid() != EN_FTPS_OK){
        std::cout << "ErrMsg: " << ftp.getLastErrMsg() << std::endl;
    }else{
        std::cout << "FTP parametes OK" << std::endl;
    }

    auto && vecFileNames = ftp.listDir("/wqiin/");
    if(vecFileNames.has_value()){
        for(const auto & item : *vecFileNames){
            std::cout << item << std::endl;
        }
    }

    auto && nRet = ftp.makeDir("/wqiin/ftp_test/test/");
    if(nRet.has_value()){
        if(*nRet == true)
            std::cout << "mkae dir OK" << std::endl;

        // nRet = ftp.rmDir("/wqiin/ftp_test/test/");
        // if(nRet.has_value() && *nRet){
        //     std::cout << "rm directory:OK" << std::endl;
        // }else{
        //     std::cout << "Error Occured when rm dir:" << ftp.getLastErrMsg() << std::endl;;
        // }
    }

    nRet = ftp.cd("/wqiin/git/");
    if(nRet.has_value() && *nRet){
        std::cout << "cd OK" << std::endl;
    }

    auto pwd = ftp.pwd();
    if(pwd.has_value()){
        std::cout << "pwd:" << *pwd << std::endl;
    }

    auto mkfile = ftp.makeFile("/wqiin/ftp_test/test/mkfile.hello");
    if(mkfile.has_value() && *mkfile){
        std::cout << "mkFile oK" << std::endl;
    }

    //CFTPSClient::createDirectory("/Users/wqiin/Desktop/C++17_file_system_test/hello.exe");

*/}
