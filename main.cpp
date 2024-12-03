
#include "cftpsclient.h"
#include "chttpclient.h"

#include <iostream>

int main(int argc, char *argv[])
{
    CFTPSClient ftp(std::string("hello"), std::string("515253"), std::string("127.0.0.1"), 21);

    auto && vecFiles = ftp.listDir_detailed("/wqiin");

    if(vecFiles.has_value()){
        for(const auto & item : *vecFiles){

            std::cout << "Permission:" << item.strPermission << " linked:" << item.nLinkCount
                      << " Owner:" << item.strOwner << " Group:" << item.strGroup
                      << " size:" << item.nSize << " Month:" << item.strMonth
                      << " day:" << item.nDay << " time:" << item.strTime
                      << " filenmame:" << item.strFileName << std::endl;
            break;
        }
    }else{
        std::cout << "ErrMsg:" << ftp.getErrMsg() << std::endl;
    }

    /*
    auto && vecDir = ftp.listDir("/wqiin");
    if(vecDir.has_value()){
        for(const auto & item : *vecDir){
            std::cout << item << "\n";
        }
    }
    */
    //ftp.cd("/wqiin/desktop");//works well

    /*
    auto && strPwd = ftp.pwd();
    if(strPwd.has_value()){
        std::cout << "pwd:" << *strPwd << std::endl;
    }else{
        std::cout << "Error Message:" << ftp.getErrMsg() << std::endl;
    }
    */


    /*
    //ftp.makeDir("/wqiin/test_ftp");
    auto && bRet = ftp.removeDir("/wqiin/test_ftp");
    if(bRet.has_value() && *bRet){
        std::cout << "Remove dir OK" << std::endl;
    }else{
        std::cout << "Error Message:" << ftp.getErrMsg() << std::endl;
    }
    */

    /*
    auto && bRet = ftp.downFile("/wqiin/install.sh", "/Users/wqiin/Desktop/install.sh");

    if(bRet.has_value() && *bRet)
    {
        std::cout << "file download OK" << std::endl;
    }
    */

    /*
    auto && bRet = ftp.upFile("/Users/wqiin/Desktop/vs_python.py", "/wqiin/vs_python.py");
    if(bRet.has_value() && *bRet){
        std::cout << "Upload OK" << std::endl;
    }
    */

    /*
     * //working not well
    auto && bRet = ftp.removeFile("/wqiin/vs_python.py");
    if(bRet.has_value() && *bRet){
        std::cout << "delete OKK" << std::endl;
    }else{
        std::cout << "Error Message:" << ftp.getErrMsg() << std::endl;
    }
    */
    /*
    auto && bRet = ftp.renameFile("/wqiin/vs_python.py", "/wqiin/rename_python.py");
    if(bRet.has_value() && *bRet){
        std::cout << "Rename OK" << std::endl;
    }else{
        std::cout << "Error Message:" << ftp.getErrMsg() << std::endl;
    }
    */

    /*
     * //not supported commands
    auto && bRet = ftp.copyFile("/wqiin/vs_python.py", "/wqiin/copy_python.py");
    if(bRet.has_value() && *bRet){
        std::cout << "copy file OK" << std::endl;
    }else{
        std::cout << "Error Message:" << ftp.getErrMsg() << std::endl;
    }
    */

    /*
    auto && nFileSize = ftp.getFileSize("/wqiin/rename_python.py");
    if(nFileSize.has_value()){
        std::cout << "file size:" << *nFileSize << std::endl;
    }else{
        std::cout << "Error Message:" << ftp.getErrMsg() << std::endl;
    }
    */

    /*
    auto && bRet = ftp.isParamsValid();

    if(bRet.has_value() && *bRet){
        std::cout << "ErrMsg: " << ftp.getErrMsg() << std::endl;
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
    */
}
