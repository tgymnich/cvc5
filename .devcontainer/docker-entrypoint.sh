#!/bin/bash

cd /workspaces/cvc5

bash ./autogen.sh

bash ./contrib/get-antlr-3.4

bash ./configure --with-antlr-dir=/workspaces/cvc5/antlr-3.4 ANTLR=/workspaces/cvc5/antlr-3.4/bin/antlr3

make -j `nproc`