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

#ifndef _observe_for_h_
#define _observe_for_h_

#include "hvml/hvml_string.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum {
    for_any = 0,    //*
    for_abort,      //图像的加载被中断
    for_blur,       //元素失去焦点
    for_change,     //域的内容被改变
    for_click,      //当用户点击某个对象时调用的事件句柄
    for_dblclick,   //当用户双击某个对象时调用的事件句柄
    for_error,      //在加载文档或图像时发生错误
    for_focus,      //元素获得焦点
    for_keydown,    //某个键盘按键被按下
    for_keypress,   //某个键盘按键被按下并松开
    for_keyup,      //某个键盘按键被松开
    for_load,       //一张页面或一幅图像完成加载
    for_mousedown,  //鼠标按钮被按下
    for_mousemove,  //鼠标被移动
    for_mouseout,   //鼠标从某元素移开
    for_mouseover,  //鼠标移到某元素之上
    for_mouseup,    //鼠标按键被松开
    for_reset,      //重置按钮被点击
    for_resize,     //窗口或框架被重新调整大小
    for_select,     //文本被选中
    for_submit,     //确认按钮被点击
    for_unload,     //用户退出页面
    for_UNKNOWN,
} OBSERVE_FOR_TYPE;

OBSERVE_FOR_TYPE get_observe_for_type(const char *str);
const char* observe_for_to_string(OBSERVE_FOR_TYPE type);

#ifdef __cplusplus
}
#endif

#endif //_observe_for_h_