#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gstdio.h>
#include "bcard.h"


static void show_nums (gchar, gchar);


void
select_cifra (struct info *vars, gchar *buf)
{
	gint wc1 = ((vars->row_val - 1) * 4) + vars->c1_val - 1;
	gint wc2 = ((vars->row_val - 1) * 4) + vars->c2_val - 1;
	
	show_nums (buf[wc1], buf[wc2]);
}


static void
show_nums (gchar c1, gchar c2)
{
	GtkWidget *dialog;
	
   	dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE, "%c - %c", c1, c2);
   	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}
