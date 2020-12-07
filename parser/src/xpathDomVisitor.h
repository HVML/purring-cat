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

#pragma once


#include "antlr4-runtime.h"
#include "xpathVisitor.h"


#include "hvml/hvml_dom.h"

#include "hvml_dom_xpath_parser.h"

struct xpathError {};
typedef std::shared_ptr<hvml_doms_t> xpathNodeset;

/**
 * This class provides an empty implementation of xpathVisitor, which can be
 * extended to create a visitor which only needs to handle a subset of the available methods.
 */
class  xpathDomVisitor : public xpathVisitor {
public:
    xpathDomVisitor(hvml_dom_t *dom, size_t idx, size_t size);
    virtual ~xpathDomVisitor() override;

    virtual antlrcpp::Any visitMain(xpathParser::MainContext *ctx) override;
    virtual antlrcpp::Any visitLocationPath(xpathParser::LocationPathContext *ctx) override;
    virtual antlrcpp::Any visitAbsoluteLocationPathNoroot(xpathParser::AbsoluteLocationPathNorootContext *ctx) override;
    virtual antlrcpp::Any visitRelativeLocationPath(xpathParser::RelativeLocationPathContext *ctx) override;
    virtual antlrcpp::Any visitStep(xpathParser::StepContext *ctx) override;
    virtual antlrcpp::Any visitAxisSpecifier(xpathParser::AxisSpecifierContext *ctx) override;
    virtual antlrcpp::Any visitNodeTest(xpathParser::NodeTestContext *ctx) override;
    virtual antlrcpp::Any visitPredicate(xpathParser::PredicateContext *ctx) override;
    virtual antlrcpp::Any visitAbbreviatedStep(xpathParser::AbbreviatedStepContext *ctx) override;
    virtual antlrcpp::Any visitExpr(xpathParser::ExprContext *ctx) override;
    virtual antlrcpp::Any visitPrimaryExpr(xpathParser::PrimaryExprContext *ctx) override;
    virtual antlrcpp::Any visitFunctionCall(xpathParser::FunctionCallContext *ctx) override;
    virtual antlrcpp::Any visitUnionExprNoRoot(xpathParser::UnionExprNoRootContext *ctx) override;
    virtual antlrcpp::Any visitPathExprNoRoot(xpathParser::PathExprNoRootContext *ctx) override;
    virtual antlrcpp::Any visitFilterExpr(xpathParser::FilterExprContext *ctx) override;
    virtual antlrcpp::Any visitOrExpr(xpathParser::OrExprContext *ctx) override;
    virtual antlrcpp::Any visitAndExpr(xpathParser::AndExprContext *ctx) override;
    virtual antlrcpp::Any visitEqualityExpr(xpathParser::EqualityExprContext *ctx) override;
    virtual antlrcpp::Any visitRelationalExpr(xpathParser::RelationalExprContext *ctx) override;
    virtual antlrcpp::Any visitAdditiveExpr(xpathParser::AdditiveExprContext *ctx) override;
    virtual antlrcpp::Any visitMultiplicativeExpr(xpathParser::MultiplicativeExprContext *ctx) override;
    virtual antlrcpp::Any visitUnaryExprNoRoot(xpathParser::UnaryExprNoRootContext *ctx) override;
    virtual antlrcpp::Any visitQName(xpathParser::QNameContext *ctx) override;
    virtual antlrcpp::Any visitFunctionName(xpathParser::FunctionNameContext *ctx) override;
    virtual antlrcpp::Any visitVariableReference(xpathParser::VariableReferenceContext *ctx) override;
    virtual antlrcpp::Any visitNameTest(xpathParser::NameTestContext *ctx) override;
    virtual antlrcpp::Any visitNCName(xpathParser::NCNameContext *ctx) override;

private:
    bool empty() const;

    xpathNodeset stepToRoot(void);
    xpathNodeset do_step(xpathNodeset &doms, xpathParser::StepContext *ctx);
    xpathNodeset do_collect_descendant_or_self(xpathNodeset &doms);
    xpathNodeset do_relative(xpathNodeset &doms, xpathParser::RelativeLocationPathContext *ctx);
    xpathNodeset do_axis(const std::string &axisName);
    xpathNodeset do_axis_ancestor(void);
    xpathNodeset do_axis_ancestor_or_self(void);
    xpathNodeset do_axis_attribute(void);
    xpathNodeset do_axis_child(void);
    xpathNodeset do_axis_descendant(void);
    xpathNodeset do_axis_descendant_or_self(void);
    xpathNodeset do_axis_following(void);
    xpathNodeset do_axis_following_sibling(void);
    xpathNodeset do_axis_parent(void);
    xpathNodeset do_axis_preceding(void);
    xpathNodeset do_axis_preceding_sibling(void);
    xpathNodeset do_axis_self(void);
    xpathNodeset do_nodetest(xpathNodeset &doms, HVML_DOM_XPATH_PRINCIPAL_TYPE principal, xpathParser::NodeTestContext *ctx);
    xpathNodeset do_predicate(xpathNodeset &doms, xpathParser::PredicateContext *ctx);
    bool         do_predicate(const antlrcpp::Any &any);
    antlrcpp::Any do_eq(const antlrcpp::Any &left, const antlrcpp::Any &right);
    antlrcpp::Any do_less(const antlrcpp::Any &left, const antlrcpp::Any &right);
    antlrcpp::Any do_lte(const antlrcpp::Any &left, const antlrcpp::Any &right);
    antlrcpp::Any do_op(const std::string &op, const antlrcpp::Any &left, const antlrcpp::Any &right);

    bool do_name_test(hvml_dom_t *dom, const char *prefix, const char *local_part);
    bool do_node_type(hvml_dom_t *dom, const std::string &nt);

public:
    static bool to_bool(const antlrcpp::Any &any);
    static long double to_number(const antlrcpp::Any &any);
    static std::string to_string(const antlrcpp::Any &any);

private:
    hvml_dom_t                     *dom_;
    const size_t                    idx_;
    const size_t                    size_;
    HVML_DOM_XPATH_AXIS_TYPE        axis_;
    HVML_DOM_XPATH_PRINCIPAL_TYPE   principal_;
};

