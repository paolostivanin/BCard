CC = clang

CFLAGS = -Wall -Wextra -O2 -Wformat=2 -fstack-protector-all -fPIE -Wstrict-prototypes -Wunreachable-code  -Wwrite-strings -Wpointer-arith -Wbad-function-cast -Wcast-align -Wcast-qual $(shell pkg-config --cflags gtk+-3.0)
DFLAGS = -D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2
NOFLAGS = -Wno-unused-result

LDFLAGS = -Wl,-z,now -Wl,-z,relro

LIBS = -lgcrypt $(shell pkg-config --libs gtk+-3.0)

SOURCES = src/main.c src/decrypt_db.c src/select_cifra.c src/calculate_hmac.c
OBJS = ${SOURCES:.c=.o}

SOURCES2 = src/main-gen.c src/calculate_hmac.c
OBJS2 = ${SOURCES2:.c=.o}

PROG = bcard
PROG2 = gen_file

.SUFFIXES:.c .o

.c.o:
	$(CC) -c $(CFLAGS) $(NOFLAGS) $(DFLAGS) $< -o $@

all: $(PROG) $(PROG2)


$(PROG) : $(OBJS)
	$(CC) $(CFLAGS) $(NOFLAGS) $(DFLAGS) $(OBJS) -o $@ $(LIBS)

$(PROG2) : $(OBJS2)
	$(CC) $(CFLAGS) $(NOFLAGS) $(DFLAGS) $(OBJS2) -o $@ $(LIBS)


.PHONY: clean

clean :
	rm -f $(PROG) $(PROG2) $(OBJS) $(OBJS2)
