#ifndef CDBCONNECTPOOL_H
#define CDBCONNECTPOOL_H

#include "mysql.h"
#include <queue>
#include <memory>
#include <string>

class CDBConnectPool
{
private:
    using DBConnPtr = std::unique_ptr<MYSQL, decltype(&mysql_close)>;

public:
    CDBConnectPool(const size_t nConnCount);
    ~CDBConnectPool() = default;

    const std::string & getErrMsg() const;
    int getConnCount() const;

    DBConnPtr & getConnetion()
    {
        return m_queConns.front();
    }

private:
    void connect2DB();

private:

    std::queue<DBConnPtr> m_queConns;
    std::string m_strErrMsg;
    size_t m_nConnCount = 2;
};

#endif // CDBCONNECTPOOL_H
