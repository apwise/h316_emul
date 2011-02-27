/* Honeywell Series 16 emulator
 * Copyright (C) 1996, 1997, 1998, 2005  Adrian Wise
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307 USA
 *
 * $Log: tree.hh,v $
 * Revision 1.1  1999/02/20 00:06:35  adrian
 * Initial revision
 *
 *
 * Simple tree management.
 *
 * Adrian Wise, 29 January 1996.
 * Adapted 19 August 1997.
 */

#include <stdio.h>

struct TREE_STRUCT
{
  unsigned long long name;
  void *record;
  
  struct TREE_STRUCT *l;
  struct TREE_STRUCT *r;
};

typedef struct TREE_STRUCT *TREE;

extern void tree_add(TREE *t, unsigned long long name, void *record);

extern void tree_free(TREE *t, void (*free_fn)(void *ptr));
extern void tree_for_all(TREE *t, void *data,
                         void (*proc)(TREE *t, unsigned long long name,
                                      void *record, void *data));
extern void tree_dump(TREE *t, FILE *fp, int depth,
                      void (*dump_proc)(TREE *t, char *space, void *record));

extern void *tree_remove_left_most(TREE *t, unsigned long long &left_name,
                                   unsigned long long name, bool check_name);

extern bool tree_left_name(TREE *t, unsigned long long &left_name);
