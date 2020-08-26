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

#ifndef _hvml_list_
#define _hvml_list_

#include "hvml/hvml_log.h"

// nc: node class
// oc: owner class
// p:  prefix
// o:  owner
// n:  node

#define MKM(nc, oc, p, t)  nc##_##oc##_##p##_##t

#define HLIST_MEMBERS(nc, oc, p)   nc *MKM(nc,oc,p,head); nc *MKM(nc,oc,p,tail); size_t MKM(nc,oc,p,count)
#define HNODE_MEMBERS(nc, oc, p)   nc *MKM(nc,oc,p,next); nc *MKM(nc,oc,p,prev); oc    *MKM(nc,oc,p,owner)


#define HLIST_APPEND(nc, oc, p, o, n)                           \
do {                                                            \
  oc *owner = (o);                                              \
  nc *node  = (n);                                              \
  A(node->MKM(nc,oc,p,owner) == NULL, "internal logic error");  \
  A(owner != NULL, "internal logic error");                     \
  node->MKM(nc,oc,p,prev)  = owner->MKM(nc,oc,p,tail);          \
  if (owner->MKM(nc,oc,p,tail)) {                               \
    owner->MKM(nc,oc,p,tail)->MKM(nc,oc,p,next) = node;         \
  } else {                                                      \
    owner->MKM(nc,oc,p,head)                = node;             \
  }                                                             \
  owner->MKM(nc,oc,p,tail)                  = node;             \
  node->MKM(nc,oc,p,owner)                  = owner;            \
  owner->MKM(nc,oc,p,count)                += 1;                \
} while (0)

#define HLIST_REMOVE(nc, oc, p, n)                                          \
do {                                                                        \
  nc *node= (n);                                                            \
  A(node!=NULL, "internal logic error");                                    \
  oc *owner = node->MKM(nc,oc,p,owner);                                     \
  A(owner!=NULL, "internal logic error");                                   \
  nc *prev  = node->MKM(nc,oc,p,prev);                                      \
  nc *next  = node->MKM(nc,oc,p,next);                                      \
  nc *head  = owner->MKM(nc,oc,p,head);                                     \
  nc *tail  = owner->MKM(nc,oc,p,tail);                                     \
  A((head==NULL && tail==NULL) || (head && tail), "internal logic error");  \
  if (prev)  prev->MKM(nc,oc,p,next)      = next;                           \
  else            owner->MKM(nc,oc,p,head)     = next;                      \
  if (next)  next->MKM(nc,oc,p,prev)      = prev;                           \
  else            owner->MKM(nc,oc,p,tail)     = prev;                      \
  node->MKM(nc,oc,p,prev)    = NULL;                                        \
  node->MKM(nc,oc,p,next)    = NULL;                                        \
  node->MKM(nc,oc,p,owner)   = NULL;                                        \
  owner->MKM(nc,oc,p,count) -= 1;                                           \
} while (0)

#define HNODE_IS_ORPHAN(nc, oc, p, n)          \
    ((n)->MKM(nc,oc,p,owner)== NULL &&         \
     (n)->MKM(nc,oc,p,prev) == NULL &&         \
     (n)->MKM(nc,oc,p,next) == NULL)

#define HLIST_IS_EMPTY(nc, oc, p, n)           \
    ((n)->MKM(nc,oc,p,count)== 0 &&            \
     (n)->MKM(nc,oc,p,head) == NULL &&         \
     (n)->MKM(nc,oc,p,tail) == NULL)

#define HNODE_OWNER(nc, oc, p, n)  (n)->MKM(nc,oc,p,owner)
#define HLIST_COUNT(nc, oc, p, o)  (o)->MKM(nc,oc,p,count)

#endif // _hvml_list_

