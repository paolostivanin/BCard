#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <gtk/gtk.h>
#include <glib.h>
#include "bcard.h"


static gint check_input (GtkWidget *, struct info *);
static void error_message (const gchar *);
static void type_pwd (struct info *);
static void ins_pwd (struct info *);
gint decrypt_db (struct info *);


gint
main (	gint argc,
	gchar **argv)
{
	struct info vars;

	GtkWidget *mainwin;
	GtkWidget *but;
	GtkWidget *grid;
	GtkWidget *label, *r1_label, *c1_label, *c2_label;

	gtk_init (&argc, &argv);
	
	mainwin = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_position (GTK_WINDOW (mainwin), GTK_WIN_POS_CENTER);
	gtk_window_set_default_size (GTK_WINDOW (mainwin), 300, 250);
	gtk_window_set_title (GTK_WINDOW (mainwin), "BCard");
	gtk_window_set_resizable (GTK_WINDOW (mainwin), FALSE);
	g_signal_connect (mainwin, "destroy", G_CALLBACK (gtk_main_quit), NULL);
	gtk_container_set_border_width (GTK_CONTAINER (mainwin), 10);

	const gchar *str = "Welcome to BCard";
	label = gtk_label_new (str);
	char *markup;
	markup = g_markup_printf_escaped ("<span foreground=\"black\" size=\"x-large\"><b>%s</b></span>", str);
	gtk_label_set_markup (GTK_LABEL (label), markup);
	g_free (markup);

	vars.row_entry = gtk_entry_new ();
	vars.c1_entry = gtk_entry_new ();
	vars.c2_entry = gtk_entry_new ();

	gtk_entry_set_visibility (GTK_ENTRY (vars.row_entry), FALSE);
	gtk_entry_set_max_length (GTK_ENTRY (vars.row_entry), 2);

	gtk_entry_set_visibility (GTK_ENTRY (vars.c1_entry), FALSE);
	gtk_entry_set_max_length (GTK_ENTRY (vars.c1_entry), 1);

	gtk_entry_set_visibility (GTK_ENTRY (vars.c2_entry), FALSE);
	gtk_entry_set_max_length (GTK_ENTRY (vars.c2_entry), 1);

	r1_label = gtk_label_new ("Riga");
	c1_label = gtk_label_new ("Cifra 1");
	c2_label = gtk_label_new ("Cifra 2");

	but = gtk_button_new_with_label ("OK");
	g_signal_connect (but, "clicked", G_CALLBACK (check_input), &vars);

	grid = gtk_grid_new();
	gtk_container_add (GTK_CONTAINER (mainwin), grid);
	gtk_grid_set_row_homogeneous (GTK_GRID (grid), TRUE);
	gtk_grid_set_column_homogeneous (GTK_GRID (grid), TRUE);
	gtk_grid_set_row_spacing (GTK_GRID (grid), 5);
	gtk_grid_set_column_spacing (GTK_GRID (grid), 5);

	gtk_grid_attach (GTK_GRID (grid), label, 0, 0, 3, 1);
	gtk_grid_attach (GTK_GRID (grid), r1_label, 0, 1, 1, 1);
	gtk_grid_attach (GTK_GRID (grid), vars.row_entry, 1, 1, 2, 1);
	gtk_grid_attach (GTK_GRID (grid), c1_label, 0, 2, 1, 1);
	gtk_grid_attach (GTK_GRID (grid), vars.c1_entry, 1, 2, 2, 1);
	gtk_grid_attach (GTK_GRID (grid), c2_label, 0, 3, 1, 1);
	gtk_grid_attach (GTK_GRID (grid), vars.c2_entry, 1, 3, 2, 1);
	gtk_grid_attach (GTK_GRID (grid), but, 1, 4, 2, 1);

	gtk_widget_show_all (mainwin);
	
	gtk_main();
	
	return 0;
}


static gint
check_input (	GtkWidget __attribute__((__unused__)) *ignored,
		struct info *vars)
{
	gchar *ep;
	gulong tmp_val;
	const gchar *tmp_row_val = gtk_entry_get_text (GTK_ENTRY (vars->row_entry));
   	const gchar *tmp_c1_val = gtk_entry_get_text (GTK_ENTRY (vars->c1_entry));
   	const gchar *tmp_c2_val = gtk_entry_get_text (GTK_ENTRY (vars->c2_entry));
   	
   	errno = 0;
   	tmp_val = strtoul (tmp_row_val, &ep, 10);
	if (errno == ERANGE || tmp_val < 1 || tmp_val > 32)
	{
		error_message ("Il valore della riga deve essere compreso fra 1 e 32");
		return -1;
	}
	vars->row_val = (guint) tmp_val;
	
	errno = 0;
   	tmp_val = strtoul (tmp_c1_val, &ep, 10);
	if (errno == ERANGE || tmp_val < 1 || tmp_val > 4)
	{
		error_message ("Il valore di cifra1 deve essere compreso fra 1 e 4");
		return -1;
	}
	vars->c1_val = (guint) tmp_val;
	
	errno = 0;
   	vars->c2_val = strtoul (tmp_c2_val, &ep, 10);
	if (errno == ERANGE || tmp_val < 1 || tmp_val > 4)
	{
		error_message ("Il valore di cifra2 deve essere compreso fra 1 e 4");
		return -1;
	}
	vars->c2_val = (guint) tmp_val;
   	
   	type_pwd (vars);
   	
   	return 0;
}


static void
error_message (const gchar *message)
{
	GtkWidget *dialog;
   	dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Errore: %s", message);
   	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}


static void
type_pwd (struct info *vars)
{
	GtkWidget *dialog, *content_area, *grid2, *label;
   	dialog = gtk_dialog_new_with_buttons (	"Password", NULL,
						GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						"_OK", GTK_RESPONSE_ACCEPT, "_Cancel", GTK_RESPONSE_REJECT,
						NULL);
   	content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
   	
   	label = gtk_label_new ("Inserisci password");
   	vars->pwd_entry = gtk_entry_new ();
   	gtk_entry_set_visibility (GTK_ENTRY(vars->pwd_entry), FALSE);

   	gtk_widget_set_size_request (dialog, 150, 100);
   	
   	grid2 = gtk_grid_new ();
	gtk_grid_set_row_homogeneous (GTK_GRID (grid2), TRUE);
	gtk_grid_set_column_homogeneous (GTK_GRID (grid2), TRUE);
	gtk_grid_set_row_spacing (GTK_GRID (grid2), 5);
	
	gtk_grid_attach(GTK_GRID(grid2), label, 0, 0, 3, 1);
	gtk_grid_attach(GTK_GRID(grid2), vars->pwd_entry, 0, 1, 3, 1);
	
	gtk_container_add (GTK_CONTAINER (content_area), grid2);
	gtk_widget_show_all (dialog);

	gint result = gtk_dialog_run (GTK_DIALOG (dialog));
	switch (result)
	{
		case GTK_RESPONSE_ACCEPT:
			ins_pwd (vars);
			break;
		case GTK_RESPONSE_REJECT:
			break;
		default:
			break;
	}
	
	gtk_widget_destroy (dialog);
}


static void
ins_pwd (struct info *vars)
{
	gint ret;
	vars->pwd_length = gtk_entry_get_text_length (GTK_ENTRY (vars->pwd_entry)) + 1;
	vars->input_key = g_malloc (vars->pwd_length);
	if (vars->input_key == NULL)
	{
		g_printerr ("Memory allocation error\n");
		return;
	}

	strncpy (vars->input_key, (const gchar *) gtk_entry_get_text (GTK_ENTRY (vars->pwd_entry)), vars->pwd_length);
	ret = decrypt_db (vars);
	if (ret == -1)
		exit (EXIT_FAILURE);
		

	g_free (vars->input_key);
}
