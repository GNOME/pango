## Process this file with automake to produce Makefile.in

INCLUDES = 					\
	-DSYSCONFDIR=\"$(sysconfdir)\" 		\
	-DLIBDIR=\"$(libdir)\" 			\
	$(FREETYPE_CFLAGS)			\
	-I$(top_srcdir)

LDADDS = @STRIP_BEGIN@ 		\
	@GLIB_LIBS@		\
	-lm			\
@STRIP_END@

noinst_LTLIBRARIES = libmini-xft.la

minixftgram.c: minixftgram.y
	$(YACC) -d minixftgram.y
	sed 's/yy/MiniXftConfig/g' y.tab.c | sed 's/__REALLY_YY__/yy/' > minixftgram.c && $(RM) y.tab.c
	sed 's/yy/MiniXftConfig/g' y.tab.h | sed 's/__REALLY_YY__/yy/' > minixftgram.h && $(RM) y.tab.h

minixftlex.c: minixftlex.l
	$(LEX) minixftlex.l
	sed 's/yy/MiniXftConfig/g' lex.yy.c | sed 's/__REALLY_YY__/yy/' > minixftlex.c && $(RM) lex.yy.c

libmini_xft_la_SOURCES =    \
	Xemu.h \
	MiniXft.h \
	MiniXftFreetype.h \
	minixftgram.h \
	minixftint.h \
	minixftgram.c \
	minixftlex.c \
	minixftdbg.c \
	minixftfont.c \
	minixftfs.c \
	minixftlist.c \
	minixftname.c \
	minixftcache.c \
	minixftdir.c \
	minixftfreetype.c \
	minixftmatch.c \
	minixftpat.c \
	minixftcfg.c \
	minixftdpy.c \
	minixftinit.c \
	minixftmatrix.c \
	minixftstr.c

EXTRA_DIST = 		\
	minixftgram.y	\
	minixftlex.l	\
	README

