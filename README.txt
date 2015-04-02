
tcm_node-cpp is portion of tcm_node.py rewritten to C++.

Features:
    - supports iblock backend and iSCSI over IP frontend
    - can run with uClibc++ on embedded devices with OpenWrt

Utility tcm_node.py is from Linux-IO Target (LIO -TM-) lio-utils
(https://github.com/Datera/lio-utils). tcm_node-cpp is tested
with Linux kernel 3.18.8 and with scripts generated from targetcli.
