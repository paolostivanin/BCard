#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <gcrypt.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "bcard.h"

//mode = 0 encrypt, mode = 1 decrypt
guchar *
calculate_hmac (const guchar *key,
		gsize keylen,
		gint mode,
		gchar *enc_file)
{
	gint fd;
	struct stat fd_stat;
	gchar *buffer;
	gsize file_size = 0, done_size = 0, diff = 0, read_bytes = 0;
	GError *err = NULL;
	
	fd = g_open (enc_file, O_RDONLY | O_NOFOLLOW);
	if (fd == -1)
	{
		g_printerr ("%s\n", g_strerror (errno));
		return NULL;
	}

  	if (fstat (fd, &fd_stat) < 0)
  	{
  		g_printerr ("%s\n", g_strerror (errno));
		return NULL;
  	}
  	
  	file_size = fd_stat.st_size;
  	if (mode == 1)
		file_size -= 64;
		
  	g_close (fd, &err); 
  	 	
	FILE *fp;
	fp = g_fopen (enc_file, "r");
	
	gcry_md_hd_t hd;
	gcry_md_open (&hd, GCRY_MD_SHA512, GCRY_MD_FLAG_HMAC);
	gcry_md_setkey (hd, key, keylen);
	
	if (file_size < 16)
	{
		buffer = g_malloc (file_size);
  		if (buffer == NULL)
  		{
  			g_printerr ("hmac: malloc error\n");
  			return NULL;
  		}
		if (fread (buffer, 1, file_size, fp) != file_size)
		{
			g_printerr ("hmac: malloc error\n");
			return NULL;
		}
		gcry_md_write (hd, buffer, file_size);
	}
	else
	{
		buffer = malloc (16);
		if (buffer == NULL)
		{
			g_printerr ("hmac: malloc error\n");
			return NULL;
		}
		while (file_size > done_size)
		{
			read_bytes = fread (buffer, 1, 16, fp);
			gcry_md_write (hd, buffer, read_bytes);
			done_size += read_bytes;
			diff = file_size - done_size;
			if (diff > 0 && diff < 16)
			{
				if (fread (buffer, 1, diff, fp) != diff)
				{
					g_printerr ("hmac: fread error\n");
					return NULL;
				}
				gcry_md_write (hd, buffer, diff);
				break;
			}
		}
	}

	gcry_md_final (hd);
	g_free (buffer);
	fclose (fp);
	
	guchar *tmp_hmac = gcry_md_read (hd, GCRY_MD_SHA512);
	guchar *hmac = g_malloc (64);
	if (hmac == NULL)
	{
		g_printerr ("%s\n", g_strerror (errno));
		return NULL;
	}
 	memcpy (hmac, tmp_hmac, 64);
	gcry_md_close (hd);
	
	return hmac;
}
