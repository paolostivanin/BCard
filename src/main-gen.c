#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <gcrypt.h>
#include "bcard.h"


static void parse_rows (GtkWidget *, gpointer);
static void clean_and_quit (GtkWidget *, gpointer);
static void encrypt_file (gpointer, struct row_entries *);
static void select_folder_dialog (GtkWidget *, gpointer);
static void show_error (const gchar *);
guchar *calculate_hmac (const guchar *, gsize, gint, gchar *);


gint
main (	gint argc,
	gchar **argv)
{
	if (!gcry_check_version ("1.5.0"))
	{
		show_error ("The required version of GCrypt is 1.5.0 or greater.");
		return -1;
	}
	gcry_control (GCRYCTL_INIT_SECMEM, 16384, 0);
	gcry_control (GCRYCTL_INITIALIZATION_FINISHED, 0);
	
	GtkWidget *mainwin;
	GtkWidget *grid, *label;
	GtkWidget *button[3];
	GtkWidget *row_label[ROW_NUMBERS];
	
	gint i, j, k;
	gchar name[10];
	
	struct row_entries entry;
	
	entry.db_path = NULL;
	
	gtk_init (&argc, &argv);
	
	mainwin = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_position (GTK_WINDOW (mainwin), GTK_WIN_POS_CENTER);
	gtk_window_set_default_size (GTK_WINDOW (mainwin), 300, 250);
	gtk_window_set_title (GTK_WINDOW (mainwin), "WeBank Card Generator");
	gtk_window_set_resizable (GTK_WINDOW (mainwin), FALSE);
	g_signal_connect (mainwin, "destroy", G_CALLBACK (gtk_main_quit), NULL);
	gtk_container_set_border_width (GTK_CONTAINER (mainwin), 10);

	const gchar *str = "Generatore Carta Telematica";
	label = gtk_label_new (str);
	char *markup;
	markup = g_markup_printf_escaped ("<span foreground=\"black\" size=\"x-large\"><b>%s</b></span>", str);
	gtk_label_set_markup (GTK_LABEL (label), markup);
	g_free (markup);

	for (i = 0; i < ROW_NUMBERS; i++)
	{
		memset (name, 0, sizeof (name));
		g_snprintf (name, sizeof (name), "Riga %d", i+1);
		row_label[i] = gtk_label_new (name);
		entry.row[i] = gtk_entry_new ();
		gtk_entry_set_visibility (GTK_ENTRY (entry.row[i]), FALSE);
		gtk_entry_set_max_length (GTK_ENTRY (entry.row[i]), 4);
	}
	
	entry.pwd[0] = gtk_entry_new ();
	gtk_entry_set_visibility (GTK_ENTRY (entry.pwd[0]), FALSE);
	gtk_entry_set_placeholder_text (GTK_ENTRY (entry.pwd[0]), "Type password");
	
	entry.pwd[1] = gtk_entry_new ();
	gtk_entry_set_visibility (GTK_ENTRY (entry.pwd[1]), FALSE);
	gtk_entry_set_placeholder_text (GTK_ENTRY (entry.pwd[1]), "Retype password");

	button[0] = gtk_button_new_with_label ("OK");
	g_signal_connect (button[0], "clicked", G_CALLBACK (parse_rows), &entry);
	
	button[1] = gtk_button_new_with_label ("Quit");
	g_signal_connect (button[1], "clicked", G_CALLBACK (clean_and_quit), NULL);
	
	button[2] = gtk_button_new_with_label ("Select DB Folder");
	g_signal_connect (button[2], "clicked", G_CALLBACK (select_folder_dialog), &entry);

	grid = gtk_grid_new();
	gtk_container_add (GTK_CONTAINER (mainwin), grid);
	gtk_grid_set_column_spacing (GTK_GRID (grid), 20);
	gtk_grid_set_row_spacing (GTK_GRID (grid), 10);

	for (i = 0, j = 0, k = 0; i < 8; i++)
	{
		gtk_grid_attach (GTK_GRID (grid), row_label[i], 0+j, 0+k, 1, 1);
		gtk_grid_attach (GTK_GRID (grid), entry.row[i], 1+j, 0+k, 1, 1);
		j += 2;
		if (j == 8)
		{
			j = 0;
			k = 1;
		}
	}
	
	for (i = 8, j = 0, k = 0; i < 16; i++)
	{
		gtk_grid_attach (GTK_GRID (grid), row_label[i], 0+j, 2+k, 1, 1);
		gtk_grid_attach (GTK_GRID (grid), entry.row[i], 1+j, 2+k, 1, 1);
		j += 2;
		if (j == 8)
		{
			j = 0;
			k = 1;
		}
	}
	
	for (i = 16, j = 0, k = 0; i < 24; i++)
	{
		gtk_grid_attach (GTK_GRID (grid), row_label[i], 0+j, 4+k, 1, 1);
		gtk_grid_attach (GTK_GRID (grid), entry.row[i], 1+j, 4+k, 1, 1);
		j += 2;
		if (j == 8)
		{
			j = 0;
			k = 1;
		}
	}
	
	for (i = 24, j = 0, k = 0; i < 32; i++)
	{
		gtk_grid_attach (GTK_GRID (grid), row_label[i], 0+j, 6+k, 1, 1);
		gtk_grid_attach (GTK_GRID (grid), entry.row[i], 1+j, 6+k, 1, 1);
		j += 2;
		if (j == 8)
		{
			j = 0;
			k = 1;
		}
	}

	gtk_widget_set_margin_top (entry.pwd[0], 15);
	gtk_widget_set_margin_top (entry.pwd[1], 15);

	gtk_grid_attach (GTK_GRID (grid), entry.pwd[0], 0, 8, 4, 1);
	gtk_grid_attach (GTK_GRID (grid), entry.pwd[1], 4, 8, 4, 1);

	gtk_grid_attach (GTK_GRID (grid), button[2], 0, 9, 8, 1);

	gtk_grid_attach (GTK_GRID (grid), button[0], 7, 10, 1, 1);
	gtk_grid_attach (GTK_GRID (grid), button[1], 5, 10, 1, 1);

	gtk_widget_show_all (mainwin);
	
	gtk_main();
	
	return 0;
}


static void
parse_rows (	GtkWidget __attribute__((__unused__)) *ignored,
		gpointer data)
{
	struct row_entries *entry = data;
	gchar *to_enc_buf;
	gint i, j, offset = 0;
	
	if (entry->db_path == NULL)
	{
		show_error ("Devi selezionare una cartella dove salvare il database");
		return;
	}
	
	to_enc_buf = gcry_malloc_secure (BUFFER_LENGTH + 1); //4 * 32 + 1
		
	for (i = 0; i < ROW_NUMBERS; i++)
	{
		if (gtk_entry_get_text_length (GTK_ENTRY (entry->row[i])) != 4)
		{
			show_error ("Ogni riga deve contenere 4 numeri");
			return;
		}
	}
	
	for (i = 0; i < ROW_NUMBERS; i++)
	{
		memcpy (to_enc_buf + offset, gtk_entry_get_text (GTK_ENTRY(entry->row[i])), 4);
		offset += 4;
	}
	
	for (j = 0; j < ROW_NUMBERS; j++)
	{
		if (!g_ascii_isdigit (to_enc_buf[j]))
		{
			show_error ("Ogni riga può contenere solo numeri");
			return;
		}
	}
	
	to_enc_buf[BUFFER_LENGTH] = '\0';
	encrypt_file (to_enc_buf, entry);
	gcry_free (to_enc_buf);
}


static void
encrypt_file (	gpointer to_enc_buffer,
		struct row_entries *entry)
{
	struct data metadata;
	gint algo;
	guchar *derived_key = NULL, *crypto_key = NULL, *mac_key = NULL, *enc_buffer = NULL;
	const char *name = "aes256";
	gsize blk_length, key_length, pwd_length;
	
	const gchar *input_key = gtk_entry_get_text (GTK_ENTRY (entry->pwd[0]));
	pwd_length = strlen (input_key) + 1;

	blk_length = gcry_cipher_get_algo_blklen (GCRY_CIPHER_AES256);
	key_length = gcry_cipher_get_algo_keylen (GCRY_CIPHER_AES256);
	algo = gcry_cipher_map_name (name);
	
	enc_buffer = gcry_malloc_secure (BUFFER_LENGTH);

	gcry_create_nonce (metadata.iv, 16);
	gcry_create_nonce (metadata.salt, 32);

	gchar *user = getenv ("USER");
	if (user == NULL)
		exit (EXIT_FAILURE);
	
	gchar *config_path = g_malloc (25 + strlen (user) + 1); // 25 = /home//.config/wegui.conf
	strncpy (config_path, "/home/", 6);
	strncat (config_path, user, strlen (user));
	strncat (config_path, "/.config/wegui.conf", 19);
	
	FILE *config = g_fopen (config_path, "w");
	FILE *fpout = g_fopen (entry->db_path, "w");
	if (fpout == NULL || config == NULL)
	{
		g_printerr ("%s\n", g_strerror (errno));
		exit (EXIT_FAILURE);
	}
	
	fwrite (entry->db_path, 1, strlen (entry->db_path), config);
	fclose (config);

	gcry_cipher_hd_t hd;
	gcry_cipher_open (&hd, algo, GCRY_CIPHER_MODE_CTR, 0);
	
	if (((derived_key = gcry_malloc_secure (64)) == NULL) || ((crypto_key = gcry_malloc_secure (32)) == NULL) || ((mac_key = gcry_malloc_secure (32)) == NULL))
	{
		g_printerr ("memory allocation error\n");
		exit (EXIT_FAILURE);
	}

	if (gcry_kdf_derive (input_key, pwd_length, GCRY_KDF_PBKDF2, GCRY_MD_SHA512, metadata.salt, 32, 150000, 64, derived_key) != 0)
	{
		g_printerr ("key derivation error\n");
		gcry_free(derived_key);
		gcry_free(crypto_key);
		gcry_free(mac_key);
		exit (EXIT_FAILURE);
	}
	memcpy (crypto_key, derived_key, 32);
	memcpy (mac_key, derived_key+32, 32);

	gcry_cipher_setkey (hd, crypto_key, key_length);
	gcry_cipher_setctr (hd, metadata.iv, blk_length);

	fseek(fpout, 0, SEEK_SET);
	fwrite(&metadata, sizeof (struct data), 1, fpout);
	
	gcry_cipher_encrypt (hd, enc_buffer, BUFFER_LENGTH, to_enc_buffer, BUFFER_LENGTH);
	fwrite (enc_buffer, 1, BUFFER_LENGTH, fpout);
	
	gcry_cipher_close (hd);
	
	fclose (fpout);
	
	guchar *hmac = calculate_hmac (mac_key, key_length, 0, entry->db_path);
	
	fpout = fopen (entry->db_path, "a");
	fwrite (hmac, 1, 64, fpout);
	fclose (fpout);
	
	g_free (hmac);
	gcry_free (mac_key);
	gcry_free (derived_key);
	gcry_free (crypto_key);
	gcry_free (enc_buffer);
	
	g_free (config_path);
	g_free (entry->db_path);
}


static void
show_error (const gchar *message)
{
	GtkWidget *dialog;
	dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "%s\n", message);
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}


static void
clean_and_quit (GtkWidget __attribute__((__unused__)) *ignored,
		gpointer __attribute__((__unused__)) data)
{
	gcry_control (GCRYCTL_TERM_SECMEM);
	gtk_main_quit ();
}


static void
select_folder_dialog (	GtkWidget __attribute__((__unused__)) *ignored,
			gpointer data)
{
	struct row_entries *entry = data;
	GtkWidget *dialog;
	gsize len;
	dialog = gtk_file_chooser_dialog_new ("Select Folder", NULL, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, "_OK", GTK_RESPONSE_ACCEPT, NULL);
	gint ret = gtk_dialog_run (GTK_DIALOG (dialog));
	switch (ret)
	{
		case GTK_RESPONSE_ACCEPT:
			len = strlen (gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog)));
			entry->db_path = g_malloc (len + 14 + 1);
			strncpy (entry->db_path, gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog)), len);
			entry->db_path[len] = '\0';
			strncat (entry->db_path, "/db_wb_crypted", 14);
			break;
		default:
			break;
	}
	gtk_widget_destroy (dialog);
}
