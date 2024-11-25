#include <QCoreApplication>

#include "cftpsclient.h"
#include "chttpclient.h"

#include <iostream>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    CFTPSClient ftp(std::string("hello"), std::string("515253"), std::string("127.0.0.1"), 21);

    if(ftp.isParamsValid() != EN_FTPS_OK){
        std::cout << "ErrMsg: " << ftp.getLastErrMsg() << std::endl;
    }else{
        std::cout << "FTP parametes OK" << std::endl;
    }

    return a.exec();
}
