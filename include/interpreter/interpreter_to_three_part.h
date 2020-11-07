#pragma once

#include "hvml/hvml_string.h"
#include "hvml/hvml_printf.h"
#include "hvml/hvml_parser.h"
#include "hvml/hvml_dom.h"
#include "hvml/hvml_jo.h"
#include "hvml/hvml_json_parser.h"
#include "hvml/hvml_log.h"
#include "hvml/hvml_utf8.h"

#include "observe_for.h"
#include "adverb_property.h"

#include <string>
#include <vector>
using namespace std;

typedef struct observe_s {
    string s_on;
    string s_to;
    OBSERVE_FOR_TYPE en_for;
    hvml_dom_t* ptr_action_group;

    observe_s() :
        s_on(""),
        s_to(""),
        en_for(for_UNKNOWN),
        ptr_action_group(NULL) {}
        
} observe_t;

typedef struct init_s {
    string s_as;
    string s_by;
    ADVERB_PROPERTY en_adverb;
    hvml_dom_t* ptr_data_group;

    init_s() :
        s_as(""),
        s_by(""),
        en_adverb(adv_sync),
        ptr_data_group(NULL) {}
        
} init_t;

typedef vector<init_t> InitGroup_t;
typedef vector<observe_t> ObserveGroup_t;

typedef struct TraverseParam_s {
    int             lvl;
    InitGroup_t*    init_part;
    ObserveGroup_t* observe_part;
} TraverseParam_t;

class Interpreter_to_ThreePart
{
public:
    Interpreter_to_ThreePart(InitGroup_t* init_part,
                             ObserveGroup_t* observe_part);

public:
    static void ReleaseTwoPart(InitGroup_t* init_part,
                               ObserveGroup_t* observe_part);

    static void DomToHtml(hvml_dom_t* dom,
                          FILE *html_part_f);

    static void DumpInitPart(InitGroup_t* init_part,
                             FILE *init_part_f);

    static void DumpObservePart(ObserveGroup_t* observe_part,
                                FILE *observe_part_f);

    static void GetOutput(hvml_dom_t* input_dom,
                          InitGroup_t* init_part,
                          ObserveGroup_t* observe_part);

private:
    TraverseParam_t m_traverse_param;
    static void traverse_for_divide(hvml_dom_t *dom,
                                    int lvl,
                                    int tag_open_close,
                                    void *arg,
                                    int *breakout);
    
    static void AddNewInit(InitGroup_t* init_part,
                           hvml_dom_t* dom);
    
    static void AddNewObserve(ObserveGroup_t* observe_part,
                              hvml_dom_t* dom);
};
