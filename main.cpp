
#include "cftpsclient.h"
#include "chttpclient.h"

#include "threadPool.hpp"
#include "cmysql.h"

#include <iostream>
#include <chrono>

int main(int argc, char *argv[])
{
    const StHostInfo stHost = {std::string("hello"), std::string("515253"), std::string("127.0.0.1"), 21, FTPMode::_EN_FTP_};
    CFTPSClient ftp(stHost);

/*
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

    */

    /*
    auto getResp = [](){
        CHTTPClient http("www.baidu.com", 80);
        auto strRet = http.get("/");
        if(strRet.has_value()){
            std::cout << "message from http:" << *strRet << std::endl;
        }else{
            std::cout << "err msg from http:" <<http.getErrMsg() << std::endl;
        }
    };

    UT::CThreadPool pool(3);
    std::vector<std::future<void>> vecFuture;
    for(int ii = 0; ii < 50; ii++){
        vecFuture.emplace_back(pool.addTask(getResp));
    }


    for(auto & item : vecFuture)
        item.get();

    std::cout << "END\n";
    */

    StDBParams params;
    params.strPassword = "shan53...";
    params.strDBName = "wqiin";
    const std::string strSQL = "select * from course";

    auto test_lambda = [](){
        CMySQL sql;
        std::vector<StCourse> vecResult;
        sql.query_table(vecResult);
    };


    UT::CThreadPool pool(4);
    std::vector<std::future<void>> vecRet;

    auto start = std::chrono::high_resolution_clock::now();
    for(int ii = 0; ii< 10000; ii++){
        vecRet.emplace_back(pool.addTask(test_lambda));
    }

    for(auto & item : vecRet){
        item.get();
    }

    // 获取当前时间点，表示代码结束执行的时刻
    auto end = std::chrono::high_resolution_clock::now();

    // 计算执行时长
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // 输出执行时长
    std::cout << "Code execution time: " << duration.count() << " milliseconds" << std::endl;


    return 0;
}
