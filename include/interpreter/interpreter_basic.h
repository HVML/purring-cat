#pragma once

#include "hvml/hvml_printf.h"
#include "hvml/hvml_parser.h"
#include "hvml/hvml_dom.h"
#include "hvml/hvml_jo.h"
#include "hvml/hvml_json_parser.h"
#include "hvml/hvml_log.h"
#include "hvml/hvml_utf8.h"

typedef struct dom_printf_s {
    int             lvl;
    FILE           *out;
} dom_printf_t;

class Interpreter_Basic
{
public:
    Interpreter_Basic(FILE *out);
    static void GetOutput(hvml_dom_t *input_dom, FILE *out);

private:
    dom_printf_t m_dom_prtt;
    static void traverse_for_printf(hvml_dom_t *dom, int lvl, int tag_open_close, void *arg, int *breakout);
};
