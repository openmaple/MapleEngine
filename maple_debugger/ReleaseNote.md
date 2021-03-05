```
# Copyright (C) [2021] Futurewei Technologies, Inc. All rights reserved.
#
# Licensed under the Mulan Permissive Software License v2.
# You can use this software according to the terms and conditions of the MulanPSL - 2.0.
# You may obtain a copy of MulanPSL - 2.0 at:
#
#   https://opensource.org/licenses/MulanPSL-2.0
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
# FIT FOR A PARTICULAR PURPOSE.
# See the MulanPSL - 2.0 for more deta
```
## Version 1.2
*__Date: 2020/09/21__*

* Added _mstep_ command to step program to a different source line
* Added _mnext_ command to step program through subroutine calls
* Added enhancement to _mlist_ command to support more list modes

## Version 1.1
*__Date: 2020/09/07__*

* Added _msymbol_ command to search and display Maple symbol
* Added _mfinish_ command to execute until selected Maple frame returns
* Added color display support for Maple commands
* Added new option -c to _mstepi_ command to display current opcode count
* Extend mtype command to display method list for a matching class
* Significant performance improvement and optimization

## Version 1.0
*__Date: 2020/08/25__*

* First release of Maple Debugger
* Initial Maple Debugger commands
   * _mbreakpoint_, alias as _mb_
   * _mbacktrace_, alias as _mbt_
   * _mup_,
   * _mdown_,
   * _msrcpath_, alias as _msp_
   * _mlist_,
   * _mlocal_,
   * _mprint_,
   * _mtype_,
   * _mstepi_, alias as _msi_
   * _mnexti_, alias as _mni_
   * _mset_,
   * _mhelp
* README.md for preparation and launch using Maple Debugger
* UserReference.md for Maple Debugger command detailed information
* .mgdbinit for Maple Debugger default configuration
