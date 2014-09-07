#ifndef BCARD_H_INCLUDED
#define BCARD_H_INCLUDED

#define ROW_NUMBERS 32
#define ROW_LENGTH 5
#define BUFFER_LENGTH 128

struct data
{
	guchar salt[32];
	guchar iv[16];
};
extern struct data metadata;


struct info
{
	GtkWidget *row_entry;
	GtkWidget *c1_entry;
	GtkWidget *c2_entry;
	GtkWidget *pwd_entry;
	gchar *input_key;
	gint row_val, c1_val, c2_val, pwd_length;
};
extern struct info vars;


struct row_entries
{
	GtkWidget *row[ROW_NUMBERS];
	GtkWidget *pwd[2];
	gchar *db_path;
};
extern struct row_entries entry;

#endif
