/* Honeywell Series 16 emulator
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
 */

#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>

#include "fp.h"
#include "menu.h"

static char *reg_names[RB_NUM] =
{  "X", "A", "B", "OP", "P/Y", "M" };

struct BUTTON_OF_SIXTEEN;
struct ONE_REG_BUTTON;

struct FRONT_PANEL
{
	struct FP_INTF *intf;

	struct BUTTON_OF_SIXTEEN *bit[17];
	struct ONE_REG_BUTTON *reg[RB_NUM];

	int current_reg;
	int saved_reg;

	short dummy_value[RB_NUM+1];
	short *saved_pointers[RB_NUM];

	gint run_idle_tag;
	GtkWidget *start_button;

	int power;

};

static void power_off(struct FRONT_PANEL *fp);

gint delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	/*g_print ("delete event occured\n");*/
	/* if you return FALSE in the "delete_event" signal handler,
	 * GTK will emit the "destroy" signal.  Returning TRUE means
	 * you don't want the window to be destroyed.
	 * This is useful for popping up 'are you sure you want to quit ?'
	 * type dialogs. */
	
	/* Change TRUE to FALSE and the main window will be destroyed with
	 * a "delete_event". */
	
	return (FALSE);
}

/* another callback */
void destroy (GtkWidget *widget, gpointer data)
{
	gtk_main_quit ();
}


/**********************************************************************
 * Create a labelled button
 *
 * The idea is to instantiate a button with a label just above it.
 * This is to suit the front-panel style.
 **********************************************************************/

static GtkWidget *labelled_button(char *label_text, char *lower_text,
																	int toggle,
																	void (*callback)(GtkWidget *widget,
																									 gpointer data),
																	gpointer data,
																	GtkWidget **pbutton)
{
	GtkWidget *box;
	GtkWidget *label;
	GtkWidget *button;

	box = gtk_vbox_new (0, 0);	

	label = gtk_label_new( label_text );
	gtk_box_pack_start (GTK_BOX (box), label, 0, 0, 0);
	gtk_widget_show (label);

	if (toggle)
		button = gtk_toggle_button_new_with_label("   ");
	else
		button = gtk_button_new_with_label("   ");

	gtk_signal_connect (GTK_OBJECT (button), "clicked",
											GTK_SIGNAL_FUNC (callback), data);
		
	gtk_box_pack_start (GTK_BOX (box), button, 0, 0, 0);
	gtk_widget_show (button);

	if (lower_text)
		{
			label = gtk_label_new( lower_text );
			gtk_box_pack_start (GTK_BOX (box), label, 0, 0, 0);
			gtk_widget_show (label);
		}

	if (pbutton)
		*pbutton = button;
	return box;
}


/**********************************************************************
 * sixteen...
 *
 * Actually there are seventeen buttons in a row, sixteen representing
 * the 16-bit register that is selected and a "clear" button.
 **********************************************************************/
struct BUTTON_OF_SIXTEEN
{
	int n; /* bit number 0-15, 16 indicates clear */
	struct FRONT_PANEL *fp;
	GtkWidget *widget;
};

static char *annotation[17] =
{
	"P", "MP", NULL,
	"DP", "EA", "ML",
	NULL, "PI", "C",
	"A", "I", "F",
	"T4", "T3", "T2",
	"T1", NULL
};

static void sixteen_callback (GtkWidget *widget, gpointer data)
{
	struct BUTTON_OF_SIXTEEN *bs;
	struct FRONT_PANEL *fp;
	int i;
	int read_only=0;

	bs = (struct BUTTON_OF_SIXTEEN *) data;
	fp = bs->fp;

	if ((fp->current_reg == RB_OP) ||
			(fp->current_reg == RB_NUM))
		read_only = 1;

	if ((fp->intf->mode != FPM_MA) && (fp->intf->mode != FPM_SI))
		read_only = 1;

	if (!fp->power)
		read_only = 1;

	/*printf("read_only = %d, mode=%d, current=%d\n", read_only, fp->intf->mode,
		fp->current_reg );*/

	if (bs->n < 16)
		{
			if (read_only)
				{
					gtk_toggle_button_set_state( GTK_TOGGLE_BUTTON (widget),
																			 ((*fp->intf->reg_value[fp->current_reg])
																				& (1<<bs->n)) ? 1 : 0);
				}
			else
				{
					if (GTK_TOGGLE_BUTTON (widget)->active)
						(*fp->intf->reg_value[fp->current_reg]) |= (1<<bs->n);
					else
						(*fp->intf->reg_value[fp->current_reg]) &= (~(1<<bs->n));
				}
		}
	else
		{
			/* clear button was pressed */
 
			if (!read_only)
				for (i=0; i<16; i++)
					gtk_toggle_button_set_state( GTK_TOGGLE_BUTTON (fp->bit[i]->widget), 0);
		}

	/*printf("value[%d]=0x%04x\n", fp->current_reg, *fp->intf->reg_value[fp->current_reg] );*/
}

static GtkWidget *tripple(int left, int num, struct FRONT_PANEL *fp)
{
	GtkWidget *box;
	GtkWidget *b;
	int i;
	char str[20];

	box = gtk_hbox_new (0, 10);	

	for (i=left; i>(left-num); i--)
		{
			if (i<16)
				sprintf(str, "%d", 16-i);
			else
				sprintf(str, "Clear");
				
			b = labelled_button(str, annotation[i], /*toggle=*/ (i<16),
													sixteen_callback, fp->bit[i],
													&fp->bit[i]->widget);

			gtk_box_pack_start (GTK_BOX (box), b, 0, 0, 0);
			gtk_widget_show (b);
		}
	return box;
}


static GtkWidget *sixteen(struct FRONT_PANEL *fp)
{
	GtkWidget *box;
	GtkWidget *b;
	int i;

	struct BUTTON_OF_SIXTEEN *bs;

	/*
	 * Create the structures that hang the whole thing
	 * together.
	 */

	for (i=0; i<=16; i++)
		{
			bs = malloc (sizeof (struct BUTTON_OF_SIXTEEN));
			bs->n = i;
			bs->fp = fp;
			bs->widget = NULL;
			fp->bit[i] = bs;
		}
	
	box = gtk_hbox_new (0, 20);	

	b=tripple(15, 1, fp);
	gtk_box_pack_start (GTK_BOX (box), b, 1, 0, 0);
	gtk_widget_show (b);

	for (i=14; i>0; i-=3)
		{
			b=tripple(i, 3, fp);
			gtk_box_pack_start (GTK_BOX (box), b, 1, 0, 0);
			gtk_widget_show (b);
		}

	b=tripple(16, 1, fp);
	gtk_box_pack_start (GTK_BOX (box), b, 1, 0, 0);
	gtk_widget_show (b);

	return box;
}

static void set_sixteen(struct FRONT_PANEL *fp)
{
	int i;

	for (i=0; i<16; i++)
		gtk_toggle_button_set_state( GTK_TOGGLE_BUTTON (fp->bit[i]->widget),
																 ((*fp->intf->reg_value[fp->current_reg]) &
																	(1<<i)) ? 1 : 0);
}

/**********************************************************************
 * Register buttons
 *
 **********************************************************************/

struct ONE_REG_BUTTON
{
	struct FRONT_PANEL *fp;
	int n;
	GtkWidget *widget;
};

static void reg_button_callback (GtkWidget *widget, gpointer data)
{
	struct FRONT_PANEL *fp;
	struct ONE_REG_BUTTON *rb;
	int i;
	int n;
	enum RB first;


	rb = (struct ONE_REG_BUTTON *) data;
	fp = rb->fp;
	first = (fp->intf->cpu == CPU_DDP516) ? RB_X : RB_A;

	if (GTK_TOGGLE_BUTTON (widget)->active)
		{
			for (i=first; i<RB_NUM; i++)
				{
					if ((i != rb->n) || (!fp->power))
						gtk_toggle_button_set_state( GTK_TOGGLE_BUTTON
																				 (fp->reg[i]->widget), 0);
				}
		}
	n = RB_NUM;
	for (i=first; i<RB_NUM; i++)
		if (GTK_TOGGLE_BUTTON (fp->reg[i]->widget)->active)
			n = i;
	
	if ((n == RB_NUM) && (fp->power) && (fp->current_reg<RB_NUM))
		gtk_toggle_button_set_state( GTK_TOGGLE_BUTTON
																 (fp->reg[fp->current_reg]->widget), 1);
	else
		{
			fp->current_reg = n;

			/*printf("reg button %d selected\n", n);*/
		}
	
	set_sixteen(fp);
}

static GtkWidget *reg_buttons(struct FRONT_PANEL *fp)
{
	GtkWidget *hbox1, *hbox2;
	GtkWidget *vbox;
	GtkWidget *b;
	GtkWidget *label;
	int i;
	enum RB first;

	struct ONE_REG_BUTTON *rb;

	/*
	 * Create the structures that hang the whole thing
	 * together.
	 */

	for (i=0; i<RB_NUM; i++)
		{
			rb = malloc (sizeof (struct ONE_REG_BUTTON));
			rb->n = i;
			rb->fp = fp;
			rb->widget = NULL;
			fp->reg[i] = rb;
		}

	hbox1 = gtk_hbox_new (0, 0);
	label = gtk_label_new( "REGISTER" );
	gtk_box_pack_end (GTK_BOX (hbox1), label, 0, 0, 0);
	gtk_widget_show (label);

	vbox = gtk_vbox_new (0, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox1, 0, 0, 0);
	gtk_widget_show (hbox1);

	hbox2 = gtk_hbox_new (1, 10);	

	first = (fp->intf->cpu == CPU_DDP516) ? RB_X : RB_A;

	for (i=first; i<RB_NUM; i++)
		{
			b = labelled_button(reg_names[i], NULL, /*toggle=*/1,
													reg_button_callback, fp->reg[i],
													&fp->reg[i]->widget);

			gtk_box_pack_start (GTK_BOX (hbox2), b, 1, 1, 0);
			gtk_widget_show (b);
		}
	
	gtk_toggle_button_set_state( GTK_TOGGLE_BUTTON (fp->reg[RB_PY]->widget), 1);

	gtk_box_pack_start (GTK_BOX (vbox), hbox2, 0, 0, 0);
	gtk_widget_show (hbox2);

	return vbox;
}

/**********************************************************************
 * Create a labelled vertical radio-button group
 *
 * The idea is to instantiate a button with a label just above it.
 * This is to suit the front-panel style.
 **********************************************************************/

struct RADIO_BUTTON;

struct RADIO_GROUP
{
	int value;
	int default_button;
	int biased;
	struct RADIO_BUTTON *buttons;
	GSList *group;

	void (*callback)(GtkWidget *widget,
									 gpointer data, int n);
	gpointer data;
};

struct RADIO_BUTTON
{
	struct RADIO_GROUP *rg;
	GtkWidget *button;
	int n;
};

static gint radio_bias_callback (gpointer data)
{
	gtk_toggle_button_set_state( GTK_TOGGLE_BUTTON(data), 1);
	return FALSE;
}

static void radio_callback (GtkWidget *widget, gpointer data)
{
	struct RADIO_BUTTON *rb;
	struct RADIO_GROUP *rg;

	rb = (struct RADIO_BUTTON *) data;
	rg = rb->rg;

	if (GTK_TOGGLE_BUTTON (widget)->active)
		{
			rg->value = rb->n;
			if (rg->callback)
				rg->callback(widget, rg->data, rg->value);
			else
				*((int *) rg->data) = rg->value;
		}

	/*printf("value=%d\n", rg->value );*/

	if (rg->biased)
		gtk_timeout_add( 250,
                     radio_bias_callback,
										 (gpointer) rg->buttons[rg->default_button].button);
}

static void destroy_radio_group (GtkWidget *widget, gpointer data)
{
	struct RADIO_GROUP *rg = (struct RADIO_GROUP *) data;

	free (rg->buttons);
	free (rg);

	/*printf("destroy_register_group()\n");*/
}

static GtkWidget *labelled_radio_group(char *label_text, int num_buttons,
																			 int default_button, int biased,
																			 char **button_text,
																			 void (*callback)(GtkWidget *widget,
																												gpointer data, int n),
																			 gpointer data)
{
	GtkWidget *box;
	GtkWidget *label;
	GtkWidget *button;
	GSList *group=NULL;
	int i;

	struct RADIO_GROUP *rg;

	rg = malloc(sizeof(struct RADIO_GROUP));
	rg->value = 0;
	rg->default_button = default_button;
	rg->biased = biased;
	rg->buttons = malloc(sizeof(struct RADIO_GROUP) * num_buttons);

	for (i=0; i<num_buttons; i++)
		{
			rg->buttons[i].rg = rg;
			rg->buttons[i].button = NULL;
			rg->buttons[i].n = i;
		}

	rg->group = NULL;
	rg->callback = callback;
	rg->data = data;

	box = gtk_vbox_new (0, 0);	

	if (label_text)
		{
			label = gtk_label_new( label_text );
			gtk_box_pack_start (GTK_BOX (box), label, 0, 0, 0);
			gtk_widget_show (label);
		}

	for (i=0; i<num_buttons; i++)
		{
			if ((button_text) && (button_text[i]))
				button = gtk_radio_button_new_with_label(group, button_text[i]);
			else
				button = gtk_radio_button_new(group);

			rg->buttons[i].button = button;

			group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));

			gtk_signal_connect (GTK_OBJECT (button), "clicked",
													GTK_SIGNAL_FUNC (radio_callback), &rg->buttons[i]);

			gtk_box_pack_start (GTK_BOX (box), button, 0, 0, 0);
			gtk_widget_show (button);
		}

	rg->group = group;

	gtk_toggle_button_set_state( GTK_TOGGLE_BUTTON
															 (rg->buttons[default_button].button), 1);
	rg->value = default_button;

	gtk_signal_connect (GTK_OBJECT (box), "destroy",
											GTK_SIGNAL_FUNC (destroy_radio_group), rg);

	return box;
}

/**********************************************************************
 * Sense switches
 *
 **********************************************************************/
static GtkWidget *sense_switches(struct FRONT_PANEL *fp)
{
	GtkWidget *hbox1, *hbox2;
	GtkWidget *vbox;
	GtkWidget *b;
	GtkWidget *label;
	int i;
	char str[20];

	hbox1 = gtk_hbox_new (0, 0);
	label = gtk_label_new( "SENSE" );
	gtk_box_pack_end (GTK_BOX (hbox1), label, 0, 0, 0);
	gtk_widget_show (label);

	vbox = gtk_vbox_new (0, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox1, 0, 0, 0);
	gtk_widget_show (hbox1);

	hbox2 = gtk_hbox_new (0, 10);	

	for (i=0; i<4; i++)
		{
			sprintf(str, "%d", i+1);

			b = labelled_radio_group(str, 2,
															 *fp->intf->ssw[i], 0, NULL,
															 NULL,
															 (gpointer) fp->intf->ssw[i]);

			gtk_box_pack_start (GTK_BOX (hbox2), b, 0, 0, 0);
			gtk_widget_show (b);
		}

	gtk_box_pack_start (GTK_BOX (vbox), hbox2, 0, 0, 0);
	gtk_widget_show (hbox2);
	
	return vbox;
}

/**********************************************************************
 * Misc call-backs
 *
 **********************************************************************/
static void master_clear_callback (GtkWidget *widget, gpointer data, int n)
{
	struct FRONT_PANEL *fp = (struct FRONT_PANEL *)data;

	if ((n!=0) && (fp->intf->mode != FPM_RUN) && (fp->power))
		{
			(*fp->intf->master_clear)(fp->intf);
			set_sixteen(fp);
		}
}

static int run_idle_function (gpointer data)
{
	struct FRONT_PANEL *fp = (struct FRONT_PANEL *)data;

	(*fp->intf->run)(fp->intf);

	set_sixteen(fp);
 
	if ((fp->intf->mode == FPM_RUN) && (!fp->intf->running))
		{
			fp->intf->mode = FPM_SI; /* fool the start_callback() routine */
			gtk_toggle_button_set_state( GTK_TOGGLE_BUTTON(fp->start_button), 0);
			fp->intf->mode = FPM_RUN;
			
			if (fp->intf->power_fail_interrupt_acknowledge)
				{
					fp->intf->power_fail_interrupt_acknowledge = 0;
					power_off(fp);
				}
		}
	return fp->intf->running;
}

static void start_callback (GtkWidget *widget, gpointer data)
{
	struct FRONT_PANEL *fp = (struct FRONT_PANEL *)data;

	//printf("%s active=%d\n", __PRETTY_FUNCTION__,
	//			 GTK_TOGGLE_BUTTON (widget)->active);

	if (fp->intf->running)
		{
			//printf("%s running\n", __PRETTY_FUNCTION__);
			
			fp->intf->start_button_interrupt_pending = 1;

			/* Since the computer is still running, set the
				 button back to 'on', over-riding the toggle-button
				 behaviour */
			gtk_toggle_button_set_state( GTK_TOGGLE_BUTTON(fp->start_button), 1);
		}
	else if (GTK_TOGGLE_BUTTON (widget)->active)
		{
			//printf("%s active\n", __PRETTY_FUNCTION__);

			if ((fp->intf->mode == FPM_RUN) && (fp->power))
				{
					fp->intf->running = 1;
					fp->run_idle_tag = gtk_idle_add( (GtkFunction) run_idle_function,
																					 (gpointer) fp );
				}
			else
				{
					if (fp->power)
						(void) (*fp->intf->run)(fp->intf);
					set_sixteen(fp);
					gtk_toggle_button_set_state( GTK_TOGGLE_BUTTON(fp->start_button), 0);
				}
		}
	//else
	//printf("%s NOTA\n", __PRETTY_FUNCTION__);

}

static void mode_callback (GtkWidget *widget, gpointer data, int n)
{
	struct FRONT_PANEL *fp = (struct FRONT_PANEL *) data;
	
	fp->intf->mode = n;

	if ((fp->intf->running) && (fp->intf->mode != FPM_RUN))
		{
			fp->intf->running = 0;
			gtk_idle_remove( fp->run_idle_tag );
			gtk_toggle_button_set_state( GTK_TOGGLE_BUTTON(fp->start_button), 0);
		}
}

static void power_off(struct FRONT_PANEL *fp)
{
	int i;

	fp->power = 0;
	for (i=0; i<RB_NUM; i++)
		{
			fp->saved_pointers[i] = fp->intf->reg_value[i];
			fp->intf->reg_value[i] = &fp->dummy_value[i];
			fp->dummy_value[i] = 0;
		}
	fp->dummy_value[RB_NUM] = 0;

	fp->saved_reg = fp->current_reg;
	if (fp->current_reg != RB_NUM)
		reg_button_callback (fp->reg[fp->current_reg]->widget,
												 fp->reg[fp->current_reg]);
	set_sixteen(fp);
}

static void power_callback (GtkWidget *widget, gpointer data, int n)
{
	struct FRONT_PANEL *fp = (struct FRONT_PANEL *) data;
	int i;

	if (n)
		{
			/* Turn the power back on */
			fp->power = n;
			
			for (i=0; i<RB_NUM; i++)			
				fp->intf->reg_value[i] = fp->saved_pointers[i];
			fp->dummy_value[RB_NUM] = ~0;
			fp->current_reg = fp->saved_reg;
			if (fp->current_reg != RB_NUM)
				reg_button_callback (fp->reg[fp->current_reg]->widget,
														 fp->reg[fp->current_reg]);
			(*fp->intf->master_clear)(fp->intf);
			set_sixteen(fp);
		}
	else
		{
			if (fp->intf->running)
				{
					/* Need to do PFI/PFH sequence */
					fp->intf->power_fail_interrupt_pending = 1;
				}
			else
				power_off(fp);
		}
}

/**********************************************************************
 * lower row of switches
 *
 **********************************************************************/
static GtkWidget *lower(struct FRONT_PANEL *fp)
{
	GtkWidget *box;
	GtkWidget *vbox;
	GtkWidget *b;
	GtkWidget *reg_b;
	static char *power_labels[] = {"OFF", "ON"};
	static char *pfi_labels[] = {"PFI", "PFH"};
	static char *fetch_labels[] = {"FETCH", "STORE"};
	static char *p_labels[] = {"P+1", "P"};
	static char *mode_labels[] = {"MA", "SI", "RUN"};

	box = gtk_hbox_new (0, 10);	

	fp->power = 1;
	fp->saved_reg = fp->current_reg = RB_PY;
	reg_b = reg_buttons(fp); /* need this before the power button */

	vbox = gtk_vbox_new (0, 0);
	b = labelled_radio_group(NULL, 2,
													 1, 0, power_labels,
													 power_callback,
													 fp);
	gtk_box_pack_end (GTK_BOX (vbox), b, 0, 0, 0);
	gtk_widget_show (b);
	gtk_box_pack_start (GTK_BOX (box), vbox, 1, 0, 0);
	gtk_widget_show (vbox);

	vbox = gtk_vbox_new (0, 0);	
	b = sense_switches(fp);
	gtk_box_pack_end (GTK_BOX (vbox), b, 0, 0, 0);
	gtk_widget_show (b);
	gtk_box_pack_start (GTK_BOX (box), vbox, 1, 0, 0);
	gtk_widget_show (vbox);

	vbox = gtk_vbox_new (0, 0);	
	gtk_box_pack_end (GTK_BOX (vbox), reg_b, 0, 0, 0);
	gtk_widget_show (reg_b);
	gtk_box_pack_start (GTK_BOX (box), vbox, 1, 0, 0);
	gtk_widget_show (vbox);

	vbox = gtk_vbox_new (0, 0);	
	b = labelled_radio_group("MSTR", 2,
													 0, 1, NULL,
													 master_clear_callback,
													 (gpointer) fp);
	/*b = labelled_button("MSTR", 0,
		master_clear_callback, fp, NULL);*/
	gtk_box_pack_end (GTK_BOX (vbox), b, 0, 0, 0);
	gtk_widget_show (b);
	gtk_box_pack_start (GTK_BOX (box), vbox, 1, 0, 0);
	gtk_widget_show (vbox);

	if (fp->intf->cpu == CPU_DDP516)
		{
			vbox = gtk_vbox_new (0, 0);	
			b = labelled_radio_group(NULL, 2,
															 0, 0, pfi_labels,
															 NULL,
															 &fp->intf->pfh_not_pfi);
			gtk_box_pack_end (GTK_BOX (vbox), b, 0, 0, 0);
			gtk_widget_show (b);
			gtk_box_pack_start (GTK_BOX (box), vbox, 1, 0, 0);
			gtk_widget_show (vbox);
		}

	vbox = gtk_vbox_new (0, 0);	
	b = labelled_radio_group(NULL, 2,
													 0, 0, fetch_labels,
													 NULL,
													 &fp->intf->store);
	gtk_box_pack_end (GTK_BOX (vbox), b, 0, 0, 0);
	gtk_widget_show (b);
	gtk_box_pack_start (GTK_BOX (box), vbox, 1, 0, 0);
	gtk_widget_show (vbox);

	vbox = gtk_vbox_new (0, 0);	
	b = labelled_radio_group(NULL, 2,
													 1, 0, p_labels,
													 NULL,
													 &fp->intf->p_not_pp1);
	gtk_box_pack_end (GTK_BOX (vbox), b, 0, 0, 0);
	gtk_widget_show (b);
	gtk_box_pack_start (GTK_BOX (box), vbox, 1, 0, 0);
	gtk_widget_show (vbox);

	vbox = gtk_vbox_new (0, 0);	
	b = labelled_radio_group(NULL, 3,
													 2, 0, mode_labels,
													 mode_callback,
													 fp);
	gtk_box_pack_end (GTK_BOX (vbox), b, 0, 0, 0);
	gtk_widget_show (b);
	gtk_box_pack_start (GTK_BOX (box), vbox, 1, 0, 0);
	gtk_widget_show (vbox);

	vbox = gtk_vbox_new (0, 0);	
	b = labelled_button("START", NULL, 1,
											start_callback, fp, &fp->start_button);
	gtk_box_pack_end (GTK_BOX (vbox), b, 0, 0, 0);
	gtk_widget_show (b);
	gtk_box_pack_start (GTK_BOX (box), vbox, 1, 0, 0);
	gtk_widget_show (vbox);

	return box;
}

/**********************************************************************
 * Front panel as a whole
 *
 **********************************************************************/
static void destroy_front_panel (GtkWidget *widget, gpointer data)
{
	struct FRONT_PANEL *fp = (struct FRONT_PANEL *) data;
	int i;

	for (i=0; i<17; i++)
		free(fp->bit[i]);

	for (i=0; i<RB_NUM; i++)
		free(fp->reg[i]);

	free (fp);

	/*printf("destroy_front_panel()\n");*/
}

static GtkWidget *front_panel(struct FP_INTF *intf)
{
	struct FRONT_PANEL *fp;
	GtkWidget *box;
	GtkWidget *b;
	int i;

	fp = malloc(sizeof(struct FRONT_PANEL));

	fp->intf = intf;

	fp->intf->reg_value[RB_NUM] = &fp->dummy_value[RB_NUM];
	fp->dummy_value[RB_NUM] = ~0;
	for (i=0; i<RB_NUM; i++)
		{
			fp->dummy_value[i] = 0;
			fp->saved_pointers[i] = fp->intf->reg_value[i];
		}

	box = gtk_vbox_new (0, 10);	

	b = sixteen(fp);
	gtk_box_pack_start (GTK_BOX (box), b, 1, 0, 0);
	gtk_widget_show (b);

	b = gtk_hseparator_new ();
	gtk_box_pack_start (GTK_BOX (box), b, 1, 0, 0);
	gtk_widget_show (b);

	b = lower(fp);
	gtk_box_pack_start (GTK_BOX (box), b, 1, 0, 0);
	gtk_widget_show (b);
	
	gtk_signal_connect (GTK_OBJECT (box), "destroy",
											GTK_SIGNAL_FUNC (destroy_front_panel), fp);

	/*
	 * Now everything is setup
	 * set the P/Y register to 1
	 */
	*intf->reg_value[RB_PY] = 1;
	set_sixteen(fp);

	return box;
}

void process_args(int *argc, char ***argv)
{
	gtk_init (argc, argv);
}

extern void setup_fp(struct FP_INTF *intf)
{
	/* GtkWidget is the storage type for widgets */
	GtkWidget *window;
	GtkWidget *vbox;
	GtkWidget *menu_bar;
	GtkWidget *fpw;
	
	/* create a new window */
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	
	/* when the window is given the "delete_event" signal (this is given
	 * by the window manager (usually the 'close' option, or on the
	 * titlebar), we ask it to call the delete_event () function
	 * as defined above.  The data passed to the callback
	 * function is NULL and is ignored in the callback. */
	gtk_signal_connect (GTK_OBJECT (window), "delete_event",
											GTK_SIGNAL_FUNC (delete_event), NULL);
	
	/* here we connect the "destroy" event to a signal handler.
	 * This event occurs when we call gtk_widget_destroy() on the window,
	 * or if we return 'FALSE' in the "delete_event" callback. */
	gtk_signal_connect (GTK_OBJECT (window), "destroy",
											GTK_SIGNAL_FUNC (destroy), NULL);
	
	/* sets the border width of the window. */
	gtk_container_border_width (GTK_CONTAINER (window), 10);
	
	vbox = gtk_vbox_new (0, 5);
	gtk_container_add (GTK_CONTAINER (window), vbox);

	menu_bar = make_menu_bar();
	gtk_box_pack_start(GTK_BOX(vbox), menu_bar, FALSE, FALSE, 0);
	gtk_widget_show (menu_bar);

	fpw = front_panel(intf);
	gtk_box_pack_start(GTK_BOX(vbox), fpw, TRUE, TRUE, 0);

	gtk_widget_show (fpw);
	gtk_widget_show (vbox);

	gtk_widget_show (window);
	
	copyright();

	/* all GTK applications must have a gtk_main().     Control ends here
	 * and waits for an event to occur (like a key press or mouse event). */
	gtk_main ();
}

