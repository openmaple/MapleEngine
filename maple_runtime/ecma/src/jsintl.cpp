/*
 * Copyright (c) [2021] Futurewei Technologies Co.,Ltd.All rights reserved.
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

#include <unordered_map>
#include <regex>
#include <unicode/utypes.h>
#include <unicode/uenum.h>
#include <unicode/ucol.h>
#include <unicode/uloc.h>
#include <unicode/numsys.h>
#include "jsobject.h"
#include "jsobjectinline.h"
#include "jsarray.h"
#include "jsmath.h"
#include "jsintl.h"

std::string kUnicodeLocaleExtensionSequence = "-u(-[a-z0-9]{2,8})+";

std::unordered_map<std::string,std::string> kTagMappings = {
  {"art-lojban", "jbo"},
  {"cel-gaulish", "cel-gaulish"},
  {"en-gb-oed", "en-GB-oed"},
  {"i-ami", "ami"},
  {"i-bnn", "bnn"},
  {"i-default", "i-default"},
  {"i-enochian", "i-enochian"},
  {"i-hak", "hak"},
  {"i-klingon", "tlh"},
  {"i-lux", "lb"},
  {"i-mingo", "i-mingo"},
  {"i-navajo", "nv"},
  {"i-pwn", "pwn"},
  {"i-tao", "tao"},
  {"i-tay", "tay"},
  {"i-tsu", "tsu"},
  {"ja-latn-hepburn-heploc", "ja-Latn-alalc97"},
  {"no-bok", "nb"},
  {"no-nyn", "nn"},
  {"sgn-be-fr", "sfb"},
  {"sgn-be-nl", "vgt"},
  {"sgn-br", "bzs"},
  {"sgn-ch-de", "sgg"},
  {"sgn-co", "csn"},
  {"sgn-de", "gsg"},
  {"sgn-dk", "dsl"},
  {"sgn-es", "ssp"},
  {"sgn-fr", "fsl"},
  {"sgn-gb", "bfi"},
  {"sgn-gr", "gss"},
  {"sgn-ie", "isg"},
  {"sgn-it", "ise"},
  {"sgn-jp", "jsl"},
  {"sgn-mx", "mfs"},
  {"sgn-ni", "ncs"},
  {"sgn-nl", "dse"},
  {"sgn-no", "nsl"},
  {"sgn-pt", "psr"},
  {"sgn-se", "swl"},
  {"sgn-us", "ase"},
  {"sgn-za", "sfs"},
  {"zh-cmn", "cmn"},
  {"zh-cmn-hans", "cmn-Hans"},
  {"zh-cmn-hant", "cmn-Hant"},
  {"zh-gan", "gan"},
  {"zh-guoyu", "cmn"},
  {"zh-hakka", "hak"},
  {"zh-min", "zh-min"},
  {"zh-min-nan", "nan"},
  {"zh-wuu", "wuu"},
  {"zh-xiang", "hsn"},
  {"zh-yue", "yue"},
};

std::unordered_map<std::string,std::vector<std::string>> kExtLangMappings = {
  {"aao", {"aao", "ar"}},
  {"abh", {"abh", "ar"}},
  {"abv", {"abv", "ar"}},
  {"acm", {"acm", "ar"}},
  {"acq", {"acq", "ar"}},
  {"acw", {"acw", "ar"}},
  {"acx", {"acx", "ar"}},
  {"acy", {"acy", "ar"}},
  {"adf", {"adf", "ar"}},
  {"ads", {"ads", "sgn"}},
  {"aeb", {"aeb", "ar"}},
  {"aec", {"aec", "ar"}},
  {"aed", {"aed", "sgn"}},
  {"aen", {"aen", "sgn"}},
  {"afb", {"afb", "ar"}},
  {"afg", {"afg", "sgn"}},
  {"ajp", {"ajp", "ar"}},
  {"apc", {"apc", "ar"}},
  {"apd", {"apd", "ar"}},
  {"arb", {"arb", "ar"}},
  {"arq", {"arq", "ar"}},
  {"ars", {"ars", "ar"}},
  {"ary", {"ary", "ar"}},
  {"arz", {"arz", "ar"}},
  {"ase", {"ase", "sgn"}},
  {"asf", {"asf", "sgn"}},
  {"asp", {"asp", "sgn"}},
  {"asq", {"asq", "sgn"}},
  {"asw", {"asw", "sgn"}},
  {"auz", {"auz", "ar"}},
  {"avl", {"avl", "ar"}},
  {"ayh", {"ayh", "ar"}},
  {"ayl", {"ayl", "ar"}},
  {"ayn", {"ayn", "ar"}},
  {"ayp", {"ayp", "ar"}},
  {"bbz", {"bbz", "ar"}},
  {"bfi", {"bfi", "sgn"}},
  {"bfk", {"bfk", "sgn"}},
  {"bjn", {"bjn", "ms"}},
  {"bog", {"bog", "sgn"}},
  {"bqn", {"bqn", "sgn"}},
  {"bqy", {"bqy", "sgn"}},
  {"btj", {"btj", "ms"}},
  {"bve", {"bve", "ms"}},
  {"bvl", {"bvl", "sgn"}},
  {"bvu", {"bvu", "ms"}},
  {"bzs", {"bzs", "sgn"}},
  {"cdo", {"cdo", "zh"}},
  {"cds", {"cds", "sgn"}},
  {"cjy", {"cjy", "zh"}},
  {"cmn", {"cmn", "zh"}},
  {"coa", {"coa", "ms"}},
  {"cpx", {"cpx", "zh"}},
  {"csc", {"csc", "sgn"}},
  {"csd", {"csd", "sgn"}},
  {"cse", {"cse", "sgn"}},
  {"csf", {"csf", "sgn"}},
  {"csg", {"csg", "sgn"}},
  {"csl", {"csl", "sgn"}},
  {"csn", {"csn", "sgn"}},
  {"csq", {"csq", "sgn"}},
  {"csr", {"csr", "sgn"}},
  {"czh", {"czh", "zh"}},
  {"czo", {"czo", "zh"}},
  {"doq", {"doq", "sgn"}},
  {"dse", {"dse", "sgn"}},
  {"dsl", {"dsl", "sgn"}},
  {"dup", {"dup", "ms"}},
  {"ecs", {"ecs", "sgn"}},
  {"esl", {"esl", "sgn"}},
  {"esn", {"esn", "sgn"}},
  {"eso", {"eso", "sgn"}},
  {"eth", {"eth", "sgn"}},
  {"fcs", {"fcs", "sgn"}},
  {"fse", {"fse", "sgn"}},
  {"fsl", {"fsl", "sgn"}},
  {"fss", {"fss", "sgn"}},
  {"gan", {"gan", "zh"}},
  {"gds", {"gds", "sgn"}},
  {"gom", {"gom", "kok"}},
  {"gse", {"gse", "sgn"}},
  {"gsg", {"gsg", "sgn"}},
  {"gsm", {"gsm", "sgn"}},
  {"gss", {"gss", "sgn"}},
  {"gus", {"gus", "sgn"}},
  {"hab", {"hab", "sgn"}},
  {"haf", {"haf", "sgn"}},
  {"hak", {"hak", "zh"}},
  {"hds", {"hds", "sgn"}},
  {"hji", {"hji", "ms"}},
  {"hks", {"hks", "sgn"}},
  {"hos", {"hos", "sgn"}},
  {"hps", {"hps", "sgn"}},
  {"hsh", {"hsh", "sgn"}},
  {"hsl", {"hsl", "sgn"}},
  {"hsn", {"hsn", "zh"}},
  {"icl", {"icl", "sgn"}},
  {"ils", {"ils", "sgn"}},
  {"inl", {"inl", "sgn"}},
  {"ins", {"ins", "sgn"}},
  {"ise", {"ise", "sgn"}},
  {"isg", {"isg", "sgn"}},
  {"isr", {"isr", "sgn"}},
  {"jak", {"jak", "ms"}},
  {"jax", {"jax", "ms"}},
  {"jcs", {"jcs", "sgn"}},
  {"jhs", {"jhs", "sgn"}},
  {"jls", {"jls", "sgn"}},
  {"jos", {"jos", "sgn"}},
  {"jsl", {"jsl", "sgn"}},
  {"jus", {"jus", "sgn"}},
  {"kgi", {"kgi", "sgn"}},
  {"knn", {"knn", "kok"}},
  {"kvb", {"kvb", "ms"}},
  {"kvk", {"kvk", "sgn"}},
  {"kvr", {"kvr", "ms"}},
  {"kxd", {"kxd", "ms"}},
  {"lbs", {"lbs", "sgn"}},
  {"lce", {"lce", "ms"}},
  {"lcf", {"lcf", "ms"}},
  {"liw", {"liw", "ms"}},
  {"lls", {"lls", "sgn"}},
  {"lsg", {"lsg", "sgn"}},
  {"lsl", {"lsl", "sgn"}},
  {"lso", {"lso", "sgn"}},
  {"lsp", {"lsp", "sgn"}},
  {"lst", {"lst", "sgn"}},
  {"lsy", {"lsy", "sgn"}},
  {"ltg", {"ltg", "lv"}},
  {"lvs", {"lvs", "lv"}},
  {"lzh", {"lzh", "zh"}},
  {"max", {"max", "ms"}},
  {"mdl", {"mdl", "sgn"}},
  {"meo", {"meo", "ms"}},
  {"mfa", {"mfa", "ms"}},
  {"mfb", {"mfb", "ms"}},
  {"mfs", {"mfs", "sgn"}},
  {"min", {"min", "ms"}},
  {"mnp", {"mnp", "zh"}},
  {"mqg", {"mqg", "ms"}},
  {"mre", {"mre", "sgn"}},
  {"msd", {"msd", "sgn"}},
  {"msi", {"msi", "ms"}},
  {"msr", {"msr", "sgn"}},
  {"mui", {"mui", "ms"}},
  {"mzc", {"mzc", "sgn"}},
  {"mzg", {"mzg", "sgn"}},
  {"mzy", {"mzy", "sgn"}},
  {"nan", {"nan", "zh"}},
  {"nbs", {"nbs", "sgn"}},
  {"ncs", {"ncs", "sgn"}},
  {"nsi", {"nsi", "sgn"}},
  {"nsl", {"nsl", "sgn"}},
  {"nsp", {"nsp", "sgn"}},
  {"nsr", {"nsr", "sgn"}},
  {"nzs", {"nzs", "sgn"}},
  {"okl", {"okl", "sgn"}},
  {"orn", {"orn", "ms"}},
  {"ors", {"ors", "ms"}},
  {"pel", {"pel", "ms"}},
  {"pga", {"pga", "ar"}},
  {"pks", {"pks", "sgn"}},
  {"prl", {"prl", "sgn"}},
  {"prz", {"prz", "sgn"}},
  {"psc", {"psc", "sgn"}},
  {"psd", {"psd", "sgn"}},
  {"pse", {"pse", "ms"}},
  {"psg", {"psg", "sgn"}},
  {"psl", {"psl", "sgn"}},
  {"pso", {"pso", "sgn"}},
  {"psp", {"psp", "sgn"}},
  {"psr", {"psr", "sgn"}},
  {"pys", {"pys", "sgn"}},
  {"rms", {"rms", "sgn"}},
  {"rsi", {"rsi", "sgn"}},
  {"rsl", {"rsl", "sgn"}},
  {"sdl", {"sdl", "sgn"}},
  {"sfb", {"sfb", "sgn"}},
  {"sfs", {"sfs", "sgn"}},
  {"sgg", {"sgg", "sgn"}},
  {"sgx", {"sgx", "sgn"}},
  {"shu", {"shu", "ar"}},
  {"slf", {"slf", "sgn"}},
  {"sls", {"sls", "sgn"}},
  {"sqk", {"sqk", "sgn"}},
  {"sqs", {"sqs", "sgn"}},
  {"ssh", {"ssh", "ar"}},
  {"ssp", {"ssp", "sgn"}},
  {"ssr", {"ssr", "sgn"}},
  {"svk", {"svk", "sgn"}},
  {"swc", {"swc", "sw"}},
  {"swh", {"swh", "sw"}},
  {"swl", {"swl", "sgn"}},
  {"syy", {"syy", "sgn"}},
  {"tmw", {"tmw", "ms"}},
  {"tse", {"tse", "sgn"}},
  {"tsm", {"tsm", "sgn"}},
  {"tsq", {"tsq", "sgn"}},
  {"tss", {"tss", "sgn"}},
  {"tsy", {"tsy", "sgn"}},
  {"tza", {"tza", "sgn"}},
  {"ugn", {"ugn", "sgn"}},
  {"ugy", {"ugy", "sgn"}},
  {"ukl", {"ukl", "sgn"}},
  {"uks", {"uks", "sgn"}},
  {"urk", {"urk", "ms"}},
  {"uzn", {"uzn", "uz"}},
  {"uzs", {"uzs", "uz"}},
  {"vgt", {"vgt", "sgn"}},
  {"vkk", {"vkk", "ms"}},
  {"vkt", {"vkt", "ms"}},
  {"vsi", {"vsi", "sgn"}},
  {"vsl", {"vsl", "sgn"}},
  {"vsv", {"vsv", "sgn"}},
  {"wuu", {"wuu", "zh"}},
  {"xki", {"xki", "sgn"}},
  {"xml", {"xml", "sgn"}},
  {"xmm", {"xmm", "ms"}},
  {"xms", {"xms", "sgn"}},
  {"yds", {"yds", "sgn"}},
  {"ysl", {"ysl", "sgn"}},
  {"yue", {"yue", "zh"}},
  {"zib", {"zib", "sgn"}},
  {"zlm", {"zlm", "ms"}},
  {"zmi", {"zmi", "ms"}},
  {"zsl", {"zsl", "sgn"}},
  {"zsm", {"zsm", "ms"}},
};

std::unordered_map<std::string,std::vector<std::string>> kDateTimeFormatComponents = {
  {"weekday", {"narrow", "short", "long"}},
  {"era", {"narrow", "short", "long"}},
  {"year", {"2-digit", "numeric"}},
  {"month", {"2-digit", "numeric", "narrow", "short", "long"}},
  {"day", {"2-digit", "numeric"}},
  {"hour", {"2-digit", "numeric"}},
  {"minute", {"2-digit", "numeric"}},
  {"second", {"2-digit", "numeric"}},
  {"timeZoneName", {"short", "long"}},
};

std::vector<std::vector<std::string>> kCollatorOptions = {
  // [0]: key, [1]: property, [2]: type, [3-]: values
  {"kn", "numeric", "boolean"},
  {"kf", "caseFirst", "string", "upper", "lower", "false"},

};

// Table 2 - Numbering systems
std::vector<std::pair<std::string,uint16_t>> kNumberingSystems = {
  {"arab", 0x0660},
  {"arabext", 0x06F0},
  {"bali", 0x01B50},
  {"beng", 0x09E6},
  {"deva", 0x0966},
  {"fullwide", 0xFF10},
  {"gujr", 0x0AE6},
  {"guru", 0x0A66},
  {"hanidec", 0x0000}, // should be handled in code.
  {"khmr", 0x17E0},
  {"knda", 0x0CE6},
  {"laoo", 0x0ED0},
  {"latn", 0x0030},
  {"limb", 0x1946},
  {"mlym", 0x0D66},
  {"mong", 0x1810},
  {"mymr", 0x1040},
  {"orya", 0x0B66},
  {"tamldec", 0x0BE6},
  {"telu", 0x0C66},
  {"thai", 0x0E50},
  {"tibt", 0x0F20},
};

__jsvalue StrToVal(std::string str) {
  __jsstring *jsstr = __jsstr_new_from_char(str.c_str());
  return __string_value(jsstr);
}

__jsvalue StrVecToVal(std::vector<std::string> strs) {
  int size = strs.size();
  __jsvalue items[size];
  for (int i = 0; i < size; i++) {
    items[i] = StrToVal(strs[i]);
  }
  __jsobject *arr_obj = __js_new_arr_elems_direct(items, size);
  return __object_value(arr_obj);
}

std::string ValToStr(__jsvalue *value) {
  if (!__is_string(value)) {
    MAPLE_JS_ASSERT(false && "not string type");
  }
  __jsstring *jsstr = __jsval_to_string(value);
  int len = __jsstr_get_length(jsstr);
  std::string res(jsstr->x.ascii, len);
  return res;
}

std::vector<std::string> VecValToVecStr(__jsvalue *value) {
  std::vector<std::string> res;

  __jsobject *o = __jsval_to_object(value);
  uint32_t size = __jsobj_helper_get_length(o);

  for (int i = 0; i < size; i++) {
    __jsvalue v = __jsarr_GetElem(o, i);
    res.push_back(ValToStr(&v));
  }
  return res;
}

// ECMA-402 1.0 8 The Intl Object
__jsvalue __js_IntlConstructor(__jsvalue *this_arg, __jsvalue *arg_list,
                            uint32_t nargs) {
  __jsobject *obj = __create_object();
  obj->extensible = true;
  obj->object_type = JSREGULAR_OBJECT;

  return __object_value(obj);
}

// ECMA-402 1.0 6.2.3
// CanonicalizeLanguageTag(locale)
__jsvalue CanonicalizeLanguageTag(__jsstring *locale) {
  std::string loc;
  if (__jsstr_is_ascii(locale)) {
    loc = std::string(locale->x.ascii, __jsstr_get_length(locale));
  }
  // Start with lower case for easier processing.
  std::transform(loc.begin(), loc.end(), loc.begin(), ::tolower);
  // Handle mappings for complete tags.
  if (kTagMappings.count(loc)) {
    //return kTagMappings[loc];
    __jsstring *str = __jsstr_new_from_char(kTagMappings[loc].c_str());
    return __string_value(str);
  }
  std::vector<std::string> subtags;
  std::stringstream ss(loc);
  std::string token;
  while (std::getline(ss, token, '-')) {
    subtags.push_back(token);
  }
  // Handle standard part: all subtags before first singleton or "x".
  int i = 0;
  while (i < subtags.size()) {
    std::string subtag = subtags[i];
    if (subtag.length() == 1 && (i > 0 || subtag == "x") ) {
      break;
    } else if (i != 0 && subtag.length() == 2) {
      std::transform(subtag.begin(), subtag.end(), subtag.begin(), ::toupper);
    } else if (subtag.length() == 4) {
      subtag[0] = toupper(subtag[0]);
    } else if (kExtLangMappings.count(subtag)) {
      subtag = kExtLangMappings[subtag][0];
      if (i == 1 && kExtLangMappings[subtag][1] == subtags[0]) {
        subtags.erase(subtags.begin());
        i--;
      }
    }
    subtags[i] = subtag;
    i++;
  }
  std::string normal;
  for (int j = 0; j < i; j++) {
    normal += subtags[j];
    if (j >= 0 && j != i-1)
      normal += "-";
  }
  // Handle extensions.
  std::vector<std::string> extensions;
  while (i < subtags.size() && subtags[i] != "x") {
    int extension_start = i;
    i++;
    while (i < subtags.size() && subtags[i].length() > 1) {
      i++;
    }
    std::string extension;
    for (int j = extension_start; j < i; j++) {
      extension += subtags[j];
      if (j >= extension_start && j < i-1) {
        extension += "-";
      }
    }
    extensions.push_back(extension);
  }
  std::sort(extensions.begin(), extensions.end());

  // Handle private use.
  std::string private_use;
  if (i < subtags.size()) {
    for (int j = i; j < subtags.size(); j++) {
      private_use += subtags[i];
      if (j >= i && j < subtags.size()-1)
        private_use += "-";
    }
  }

  // Put everyting back together.
  std::string canonical = normal;
  if (extensions.size() > 0) {
    for (int i = 0; i < extensions.size(); i++) {
      canonical += "-" + extensions[i];
    }
  }
  if (!private_use.empty()) {
    if (canonical.length() > 0) {
      canonical += "-" + private_use;
    } else {
      canonical = private_use;
    }
  }

  __jsstring *str = __jsstr_new_from_char(canonical.c_str());
  return __string_value(str);
}

// ECMA-402 1.0 6.2.2
// IsStructurallyValidLanguageTag(locale)
bool IsStructurallyValidLanguageTag(__jsstring *tag) {
  __jsvalue tag_val = __string_value(tag);

  std::string alpha = "[a-zA-Z]";
  std::string digit = "[0-9]";
  std::string alpha_num = "(" + alpha + "|" + digit + ")";
  std::string regular = "(art-lojban|cel-gaulish|no-bok|no-nyn|zh-guoyu|zh-hakka|zh-min|zh-min-nan|zh-xiang)";
  std::string irregular = "(en-GB-oed|i-ami|i-bnn|i-default|i-enochian|i-hak|i-klingon|i-lux|i-mingo|i-navajo|i-pwn|i-tao|i-tay|i-tsu|sgn-BE-FR|sgn-BE-NL|sgn-CH-DE)";
  std::string singleton = "(" + digit + "|[A-WY-Za-wy-z])";
  std::string grandfathered = "(" + irregular + "|" + regular + ")";
  std::string private_use = "(x(-[a-z0-9]{1,8})+)";
  std::string extension = "(" + singleton + "(-" + alpha_num + "{2,8})+)";
  std::string ext_lang = "(" + alpha + "{3}(-" + alpha + "{3}){0,2})";
  std::string language = "(" + alpha + "{2,3}(-" + ext_lang + ")?|" + alpha + "{4}|"
                         + alpha + "{5,8})";
  std::string variant = "(" + alpha_num + "{5,8}|(" + digit + alpha_num + "{3}))";
  std::string region = "(" + alpha + "{2}|" + digit + "{3})";
  std::string script = "(" + alpha + "{4})";
  std::string lang_tag = language + "(-" + script + ")?(-" + region + ")?(-"
                         + variant + ")*(-" + extension + ")*(-" + private_use
                         + ")?";
  // For language tag.
  std::string language_tag = "^(" + lang_tag + "|" + private_use + "|"
                             + grandfathered + ")$";
  __jsstring *language_tag_str = __jsstr_new_from_char(language_tag.c_str());
  __jsvalue language_tag_re = __string_value(language_tag_str);

  // For duplicate singleton.
  std::string duplicate_singleton = "-" + singleton + "-(.*-)?\\1(?!" + alpha_num
                                    + ")";
  __jsstring *duplicate_singleton_str = __jsstr_new_from_char(duplicate_singleton.c_str());
  __jsvalue duplicate_singleton_re = __string_value(duplicate_singleton_str);

  // For duplicate variant.
  std::string duplicate_variant = "(" + alpha_num + "{2,8}-)+" + variant + "-("
                                  + alpha_num + "{2,8}-)*\\3(?!" + alpha_num + ")";
  std::string tag_str;
  int len = __jsstr_get_length(tag);
  tag_str.assign(tag->x.ascii, len);

  std::smatch sm;
  std::regex re(language_tag);
  if (std::regex_search(tag_str, sm, re) == false) {
    return false;
  }

  std::string separator = "-x-";
  std::string tag0 = tag_str.substr(0, tag_str.find(separator));

  re = (duplicate_singleton);
  if (std::regex_search(tag0, sm, re) == true)
    return false;
  re = (duplicate_variant);
  if (std::regex_search(tag0, sm, re) == true)
    return false;

  return true;
}

// ECMA-402 1.0 6.3.1
// IsWellFormedCurrencyCode(currency)
bool IsWellFormedCurrencyCode(__jsvalue *currency) {
  // Step 2.
  __jsvalue normalized = __jsstr_toUpperCase(currency);
  __jsstring *normalized_str = __jsval_to_string(&normalized);
  // Step 3.
  uint32_t len = __jsstr_get_length(normalized_str);
  if (len != 3) {
    return false;
  }
  if (__jsstr_is_ascii(normalized_str)) {
    for (int i = 0; i < len; i++) {
      uint16_t c = __jsstr_get_char(normalized_str, i);
      if (c < 'A' || c > 'Z')
        return false;
    }
  } else {
    for (int i = 0; i < len; i++) {
      uint16_t c = __jsstr_get_char(normalized_str, i);
      if (c < 0x0041 || c > 0x005a)
        return false;
    }
  }
  return true;
}

// ECMA-402 1.0 9.2.1
// CanonicalizeLocaleList(locales)
// interpretes the 'locales' arguments as an array and copies its elements into
// a List, validating the elements as structurally valid language tags and
// canonicalizing them, and omitting duplicates.
__jsvalue CanonicalizeLocaleList(__jsvalue *locales) {
  // Step 1.
  // If 'locales' is undefined, then
  if (__is_undefined(locales)) {
    // Step 1a.
    // Return a new empty List.
    __jsobject *arr = __js_new_arr_internal(0);
    return __object_value(arr);
  }
  // Step 2.
  // Let 'seen' be a new empty List.
  __jsvalue seen = __object_value(__js_new_arr_internal(0));

  // Step 3.
  // If 'locales' is String value, then
  if (__is_string(locales)) {
    // Step 3a.
    // Let 'locales' be a new array created as if by the expression 'new Array(locales)'
    // where 'Array' is the standard built-in constructor with the name and
    // 'locales' is the value of 'locales'.
    __jsobject *locales_arr_obj = __js_new_arr_elems_direct(locales, 1); // 1st argument does not matter.
    *locales = __object_value(locales_arr_obj);
  }

  if (__is_null(locales)) { // handle the case when locales is null.
    MAPLE_JS_TYPEERROR_EXCEPTION();
  }

  // Check if 'locales' is non-object (JSI9405).
  if (!__is_js_object(locales)) {
    __jsobject *arr = __js_new_arr_internal(0);
    return __object_value(arr);
  }

  // Step 4.
  // Let 'O' be ToObject(locales).
  __jsobject *o_obj = __jsval_to_object(locales);
  __jsvalue o = __object_value(o_obj);

  // Step 5.
  // Let 'lenValue' be the result of calling the [[Get]] internal method of 'O'
  // with the argument "length".
  __jsvalue len_value = __jsobj_helper_get_length_value(o_obj);

  // Step 6.
  // Let 'len' be ToUint32(lenValue).
  uint32_t len = __jsval_to_uint32(&len_value);

  // Step 7.
  uint32_t k = 0;

  // Step 8.
  while (k < len) {
    // Step 8a.
    __jsstring *p_k = __js_NumberToString(k);
    // Step 8b.
    bool k_present = __jsobj_internal_HasProperty(o_obj, p_k);
    // Step 8c.
    if (k_present) {
      // Step 8c i.
      __jsvalue prop = __string_value(p_k);
      __jsvalue k_value = __jsop_getprop(&o, &prop);
      // Step 8c ii.
      if (!(__is_string(&k_value) || __is_js_object(&k_value))) {
        MAPLE_JS_TYPEERROR_EXCEPTION();
      }
      // Step 8c iii.
      __jsstring *tag_str = __jsval_to_string(&k_value);
      // Step 8c iv.
      if (!IsStructurallyValidLanguageTag(tag_str)) {
        MAPLE_JS_RANGEERROR_EXCEPTION();
      }
      // Step 8c v.
      __jsvalue tag = CanonicalizeLanguageTag(tag_str);
      // step 8c vi.
      __jsvalue idx = __jsarr_pt_indexOf(&seen, &tag, 1);
      if (__js_ToNumber(&idx) == -1) {
        seen = __jsarr_pt_concat(&seen, &tag, 1);
      }
    }
    // Step 8d.
    k++;
  }
  // Step 9.
  return seen;
}

// ECMA-402 1.0 9.2.2
// BestAvailableLocale(availableLocales, locale)
__jsvalue BestAvailableLocale(__jsvalue *available_locales, __jsvalue *locale) {
  // Step 1.
  __jsvalue candidate = *locale;
  // Step 2.
  while (true) {
    // Step 2a.
    __jsvalue idx = __jsarr_pt_indexOf(available_locales, &candidate, 1); // 1: argNum of 'candidate'
    if (__jsval_to_number(&idx) != -1) {
      return candidate;
    }
    // Step 2b.
    // Let 'pos' be the character index of the last occurence of "-" (U+002D)
    // within 'candidate'. If that character does not occur, return undefined.
    __jsvalue search_element = StrToVal("-");
    __jsvalue from_index = __number_value(0);
    __jsvalue pos = __jsstr_lastIndexOf(&candidate, &search_element, &from_index);
    if (__jsval_to_number(&pos) == -1) {
      return __undefined_value();
    }
    // Step 2c.
    __jsstring *candidate_str = __jsval_to_string(&candidate);
    int p = __jsval_to_number(&pos);
    if (__jsval_to_number(&pos) >= 2 && __jsstr_get_char(candidate_str, p - 2) == '-') {
      pos  = __number_value(p - 2);
    }
    // Step 2d.
    __jsvalue start = __number_value(0);
    __jsvalue end = pos;
    candidate = __jsstr_substring(&candidate, &start, &end);
  }
  return __undefined_value();
}

// Used for ECMA-402 1.0 9.2.3
__jsvalue RemoveUnicodeExtensions(__jsvalue *locale) {
  std::string loc;
  __jsstring *loc_str = __jsval_to_string(locale);
  int len = __jsstr_get_length(loc_str);
  loc.assign(loc_str->x.ascii, len);
  loc += '\0';

  // "Unicode locale extension sequence" defined as:
  // "any substring of a langauge tag that starts with a separator '-' and
  // the singleton 'u' and includes the maximum sequence of following
  // non-singleton subtags and their preceding '-' separators."
  std::regex re(kUnicodeLocaleExtensionSequence);
  std::smatch sm;
  std::string res;
  if (std::regex_search(loc, sm, re)) {
    res = sm.prefix().str(); // TODO: double-check!
  } else {
    res = loc;
  }
  __jsstring *js_res = __jsstr_new_from_char(res.c_str());
  return __string_value(js_res);
}

// ECMA-402 1.0 9.2.3
// LookupMatcher(availableLocales, requestedLocales)
// compares 'requestedLocales', which must be a List as returned by 'CanonicalizeLocaleList',
// against the locales in 'availableLocales' and determines the best available
// language to meet the request.
__jsvalue LookupMatcher(__jsvalue *available_locales, __jsvalue *requested_locales) {
  // Step 1.
  int i = 0;
  // Step 2.
  __jsobject *o = __jsval_to_object(requested_locales);
  uint32 len = __jsobj_helper_get_length(o);
  // Step 3.
  __jsvalue available_locale = __undefined_value();
  // Step 4.
  __jsvalue locale;
  __jsvalue no_extension_locale;
  while (i < len && __is_undefined(&available_locale)) {
    // Step 4a.
    __jsobject *requested_locales_obj = __jsval_to_object(requested_locales);
    locale = __jsarr_GetElem(requested_locales_obj, i);
    // Step 4b.
    no_extension_locale = RemoveUnicodeExtensions(&locale);
    // Step 4c.
    // Let 'availableLocale' be the result of calling the 'BestAvailableLocale'
    // abstract operation with arguments 'availableLocales' and 'noExtensionLocale'.
    available_locale = BestAvailableLocale(available_locales, &no_extension_locale);
    // Step 4d.
    i++;
  }
  // Step 5.
  __jsobject *result_obj = __js_new_obj_obj_0();
  __jsvalue result = __object_value(result_obj);

  // Step 6.
  // If 'availableLocale' is not undefined, then
  __jsvalue prop;
  if (!__is_undefined(&available_locale)) {
    // Step 6a.
    // Set result.[[locale]] to 'availableLocale'.
    prop = StrToVal("locale");
    __jsop_setprop(&result, &prop, &available_locale);
    // Step 6b.
    __jsstring *locale_str = __jsval_to_string(&locale);
    __jsstring *no_extension_locale_str = __jsval_to_string(&no_extension_locale);
    if (!__jsstr_equal(locale_str, no_extension_locale_str)) {
      // Step 6b i.
      std::string loc;
      int len = __jsstr_get_length(locale_str);
      loc.assign(locale_str->x.ascii, len);
      loc += '\0';

      // Step 6b ii.
      std::regex re(kUnicodeLocaleExtensionSequence);
      std::smatch sm;
      std::string extension;
      int extension_index;
      std::regex_search(loc, sm, re);
      extension = sm[0].str();
      extension_index = sm.position(0);

      // Step 6b iii.
      prop = StrToVal("extension");
      __jsstring *v_str = __jsstr_new_from_char(extension.c_str());
      __jsvalue value = __string_value(v_str);
      __jsop_setprop(&result, &prop, &value);

      // Step 6b iv.
      prop = StrToVal("extensionIndex");
      value = __number_value(extension_index);
      __jsop_setprop(&result, &prop, &value);
    }
  } else {
    // Step 7.
    // Else
    // Step 7a.
    // Set result.[[locale]] to the value returned by the DefaultLocale abstract
    // operation.
    prop = StrToVal("locale");
    __jsvalue default_locale = DefaultLocale();
    __jsop_setprop(&result, &prop, &default_locale);
  }
  // Step 8.
  return result;
}
// ECMA-402 1.0 9.2.4
// BestFitMatcher(availableLocales, requestedLocales)
__jsvalue BestFitMatcher(__jsvalue *available_locales, __jsvalue *requested_locales) {
  return LookupMatcher(available_locales, requested_locales);
}

// ECMA-402 1.0 9.2.5
// ResolveLocale(availableLocales, requestedLocales, options, relevantExtensionKeys,
//               localeData)
// compares a BCP 47 language priority list 'requestedLocales' against the locales
// in 'availableLocales' and deteermines the best available language to meet the
// request. 'availableLocales' and 'requestedLocales' must be provided as List values,
// 'options' as a Record.
__jsvalue ResolveLocale(__jsvalue *available_locales, __jsvalue *requested_locales,
                        __jsvalue *options, __jsvalue *relevant_extension_keys,
                        __jsvalue *locale_data) {
  // Step 1.
  __jsvalue prop = StrToVal("localeMatcher");
  __jsvalue locale_matcher = __jsop_getprop(options, &prop);
  std::string matcher = ValToStr(&locale_matcher);
  // Step 2.
  __jsvalue r;
  if (matcher == "lookup") {
    r = LookupMatcher(available_locales, requested_locales);
  } else {
    // Step 3a.
    r = BestFitMatcher(available_locales, requested_locales);
  }
  // Step 4.
  // Let 'foundLocale' be the value of r.[[locale]].
  prop = StrToVal("locale");
  __jsvalue found_locale = __jsop_getprop(&r, &prop);
  // Step 5a.
  prop = StrToVal("extension");
  __jsvalue extension = __jsop_getprop(&r, &prop);
  __jsvalue extension_subtags = __undefined_value();
  __jsvalue extension_subtags_length = __undefined_value();
  __jsvalue extension_index = __undefined_value();
  if (!__is_undefined(&extension)) {
    // Step 5b.
    prop = StrToVal("extensionIndex");
    // Let 'extensionIndex' be the value of r.[[extensionIndex]].
    extension_index = __jsop_getprop(&r, &prop);
    // Step 5c-5d.
    __jsvalue separator = StrToVal("-");
    __jsvalue limit = __undefined_value();
    extension_subtags = __jsstr_split(&extension, &separator, &limit);
    // Let 'extensionSubtags' be the result of calling the [[Call]]
    // internal method of 'split' with 'extension'a as the this value
    // and an argument list containing the single item "-".
    // Step 5e.
    // Let 'extensionSubtagsLength' be the result of calling the [[Get]]
    // internal method of 'extensionSubtags' with argument "length".
    prop = StrToVal("length");
    extension_subtags_length = __jsop_getprop(&extension_subtags, &prop);
  }
  // Step 6.
  // Let 'result' be a new Record.
  __jsobject *result_obj = __js_new_obj_obj_0();
  __jsvalue result = __object_value(result_obj);
  // Step 7.
  // Set result.[[dataLocale]] to 'foundLocale'.
  prop = StrToVal("dataLocale");
  __jsop_setprop(&result, &prop, &found_locale);
  // Step 8.
  __jsvalue supported_extension = StrToVal("-u");
  // Step 9.
  int i = 0;
  // Step 10.
  int len;
  __jsvalue len_val;
  if (!__is_undefined(relevant_extension_keys)) {
    prop = StrToVal("length");
    len_val = __jsop_getprop(relevant_extension_keys, &prop);
    len = __jsval_to_number(&len_val);
  } else {
    len = 0;
  }
  // Step 11.
  while (i < len) {
    // default for each entry.
    __jsvalue value = __null_value();

    // Step 11a.
    __jsvalue key = __jsarr_GetElem(__jsval_to_object(relevant_extension_keys), i);
    // Step 11b.
    __jsvalue found_locale_data = __jsop_getprop(locale_data, &found_locale);

    // Check if 'found_locale_data' is undefined (JSI9398).
    if (__is_undefined(&found_locale_data)) {
      break;
    }

    // Step 11c.
    // Let 'keyLocaleData' be the result of calling the [[Get]] internal method
    // of 'foundLocaleData' with the argument 'key'.
    __jsvalue key_locale_data = __jsop_getprop(&found_locale_data, &key);

    // Error handling for null or undefined key_locale_data.
    if (__is_null_or_undefined(&key_locale_data)) {
      __jsop_setprop(&result, &key, &value);
      i++;
      continue;
    }
    // Step 11d.
    // Let 'value' be the result of calling the [[Get]] internal method of 'keyLocaleData'
    // with argument "0".
    __jsobject *o = __jsval_to_object(&key_locale_data);

    value = __jsarr_GetElem(o, 0);
    // Step 11e.
    __jsvalue supported_extension_addition = StrToVal("");
    // Step 11g.
    if (!__is_undefined(&extension_subtags)) {
      // Step 11g i.
      // Let 'keyPos' be the result of calling the [[Call]] internal method of
      // 'indexOf' with 'extensionSubtags' as the 'this' value and an argument
      // list containing the single item 'key'.
      __jsvalue pos_val = __number_value(0);
      __jsvalue key_pos = __jsstr_indexOf(&extension_subtags, &key, &pos_val);
      // Step 11g ii.
      int pos = __jsval_to_number(&key_pos);
      if (pos != -1) {
        // Step 11g ii 1.
        prop = StrToVal(std::to_string(pos+1));
        __jsvalue cond = __jsop_getprop(&extension_subtags, &prop);
        if ((pos + 1 < __jsval_to_number(&extension_subtags_length)) &&
            (__jsval_to_number(&cond) > 2)) {
            // Step 11g ii 1a.
            __jsvalue requested_value = __jsop_getprop(&extension_subtags, &prop);
            // Step 11g ii 1b.
            __jsvalue value_pos = __jsstr_indexOf(&key_locale_data, &requested_value, 0);
            // Step 11g ii 1c.
            if (__jsval_to_number(&value_pos) != -1) {
              // Step 11g ii 1c i.
              value = requested_value;
              // Step 11g ii 1c ii.
              std::string d = "-";
              __jsstring *dash = __jsstr_new_from_char(d.c_str());
              __jsstring *dash_key = __jsstr_concat_2(dash, __jsval_to_string(&key));
              __jsstring *dash_value = __jsstr_concat_2(dash, __jsval_to_string(&value));
              __jsstring *addition = __jsstr_concat_2(dash_key, dash_value);
              supported_extension_addition = __string_value(addition);
            }
        } else {
          // Step 11g ii 2a.
          prop = StrToVal("true");
          __jsvalue zero_idx = __number_value(0);
          __jsvalue value_pos = __jsstr_indexOf(&key_locale_data, &prop, &zero_idx);
          // Step 11g ii 2b.
          if (__jsval_to_number(&value_pos) != -1) {
            // Step 11g ii 2b i.
            value = StrToVal("true");
          }
        }
      }
    }
    // Step 11h i.
    __jsvalue options_value = __undefined_value();
    if (!__is_null_or_undefined(&key)) {
      options_value = __jsop_getprop(options, &key);
    }
    if (!__is_undefined(&options_value)) {
      // Step 11h ii.
      __jsvalue zero_idx = __number_value(0);
      __jsvalue idx = __jsstr_indexOf(&key_locale_data, &options_value, &zero_idx);
      if (__jsval_to_number(&idx) != -1) {
        // Step 11h ii 1.
        __jsstring *options_value_str = __jsval_to_string(&options_value);
        __jsstring *value_str = __jsval_to_string(&value);
        if (__jsstr_ne(options_value_str, value_str)) {
          // Step 11h ii 1a.
          value = options_value;
          supported_extension_addition = StrToVal("");
        }
      }
    }
    // Step 11i.
    // Set result.[[<key>]] to 'value'.
    __jsop_setprop(&result, &key, &value);
    // Step 11j.
    __jsstr_concat(&supported_extension, &supported_extension_addition, 1);
    // Step 11h k.
    i++;
  } // end of while
  // Step 12.
  if (__jsstr_get_length(__jsval_to_string(&supported_extension)) > 2) {
    // Step 12a.
    __jsvalue zero = __number_value(0);
    __jsvalue pre_extension = __jsstr_split(&found_locale, &zero, &extension_index);
    // Step 12b.
    __jsvalue undefined = __undefined_value();
    __jsvalue post_extension = __jsstr_split(&found_locale, &extension_index, &undefined);
    found_locale = __jsstr_concat(&pre_extension, &post_extension, 1);

  }
  // Step 13.
  // Set result.[[locale]] to be 'foundLocale'.
  prop = StrToVal("locale");
  __jsop_setprop(&result, &prop, &found_locale);
  // Step 14.
  return result;
}

// ECMA-402 1.0 9.2.6
// LookupSupportedLocales(availableLocales, requestedLocales)
__jsvalue LookupSupportedLocales(__jsvalue *available_locales,
                                __jsvalue *requested_locales) {
  // Step 1.
  // Let 'len' be the number of elements in 'requestedLocales'.
  //int len = __jsarr_getIndex(requested_locales);
  __jsobject *requested_locales_obj = __jsval_to_object(requested_locales);
  uint32_t len = __jsobj_helper_get_length(requested_locales_obj);
  // Step 2.
  // Let 'subset' be a new empty List.
  __jsvalue subset = __object_value(__js_new_arr_internal(0));
  // Step 3.
  int k = 0;
  // Step 4.
  while (k < len) {
    // Step 4a.
    // Let 'locale' be the element of 'requestedLocales' at 0-origined list position k.
    //__jsobject *o = __jsval_to_object(requested_locales);
    __jsvalue locale = __jsarr_GetElem(requested_locales_obj, k);
    // Step 4b.
    __jsvalue no_extensions_locale = RemoveUnicodeExtensions(&locale);
    // Step 4c.
    __jsvalue available_locale = BestAvailableLocale(available_locales, &no_extensions_locale);
    // Step 4d.
    // If 'availableLocale' is not undefined, then append 'locale' to the end of 'subset'.
    if (!__is_undefined(&available_locale)) {
      __jsarr_pt_push(&subset, &locale, 1);
    }
    // Step 4e.
    k++;
  }
  // Step 5.
  __jsvalue subset_array = subset; // TODO: double-check!
  // Step 6.
  return subset_array;
}

// ECMA-402 1.0 9.2.7
// BestFitSupportedLocales(availableLocales, requestedLocales)
__jsvalue BestFitSupportedLocales(__jsvalue *available_locales,
                                  __jsvalue *requested_locales) {
  return LookupSupportedLocales(available_locales, requested_locales);
}

// ECMA-402 1.0 9.2.8
// SupportedLocales(availableLocales, requestedLocales, options)
__jsvalue SupportedLocales(__jsvalue *available_locales,
                           __jsvalue *requested_locales, __jsvalue *options) {
  // Step 1.
  __jsvalue prop;
  __jsvalue matcher = __undefined_value();
  std::string matcher_str;
  if (!__is_undefined(options)) {
    // Step 1a-1b.
    prop = StrToVal("localeMatcher");
    matcher = __jsop_getprop(options, &prop);
    // Step 1c.
    if (!__is_undefined(&matcher)) {
      // Step 1c i.
      matcher_str = ValToStr(&matcher);
      // Step 1c ii.
      if (matcher_str != "lookup" && matcher_str != "best fit") {
        MAPLE_JS_RANGEERROR_EXCEPTION();
      }
    }
  }
  // Step 2.
  __jsvalue subset = __undefined_value();
  if (__is_undefined(&matcher) || matcher_str == "best fit") {
    // Step 2a.
    subset = BestFitSupportedLocales(available_locales, requested_locales);
  } else {
    // Step 3.
    subset = LookupSupportedLocales(available_locales, requested_locales);
  }
  // Step 4.
  // For named own property name 'P' of 'subset',
  __jsvalue list = __jsobj_getOwnPropertyNames(&subset, &subset); // 1st arguement not used.
  __jsobject *list_object = __jsval_to_object(&list);
  uint32_t size = __jsobj_helper_get_length(list_object);
  __jsobject *subset_object = __jsval_to_object(&subset);
  for (int i = 0; i < size; i++) {
    // Step 4a.
    // Let 'desc' be the result of calling the [[GetOwnProperty]] internal method of 'subset' with 'P'.
    __jsvalue p = __jsarr_GetElem(list_object, i);
    // Step 4b-4c.
    __jsprop_desc desc = __jsobj_internal_GetProperty(subset_object, &p);
    // Set desc.[[Writable]] to false.
    // Set desc.[[Configurable]] to false.
    __set_writable(&desc, true);
    __set_configurable(&desc, false);
    // Step 4d.
    // Call the [[DefineOwnProperty]] internal method of 'subset' with 'P', 'desc',
    // and true as arguments.
    __jsobj_internal_DefineOwnProperty(subset_object, &p, desc, true);
  }
  // Step 5.
  return subset;
}

// ECMA-402 1.0 9.2.9
// GetOption(options, property, type, values, fallback)
__jsvalue GetOption(__jsvalue *options, __jsvalue *property, __jsvalue *type,
                    __jsvalue *values, __jsvalue *fallback) {
  // Step 1.
  __jsvalue value = __jsop_getprop(options, property);

  // Additional null check for 'value'.
  if (__is_null(&value)) {
    MAPLE_JS_REFERENCEERROR_EXCEPTION();
  }

  // Step 2.
  // Additional check if 'value' is none (JSI9479).
  if (!__is_undefined(&value) && !__is_none(&value)) {
    // Step 2a.
    // Assert: 'type' is "boolean" or "string".
    __jsstring *jsstr_type = __jsval_to_string(type);
    __jsstring *string_type = __jsstr_new_from_char("string");
    __jsstring *boolean_type = __jsstr_new_from_char("boolean");
    MAPLE_JS_ASSERT(__jsstr_equal(jsstr_type, string_type) || __jsstr_equal(jsstr_type, boolean_type));
    // Step 2b.
    // If 'type' is "boolean", then let 'value' to be ToBoolean(value).
    if (__jsstr_equal(jsstr_type, boolean_type)) {
      value = __boolean_value(__jsval_to_boolean(&value));
    }
    // Step 2c.
    // If 'type' is "string", then let 'value' to be ToString(value).
    if (__jsstr_equal(jsstr_type, string_type)) {
      value = __string_value(__jsval_to_string(&value));
    }
    // Step 2d.
    // If 'values' is not undefined, then
    if (!__is_undefined(values)) {
      // Step 2d i.
      // If 'values' does not contain an element equal to 'value', then throw a 'RangeError' exception.
      __jsvalue idx = __jsarr_pt_indexOf(values, &value, 1);
      if (__jsval_to_number(&idx) == -1) {
        MAPLE_JS_RANGEERROR_EXCEPTION();
      }
    }
    // Step 2e.
    // Else return 'value'.
    return value;
  }
  // Step 3.
  return *fallback;
}

// ECMA-402 1.0 9.2.10
// GetNumberOption(options, property, minimum, maximum, fallback)
__jsvalue GetNumberOption(__jsvalue *options, __jsvalue *property,
                          __jsvalue *minimum, __jsvalue *maximum,
                          __jsvalue *fallback) {
  // Step 1.
  __jsvalue value = __jsop_getprop(options, property);
  // Additional check for 'value' (JSI9465).
  if (__is_infinity(&value) || __is_neg_infinity(&value)) {
    MAPLE_JS_REFERENCEERROR_EXCEPTION();
  }
  // Step 2.
  if (!__is_undefined(&value)) {
    // Step 2a.
    if (__is_nan(&value) ||
        __jsval_to_number(&value) < __jsval_to_number(minimum) ||
        __jsval_to_number(&value) > __jsval_to_number(maximum)) {
      MAPLE_JS_RANGEERROR_EXCEPTION();
    }
    int v = floor(__jsval_to_double(&value));
    return __number_value(v);
  }
  // Step 3.
  return *fallback;
}

// ECMA-402 6,2,4
// DefaultLocale()
__jsvalue DefaultLocale() {
#if 0
  char *locale = getenv("LANG");
  if (!locale || !strcmp(locale, "C"))
    locale = const_cast<char*>("und");
  std::string l(locale);
#else
  std::string l("en-US");
#endif

  __jsstring *jsstr_locale = __jsstr_new_from_char(l.c_str());
  return CanonicalizeLanguageTag(jsstr_locale);
}

// Return the value of 'availableLocales' property of the standard built-in object.
__jsvalue GetAvailableLocales() {
  int num_locales = uloc_countAvailable();
  std::vector<std::string> locales;
  for (int i = 0; i < num_locales; i++) {
    std::string str(uloc_getAvailable(i));
    std::replace(str.begin(), str.end(), '_', '-');
    locales.push_back(str);
  }
  __jsvalue value = StrVecToVal(locales);
  return value;
}

std::vector<std::string> GetNumberingSystems() {
  std::vector<std::string> vec;
  for (int i = 0; i < kNumberingSystems.size(); i++) {
    vec.push_back(kNumberingSystems[i].first);
  }
  icu::NumberingSystem num_sys;
  std::string default_numsys(num_sys.getName());
  vec.insert(vec.begin(), default_numsys);

  return vec;
}
