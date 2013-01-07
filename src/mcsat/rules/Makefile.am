AM_CPPFLAGS = \
	-D__BUILDING_CVC4LIB \
	-D __STDC_LIMIT_MACROS \
	-D __STDC_FORMAT_MACROS \
	-I@srcdir@/../../include -I@srcdir@/../.. -I@builddir@/../.. 
AM_CXXFLAGS = -Wall -Wno-parentheses -Wno-unknown-pragmas $(FLAG_VISIBILITY_HIDDEN)

noinst_LTLIBRARIES = libmcsat_rules.la

libmcsat_rules_la_SOURCES = \
	proof_rule.h \
	proof_rule.cpp 
	
EXTRA_DIST = 

SUBDIRS = 

