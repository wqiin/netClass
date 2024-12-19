#ifndef SYSCONFIG_H
#define SYSCONFIG_H

#include "cmysql.h"

const int nConnCount = 10;//DB connection count
const int nThreadCount = 4;//thread count

const StDBParams DBparams = {
    "127.0.0.1",
    3306,
    "root",
    "shan53...",
    "wqiin"
};


#endif // SYSCONFIG_H
