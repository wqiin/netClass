#include "cresourceinit.h"

CResourceInit::CResourceInit()
{
    this->m_code = curl_global_init(CURL_GLOBAL_DEFAULT);
}

CResourceInit::~CResourceInit()
{
    if(CURLE_OK == this->m_code){
        curl_global_cleanup();
    }
}

bool CResourceInit::init()
{
    static CResourceInit resInstance;
    if(CURLE_OK != resInstance.m_code){
        CResourceInit::g_strErrMsg = std::string(curl_easy_strerror(resInstance.m_code));
        return false;
    }

    return true;
}

const std::string & CResourceInit::getErrMsg()
{
    return CResourceInit::g_strErrMsg;
}
