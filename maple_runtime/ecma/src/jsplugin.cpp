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
#include <cstdint>
#include "jsplugin.h"
#include <cstdio>
#include <unistd.h>
#include <string>
#include <dlfcn.h>
#include <cstring>
#include "vmmemory.h"

static void GetCurrentDir(const char *name, std::string &newDir) {
  uint32_t len = strlen(name);
  int32_t i;
  for (i = len - 1; i >= 0; i--) {
    if (name[i] == '/')
      break;
  }
  assert(i >= 0 && "error");
  newDir.append(name, 0, i);
}


uint32_t JsPlugin::GetIndexForSo (char *name) {
  uint32_t len = strlen(name);
  int32_t i;
  for (i = len - 1; i >= 0; i--) {
    if (name[i] == '/')
      break;
  }
  return i >= 0 ? (i + 1) : 0;
}

JsFileInforNode *JsPlugin::LoadRequiredFile(char *name, bool &iscached) {
  char *soName = name + GetIndexForSo(name);
  JsFileInforNode *jsfileinfo = FindJsFile(soName);
  if (!jsfileinfo) {
    // std::string file_name(name);
    // std::string file_name_save(oldIS->fileName);
    // oldIS->fileName = file_name;  // put name of to-be-read file in module
    // MIRParser theparser(*mirModule);
    //if (!theparser.ParseMIR(++file_index_)) {
    //  theparser.EmitError(file_name);
    //  return NULL;
    //}
    void *handle = nullptr;
    std::string soPath(name);
    std::string newPath("");
    std::string newDir("");
    if (soPath.size() >= 1 && soPath.at(0) != 47) {
      // this is a absolute path
      char *filePath = get_current_dir_name();
      newPath.append(filePath);
      newPath.append("/");
      newPath.append(soPath);
      GetCurrentDir(newPath.c_str(),newDir);
      free(filePath);
      handle = dlopen(newPath.c_str(), RTLD_NOW);
    } else {
      GetCurrentDir(name,newDir);
      handle = dlopen(name, RTLD_NOW);
    }
    if (chdir(newDir.c_str()) != 0) {
      fprintf(stderr, "failed to change dir in plugin LoadRequiredFile %s", newDir.c_str());
      exit(1);
    }
    if (!handle) {
      fprintf(stderr, "failed to open %s\n", name);
      exit(1);
    }
    uint16_t *mpljsMdD = (uint16_t *)dlsym(handle, "__mpljs_module_decl__");
    if (!mpljsMdD) {
      fprintf(stderr, "failed to open __mpljs_module_decl__ %s\n", name);
      exit(1);
    }
    void *mainFn = dlsym(handle, "__jsmain");
    if (!mainFn) {
      fprintf(stderr, "failed to open __jsmain for plugin %s", name);
      exit(1);
    }

    uint16_t globalMemSize = mpljsMdD[0];
    uint8_t *newGp = (uint8_t *)malloc(globalMemSize);
    memcpy(newGp, mpljsMdD + 1, globalMemSize);
    // uint8_t *newTopgp = newGp + globalMemSize;
    // mirModule->fileName = file_name_save;  // restore original file name
    jsfileinfo = (JsFileInforNode *)VMMallocNOGC(sizeof(JsFileInforNode));
    jsfileinfo->Init(soName, ++fileIndex, newGp, globalMemSize, (uint8_t *)mainFn + 4, NULL);
    InsertJsFile(jsfileinfo);
    iscached = false;
  } else {
    iscached = true;
  }
  return jsfileinfo;
}


JsFileInforNode *JsPlugin::FindJsFile(char *name) {
  JsFileInforNode *node = mainFileInfo;
  for (; node; node = node->next) {
    if (!strcmp(node->fileName, name)) {
      return node;
    }
  }
  return NULL;
}

JsFileInforNode *JsPlugin::FindJsFile(uint32_t index) {
  for (JsFileInforNode *node = mainFileInfo; node; node = node->next) {
    if (index == node->fileIndex)
      return node;
  }
  return NULL;
}



void JsPlugin::InsertJsFile(JsFileInforNode *node) {
  node->next = mainFileInfo->next;
  mainFileInfo->next = node;
}
