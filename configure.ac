AC_INIT(cockroach, 0.1.0)
AM_INIT_AUTOMAKE()
AC_CONFIG_MACRO_DIR([m4])
AC_DISABLE_STATIC
AC_PROG_LIBTOOL
AC_PROG_CXX
AM_PROG_AS
AM_PROG_CC_C_O

dnl cutter
CUTTER_REQUIRED=1.2.2
have_cutter=no

m4_ifdef([AC_CHECK_GCUTTER], [
AC_CHECK_GCUTTER(>= $CUTTER_REQUIRED)
],
[ac_cv_use_cutter="no"])

if test "$ac_cv_use_cutter" != "no"; then
  m4_ifdef([AC_CHECK_CPPCUTTER], [
  AC_CHECK_CPPCUTTER(>= $CUTTER_REQUIRED)
  ],
  [ac_cv_use_cutter="no"])
fi

AM_CONDITIONAL([WITH_CUTTER], [test "$ac_cv_use_cutter" != "no"])
if test "$ac_cv_use_cutter" != "no"; then
  AC_DEFINE(WITH_CUTTER, 1, [Define to 1 if you use Cutter])
  have_cutter=yes
fi

if test "$have_cutter" = "yes"; then
  dnl glib (for test)
  GLIB_REQUIRED=2.22
  PKG_CHECK_MODULES(GLIB, glib-2.0 >= $GLIB_REQUIRED)

  dnl boost (for test)
  AC_LANG_PUSH([C++])
  AC_CHECK_HEADERS([boost/foreach.hpp], [], [AC_MSG_ERROR(Not found Boost libraries.)])
  AC_LANG_POP([C++])
fi

AC_MSG_CHECKING([for supported architecture])
case "$host_cpu" in
i[[3456]]86|pentium)
  arch=i386
  AC_DEFINE([I386], 1, [Define for the i386 architecture.])
  ;;
x86?64*)
  arch=x86_64
  AC_DEFINE([X86_64], 1, [Define for the AMD x86-64 architecture.])
  ;;
*)
  AC_MSG_RESULT([NO!])
  AC_MSG_ERROR([architecture $host_cpu is not supported by strace])
  ;;
esac
AC_MSG_RESULT($arch)
AC_SUBST(arch)

AM_CONDITIONAL([I386], [test x$arch = xi386])
AM_CONDITIONAL([X86_64], [test x$arch = xx86_64])

AC_OUTPUT([
Makefile
src/Makefile
utils/Makefile
test/Makefile
test/fixtures/Makefile
])

dnl **************************************************************
dnl Show configure result
dnl **************************************************************
echo
echo "Configure Result:"
echo
echo "  C Unit test (cutter) : ${have_cutter}"
echo
