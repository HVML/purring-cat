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
             [](init_t &item)->void{ hvml_dom_destroy(item.ptr_data_group); });
    init_part->clear();

    for_each(observe_part->begin(),
             observe_part->end(),
             [](observe_t &item)->void{ hvml_dom_destroy(item.ptr_action_group); });
    observe_part->clear();
}

void Interpreter_to_ThreePart::DumpHtmlPart(hvml_dom_t** html_part,
                                            FILE *html_part_f)
{
    hvml_dom_printf(*html_part, html_part_f);
}

void Interpreter_to_ThreePart::DumpInitPart(InitGroup_t* init_part,
                                            FILE *init_part_f)
{
    for_each(init_part->begin(),
             init_part->end(),
             [&](init_t& item)->void{

                 fprintf(init_part_f, "<init ");

                 if (item.s_init_as.size() > 0) {
                    fprintf(init_part_f, "as=\"%s\" ", item.s_init_as);
                 }

                 if (item.s_init_uniquely_by.size() > 0) {
                     fprintf(init_part_f, "uniquely by=\"%s\"", item.s_init_uniquely_by);
                 }

                 if (adv_UNKNOWN != item.en_init_adverb) {
                     fprintf(init_part_f, " %s",
                             adverb_to_string(item.en_init_adverb));
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

                 if (item.s_observe_on.size() > 0) {
                    fprintf(observe_part_f, "on=\"%s\" ", item.s_observe_on);
                 }

                 if (for_UNKNOWN != item.en_observe_for) {
                     fprintf(observe_part_f, "for=\"%s\"",
                             observe_for_to_string(item.en_observe_for));
                 }

                 if (item.s_observe_to.size() > 0) {
                     fprintf(observe_part_f, "to=\"%s\" ", item.s_observe_to);
                 }

                 fprintf(observe_part_f, ">\n");
                 hvml_dom_printf(item.ptr_action_group, observe_part_f);
                 fprintf(observe_part_f, "</observe>\n");
             });
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
    A(param, "internal logic error");

    *breakout = 0;

    // switch (hvml_dom_type(dom)) {
    //     case MKDOT(D_TAG):
    //     {
    //         switch (tag_open_close) {
    //             case 1: {
    //                 //fprintf(param->out, "<%s", hvml_dom_tag_name(dom));

    //                 if (0 == strcmp("init", hvml_dom_tag_name(dom))) {
    //                     param->
    //                 }
    //                 else if (0 == strcmp("observe", hvml_dom_tag_name(dom))) {

    //                 }
    //                 else {

    //                 }




    //             } break;
    //             case 2: {
    //                 //fprintf(param->out, "/>");
    //             } break;
    //             case 3: {
    //                 //fprintf(param->out, ">");
    //             } break;
    //             case 4: {
    //                 //fprintf(param->out, "</%s>", hvml_dom_tag_name(dom));
    //             } break;
    //             default: {
    //                 A(0, "internal logic error");
    //             } break;
    //         }
    //         param->lvl = lvl;
    //     } break;
    //     case MKDOT(D_ATTR):
    //     {
    //         A(param->lvl >= 0, "internal logic error");
    //         const char *key = hvml_dom_attr_key(dom);
    //         const char *val = hvml_dom_attr_val(dom);
    //         fprintf(param->out, " ");
    //         fprintf(param->out, "%s", key);
    //         if (val) {
    //             fprintf(param->out, "=\"");
    //             hvml_dom_attr_val_serialize(val, strlen(val), param->out);
    //             fprintf(param->out, "\"");
    //         }
    //     } break;
    //     case MKDOT(D_TEXT):
    //     {
    //         const char *text = hvml_dom_text(dom);
    //         hvml_dom_str_serialize(text, strlen(text), param->out);
    //         param->lvl = lvl;
    //     } break;
    //     case MKDOT(D_JSON):
    //     {
    //         hvml_jo_value_printf(hvml_dom_jo(dom), param->out);
    //         param->lvl = lvl;
    //     } break;
    //     default: {
    //         A(0, "internal logic error");
    //     } break;
    // }
}
