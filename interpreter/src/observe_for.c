#include "interpreter/observe_for.h"
#include <string.h>

static char *observe_for_type_flag[] = {
    "*",
    "abort",      //图像的加载被中断
    "blur",       //元素失去焦点
    "change",     //域的内容被改变
    "click",      //当用户点击某个对象时调用的事件句柄
    "dblclick",   //当用户双击某个对象时调用的事件句柄
    "error",      //在加载文档或图像时发生错误
    "focus",      //元素获得焦点
    "keydown",    //某个键盘按键被按下
    "keypress",   //某个键盘按键被按下并松开
    "keyup",      //某个键盘按键被松开
    "load",       //一张页面或一幅图像完成加载
    "mousedown",  //鼠标按钮被按下
    "mousemove",  //鼠标被移动
    "mouseout",   //鼠标从某元素移开
    "mouseover",  //鼠标移到某元素之上
    "mouseup",    //鼠标按键被松开
    "reset",      //重置按钮被点击
    "resize",     //窗口或框架被重新调整大小
    "select",     //文本被选中
    "submit",     //确认按钮被点击
    "unload",     //用户退出页面
};

#define OBSERVE_FOR_FLAG_COUNT (sizeof(observe_for_type_flag) / sizeof(observe_for_type_flag[0]))

OBSERVE_FOR_TYPE get_observe_for_type(hvml_string_t str)
{
    int i;
    for (i = 0; i < OBSERVE_FOR_FLAG_COUNT; i ++) {
        if (0 == strncmp(observe_for_type_flag[i], str.str, str.len)) {
            return i;
        }
    }

    return for_UNKNOWN;
}

const char* observe_for_to_string(OBSERVE_FOR_TYPE type)
{
    if (type < 0 || type >= for_UNKNOWN) return NULL;
    return observe_for_type_flag[type];
}
