#include "cdbconnectpool.h"

#include "SysConfig.h"

#include <stdexcept>

CDBConnectPool::CDBConnectPool(const size_t nConnCount)
{
    if(0 != nConnCount)
        this->m_nConnCount = nConnCount;

    this->connect2DB();
}

void CDBConnectPool::connect2DB()
{
    int nTryTime = 3;
    for(size_t ii = 0; ii < this->m_nConnCount; ii++){
        DBConnPtr pConnTemp(nullptr, &mysql_close);

        pConnTemp.reset(mysql_init(NULL));
        if(!pConnTemp){
            if(0 == --nTryTime)
                throw std::runtime_error("failed to calling 'mysql_init'...");
            continue;
        }

        auto pRet = mysql_real_connect(pConnTemp.get(),
                                       DBparams.strIp.c_str(),
                                       DBparams.strUsername.c_str(),
                                       DBparams.strPassword.c_str(),
                                       DBparams.strDBName.c_str(),
                                       DBparams.nPort, NULL, 0);

        if(nullptr != pRet){
            //m_queConns.emplace(std::move(pConnTemp));
            m_lstConns.emplace_back(false, std::move(pConnTemp));
        }else{
            this->m_strErrMsg = std::string(mysql_error(pConnTemp.get()));
            if(0 == --nTryTime)
                throw std::runtime_error(this->m_strErrMsg);
        }
    }
}


const std::string & CDBConnectPool::getErrMsg() const
{
    return this->m_strErrMsg;
}

int CDBConnectPool::getConnCount() const
{
    return this->m_nConnCount;
}

std::pair<std::atomic<bool>, CDBConnectPool::DBConnPtr> & CDBConnectPool::getAConn()
{
    {
        std::lock_guard<std::mutex> lock_guard(this->m_mtx);
        for(auto & conn : m_lstConns){
            if(false == conn.first.load()){
                conn.first.store(true);
                return conn;
            }
        }
    }

    //if all conn being busy, and creat a new connection
    int nTryTime = 3;
    for(size_t ii = 0; ii < 3; ii++){
        DBConnPtr pConnTemp(nullptr, &mysql_close);
        pConnTemp.reset(mysql_init(NULL));
        if(!pConnTemp){
            if(0 == --nTryTime)
                throw std::runtime_error("failed to calling 'mysql_init'...");
            continue;
        }

        auto pRet = mysql_real_connect(pConnTemp.get(),
                                       DBparams.strIp.c_str(),
                                       DBparams.strUsername.c_str(),
                                       DBparams.strPassword.c_str(),
                                       DBparams.strDBName.c_str(),
                                       DBparams.nPort, NULL, 0);
        if(nullptr != pRet){
            std::lock_guard<std::mutex> lock_guard(this->m_mtx);
            m_lstConns.emplace_back(true, std::move(pConnTemp));//making flag busy
            return m_lstConns.back();
        }else{
            this->m_strErrMsg = std::string(mysql_error(pConnTemp.get()));
            if(0 == --nTryTime)
                throw std::runtime_error(this->m_strErrMsg);
        }
    }
}
