/* Honeywell Series 16 emulator
 * Copyright (C) 1998, 2001, 2012, 2020  Adrian Wise
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
 */

#define GTK_ENABLE_BROKEN

#include <gtk/gtk.h>
#include <stdio.h>
#include "gpl.h"
#include "version.h"
#include "menu.h"

enum ABOUT_SECTIONS
{
  AB_ABOUT,
  AB_FULL,
  AB_PREAMBLE,
  AB_COPYING,
  AB_WARRANTY,
  AB_HOWTO,

  AB_NUM
};

static char *instructions_text[] =
{
  "See \"Copying\", under \"Help\" or below, for details.\n",
  "There is absolutely no warranty for " NAME ".\n",
  "See \"Warranty\", under \"Help\" or below, for details.\n",
  "\n",
  NULL
};

static void popup_about_notebook(enum ABOUT_SECTIONS default_section);

#define COPYRIGHT_TIMEOUT 3000

struct COPYRIGHT_STRUCT 
{
  gint timeout_tag;
  GtkWidget *window;
};

static gint copyright_timeout( gpointer data)
{
  struct COPYRIGHT_STRUCT *cs = (struct COPYRIGHT_STRUCT *) data;

  gtk_widget_destroy(GTK_WIDGET(cs->window));
  g_free(cs);
  
  return FALSE;
}

static void copyright_close (GtkDialog *dialog,
                             gpointer   user_data)
{
  struct COPYRIGHT_STRUCT *cs = (struct COPYRIGHT_STRUCT *) user_data;

  g_source_remove(cs->timeout_tag);
  (void) copyright_timeout(user_data);
}

static void copyright_response (GtkDialog *dialog,
                                gint       response_id,
                                gpointer   user_data)
{
  struct COPYRIGHT_STRUCT *cs = (struct COPYRIGHT_STRUCT *) user_data;
  
  switch (response_id) {
    
  case GTK_RESPONSE_CLOSE: case GTK_RESPONSE_DELETE_EVENT:
    g_source_remove(cs->timeout_tag);
    (void) copyright_timeout(user_data);
    break;
  case AB_WARRANTY: case AB_COPYING:
    popup_about_notebook(response_id);
    break;

  }
}

void copyright()
{
  GtkWidget *window;
  GtkWidget *page;
  GtkWidget *text;
  GtkTextBuffer *buffer;
  int i;
  struct COPYRIGHT_STRUCT *cs = NULL;

  window = gtk_dialog_new_with_buttons("Copyright", NULL, 0,
                                       "Warranty",  AB_WARRANTY,
                                       "Copying",   AB_COPYING,
                                       "_Close",    GTK_RESPONSE_CLOSE,
                                       NULL);

  gtk_widget_set_size_request(window, 500, 250);

  page = gtk_scrolled_window_new(NULL, NULL);
  text  = gtk_text_view_new( );;
  buffer = gtk_text_view_get_buffer( GTK_TEXT_VIEW(text) );;
 
  gtk_text_view_set_editable (GTK_TEXT_VIEW (text), FALSE);
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW (text), GTK_WRAP_WORD);
  gtk_widget_show (text); 

  gtk_container_add( GTK_CONTAINER(page), text);
  gtk_widget_show (page); 

  gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(window))),
                     page, TRUE, TRUE, 0);

  for (i=0; copyright_text[i]; i++) {
    gtk_text_buffer_insert_at_cursor( buffer,
                                      copyright_text[i], -1);
  }
  for (i=0; instructions_text[i]; i++) {
    gtk_text_buffer_insert_at_cursor( buffer,
                                      instructions_text[i], -1);
  }

  cs = g_malloc(sizeof(struct COPYRIGHT_STRUCT));
  
  cs->window = window;
  cs->timeout_tag = g_timeout_add( COPYRIGHT_TIMEOUT,
                                   copyright_timeout,
                                   cs);

  g_signal_connect(window, "response",
                   G_CALLBACK(copyright_response), cs);
  g_signal_connect(window, "close",
                   G_CALLBACK(copyright_close), cs);
    
  gtk_widget_show (window);
}

/**********************************************************************
 * File menu
 *
 **********************************************************************/

static void file_menu_quit(GtkWidget *widget, gpointer data)
{
  gtk_main_quit ();
}

static GtkWidget *make_file_menu()
{
  GtkWidget *file_item, *file_menu, *quit_item;

  file_menu = gtk_menu_new();    /* Don't need to show menus */

  /* Create the menu items */
  quit_item = gtk_menu_item_new_with_label("Quit");

  /* Add them to the menu */
  gtk_menu_shell_append( GTK_MENU_SHELL(file_menu), quit_item);

  /* We can attach the Quit menu item to our exit function */
  g_signal_connect( quit_item, "activate",
                    G_CALLBACK(file_menu_quit), NULL);

  /* We do need to show menu items */
  gtk_widget_show( quit_item );

  file_item = gtk_menu_item_new_with_label("File");
  gtk_widget_show(file_item);
  gtk_menu_item_set_submenu( GTK_MENU_ITEM(file_item), file_menu );

  return file_item;
}

/**********************************************************************
 * Help menu
 *
 **********************************************************************/

static void help_menu_about(GtkWidget *widget, gpointer data)
{
  enum ABOUT_SECTIONS d = ((enum ABOUT_SECTIONS) ((int) ((size_t) data)));

  popup_about_notebook(d);
}

static GtkWidget *make_help_menu()
{
  GtkWidget *help_item, *help_menu, *about_item;
  GtkWidget *full_license_item, *copying_item, *warranty_item;

  help_menu = gtk_menu_new();    /* Don't need to show menus */

  /* Create the menu items */
  about_item = gtk_menu_item_new_with_label("About");
  full_license_item = gtk_menu_item_new_with_label("Full license");
  copying_item = gtk_menu_item_new_with_label("Copying");
  warranty_item = gtk_menu_item_new_with_label("Warranty");

  /* Add them to the menu */
  gtk_menu_shell_append( GTK_MENU_SHELL(help_menu), about_item);
  gtk_menu_shell_append( GTK_MENU_SHELL(help_menu), full_license_item);
  gtk_menu_shell_append( GTK_MENU_SHELL(help_menu), copying_item);
  gtk_menu_shell_append( GTK_MENU_SHELL(help_menu), warranty_item);

  /* We can attach the Quit menu item to our exit function */
  g_signal_connect( about_item, "activate",
                    G_CALLBACK(help_menu_about), (gpointer) AB_ABOUT);
  g_signal_connect( full_license_item, "activate",
                    G_CALLBACK(help_menu_about), (gpointer) AB_FULL);
  g_signal_connect( copying_item, "activate",
                    G_CALLBACK(help_menu_about), (gpointer) AB_COPYING);
  g_signal_connect( warranty_item, "activate",
                    G_CALLBACK(help_menu_about), (gpointer) AB_WARRANTY);

  /* We do need to show menu items */
  gtk_widget_show( about_item );
  gtk_widget_show( full_license_item );
  gtk_widget_show( copying_item );
  gtk_widget_show( warranty_item );

  help_item = gtk_menu_item_new_with_label("Help");
  gtk_widget_show(help_item);
  gtk_menu_item_set_submenu( GTK_MENU_ITEM(help_item), help_menu );

  return help_item;
}

/**********************************************************************
 * The overall menu
 *
 **********************************************************************/

GtkWidget *make_menu_bar()
{
  GtkWidget *menu_bar;
  GtkWidget *file_item;
  GtkWidget *help_item;

  menu_bar = gtk_menu_bar_new();
  gtk_widget_show( menu_bar );

  file_item = make_file_menu();
  gtk_menu_shell_append( GTK_MENU_SHELL(menu_bar), file_item );

  help_item = make_help_menu();
  gtk_menu_shell_append( GTK_MENU_SHELL(menu_bar), help_item );

  return menu_bar;
}


/**********************************************************************
 * Pop-up, about notebook
 *
 **********************************************************************/
static void about_response (GtkDialog *dialog,
                            gint       response_id,
                            gpointer   user_data)
{
  if (response_id == GTK_RESPONSE_CLOSE) {
    gtk_widget_destroy(GTK_WIDGET(dialog));
  }
}
  
static void about_notebook(char *prognam, char *about_text[],
                           enum ABOUT_SECTIONS default_section)
{
  GtkWidget *window;
  GtkWidget *notebook;
  char **ptr = about_text;
  char str[1024];
  static char *tags[] = {"About",
                         "Full License", "Preamble",
                         "Copying", "Warranty", "Howto",
                         NULL};
  int i;
  int tp = 0;
  int sp = 0;
  char **last=NULL;

  sprintf(str, "About %s", prognam);

  window = gtk_dialog_new_with_buttons(str, NULL, 0,
                                       "_Close", GTK_RESPONSE_CLOSE,
                                       NULL);

  notebook = gtk_notebook_new();
  gtk_widget_set_size_request(notebook, 600, 200);

  gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(window))), notebook, TRUE, TRUE, 0);
 
  do
    {
      GtkWidget *page  = gtk_scrolled_window_new(NULL, NULL);
      GtkWidget *label = gtk_label_new(tags[tp]);;
      GtkWidget *text  = gtk_text_view_new( );;
      GtkTextBuffer *buffer = gtk_text_view_get_buffer( GTK_TEXT_VIEW(text) );;
      
      gtk_text_view_set_editable (GTK_TEXT_VIEW (text), FALSE);
      gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW (text), GTK_WRAP_WORD);
      gtk_widget_show (text); 

      gtk_container_add( GTK_CONTAINER(page), text);
      gtk_widget_show (page); 

      gtk_notebook_append_page( GTK_NOTEBOOK(notebook),
                                GTK_WIDGET(/*vbox*/page),
                                label );

      for (i=0; ptr[i] && ((!last) || (&ptr[i]<last)); i++) {
        sprintf(str, "%s\n", ptr[i]);
        gtk_text_buffer_insert_at_cursor(buffer, str, -1);
      }
      
      tp++; /* Next tag */

      ptr = NULL;
      if (sp == 0) {
        ptr = full_license_text;
        last = NULL;
      } else if (license_sections[sp]>=0) {
        ptr = &full_license_text[license_sections[sp-1]];
        last = &full_license_text[license_sections[sp]];
      } else {
        ptr = NULL;
        last = NULL;
      }

      sp++;

   } while( (ptr) && tags[tp] );

  gtk_notebook_set_current_page( GTK_NOTEBOOK(notebook), default_section );
  gtk_widget_show (notebook); 

  g_signal_connect(window, "response",
                   G_CALLBACK(about_response), NULL);
  
  gtk_widget_show (window);
}

static void popup_about_notebook(enum ABOUT_SECTIONS default_section)
{
  static char *about_text[] = 
  {
    "\"" NAME "\" is an emulator for the Honeywell series-16",
    "range of computers that were built in the late 1960s",
    "and early 1970s.",
    "",
    "Written by Adrian Wise",
    "apw@adrianwise.co.uk",
    "http://www.series16.adrianwise.co.uk",
    NULL
  };

  about_notebook(NAME, about_text, default_section);
}
