
#include "cftpsclient.h"
#include "chttpclient.h"

#include <iostream>

int main(int argc, char *argv[])
{
    CFTPSClient ftp(std::string("hello"), std::string("515253"), std::string("127.0.0.1"), 21);

    if(ftp.isParamsValid() != EN_FTPS_OK){
        std::cout << "ErrMsg: " << ftp.getLastErrMsg() << std::endl;
    }else{
        std::cout << "FTP parametes OK" << std::endl;
    }

    auto && vecFileNames = ftp.lstDir("/wqiin/");
    if(vecFileNames.has_value()){
        for(const auto & item : *vecFileNames){
            std::cout << item << std::endl;
        }
    }

    auto && nRet = ftp.mkDir("/wqiin/ftp_test/");
    if(nRet.has_value()){
        std::cout << "mkae dir OK" << std::endl;

        nRet = ftp.rmDir("/wqiin/ftp_test");
        if(nRet.has_value() && *nRet){
            std::cout << "rm directory:OK" << std::endl;
        }
    }

    nRet = ftp.cd("/wqiin/git/");
    if(nRet.has_value() && *nRet){
        std::cout << "cd OK" << std::endl;
    }

}
