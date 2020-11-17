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

#ifndef _hvml_runtime_h_
#define _hvml_runtime_h_

#include "interpreter/interpreter_basic.h"
#include "interpreter/interpreter_runtime.h"

class HvmlRuntime : public Interpreter_Runtime
{
public:
    HvmlRuntime(FILE *hvml_in_f);
    ~HvmlRuntime();
    size_t GetIndexResponse(char* response,
                            size_t response_limit);

    bool Refresh(void);

private:
    hvml_dom_t *m_vdom; // origin hvml dom
    hvml_dom_t *m_udom; // dom for display
    MustacheGroup_t  m_mustache_part;
    ArchetypeGroup_t m_archetype_part;
    IterateGroup_t   m_iterate_part;
    InitGroup_t      m_init_part;
    ObserveGroup_t   m_observe_part;

private:
    void TransformMustacheGroup();
    void TransformArchetypeGroup();
    void TransformIterateGroup();
    void TransformObserveGroup();
    hvml_string_t TransformMustacheString(hvml_string_t& mustache_s);
};

#endif //_hvml_runtime_h_