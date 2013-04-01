AM_CPPFLAGS = \
	-D__BUILDING_CVC4LIB \
	-D __STDC_LIMIT_MACROS \
	-D __STDC_FORMAT_MACROS \
	-I@srcdir@/../../include -I@srcdir@/../.. -I@builddir@/../.. 
AM_CXXFLAGS = -Wall -Wno-parentheses -Wno-unknown-pragmas $(FLAG_VISIBILITY_HIDDEN)

noinst_LTLIBRARIES = libmcsat_fm.la

libmcsat_fm_la_SOURCES = \
	fm_plugin.h \
	fm_plugin.cpp \
	fm_plugin_types.h \
	fm_plugin_types.cpp \
	linear_constraint.h \
	linear_constraint.cpp \
	assigned_watch_manager.h 
	
EXTRA_DIST = 

SUBDIRS = 

