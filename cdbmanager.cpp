#include "cdbmanager.h"

#include <iostream>

CDBManager::CDBManager(const size_t nThreadCount/*=4*/, const size_t nConnCount/*=10*/):m_threadPool(nThreadCount), m_connPool(nConnCount)
{}


template<typename Func, typename... Args>
auto CDBManager::submit(Func && func, Args&&... args)->std::future<decltype(func(args...))>
{
    return m_threadPool.addTask(func, args...);
}

CDBManager::optResult CDBManager::query(const std::string & strSQL)
{
    if(strSQL.empty())
        return std::nullopt;

    auto query_lambda = [this, strSQL]()->std::pair<std::string, query_result>{
        auto & pConn = this->m_connPool.getAConn();
        CDBConnectPool::ConnManager connManger(pConn);

        std::cout << "thread_id:" << std::this_thread::get_id() << "  conn_address:" << pConn.second.get() << std::endl;

        //perform db query
        if (mysql_query(pConn.second.get(), strSQL.c_str())) {
            return {std::string(mysql_error(pConn.second.get())), {}};
        }

        //get the result
        std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> pRes(mysql_store_result(pConn.second.get()), &mysql_free_result);
        if (nullptr == pRes) {
            return {std::string(mysql_error(pConn.second.get())), {}};
        }

        //get the result of the query
        MYSQL_FIELD *fields = mysql_fetch_fields(pRes.get());
        int nFiledCount = mysql_num_fields(pRes.get());
        MYSQL_ROW row = nullptr;

        int nRowCount = 0;
        query_result mpResult;
        while ((row = mysql_fetch_row(pRes.get()))) {
            hash_map<std::string, std::string> mpFiledValue;//a row, key->field name, value->field value

            for (int i = 0; i < nFiledCount; i++) {
                mpFiledValue.emplace(std::string(fields[i].name), std::string(row[i] ? row[i] : "NULL"));
            }

            mpResult.emplace(nRowCount++, std::move(mpFiledValue));
        }

        return {std::string(), std::move(mpResult)};
    };

    return this->submit(query_lambda);
}
