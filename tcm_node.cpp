//
// Copyright (c) 2015 Radovan Augustin, rado_augustin@yahoo.com
// All rights reserved.
//
// This file is part of tcm_node-cpp, licensed under BSD 3-Clause License.
//

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "_py.h"
#include "tcm_modules.h"

static PY_STRING tcm_root = "/sys/kernel/config/target/core";

//
// Forward declarations
//

static PY_STRING    tcm_get_unit_serial     (char * dev_path);
static void         __tcm_freevirtdev       (char * dev_path);
static void         tcm_set_wwn_unit_serial (char * dev_path, char * unit_serial);

//
// Functions
//

static void tcm_err(char * msg)
{
    fprintf(stderr, "%s" "\n", msg);
    _py_sys_exit(1);
}

static PY_STRING tcm_read(char * filename)
{
    PY_STRING   s;
    PY_FILE     f;

    try
    {
        f.open(filename);
        s = f.read();
        f.close();
    }
    catch (_py_IOError const & e)
    {
        tcm_err(PY_STRING().format("%s %s\n%s", filename, e.what(), "Is kernel module loaded?"));
    }
    return s;
}

static void tcm_write(char * filename, char * value, bool newline = true)
{
    PY_FILE f;

    try
    {
        f.open(filename, "w");
        f.write(value);
        if (newline)
            f.write("\n");
        f.close();
    }
    catch (_py_IOError const & e)
    {
        tcm_err(PY_STRING().format("%s %s\n%s", filename, e.what(), "Is kernel module loaded?"));
    }
}

static PY_STRING tcm_full_path(char * arg)
{
    return tcm_root + "/" + arg;
}

static void tcm_check_dev_exists(char * dev_path)
{
    PY_STRING full_path;

    full_path = tcm_full_path(dev_path);
    if (!_py_os_path_isdir(full_path))
        tcm_err(PY_STRING("TCM/ConfigFS storage object does not exist: ") + full_path);
}

static void tcm_alua_check_metadata_dir(char * dev_path)
{
    PY_STRING alua_path;

    alua_path = PY_STRING("/var/target/alua/tpgs_") + tcm_get_unit_serial(dev_path) + "/";
    if (_py_os_path_isdir(alua_path))
        return;

    _py_os_makedirs(alua_path);
}

static void tcm_alua_process_metadata(char * dev_path, char * gp_name, char * gp_id)
{
    PY_STRING alua_gp_path;    
    PY_STRING alua_md_path;

    alua_gp_path = tcm_full_path(dev_path) + "/alua/" + gp_name;
    alua_md_path = PY_STRING("/var/target/alua/tpgs_") + tcm_get_unit_serial(dev_path) + "/" + gp_name;

    if (!_py_os_path_isfile(alua_md_path))
    {
        tcm_write(alua_gp_path + "/alua_write_metadata", "1");
        return;
    }

    LIST_PY_STRING      lines;
    LIST_PY_STRING_IT   it;
    VECTOR_PY_STRING    items;
    MAP_PY_STRING       d;
    MAP_PY_STRING_IT    d_it;
    PY_STRING           s;
    PY_FILE             p;

    p.open(alua_md_path);
    lines = p.readlines();
    p.close();
    for (it = lines.begin();
         it != lines.end();
         it ++)
    {
        items = (*it).split('=');
        d[items[0].strip()] = items[1].strip();
    }

    d_it = d.find(PY_STRING("tg_pt_gp_id"));
    if ((d_it != d.end()) && (d_it->second != PY_STRING(gp_id)))
        throw _py_IOError(PY_STRING("").format("Passed tg_pt_gp_id: %s does not match extracted: %s", gp_id, (char *)d_it->second));

    d_it = d.find(PY_STRING("alua_access_state"));
    if (d_it != d.end())
         tcm_write(alua_gp_path + "/alua_access_state", d_it->second);

    d_it = d.find(PY_STRING("alua_access_status"));
    if (d_it != d.end())
        tcm_write(alua_gp_path + "/alua_access_status", d_it->second);

    tcm_write(alua_gp_path + "/alua_write_metadata", "1");
}

static void tcm_add_alua_tgptgp_with_md(char * dev_path, char * gp_name, char * gp_id)
{
    PY_STRING alua_gp_path;

    alua_gp_path = tcm_full_path(dev_path) + "/alua/" + gp_name;

    tcm_check_dev_exists(dev_path);

    if ((PY_STRING(gp_name) == "default_tg_pt_gp") && (PY_STRING(gp_id) == "0"))
    {
        tcm_alua_process_metadata(dev_path, gp_name, gp_id);
        return;
    }

    _py_os_makedirs(alua_gp_path);

    try
    {
        tcm_write(alua_gp_path + "/tg_pt_gp_id", gp_id);
    }
    catch (...)
    {
        _py_os_rmdir(alua_gp_path);
        throw;
    }

    tcm_alua_process_metadata(dev_path, gp_name, gp_id);
}

static void tcm_delhba(char * hba_name)
{
    PY_STRING hba_path;

    hba_path = tcm_full_path(hba_name);

    LIST_PY_STRING      gs;
    LIST_PY_STRING_IT   gs_it;

    gs = _py_os_listdir(hba_path);
    for (gs_it = gs.begin();
         gs_it != gs.end();
         gs_it ++)
    {
        if ((*gs_it == PY_STRING("hba_info")) ||
            (*gs_it == PY_STRING("hba_mode")))
            continue;
        __tcm_freevirtdev(PY_STRING(hba_name) + "/" + *gs_it);
    }

    _py_os_rmdir(hba_path);
}

static void tcm_del_alua_lugp(char * lu_gp_name)
{
    if (!_py_os_path_isdir(tcm_root + "/alua/lu_gps/" + lu_gp_name))
        tcm_err(PY_STRING("ALUA Logical Unit Group: ") + lu_gp_name + " does not exist!");

    _py_os_rmdir(tcm_root + "/alua/lu_gps/" + lu_gp_name);
}

static void __tcm_del_alua_tgptgp(char * dev_path, char * gp_name)
{
    PY_STRING full_path;

    tcm_check_dev_exists(dev_path);

    full_path = tcm_full_path(dev_path);

    if (!_py_os_path_isdir(full_path + "/alua/" + gp_name))
        tcm_err(PY_STRING("ALUA Target Port Group: ") + gp_name + " does not exist!");

    _py_os_rmdir(full_path + "/alua/" + gp_name);
}

static void tcm_generate_uuid_for_unit_serial(char * dev_path)
{
    tcm_set_wwn_unit_serial(dev_path, _py_uuid_uuid4());
}

static void tcm_createvirtdev(char * dev_path, char * plugin_params, bool establishdev = false)
{
    VECTOR_PY_STRING    parts;
    PY_STRING           hba_path;
    PY_STRING           hba_full_path;
    PY_STRING           full_path;
    bool                gen_uuid;

    parts = PY_STRING(dev_path).split('/');
    if (0 < parts.size())
        hba_path = parts[0];

    hba_full_path = tcm_full_path(hba_path);
    if (!_py_os_path_isdir(hba_full_path))
        _py_os_mkdir(hba_full_path);

    full_path = tcm_full_path(dev_path);
    if (_py_os_path_isdir(full_path))
        tcm_err(PY_STRING("TCM/ConfigFS storage object already exists: ") + full_path);
    else
        _py_os_mkdir(full_path);

    gen_uuid = !establishdev;

    for (TCM_MODULE * tcm = tcm_modules;
         tcm->name != NULL;
         tcm ++)
    {
        if (!hba_path.starts_with(PY_STRING(tcm->name) + "_"))
            continue;
        try
        {
            if (tcm->fnc_createvirtdev != NULL)
                tcm->fnc_createvirtdev(dev_path, plugin_params);
            else
                tcm_err(PY_STRING("no module for ") + tcm->name);
        }
        catch (...)
        {
            _py_os_rmdir(full_path);
            printf("%s\n", (char *)(PY_STRING("Unable to register TCM/ConfigFS storage object: ") + full_path));
            throw;
        }

        printf("%s" "\n", (char *)tcm_read(full_path + "/info"));

        if (tcm->gen_uuid && gen_uuid)
        {
            tcm_generate_uuid_for_unit_serial(dev_path);
            tcm_alua_check_metadata_dir(dev_path);
        }
        break;
    }
}

static PY_STRING tcm_get_unit_serial(char * dev_path)
{
    PY_STRING string;

    string = tcm_read(tcm_full_path(dev_path) + "/wwn/vpd_unit_serial");
    return string.split(':')[1].strip();
}

static void tcm_process_aptpl_metadata(char * dev_path)
{
    PY_STRING               full_path;
    PY_STRING               aptpl_file;
    VECTOR_PY_STRING        lines;
    VECTOR_PY_STRING_IT     lines_it;
    LIST_PY_STRING          res_list;
    LIST_LIST_PY_STRING     reservations;
    LIST_LIST_PY_STRING_IT  reservations_it;

    tcm_check_dev_exists(dev_path);

    full_path = tcm_full_path(dev_path);

    aptpl_file = PY_STRING("/var/target/pr/aptpl_") + tcm_get_unit_serial(dev_path);
    if (!_py_os_path_isfile(aptpl_file))
        return;

    lines = tcm_read(aptpl_file).split();

    if (lines[0].starts_with("PR_REG_START:"))
        return;

    for (lines_it = lines.begin();
         lines_it != lines.end();
         lines_it ++)
    {
        if ((*lines_it).starts_with("PR_REG_START:"))
            res_list.clear();
        else
        if ((*lines_it).starts_with("PR_REG_END:"))
            reservations.push_back(res_list);
        else
            res_list.push_back((*lines_it).strip());
    }

    for (reservations_it = reservations.begin();
         reservations_it != reservations.end();
         reservations_it ++)
    {
        tcm_write(full_path + "/pr/res_aptpl_metadata", PY_STRING(",").join(*reservations_it));
    }
}

static void tcm_establishvirtdev(char * dev_path, char * plugin_params)
{
    tcm_createvirtdev(dev_path, plugin_params, true);
}

static void __tcm_freevirtdev(char * dev_path)
{
    PY_STRING           full_path;
    LIST_PY_STRING      tg_pt_gps;
    LIST_PY_STRING_IT   tg_pt_gps_it;

    tcm_check_dev_exists(dev_path);

    full_path = tcm_full_path(dev_path);

    tg_pt_gps = _py_os_listdir(full_path + "/alua/");
    for (tg_pt_gps_it = tg_pt_gps.begin();
         tg_pt_gps_it != tg_pt_gps.end();
         tg_pt_gps_it ++)
    {
        if (*tg_pt_gps_it == "default_tg_pt_gp")
            continue;
        __tcm_del_alua_tgptgp(dev_path, *tg_pt_gps_it);
    }

    _py_os_rmdir(full_path);
}

static void tcm_set_wwn_unit_serial(char * dev_path, char * unit_serial)
{
    tcm_check_dev_exists(dev_path);
    tcm_write(tcm_full_path(dev_path) + "/wwn/vpd_unit_serial", unit_serial);
}

static void tcm_set_wwn_unit_serial_with_md(char * dev_path, char * unit_serial)
{
    tcm_check_dev_exists(dev_path);
    tcm_set_wwn_unit_serial(dev_path, unit_serial);
    tcm_process_aptpl_metadata(dev_path);
    tcm_alua_check_metadata_dir(dev_path);
}

static void tcm_unload(void)
{
    if (!_py_os_path_isdir(tcm_root))
        tcm_err(PY_STRING("Unable to access tcm_root: ") + tcm_root);

    LIST_PY_STRING      hba_root;
    LIST_PY_STRING_IT   hba_root_it;

    hba_root = _py_os_listdir(tcm_root);
    for (hba_root_it = hba_root.begin();
         hba_root_it != hba_root.end();
         hba_root_it ++)
    {
        if (*hba_root_it == "alua")
            continue;
        tcm_delhba(*hba_root_it);
    }

    LIST_PY_STRING      lu_gps;
    LIST_PY_STRING_IT   lu_gps_it;

    lu_gps = _py_os_listdir(tcm_root + "/alua/lu_gps");
    for (lu_gps_it = lu_gps.begin();
         lu_gps_it != lu_gps.end();
         lu_gps_it ++)
    {
        if (*lu_gps_it == "default_lu_gp")
            continue;
        tcm_del_alua_lugp(*lu_gps_it);
    }

    _py_os_system("rmmod target_core_iblock");
    _py_os_system("rmmod target_core_file");
    _py_os_system("rmmod target_core_pscsi");
    _py_os_system("rmmod target_core_stgt");

    if (0 != _py_os_system("rmmod target_core_mod"))
        tcm_err("Unable to rmmod target_core_mod");
}

static void tcm_version(void)
{
    printf("%s\n", (char *)(tcm_read("/sys/kernel/config/target/version").strip()));
}

//
// Callback dispatcher
//

enum {
    CID_TCM_ADD_ALUA_TGPTGP_WITH_MD,
    CID_TCM_ESTABLISHVIRTDEV,
    CID_TCM_SET_WWN_UNIT_SERIAL_WITH_MD,
    CID_TCM_UNLOAD,
    CID_TCM_VERSION
};

static void arg_callback(int cid, int argc_req, int * argc, char *** argv)
{
    int     _argc = *argc;
    char ** _argv = *argv;

    if (argc_req > _argc)
    {
        printf("ERROR: Not enough parameters\n");
        return;
    }

    switch (cid)
    {
        case CID_TCM_ADD_ALUA_TGPTGP_WITH_MD:
            tcm_add_alua_tgptgp_with_md(_argv[0], _argv[1], _argv[2]);
            break;
        case CID_TCM_ESTABLISHVIRTDEV:
            tcm_establishvirtdev(_argv[0], _argv[1]);
            break;
        case CID_TCM_SET_WWN_UNIT_SERIAL_WITH_MD:
            tcm_set_wwn_unit_serial_with_md(_argv[0], _argv[1]);
            break;
        case CID_TCM_UNLOAD:
            tcm_unload();
            break;
        case CID_TCM_VERSION:
            tcm_version();
            break;

        default:
            printf("INFO: Argument callback not implemented\n");
            break;
    }
    *argc -= argc_req;
    *argv += argc_req;
}

//
// Main
//

int main(int argc, char *argv[])
{
    int *       pargc = &argc;
    char ***    pargv = &argv;
    int         idx;
    int         status = 0;

    try
    {
        // Process command line arguments
        (*pargc) --;
        (*pargv) ++;
        for (idx = 0; idx < argc; idx ++)
        {
            (*pargc) --;
            (*pargv) ++;

            if ((0 == strcmp(*(argv - 1), "--addtpgtpgwithmd")) ||
                (0 == strcmp(*(argv - 1), "--addaluatpgwithmd")))
            {
                arg_callback(CID_TCM_ADD_ALUA_TGPTGP_WITH_MD, 3, pargc, pargv);
                continue;
            }
            if (0 == strcmp(*(argv - 1), "--establishdev"))
            {
                arg_callback(CID_TCM_ESTABLISHVIRTDEV, 2, pargc, pargv);
                continue;
            }
            if (0 == strcmp(*(argv - 1), "--setunitserialwithmd"))
            {
                arg_callback(CID_TCM_SET_WWN_UNIT_SERIAL_WITH_MD, 2, pargc, pargv);
                continue;
            }
            if (0 == strcmp(*(argv - 1), "--unload"))
            {
                arg_callback(CID_TCM_UNLOAD, 0, pargc, pargv);
                continue;
            }
            if (0 == strcmp(*(argv - 1), "--version"))
            {
                arg_callback(CID_TCM_VERSION, 0, pargc, pargv);
                continue;
            }
        }
    }
    catch (_py_SystemExit const & e)
    {
        status = e.code();
    }
    catch (std::exception const & e)
    {
        printf("Exception: %s\n", e.what());
        status = 1;
    }

    return status;
}
