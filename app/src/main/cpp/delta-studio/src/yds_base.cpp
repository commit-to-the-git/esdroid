#include "../include/yds_base.h"

#include <stdarg.h>
#include <stdio.h>

ysObject::ysObject() {
    m_typeID = "YS_OBJECT";
}

ysObject::ysObject(const char *typeID) {
    m_typeID = typeID;
}

ysObject::~ysObject() {
    /* void */
}

// #ifdef _debug
// void ysobject::raiseerrorbool condition const char *format
// {
//
// if condition
// {
//
// va_list argptr
// va_startargptr format
//
// char intermediatebuffer1024
// vsprintf_sintermediatebuffer 1024 format argptr
//
// char errorbuffer2048
// sprintf_serrorbuffer 2048 type %s\nname %s\n--------------------\n%s m_typeid m_debugname intermediatebuffer
//
// //yds_assertcondition errorbuffer
//
// }
//
// }
// #endif
