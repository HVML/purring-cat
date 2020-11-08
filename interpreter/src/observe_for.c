// This file is a part of Purring Cat, a reference implementation of HVML.
//
// Copyright (C) 2020, <liuxinouc@126.com>.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

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

#define OBSERVE_FOR_STRING_LEN_MAX 10
#define OBSERVE_FOR_FLAG_COUNT (sizeof(observe_for_type_flag) / sizeof(observe_for_type_flag[0]))

OBSERVE_FOR_TYPE get_observe_for_type(const char *str)
{
    int i;
    for (i = 0; i < OBSERVE_FOR_FLAG_COUNT; i ++) {
        if (0 == strncmp(observe_for_type_flag[i], str, OBSERVE_FOR_STRING_LEN_MAX)) {
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
