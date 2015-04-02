//
// Copyright (c) 2015 Radovan Augustin, rado_augustin@yahoo.com
// All rights reserved.
//
// This file is part of tcm_node-cpp, licensed under BSD 3-Clause License.
//

#ifndef _TCM_MODULES_H_
#define _TCM_MODULES_H_ 1

#include "_py.h"

typedef int (* FNC_CREATEVIRTDEV)(char * path, char * params);

typedef struct
{
    char *              name;
    FNC_CREATEVIRTDEV   fnc_createvirtdev;
    bool                gen_uuid;
} TCM_MODULE;

extern TCM_MODULE       tcm_modules[];

#endif /* _TCM_MODULES_H_ */
