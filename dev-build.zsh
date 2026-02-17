#!/bin/zsh
make distclean 2>/dev/null
./configure --without-all --without-x --without-ns --with-gnutls
make
