#ifndef CMYSQL_H
#define CMYSQL_H

#include "data_type_defination.h"

typedef struct ST_dbParams{
    std::string strIp = "127.0.0.1";
    std::uint16_t nPort = 3306;
    std::string strUsername = "root";
    std::string strPassword;
    std::string strDBName;
}StDBParams;

template<typename key, typename value>
using hash_map = std::unordered_map<key, value>;

// such the class being designed to focus on the basic db operation

class CMySQL
{
public:
    CMySQL() = default;
    ~CMySQL() = default;

    // void setParams(const StDBParams & stDBParams);

    // //connect to db
    // bool connectDB();

    // const std::string & getErrMsg() const;

    bool query_table(std::vector<StCourse> & vecResult);

    // template<class T>
    // T getItem(const size_t nIndex, const std::string & strFieldName);

private:
    //bool query(const std::string & strQuery);

private:
    // StDBParams m_stDBParams;
    // std::unique_ptr<MYSQL, decltype(&mysql_close)> m_pConn;

    // std::string m_strErrMsg;

    // hash_map<std::uint64_t, hash_map<std::string, std::string>> m_mpResult;
};

#endif // CMYSQL_H
