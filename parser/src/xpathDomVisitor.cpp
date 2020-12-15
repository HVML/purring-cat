// This file is a part of Purring Cat, a reference implementation of HVML.
//
// Copyright (C) 2020, <freemine@yeah.net>.
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

#include "xpathDomVisitor.h"

#include "hvml/hvml_log.h"
#include "hvml/hvml_string.h"
#include "hvml_dom_xpath_parser.h"

#include <float.h>
#include <math.h>
#include <string.h>
#include <libgen.h>

__attribute__ ((format (printf, 4, 5)))
static void hvml_throw(const char *cfile, int cline, const char *cfunc, const char *fmt, ...) {
    char   buf[4096];
    int    bytes = sizeof(buf);
    char  *p     = buf;
    int    n;

    n = snprintf(p, bytes, "=%s[%d]%s()=", basename((char*)cfile), cline, cfunc);
    p += n;
    bytes -= n;

    if (bytes>0) {
        va_list arg;
        va_start(arg, fmt);
        n = vsnprintf(p, bytes, fmt, arg);
        va_end(arg);
    }

    throw std::runtime_error(buf);
}


#define T(fmt, ...)                                                                 \
do {                                                                                \
    hvml_throw(__FILE__, __LINE__, __FUNCTION__, "%s" fmt "", "", ##__VA_ARGS__);   \
    throw std::logic_error("never reached here");                                   \
} while(0)


static xpathNodeset make_nodeset() {
    hvml_doms_t *doms = (hvml_doms_t*)calloc(1, sizeof(*doms));
    if (!doms) T("out of memory");
    return std::shared_ptr<hvml_doms_t>{doms, hvml_doms_destroy};
}

typedef struct doms_collect_relative_s          doms_collect_relative_t;
struct doms_collect_relative_s {
    hvml_doms_t                 *out;
    hvml_dom_t                  *relative;
    unsigned int                 forward:1;
    unsigned int                 self:1;
    unsigned int                 immediate:1;
    unsigned int                 hit:1;
    unsigned int                 failed:1;
};

static void doms_collect_relative_cb(hvml_dom_t *dom, int lvl, int tag_open_close, void *arg, int *breakout) {
    (void)lvl;
    doms_collect_relative_t *parg = (doms_collect_relative_t*)arg;
    A(parg,               "internal logic error");
    A(parg->out,          "internal logic error");
    A(parg->relative,     "internal logic error");
    A(parg->failed==0,    "internal logic error");

    *breakout = 0;

    switch (hvml_dom_type(dom)) {
        case MKDOT(D_ROOT): break;
        case MKDOT(D_TAG):
        {
            switch (tag_open_close) {
                case 1: break;
                case 2: return;
                case 3: return;
                case 4: return;
                default: {
                    A(0, "internal logic error");
                } break;
            }
        } break;
        case MKDOT(D_ATTR): break;
        case MKDOT(D_TEXT): break;
        case MKDOT(D_JSON): break;
        default: {
            A(0, "internal logic error");
        } break;
    }

    if (dom==parg->relative) {
        A(!parg->hit, "internal logic error");
        parg->hit = 1;
        do {
            if (parg->self) {
                parg->failed = hvml_doms_append_dom(parg->out, dom);
            }
        } while (0);
        if (parg->failed) {
            *breakout    = 1;
        }
        if (!parg->forward) {
            *breakout    = 1;
        }
        return;
    }

    do {
        if (parg->forward && !parg->hit) break;
        if (!parg->immediate) {
            if (parg->forward) {
                A(parg->hit, "internal logic error");
                hvml_dom_t *p = dom;
                while (p && p!=parg->relative) {
                    p = hvml_dom_parent(p);
                }
                if (p==parg->relative) break;
            } else {
                A(parg->hit==0, "internal logic error");
                hvml_dom_t *p = parg->relative;
                while (p && p!=dom) {
                    p = hvml_dom_parent(p);
                }
                if (p==dom) break;
            }
        }

        parg->failed = hvml_doms_append_dom(parg->out, dom);
        if (parg->failed) break;
    } while (0);

    if (parg->failed) {
        *breakout = 1;
        parg->failed = 1;
    }
}

static int do_doms_append_relative(hvml_doms_t *out, int forward, int self, int immediate, hvml_dom_t *start, hvml_dom_t *dom) {
    A(out,    "internal logic error");
    A(start,  "internal logic error");
    A(dom,    "internal logic error");
    doms_collect_relative_t      collect;
    memset(&collect, 0, sizeof(collect));
    collect.out       = out;
    collect.relative  = dom;
    collect.forward   = forward;
    collect.self      = self;
    collect.immediate = immediate;
    collect.hit       = 0;
    collect.failed    = 0;

    hvml_dom_traverse(start, &collect, doms_collect_relative_cb);
    return collect.failed ? -1 : 0;
}

xpathDomVisitor::xpathDomVisitor(hvml_dom_t *dom, size_t idx, size_t size)
:dom_(dom)
,idx_(idx)
,size_(size)
,axis_(HVML_DOM_XPATH_AXIS_UNSPECIFIED)
,principal_(HVML_DOM_XPATH_PRINCIPAL_UNSPECIFIED)
{
    A(dom_,       "internal logic error");
    A(idx_<size_, "internal logic error");
}

xpathDomVisitor::~xpathDomVisitor() {
}

antlrcpp::Any xpathDomVisitor::visitMain(xpathParser::MainContext *ctx) {
    // LocationPathContext *locationPath();
    antlrcpp::Any any = visitChildren(ctx);
    if (!any.is<xpathNodeset>()) return any;
    xpathNodeset doms = any;
    xpathNodeset output = make_nodeset();
    if (hvml_doms_sort(output.get(), doms.get())) T("out of memory");
    return output;
}

antlrcpp::Any xpathDomVisitor::visitLocationPath(xpathParser::LocationPathContext *ctx) {
    // RelativeLocationPathContext *relativeLocationPath();
    // AbsoluteLocationPathNorootContext *absoluteLocationPathNoroot();
    return visitChildren(ctx);
}

antlrcpp::Any xpathDomVisitor::visitAbsoluteLocationPathNoroot(xpathParser::AbsoluteLocationPathNorootContext *ctx) {
    // antlr4::tree::TerminalNode *PATHSEP();
    // RelativeLocationPathContext *relativeLocationPath();
    // antlr4::tree::TerminalNode *ABRPATH();
    xpathNodeset doms = make_nodeset();
    if (ctx->PATHSEP()) {
        doms = stepToRoot();
    } else if (ctx->ABRPATH()) {
        A(dom_, "internal logic error");
        hvml_dom_t *root = hvml_dom_root(dom_);
        A(root, "internal logic error");
        A(hvml_dom_type(root)==MKDOT(D_ROOT), "internal logic error");
        if (hvml_doms_append_dom(doms.get(), root)) T("out of memory");
        A(doms->ndoms==1, "internal logic error");
        doms = do_collect_descendant_or_self(doms);
        A(doms->ndoms>1, "internal logic error");
        A(ctx->relativeLocationPath(), "internal logic error");
    } else {
        A(0, "internal logic error");
        // never reached here
        return xpathError();
    }

    if (doms->ndoms==0) return doms;

    if (!ctx->relativeLocationPath()) return doms;

    return do_relative(doms, ctx->relativeLocationPath());
}

antlrcpp::Any xpathDomVisitor::visitRelativeLocationPath(xpathParser::RelativeLocationPathContext *ctx) {
    // std::vector<StepContext *> step();
    // StepContext* step(size_t i);
    // std::vector<antlr4::tree::TerminalNode *> PATHSEP();
    // antlr4::tree::TerminalNode* PATHSEP(size_t i);
    // std::vector<antlr4::tree::TerminalNode *> ABRPATH();
    // antlr4::tree::TerminalNode* ABRPATH(size_t i);
    xpathNodeset doms = make_nodeset();
    int r = hvml_doms_append_dom(doms.get(), dom_);
    if (r) T("out of memory");

    for (size_t i=0; i<ctx->children.size(); ++i) {
        int visited = 0;
        auto node = ctx->children[i];
        for (size_t j=0; j<ctx->step().size(); ++j) {
            auto step = ctx->step(j);
            if (node != step) continue;
            doms = do_step(doms, step);
            if (doms->ndoms==0) return doms;
            visited = 1;
            break;
        }
        if (visited) continue;
        for (size_t j=0; j<ctx->PATHSEP().size(); ++j) {
            auto pathsep = ctx->PATHSEP(j);
            if (node != pathsep) continue;
            visited = 1;
            break;
        }
        if (visited) continue;
        for (size_t j=0; j<ctx->ABRPATH().size(); ++j) {
            auto abrpath = ctx->ABRPATH(j);
            if (node != abrpath) continue;
            antlrcpp::Any any = do_collect_descendant_or_self(doms);
            if (!any.is<xpathNodeset>()) T("expecting node set but failed");
            doms = any;
            if (doms->ndoms==0) return doms;
            visited = 1;
            break;
        }
    }

    return doms;
}

antlrcpp::Any xpathDomVisitor::visitStep(xpathParser::StepContext *ctx) {
    // AxisSpecifierContext *axisSpecifier();
    // NodeTestContext *nodeTest();
    // std::vector<PredicateContext *> predicate();
    // PredicateContext* predicate(size_t i);
    // AbbreviatedStepContext *abbreviatedStep();
    if (ctx->abbreviatedStep()) {
        return visitAbbreviatedStep(ctx->abbreviatedStep());
    }

    A(ctx->axisSpecifier(), "internal logic error");
    antlrcpp::Any any = visitAxisSpecifier(ctx->axisSpecifier());
    if (!any.is<xpathNodeset>()) T("expecting node set but failed");
    xpathNodeset doms = any;
    if (doms->ndoms==0) return doms;

    doms = do_nodetest(doms, principal_, ctx->nodeTest());
    for (size_t i=0; i<ctx->predicate().size(); ++i) {
        auto predicate = ctx->predicate(i);
        doms = do_predicate(doms, predicate);
        if (doms->ndoms==0) break;
    }

    return doms;
}

antlrcpp::Any xpathDomVisitor::visitAxisSpecifier(xpathParser::AxisSpecifierContext *ctx) {
    // antlr4::tree::TerminalNode *AxisName();
    // antlr4::tree::TerminalNode *CC();
    // antlr4::tree::TerminalNode *AT();
    if (ctx->AxisName()) {
        return do_axis(ctx->AxisName()->getText());
    }
    if (ctx->AT()) {
        return do_axis_attribute();
    }
    return do_axis_child();
}

antlrcpp::Any xpathDomVisitor::visitNodeTest(xpathParser::NodeTestContext *ctx) {
    // NameTestContext *nameTest();
    // antlr4::tree::TerminalNode *NodeType();
    // antlr4::tree::TerminalNode *LPAR();
    // antlr4::tree::TerminalNode *RPAR();
    // antlr4::tree::TerminalNode *Literal();
    // antlr4::tree::TerminalNode *DIV();
    // antlr4::tree::TerminalNode *MOD();
    if (ctx->nameTest()) {
        return visitNameTest(ctx->nameTest());
    }
    if (ctx->NodeType()) {
        const std::string &nt = ctx->NodeType()->getText();
        return do_node_type(dom_, nt);
    }
    if (ctx->Literal()) {
        W("processing-instruction: not supported yet");
        return false;
    }
    if (ctx->DIV()) {
        return do_name_test(dom_, NULL, "div");
    }
    if (ctx->MOD()) {
        return do_name_test(dom_, NULL, "mod");
    }
    A(0, "internal logic error");
    // never reached here
    return false;
}

bool xpathDomVisitor::do_predicate(const antlrcpp::Any &any) {
    bool ok = false;
    if (any.is<bool>()) {
        ok = any;
    } else if (any.is<long double>()) {
        long double number = any;
        if (fabs(number - (idx_+1))<LDBL_EPSILON) ok = true;
    } else if (any.is<std::string>()) {
        const std::string &literal = any;
        if (!literal.empty()) ok = true;
    } else if (any.is<xpathNodeset>()) {
        xpathNodeset doms = any;
        if (doms->ndoms>0) ok = true;
    } else {
        T("internal logic error");
    }
    return ok;
}

antlrcpp::Any xpathDomVisitor::visitPredicate(xpathParser::PredicateContext *ctx) {
    // antlr4::tree::TerminalNode *LBRAC();
    // ExprContext *expr();
    // antlr4::tree::TerminalNode *RBRAC();
    antlrcpp::Any any = visitExpr(ctx->expr());
    return do_predicate(any);
}

antlrcpp::Any xpathDomVisitor::visitAbbreviatedStep(xpathParser::AbbreviatedStepContext *ctx) {
    // antlr4::tree::TerminalNode *DOT();
    // antlr4::tree::TerminalNode *DOTDOT();
    xpathNodeset doms = make_nodeset();
    if (ctx->DOT()) {
        doms = do_axis("self");
    } else if (ctx->DOTDOT()) {
        doms = do_axis("parent");
    } else {
        T("internal logic error");
    }
    if (doms->ndoms==0) return doms;
    A(doms->ndoms==1, "internal logic error");
    hvml_dom_t *dom = doms->doms[0];
    if (!do_node_type(dom, "node")) return make_nodeset();
    return doms;
}

antlrcpp::Any xpathDomVisitor::visitExpr(xpathParser::ExprContext *ctx) {
    // OrExprContext *orExpr();
    return visitOrExpr(ctx->orExpr());
}

antlrcpp::Any xpathDomVisitor::visitPrimaryExpr(xpathParser::PrimaryExprContext *ctx) {
    // VariableReferenceContext *variableReference();
    // antlr4::tree::TerminalNode *LPAR();
    // ExprContext *expr();
    // antlr4::tree::TerminalNode *RPAR();
    // antlr4::tree::TerminalNode *Literal();
    // antlr4::tree::TerminalNode *Number();
    // FunctionCallContext *functionCall();
    if (ctx->variableReference()) {
        T("not implemented yet");
    }
    if (ctx->expr()) {
        return visitExpr(ctx->expr());
    }
    if (ctx->Literal()) {
        const std::string &s = ctx->Literal()->getText();
        A(s.size()>=2, "internal logic error");
        std::string literal(s.c_str()+1, s.size()-2);
        return literal;
    }
    if (ctx->Number()) {
        const std::string &number = ctx->Number()->getText();
        long double ldbl = NAN;
        hvml_string_to_number(number.c_str(), &ldbl);
        return ldbl;
    }
    if (ctx->functionCall()) {
        return visitFunctionCall(ctx->functionCall());
    }
    T("internal logic error");
}

antlrcpp::Any xpathDomVisitor::visitFunctionCall(xpathParser::FunctionCallContext *ctx) {
    // FunctionNameContext *functionName();
    // antlr4::tree::TerminalNode *LPAR();
    // antlr4::tree::TerminalNode *RPAR();
    // std::vector<ExprContext *> expr();
    // ExprContext* expr(size_t i);
    // std::vector<antlr4::tree::TerminalNode *> COMMA();
    // antlr4::tree::TerminalNode* COMMA(size_t i);
    std::tuple<std::string, std::string> funcName = visitFunctionName(ctx->functionName());
    if (!std::get<0>(funcName).empty()) {
        T("not implemented yet");
    }
    if (std::get<1>(funcName) == "position") {
        A(ctx->expr().size()==0, "internal logic error");
        return (long double)(idx_ + 1);
    }
    if (std::get<1>(funcName) == "last") {
        A(ctx->expr().size()==0, "internal logic error");
        return (long double)size_;
    }
    T("not implemented yet");
}

antlrcpp::Any xpathDomVisitor::visitUnionExprNoRoot(xpathParser::UnionExprNoRootContext *ctx) {
    // PathExprNoRootContext *pathExprNoRoot();
    // antlr4::tree::TerminalNode *PIPE();
    // UnionExprNoRootContext *unionExprNoRoot();
    // antlr4::tree::TerminalNode *PATHSEP();
    antlrcpp::Any head;

    if (ctx->PATHSEP()) {
        hvml_dom_t *root = hvml_dom_root(dom_);
        A(root && root == dom_, "internal logic error");
        xpathNodeset nodes = make_nodeset();
        if (hvml_doms_append_dom(nodes.get(), root)) T("out of memory");
        head = nodes;
    } else if (ctx->pathExprNoRoot()) {
        head = visitPathExprNoRoot(ctx->pathExprNoRoot());
    } else {
        A(0, "internal logic error");
        // never reached here
        return xpathError();
    }

    if (!ctx->unionExprNoRoot()) return head;

    antlrcpp::Any tail = visitUnionExprNoRoot(ctx->unionExprNoRoot());
    if (!head.is<xpathNodeset>()) T("expecting node set but failed");
    if (!tail.is<xpathNodeset>()) T("expecting node set but failed");

    xpathNodeset hdoms = head;
    xpathNodeset tdoms = tail;
    if (hvml_doms_append_doms(hdoms.get(), tdoms.get())) T("out of memory");
    return hdoms;
}

antlrcpp::Any xpathDomVisitor::visitPathExprNoRoot(xpathParser::PathExprNoRootContext *ctx) {
    // LocationPathContext *locationPath();
    // FilterExprContext *filterExpr();
    // RelativeLocationPathContext *relativeLocationPath();
    // antlr4::tree::TerminalNode *PATHSEP();
    // antlr4::tree::TerminalNode *ABRPATH();
    if (ctx->locationPath()) {
        xpathDomVisitor visitor(dom_, 0, 1);
        return visitor.visitLocationPath(ctx->locationPath());
    } else if (ctx->filterExpr()) {
        antlrcpp::Any any = visitFilterExpr(ctx->filterExpr());
        if (ctx->relativeLocationPath()) {
            xpathNodeset head = any;
            return do_relative(head, ctx->relativeLocationPath());
        } else {
            A(ctx->PATHSEP()==NULL, "internal logic error");
            A(ctx->ABRPATH()==NULL, "internal logic error");
            return any;
        }
    } else {
        A(0, "internal logic error");
        // never reached here
        return xpathError();
    }
}

antlrcpp::Any xpathDomVisitor::visitFilterExpr(xpathParser::FilterExprContext *ctx) {
    // PrimaryExprContext *primaryExpr();
    // std::vector<PredicateContext *> predicate();
    // PredicateContext* predicate(size_t i);
    A(ctx->primaryExpr(), "internal logic error");
    antlrcpp::Any any = visitPrimaryExpr(ctx->primaryExpr());
    if (ctx->predicate().size()==0) return any;

    xpathNodeset doms = make_nodeset();
    do {
        if (any.is<bool>()) {
            bool ok = any;
            if (!ok) return false;
            if (hvml_doms_append_dom(doms.get(), dom_)) T("out of memory");
            break;
        }
        if (any.is<std::string>()) {
            const std::string &literal = any;
            if (literal.empty()) return false;
            if (hvml_doms_append_dom(doms.get(), dom_)) T("out of memory");
            break;
        }
        if (any.is<long double>()) {
            long double ldbl = any;
            if (!(fabs(ldbl - (idx_ + 1)) < LDBL_EPSILON)) return false;
            if (hvml_doms_append_dom(doms.get(), dom_)) T("out of memory");
            break;
        }
        if (any.is<xpathNodeset>()) {
            xpathNodeset r = any;
            if (r->ndoms == 0) return false;
            if (hvml_doms_append_doms(doms.get(), r.get())) T("out of memory");
            break;
        }
        A(0, "internal logic error");
    } while (0);

    A(doms->ndoms, "internal logic error");

    for (size_t i=0; i<ctx->predicate().size(); ++i) {
        auto predicate = ctx->predicate(i);
        antlrcpp::Any any = do_predicate(doms, predicate);
        if (!any.is<xpathNodeset>()) T("expecting node set but failed");
        xpathNodeset r = any;
        if (r->ndoms==0) return false;
        doms = r;
    }

    return doms->ndoms>0 ? true : false;
}

antlrcpp::Any xpathDomVisitor::visitOrExpr(xpathParser::OrExprContext *ctx) {
    // std::vector<AndExprContext *> andExpr();
    // AndExprContext* andExpr(size_t i);
    for (size_t i=0; i<ctx->andExpr().size(); ++i) {
        antlrcpp::Any any = visitAndExpr(ctx->andExpr(i));
        if (ctx->andExpr().size()==1) return any;
        if (any.is<bool>()) {
            bool ok = any;
            if (ok) return true;
            continue;
        }
        if (any.is<std::string>()) {
            const std::string &literal = any;
            if (!literal.empty()) return true;
            continue;
        }
        if (any.is<long double>()) {
            long double ldbl = any;
            if (fabs(ldbl - (idx_ + 1)) < LDBL_EPSILON) return true;
            continue;
        }
        if (any.is<xpathNodeset>()) {
            xpathNodeset doms = any;
            bool ok = doms->ndoms > 0;
            if (ok) return true;
            continue;
        }
        A(0, "internal logic error");
    }

    return false;
}

antlrcpp::Any xpathDomVisitor::visitAndExpr(xpathParser::AndExprContext *ctx) {
    // std::vector<EqualityExprContext *> equalityExpr();
    // EqualityExprContext* equalityExpr(size_t i);
    for (size_t i=0; i<ctx->equalityExpr().size(); ++i) {
        antlrcpp::Any any = visitEqualityExpr(ctx->equalityExpr(i));
        if (ctx->equalityExpr().size()==1) return any;
        if (any.is<bool>()) {
            bool ok = any;
            if (!ok) return false;
            continue;
        }
        if (any.is<std::string>()) {
            const std::string &literal = any;
            if (literal.empty()) return false;
            continue;
        }
        if (any.is<long double>()) {
            long double ldbl = any;
            if (fabs(ldbl - (idx_ + 1)) >= LDBL_EPSILON) return false;
            continue;
        }
        if (any.is<xpathNodeset>()) {
            xpathNodeset doms = any;
            bool ok = doms->ndoms > 0;
            if (!ok) return false;
            continue;
        }
        A(0, "internal logic error");
    }
    return true;
}

antlrcpp::Any xpathDomVisitor::visitEqualityExpr(xpathParser::EqualityExprContext *ctx) {
    // std::vector<RelationalExprContext *> relationalExpr();
    // RelationalExprContext* relationalExpr(size_t i);
    // antlr4::tree::TerminalNode *EQ();
    // antlr4::tree::TerminalNode *NEQ();
    A(ctx->relationalExpr().size()>=1, "internal logic error");

    antlrcpp::Any l = visitRelationalExpr(ctx->relationalExpr(0));
    if (ctx->relationalExpr(1) == NULL) return l;
    antlrcpp::Any r = visitRelationalExpr(ctx->relationalExpr(1));
    if (ctx->EQ()) {
        return do_eq(l, r);
    } else if (ctx->NEQ()) {
        return !do_eq(l, r);
    } else {
        T("internal logic error");
    }
}

antlrcpp::Any xpathDomVisitor::visitRelationalExpr(xpathParser::RelationalExprContext *ctx) {
    // std::vector<AdditiveExprContext *> additiveExpr();
    // AdditiveExprContext* additiveExpr(size_t i);
    // antlr4::tree::TerminalNode *LESS();
    // antlr4::tree::TerminalNode *MORE_();
    // antlr4::tree::TerminalNode *LE();
    // antlr4::tree::TerminalNode *GE();
    A(ctx->additiveExpr().size()>=1, "internal logic error");

    antlrcpp::Any l = visitAdditiveExpr(ctx->additiveExpr(0));
    if (ctx->additiveExpr(1) == NULL) return l;
    antlrcpp::Any r = visitAdditiveExpr(ctx->additiveExpr(1));
    if (ctx->LESS()) {
        return do_less(l, r);
    } else if (ctx->MORE_()) {
        return do_less(r, l);
    } else if (ctx->LE()) {
        return do_lte(l, r);
    } else if (ctx->GE()) {
        return do_lte(r, l);
    } else {
        T("internal logic error");
    }
}

antlrcpp::Any xpathDomVisitor::visitAdditiveExpr(xpathParser::AdditiveExprContext *ctx) {
    // std::vector<MultiplicativeExprContext *> multiplicativeExpr();
    // MultiplicativeExprContext* multiplicativeExpr(size_t i);
    // std::vector<antlr4::tree::TerminalNode *> PLUS();
    // antlr4::tree::TerminalNode* PLUS(size_t i);
    // std::vector<antlr4::tree::TerminalNode *> MINUS();
    // antlr4::tree::TerminalNode* MINUS(size_t i);
    A(ctx->children.size()>=1, "internal logic error");
    A(ctx->children[0] == ctx->multiplicativeExpr(0), "internal logic error");
    antlrcpp::Any l = visitMultiplicativeExpr(ctx->multiplicativeExpr(0));
    if (ctx->children.size()==1) return l;
    for (size_t i=1; i<ctx->children.size(); ++i) {
        std::string op;
        do {
            int found = 0;
            for (size_t j=0; j<ctx->PLUS().size(); ++j) {
                if (ctx->children[i] != ctx->PLUS(j)) continue;
                op = "+";
                found = 1;
                break;
            }
            if (found) break;
            for (size_t j=0; j<ctx->MINUS().size(); ++j) {
                if (ctx->children[i] != ctx->MINUS(j)) continue;
                op = "-";
                found = 1;
                break;
            }
            A(found, "internal logic error");
        } while (0);

        ++i;
        A(i<ctx->children.size(), "internal logic error");

        antlrcpp::Any r;
        int found = 0;
        for (size_t j=0; j<ctx->multiplicativeExpr().size(); ++j) {
            if (ctx->children[i] != ctx->multiplicativeExpr(j)) continue;
            r = visitMultiplicativeExpr(ctx->multiplicativeExpr(j));
            found = 1;
            break;
        }
        A(found, "internal logic error");

        l = do_op(op, l, r);
    }
    return l;
}

antlrcpp::Any xpathDomVisitor::visitMultiplicativeExpr(xpathParser::MultiplicativeExprContext *ctx) {
    // std::vector<UnaryExprNoRootContext *> unaryExprNoRoot();
    // UnaryExprNoRootContext* unaryExprNoRoot(size_t i);
    // std::vector<antlr4::tree::TerminalNode *> MUL();
    // antlr4::tree::TerminalNode* MUL(size_t i);
    // std::vector<antlr4::tree::TerminalNode *> DIV();
    // antlr4::tree::TerminalNode* DIV(size_t i);
    // std::vector<antlr4::tree::TerminalNode *> MOD();
    // antlr4::tree::TerminalNode* MOD(size_t i);
    A(ctx->children.size()>=1, "internal logic error");
    A(ctx->children[0] == ctx->unaryExprNoRoot(0), "internal logic error");
    antlrcpp::Any l = visitUnaryExprNoRoot(ctx->unaryExprNoRoot(0));
    if (ctx->children.size()==1) return l;
    for (size_t i=1; i<ctx->children.size(); ++i) {
        std::string op;
        do {
            int found = 0;
            for (size_t j=0; j<ctx->MUL().size(); ++j) {
                if (ctx->children[i] != ctx->MUL(j)) continue;
                op = "*";
                found = 1;
                break;
            }
            if (found) break;
            for (size_t j=0; j<ctx->DIV().size(); ++j) {
                if (ctx->children[i] != ctx->DIV(j)) continue;
                op = "/";
                found = 1;
                break;
            }
            if (found) break;
            for (size_t j=0; j<ctx->MOD().size(); ++j) {
                if (ctx->children[i] != ctx->MOD(j)) continue;
                op = "%";
                found = 1;
                break;
            }
            if (found) break;
            A(found, "internal logic error");
        } while (0);

        ++i;
        A(i<ctx->children.size(), "internal logic error");

        antlrcpp::Any r;
        int found = 0;
        for (size_t j=0; j<ctx->unaryExprNoRoot().size(); ++j) {
            if (ctx->children[i] != ctx->unaryExprNoRoot(j)) continue;
            r = visitUnaryExprNoRoot(ctx->unaryExprNoRoot(j));
            found = 1;
            break;
        }
        A(found, "internal logic error");

        l = do_op(op, l, r);
    }
    return l;
}

antlrcpp::Any xpathDomVisitor::visitUnaryExprNoRoot(xpathParser::UnaryExprNoRootContext *ctx) {
    // UnionExprNoRootContext *unionExprNoRoot();
    // std::vector<antlr4::tree::TerminalNode *> MINUS();
    // antlr4::tree::TerminalNode* MINUS(size_t i);
    A(ctx->children.size()>=1, "internal logic error");
    A(ctx->children[0] == ctx->unionExprNoRoot(), "internal logic error");
    antlrcpp::Any l = visitUnionExprNoRoot(ctx->unionExprNoRoot());
    if (ctx->children.size()==1) return l;
    if (!l.is<long double>()) T("expecting number but failed");
    if (ctx->MINUS().size() % 2 == 0) return l;
    long double ldbl = l;
    l = -ldbl;
    return l;
}

antlrcpp::Any xpathDomVisitor::visitQName(xpathParser::QNameContext *ctx) {
    // std::vector<NCNameContext *> nCName();
    // NCNameContext* nCName(size_t i);
    // antlr4::tree::TerminalNode *COLON();
    std::string sprefix, slocal_part;
    if (ctx->COLON()) {
        antlrcpp::Any any = visitNCName(ctx->nCName(0));
        sprefix = any.as<std::string>();
        any = visitNCName(ctx->nCName(1));
        slocal_part = any.as<std::string>();
    } else {
        antlrcpp::Any any = visitNCName(ctx->nCName(0));
        slocal_part = any.as<std::string>();
    }

    return do_name_test(dom_, sprefix.empty() ? NULL : sprefix.c_str(), slocal_part.c_str());
}

antlrcpp::Any xpathDomVisitor::visitFunctionName(xpathParser::FunctionNameContext *ctx) {
    // std::vector<NCNameContext *> nCName();
    // NCNameContext* nCName(size_t i);
    // antlr4::tree::TerminalNode *COLON();
    // antlr4::tree::TerminalNode *NCName();
    // antlr4::tree::TerminalNode *AxisName();
    if (ctx->COLON()) {
        const std::string &prefix = visitNCName(ctx->nCName(0));
        const std::string &local_part = visitNCName(ctx->nCName(1));
        return std::tuple<std::string, std::string>{prefix, local_part};
    }
    if (ctx->NCName()) {
        const std::string &local_part = ctx->NCName()->getText();
        return std::tuple<std::string, std::string>{"", local_part};
    }
    if (ctx->AxisName()) {
        const std::string &local_part = ctx->AxisName()->getText();
        return std::tuple<std::string, std::string>{"", local_part};
    }
    A(0, "internal logic error");
    // never reached here
    return false;
}

antlrcpp::Any xpathDomVisitor::visitVariableReference(xpathParser::VariableReferenceContext *ctx) {
    (void)ctx;
    T("internal logic error");
}

antlrcpp::Any xpathDomVisitor::visitNameTest(xpathParser::NameTestContext *ctx) {
    // antlr4::tree::TerminalNode *MUL();
    // NCNameContext *nCName();
    // antlr4::tree::TerminalNode *COLON();
    // QNameContext *qName();
    if (ctx->MUL()) {
        return do_name_test(dom_, NULL, "*");
    }
    if (ctx->nCName()) {
        T("internal logic error");
    }
    if (ctx->qName()) {
        return visitQName(ctx->qName());
    }
    A(0, "internal logic error");
    // never reached here
    return false;
}

antlrcpp::Any xpathDomVisitor::visitNCName(xpathParser::NCNameContext *ctx) {
    // antlr4::tree::TerminalNode *NCName();
    // antlr4::tree::TerminalNode *AxisName();
    // antlr4::tree::TerminalNode *NodeType();
    if (ctx->NCName()) return ctx->NCName()->getText();
    if (ctx->AxisName()) return ctx->AxisName()->getText();
    if (ctx->NodeType()) return ctx->NodeType()->getText();
    T("internal logic error");
}







xpathNodeset xpathDomVisitor::stepToRoot(void) {
    hvml_dom_t *root = hvml_dom_root(dom_);
    A(root && root == dom_, "internal logic error");
    xpathNodeset nodes = make_nodeset();
    if (hvml_doms_append_dom(nodes.get(), root)) T("out of memory");
    return nodes;
}

xpathNodeset xpathDomVisitor::do_step(xpathNodeset &doms, xpathParser::StepContext *ctx) {
    // AxisSpecifierContext *axisSpecifier();
    // NodeTestContext *nodeTest();
    // std::vector<PredicateContext *> predicate();
    // PredicateContext* predicate(size_t i);
    // AbbreviatedStepContext *abbreviatedStep();
    xpathNodeset output = make_nodeset();
    for (size_t i=0; i<doms->ndoms; ++i) {
        hvml_dom_t *dom = doms->doms[i];
        xpathDomVisitor visitor(dom, 0, 1);
        antlrcpp::Any any = visitor.visitStep(ctx);
        if (!any.is<xpathNodeset>()) T("expecting node set but failed");
        xpathNodeset tail = any;
        if (hvml_doms_append_doms(output.get(), tail.get())) T("out of memory");
    }
    return output;
}

xpathNodeset xpathDomVisitor::do_collect_descendant_or_self(xpathNodeset &doms) {
    xpathNodeset output = make_nodeset();
    for (size_t i=0; i<doms->ndoms; ++i) {
        hvml_dom_t *dom = doms->doms[i];
        xpathDomVisitor visitor(dom, 0, 1);
        xpathNodeset tail = visitor.do_axis_descendant_or_self();
        if (hvml_doms_append_doms(output.get(), tail.get())) T("out of memory");
    }
    return output;
}

xpathNodeset xpathDomVisitor::do_relative(xpathNodeset &doms, xpathParser::RelativeLocationPathContext *ctx) {
    xpathNodeset output = make_nodeset();
    for (size_t i=0; i<doms->ndoms; ++i) {
        xpathDomVisitor visitor(doms->doms[i], 0, 1);
        antlrcpp::Any any = visitor.visitRelativeLocationPath(ctx);
        if (!any.is<xpathNodeset>()) T("expecting node set but failed");
        xpathNodeset tail = any;
        if (hvml_doms_append_doms(output.get(), tail.get())) T("out of memory");
    }
    return output;
}

xpathNodeset xpathDomVisitor::do_axis(const std::string &axisName) {
    // A(0, "not implemented yet");
    if (axisName == "ancestor") {
        return do_axis_ancestor();
    }
    if (axisName == "ancestor-or-self") {
        return do_axis_ancestor_or_self();
    }
    if (axisName == "attribute") {
        return do_axis_attribute();
    }
    if (axisName == "child") {
        return do_axis_child();
    }
    if (axisName == "descendant") {
        return do_axis_descendant();
    }
    if (axisName == "descendant-or-self") {
        return do_axis_descendant_or_self();
    }
    if (axisName == "following") {
        return do_axis_following();
    }
    if (axisName == "following-sibling") {
        return do_axis_following_sibling();
    }
    if (axisName == "namespace") {
        T("not implemented yet");
    }
    if (axisName == "parent") {
        return do_axis_parent();
    }
    if (axisName == "preceding") {
        return do_axis_preceding();
    }
    if (axisName == "preceding-sibling") {
        return do_axis_preceding_sibling();
    }
    if (axisName == "self") {
        return do_axis_self();
    }
    T("not implemented yet");
}

xpathNodeset xpathDomVisitor::do_axis_ancestor(void) {
    A(principal_==HVML_DOM_XPATH_PRINCIPAL_UNSPECIFIED, "internal logic error");
    principal_ = HVML_DOM_XPATH_PRINCIPAL_ELEMENT;
    xpathNodeset doms = make_nodeset();

    hvml_dom_t *dom = hvml_dom_parent(dom_);
    while (dom) {
        if (hvml_doms_append_dom(doms.get(), dom)) T("out of memory");
        dom = hvml_dom_parent(dom);
    }

    return doms;
}

xpathNodeset xpathDomVisitor::do_axis_ancestor_or_self(void) {
    A(principal_==HVML_DOM_XPATH_PRINCIPAL_UNSPECIFIED, "internal logic error");
    principal_ = HVML_DOM_XPATH_PRINCIPAL_ELEMENT;
    xpathNodeset doms = make_nodeset();

    hvml_dom_t *dom = dom_;
    while (dom) {
        if (hvml_doms_append_dom(doms.get(), dom)) T("out of memory");
        dom = hvml_dom_parent(dom);
    }

    return doms;
}

xpathNodeset xpathDomVisitor::do_axis_attribute(void) {
    A(principal_==HVML_DOM_XPATH_PRINCIPAL_UNSPECIFIED, "internal logic error");
    principal_ = HVML_DOM_XPATH_PRINCIPAL_ATTRIBUTE;
    xpathNodeset doms = make_nodeset();

    hvml_dom_t *dom = hvml_dom_attr_head(dom_);
    while (dom) {
        if (hvml_doms_append_dom(doms.get(), dom)) T("out of memory");
        dom = hvml_dom_attr_next(dom);
    }

    return doms;
}

xpathNodeset xpathDomVisitor::do_axis_child(void) {
    A(principal_==HVML_DOM_XPATH_PRINCIPAL_UNSPECIFIED, "internal logic error");
    principal_ = HVML_DOM_XPATH_PRINCIPAL_ELEMENT;
    xpathNodeset doms = make_nodeset();

    hvml_dom_t *dom = hvml_dom_child(dom_);
    while (dom) {
        if (hvml_doms_append_dom(doms.get(), dom)) T("out of memory");
        dom = hvml_dom_next(dom);
    }

    return doms;
}

xpathNodeset xpathDomVisitor::do_axis_descendant(void) {
    A(principal_==HVML_DOM_XPATH_PRINCIPAL_UNSPECIFIED, "internal logic error");
    principal_ = HVML_DOM_XPATH_PRINCIPAL_ELEMENT;
    xpathNodeset doms = make_nodeset();
    if (do_doms_append_relative(doms.get(), 1, 0, 1, dom_, dom_)) T("out of memory");

    return doms;
}

xpathNodeset xpathDomVisitor::do_axis_descendant_or_self(void) {
    A(principal_==HVML_DOM_XPATH_PRINCIPAL_UNSPECIFIED, "internal logic error");
    principal_ = HVML_DOM_XPATH_PRINCIPAL_ELEMENT;
    xpathNodeset doms = make_nodeset();
    A(doms.get(), "internal logic error");
    A(dom_, "internal logic error");
    if (do_doms_append_relative(doms.get(), 1, 1, 1, dom_, dom_)) T("out of memory");

    return doms;
}

xpathNodeset xpathDomVisitor::do_axis_following(void) {
    A(principal_==HVML_DOM_XPATH_PRINCIPAL_UNSPECIFIED, "internal logic error");
    principal_ = HVML_DOM_XPATH_PRINCIPAL_ELEMENT;
    xpathNodeset doms = make_nodeset();
    hvml_dom_t *root = hvml_dom_root(dom_);
    A(root, "internal logic error");
    if (do_doms_append_relative(doms.get(), 1, 0, 0, root, dom_)) T("out of memory");

    return doms;
}

xpathNodeset xpathDomVisitor::do_axis_following_sibling(void) {
    A(principal_==HVML_DOM_XPATH_PRINCIPAL_UNSPECIFIED, "internal logic error");
    principal_ = HVML_DOM_XPATH_PRINCIPAL_ELEMENT;
    xpathNodeset doms = make_nodeset();

    hvml_dom_t *dom = hvml_dom_next(dom_);
    while (dom) {
        if (hvml_doms_append_dom(doms.get(), dom)) T("out of memory");
        dom = hvml_dom_next(dom);
    }

    return doms;
}

xpathNodeset xpathDomVisitor::do_axis_parent(void) {
    A(principal_==HVML_DOM_XPATH_PRINCIPAL_UNSPECIFIED, "internal logic error");
    principal_ = HVML_DOM_XPATH_PRINCIPAL_ELEMENT;
    xpathNodeset doms = make_nodeset();
    hvml_dom_t *dom = hvml_dom_parent(dom_);
    if (dom && hvml_doms_append_dom(doms.get(), dom)) T("out of memory");

    return doms;
}

xpathNodeset xpathDomVisitor::do_axis_preceding(void) {
    A(principal_==HVML_DOM_XPATH_PRINCIPAL_UNSPECIFIED, "internal logic error");
    principal_ = HVML_DOM_XPATH_PRINCIPAL_ELEMENT;
    xpathNodeset doms = make_nodeset();
    hvml_dom_t *root = hvml_dom_root(dom_);
    A(root, "internal logic error");
    if (do_doms_append_relative(doms.get(), 0, 0, 0, root, dom_)) T("out of memory");
    if (hvml_doms_reverse(doms.get())) T("out of memory");

    return doms;
}

xpathNodeset xpathDomVisitor::do_axis_preceding_sibling(void) {
    A(principal_==HVML_DOM_XPATH_PRINCIPAL_UNSPECIFIED, "internal logic error");
    principal_ = HVML_DOM_XPATH_PRINCIPAL_ELEMENT;
    xpathNodeset doms = make_nodeset();

    hvml_dom_t *dom = hvml_dom_prev(dom_);
    while (dom) {
        if (hvml_doms_append_dom(doms.get(), dom)) T("out of memory");
        dom = hvml_dom_prev(dom);
    }

    return doms;
}

xpathNodeset xpathDomVisitor::do_axis_self(void) {
    A(principal_==HVML_DOM_XPATH_PRINCIPAL_UNSPECIFIED, "internal logic error");
    principal_ = HVML_DOM_XPATH_PRINCIPAL_ELEMENT;
    xpathNodeset doms = make_nodeset();
    if (hvml_doms_append_dom(doms.get(), dom_)) T("out of memory");

    return doms;
}


xpathNodeset xpathDomVisitor::do_nodetest(xpathNodeset &doms, HVML_DOM_XPATH_PRINCIPAL_TYPE principal, xpathParser::NodeTestContext *ctx) {
    xpathNodeset output = make_nodeset();
    for (size_t i=0; i<doms->ndoms; ++i) {
        hvml_dom_t *dom = doms->doms[i];
        xpathDomVisitor visitor(dom, 0, 1);
        visitor.principal_ = principal;
        antlrcpp::Any any = visitor.visitNodeTest(ctx);
        if (!any.is<bool>()) T("expecting boolean but failed");
        if (!any) continue;
        if (hvml_doms_append_dom(output.get(), dom)) T("out of memory");
    }
    return output;
}

xpathNodeset xpathDomVisitor::do_predicate(xpathNodeset &doms, xpathParser::PredicateContext *ctx) {
    xpathNodeset output = make_nodeset();

    for (size_t i=0; i<doms->ndoms; ++i) {
        hvml_dom_t *dom = doms->doms[i];
        xpathDomVisitor visitor(dom, i, doms->ndoms);
        antlrcpp::Any any = visitor.visitPredicate(ctx);
        if (!any.is<bool>()) T("expecting boolean but failed");
        bool ok = any;
        if (!ok) continue;
        if (hvml_doms_append_dom(output.get(), dom)) T("out of memory");
    }

    return output;
}

static antlrcpp::Any do_compare(HVML_DOM_XPATH_OP_TYPE op, const antlrcpp::Any &left, const antlrcpp::Any &right);

antlrcpp::Any xpathDomVisitor::do_eq(const antlrcpp::Any &left, const antlrcpp::Any &right) {
    return do_compare(HVML_DOM_XPATH_OP_EQ, left, right);
}

antlrcpp::Any xpathDomVisitor::do_less(const antlrcpp::Any &left, const antlrcpp::Any &right) {
    return do_compare(HVML_DOM_XPATH_OP_LT, left, right);
}

antlrcpp::Any xpathDomVisitor::do_lte(const antlrcpp::Any &left, const antlrcpp::Any &right) {
    return do_compare(HVML_DOM_XPATH_OP_LTE, left, right);
}

antlrcpp::Any xpathDomVisitor::do_op(const std::string &op, const antlrcpp::Any &left, const antlrcpp::Any &right) {
    (void)left;
    (void)right;
    if (op == "+") {
        T("not implemented yet");
    } else if (op == "-") {
        T("not implemented yet");
    } else if (op == "*") {
        T("not implemented yet");
    } else if (op == "/") {
        T("not implemented yet");
    } else if (op == "%") {
        T("not implemented yet");
    } else {
        A(0, "internal logic error");
    }
    A(0, "internal logic error");
    return false;
}

bool xpathDomVisitor::do_node_type(hvml_dom_t *dom, const std::string &nt) {
    if (nt == "comment") {
        T("not implemented yet");
    }
    if (nt == "text") {
        return hvml_dom_type(dom)==MKDOT(D_TEXT) ? true : false;
    }
    if (nt == "json") {
        return hvml_dom_type(dom)==MKDOT(D_JSON) ? true : false;
    }
    if (nt == "node") {
        return true;
    }
    if (nt == "processing-instruction") {
        T("not implemented yet");
    }
    T("internal logic error");
}

bool xpathDomVisitor::do_name_test(hvml_dom_t *dom, const char *prefix, const char *local_part) {
    A(principal_!=HVML_DOM_XPATH_PRINCIPAL_UNSPECIFIED, "internal logic error");
    const char *tok = NULL;
    if (hvml_dom_type(dom) == MKDOT(D_TAG)) {
        tok = hvml_dom_tag_name(dom);
    } else if (hvml_dom_type(dom) == MKDOT(D_ATTR)) {
        tok = hvml_dom_attr_key(dom);
    }
    const char *colon = tok ? strchr(tok, ':') : NULL;

    if (strcmp(local_part, "*")==0) {
        if (prefix==NULL) {
            // "*"
            switch (principal_) {
                case HVML_DOM_XPATH_PRINCIPAL_UNSPECIFIED: {
                    A(0, "internal logic error");
                } break;
                case HVML_DOM_XPATH_PRINCIPAL_ATTRIBUTE: {
                    return hvml_dom_type(dom)==MKDOT(D_ATTR);
                } break;
                case HVML_DOM_XPATH_PRINCIPAL_NAMESPACE: {
                    T("not implemented yet");
                } break;
                case HVML_DOM_XPATH_PRINCIPAL_ELEMENT: {
                    return hvml_dom_type(dom)==MKDOT(D_TAG);
                } break;
                default: {
                    A(0, "internal logic error");
                } break;
            }
            return true;
        }
        // prefix:*
        if (!tok) return false;
        if (strstr(tok, prefix)!=tok) return false;
        if (tok[strlen(prefix)]!=':') return false;
        return true;
    }
    if (prefix && strcmp(prefix, "*")==0) {
        // "*:xxx"
        if (!tok) return false;
        if (!colon) return false;
        if (strcmp(colon+1, local_part)) return false;
        return true;
    }
    if (!prefix) {
        // "xxx"
        if (!tok) return false;
        if (strcmp(tok, local_part)) return false;
        return true;
    }
    // "xxx:yyy"
    if (!tok) return false;
    if (!colon) return false;
    if (strstr(tok, prefix)!=tok) return false;
    if (strcmp(colon+1, local_part)) return false;
    return true;
}

bool xpathDomVisitor::to_bool(const antlrcpp::Any &any) {
    bool v = false;
    if (any.is<bool>()) {
        v = any;
    } else if (any.is<long double>()) {
        long double number = any;
        int fc = fpclassify(number);
        switch (fc) {
            case FP_INFINITE:
            case FP_SUBNORMAL:
            case FP_NORMAL: {
                v = true;
            } break;
            case FP_NAN:
            case FP_ZERO: {
                v = false;
            } break;
            default: {
                T("internal logic error");
            } break;
        }
    } else if (any.is<std::string>()) {
        const std::string &literal = any;
        if (!literal.empty()) v = true;
    } else if (any.is<xpathNodeset>()) {
        xpathNodeset doms = any;
        if (doms->ndoms>0) v = true;
    } else {
        T("internal logic error");
    }
    return v;
}

long double xpathDomVisitor::to_number(const antlrcpp::Any &any) {
    long double v = NAN;
    if (any.is<bool>()) {
        bool t = any;
        v = t ? 1 : 0;
    } else if (any.is<long double>()) {
        v = any;
    } else if (any.is<std::string>()) {
        const std::string &literal = any;
        hvml_string_to_number(literal.c_str(), &v);
    } else if (any.is<xpathNodeset>()) {
        std::string s = to_string(any);
        hvml_string_to_number(s.c_str(), &v);
    } else {
        T("internal logic error");
    }
    return v;
}

std::string xpathDomVisitor::to_string(const antlrcpp::Any &any) {
    std::string v;
    if (any.is<bool>()) {
        bool t = any;
        v = t ? "true" : "false";
    } else if (any.is<long double>()) {
        long double ldbl = any;
        int fc = fpclassify(ldbl);
        switch (fc) {
            case FP_NAN: {
                v = "NaN";
            } break;
            case FP_INFINITE: {
                int sb = signbit(ldbl);
                v = sb ? "-Infinity" : "Infinity";
            } break;
            case FP_ZERO: {
                v = "0";
            } break;
            case FP_SUBNORMAL: {
                char buf[128];
                snprintf(buf, sizeof(buf), "%.*Lf", int(sizeof(buf)/2), ldbl);
                v = buf;
            } break;
            case FP_NORMAL: {
                char buf[128];
                snprintf(buf, sizeof(buf), "%.*Lf", int(sizeof(buf)/2), ldbl);
                v = buf;
            } break;
            default: {
                T("internal logic error");
            } break;
        }
    } else if (any.is<std::string>()) {
        v = any.as<std::string>();
    } else if (any.is<xpathNodeset>()) {
        xpathNodeset doms = any;
        if (doms->ndoms==0) return "";
        hvml_dom_t *dom = doms->doms[0];
        const char *s = NULL;
        int allocated = 0;
        if (hvml_dom_string_for_xpath(dom, &s, &allocated)) T("out of memory");
        A(s, "internal logic error");
        std::string str = s;
        if (allocated) free((void*)s);
        return str;
    } else {
        T("internal logic error");
    }
    return v;
}

static antlrcpp::Any do_compare(HVML_DOM_XPATH_OP_TYPE op, const antlrcpp::Any &left, const antlrcpp::Any &right) {
    if (left.is<xpathNodeset>() && right.is<xpathNodeset>()) {
        D("not implemented yet");
        return false;
    }
    if (left.is<xpathNodeset>() || right.is<xpathNodeset>()) {
        if (right.is<xpathNodeset>()) {
            switch (op) {
                case HVML_DOM_XPATH_OP_EQ: {
                    return do_compare(op, right, left);
                } break;
                case HVML_DOM_XPATH_OP_LT: {
                    return do_compare(HVML_DOM_XPATH_OP_GT, right, left);
                } break;
                case HVML_DOM_XPATH_OP_LTE: {
                    return do_compare(HVML_DOM_XPATH_OP_GTE, right, left);
                } break;
                default: {
                    A(0, "internal logic error");
                    // never reached here
                    return false;
                } break;
            }
        }
    }
    if (op == HVML_DOM_XPATH_OP_EQ) {
        if (left.is<bool>() || right.is<bool>()) {
            bool l = xpathDomVisitor::to_bool(left);
            bool r = xpathDomVisitor::to_bool(right);
            return l == r;
        }
        if (left.is<long double>() || right.is<long double>()) {
            long double l = xpathDomVisitor::to_number(left);
            long double r = xpathDomVisitor::to_number(right);
            return fabs(l-r) < LDBL_EPSILON;
        }
        if (left.is<std::string>() || right.is<std::string>()) {
            std::string l = xpathDomVisitor::to_string(left);
            std::string r = xpathDomVisitor::to_string(right);
            return l == r;
        }
        A(0, "internal logic error");
        return false;
    } else {
        long double l = xpathDomVisitor::to_number(left);
        long double r = xpathDomVisitor::to_number(right);
        switch (op) {
            case HVML_DOM_XPATH_OP_LT:  return l < r;
            case HVML_DOM_XPATH_OP_GT:  return l > r;
            case HVML_DOM_XPATH_OP_LTE: return l <= r;
            case HVML_DOM_XPATH_OP_GTE: return l >= r;
            default: {
                A(0, "internal logic error");
                return false;
            } break;
        }
    }
}

