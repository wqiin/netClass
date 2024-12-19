#include "cdbmanager.h"

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

    //从连接池中，取一下，并将其置为不可使用，这个函数调用完成后，将其置为可用
    auto query_lambda = [this, strSQL]()->std::pair<std::string, query_result>{
        auto & pConn = this->m_connPool.getConnetion();
        //perform db query
        if (mysql_query(pConn.get(), strSQL.c_str())) {
            return {std::string(mysql_error(pConn.get())), {}};
        }

        //get the result
        std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> pRes(nullptr, &mysql_free_result);
        pRes.reset(mysql_store_result(pConn.get()));
        if (nullptr == pRes) {
            return {std::string(mysql_error(pConn.get())), {}};
        }

        // 打印查询结果
        MYSQL_FIELD *fields = mysql_fetch_fields(pRes.get());
        int nFiledCount = mysql_num_fields(pRes.get());
        MYSQL_ROW row = nullptr;

        int nRowCount = 0;
        query_result mpResult;
        while ((row = mysql_fetch_row(pRes.get()))) {
            hash_map<std::string, std::string> mpFiledValue;

            for (int i = 0; i < nFiledCount; i++) {
                mpFiledValue.emplace(std::string(fields[i].name), std::string(row[i] ? row[i] : "NULL"));
            }

            mpResult.emplace(nRowCount++, std::move(mpFiledValue));
        }

        return {std::string(), std::move(mpResult)};
    };


    return this->submit(query_lambda);
}
