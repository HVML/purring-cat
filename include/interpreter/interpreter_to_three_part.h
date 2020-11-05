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

#include <vector>
using namespace std;


typedef struct observe_s {
    hvml_string_t s_observe_on;
    hvml_string_t s_observe_to;
    OBSERVE_FOR_TYPE en_observe_for;
    hvml_dom_t* ptr_action_group;
} observe_t;

typedef vector<hvml_dom_t> InitGroup;
typedef vector<observe_t> ObserveGroup;


class Interpreter_to_ThreePart
{
public:
    Interpreter_to_ThreePart(hvml_dom_t** html_part,
                             InitGroup* init_part,
                             ObserveGroup* observe_part);

public:
    static void ReleaseThreePart(hvml_dom_t** html_part,
                                 InitGroup* init_part,
                                 ObserveGroup* observe_part);

    static void GetOutput(hvml_dom_t *input_dom,
                          hvml_dom_t** html_part,
                          InitGroup* init_part,
                          ObserveGroup* observe_part);
};
