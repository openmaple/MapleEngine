#include "jsdataview.h"
#include "jsobject.h"
#include "jsvalueinline.h"


void __jsdataview_pt_setInt8(__jsvalue *thisArg, __jsvalue *argList, uint32_t nargs) {
  __jsobject *obj = __jsval_to_object(thisArg);
  MAPLE_JS_ASSERT(obj->object_class == JSDATAVIEW);
  __jsdataview *dataView = obj->shared.dataView;
  uint32_t length = __jsval_to_number(&dataView->arrayByte->length);
  uint32_t byteOffset = __jsval_to_number(&argList[0]);
  MAPLE_JS_ASSERT(byteOffset < length);
  dataView->arrayByte->arrayRaw[byteOffset] = __jsval_to_number(&argList[1]);
}

static uint8_t *__jsdataview_pt_getArrayRaw(__jsvalue *thisArg, __jsvalue *argList, uint32_t nargs, uint32_t &byteOffset) {
  __jsobject *obj = __jsval_to_object(thisArg);
  MAPLE_JS_ASSERT(obj->object_class == JSDATAVIEW);
  __jsdataview *dataView = obj->shared.dataView;
  uint32_t length = __jsval_to_number(&dataView->arrayByte->length);
  byteOffset = __jsval_to_number(&argList[0]);
  MAPLE_JS_ASSERT(byteOffset < length);
  return dataView->arrayByte->arrayRaw;
}

__jsvalue __jsdataview_pt_getInt8(__jsvalue *thisArg, __jsvalue *argList, uint32_t nargs) {
  uint32_t byteOffset = 0;
  uint8_t *arrayRaw = __jsdataview_pt_getArrayRaw(thisArg, argList, nargs, byteOffset);
  return __number_value ((int32_t)(int8_t) arrayRaw[byteOffset]);
}


void __jsdataview_pt_setUint8(__jsvalue *thisArg, __jsvalue *argList, uint32_t nargs) {
  __jsdataview_pt_setInt8(thisArg, argList, nargs);
}

__jsvalue __jsdataview_pt_getUint8(__jsvalue *thisArg, __jsvalue *argList, uint32_t nargs) {
  uint32_t byteOffset = 0;
  uint8_t *arrayRaw = __jsdataview_pt_getArrayRaw(thisArg, argList, nargs, byteOffset);
  return __number_value ((int32_t)(uint8_t) arrayRaw[byteOffset]);
}


void __jsdataview_pt_setInt16(__jsvalue *thisArg, __jsvalue *argList, uint32_t nargs) {
  uint32_t byteOffset = 0;
  int16_t *arrayRaw = (int16_t *)__jsdataview_pt_getArrayRaw(thisArg, argList, nargs, byteOffset);
  arrayRaw[byteOffset] = __jsval_to_number(&argList[1]);
}

__jsvalue __jsdataview_pt_getInt16(__jsvalue *thisArg, __jsvalue *argList, uint32_t nargs) {
  uint32_t byteOffset = 0;
  int16_t *arrayRaw = (int16_t *)__jsdataview_pt_getArrayRaw(thisArg, argList, nargs, byteOffset);
  return __number_value ((int32_t)(int16_t) arrayRaw[byteOffset]);
}

void __jsdataview_pt_setUint16(__jsvalue *thisArg, __jsvalue *argList, uint32_t nargs) {
  uint32_t byteOffset = 0;
  uint16_t *arrayRaw = (uint16_t *)__jsdataview_pt_getArrayRaw(thisArg, argList, nargs, byteOffset);
  arrayRaw[byteOffset] = __jsval_to_number(&argList[1]);
}

__jsvalue __jsdataview_pt_getUint16(__jsvalue *thisArg, __jsvalue *argList, uint32_t nargs) {
  uint32_t byteOffset = 0;
  uint16_t *arrayRaw = (uint16_t *)__jsdataview_pt_getArrayRaw(thisArg, argList, nargs, byteOffset);
  return __number_value ((int32_t)(uint16_t) arrayRaw[byteOffset]);
}

void __jsdataview_pt_setInt32(__jsvalue *thisArg, __jsvalue *argList, uint32_t nargs) {
  uint32_t byteOffset = 0;
  int32_t *arrayRaw = (int32_t *)__jsdataview_pt_getArrayRaw(thisArg, argList, nargs, byteOffset);
  arrayRaw[byteOffset] = __jsval_to_number(&argList[1]);
}

__jsvalue __jsdataview_pt_getInt32(__jsvalue *thisArg, __jsvalue *argList, uint32_t nargs) {
  uint32_t byteOffset = 0;
  int32_t *arrayRaw = (int32_t *)__jsdataview_pt_getArrayRaw(thisArg, argList, nargs, byteOffset);
  return __number_value ((int32_t) arrayRaw[byteOffset]);
}

void __jsdataview_pt_setUint32(__jsvalue *thisArg, __jsvalue *argList, uint32_t nargs) {
  uint32_t byteOffset = 0;
  uint32_t *arrayRaw = (uint32_t *)__jsdataview_pt_getArrayRaw(thisArg, argList, nargs, byteOffset);
  arrayRaw[byteOffset] = __jsval_to_number(&argList[1]);
}

__jsvalue __jsdataview_pt_getUint32(__jsvalue *thisArg, __jsvalue *argList, uint32_t nargs) {
  uint32_t byteOffset = 0;
  uint32_t *arrayRaw = (uint32_t *)__jsdataview_pt_getArrayRaw(thisArg, argList, nargs, byteOffset);
  return __number_value ((int32_t)(uint32_t) arrayRaw[byteOffset]);
}
