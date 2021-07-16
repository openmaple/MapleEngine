#ifndef JSDATAVIEW_H
#define JSDATAVIEW_H
#include "jsvalue.h"
struct __jsarraybyte {
  __jsvalue length;
  uint8_t *arrayRaw;
};
struct __jsdataview {
  __jsarraybyte *arrayByte;
  uint32_t startIndex;
  uint32_t endIndex;
};

void __jsdataview_pt_setInt8(__jsvalue *thisArg, __jsvalue *argList, uint32_t nargs);
__jsvalue __jsdataview_pt_getInt8(__jsvalue *thisArg, __jsvalue *argList, uint32_t nargs);
void __jsdataview_pt_setUint8(__jsvalue *thisArg, __jsvalue *argList, uint32_t nargs);
__jsvalue __jsdataview_pt_getUint8(__jsvalue *thisArg, __jsvalue *argList, uint32_t nargs);
void __jsdataview_pt_setInt16(__jsvalue *thisArg, __jsvalue *argList, uint32_t nargs);
__jsvalue __jsdataview_pt_getInt16(__jsvalue *thisArg, __jsvalue *argList, uint32_t nargs);
void __jsdataview_pt_setUint16(__jsvalue *thisArg, __jsvalue *argList, uint32_t nargs);
__jsvalue __jsdataview_pt_getUint16(__jsvalue *thisArg, __jsvalue *argList, uint32_t nargs);
void __jsdataview_pt_setInt32(__jsvalue *thisArg, __jsvalue *argList, uint32_t nargs);
__jsvalue __jsdataview_pt_getInt32(__jsvalue *thisArg, __jsvalue *argList, uint32_t nargs);
void __jsdataview_pt_setUint32(__jsvalue *thisArg, __jsvalue *argList, uint32_t nargs);
__jsvalue __jsdataview_pt_getUint32(__jsvalue *thisArg, __jsvalue *argList, uint32_t nargs);
#endif // JSDATAVIEW_H
