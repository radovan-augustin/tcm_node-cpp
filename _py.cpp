//
// Copyright (c) 2015 Radovan Augustin, rado_augustin@yahoo.com
// All rights reserved.
//
// This file is part of tcm_node-cpp, licensed under BSD 3-Clause License.
//

#include <sys/stat.h>
#include <sys/types.h>
#include <uuid/uuid.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>

#include "_py.h"

#define STR_ERR_CAN_NOT_ALLOCATE_MEMORY     "Can not allocate memory"

//
// Exceptions
//

_py_ExceptionBase::_py_ExceptionBase(void)
    : m_Msg(NULL)
{
}

_py_ExceptionBase::_py_ExceptionBase(const char * msg)
    : m_Msg(strdup(msg))
{
}

_py_ExceptionBase::_py_ExceptionBase(int code)
{
    char buffer[32];

    sprintf(buffer, "%d", code);
    m_Msg = strdup(buffer);
}

_py_ExceptionBase::~_py_ExceptionBase() throw()
{
    if (m_Msg != NULL)
        free((char *)m_Msg);
}

// _py_SystemExit

_py_SystemExit::_py_SystemExit(void)
    : _py_ExceptionBase()
    , m_Code(0)
{
}

_py_SystemExit::_py_SystemExit(int code)
    : _py_ExceptionBase()
    , m_Code(code)
{
}

const char * _py_SystemExit::what() const throw()
{
    return m_Msg != NULL ? m_Msg : "System exit";
}

int _py_SystemExit::code(void) const throw()
{
    return m_Code;
}

// _py_IOError

_py_IOError::_py_IOError(void)
    : _py_ExceptionBase()
{
}

_py_IOError::_py_IOError(const char * msg)
    : _py_ExceptionBase(msg)
{
}

const char * _py_IOError::what() const throw()
{
    return m_Msg != NULL ? m_Msg : "IO error";
}

// _py_OSError

_py_OSError::_py_OSError(void)
    : _py_ExceptionBase()
{
}

_py_OSError::_py_OSError(const char * msg)
    : _py_ExceptionBase(msg)
{
}

const char * _py_OSError::what() const throw()
{
    return m_Msg != NULL ? m_Msg : "OS error";
}

//
// PY_STRING
//

typedef struct py_string_info
{
    int     bytes_num;                  // Number of allocated bytes for buffer for storing string - sizeof(PY_STRING_INFO), can be > strlen(PY_STRING::m_Buffer)
    int     ref_cnt;                    // Reference counter for string
} PY_STRING_INFO;

PY_STRING::PY_STRING(void)
    : m_Buffer(NULL)
{
}

PY_STRING::PY_STRING(const PY_STRING & other)
    : m_Buffer(NULL)
{
    char *              other_str = (char *) other;
    PY_STRING_INFO *    info;

    if (other_str == NULL)
        return;

    // Increment external ref_cnt
    info = (PY_STRING_INFO *) (other_str - sizeof(PY_STRING_INFO));
    info->ref_cnt ++;

    m_Buffer = other_str;
}

PY_STRING::PY_STRING(const char * str)
    : m_Buffer(NULL)
{
    if ((str == NULL) || (*str == '\0'))
        return;

    int                 bytes_num_req = strlen(str) + 1;
    PY_STRING_INFO *    info;

    m_Buffer = (char *) malloc(sizeof(PY_STRING_INFO) + bytes_num_req);
    if (m_Buffer == NULL)
        throw _py_OSError(STR_ERR_CAN_NOT_ALLOCATE_MEMORY);
    info = (PY_STRING_INFO *) m_Buffer;
    m_Buffer += sizeof(PY_STRING_INFO);

    info->bytes_num = bytes_num_req;
    info->ref_cnt = 1;
    memmove(m_Buffer, str, bytes_num_req);
}

PY_STRING::~PY_STRING(void)
{
    if (m_Buffer == NULL)
        return;

    PY_STRING_INFO * info = (PY_STRING_INFO *) (m_Buffer - sizeof(PY_STRING_INFO));

    info->ref_cnt --;
    if (info->ref_cnt == 0)
        free(info);
}

PY_STRING & PY_STRING::operator=(const PY_STRING & rs)
{
    if (this == &rs)
        return *this;

    char *              rs_str = (char *) rs;
    PY_STRING_INFO *    info = NULL;

    // Decrement internal ref_cnt
    if (m_Buffer != NULL)
    {
        info = (PY_STRING_INFO *) (m_Buffer - sizeof(PY_STRING_INFO));
        info->ref_cnt --;
        if (info->ref_cnt == 0)
            free(info);
        m_Buffer = NULL;
    }

    // Increment external ref_cnt
    if (rs_str != NULL)
    {
        info = (PY_STRING_INFO *) (rs_str - sizeof(PY_STRING_INFO));
        info->ref_cnt ++;
    }

    m_Buffer = rs_str;

    return *this;
}

PY_STRING & PY_STRING::operator=(const char * rs)
{
    PY_STRING_INFO *    info;
    int                 bytes_num_req = 0;

    if (m_Buffer == rs)
        return *this;

    if ((rs != NULL) && (*rs != '\0'))
        bytes_num_req = strlen(rs) + 1;

    // Decrement internal ref_cnt
    if (m_Buffer != NULL)
    {
        info = (PY_STRING_INFO *) (m_Buffer - sizeof(PY_STRING_INFO));
        info->ref_cnt --;
        // Test if m_Buffer can be used
        if (info->ref_cnt != 0)
            m_Buffer = NULL;
    }

    // Free not needed or not usable m_Buffer
    if (m_Buffer != NULL)
        if ((bytes_num_req == 0) || (info->bytes_num < bytes_num_req))
        {
            free(info);
            m_Buffer = NULL;
        }

    if (bytes_num_req == 0)
        return *this;

    if (m_Buffer == NULL)
    {
        m_Buffer = (char *) malloc(sizeof(PY_STRING_INFO) + bytes_num_req);
        if (m_Buffer == NULL)
            throw _py_OSError(STR_ERR_CAN_NOT_ALLOCATE_MEMORY);
        info = (PY_STRING_INFO *) m_Buffer;
        info->bytes_num = bytes_num_req;
        m_Buffer += sizeof(PY_STRING_INFO);
    }

    memmove(m_Buffer, rs, bytes_num_req);

    info->ref_cnt = 1;

    return *this;
}

PY_STRING & PY_STRING::operator+=(const PY_STRING & rs)
{
    return this->operator+=((char *)rs);
}

PY_STRING & PY_STRING::operator+=(const char * rs)
{
    PY_STRING_INFO *    info;
    char *              str = NULL;
    bool                str_free = false;
    int                 bytes_num_req;

    if ((rs == NULL) || (*rs == '\0'))
        return *this;

    bytes_num_req = strlen(rs) + 1;
    if (m_Buffer != NULL)
        bytes_num_req += strlen(m_Buffer);

    // Decrement internal ref_cnt
    if (m_Buffer != NULL)
    {
        info = (PY_STRING_INFO *) (m_Buffer - sizeof(PY_STRING_INFO));
        info->ref_cnt --;
        // Test if m_Buffer can be used
        if (info->ref_cnt != 0)
        {
            str = m_Buffer;
            m_Buffer = NULL;
        }
    }

    // Test if m_Buffer has enough size
    if (m_Buffer != NULL)
        if (info->bytes_num < bytes_num_req)
        {
            str = m_Buffer;
            str_free = true;
            m_Buffer = NULL;
        }

    if (m_Buffer == NULL)
    {
        m_Buffer = (char *) malloc(sizeof(PY_STRING_INFO) + bytes_num_req);
        if (m_Buffer == NULL)
            throw _py_OSError(STR_ERR_CAN_NOT_ALLOCATE_MEMORY);
        info = (PY_STRING_INFO *) m_Buffer;
        info->bytes_num = bytes_num_req;
        m_Buffer += sizeof(PY_STRING_INFO);
        *m_Buffer = '\0';
    }

    if (str != NULL)
    {
        strcpy(m_Buffer, str);
        if (str_free)
            free(str - sizeof(PY_STRING_INFO));
    }
    strcat(m_Buffer, rs);

    info->ref_cnt = 1;

    return *this;
}

const PY_STRING PY_STRING::operator+(const PY_STRING & other) const
{
    return PY_STRING(*this) += other;
}

const PY_STRING PY_STRING::operator+(const char * other) const
{
    return PY_STRING(*this) += other;
}

bool PY_STRING::operator==(const PY_STRING & other) const
{
    return this->operator==((char *)other);
}

bool PY_STRING::operator==(const char * other) const
{
    if ((m_Buffer == NULL) && (other == NULL))
        return true;
    if ((m_Buffer == NULL) && (*other == '\0'))
        return true;
    if ((m_Buffer == NULL) || (other == NULL))
        return false;
    if ((m_Buffer == NULL) || (*other == '\0'))
        return false;
    return (0 == strcmp(m_Buffer, other));
}

bool PY_STRING::operator!=(const PY_STRING & other) const
{
    return !(*this == other);
}

bool PY_STRING::operator!=(const char * other) const
{
    return !(*this == other);
}

PY_STRING::operator char *() const
{
    return m_Buffer;
}

PY_STRING PY_STRING::format(const char * str, ...)
{
    PY_STRING   s;
    va_list     list;
    int         bytes_num_req;
    char *      buffer = NULL;

    if (str == NULL)
        return s;

    va_start(list, str);
    bytes_num_req = vsnprintf(buffer, 0, str, list);
    va_end(list);

    if (bytes_num_req == 0)
        return s;
    bytes_num_req ++;

    buffer = (char *) malloc(bytes_num_req);
    if (buffer == NULL)
        throw _py_OSError(STR_ERR_CAN_NOT_ALLOCATE_MEMORY);

    va_start(list, str);
    vsnprintf(buffer, bytes_num_req, str, list);
    va_end(list);

    s = buffer;
    free(buffer);

    return s;
}

PY_STRING PY_STRING::join(LIST_PY_STRING & list)
{
    PY_STRING           s;
    char *              delimiter;
    int                 delimiter_length;
    int                 bytes_num_req;
    LIST_PY_STRING_IT   it;
    char *              buffer;

    delimiter = m_Buffer;
    delimiter_length = delimiter == NULL ? 0 : strlen(delimiter);

    bytes_num_req = 0;
    for (it = list.begin();
         it != list.end();
         it ++)
        bytes_num_req += strlen(*it) + delimiter_length;
    if (bytes_num_req > 0)
        bytes_num_req -= delimiter_length;      // Delimiter after last string is not needed
    if (bytes_num_req > 0)
        bytes_num_req ++;                       // Add for '\0'

    buffer = (char *) malloc(bytes_num_req);
    if (buffer == NULL)
        throw _py_OSError(STR_ERR_CAN_NOT_ALLOCATE_MEMORY);

    bytes_num_req = 0;
    for (it = list.begin();
         it != list.end();
         it ++)
    {
        if (it != list.begin())
        {
            if (delimiter_length > 0)
                strcpy(buffer + bytes_num_req, delimiter);
            bytes_num_req += delimiter_length;
        }
        strcpy(buffer + bytes_num_req, *it);
        bytes_num_req += strlen(*it);
    }

    s = buffer;
    free(buffer);

    return s;
}

PY_STRING PY_STRING::lower(void)
{
    PY_STRING   s;
    char *      str;
    char *      str_new;

    if (m_Buffer == NULL)
        return s;

    // Test if string will be changed
    for (str = m_Buffer; *str != '\0'; str ++)
        if (*str != tolower(*str))
            break;
    if (*str == '\0')
    {
        s = *this;
        return s;
    }

    str_new = strdup(m_Buffer);
    if (str_new == NULL)
        throw _py_OSError();

    for (str = str_new; *str != '\0'; str ++)
        *str = tolower(*str);

    s = str_new;
    free(str_new);

    return s;
}

PY_STRING PY_STRING::rstrip(void)
{
    PY_STRING   s;
    int         chars_num;
    char *      str;
    char *      str_new;


    if (m_Buffer == NULL)
        return s;

    chars_num = strlen(m_Buffer);
    if (chars_num == 0)
        return s;

    // Test if string will be changed
    for (str = m_Buffer + chars_num - 1; (*str <= ' ') && (str != m_Buffer); str --);
    if (str == m_Buffer + chars_num - 1)
    {
        s = *this;
        return s;
    }

    str_new = strdup(m_Buffer);
    if (str_new == NULL)
        throw _py_OSError();

    for (str = str_new + chars_num; (*str <= ' ') && (str != str_new); *str = '\0', str --);

    s = str_new;
    free(str_new);

    return s;
}

VECTOR_PY_STRING PY_STRING::split(void)
{
    VECTOR_PY_STRING    v;
    char *              str_s;
    char *              str_e;
    int                 ch;

    if (m_Buffer == NULL)
        return v;

    str_s = m_Buffer;
    while (1)
    {
        // Find start of string
        for (; (*str_s <= ' ') && (*str_s != '\0'); str_s ++);
        if (*str_s == '\0')
            break;
        // Find end of string
        for (str_e = str_s + 1; (*str_e > ' ') && (*str_e != '\0'); str_e ++);
        ch = -1;
        if (*str_e != '\0')
        {
            ch = *str_e;
            *str_e = '\0';
        }
        v.push_back(PY_STRING(str_s));
        if (ch != -1)
            *str_e = ch;
        str_s = str_e;
    }
    return v;
}

VECTOR_PY_STRING PY_STRING::split(char delimiter)
{
    VECTOR_PY_STRING    v;
    char *              str_s;
    char *              str_e;
    int                 ch;

    if (m_Buffer == NULL)
        return v;

    str_s = m_Buffer;
    while (1)
    {
        // Find end of string
        for (str_e = str_s; (*str_e != delimiter) && (*str_e != '\0'); str_e ++);
        ch = -1;
        if (*str_e != '\0')
        {
            ch = *str_e;
            *str_e = '\0';
        }
        v.push_back(PY_STRING(str_s));
        if (ch != -1)
            *str_e = ch;

        if (*str_e == '\0')
            break;
        str_s = str_e + 1;
    }
    return v;
}

bool PY_STRING::starts_with(const char * str)
{
    if (m_Buffer == NULL)
        return false;
    if ((str == NULL) || (*str == '\0'))
        return false;
    return (0 > strcmp(str, m_Buffer));
}

PY_STRING PY_STRING::string_after(const char * str)
{
    PY_STRING   s;
    char *      str_s;

    if (m_Buffer == NULL)
        return s;

    str_s = ::strstr(m_Buffer, str);
    if (str_s == NULL)
        return s;
    str_s += strlen(str);

    s = str_s;
    return s;
}

PY_STRING PY_STRING::strip(void)
{
    PY_STRING           s;
    int                 chars_num;
    char *              str;
    char *              str_new;

    if (m_Buffer == NULL)
        return s;

    chars_num = strlen(m_Buffer);
    if (chars_num == 0)
        return s;

    // Test if string will be changed
    for (str = m_Buffer; (*str <= ' ') && (*str != '\0'); str++);
    if (str == m_Buffer)
    {
        for (str = m_Buffer + chars_num - 1; (*str <= ' ') && (str != m_Buffer); str --);
        if (str == m_Buffer + chars_num - 1)
        {
            s = *this;
            return s;
        }
    }

    str_new = strdup(m_Buffer);
    if (str_new == NULL)
        throw _py_OSError();

    for (str = str_new; (*str <= ' ') && (*str != '\0'); str++);
    if (str != str_new)
    {
        chars_num -= str - str_new;
        memmove(str_new, str, chars_num + 1);
    }
    for (str = str_new + chars_num; (*str <= ' ') && (str != str_new); *str = '\0', str --);

    s = str_new;
    free(str_new);

    return s;
}

char * PY_STRING::strstr(const char * str)
{
    return ::strstr(m_Buffer, str);
}

//
// PY_FILE
//

PY_FILE::PY_FILE(void)
    : m_File(NULL)
{
}

PY_FILE::PY_FILE(const PY_FILE & other)
    : m_File(NULL)
{
    throw _py_IOError("PY_FILE copy constructor");
}

PY_FILE::~PY_FILE()
{
    close();
}

PY_FILE & PY_FILE::operator=(const PY_FILE & rs)
{
    throw _py_IOError("PY_FILE::operator=");
}

void PY_FILE::open(const char * filename)
{
    open(filename, "r");
}

void PY_FILE::open(const char * filename, const char * mode)
{
    m_File = fopen(filename, mode);
    if (m_File == NULL)
        throw _py_IOError(strerror(errno));
}

PY_STRING PY_FILE::read(void)
{
    char *      buffer = NULL;
    PY_STRING   s;
    long        offset;
    struct stat st;
    int         can_read;

    if (m_File == NULL)
        goto FNC_EXIT_ERR;

    if (0 != fstat(fileno(m_File), &st))
        goto FNC_EXIT_ERR;
    offset = ftell(m_File);
    if (offset < 0)
        goto FNC_EXIT_ERR;

    can_read = st.st_size - offset;
    if (can_read < 1)
        goto FNC_EXIT_ERR;
    if (can_read > 4 * 1024)
        can_read = 4 * 1024;

    buffer = (char *) malloc(can_read + 1);
    if (buffer == NULL)
        goto FNC_EXIT_ERR;

    can_read = fread(buffer, 1, can_read, m_File);
    if (can_read <= 0)
        goto FNC_EXIT_ERR;
    *(buffer + can_read) = '\0';

    s = buffer;
    free(buffer);

    return s;

FNC_EXIT_ERR:
    if (buffer != NULL)
        free(buffer);
    throw _py_IOError();
}

PY_STRING PY_FILE::readline(void)
{
    char *      line = NULL;
    PY_STRING   s;

    if (m_File == NULL)
        goto FNC_EXIT_ERR;

    line = (char *)malloc(4 * 1024 + 1);
    if (line == NULL)
        goto FNC_EXIT_ERR;

    if (NULL == fgets(line, 4 * 1024 + 1, m_File))
    {
        if (0 != feof(m_File))
            goto FNC_EXIT_ERR;
    }
    else
        s = line;
    free(line);

    return s;

FNC_EXIT_ERR:
    if (line != NULL)
        free(line);
    throw _py_IOError();
}

LIST_PY_STRING PY_FILE::readlines(void)
{
    LIST_PY_STRING  ret;
    PY_STRING       s;

    while ((s = readline()) != NULL)
        ret.push_back(s);

    return ret;
}

void PY_FILE::write(const char * str)
{
    if (m_File == NULL)
        goto FNC_EXIT_ERR;

    if (0 > fprintf(m_File, "%s", str))
        goto FNC_EXIT_ERR;

    return;

FNC_EXIT_ERR:
    throw _py_IOError();
}

void PY_FILE::close(void)
{
    if (m_File != NULL)
        fclose(m_File);
    m_File = NULL;
}

bool PY_FILE::isopen(void)
{
    return (m_File != NULL);
}

//
// _py_x() functions
//

void _py_sys_exit(void)
{
    throw _py_SystemExit();
}

void _py_sys_exit(int code)
{
    throw _py_SystemExit(code);
}

LIST_PY_STRING _py_os_listdir(const char * dirname)
{
    DIR *           dir;
    struct dirent * dir_ent;
    LIST_PY_STRING  list;

    dir = opendir(dirname);
    if (dir == NULL)
        throw _py_OSError(strerror(errno));

    while (NULL != (dir_ent = readdir(dir)))
    {
        if (0 == strcmp(dir_ent->d_name, "."))
            continue;
        if (0 == strcmp(dir_ent->d_name, ".."))
            continue;
        list.push_back(PY_STRING(dir_ent->d_name));
    }
    closedir(dir);

    return list;
}

void _py_os_mkdir(const char * dirname)
{
    if (0 != mkdir(dirname, 0777))
        throw _py_OSError(strerror(errno));
}

void _py_os_rmdir(const char * dirname)
{
    if (0 != rmdir(dirname))
        throw _py_OSError(strerror(errno));
}

int _py_os_system(const char * cmd)
{
    return system(cmd);
}

void _py_os_unlink(const char * pathname)
{
    if (0 != unlink(pathname))
        throw _py_OSError(strerror(errno));
}

int _py_os_major(const char * devname)
{
    struct stat st;

    if (0 != stat(devname, &st))
        return -1;
    if (S_ISREG(st.st_mode))
        return major(st.st_dev);
    return major(st.st_rdev);
}

void _py_os_makedirs(const char * pathname)
{
    int     ret = -1;
    char *  buffer = NULL;

    buffer = (char *) malloc(16 + strlen(pathname));
    if (buffer == NULL)
        goto FNC_EXIT;

    sprintf(buffer, "mkdir -p %s", pathname);

    if (0 == system(buffer))
        ret = 0;

FNC_EXIT:
    if (buffer != NULL)
        free(buffer);
    if (ret != 0)
        throw _py_OSError();
}

void _py_os_makedirs(const char * pathname, int mode)
{
    int     ret = -1;
    char *  buffer = NULL;

    buffer = (char *) malloc(24 + strlen(pathname));
    if (buffer == NULL)
        goto FNC_EXIT;

    sprintf(buffer, "mkdir -m %04o -p %s", mode, pathname);

    if (0 == system(buffer))
        ret = 0;

FNC_EXIT:
    if (buffer != NULL)
        free(buffer);
    if (ret != 0)
        throw _py_OSError();
}

bool _py_os_path_isdir(char * pathname)
{
    struct stat st;

    if (0 != stat(pathname, &st))
        return false;
    if (S_ISDIR(st.st_mode))
        return true;
    return false;
}

bool _py_os_path_isfile(char * pathname)
{
    struct stat st;

    if (0 != stat(pathname, &st))
        return false;
    if (S_ISREG(st.st_mode))
        return true;
    return false;
}

bool _py_os_path_islink(char * pathname)
{
    struct stat st;

    if (0 != stat(pathname, &st))
        return false;
    if (S_ISLNK(st.st_mode))
        return true;
    return false;
}

PY_STRING _py_uuid_uuid4(void)
{
    PY_STRING   s;
    char        buffer[48];
    uuid_t      uuid;

    uuid_generate(uuid);
    uuid_unparse_lower(uuid, buffer);
    s = buffer;

    return s;
}
