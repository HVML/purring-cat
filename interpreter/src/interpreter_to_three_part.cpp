#include "interpreter/interpreter_to_three_part.h"
#include <string.h>

#include<algorithm> // for_each

Interpreter_to_ThreePart::Interpreter_to_ThreePart(InitGroup_t* init_part,
                                                   ObserveGroup_t* observe_part)
{
    m_traverse_param.lvl = -1;
    m_traverse_param.init_part = init_part;
    m_traverse_param.observe_part = observe_part;
}

void Interpreter_to_ThreePart::ReleaseTwoPart(InitGroup_t* init_part,
                                              ObserveGroup_t* observe_part)
{
    for_each(init_part->begin(),
             init_part->end(),
             [](init_t &item)->void{ hvml_dom_destroy(item.ptr_data_group); });
    init_part->clear();

    for_each(observe_part->begin(),
             observe_part->end(),
             [](observe_t &item)->void{ hvml_dom_destroy(item.ptr_action_group); });
    observe_part->clear();
}

void Interpreter_to_ThreePart::DomToHtml(hvml_dom_t* dom,
                                         FILE *html_part_f)
{
    hvml_dom_printf(dom, html_part_f);// waiting for change
}

void Interpreter_to_ThreePart::DumpInitPart(InitGroup_t* init_part,
                                            FILE *init_part_f)
{
    for_each(init_part->begin(),
             init_part->end(),
             [&](init_t& item)->void{

                 fprintf(init_part_f, "<init ");

                 if (item.s_as.size() > 0) {
                    fprintf(init_part_f, "as=\"%s\" ", item.s_as);
                 }

                 if (adv_UNKNOWN != item.en_adverb) {
                     fprintf(init_part_f, " %s",
                             adverb_to_string(item.en_adverb));
                 }

                 if (item.s_by.size() > 0) {
                     fprintf(init_part_f, " by=\"%s\"", item.s_by);
                 }

                 fprintf(init_part_f, ">\n");
                 hvml_dom_printf(item.ptr_data_group, init_part_f);
                 fprintf(init_part_f, "</init>\n");
             });
}

void Interpreter_to_ThreePart::DumpObservePart(ObserveGroup_t* observe_part,
                                               FILE *observe_part_f)
{
    for_each(observe_part->begin(),
             observe_part->end(),
             [&](observe_t& item)->void{

                 fprintf(observe_part_f, "<observe ");

                 if (item.s_on.size() > 0) {
                    fprintf(observe_part_f, "on=\"%s\" ", item.s_on);
                 }

                 if (for_UNKNOWN != item.en_for) {
                     fprintf(observe_part_f, "for=\"%s\"",
                             observe_for_to_string(item.en_for));
                 }

                 if (item.s_to.size() > 0) {
                     fprintf(observe_part_f, "to=\"%s\" ", item.s_to);
                 }

                 fprintf(observe_part_f, ">\n");
                 hvml_dom_printf(item.ptr_action_group, observe_part_f);
                 fprintf(observe_part_f, "</observe>\n");
             });
}

void Interpreter_to_ThreePart::GetOutput(hvml_dom_t* input_dom, 
                                         InitGroup_t* init_part,
                                         ObserveGroup_t* observe_part)
{
    Interpreter_to_ThreePart ipter(init_part, observe_part);
    hvml_dom_traverse(input_dom, &ipter.m_traverse_param, ipter.traverse_for_divide);
}

void Interpreter_to_ThreePart::traverse_for_divide(hvml_dom_t *dom,
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
            switch (tag_open_close) {
                case 1: {
                    //fprintf(param->out, "<%s", hvml_dom_tag_name(dom));

                    if (0 == strcmp("init", hvml_dom_tag_name(dom))) {
                        AddNewInit(param->init_part, dom);
                    }
                    else if (0 == strcmp("observe", hvml_dom_tag_name(dom))) {
                        AddNewObserve(param->observe_part, dom);
                    }
                } break;

                case 2: {
                    //fprintf(param->out, "/>");
                } break;
                case 3: {
                    //fprintf(param->out, ">");
                } break;
                case 4: {
                    //fprintf(param->out, "</%s>", hvml_dom_tag_name(dom));
                } break;
                default: {
                    A(0, "internal logic error");
                } break;
            }
            param->lvl = lvl;
        } break;

        case MKDOT(D_ATTR): {
        } break;

        case MKDOT(D_TEXT): {
        } break;

        case MKDOT(D_JSON): {
        } break;

        default: {
            A(0, "internal logic error");
        } break;
    }
}

void Interpreter_to_ThreePart::AddNewInit(InitGroup_t* init_part,
                                          hvml_dom_t* dom)
{
    init_t new_init;

    hvml_dom_t *attr = hvml_dom_attr_head(dom);
    while (attr) {
        const char *key = hvml_dom_attr_key(attr);
        const char *val = hvml_dom_attr_val(attr);
        if (0 == strcmp("as", key)) {
            new_init.s_as = val;
        }
        else if (0 == strcmp("by", key)) {
            new_init.s_by = val;
        }
        else {
            new_init.en_adverb = get_adverb_type(key);
        }
        attr = hvml_dom_attr_next(attr);
    }

    hvml_dom_t *child = hvml_dom_child(dom);
    if (child) {
        new_init.ptr_data_group = hvml_dom_clone(child);
    }

    init_part->push_back(new_init);
}

void Interpreter_to_ThreePart::AddNewObserve(ObserveGroup_t* observe_part,
                                             hvml_dom_t* dom)
{
    observe_t new_observe;

    hvml_dom_t *attr = hvml_dom_attr_head(dom);
    while (attr) {
        const char *key = hvml_dom_attr_key(attr);
        const char *val = hvml_dom_attr_val(attr);
        if (0 == strcmp("on", key)) {
            new_observe.s_on = val;
        }
        else if (0 == strcmp("to", key)) {
            new_observe.s_to = val;
        }
        else {
            new_observe.en_for = get_observe_for_type(key);
        }
        attr = hvml_dom_attr_next(attr);
    }

    hvml_dom_t *child = hvml_dom_child(dom);
    if (child) {
        new_observe.ptr_action_group = hvml_dom_clone(child);
    }

    observe_part->push_back(new_observe);
}
