/* Honeywell Series 16 emulator $Id: menu.c,v 1.3 2001/06/09 22:23:46 adrian Exp $
 * Copyright (C) 1998  Adrian Wise
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
 * $Log: menu.c,v $
 * Revision 1.3  2001/06/09 22:23:46  adrian
 * Updated version number
 *
 * Revision 1.2  1999/09/10 07:35:59  adrian
 * Added #include of stdio.h
 *
 * Revision 1.1  1999/02/20 00:11:29  adrian
 * Initial revision
 *
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
static void widget_popup_about_notebook(GtkWidget *widget,
                                        enum ABOUT_SECTIONS default_section);

#define COPYRIGHT_TIMEOUT 3000


struct COPYRIGHT_STRUCT 
{
  gint timeout_tag;
  GtkWidget *window;
};

static gint copyright_timeout( gpointer data)
{
  struct COPYRIGHT_STRUCT *cs = (struct COPYRIGHT_STRUCT *) data;

  gtk_widget_destroy(cs->window);
  
  g_free(cs);
  return FALSE;
}

static void copyright_close( GtkWidget *widget, gpointer data )
{
  struct COPYRIGHT_STRUCT *cs = (struct COPYRIGHT_STRUCT *) data;

  gtk_timeout_remove(cs->timeout_tag);
  
  gtk_widget_destroy(cs->window);

  g_free(cs);
}

void copyright()
{
  GtkWidget *window;
  GtkWidget *hbox, *text, *vscrollbar;
  GtkWidget *button;
  int i;
  struct COPYRIGHT_STRUCT *cs;

  window = gtk_dialog_new ();

  gtk_window_set_title (GTK_WINDOW (window), "Copyright");
  gtk_container_border_width (GTK_CONTAINER (window), 0);
  gtk_widget_set_usize(window, 500, 200);

  hbox = gtk_hbox_new (0, 0); 

  text = gtk_text_new( NULL, NULL );
  gtk_text_set_editable (GTK_TEXT (text), FALSE);
  gtk_text_set_word_wrap(GTK_TEXT (text), TRUE);

  gtk_box_pack_start(GTK_BOX(hbox), text, TRUE, TRUE, 0);
  gtk_widget_show (text); 

  vscrollbar = gtk_vscrollbar_new (GTK_TEXT(text)->vadj);
  gtk_box_pack_start(GTK_BOX(hbox), vscrollbar, FALSE, FALSE, 0);
  gtk_widget_show (vscrollbar); 

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG(window)->vbox), hbox, 
                      TRUE, TRUE, 0);
  gtk_widget_show (hbox); 

  gtk_widget_realize (text);
  gtk_text_freeze (GTK_TEXT (text));

  for (i=0; copyright_text[i]; i++)
    gtk_text_insert( GTK_TEXT(text),
                     NULL, &text->style->black, NULL,
                     copyright_text[i],
                     -1);
  for (i=0; instructions_text[i]; i++)
    gtk_text_insert( GTK_TEXT(text),
                     NULL, &text->style->black, NULL,
                     instructions_text[i],
                     -1);

  gtk_text_thaw (GTK_TEXT (text));
  
  cs = g_malloc(sizeof(struct COPYRIGHT_STRUCT *));

  cs->timeout_tag = gtk_timeout_add( COPYRIGHT_TIMEOUT,
                                     copyright_timeout,
                                     cs);
  cs->window = window;

  hbox = gtk_hbox_new (TRUE, 10); 
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area),
                      hbox, TRUE, TRUE, 0);

  button = gtk_button_new_with_label ("Warranty");
  gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
  gtk_signal_connect(GTK_OBJECT (button), "clicked",
                     (GtkSignalFunc) widget_popup_about_notebook,
                     (gpointer) AB_WARRANTY);
  gtk_widget_show(button);

  button = gtk_button_new_with_label ("Copying");
  gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
  gtk_signal_connect(GTK_OBJECT (button), "clicked",
                     (GtkSignalFunc) widget_popup_about_notebook,
                     (gpointer) AB_COPYING);
  gtk_widget_show(button);

  
  /* Add a "close" button to the bottom of the dialog */
  button = gtk_button_new_with_label ("close");
  gtk_signal_connect(GTK_OBJECT (button), "clicked",
                     (GtkSignalFunc) copyright_close,
                     cs);
  gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
  gtk_widget_show(button);
  gtk_widget_show(hbox);
    
  /* this makes it so the button is the default. */
  
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  
  /* This grabs this button to be the default button. Simply hitting
   * the "Enter" key will cause this button to activate. */
  gtk_widget_grab_default (button);
  
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
  gtk_menu_append( GTK_MENU(file_menu), quit_item);

  /* We can attach the Quit menu item to our exit function */
  gtk_signal_connect( GTK_OBJECT(quit_item), "activate",
                      GTK_SIGNAL_FUNC(file_menu_quit), NULL);

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
  gtk_menu_append( GTK_MENU(help_menu), about_item);
  gtk_menu_append( GTK_MENU(help_menu), full_license_item);
  gtk_menu_append( GTK_MENU(help_menu), copying_item);
  gtk_menu_append( GTK_MENU(help_menu), warranty_item);

  /* We can attach the Quit menu item to our exit function */
  gtk_signal_connect( GTK_OBJECT(about_item), "activate",
                      GTK_SIGNAL_FUNC(help_menu_about), (gpointer) AB_ABOUT);
  gtk_signal_connect( GTK_OBJECT(full_license_item), "activate",
                      GTK_SIGNAL_FUNC(help_menu_about), (gpointer) AB_FULL);
  gtk_signal_connect( GTK_OBJECT(copying_item), "activate",
                      GTK_SIGNAL_FUNC(help_menu_about), (gpointer) AB_COPYING);
  gtk_signal_connect( GTK_OBJECT(warranty_item), "activate",
                      GTK_SIGNAL_FUNC(help_menu_about), (gpointer) AB_WARRANTY);

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
  gtk_menu_bar_append( GTK_MENU_BAR (menu_bar), file_item );

  help_item = make_help_menu();
  gtk_menu_item_right_justify( GTK_MENU_ITEM(help_item) );
  gtk_menu_bar_append( GTK_MENU_BAR (menu_bar), help_item );

  return menu_bar;
}


/**********************************************************************
 * Pop-up, about notebook
 *
 **********************************************************************/

static void about_notebook(char *prognam, char *about_text[],
                           enum ABOUT_SECTIONS default_section)
{
  GtkWidget *window;
  GtkWidget *notebook;
  GtkWidget *button;
  GtkWidget *hbox;
  GtkWidget *text;
  GtkWidget *label;
  GtkWidget *vscrollbar;
  char str[1024];
  char **ptr = about_text;
  static char *tags[] = {"About",
                         "Full License", "Preamble",
                         "Copying", "Warranty", "Howto",
                         NULL};
  int i;
  int tp = 0;
  int sp = 0;
  char **last=NULL;

  window = gtk_dialog_new ();

  sprintf(str, "About %s", prognam);
  gtk_window_set_title (GTK_WINDOW (window), str);
  gtk_container_border_width (GTK_CONTAINER (window), 0);
  gtk_widget_set_usize(window, 600, 200);

  notebook = gtk_notebook_new();

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG(window)->vbox), notebook, 
                      TRUE, TRUE, 0);

  do
    {
      hbox = gtk_hbox_new (0, 0); 

      label = gtk_label_new(tags[tp]);

      gtk_notebook_append_page( GTK_NOTEBOOK(notebook),
                                GTK_WIDGET(hbox),
                                label );
      
      text = gtk_text_new( NULL, NULL );
      gtk_text_set_editable (GTK_TEXT (text), FALSE);
      gtk_text_set_word_wrap(GTK_TEXT (text), TRUE);
      
      gtk_box_pack_start(GTK_BOX(hbox), text, TRUE, TRUE, 0);
      gtk_widget_show (text); 
      
      vscrollbar = gtk_vscrollbar_new (GTK_TEXT(text)->vadj);
      gtk_box_pack_start(GTK_BOX(hbox), vscrollbar, FALSE, FALSE, 0);
      gtk_widget_show (vscrollbar); 
      

      gtk_widget_show (hbox); 
      
      gtk_widget_realize (text);
      gtk_text_freeze (GTK_TEXT (text));
      for (i=0; ptr[i] && ((!last) || (&ptr[i]<last)); i++)
        {
          sprintf(str, "%s\n", ptr[i]);
          gtk_text_insert( GTK_TEXT(text),
                           NULL, NULL, NULL,
                           str,
                           -1);
        }
      gtk_text_thaw (GTK_TEXT (text));
      
      tp++; /* Next tag */

      ptr = NULL;
      if (sp == 0)
        {
          ptr = full_license_text;
          last = NULL;
        }
      else if (license_sections[sp]>=0)
        {
          ptr = &full_license_text[license_sections[sp-1]];
          last = &full_license_text[license_sections[sp]];
        }
      else
        {
          ptr = NULL;
          last = NULL;
        }

      sp++;

    } while( (ptr) && tags[tp] );

  gtk_notebook_set_page( GTK_NOTEBOOK(notebook), default_section );
  gtk_widget_show (notebook); 

  /* Add a "close" button to the bottom of the dialog */
  button = gtk_button_new_with_label ("close");
  gtk_signal_connect_object(GTK_OBJECT (button), "clicked",
                            (GtkSignalFunc) gtk_widget_destroy,
                            GTK_OBJECT (window));
     
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area),
                      button, TRUE, TRUE, 0);
  gtk_widget_show (button);
  
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

static void widget_popup_about_notebook(GtkWidget *widget,
                                        enum ABOUT_SECTIONS default_section)
{
  popup_about_notebook(default_section);
}
