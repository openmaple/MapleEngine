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
#include <unicode/uenum.h>
#include <unicode/uloc.h>
#include <unicode/numsys.h>
#include "jsintl.h"
#include "jsvalueinline.h"
#include "jsvalue.h"
#include "jsobject.h"
#include "jsobjectinline.h"
#include "jsarray.h"
#include "jsmath.h"

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
  {"kn", "numeric", "boolean"},
  {"kf", "caseFirst", "string", "upper", "lower", "false"},

};

static inline __jsvalue StrToVal(std::string str) {
  __jsstring *jsstr = __jsstr_new_from_char(str.c_str());
  int len = __jsstr_get_length(jsstr);
  return __string_value(jsstr);
}

static inline __jsvalue StrVecToVal(std::vector<std::string> strs) {
  int size = strs.size();
  __jsvalue items[size];
  for (int i = 0; i < strs.size(); i++) {
    items[i] = StrToVal(strs[i]);

  }
  __jsobject *arr_obj = __js_new_arr_elems_direct(items, size);
  return __object_value(arr_obj);
}

static inline std::string ValToStr(__jsvalue *value) {
  __jsstring *jsstr = __jsval_to_string(value);
  int len = __jsstr_get_length(jsstr);
  std::string res(jsstr->x.ascii, len);
  return res;
}

// ECMA-402 1.0 8 The Intl Object
__jsvalue __js_IntlConstructor(__jsvalue *this_arg, __jsvalue *arg_list,
                            uint32_t nargs) {
  __jsobject *obj = __create_object();
  obj->object_class = JSINTL;
  obj->extensible = true;
  obj->object_type = JSREGULAR_OBJECT;

  return __object_value(obj);
}

// ECMA-402 1.0 10.1 The Intl.Collator Constructor
__jsvalue __js_CollatorConstructor(__jsvalue *this_arg, __jsvalue *arg_list,
                                   uint32_t nargs) {
  __jsobject *obj = __create_object();
  obj->object_class = JSINTL;
  obj->extensible = true;
  obj->object_type = JSREGULAR_OBJECT;
  obj->shared.intl = (__jsintl*) VMMallocGC(sizeof(__jsintl));
  obj->shared.intl->kind = JSINTL_COLLATOR;
  __jsvalue obj_val = __object_value(obj);

  __jsvalue locales = __undefined_value();
  __jsvalue options = __undefined_value();
  if (nargs == 0) {
    // Do nothing.
  } else if (nargs == 1) {
    locales = arg_list[0];
  } else if (nargs == 2) {
    locales = arg_list[0];
    options = arg_list[1];
  } else {
    MAPLE_JS_SYNTAXERROR_EXCEPTION();
  }
  InitializeCollator(&obj_val, &locales, &options);

  return obj_val;
}

// ECMA-402 1.0 11.1.3.1 The Intl.NumberFormat Constructor
__jsvalue __js_NumberFormatConstructor(__jsvalue *this_arg, __jsvalue *arg_list,
                                       uint32_t nargs) {
  __jsobject *obj = __create_object();
  obj->object_class = JSINTL;
  obj->extensible = true;
  obj->object_type = JSREGULAR_OBJECT;
  obj->shared.intl = (__jsintl*) VMMallocGC(sizeof(__jsintl));
  obj->shared.intl->kind = JSINTL_NUMBERFORMAT;
  __jsvalue obj_val = __object_value(obj);

  __jsvalue prop = StrToVal("InitializedIntlObject");
  __jsvalue value = __boolean_value(false);
  __jsop_setprop(&obj_val, &prop, &value);

  __jsvalue locales = __undefined_value();
  __jsvalue options = __undefined_value();
  if (nargs == 0) {
    // Do nothing.
  } else if (nargs == 1) {
    locales = arg_list[0];
  } else if (nargs == 2) {
    locales = arg_list[0];
    options = arg_list[1];
  } else {
    MAPLE_JS_SYNTAXERROR_EXCEPTION();
  }
  InitializeNumberFormat(&obj_val, &locales, &options);

  return __object_value(obj);
}

// ECMA-402 1.0 12.1 The Intl.DateTimeFormat Constructor
__jsvalue __js_DateTimeFormatConstructor(__jsvalue *this_arg,
                                        __jsvalue *arg_list, uint32_t nargs) {
  __jsobject *obj = __create_object();
  obj->object_class = JSINTL;
  obj->extensible = true;
  obj->object_type = JSREGULAR_OBJECT;
  obj->shared.intl = (__jsintl*) VMMallocGC(sizeof(__jsintl));
  obj->shared.intl->kind = JSINTL_DATETIMEFORMAT;
  __jsvalue obj_val = __object_value(obj);

  __jsvalue locales = __undefined_value();
  __jsvalue options = __undefined_value();
  if (nargs == 0) {
    // Do nothing.
  } else if (nargs == 1) {
    locales = arg_list[0];
  } else if (nargs == 2) {
    locales = arg_list[1];
    options = arg_list[2];
  } else {
    MAPLE_JS_SYNTAXERROR_EXCEPTION();
  }
  InitializeDateTimeFormat(&obj_val, &locales, &options);

  return obj_val;
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
__jsvalue CanonicalizeLocaleList(__jsvalue *locales) {
  // Step 1.
  if (__is_undefined(locales)) {
    __jsobject *arr = __js_new_arr_internal(0);
    return __object_value(arr);
  }
  // Step 2.
  __jsvalue seen = __object_value(__js_new_arr_internal(0));

  // Step 3.
  __jsvalue locales2 = *locales;
  if (__is_string(locales)) {
    __jsobject *arr_obj = __js_new_arr_elems_direct(locales, 1);
    locales2 = __object_value(arr_obj);
  }

  // Step 4.
  __jsobject *o = __jsval_to_object(&locales2);

  // Step 5.
  __jsvalue len_value = __jsobj_helper_get_length_value(o);

  // Step 6.
  uint32_t len = __jsval_to_uint32(&len_value);

  // Step 7.
  uint32_t k = 0;

  // Step 8.
  while (k < len) {
    // Step 8a.
    __jsstring *p_k = __js_NumberToString(k);
    // Step 8b.
    __jsvalue k_value;
    bool k_present = __jsobj_helper_HasPropertyAndGet(o, p_k, &k_value);
    // Step 8c.
    if (k_present) {
      // Step 8c ii.
      if (!(__is_string(&k_value) || __is_js_object(&k_value))) {
        MAPLE_JS_TYPEERROR_EXCEPTION();
      }
      // Step 8c iii.
      __jsstring *tag = __jsval_to_string(&k_value);
      // Step 8c iv.
      if (!IsStructurallyValidLanguageTag(tag)) {
        MAPLE_JS_RANGEERROR_EXCEPTION();
      }
      // Step 8c v.
      __jsvalue tag2 = CanonicalizeLanguageTag(tag);
      // step 8c vi.
      __jsvalue idx = __jsarr_pt_indexOf(&seen, &tag2, 0);
      if (__js_ToNumber(&idx) == -1) {
        seen = __jsarr_pt_concat(&seen, &tag2, 1);
      }
      // Step 8d.
      k++;
    }
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
    __jsvalue idx = __jsarr_pt_indexOf(available_locales, &candidate, 0);
    if (__jsval_to_number(&idx) != -1) {
      return candidate;
    }
    // Step 2b.
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

  // "Unicode locale extension sequence" defined as:
  // "any substring of a langauge tag that starts with a separator '-' and
  // the singleton 'u' and includes the maximum sequence of following
  // non-singleton subtags and their preceding '-' separators."
  std::regex re(kUnicodeLocaleExtensionSequence);
  std::smatch sm;
  std::string res;
  std::regex_search(loc, sm, re);
  if (sm.ready()) {
    res = sm.prefix().str(); // TODO: double-check!
  }
  __jsstring *js_res = __jsstr_new_from_char(res.c_str());
  return __string_value(js_res);
}

// ECMA-402 1.0 9.2.3
// LookupMatcher(availableLocales, requestedLocales)
__jsvalue LookupMatcher(__jsvalue *available_locales, __jsvalue *requested_locales) {
  // Step 1.
  int i = 0;
  // Step 2.
  int len = __jsarr_getIndex(requested_locales);
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
    __jsvalue available_locale = BestAvailableLocale(available_locales, &no_extension_locale);
    // Step 4d.
    i--;
  }
  // Step 5.
  __jsobject *result_obj = __js_new_obj_obj_0();
  __jsvalue result = __object_value(result_obj);

  // Step 6.
  __jsvalue prop;
  if (!__is_undefined(&available_locale)) {
    // Step 6a.
    prop = StrToVal("locale");
    __jsop_setprop(&result, &prop, available_locales);
    // Step 6b.
    __jsstring *locale_str = __jsval_to_string(&locale);
    __jsstring *no_extension_locale_str = __jsval_to_string(&no_extension_locale);
    if (!__jsstr_equal(locale_str, no_extension_locale_str)) {
      // Step 6b i.
      std::string loc;
      int len = __jsstr_get_length(locale_str);
      loc.assign(locale_str->x.ascii, len);

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
__jsvalue ResolveLocale(__jsvalue *available_locales, __jsvalue *requested_locales,
                        __jsvalue *options, __jsvalue *relevant_extension_keys,
                        __jsvalue *locale_data) {
  // Step 1.
  __jsvalue prop = StrToVal("localeMatcher");
  __jsvalue matcher_val = __jsop_getprop(options, &prop);
  std::string matcher = ValToStr(&matcher_val);
  // Step 2.
  //__jsobject *r_obj = __js_new_obj_obj_0();
  __jsvalue r; // = __object_value(r_obj);
  if (matcher == "lookup") {
    r = LookupMatcher(available_locales, requested_locales);
  } else {
    // Step 3a.
    r = BestFitMatcher(available_locales, requested_locales);
  }
  // Step 4.
  prop = StrToVal("locale");
  __jsvalue found_locale = __jsop_getprop(&r, &prop); // TODO: check!
  // Step 5a.
  prop = StrToVal("extension");
  __jsvalue extension = __jsop_getprop(&r, &prop);
  __jsvalue extension_subtags;
  __jsvalue extension_subtags_length;
  __jsvalue extension_index;
  if (!__is_undefined(&extension)) {
    // Step 5b.
    prop = StrToVal("extensionIndex");
    extension_index = __jsop_getprop(&r, &prop);
    // Step 5c-5d.
    __jsvalue separator = StrToVal("-");
    __jsvalue limit = __undefined_value();
    extension_subtags = __jsstr_split(&extension, &separator, &limit);
    prop = StrToVal("length");
    extension_subtags = __jsop_getprop(&extension_subtags, &prop);
    // Step 5e.
    prop = StrToVal("length");
    extension_subtags_length = __jsop_getprop(&extension_subtags, &prop);
  }
  // Step 6.
  __jsobject *result_obj = __js_new_obj_obj_0();
  __jsvalue result = __object_value(result_obj);
  // Step 7.
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
    // Step 11a.
    //prop = StrToVal(std::to_string(i));
    //__jsvalue key = __jsop_getprop(relevant_extension_keys, &prop);
    __jsvalue key = __jsarr_GetElem(__jsval_to_object(relevant_extension_keys), i);
    // Step 11b.
    prop = StrToVal("localeData");
    __jsvalue found_locale_data = __jsop_getprop(&found_locale, &prop); // TODO: failed here.
    // Step 11c.
    __jsvalue key_locale_data = __jsop_getprop(&found_locale_data, &key);
    // Step 11d.
    prop = StrToVal("0");
    __jsvalue value = __jsop_getprop(&key_locale_data, &prop);
    // Step 11e.
    __jsvalue supported_extension_addition = StrToVal("");
    // Step 11g.
    if (!__is_undefined(&extension_subtags)) {
      // Step 11g i.
      __jsvalue key_pos = __jsstr_indexOf(&extension_subtags, &key, 0);
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
          __jsvalue value_pos = __jsstr_indexOf(&key_locale_data, &prop, 0);
          // Step 11g ii 2b.
          if (__jsval_to_number(&value_pos) != -1) {
            // Step 11g ii 2b i.
            value = StrToVal("true");
          }
        }
      }
    }
    // Step 11h i.
    prop = StrToVal("key");
    __jsvalue options_value = __jsop_getprop(options, &prop);
    if (!__is_undefined(&options_value)) {
      // Step 11h ii.
      __jsvalue idx = __jsstr_indexOf(&key_locale_data, &options_value, 0);
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
    // Step 11h j.
    __jsstr_concat(&supported_extension_addition, &supported_extension, 1);
    // Step 11h k.
    i++;
  }
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
  int len = __jsarr_getIndex(requested_locales);
  // Step 2.
  __jsvalue subset = __object_value(__js_new_arr_internal(0));
  // Step 3.
  int k = 0;
  // Step 4.
  while (k < len) {
    // Step 4a.
    __jsobject *o = __jsval_to_object(requested_locales);
    __jsvalue locale = __jsarr_GetElem(o, 0);
    // Step 4b.
    __jsvalue no_extensions_locale = RemoveUnicodeExtensions(&locale);
    // Step 4c.
    __jsvalue available_locale = BestAvailableLocale(available_locales, &no_extensions_locale);
    // Step 4d.
    if (!__is_undefined(&available_locale)) {
      __jsarr_pt_concat(&subset, &locale, 1);
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
  __jsvalue matcher;
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
  __jsvalue subset;
  if (__is_undefined(&matcher) || matcher_str == "best fit") {
    // Step 2a.
    subset = BestFitSupportedLocales(available_locales, requested_locales);
  } else {
    // Step 3.
    subset = LookupSupportedLocales(available_locales, requested_locales);
  }
  // Step 4.
  __jsvalue list = __jsobj_getOwnPropertyNames(&subset, &subset); // 1st arguement not used.
  int size = __jsarr_getIndex(&list);
  for (int i = 0; i < size; i++) {
    // Step 4a.
    __jsobject *o = __jsval_to_object(&subset);
    __jsvalue p = __jsarr_GetElem(o, i);
    //__jsprop_desc d = __jsobj_internal_GetProperty(o, &p);
    // Step 4b-4c.
    __jsvalue attrs = __number_value(JSPROP_DESC_HAS_VWUEUC); // TODO: check!
    // Step 4d.
    __jsobj_defineProperty(&subset, &subset, &p, &attrs); // 1st argument not used.
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
  // Step 2.
  if (!__is_undefined(&value)) {
    // Step 2a.
    __jsstring *jsstr_type = __jsval_to_string(type);
    char *str_type = jsstr_type->x.ascii;
    MAPLE_JS_ASSERT(str_type == "boolean" || str_type == "string");
    // Step 2b and 2c.
    // No need to call ToBoolean(value) or ToString(value).
    // Step 2d.
    if (!__is_undefined(values)) {
      __jsvalue idx = __jsarr_pt_indexOf(values, &value, 0);
      if (__jsval_to_number(&idx) != -1) {
        MAPLE_JS_RANGEERROR_EXCEPTION();
      }
      return value;
    }
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

// ECMA-402 1.0 10.1.1.1
// InitializeCollator(collator, locales, options)
void InitializeCollator(__jsvalue *collator, __jsvalue *locales,
                        __jsvalue *options) {
  // Step 1.
  __jsvalue prop = StrToVal("initializedIntlObject");
  __jsvalue v = __jsop_getprop(collator, &prop);
  if (!__is_undefined(&v)) {
    if (__is_boolean(&v) && __jsval_to_boolean(&v) == true) {
      MAPLE_JS_TYPEERROR_EXCEPTION();
    }
  }
  // Step 2.
  v = __boolean_value(true);
  __jsop_setprop(collator, &prop, &v);
  // Step 3.
  __jsvalue requested_locales = CanonicalizeLocaleList(locales);
  // Step 4.
  if (__is_undefined(options)) {
    // Step 4a.
    __jsobject *o = __js_new_obj_obj_0();
    *options = __object_value(o);
  }
}

// ECMA-402 1.0 10.2.2
// Intl.Collator.supportedLocalesOf(locales [, options])
__jsvalue __jsintl_CollatorSupportedLocalesOf(__jsvalue *locales,
                                              __jsvalue *arg_list,
                                              uint32_t nargs) {
  // Step 1.
  __jsvalue options;
  if (nargs == 1) {
    options = __undefined_value();
  } else {
    options = arg_list[1];
  }
  // Step 2.
  __jsvalue prop = StrToVal("availableLocales");
  __jsvalue collator_init; // TODO
  __jsvalue available_locales = __jsop_getprop(&collator_init, &prop);
  // Step 3.
  __jsvalue requested_locales = CanonicalizeLocaleList(locales);
  // Step 4.
  return SupportedLocales(&available_locales, &requested_locales, &options);
}

// ECMA-402 1.0 10.3.2
// Intl.Collator.prototype.compare
__jsvalue __jsintl_CollatorCompare(__jsvalue *this_arg, __jsvalue *arg_list,
                                   uint32_t nargs) {
  // TODO: not implemented yet.
  return __null_value();
}

// ECMA-402 1.0 10.3.3
// Intl.Collator.prototype.resolvedOptions()
__jsvalue __jsintl_CollatorResolvedOptions(__jsvalue *this_arg,
                                           __jsvalue *arg_list, uint32_t nargs) {
  // TODO: not implemented yet.
  return __null_value();
}

// ECMA-402 1.0 11.1.1.1
// InitializeNumberFormat(numberFormat, locales, options)
void InitializeNumberFormat(__jsvalue *number_format, __jsvalue *locales,
                            __jsvalue *options) {
  // Step 1.
  __jsvalue prop = StrToVal("initializedIntlObject");
  if (!__is_undefined(number_format)) {
    __jsvalue init = __jsop_getprop(number_format, &prop);
    if (__is_boolean(&init) && __jsval_to_boolean(&init) == true) {
      MAPLE_JS_TYPEERROR_EXCEPTION();
    }
  }
  // Step 2.
  __jsvalue value = __boolean_value(true);
  __jsop_setprop(number_format, &prop, &value);
  // Step 3.
  __jsvalue requested_locales = CanonicalizeLocaleList(locales);
  // Step 4.
  __jsobject *options_obj;
  if (__is_undefined(options)) {
    options_obj = __js_new_obj_obj_0();
    *options = __object_value(options_obj);
  }
  // Step 6.
  __jsobject *opt_obj = __js_new_obj_obj_0();
  __jsvalue opt = __object_value(opt_obj);
  // Step 7.
  __jsvalue property = StrToVal("property");
  __jsvalue type = StrToVal("string");
  __jsvalue values = StrVecToVal({"lookup", "best fit"});
  __jsvalue fallback = StrToVal("best fit");
  __jsvalue matcher = GetOption(options, &property, &type, &values, &fallback);
  // Step 8.
  prop = StrToVal("localeMatcher");
  __jsop_setprop(&opt, &prop, &matcher);
  // Step 9.
  __jsobject *number_format_obj = __create_object();
  number_format_obj->object_class = JSINTL;
  number_format_obj->extensible = true;
  number_format_obj->object_type = JSREGULAR_OBJECT;
  number_format_obj->shared.intl = (__jsintl*) VMMallocGC(sizeof(__jsintl));
  number_format_obj->shared.intl->kind = JSINTL_NUMBERFORMAT;
  __jsvalue number_format_init = __object_value(number_format_obj);
  // TODO: set 'availableLocales', 'relevantExtensionKeys' as "nu", and 'localeData'.
  InitProperty(&number_format_init, "availableLocales");
  InitProperty(&number_format_init, "relevantExtensionKeys");
  InitProperty(&number_format_init, "localeData");
  // Step 10.
  prop = StrToVal("localeData");
  __jsvalue locale_data = __jsop_getprop(&number_format_init, &prop);
  // Step 11.
  prop = StrToVal("availableLocales");
  __jsvalue available_locales = __jsop_getprop(&number_format_init, &prop);
  prop = StrToVal("relevantExtensionKeys");
  __jsvalue relevant_extension_keys = __jsop_getprop(&number_format_init, &prop);
  __jsvalue r = ResolveLocale(&available_locales, &requested_locales,
                              &opt, &relevant_extension_keys, &locale_data);
  // Step 12.
  prop = StrToVal("locale");
  value = __jsop_getprop(&r, &prop);
  __jsop_setprop(number_format, &prop, &value);
  // Step 13.
  prop = StrToVal("nu");
  value = __jsop_getprop(&r, &prop);
  prop = StrToVal("numberingSystem");
  __jsop_setprop(number_format, &prop, &value);
  // Step 14.
  prop = StrToVal("dataLocale");
  __jsvalue data_locale = __jsop_getprop(&r, &prop);
  // Step 15.
  property = StrToVal("style");
  type = StrToVal("string");
  values = StrVecToVal({"decimal", "percent", "currency"});
  fallback = StrToVal("decimal");
  __jsvalue s = GetOption(options, &property, &type, &values, &fallback);
  // Step 16.
  prop = StrToVal("style");
  __jsop_setprop(number_format, &prop, &s);
  // Step 17.
  property = StrToVal("currency");
  type = StrToVal("string");
  values = __undefined_value();
  fallback = __undefined_value();
  __jsvalue c = GetOption(options, &property, &type, &values, &fallback);
  // Step 18.
  if (!__is_undefined(&c) && !IsWellFormedCurrencyCode(&c)) {
    MAPLE_JS_RANGEERROR_EXCEPTION();
  }
  // Step 19.
  std::string s_str = ValToStr(&s);
  if (s_str == "currency" && __is_undefined(&c)) {
    MAPLE_JS_TYPEERROR_EXCEPTION();
  }
  // Step 20.
  __jsvalue c_digits;
  if (s_str == "currency") {
    // Step 20a.
    c = __jsstr_toUpperCase(&c);
    // Step 20b.
    prop = StrToVal("currency");
    __jsop_setprop(number_format, &prop, &c);
    // Step 20c.
    c_digits = CurrencyDigits(&c);
  }
  // Step 21.
  property = StrToVal("currencyDisplay");
  type = StrToVal("string");
  values = StrVecToVal({"code", "symbol", "name"});
  fallback = StrToVal("symbol");
  __jsvalue cd = GetOption(options, &property, &type, &values, &fallback);
  // Step 22.
  if (s_str == "currency") {
    prop = StrToVal("currencyDisplay");
    __jsop_setprop(number_format, &prop, &cd);
  }
  // Step 23.
  property = StrToVal("minimumIntegerDigits");
  __jsvalue minimum = __number_value(1);
  __jsvalue maximum = __number_value(21);
  fallback = __number_value(1);
  __jsvalue mnid = GetNumberOption(options, &property, &minimum, &maximum, &fallback);
  // Step 24.
  prop = StrToVal("minimumIntegerDigits");
  __jsop_setprop(number_format, &prop, &mnid);
  // Step 25.
  __jsvalue mnfd_default;
  if (s_str == "currency") {
    mnfd_default = c_digits;
  } else {
    mnfd_default = __number_value(0);
  }
  // Step 26.
  property = StrToVal("minimumFractionDigits");
  minimum = __number_value(0);
  maximum = __number_value(20);
  fallback = mnfd_default;
  __jsvalue mnfd = GetNumberOption(options, &property, &minimum, &maximum, &fallback);
  // Step 27.
  prop = StrToVal("minimumFractionDigits");
  __jsop_setprop(number_format, &prop, &mnfd);
  // Step 28.
  __jsvalue mxfd_default;
  if (s_str == "currency") {
    mxfd_default = __jsmath_pt_max(&mnfd, &c_digits, 1);
  } else if (s_str == "percent") {
    __jsvalue zero = __number_value(0);
    mxfd_default = __jsmath_pt_max(&mnfd, &zero, 1);
  } else {
    __jsvalue three = __number_value(3);
    mxfd_default = __jsmath_pt_max(&mnfd, &three, 1);
  }
  // Step 29.
  property = StrToVal("maximumFractionDigits");
  maximum = mnfd;
  minimum = __number_value(20);
  fallback = mxfd_default;
  __jsvalue mxfd = GetNumberOption(options, &property, &minimum, &maximum, &fallback);
  // Step 30.
  prop = StrToVal("maximumFractionDigits");
  __jsop_setprop(number_format, &prop, &mxfd);
  // Step 31.
  prop = StrToVal("minimumSignificantDigits");
  __jsvalue mnsd = __jsop_getprop(options, &prop);
  // Step 32.
  prop = StrToVal("maximumSignificantDigits");
  __jsvalue mxsd = __jsop_getprop(options, &prop);
  // Step 33.
  if (!__is_undefined(&mnsd) || !__is_undefined(&mxsd)) {
    // Step 33a.
    property = StrToVal("minimumSignificantDigits");
    minimum = __number_value(1);
    maximum = __number_value(21);
    fallback = __number_value(1);
    mnsd = GetNumberOption(options, &property, &minimum, &maximum, &fallback);
    // Step 33b.
    property = StrToVal("maximumSignificantDigits");
    minimum = mnsd;
    maximum = __number_value(21);
    fallback = __number_value(21);
    mxsd = GetNumberOption(options, &property, &minimum, &maximum, &fallback);
    // Step 33c.
    prop = StrToVal("minimumSignificantDigits");
    __jsop_setprop(number_format, &prop, &mnsd);
    prop = StrToVal("maximumSignificantDigits");
    __jsop_setprop(number_format, &prop, &mxsd);
  }
  // Step 34.
  property = StrToVal("useGrouping");
  type = StrToVal("boolean");
  values = __undefined_value();
  fallback = __boolean_value(true);
  __jsvalue g = GetOption(options, &property, &type, &values, &fallback);
  // Step 35.
  prop = StrToVal("useGrouping");
  __jsop_setprop(number_format, &prop, &g);
  // Step 36.
  prop = StrToVal("dataLocale");
  __jsvalue data_locale_data;
  if (!__is_undefined(&locale_data)) {
    data_locale_data = __jsop_getprop(&locale_data, &prop);
  } else {
    data_locale_data = __undefined_value();
  }
  // Step 37.
  prop = StrToVal("patterns");
  __jsvalue patterns;
  if (!__is_undefined(&locale_data)) {
    patterns = __jsop_getprop(&locale_data, &prop);
  } else {
    patterns = __undefined_value();
  }
  // Step 38.
  MAPLE_JS_ASSERT(__is_js_object(&patterns)); // TODO: failed here.
  // initial Intl.NumberFormat, locale_data, patterns not set.
  // Step 39.
  prop = s;
  __jsvalue style_patterns;
  if (!__is_undefined(&patterns)) {
    style_patterns = __jsop_getprop(&patterns, &prop);
  } else {
    style_patterns = __undefined_value();
  }
  // Step 40.
  prop = StrToVal("positivePattern");
  if (!__is_undefined(&style_patterns)) {
    value = __jsop_getprop(&style_patterns, &prop);
  } else {
    value = __undefined_value();
  }
  __jsop_setprop(number_format, &prop, &value);
  // Step 41.
  prop = StrToVal("negativePattern");
  if (!__is_undefined(&style_patterns)) {
    value = __jsop_getprop(&style_patterns, &prop);
  } else {
    value = __undefined_value();
  }
  __jsop_setprop(number_format, &prop, &value);
  // Step 42.
  prop = StrToVal("boundFormat");
  value = __undefined_value();
  __jsop_setprop(number_format, &prop, &value);
  // Step 43.
  prop = StrToVal("initializedNumberFormat");
  value = __boolean_value(true);
  __jsop_setprop(number_format, &prop, &value);
}

// ECMA-402 1.0 11.1.1.1
__jsvalue CurrencyDigits(__jsvalue *currency) {
  // TODO: not implemented yet.
  return __null_value();
}

// ECMA-402 1.0 11.2.2
// Intl.NumberFormat.supportedLocalesOf(locales [, options])
__jsvalue __jsintl_NumberFormatSupportedLocalesOf(__jsvalue *this_arg,
                                                  __jsvalue *arg_list,
                                                  uint32_t nargs) {
  __jsvalue locales = *this_arg;
  // Step 1.
  __jsvalue options;
  if (nargs == 1) {
    options = __undefined_value();
  } else {
    options = arg_list[1];
  }
  // Step 2.
  __jsvalue available_locales; // TODO: initial value of "availableLocales" of Intl.DataTimeFormat.
  // Step 3.
  __jsvalue requested_locales = CanonicalizeLocaleList(&locales);
  // Step 4.
  return SupportedLocales(&available_locales, &requested_locales, &options);
}

// ECMA-402 1.0 11.3.2
// Intl.NumberFormat.prototype.format
__jsvalue __jsintl_NumberFormatFormat(__jsvalue *number_format, __jsvalue *arg_list,
                                      uint32_t nargs) {
  // Step 1.
  __jsvalue prop = StrToVal("boundFormat");
  __jsvalue bound_format = __jsop_getprop(number_format, &prop);
  if (__is_undefined(&bound_format)) {
    // Step 1a.
    void *func = (void*) FormatNumber;
    //__jsvalue f = __js_new_function(func, NULL,
    __jsvalue length_value = __number_value(1);
    //__jsobj_helper_init_value_property(f, JSBUILTIN_STRING_LENGTH, &length_value, JSPROP_DESC_HAS_VUWUEC);
    __jsvalue value;
    prop = StrToVal("value");
    // TODO
  }
  return __jsop_getprop(number_format, &prop);
}

// ECMA-402 1.0 11.3.3
// Intl.NumberFormat.prototype.resolvedOptions()
__jsvalue __jsintl_NumberFormatResolvedOptions(__jsvalue *this_arg,
                                               __jsvalue *arg_list,
                                               uint32_t nargs) {
  // TODO: not implemented yet.
  return __null_value();
}

// ECMA-402 1.0 12.1.1.1
// InitializeDateTimeFormat(dateTimeFormat, locales, options)
void InitializeDateTimeFormat(__jsvalue *date_time_format, __jsvalue *locales,
                              __jsvalue *options) {
  // Step 1.
  __jsvalue prop = StrToVal("initializedIntlObject");
  if (!__is_undefined(date_time_format)) {
    __jsvalue init = __jsop_getprop(date_time_format, &prop);
    if (__is_boolean(&init) && __jsval_to_boolean(&init) == true) {
      MAPLE_JS_TYPEERROR_EXCEPTION();
    }
  }
  // Step 2.
  __jsvalue value = __boolean_value(true);
  __jsop_setprop(date_time_format, &prop, &value);
  // Step 3.
  __jsvalue requested_locales = CanonicalizeLocaleList(locales);
  // Step 4.
  __jsvalue required = StrToVal("any");
  __jsvalue default_val = StrToVal("date");
  *options = ToDateTimeOptions(options, &required, &default_val);
  // Step 5.
  __jsobject *opt_obj = __js_new_obj_obj_0();
  __jsvalue opt = __object_value(opt_obj);
  // Step 6.
  __jsvalue property = StrToVal("localMatcher");
  __jsvalue type = StrToVal("string");
  __jsvalue values = StrVecToVal({"lookup", "best fit"});
  __jsvalue fallback = StrToVal("best fit");
  __jsvalue matcher = GetOption(options, &property, &type, &values, &fallback);
  // Step 7.
  prop = StrToVal("localeMatcher");
  __jsop_setprop(&opt, &prop, &matcher);
  // Step 8.
  __jsvalue date_time_format_init; // TODO
  // Step 9.
  prop = StrToVal("localeData");
  __jsvalue locale_data = __jsop_getprop(&date_time_format_init, &prop);
  // Step 10.
  prop = StrToVal("availableLocales");
  __jsvalue available_locales = __jsop_getprop(&date_time_format_init, &prop);
  prop = StrToVal("relevantExtensionKeys");
  __jsvalue relevant_extension_keys = __jsop_getprop(&date_time_format_init, &prop);
  __jsvalue r = ResolveLocale(&available_locales, &requested_locales,
                              &opt, &relevant_extension_keys, &locale_data);
  // Step 11.
  prop = StrToVal("dateTimeFormat");
  value = __jsop_getprop(&r, &prop);
  __jsop_setprop(date_time_format, &prop, &value);
  // Step 12.
  prop = StrToVal("ca");
  value = __jsop_getprop(&r, &prop);
  prop = StrToVal("calendar");
  __jsop_setprop(date_time_format, &prop, &value);
  // Step 13.
  prop = StrToVal("nu");
  value = __jsop_getprop(&r, &prop);
  prop = StrToVal("numberingSystem");
  __jsop_setprop(date_time_format, &prop, &value);
  // Step 14.
  prop = StrToVal("dataLocale");
  __jsvalue data_locale = __jsop_getprop(&r, &prop);
  // Step 15.
  prop = StrToVal("timeZone");
  __jsvalue tz = __jsop_getprop(options, &prop);
  // Step 16.
  if (!__is_undefined(&tz)) {
    // Step 16a.
    std::string tz_str = ValToStr(&tz);
    // Step 16b.
    std::transform(tz_str.begin(), tz_str.end(), tz_str.begin(), ::toupper);
    if (tz_str != "UTC") {
      MAPLE_JS_RANGEERROR_EXCEPTION();
    }
  }
  // Step 17.
  prop = StrToVal("timeZone");
  __jsop_setprop(date_time_format, &prop, &tz);
  // Step 18.
  opt_obj = __js_new_obj_obj_0();
  opt = __object_value(opt_obj);
  // Step 19.
  auto it = kDateTimeFormatComponents.begin();
  while (it != kDateTimeFormatComponents.end()) {
    // Step 19a.
    std::string p = it->first;
    prop = StrToVal(p);
    // Step 19b.
    std::vector<std::string> v = it->second;
    __jsvalue type = StrToVal("string");
    __jsvalue values = StrVecToVal(v);
    __jsvalue fallback = __undefined_value();
    value = GetOption(options, &prop, &type, &values, &fallback);
    // Step 19c.
    __jsop_setprop(&opt, &prop, &value);
  }
  // Step 20.
  prop = StrToVal("dataLocale");
  __jsvalue data_locale_data = __jsop_getprop(&locale_data, &prop);
  // Step 21.
  prop = StrToVal("formats");
  __jsvalue formats = __jsop_getprop(&data_locale_data, &prop);
  // Step 22.
  property = StrToVal("formatMatcher");
  type = StrToVal("string");
  values = StrVecToVal({"basic", "best fit"});
  fallback = StrToVal("best fit");
  matcher = GetOption(options, &property, &type, &values, &fallback);
  // Step 23.
  std::string matcher_str = ValToStr(&matcher);
  __jsvalue best_format;
  if (matcher_str == "basic") {
    // Step 23a.
    best_format = BasicFormatMatcher(&opt, &formats);
  } else {
    // Step 24.
    best_format = BestFitFormatMatcher(&opt, &formats);
  }
  // Step 25.
  it = kDateTimeFormatComponents.begin();
  while (it != kDateTimeFormatComponents.end()) {
    // Step 25a.
    std::string p = it->first;
    prop = StrToVal(p);
    // Step 25b.
    __jsvalue p_desc = __jsobj_getOwnPropertyNames(&best_format, &prop);
    // Step 25c.
    if (!__is_undefined(&p_desc)) {
      // Step 25c i.
      __jsvalue p = __jsop_getprop(&best_format, &prop);
      // Step 25c ii.
      __jsop_setprop(date_time_format, &prop, &p);
    }
  }
  // Step 26.
  property = StrToVal("hour12");
  type = StrToVal("boolean");
  values = __undefined_value();
  fallback = __undefined_value();
  __jsvalue hr12 = GetOption(options, &property, &type, &values, &fallback);
  // Step 27.
  prop = StrToVal("hour");
  value = __jsop_getprop(date_time_format, &prop);
  __jsvalue pattern;
  if (!__is_null_or_undefined(&value)) {
    // Step 27a.
    if (__is_undefined(&hr12)) {
      prop = StrToVal("hour12");
      hr12 = __jsop_getprop(&data_locale_data, &prop);
    }
    // Step 27b.
    __jsop_setprop(date_time_format, &prop, &hr12);
    // Step 27c.
    if (__jsval_to_boolean(&hr12) == true) {
      // Step 27c i.
      prop = StrToVal("hourNo0");
      __jsvalue hour_no_0 = __jsop_getprop(&data_locale_data, &prop);
      // Step 27c ii.
      __jsop_setprop(date_time_format, &prop, &hour_no_0);
      // Step 27c iii.
      prop = StrToVal("pattern12");
      pattern = __jsop_getprop(&best_format, &prop);
    } else {
      prop = StrToVal("pattern");
      pattern = __jsop_getprop(&best_format, &prop);
    }
  } else {
    // Step 28.
    prop = StrToVal("pattern");
    pattern = __jsop_getprop(&best_format, &prop);
  }
  // Step 29.
  __jsop_setprop(date_time_format, &prop, &pattern);
  // Step 30.
  prop = StrToVal("boundFormat");
  value = __undefined_value();
  __jsop_setprop(date_time_format, &prop, &value);
  // Step 31.
  prop = StrToVal("initializedDateTimeFormat");
  value = __boolean_value(true);
  __jsop_setprop(date_time_format, &prop, &value);
}

// ECMA-402 1.0 12.1.1.1
__jsvalue ToDateTimeOptions(__jsvalue *options, __jsvalue *required, __jsvalue *defaults) {
  // Step 1.
  if (__is_undefined(options)) {
    *options = __null_value();
  }
  // Step 2.
  __jsvalue create; //  = __jsobj_create(); // TODO
  // Step 3.
  *options = __jsfun_pt_call(&create, options, 1);
  // Step 4.
  __jsvalue need_defaults = __boolean_value(true);
  // Step 5.
  std::string required_str = ValToStr(required);
  std::vector<std::string> vprops;
  __jsvalue prop, value;
  if (required_str == "date" || required_str == "any") {
    // Step 5a.
    vprops = {"weekday", "year", "day"};
    for (int i = 0; i < vprops.size(); i++) {
      // Step 5a i.
      prop = StrToVal(vprops[i]);
      value = __jsop_getprop(options, &prop);
      if (!__is_undefined(&value)) {
        need_defaults = __boolean_value(false);
      }
    }
  }
  // Step 6.
  if (required_str == "time" || required_str == "any") {
    // Step 6a.
    vprops = {"hour", "minute", "second"};
    for (int i = 0; i < vprops.size(); i++) {
      // Step 6a i.
      prop = StrToVal(vprops[i]);
      value = __jsop_getprop(options, &prop);
      if (!__is_undefined(&value)) {
        need_defaults = __boolean_value(false);
      }
    }
  }
  // Step 7.
  std::string defaults_str = ValToStr(defaults);
  __jsvalue v;
  __jsprop_desc desc;
  __jsobject *o;
  if (__jsval_to_boolean(&need_defaults) == true &&
      (defaults_str == "date" || defaults_str == "all")) {
    // Step 7a.
    vprops = {"year", "month", "day"};
    for (int i = 0 ; i < vprops.size(); i++) {
      prop = StrToVal(vprops[i]);
      // Step 7a i.
      o = __jsval_to_object(options);
      v = StrToVal("numeric");
      desc = __new_value_desc(&v, JSPROP_DESC_HAS_VWEC);
      __jsobj_internal_DefineOwnProperty(o, &prop, desc, false);
    }
  }
  // Step 8.
  if (__jsval_to_boolean(&need_defaults) == true &&
      (defaults_str == "time" || defaults_str == "all'")) {
    // Step 8a.
    vprops = {"hour", "minute", "second"};
    for (int i = 0; i < vprops.size(); i++) {
      // Step 8a i.
      o = __jsval_to_object(options);
      v = StrToVal("numeric");
      desc = __new_value_desc(&v, JSPROP_DESC_HAS_VWEC);
      __jsobj_internal_DefineOwnProperty(o, &prop, desc, false);
    }
  }
  return *options;
}

// ECMA-402 1.0 12.1.1.1
__jsvalue BasicFormatMatcher(__jsvalue *options, __jsvalue *formats) {
  // Step 1.
  int removal_penalty = 120;
  // Step 2.
  int addition_penalty = 20;
  // Step 3.
  int long_less_penalty = 8;
  // Step 4.
  int long_more_penalty = 6;
  // Step 5.
  int short_less_penalty = 6;
  // Step 6.
  int short_more_penalty = 3;
  // Step 7.
  int best_score = (int)-INFINITY;
  // Step 8.
  __jsvalue best_format = __undefined_value();
  // Step 9.
  int i = 0;
  // Step 10.
  __jsvalue prop = StrToVal("length");
  __jsvalue len = __jsop_getprop(formats, &prop);
  // Step 11.
  while (i < __jsval_to_number(&len)) {
    // Step 11a.
    prop = StrToVal("formats");
    __jsvalue format = __jsop_getprop(formats, &prop);
    // Step 11b.
    int score = 0;
    // Step 11c.
    auto it = kDateTimeFormatComponents.begin();
    while (it != kDateTimeFormatComponents.end()) {
      // Step 11c i.
      std::string p = it->first;
      __jsvalue property = StrToVal(p);
      __jsvalue options_prop = __jsop_getprop(options, &property);
      // Step 11c ii.
      __jsvalue format_prop_desc = __jsobj_getOwnPropertyNames(formats, &property);
      // Step 11c iii.
      if (!__is_undefined(&format_prop_desc)) {
        // Step 11c iii 1.
        __jsvalue format_prop = __jsop_getprop(&format, &property);
        // Step 11c iv.
        if (__is_undefined(&options_prop) && !__is_undefined(&format_prop)) {
          score -= addition_penalty;
        } else if (!__is_undefined(&options_prop) && __is_undefined(&format_prop)) {
          // Step 11c v.
          score -= removal_penalty;
        } else {
          // Step 11c vi 1.
          std::vector<std::string> v = {"2-digit", "numeric", "narrow", "short", "long"};
          __jsvalue values = StrVecToVal(v);
          // Step 11c vi 2.
          __jsvalue options_prop_index = __jsarr_pt_indexOf(&values, &options_prop, 0);
          // Step 11c vi 3.
          __jsvalue format_prop_index = __jsarr_pt_indexOf(&values, &format_prop, 0);
          // Step 11c vi 4.
          int delta = std::max(std::min(__jsval_to_number(&format_prop_index) -
                                        __jsval_to_number(&options_prop_index), 2), -2);
          // Step 11c vi 5.
          if (delta == 2) {
            score -= long_more_penalty;
          } else if (delta == 1) {
            // Step 11c vi 6.
            score -= short_more_penalty;
          } else if (delta == -1) {
            // Step 11c vi 7.
            score -= short_less_penalty;
          } else if (delta == -2) {
            // Step 11c vi 8.
            score -= long_less_penalty;
          }
        }
      }
    }
    // Step 11d.
    if (score > best_score) {
      // Step 11d i.
      best_score = score;
      // Step 11d ii.
      best_format = format;
    }
    // Step 11e.
    i--;
  }
  // Step 12.
  return best_format;
}

__jsvalue BestFitFormatMatcher(__jsvalue *options, __jsvalue *formats) {
  return BasicFormatMatcher(options, formats);
}

// ECMA-402 1.0 12.2.2
// Intl.DateTimeFormat.supportedLocalesOf(locales [, options])
__jsvalue __jsintl_DateTimeFormatSupportedLocalesOf(__jsvalue *locales,
                                                    __jsvalue *arg_list,
                                                    uint32_t nargs) {
  // Step 1.
  __jsvalue options;
  if (nargs == 1) {
    options = __undefined_value();
  } else {
    options = arg_list[0];
  }
  // Step 2.
  __jsvalue prop = StrToVal("availableLocales");
  __jsvalue date_time_format_init; // TODO
  __jsvalue available_locales = __jsop_getprop(&date_time_format_init, &prop);
  // Step 3.
  __jsvalue requested_locales = CanonicalizeLocaleList(&options);
  // Step 4.
  return SupportedLocales(&available_locales, &requested_locales, &options);
}

// ECMA-402 1.0 12.3.2
// Intl.DateTimeFormat.prototype.format
__jsvalue __jsintl_DateTimeFormatFormat(__jsvalue *this_arg,
                                        __jsvalue *arg_list, uint32_t nargs) {
  // TODO: not implemented yet.
  return __null_value();
}

// ECMA-402 1.0 12.3.3
// Intl.DateTimeFormat.prototype.resolvedOptions()
__jsvalue __jsintl_DateTimeFormatResolvedOptions(__jsvalue *this_arg,
                                                 __jsvalue *arg_list,
                                                 uint32_t nargs) {
  // TODO: not implemented yet.
  return __null_value();
}

// ECMA-402 6,2,4
// DefaultLocale()
__jsvalue DefaultLocale() {
  char *locale = getenv("LANG");
  if (!locale || !strcmp(locale, "C"))
    locale = const_cast<char*>("und");
  std::string l(locale);

  return StrToVal(l);
}

// ECMA-402 11.3.2
// FormatNumber(numberFormat, x)
__jsvalue FormatNumber(__jsvalue *number_format, __jsvalue *x) {
  // Step 1.
  __jsvalue negative = __boolean_value(false);
  // Step 2.
  if (__is_infinity(x) == true) {
    // Step 2a.
    if (__is_nan(x)) {
    }
  }
  // TODO
  return __null_value();
}

void InitProperty(__jsvalue *object, std::string prop) {
  __jsobject *obj = __jsval_to_object(object);
  if (prop == "availableLocales") {
    int n = uloc_countAvailable();
    std::vector<std::string> locs;
    for (int i = 0; i < n; i++) {
      std::string str(uloc_getAvailable(i));
      std::replace(str.begin(), str.end(), '_', '-');
      locs.push_back(str);
    }
    __jsvalue p = StrToVal(prop);
    __jsvalue v = StrVecToVal(locs);
    __jsop_setprop(object, &p, &v);
  } else if (prop == "relevantExtensionKeys") {
    __jsvalue p = StrToVal(prop);
    std::vector<std::string> values;
    switch (obj->shared.intl->kind) {
      case JSINTL_COLLATOR:
        values = {"co", "kn", "kf"};
        break;
      case JSINTL_NUMBERFORMAT:
        values = {"nu"};
        break;
      case JSINTL_DATETIMEFORMAT:
        values = {"ca", "nu"};
        break;
    }
    __jsvalue v = StrVecToVal(values);
    __jsop_setprop(object, &p, &v);
  } else if (prop == "localeData") {
    __jsintl_type kind = obj->shared.intl->kind;
    if (kind == JSINTL_COLLATOR) {
      // TODO
    } else if (kind == JSINTL_COLLATOR) {
      // TODO
    } else if (kind == JSINTL_NUMBERFORMAT) {
      __jsvalue p = StrToVal("nu");
      // Add default system numbering system as the first element of vec.
      icu::NumberingSystem num_sys;
      std::string default_nu(num_sys.getName());
      std::vector<std::string> vec = {"arab", "arabext", "bali", "beng",
                                      "deva", "fullwide", "gujr", "guru",
                                      "hanidec", "khmr", "knda", "laoo",
                                      "latn", "limb", "mlym", "mong",
                                      "mymr", "orya", "tamldec", "telu",
                                      "thai", "tibt"};
      vec.insert(vec.begin(), default_nu);
      __jsvalue v = StrVecToVal(vec);
      __jsop_setprop(object, &p, &v);
    } else if (kind == JSINTL_DATETIMEFORMAT) {
      for (auto it = kDateTimeFormatComponents.begin();
          it != kDateTimeFormatComponents.end(); it++) {
        __jsvalue p = StrToVal(it->first);
        __jsvalue v = StrVecToVal(it->second);
        __jsop_setprop(object, &p, &v);
      }
    }
  } else if (prop == "sortLocaleData") {
    // TODO
  } else if (prop == "searchLocaleData") {
    // TODO
  }
}
