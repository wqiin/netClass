#ifndef CRESOURCEINIT_H
#define CRESOURCEINIT_H

#include <cstddef>
#include <string>

#include "curl/curl.h"

class CResourceInit
{
private:
    CResourceInit();

public:
    void * operator new(size_t) = delete;
    void * operator new[](size_t) = delete;

public:
    ~CResourceInit();

private:
    CURLcode m_code;

public:
    static bool init();//a singlton instance
    static const std::string & getErrMsg();

private:
    inline static std::string g_strErrMsg = "";//error message of curl init if
};

#endif // CRESOURCEINIT_H
