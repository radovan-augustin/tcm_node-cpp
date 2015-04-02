//
// Copyright (c) 2015 Radovan Augustin, rado_augustin@yahoo.com
// All rights reserved.
//
// This file is part of tcm_node-cpp, licensed under BSD 3-Clause License.
//

#include "tcm_modules.h"
#include "tcm_iblock.h"

TCM_MODULE tcm_modules[] =
{
    {"iblock",  iblock_createvirtdev,   true},
    {NULL,      NULL,                   false}
};
