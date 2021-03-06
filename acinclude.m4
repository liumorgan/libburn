AC_DEFUN([LIBBURNIA_SET_FLAGS],
[
case $target_os in
freebsd* | netbsd*)
        LDFLAGS="$LDFLAGS -L/usr/local/lib"
        CPPFLAGS="$CPPFLAGS -I/usr/local/include"
        ;;
esac
])


AC_DEFUN([TARGET_SHIZZLE],
[
  ARCH=""
  LIBBURNIA_PKGCONFDIR="$libdir"/pkgconfig

  AC_MSG_CHECKING([target operating system])

  LIBBURNIA_LDCONFIG_CMD="echo 'No ldconfig run performed. If needed, configure manually for:'"
  
  case $target in
    *-*-linux*)
      ARCH=linux
      LIBBURN_ARCH_LIBS=
      LIBBURNIA_LDCONFIG_CMD=ldconfig
      ;;
    *-*-freebsd*)
      ARCH=freebsd
      LIBBURN_ARCH_LIBS=-lcam
      LIBBURNIA_PKGCONFDIR=$(echo "$libdir" | sed 's/\/lib$/\/libdata/')/pkgconfig
      ;;
    *-kfreebsd*-gnu*)
      ARCH=freebsd
      LIBBURN_ARCH_LIBS=-lcam
      ;;
    *-solaris*)
      ARCH=solaris
      LIBBURN_ARCH_LIBS=-lvolmgt
      ;;
    *)
      ARCH=
      LIBBURN_ARCH_LIBS=
#      AC_ERROR([You are attempting to compile for an unsupported platform])
      ;;
  esac

  AC_MSG_RESULT([$ARCH])
])


dnl LIBBURN_ASSERT_VERS_LIBS is by Thomas Schmitt, libburnia project
dnl It tests whether -Wl,--version-script=... works with the compiler
AC_DEFUN([LIBBURN_ASSERT_VERS_LIBS],
[
    libburnia_save_LDFLAGS="$LDFLAGS"
    LDFLAGS="$LDFLAGS -Wl,--version-script=libburn/libburn.ver"
    AC_TRY_LINK([#include <stdio.h>], [printf("Hello\n");],
                [vers_libs_test="yes"], [vers_libs_test="no"])
    if test x$vers_libs_test = xyes
    then
        LIBLDFLAGS="-Wl,--version-script=libburn/libburn.ver"
    fi
    LDFLAGS="$libburnia_save_LDFLAGS"
    AC_SUBST(LIBLDFLAGS)
])
 

dnl LIBBURNIA_SET_PKGCONFIG determines the install directory for the *.pc file.
dnl Important: Must be performed _after_ TARGET_SHIZZLE
dnl 
AC_DEFUN([LIBBURNIA_SET_PKGCONFIG],
[
### for testing --enable-libdir-pkgconfig on Linux
### LIBBURNIA_PKGCONFDIR="$libdir"data/pkgconfig

if test "x$LIBBURNIA_PKGCONFDIR" = "x$libdir"/pkgconfig
then
  dummy=dummy
else
  AC_ARG_ENABLE(libdir-pkgconfig,
  [  --enable-libdir-pkgconfig  Install to $libdir/pkgconfig on any OS, default=no],
   , enable_libdir_pkgconfig="no")
  AC_MSG_CHECKING([for --enable-libdir-pkgconfig])
  if test "x$enable_libdir_pkgconfig" = xyes
  then
    LIBBURNIA_PKGCONFDIR="$libdir"/pkgconfig
  fi
  AC_MSG_RESULT([$enable_libdir_pkgconfig])
fi

libburnia_pkgconfig_override="no"
AC_ARG_ENABLE(pkgconfig-path,
[  --enable-pkgconfig-path=DIR  Absolute path of directory for libisofs-*.pc],
libburnia_pkgconfig_override="yes" , enable_pkgconfig_path="none")
AC_MSG_CHECKING([for overridden pkgconfig directory path])
if test "x$enable_pkgconfig_path" = xno
then
  libburnia_pkgconfig_override="no"
fi
if test "x$enable_pkgconfig_path" = x -o "x$enable_pkgconfig_path" = xyes
then
  libburnia_pkgconfig_override="invalid argument"
fi
if test "x$libburnia_pkgconfig_override" = xyes
then
  LIBBURNIA_PKGCONFDIR="$enable_pkgconfig_path"
  AC_MSG_RESULT([$LIBBURNIA_PKGCONFDIR])
else
  AC_MSG_RESULT([$libburnia_pkgconfig_override])
fi
AC_SUBST(LIBBURNIA_PKGCONFDIR)

dnl For debugging only
### AC_MSG_RESULT([LIBBURNIA_PKGCONFDIR = $LIBBURNIA_PKGCONFDIR])

])

dnl LIBBURNIA_CHECK_ARCH_LIBS is by Thomas Schmitt, libburnia project
dnl It tests whether the OS dependent libraries are available.
dnl With libisoburn they are needed only for the case that indirect linking
dnl does not work. So it is worth a try to omit them.
dnl $1 = "mandatory" or "optional" define the action if test linking fails.
AC_DEFUN([LIBBURNIA_CHECK_ARCH_LIBS],
[
    libburnia_save_LIBS="$LIBS"
    if test "x$LIBBURN_ARCH_LIBS" = x
    then
      dummy=dummy
    else
      LIBS="$LIBS $LIBBURN_ARCH_LIBS"
      AC_TRY_LINK([#include <stdio.h>], [printf("Hello\n");],
                  [archlibs_test="yes"], [archlibs_test="no"])
      LIBS="$libburnia_save_LIBS"
      if test x$archlibs_test = xno
      then
        if test x"$1" = xmandatory
        then
          echo >&2
          echo "FATAL: Test linking with mandatory library options failed: $LIBBURN_ARCH_LIBS" >&2
          echo >&2
          (exit 1); exit 1;
        else
          echo "disabled linking with $LIBBURN_ARCH_LIBS (because not found)"
          LIBBURN_ARCH_LIBS=""
        fi
      else
        echo "enabled  linking with $LIBBURN_ARCH_LIBS"
      fi
    fi
])


dnl LIBBURNIA_CHECK_LINUX_SCSI is by Thomas Schmitt, libburnia project
dnl
AC_DEFUN([LIBBURNIA_CHECK_LINUX_SCSI],
[
  dnl Check whether it is a Linux without scsi/scsi.h
  AH_TEMPLATE([Libburn_use_sg_dummY],
              [Define to compile without OS specific SCSI features])
  AC_MSG_CHECKING([for missing scsi/scsi.h on Linux])
  AC_TRY_COMPILE([
#ifdef __linux
#include <scsi/scsi.h>
#endif
     ],
     [;],
     [AC_MSG_RESULT([no])],
     [AC_DEFINE([Libburn_use_sg_dummY], [yes])
      AC_MSG_RESULT([yes])]
  )
])

