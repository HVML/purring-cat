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

#ifndef _interpreter_runtime_h_
#define _interpreter_runtime_h_

#include "hvml/hvml_string.h"
#include "hvml/hvml_printf.h"
#include "hvml/hvml_parser.h"
#include "hvml/hvml_dom.h"
#include "hvml/hvml_jo.h"
#include "hvml/hvml_json_parser.h"
#include "hvml/hvml_log.h"
#include "hvml/hvml_utf8.h"

#include "ext_tools.h"
#include "observe_for.h"
#include "adverb_property.h"

#include <string.h>

#include <vector>
using namespace std;

typedef struct mustache_s {
    hvml_string_t s_inner_str;
    hvml_dom_t* vdom;
    hvml_dom_t* udom_owner;
    hvml_dom_t* udom;

    mustache_s(const char* str_inner,
               size_t      str_inner_len,
               hvml_dom_t* vdom_in,
               hvml_dom_t* udom_owner_in,
               hvml_dom_t* udom_in)
    : s_inner_str({NULL, 0})
    , vdom(vdom_in)
    , udom_owner(udom_owner_in)
    , udom(udom_in)
    {
        hvml_string_set(&s_inner_str,
                        str_inner,
                        str_inner_len);
        str_trim(s_inner_str.str);
        s_inner_str.len = strlen(s_inner_str.str);
    }
} mustache_t;

typedef struct archetype_s {
    hvml_string_t s_id;
    hvml_dom_t* vdom;
    hvml_dom_t* udom_owner;
    hvml_dom_t* udom;

    archetype_s()
    : s_id({NULL, 0})
    , vdom(NULL)
    , udom_owner(NULL)
    , udom(NULL)
    {}
} archetype_t;

typedef struct iterate_s {
    hvml_string_t s_on;
    hvml_string_t s_with;
    hvml_string_t s_to;
    hvml_dom_t* vdom;
    hvml_dom_t* udom_owner;
    hvml_dom_t* udom;

    iterate_s()
    : s_on({NULL, 0})
    , s_with({NULL, 0})
    , s_to({NULL, 0})
    , vdom(NULL)
    , udom_owner(NULL)
    , udom(NULL)
    {}
} iterate_t;

typedef struct init_s {
    hvml_string_t s_as;
    hvml_string_t s_by;
    ADVERB_PROPERTY en_adverb;
    hvml_dom_t* vdom;

    init_s()
    : s_as({NULL, 0})
    , s_by({NULL, 0})
    , en_adverb(adv_sync)
    , vdom(NULL)
    {}
} init_t;

typedef struct observe_s {
    hvml_string_t s_on;
    hvml_string_t s_to;
    OBSERVE_FOR_TYPE en_for;
    hvml_dom_t* vdom;

    observe_s()
    : s_on({NULL, 0})
    , s_to({NULL, 0})
    , en_for(for_UNKNOWN)
    , vdom(NULL)
    {}
} observe_t;

typedef vector<mustache_t>  MustacheGroup_t;
typedef vector<archetype_t> ArchetypeGroup_t;
typedef vector<iterate_t>   IterateGroup_t;
typedef vector<init_t>      InitGroup_t;
typedef vector<observe_t>   ObserveGroup_t;


class Interpreter_Runtime
{
public:
    static void DumpUdomPart(hvml_dom_t* udom,
                             FILE *udom_part_f);

    static void DumpMustachePart(MustacheGroup_t* mustache_part,
                                 FILE *mustache_part_f);

    static void DumpArchetypePart(ArchetypeGroup_t* archetype_part,
                                  FILE *archetype_part_f);

    static void DumpIteratePart(IterateGroup_t* iterate_part,
                                FILE *iterate_part_f);

    static void DumpInitPart(InitGroup_t* init_part,
                             FILE *init_part_f);

    static void DumpObservePart(ObserveGroup_t* observe_part,
                                FILE *observe_part_f);

    static void GetRuntime(hvml_dom_t*  input_dom,
                           hvml_dom_t** udom_part,
                           MustacheGroup_t* mustache_part,
                           ArchetypeGroup_t* archetype_part,
                           IterateGroup_t* iterate_part,
                           InitGroup_t* init_part,
                           ObserveGroup_t* observe_part);

private:
    static void traverse_for_divide(hvml_dom_t *dom,
                                    int lvl,
                                    int tag_open_close,
                                    void *arg,
                                    int *breakout);

    static void AddNewMustache(MustacheGroup_t* mustache_part,
                               const char* str_inner,
                               size_t      str_inner_len,
                               hvml_dom_t* vdom,
                               hvml_dom_t* udom_owner,
                               hvml_dom_t* udom);

    static void AddNewArchetype(ArchetypeGroup_t* archetype_part,
                                hvml_dom_t* vdom,
                                hvml_dom_t* udom_owner);

    static void AddNewIterate(IterateGroup_t* iterate_part,
                              hvml_dom_t* vdom,
                              hvml_dom_t* udom_owner);

    static void AddNewInit(InitGroup_t* init_part,
                           hvml_dom_t* dom);

    static void AddNewObserve(ObserveGroup_t* observe_part,
                              hvml_dom_t* dom);
};

#endif //_interpreter_runtime_h_