#!/bin/sh
autoheader
aclocal -I m4
libtoolize --copy
automake --add-missing
autoconf
