
# Make for x509AC API
# Daniel Díaz-Sanchez 
# Andrés Marín
# Universidad Carlos III de Madrid


SRCS =  x509ac_utils.c \
	x509ac.c \
        x509ac_ext.c \
	x509ac-supp.c \
	x509ac_log.c \
	x509ac_ext.c \
	x509attr.c \
	x509attr-supp.c
	
	
OBJS =  x509ac_utils.o \
	x509ac.o \
        x509ac_ext.o \
	x509ac-supp.o \
	x509ac_log.o \
	x509ac_ext.o \
	x509attr.o \
	x509attr-supp.o

#OPENSSLDIR = ../openssl-0.9.8a/
OPENSSLDIR = /home/mohd/tp/openSSLAC/openssl-0.9.8a/source/openssl-0.9.8a/
OPENSSLINC = $(OPENSSLDIR)include
#CFLAGS = -I$(OPENSSLINC) -fPIC 
CFLAGS = -fPIC
CC = gcc
LDFLAGS = 
TMP_DIR = ./tmp/

all: lib

lib: $(OBJS)	
	#$(CC) -shared -L$(OPENSSLDIR) -lcrypto -lssl -Wl,-zmuldefs  -Wl,-soname,libx509ac.so.0.1 -o libx509ac.so.0.1 $(OBJS)	
	$(CC) -shared -Wl,-zmuldefs  -Wl,-soname,libx509ac.so.0.1 -o libx509ac.so.0.1 $(OBJS)  -lcrypto -lssl
	ln -s  libx509ac.so.0.1 libx509ac.so
clean:
	rm -f *.o
	rm -f *.c~
	rm -f *.h~
	rm -f libx509ac.so 
	rm -f libx509ac.so.0.1 


