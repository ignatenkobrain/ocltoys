AC_INIT([OCLToys], [1.1],
        [David Bucciarelli <dade916@gmail.com>],
        [ocltoys], [http://code.google.com/p/ocltoys/])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([-Wall])
m4_pattern_allow([AM_PROG_AR])
AM_PROG_AR
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AX_BOOST_BASE([1.44.0], [], [You must have boost > 1.44.0])
AX_BOOST_SYSTEM
AX_BOOST_FILESYSTEM
AX_BOOST_PROGRAM_OPTIONS
AX_BOOST_REGEX
AX_BOOST_THREAD
AX_CHECK_GL
AX_CHECK_GLU
AX_CHECK_GLUT
AC_CHECK_LIB([OpenCL], [clGetPlatformIDs])
CFLAGS="-Wall"
CPPFLAGS="-Wall -msse -msse2"
package_datadir="${prefix}/share/ocltoys/"
AC_DEFINE_UNQUOTED([PACKAGE_DATADIR], ["$package_datadir"], [Configuration machine file.])
AC_SUBST(package_datadir)
AC_CONFIG_HEADERS(common/config.h)
AC_CONFIG_FILES([
                Makefile
                common/Makefile
                jugCLer/Makefile
                juliagpu/Makefile
                mandelgpu/Makefile
                smallptgpu/Makefile
                ])
AC_OUTPUT
