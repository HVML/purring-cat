#include "interpreter/interpreter_to_three_part.h"

#include<algorithm> // for_each

Interpreter_to_ThreePart::Interpreter_to_ThreePart(hvml_dom_t** html_part, 
                                                   InitGroup_t* init_part,
                                                   ObserveGroup_t* observe_part)
{
    m_traverse_param.lvl = -1;
    m_traverse_param.html_part = html_part;
    m_traverse_param.init_part = init_part;
    m_traverse_param.observe_part = observe_part;
}

void Interpreter_to_ThreePart::ReleaseThreePart(hvml_dom_t** html_part,
                                                InitGroup_t* init_part,
                                                ObserveGroup_t* observe_part)
{
    hvml_dom_destroy(*html_part);

    for_each(init_part->begin(),
             init_part->end(),
             [](hvml_dom_t *dom)->void{ hvml_dom_destroy(dom); });
    init_part->clear();

    for_each(observe_part->begin(),
             observe_part->end(),
             [](observe_t &item)->void{ hvml_dom_destroy(item.ptr_action_group); });
    observe_part->clear();
}

void Interpreter_to_ThreePart::DumpHtmlPart(hvml_dom_t** html_part,
                                            FILE *html_part_f)
{
}

void Interpreter_to_ThreePart::DumpInitPart(InitGroup_t* init_part,
                                            FILE *init_part_f)
{

}

void Interpreter_to_ThreePart::DumpObservePart(ObserveGroup_t* observe_part,
                                               FILE *observe_part_f)
{

}

void Interpreter_to_ThreePart::GetOutput(hvml_dom_t *input_dom, 
                                         hvml_dom_t** html_part,
                                         InitGroup_t* init_part,
                                         ObserveGroup_t* observe_part)
{
    Interpreter_to_ThreePart ipter(html_part, init_part, observe_part);
    hvml_dom_traverse(input_dom, &ipter.m_traverse_param, ipter.traverse_for_divide);
}

// tag_open_close: 1-open, 2-single-close, 3-half-close, 4-close
//int hvml_dom_traverse(hvml_dom_t *dom,
//                      void *arg,
//                      void (*traverse_cb)(hvml_dom_t *dom,
//                                          int lvl,
//                                          int tag_open_close,
//                                          void *arg,
//                                          int *breakout));

void Interpreter_to_ThreePart::traverse_for_divide(hvml_dom_t *dom,
                                                   int lvl,
                                                   int tag_open_close,
                                                   void *arg,
                                                   int *breakout)
{
    TraverseParam_t *param = (TraverseParam_t*)arg;
    //A(parg, "internal logic error");

    // *breakout = 0;

    // switch (hvml_dom_type(dom)) {
    //     case MKDOT(D_TAG):
    //     {
    //         switch (tag_open_close) {
    //             case 1: {
    //                 fprintf(parg->out, "<%s", hvml_dom_tag_name(dom));
    //             } break;
    //             case 2: {
    //                 fprintf(parg->out, "/>");
    //             } break;
    //             case 3: {
    //                 fprintf(parg->out, ">");
    //             } break;
    //             case 4: {
    //                 fprintf(parg->out, "</%s>", hvml_dom_tag_name(dom));
    //             } break;
    //             default: {
    //                 A(0, "internal logic error");
    //             } break;
    //         }
    //         parg->lvl = lvl;
    //     } break;
    //     case MKDOT(D_ATTR):
    //     {
    //         A(parg->lvl >= 0, "internal logic error");
    //         const char *key = hvml_dom_attr_key(dom);
    //         const char *val = hvml_dom_attr_val(dom);
    //         fprintf(parg->out, " ");
    //         fprintf(parg->out, "%s", key);
    //         if (val) {
    //             fprintf(parg->out, "=\"");
    //             hvml_dom_attr_val_serialize(val, strlen(val), parg->out);
    //             fprintf(parg->out, "\"");
    //         }
    //     } break;
    //     case MKDOT(D_TEXT):
    //     {
    //         const char *text = hvml_dom_text(dom);
    //         hvml_dom_str_serialize(text, strlen(text), parg->out);
    //         parg->lvl = lvl;
    //     } break;
    //     case MKDOT(D_JSON):
    //     {
    //         hvml_jo_value_printf(hvml_dom_jo(dom), parg->out);
    //         parg->lvl = lvl;
    //     } break;
    //     default: {
    //         A(0, "internal logic error");
    //     } break;
    // }
}
