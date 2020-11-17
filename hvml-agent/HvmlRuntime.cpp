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

#include "HvmlRuntime.h"

HvmlRuntime::HvmlRuntime(FILE *hvml_in_f)
: m_vdom(NULL)
, m_udom(NULL)
{
    m_vdom = hvml_dom_load_from_stream(hvml_in_f);
    GetRuntime(m_vdom,
               &m_udom,
               &m_mustache_part,
               &m_archetype_part,
               &m_iterate_part,
               &m_init_part,
               &m_observe_part);
}

HvmlRuntime::~HvmlRuntime()
{
    if (m_vdom) hvml_dom_destroy(m_vdom);
    if (m_udom) hvml_dom_destroy(m_udom);
    m_mustache_part.clear();
    m_archetype_part.clear();
    m_iterate_part.clear();
    m_init_part.clear();
    m_observe_part.clear();
}


const char html_filename[] = "index/index.html";

size_t HvmlRuntime::GetIndexResponse(char* response,
                                     size_t response_limit)
{
    if (! m_udom) return 0;

    FILE *out = fopen(html_filename, "wb+");
    if (! out) {
        E("failed to create output file: %s", html_filename);
        
        return 0;
    }

    DumpUdomPart(m_udom, out);

    fseek(out, 0, SEEK_SET);
    size_t ret_len = fread(response, 1, response_limit, out);
    response[ret_len] = '\0';
    fclose(out);
    return ret_len;
}

void HvmlRuntime::Refresh(void)
{
    TransformMustacheGroup();
    TransformArchetypeGroup();
    TransformIterateGroup();
    TransformObserveGroup();
}

void HvmlRuntime::TransformMustacheGroup()
{
    for_each(m_mustache_part.begin(),
             m_mustache_part.end(),
             [&](mustache_t& item)->void{

                 hvml_dom_t *dom = item.vdom;
                 switch (hvml_dom_type(dom))
                 {
                     case MKDOT(D_ATTR): {
                        const char *key = hvml_dom_attr_key(dom);
                        const char *val = hvml_dom_attr_val(dom);
                     }
                     break;

                     case MKDOT(D_TEXT): {
                         const char *text = hvml_dom_text(dom);
                     }
                     break;
                 }
             });
}

void HvmlRuntime::TransformArchetypeGroup()
{
    ;
}

void HvmlRuntime::TransformIterateGroup()
{
    ;
}

void HvmlRuntime::TransformObserveGroup()
{
    ;
}
