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

#include "hvml/hvml_dom.h"
#include "hvml/hvml_log.h"

#include "antlr4-runtime.h"
#include "xpathLexer.h"
#include "xpathParser.h"
#include "xpathDomVisitor.h"

using namespace antlr4;

int hvml_dom_qry(hvml_dom_t *dom, const char *path, hvml_doms_t *doms) {
    ANTLRInputStream input(path);
    xpathLexer lexer(&input);
    CommonTokenStream tokens(&lexer);

    tokens.fill();

    xpathParser parser(&tokens);
    tree::ParseTree* tree = parser.main();

    int r = 0;
    try {
        do {
            xpathDomVisitor visitor(dom, 0, 1);
            xpathNodeset ns = visitor.visit(tree);
            if (doms) {
                r = hvml_doms_append_doms(doms, ns.get());
            }
        } while (0);
    } catch (std::exception &e) {
        D("exception: [%s]", e.what());
        r = -1;
        throw;
    }

    return r ? -1 : 0;
}

