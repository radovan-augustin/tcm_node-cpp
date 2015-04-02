//
// Copyright (c) 2015 Radovan Augustin, rado_augustin@yahoo.com
// All rights reserved.
//
// This file is part of tcm_node-cpp, licensed under BSD 3-Clause License.
//

#ifndef __PY_H_
#define __PY_H_ 1

#include <list>
#include <map>
#include <vector>
#include <exception>
#include <stdio.h>

//
// Forward declarations
//

class PY_STRING;

//
// Exceptions
//

class _py_ExceptionBase: public std::exception
{
public:
    _py_ExceptionBase(void);
    _py_ExceptionBase(const char * msg);
    _py_ExceptionBase(int code);
    virtual ~_py_ExceptionBase() throw();

protected:
    const char * m_Msg;
};

class _py_SystemExit: public _py_ExceptionBase
{
public:
    _py_SystemExit(void);
    _py_SystemExit(int code);

    const char *    what() const throw();
    int             code(void) const throw();

protected:
    int m_Code;
};

class _py_IOError: public _py_ExceptionBase
{
public:
    _py_IOError(void);
    _py_IOError(const char * msg);

    const char * what() const throw();
};

class _py_OSError: public _py_ExceptionBase
{
public:
    _py_OSError(void);
    _py_OSError(const char * msg);

    const char * what() const throw();
};

//
// LIST_PY_STRING
// LIST_LIST_PY_STRING
//

typedef std::list<PY_STRING>            LIST_PY_STRING;
typedef LIST_PY_STRING::iterator        LIST_PY_STRING_IT;

typedef std::list<LIST_PY_STRING>       LIST_LIST_PY_STRING;
typedef LIST_LIST_PY_STRING::iterator   LIST_LIST_PY_STRING_IT;


//
// MAP_PY_STRING
//

typedef std::map<PY_STRING, PY_STRING>  MAP_PY_STRING;
typedef MAP_PY_STRING::iterator         MAP_PY_STRING_IT;

//
// VECTOR_PY_STRING
//

typedef std::vector<PY_STRING>          VECTOR_PY_STRING;
typedef VECTOR_PY_STRING::iterator      VECTOR_PY_STRING_IT;

//
// PY_STRING
//

class PY_STRING
{
public:
    PY_STRING(void);
    PY_STRING(const PY_STRING & other);
    PY_STRING(const char * other);

    ~PY_STRING(void);

    PY_STRING & operator=(const PY_STRING & rs);
    PY_STRING & operator=(const char * rs);

    PY_STRING & operator+=(const PY_STRING & rs);
    PY_STRING & operator+=(const char * rs);

    const PY_STRING operator+(const PY_STRING & other) const;
    const PY_STRING operator+(const char * other) const;

    bool operator==(const PY_STRING & other) const;
    bool operator==(const char * other) const;

    bool operator!=(const PY_STRING & other) const;
    bool operator!=(const char * other) const;

    operator char *() const;

    PY_STRING           format(const char * str, ...);          // Returns new instance of string
    PY_STRING           join(LIST_PY_STRING & list);            // Joins strings into new instance, string stored in instance is delimiter
    PY_STRING           lower(void);                            // Returns new instance of string
    PY_STRING           rstrip(void);                           // Returns new instance of string
    VECTOR_PY_STRING    split(void);
    VECTOR_PY_STRING    split(char delimiter);
    bool                starts_with(const char * str);
    PY_STRING           string_after(const char * str);         // Returns new instance of string
    PY_STRING           strip(void);                            // Returns new instance of string
    char *              strstr(const char * str);

protected:
    char *  m_Buffer;
};

//
// PY_FILE
//

class PY_FILE
{
public:
    PY_FILE(void);
    PY_FILE(const PY_FILE & other);

    ~PY_FILE();

    PY_FILE & operator=(const PY_FILE & rs);

    void            open(const char * filename);
    void            open(const char * filename, const char * mode);
    PY_STRING       read(void);                                             // Reads max. 4 * 1024 bytes
    PY_STRING       readline(void);
    LIST_PY_STRING  readlines(void);
    void            write(const char * str);
    void            close(void);
    bool            isopen(void);

protected:
    FILE *  m_File;
};

//
// _py_x() functions
//

void _py_sys_exit(void);                                                    // throws _py_SystemExit
void _py_sys_exit(int code);                                                // throws _py_SystemExit

LIST_PY_STRING  _py_os_listdir  (const char * dirname);                     // throws _py_OSError
void            _py_os_mkdir    (const char * dirname);                     // throws _py_OSError
void            _py_os_rmdir    (const char * dirname);                     // throws _py_OSError
int             _py_os_system   (const char * cmd);
void            _py_os_unlink   (const char * pathname);                    // throws _py_OSError
int             _py_os_major    (const char * devname);

void            _py_os_makedirs (const char * pathname);                    // throws _py_OSError
void            _py_os_makedirs (const char * pathname, int mode);          // throws _py_OSError

bool _py_os_path_isdir  (char * pathname);
bool _py_os_path_isfile (char * pathname);
bool _py_os_path_islink (char * pathname);

PY_STRING   _py_uuid_uuid4(void);

#endif /* __PY_H_ */
