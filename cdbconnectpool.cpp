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
            m_queConns.emplace(std::move(pConnTemp));
        }else{
            this->m_strErrMsg = std::string(mysql_error(pConnTemp.get()));
            if(0 == --nTryTime)
                throw std::runtime_error(this->m_strErrMsg);
            continue;
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

