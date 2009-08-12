# Copyright © 2009 Tyler Hayes
# ALL RIGHTS RESERVED
#
# [This program is licensed under the GPL version 3 or later.]
# Please see the file COPYING in the source
# distribution of this software for license terms.
#
# Thanks to Tom Szilagyi (http://tap-plugins.sourceforge.net/)
# for his Makefile as an example for me.  Also special thanks
# to Bart Massey (http://web.cecs.pdx.edu/~bart/) for his help
# in getting the correct flags for creating the shared object.
#
# NOTE: since LADSPA_PATH varies depending on your distro,
# you must change the LADSPA_PATH variable and UNINSTALL
# variable in this Makefile to match your LADSPA_PATH
# environment variable!

#-----------------------------------------------------

CC = gcc
CFLAGS	= -Wall -O3 -fPIC
LDFLAGS = -nostartfiles -shared -Wl,-Bsymbolic
LADSPA_PATH = /usr/lib/ladspa      # change these 2 variables to match
UNINSTALL = /usr/lib/ladspa/sb_*   # your LADSPA_PATH environment
                                   # variable (type 'echo $LADSPA_PATH
                                   # at your shell prompt)
PLUGINS	=	sb_ringer.so

# ----------------------------------------------------

all: $(PLUGINS)

sb_ringer.o: sb_ringer.c ../ladspa.h
	$(CC) $(CFLAGS) -c sb_ringer.c

sb_ringer.so: sb_ringer.o
	$(CC) $(LDFLAGS) -o sb_ringer.so sb_ringer.o

install: sb_ringer.so
	cp sb_ringer.so $(LADSPA_PATH)

uninstall:
	rm -f $(UNINSTALL)

clean:
	rm -f *.o *.so *~
