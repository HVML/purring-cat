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

#include "interpreter/interpreter_runtime.h"
#include "interpreter/ext_tools.h"
#include <string.h>

#include<algorithm> // for_each

typedef struct TraverseParam_s {
    hvml_dom_t**      udom_pptr;
    hvml_dom_t*       udom_curr_ptr;
    hvml_dom_t*       vdom_ignore;
    MustacheGroup_t*  mustache_part;
    ArchetypeGroup_t* archetype_part;
    IterateGroup_t*   iterate_part;
    InitGroup_t*      init_part;
    ObserveGroup_t*   observe_part;

    TraverseParam_s(hvml_dom_t**      udom_part_in,
                    MustacheGroup_t*  mustache_part_in,
                    ArchetypeGroup_t* archetype_part_in,
                    IterateGroup_t*   iterate_part_in,
                    InitGroup_t*      init_part_in,
                    ObserveGroup_t*   observe_part_in)
    : udom_pptr(udom_part_in)
    , udom_curr_ptr(NULL)
    , vdom_ignore(NULL)
    , mustache_part(mustache_part_in)
    , archetype_part(archetype_part_in)
    , iterate_part(iterate_part_in)
    , init_part(init_part_in)
    , observe_part(observe_part_in)
    {}
} TraverseParam_t;

void Interpreter_Runtime::DumpUdomPart(hvml_dom_t* udom,
                                       FILE *udom_part_f)
{
    hvml_dom_printf(udom, udom_part_f);
}

void Interpreter_Runtime::DumpMustachePart(MustacheGroup_t* mustache_part,
                                           FILE *mustache_part_f)
{
    for_each(mustache_part->begin(),
             mustache_part->end(),
             [&](mustache_t& item)->void{

                 fprintf(mustache_part_f, "{{ ");

                 if (! hvml_string_is_empty(&item.s_inner_str)) {
                    fprintf(mustache_part_f, "%s", item.s_inner_str.str);
                 }

                 fprintf(mustache_part_f, " }}\n");
                 
                 hvml_dom_t *dom = item.vdom;
                 switch (hvml_dom_type(dom))
                 {
                     case MKDOT(D_ATTR): {
                        const char *key = hvml_dom_attr_key(dom);
                        const char *val = hvml_dom_attr_val(dom);
                        fprintf(mustache_part_f, "%s=\"%s\"\n", key, val);
                     }
                     break;

                     case MKDOT(D_TEXT): {
                         const char *text = hvml_dom_text(dom);
                         fprintf(mustache_part_f, "%s\n", text);
                     }
                     break;
                 }
             });
}

void Interpreter_Runtime::DumpArchetypePart(ArchetypeGroup_t* archetype_part,
                                            FILE *archetype_part_f)
{
    for_each(archetype_part->begin(),
             archetype_part->end(),
             [&](archetype_t& item)->void{

                 fprintf(archetype_part_f, "<archetype");

                 if (! hvml_string_is_empty(&item.s_id)) {
                    fprintf(archetype_part_f, " id=\"%s\"", item.s_id.str);
                 }

                 fprintf(archetype_part_f, ">\n");
                 hvml_dom_printf(item.vdom, archetype_part_f);
                 fprintf(archetype_part_f, "\n</archetype>\n\n");
             });
}

void Interpreter_Runtime::DumpIteratePart(IterateGroup_t* iterate_part,
                                          FILE *iterate_part_f)
{
    for_each(iterate_part->begin(),
             iterate_part->end(),
             [&](iterate_t& item)->void{

                 fprintf(iterate_part_f, "<iterate");

                 if (! hvml_string_is_empty(&item.s_on)) {
                    fprintf(iterate_part_f, " on=\"%s\"", item.s_on.str);
                 }

                 if (! hvml_string_is_empty(&item.s_with)) {
                     fprintf(iterate_part_f, " with=\"%s\"", item.s_with.str);
                 }

                 if (! hvml_string_is_empty(&item.s_to)) {
                     fprintf(iterate_part_f, " to=\"%s\"", item.s_to.str);
                 }

                 fprintf(iterate_part_f, ">\n");
                 hvml_dom_printf(item.vdom, iterate_part_f);
                 fprintf(iterate_part_f, "\n</iterate>\n\n");
             });
}

void Interpreter_Runtime::DumpInitPart(InitGroup_t* init_part,
                                       FILE *init_part_f)
{
    for_each(init_part->begin(),
             init_part->end(),
             [&](init_t& item)->void{

                 fprintf(init_part_f, "<init");

                 if (! hvml_string_is_empty(&item.s_as)) {
                    fprintf(init_part_f, " as=\"%s\"", item.s_as.str);
                 }

                 if (adv_UNKNOWN != item.en_adverb) {
                     fprintf(init_part_f, " %s",
                             adverb_to_string(item.en_adverb));
                 }

                 if (! hvml_string_is_empty(&item.s_by)) {
                     fprintf(init_part_f, " by=\"%s\"", item.s_by.str);
                 }

                 fprintf(init_part_f, ">\n");
                 hvml_dom_printf(item.vdom, init_part_f);
                 fprintf(init_part_f, "\n</init>\n\n");
             });
}

void Interpreter_Runtime::DumpObservePart(ObserveGroup_t* observe_part,
                                          FILE *observe_part_f)
{
    for_each(observe_part->begin(),
             observe_part->end(),
             [&](observe_t& item)->void{

                 fprintf(observe_part_f, "<observe");

                 if (! hvml_string_is_empty(&item.s_on)) {
                    fprintf(observe_part_f, " on=\"%s\"", item.s_on.str);
                 }

                 if (for_UNKNOWN != item.en_for) {
                     fprintf(observe_part_f, " for=\"%s\"",
                             observe_for_to_string(item.en_for));
                 }

                 if (! hvml_string_is_empty(&item.s_to)) {
                     fprintf(observe_part_f, " to=\"%s\"", item.s_to.str);
                 }

                 fprintf(observe_part_f, ">\n");
                 hvml_dom_printf(item.vdom, observe_part_f);
                 fprintf(observe_part_f, "\n</observe>\n\n");
             });
}

void Interpreter_Runtime::GetRuntime(hvml_dom_t* input_dom, 
                                     hvml_dom_t** udom_part,
                                     MustacheGroup_t* mustache_part,
                                     ArchetypeGroup_t* archetype_part,
                                     IterateGroup_t* iterate_part,
                                     InitGroup_t* init_part,
                                     ObserveGroup_t* observe_part)
{
    TraverseParam_t traverse_param(udom_part,
                                   mustache_part,
                                   archetype_part,
                                   iterate_part,
                                   init_part,
                                   observe_part);
    hvml_dom_traverse(input_dom,
                      &traverse_param,
                      Interpreter_Runtime::traverse_for_divide);

    A(traverse_param.udom_curr_ptr == *(traverse_param.udom_pptr), 
        "internal logic error");
}

void Interpreter_Runtime::traverse_for_divide(hvml_dom_t *dom,
                                              int lvl,
                                              int tag_open_close,
                                              void *arg,
                                              int *breakout)
{
    TraverseParam_t *param = (TraverseParam_t*)arg;
    A(param, "internal logic error");

    *breakout = 0;

    switch (hvml_dom_type(dom)) {
        case MKDOT(D_TAG):
        {
            const char* tag_name = hvml_dom_tag_name(dom);
            switch (tag_open_close) {
                case 1: { //fprintf(param->out, "<%s", hvml_dom_tag_name(dom));

                    if (param->vdom_ignore) {
                        I("----- (ignore) <%s> ---", tag_name);
                        break;
                    }
                    I("----- done to %s", tag_name);

                    if (0 == strcmp("archetype", tag_name)) {
                        I("----- <archetype> ---");
                        AddNewArchetype(param->archetype_part,
                                        dom,
                                        param->udom_curr_ptr);
                        param->vdom_ignore = dom;
                    }
                    else if (0 == strcmp("iterate", tag_name)) {
                        I("----- <iterate> ---");
                        AddNewIterate(param->iterate_part,
                                      dom,
                                      param->udom_curr_ptr);
                        param->vdom_ignore = dom;
                    }
                    else if (0 == strcmp("init", tag_name)) {
                        I("----- <init> ---");
                        AddNewInit(param->init_part, dom);
                        param->vdom_ignore = dom;
                    }
                    else if (0 == strcmp("observe", tag_name)) {
                        I("----- <observe> ---");
                        AddNewObserve(param->observe_part, dom);
                        param->vdom_ignore = dom;
                    }
                    else {
                        I("----- A <udom-%s> ---", tag_name);
                        hvml_dom_t* u = hvml_dom_add_tag(param->udom_curr_ptr,
                                        tag_name, strlen(tag_name));
                        A(u, "internal logic error");
                        param->udom_curr_ptr = u;
                        if (0 == strcmp("hvml", tag_name)) {
                            *(param->udom_pptr) = u;
                        }
                    }
 
                } break;

                case 3: {
                    //fprintf(param->out, ">");
                } break;

                case 2:   //fprintf(param->out, "/>");
                case 4: { //fprintf(param->out, "</%s>", hvml_dom_tag_name(dom));
                    if (param->vdom_ignore) {
                        if (param->vdom_ignore == dom) {
                            param->vdom_ignore = NULL;
                            I("----- Cancel ignore <%s> ---",
                              hvml_dom_tag_name(dom));
                        }
                        else {
                            I("----- (ignore) <%s> ---", tag_name);
                        }
                        break;
                    }
                    I("----- up from <udom-%s>", hvml_dom_tag_name(param->udom_curr_ptr));
                    if (param->udom_curr_ptr != *(param->udom_pptr)) {
                        param->udom_curr_ptr = hvml_dom_parent(param->udom_curr_ptr);
                    }
                    I("----- up to <udom-%s>", hvml_dom_tag_name(param->udom_curr_ptr));
                } break;

                default: {
                    A(0, "internal logic error");
                } break;
            }
        } break;

        case MKDOT(D_ATTR): {
            
            if (param->vdom_ignore) {
                I("----- (ignore) ATTR %s ---", hvml_dom_attr_key(dom));
                break;
            }
            I("----- ATTR %s ---", hvml_dom_attr_key(dom));

            const char *key = hvml_dom_attr_key(dom);
            const char *val = hvml_dom_attr_val(dom);
            A(key, "internal logic error");
            hvml_dom_t* u = hvml_dom_append_attr(param->udom_curr_ptr,
                            key, strlen(key), val, val ? strlen(val) : 0);
            A(u, "internal logic error");

            size_t mustache_str_len;
            const char *mustache_str = find_mustache(val, &mustache_str_len);
            if (mustache_str) {
                AddNewMustache(param->mustache_part,
                               mustache_str + 2, // skip the "{{"
                               mustache_str_len - 2,
                               dom,
                               param->udom_curr_ptr,
                               u);
            }
        } break;

        case MKDOT(D_TEXT): {
            
            if (param->vdom_ignore) {
                I("----- (ignore) TEXT ---");
                break;
            }
            I("----- TEXT ---");

            const char *text = hvml_dom_text(dom);
            A(text, "internal logic error");
            hvml_dom_t* u = hvml_dom_append_content(param->udom_curr_ptr,
                            text, strlen(text));
            A(u, "internal logic error");

            size_t mustache_str_len;
            const char *mustache_str = find_mustache(text, &mustache_str_len);
            if (mustache_str) {
                AddNewMustache(param->mustache_part,
                               mustache_str + 2, // skip the "{{"
                               mustache_str_len - 2,
                               dom,
                               param->udom_curr_ptr,
                               u);
            }
        } break;

        case MKDOT(D_JSON): {
            
            if (param->vdom_ignore) {
                I("----- (ignore) JOSN ---");
                break;
            }
            I("----- JOSN ---");

            hvml_jo_value_t *jo = hvml_dom_jo(dom);
            A(jo, "internal logic error");
            jo = hvml_jo_clone(jo);
            A(jo, "internal logic error");
            hvml_dom_t* v = hvml_dom_append_json(param->udom_curr_ptr, jo);
            A(v, "internal logic error");
        } break;

        default: {
            A(0, "internal logic error");
        } break;
    }
}

void Interpreter_Runtime::AddNewMustache(MustacheGroup_t* mustache_part,
                                         const char* str_inner,
                                         size_t      str_inner_len,
                                         hvml_dom_t* vdom,
                                         hvml_dom_t* udom_owner,
                                         hvml_dom_t* udom)
{
    mustache_t new_mustache(str_inner,
                            str_inner_len,
                            vdom,
                            udom_owner,
                            udom);
    mustache_part->push_back(new_mustache);
}

void Interpreter_Runtime::AddNewArchetype(ArchetypeGroup_t* archetype_part,
                                          hvml_dom_t* vdom,
                                          hvml_dom_t* udom_owner)
{
    archetype_t new_archetype;

    hvml_dom_t *attr = hvml_dom_attr_head(vdom);
    while (attr) {
        const char *key = hvml_dom_attr_key(attr);
        const char *val = hvml_dom_attr_val(attr);
        if (0 == strcmp("id", key)) {
            hvml_string_set(&new_archetype.s_id, val, strlen(val));
        }
        attr = hvml_dom_attr_next(attr);
    }
    new_archetype.vdom = hvml_dom_child(vdom);
    new_archetype.udom_owner = udom_owner;
    archetype_part->push_back(new_archetype);
}

void Interpreter_Runtime::AddNewIterate(IterateGroup_t* iterate_part,
                                        hvml_dom_t* vdom,
                                        hvml_dom_t* udom_owner)
{
    iterate_t new_iterate;

    hvml_dom_t *attr = hvml_dom_attr_head(vdom);
    while (attr) {
        const char *key = hvml_dom_attr_key(attr);
        const char *val = hvml_dom_attr_val(attr);

        if (0 == strcmp("on", key)) {
            hvml_string_set(&new_iterate.s_on, val, strlen(val));
        }
        else if (0 == strcmp("with", key)) {
            hvml_string_set(&new_iterate.s_with, val, strlen(val));
        }
        else if (0 == strcmp("to", key)) {
            hvml_string_set(&new_iterate.s_to, val, strlen(val));
        }
        attr = hvml_dom_attr_next(attr);
    }
    new_iterate.vdom = hvml_dom_child(vdom);
    new_iterate.udom_owner = udom_owner;
    iterate_part->push_back(new_iterate);
}

void Interpreter_Runtime::AddNewInit(InitGroup_t* init_part,
                                     hvml_dom_t* vdom)
{
    init_t new_init;

    hvml_dom_t *attr = hvml_dom_attr_head(vdom);
    while (attr) {
        const char *key = hvml_dom_attr_key(attr);
        const char *val = hvml_dom_attr_val(attr);
        if (0 == strcmp("as", key)) {
            hvml_string_set(&new_init.s_as, val, strlen(val));
        }
        else if (0 == strcmp("by", key)) {
            hvml_string_set(&new_init.s_by, val, strlen(val));
        }
        else {
            new_init.en_adverb = get_adverb_type(key);
        }
        attr = hvml_dom_attr_next(attr);
    }
    new_init.vdom = hvml_dom_child(vdom);
    init_part->push_back(new_init);
}

void Interpreter_Runtime::AddNewObserve(ObserveGroup_t* observe_part,
                                        hvml_dom_t* vdom)
{
    observe_t new_observe;

    hvml_dom_t *attr = hvml_dom_attr_head(vdom);
    while (attr) {
        const char *key = hvml_dom_attr_key(attr);
        const char *val = hvml_dom_attr_val(attr);
        if (0 == strcmp("on", key)) {
            hvml_string_set(&new_observe.s_on, val, strlen(val));
        }
        else if (0 == strcmp("to", key)) {
            hvml_string_set(&new_observe.s_to, val, strlen(val));
        }
        else if (0 == strcmp("for", key)) {
            new_observe.en_for = get_observe_for_type(val);
        }
        attr = hvml_dom_attr_next(attr);
    }

    new_observe.vdom = hvml_dom_child(vdom);
    observe_part->push_back(new_observe);
}
