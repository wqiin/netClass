#ifndef CDBMANAGER_H
#define CDBMANAGER_H

#include "threadPool.hpp"
#include "cdbconnectpool.h"

#include <optional>
#include <sstream>

template<typename key, typename value>
using hash_map = std::unordered_map<key, value>;
using map_result = hash_map<std::uint64_t, hash_map<std::string, std::string>>;

class query_result : public map_result{
    public:
    template<class T>
    T getItem(const size_t nIndex, const std::string & strFieldName)
    {
        if(!this->count(nIndex))
            throw std::out_of_range("invalid nIndex access...");

        if(strFieldName.empty())
            throw std::runtime_error("empty field name");

        if(!this->at(nIndex).count(strFieldName))
            throw std::runtime_error("invalid filed name access...");

        T retValue;
        const std::string & strValue = this->at(nIndex).at(strFieldName);
        std::istringstream stream(strValue);
        if (!(stream >> retValue)) {
            throw std::invalid_argument("invalid conversion from string to " + std::string(typeid(T).name()));
        }

        return retValue;
    }
};



class CDBManager
{
public:
    static CDBManager & getInst(){
        static CDBManager inst(1, 1);
        return inst;
    }

public:
    explicit CDBManager(const size_t nThreadCount = 4, const size_t nConnCount = 10);

    //std::optional<std::future<std::pair<std::string, std::unordered_map<std::uint64_t, std::unordered_map<std::string, std::string>>>>>
    using optResult = std::optional<std::future<std::pair<std::string, query_result>>>;
    optResult query(const std::string & strSQL);

private:
    template<typename Func, typename... Args>
    auto submit(Func && func, Args&&... args)->std::future<decltype(func(args...))>;


private:
    UT::CThreadPool m_threadPool;
    CDBConnectPool m_connPool;
};

#define DBOPT CDBManager::getInst()

#endif // CDBMANAGER_H
