#include <glib.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <string.h>
#include <gcrypt.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "bcard.h"


void select_cifra (struct info *, gchar *);
static void message_and_cleanup (const gchar *, gpointer, gpointer, gpointer);
guchar *calculate_hmac (const guchar *, gsize, gint, gchar *);


gint
decrypt_db (struct info *vars)
{
	gint algo, fd;	
	struct data metadata;
	struct stat file_stat;
	memset (&metadata, 0, sizeof (struct data));
	guchar *derived_key = NULL, *crypto_key = NULL, *mac_key = NULL;
	gchar *dec_buf = NULL, *enc_file_path, tmp[512];
	guchar cipher_text[16], mac_of_file[64];
	goffset file_size = 0, done_size = 0, bytes_before_mac = 0, dec_offset = 0;
	const gchar *name = "aes256";
	gsize blk_length, key_length, txt_length = 16, retval = 0;
	glong current_file_offset;
	FILE *fp, *config;
	GError *err = NULL;

	blk_length = gcry_cipher_get_algo_blklen (GCRY_CIPHER_AES256);
	key_length = gcry_cipher_get_algo_keylen (GCRY_CIPHER_AES256);
	algo = gcry_cipher_map_name (name);
	
	gchar *user = getenv ("USER");
	if (user == NULL)
		exit (EXIT_FAILURE);
	
	gchar *config_path = g_malloc (25 + strlen (user) + 1); // 25 = /home//.config/wegui.conf
	strncpy (config_path, "/home/", 6);
	strncat (config_path, user, strlen (user));
	strncat (config_path, "/.config/wegui.conf", 19);

	config = g_fopen (config_path, "r");
	if (config == NULL)
	{
		g_printerr ("%s\n", g_strerror (errno));
		return -1;
	}
	
	fgets (tmp, 512, config);
	enc_file_path = g_malloc (strlen (tmp) + 1);
	strncpy (enc_file_path, tmp, strlen (tmp));
	enc_file_path[strlen (tmp)] = '\0';
	fclose (config);
	
	fd = g_open (enc_file_path, O_RDONLY | O_NOFOLLOW);
	if (fd == -1)
	{
		g_printerr ("%s\n", g_strerror (errno));
		return -1;
	}
  	
  	if (fstat (fd, &file_stat) < 0)
  	{
		g_printerr ("%s\n", g_strerror (errno));
		g_close (fd, &err);
		return -1;
  	}
  	file_size = file_stat.st_size;
  	g_close(fd, &err);
  	
  	bytes_before_mac = file_size - 64;
  	file_size = file_size - 64 - sizeof (struct data);

  	dec_buf = gcry_malloc_secure (file_size + 1);

	fp = g_fopen (enc_file_path, "r");
  	if (fp == NULL)
  	{
		g_printerr ("%s\n", g_strerror (errno));
		return -1;
  	}
	fseek (fp, 0, SEEK_SET);

	retval = fread (&metadata, sizeof(struct data), 1, fp);
	if (retval != 1)
	{
		g_printerr ("fread error\n");
		return -1;
	}

	gcry_cipher_hd_t hd;
	gcry_cipher_open (&hd, algo, GCRY_CIPHER_MODE_CTR, 0);
	if (((derived_key = gcry_malloc_secure (64)) == NULL) || ((crypto_key = gcry_malloc_secure (32)) == NULL) || ((mac_key = gcry_malloc_secure (32)) == NULL))
	{
		g_printerr ("gcry_malloc_error\n");
		return -1;
	}

	if (gcry_kdf_derive (vars->input_key, vars->pwd_length, GCRY_KDF_PBKDF2, GCRY_MD_SHA512, metadata.salt, 32, 150000, 64, derived_key) != 0)
	{
		g_printerr ("Key derivation error\n");
		gcry_free (derived_key);
		gcry_free (crypto_key);
		gcry_free (mac_key);
		return -1;
	}
	memcpy (crypto_key, derived_key, 32);
	memcpy (mac_key, derived_key + 32, 32);
	gcry_cipher_setkey (hd, crypto_key, key_length);
	gcry_cipher_setctr (hd, metadata.iv, blk_length);

	if ((current_file_offset = ftell (fp)) == -1)
	{
		message_and_cleanup ("ftell error", derived_key, crypto_key, mac_key);
		return -1;		
	}
	if (fseek (fp, bytes_before_mac, SEEK_SET) == -1)
	{
		message_and_cleanup ("fseek error", derived_key, crypto_key, mac_key);
		return -1;		
	}
	if (fread (mac_of_file, 1, 64, fp) != 64)
	{
		message_and_cleanup ("fread MAC error", derived_key, crypto_key, mac_key);
		return -1;
	}
	
	guchar *hmac = calculate_hmac (mac_key, key_length, 1, enc_file_path);
	if (hmac == NULL)
	{
		message_and_cleanup ("Error during HMAC calculation", derived_key, crypto_key, mac_key);
		return -1;
	}
	
	if (memcmp (mac_of_file, hmac, 64) != 0)
	{
		message_and_cleanup ("CRITICAL ERROR: hmac doesn't match. This is caused by\n1) wrong password\nor\n2) corrupted file\n", derived_key, crypto_key, mac_key);
		return -1;
	}
	g_free (hmac);
	
	if (fseek (fp, current_file_offset, SEEK_SET) == -1)
	{
		message_and_cleanup ("fseek error", derived_key, crypto_key, mac_key);
		return -1;		
	}

	while (file_size > done_size)
	{
		memset (cipher_text, 0, sizeof (cipher_text));
		retval = fread (cipher_text, 1, txt_length, fp);
		gcry_cipher_decrypt (hd, dec_buf + dec_offset, txt_length, cipher_text, retval);
		done_size += retval;
		dec_offset += retval;
	}

	gcry_cipher_close(hd);
	
	select_cifra (vars, dec_buf);
	
	message_and_cleanup (NULL, derived_key, crypto_key, mac_key);
	gcry_free (dec_buf);
	g_free (enc_file_path);
	fclose (fp);

	return 0;
}


static void
message_and_cleanup (	const gchar *message,
			gpointer key1,
			gpointer key2,
			gpointer key3)
{
	if (message != NULL)
	{
		GtkWidget *dialog;
		dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "%s\n", message);
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
	}
	
	gcry_free (key1);
	gcry_free (key2);
	gcry_free (key3);
}
