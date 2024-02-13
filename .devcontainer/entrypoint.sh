#!/bin/bash

cd /app/cvc5

/app/cvc5/autogen.sh

./contrib/get-antlr-3.4

./configure --with-antlr-dir=/app/cvc5/antlr-3.4 ANTLR=/app/cvc5/antlr-3.4/bin/antlr3

make -j `nproc`