AM_CPPFLAGS = \
	-D__BUILDING_CVC4LIB \
	-D __STDC_LIMIT_MACROS \
	-D __STDC_FORMAT_MACROS \
	-I@srcdir@/../include -I@srcdir@/.. -I@builddir@/.. 
AM_CXXFLAGS = -Wall -Wno-parentheses -Wno-unknown-pragmas $(FLAG_VISIBILITY_HIDDEN)

noinst_LTLIBRARIES = libmcsat.la

libmcsat_la_SOURCES = \
	solver_trail.h \
	solver_trail.cpp \
	solver.h \
	solver.cpp \
	inner_class.h \
	watch_list_manager.h \
	watch_list_manager.cpp \
	bcp_engine.h \
	bcp_engine.cpp 

libmcsat_noinst_la_LIBADD = \
	@builddir@/clause/libmcsat_clause.la \
	@builddir@/rules/libmcsat_rules.la \
	@builddir@/plugin/libmcsat_solver_plugin.la \
	@builddir@/variable/libmcsat_variable.la \
	@builddir@/cnf/libmcsat_cnf.la
libmcsat_la_LIBADD = \
	@builddir@/clause/libmcsat_clause.la \
	@builddir@/rules/libmcsat_rules.la \
	@builddir@/plugin/libmcsat_solver_plugin.la \
	@builddir@/variable/libmcsat_variable.la \
	@builddir@/cnf/libmcsat_cnf.la
	
EXTRA_DIST = 

SUBDIRS = variable clause rules cnf plugin . main 

