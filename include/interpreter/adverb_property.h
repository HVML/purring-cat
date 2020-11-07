#ifndef _adverv_property_h_
#define _adverv_property_h_

#include "hvml/hvml_string.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ascendingly：在使用内置迭代器、选择器或者规约器时，用于指定数据项的排列顺序为升序；
//   可简写为 asc。
// descendingly：在使用内置迭代器、选择器或者规约器时，用于指定数据项的排列顺序为降序；
//   可简写为 desc。
// synchronously：在 init、request、call 标签中，用于定义从外部数据源（或操作组）获取数据时采用同步请求方式；
//   默认值；可简写为 sync。
// asynchronously：在 init、request、call 标签中，用于定义从外部数据源（或操作组）获取数据时采用异步请求方式；
//   可简写为 async。
// exclusively：在 match 动作标签中，用于定义排他性；具有这一属性时，匹配当前动作时，将不再处理同级其他 match 标签；
//   可简写为 excl。
// uniquely：在 init 动作标签中，用于定义集合；具有这一属性时，init 定义的变量将具有唯一性条件；
//   可简写为 uniq。
typedef enum {
    adv_sync = 0,   //synchronously, DEFAULT
    adv_asc,        //ascendingly
    adv_desc,       //descendingly
    adv_async,      //asynchronously
    adv_excl,       //exclusively
    adv_uniq,       //uniquely
    adv_UNKNOWN,
} ADVERB_PROPERTY;

ADVERB_PROPERTY get_adverb_type(hvml_string_t str);
const char* adverb_to_string(ADVERB_PROPERTY type);
const char* adverb_to_abbreviation(ADVERB_PROPERTY type);

#ifdef __cplusplus
}
#endif

#endif //_adverv_property_h_