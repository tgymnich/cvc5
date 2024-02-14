#!/bin/bash

cd /workspaces/cvc5

bash ./autogen.sh

bash ./configure --with-antlr-dir=/app/antlr/antlr-3.4 ANTLR=/app/antlr/antlr-3.4/bin/antlr3

# make -j `nproc`