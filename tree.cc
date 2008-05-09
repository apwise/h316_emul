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
 * Simple tree management.
 *
 * Adrian Wise, 29 January 1996.
 * Adapted 19 August 1997
 *
 * This is now a weird tree.
 * 
 * It can have duplicate "name"s which are always
 * added top the right of all of the entries with
 * the same name.
 *
 * tree_read and tree_delete no longer work so
 * are removed.
 *
 */

static const char *RcsId __attribute__ ((unused)) = "$Id: tree.cc,v 1.2 1999/02/25 06:54:55 adrian Exp $";

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "stdtty.hh"
#include "tree.hh"

/*
 * This routine compares a to b returning
 * a<b -1
 * a=b 0
 * a>b +1
 *
 * However the comparison is made modulo 2^32 so that
 * for example, using 3 bit numbers;
 *
 *   \b 0  1  2  3  4  5  6  7
 * a 0  0 -1 -1 -1 +1 +1 +1 +1
 *   1 +1  0 -1 -1 -1 +1 +1 +1
 *   2 +1 +1  0 -1 -1 -1 +1 +1
 *   3 +1 +1 +1  0 -1 -1 -1 +1
 *   4 +1 +1 +1 +1  0 -1 -1 -1
 *   5 -1 +1 +1 +1 +1  0 -1 -1
 *   6 -1 -1 +1 +1 +1 +1  0 -1
 *   7 -1 -1 -1 +1 +1 +1 +1  0
 */
int lt(unsigned long a, unsigned long b)
{
  signed long s = a-b;
  
  return (s < 0) ? -1 : ((s>0) ? +1 : 0);
}

void tree_add(TREE *t, unsigned long name, void *record)
{
  const char *PN = "tree_add()";
  TREE n;
  
  if (*t)
    {
      if (lt(name, (*t)->name) < 0)
        tree_add(&((*t)->l), name, record);
      else /* add greater OR SAME to the right */
        tree_add(&((*t)->r), name, record);
    }
  else
    {
      if (!record)
        {
          fprintf(stderr, "%s: (name <%ld>) NULL record", PN, name);
          exit(1);
        }
      n = (TREE) malloc(sizeof(struct TREE_STRUCT));
      n->name = name;
      n->record = record;
      n->l = NULL;
      n->r = NULL;

      (*t) = n;
    }
}


void tree_free(TREE *t, void (*free_fn)(void *ptr))
{
  //    char *PN = "tree_free()";
  
  if (*t)
    {
      tree_free(&((*t)->l), free_fn);
      tree_free(&((*t)->r), free_fn);
      
      if (free_fn)
        free_fn((*t)->record);
      free(*t);
      *t = NULL;
    }
}

void tree_for_all(TREE *t, void *data,
                  void (*proc)(TREE *t, unsigned long name, void *record, void *data))
{
  //    char *PN="tree_for_all()";
  
  if (*t)
    {
      tree_for_all(&((*t)->l), data, proc);
      proc(t, (*t)->name, (*t)->record, data);
      tree_for_all(&((*t)->r), data, proc);
    }
}

static char *spaces(int depth)
{
  static char buf[129];
  int i;
  
  if (depth > 64)
    depth = 64;
  
  for (i=0; i<(2*depth); i++)
    buf[i] = ' ';
  buf[i] = '\0';
  return buf;
}

void tree_dump(TREE *t, FILE *fp, int depth,
               void (*dump_proc)(TREE *t, char *space, void *record))
{
  //char *PN="tree_dump()";
  char *s = strdup(spaces(depth));
  
  if (*t)
    {
      fprintf(fp, "%sNode <%ld> -> 0x%08x\n", s,
              ((*t)->name), (unsigned int)((*t)->record));
      if (dump_proc)
        dump_proc(t, s, (*t)->record);
      
      if ((*t)->l)
        {
          fprintf(fp, "%sLeft:\n", s);
          tree_dump(&((*t)->l), fp, depth+1, dump_proc);
        }
      else
        fprintf(fp, "%sLeft: (NULL)\n", s);
      
      if ((*t)->r)
        {
          fprintf(fp, "%sRight:\n", s);
          tree_dump(&((*t)->r), fp, depth+1, dump_proc);
        }
      else
        fprintf(fp, "%sRight: (NULL)\n",s);
    }
  
  free(s);
}


extern void *tree_remove_left_most(TREE *t, unsigned long &left_name,
                                   unsigned long name, bool check_time)
{
  TREE p=(*t);
  TREE q = NULL;
  TREE up = NULL;
  void *record = NULL;
  
  while (p)
    {
      up = q;
      q = p;
      p = p->l;
    }
  
  /* q points at the left-most element.
     We know it hasn't got a left (because it
     wouldn't be the left-most if it had). But it
     may have elements to the right, which need
     to become the left of the parent of q.
     
     Only return the record if the name <= name
     passed to the routine */
  
  if (q)
    {
      if ((!check_time) || (lt(q->name, name) <= 0))
        {
          record = q->record;
          left_name = q->name;
          
          if (up)
            up->l = q->r;
          else
            *t = q->r;
          
          free (q);
        }
    }

  return record;
}

extern bool tree_left_name(TREE *t, unsigned long &left_name)
{
  TREE p=(*t);
  TREE q = NULL;
  bool r=0;

  while (p)
    {
      q = p;
      p = p->l;
    }

  if (q)
    {
      left_name = q->name;
      r = 1;
    }
  return r;
}
