#ifndef YDS_BASE_H
#define YDS_BASE_H

#include "yds_dynamic_array.h"

class ysObject : public ysDynamicArrayElement {
public:
    ysObject();
    ysObject(const char *typeID);
    ~ysObject();

// #ifdef _debug
// void raiseerrorbool condition const char *format
// #else
// #define raiseerrorcondition format void0
// #endif

    const char *GetTypeID() const { return m_typeID; }

protected:
    const char *m_typeID;
};

// temp
#include "yds_error_system.h"

#endif /* YDS_BASE_H  */
