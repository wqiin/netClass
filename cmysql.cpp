#include "cmysql.h"

#include "cdbmanager.h"

/*
CMySQL::CMySQL(const StDBParams & stDBParams)//:m_pConn(nullptr, &mysql_close)
{
    // this->m_stDBParams = stDBParams;
    // this->connectDB();
}
void CMySQL::setParams(const StDBParams & stDBParams)
{
    this->m_stDBParams = stDBParams;
}

bool CMySQL::connectDB()
{
    m_pConn.reset(mysql_init(NULL));
    if(!m_pConn){
        this->m_strErrMsg = std::string("Failed init MYSQL resource...");
        return false;
    }

    auto pRet = mysql_real_connect(m_pConn.get(),
                       this->m_stDBParams.strIp.c_str(),
                       this->m_stDBParams.strUsername.c_str(),
                       this->m_stDBParams.strPassword.c_str(),
                       this->m_stDBParams.strDBName.c_str(),
                       this->m_stDBParams.nPort, NULL, 0);
    if(nullptr == pRet){
        this->m_strErrMsg = std::string(mysql_error(this->m_pConn.get()));
        return false;
    }

    return true;
}

const std::string & CMySQL::getErrMsg() const
{
    return this->m_strErrMsg;
}

bool CMySQL::query(const std::string & strQuery)
{
    std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> pRes(nullptr, &mysql_free_result);

    //perform db query
    if (mysql_query(this->m_pConn.get(), strQuery.c_str())) {
        this->m_strErrMsg = std::string(mysql_error(this->m_pConn.get()));
        return false;
    }

    //get the result
    pRes.reset(mysql_store_result(this->m_pConn.get()));
    if (nullptr == pRes) {
        this->m_strErrMsg = std::string(mysql_error(this->m_pConn.get()));
        return false;
    }

    // 打印查询结果
    MYSQL_FIELD *fields = mysql_fetch_fields(pRes.get());
    int nFiledCount = mysql_num_fields(pRes.get());
    MYSQL_ROW row = nullptr;

    int nRowCount = 0;
    while ((row = mysql_fetch_row(pRes.get()))) {
        hash_map<std::string, std::string> mpFiledValue;

        for (int i = 0; i < nFiledCount; i++) {
            mpFiledValue.emplace(std::string(fields[i].name), std::string(row[i] ? row[i] : "NULL"));
        }

        m_mpResult.emplace(nRowCount++, std::move(mpFiledValue));
    }

    return true;
}

template<class T>
T CMySQL::getItem(const size_t nIndex, const std::string & strFieldName)
{
    if(!m_mpResult.count(nIndex))
        throw std::out_of_range("invalid nIndex access...");

    if(strFieldName.empty())
        throw std::runtime_error("empty field name");

    if(!m_mpResult.at(nIndex).count(strFieldName))
        throw std::runtime_error("invalid filed name access...");

    T retValue;
    const std::string & strValue = m_mpResult.at(nIndex).at(strFieldName);
    std::istringstream stream(strValue);
    if (!(stream >> retValue)) {
        throw std::invalid_argument("invalid conversion from string to " + std::string(typeid(T).name()));
    }

    return retValue;
}
*/
bool CMySQL::query_table(std::vector<StCourse> & vecResult)
{
    const std::string strSQL = "select * from course";
    vecResult.clear();

    auto && query_result = DBOPT.query(strSQL);
    if(!query_result.has_value())
        return false;

    auto && pairResult = query_result->get();
    if(!pairResult.first.empty()){
        std::cout << "Db operation error message:"  << pairResult.first << std::endl;
        return false;
    }
    auto && records = pairResult.second;

    for(size_t ii = 0; ii < records.size(); ii++){
        StCourse stTemp;
        stTemp.nID = records.getItem<std::int64_t>(ii, "id");
        stTemp.strCourseName = records.getItem<std::string>(ii, std::string("name"));
        stTemp.nRelatedID = records.getItem<std::uint64_t>(ii, std::string("t_id"));
        stTemp.fFloat = records.getItem<float>(ii, "float");
        stTemp.fDouble = records.getItem<double>(ii, "double");
        stTemp.nDecimal = records.getItem<int>(ii, "decimal");
        stTemp.time = records.getItem<StTime>(ii, "time");
        stTemp.date = records.getItem<StDate>(ii, "date");
        stTemp.datetime = records.getItem<StDateTime>(ii, "datetime");
        stTemp.point = records.getItem<StPoint<int>>(ii, "point");
        stTemp.rect = records.getItem<StRect<int>>(ii, "rectangle");
        vecResult.emplace_back(stTemp);
    }

    return true;
}
