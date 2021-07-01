/*
 * Copyright (C) [2021] Futurewei Technologies, Inc. All rights reserved.
 *
 * OpenArkCompiler is licensed under the Mulan Permissive Software License v2.
 * You can use this software according to the terms and conditions of the MulanPSL - 2.0.
 * You may obtain a copy of MulanPSL - 2.0 at:
 *
 *   https://opensource.org/licenses/MulanPSL-2.0
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the MulanPSL - 2.0 for more details.
 */

#ifndef MAPLEENGIN_INCLUDE_JSPLUGIN_H_
#define MAPLEENGIN_INCLUDE_JSPLUGIN_H_
class InterSource;

struct JsFileInforNode {  // this is the file information
  char *fileName;
  uint32_t fileIndex;
  uint8_t *gp;
  uint32_t glbMemsize;
  void *mainFn;
  JsFileInforNode *next;

  void Init(char *name, uint32_t idx, uint8_t *newGp, uint32_t glbmemsize, void *mf, JsFileInforNode *next) {
    fileName = name;
    fileIndex = idx;
    gp = newGp;
    glbMemsize = glbmemsize;
    mainFn = mf;
    this->next = next;
  }
};


class JsPlugin {
 public:
  JsFileInforNode *mainFileInfo;  // the link list, pointing to the main file, suppose there wouldn't be much files
                                     // to be plugined, otherwise use hash table
  uint16_t fileIndex;
  JsFileInforNode *formalFileInfo;

 public:
  void Init(JsFileInforNode *node) {
    mainFileInfo = node;
    fileIndex = 0;
    formalFileInfo = nullptr;
  }

  JsFileInforNode *FindJsFile(char *);  // find by the name of the file
  JsFileInforNode *FindJsFile(uint32_t);  // find by the file index
  JsFileInforNode *LoadRequiredFile(char *, bool &);
  void InsertJsFile(JsFileInforNode *);
  uint32_t GetIndexForSo (char *name);
};
#endif // MAPLEENGIN_INCLUDE_JSPLUGIN_H_
