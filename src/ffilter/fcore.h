/*

 Copyright (c) 2015-2017, Imrich Stoffa

 This file is part of libnf.net project.

 Libnf is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Libnf is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with libnf.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * \file fcore.h
 * \brief netflow fiter tree (abstract syntax tree) evaluation function and structures
 */

#ifndef NFFILTER_FCORE_H
#define NFFILTER_FCORE_H

#include "ffilter.h"

ff_attr_t ff_negate(ff_attr_t o);

ff_attr_t ff_validate(ff_type_t type, ff_oper_t op, char* data, ff_lvalue_t* info);

int ff_oper_eval_V2(char* buf, size_t size, ff_node_t *node);

#endif //NFFILTER_FCORE_H
