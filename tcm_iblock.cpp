//
// Copyright (c) 2015 Radovan Augustin, rado_augustin@yahoo.com
// All rights reserved.
//
// This file is part of tcm_node-cpp, licensed under BSD 3-Clause License.
//

#include "_py.h"

static PY_STRING tcm_root = "/sys/kernel/config/target/core";

int iblock_createvirtdev(char * path, char * params)
{
    PY_STRING   cfs_path;
    PY_STRING   udev_path;
    PY_STRING   set_udev_path_op;
    PY_STRING   control_opt;
    PY_STRING   enable_opt;
    int         major;

    printf("%s" "\n", (char *)(PY_STRING("Calling iblock createvirtdev: path ") + path));

    cfs_path = tcm_root + "/" + path + "/";
//    printf("%s" "\n", (char *)(PY_STRING("Calling iblock createvirtdev: params ") + params));

    udev_path = PY_STRING(params).strip();
    if (!udev_path.starts_with("/dev/"))
    {
        printf("IBLOCK: Please reference a valid /dev/ block_device" "\n");
        return -1;
    }

    major = _py_os_major(udev_path);

    if (major == 11)
    {
        printf("Unable to export Linux/SCSI TYPE_CDROM from IBLOCK, please use pSCSI export" "\n");
        return -1;
    }
    if (major == 22)
    {
        printf("Unable to export IDE CDROM from IBLOCK" "\n");
        return -1;
    }

    set_udev_path_op = PY_STRING("echo -n ") + udev_path + " > " + cfs_path + "udev_path";
    if (0 != _py_os_system(set_udev_path_op))
    {
        printf("%s" "\n", (char *)(PY_STRING("IBLOCK: Unable to set udev_path in ") + cfs_path + " for: " + udev_path));
        return -1;
    }

    control_opt = PY_STRING("echo -n udev_path=") + udev_path + " > " + cfs_path + "control";
    if (0 != _py_os_system(control_opt))
    {
        printf("%s" "\n", (char *)(PY_STRING("IBLOCK: createvirtdev failed for control_opt with ") + control_opt));
        return -1;
    }
    enable_opt = PY_STRING("echo 1 > ") +  cfs_path + "enable";
    if (0 != _py_os_system(enable_opt))
    {
        printf("%s" "\n", (char *)(PY_STRING("IBLOCK: createvirtdev failed for enable_opt with ") + enable_opt));
        return -1;
    }
    return 0;
}
