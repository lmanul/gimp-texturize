## Process this file with automake to produce Makefile.in

bin_PROGRAMS = texturize

bindir = $(GIMP_LIBDIR)/plug-ins

texturize_SOURCES = \
	plugin-intl.h	\
	texturize.h	\
	interface.c 	\
	interface.h	\
	main.c		\
	main.h		\
	render.h	\
	block.h		\
	graph.h		\
	graph.cpp	\
	maxflow.cpp	\
	render.c	\
	initialiser.c	\
	compter.c	\
	offset.c	\
	graphcut.cpp

AM_CPPFLAGS = \
	-DLOCALEDIR=\""$(LOCALEDIR)"\"		\
	-DDATADIR=\""$(DATADIR)"\"

INCLUDES =\
	-I$(top_srcdir)		\
	@GIMP_CFLAGS@		\
	-I$(includedir)

LDADD = $(GIMP_LIBS)

