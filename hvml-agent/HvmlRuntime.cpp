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
               &m_archetype_part,
               &m_iterate_part,
               &m_init_part,
               &m_observe_part);
}

HvmlRuntime::~HvmlRuntime()
{
    if (m_vdom) hvml_dom_destroy(m_vdom);
    if (m_udom) hvml_dom_destroy(m_udom);
    m_archetype_part.clear();
    m_iterate_part.clear();
    m_init_part.clear();
    m_observe_part.clear();
}
