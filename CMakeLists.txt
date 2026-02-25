cmake_minimum_required(VERSION 3.25)
project(ez C)
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
find_package(LibXml2 REQUIRED)
find_package(Curses REQUIRED)
find_package(ZLIB REQUIRED)
add_executable(ez
        src/compat.c
        src/emacs.c
        src/emacs-module.c
        src/regex-emacs.c
        src/alloc.c
        src/atimer.c
        src/bignum.c
        src/buffer.c
        src/ccl.c
        src/coding.c
        src/composite.c
        src/data.c
        src/dispnew.c
        src/editfns.c
        src/eval.c
        src/fileio.c
        src/filelock.c
        src/font.c
        src/fontset.c
        src/fns.c
        src/frame.c
        src/indent.c
        src/intervals.c
        src/itree.c
        src/keyboard.c
        src/macros.c
        src/marker.c
        src/minibuf.c
        src/pdumper.c
        src/process.c
        src/process.c
        src/search.c
        src/u64.c
        src/sha1.c
        src/sha256.c
        src/sha512.c
        src/md5.c
        src/sysdep.c
        src/term.c
        src/terminal.c
        src/textprop.c
        src/thread.c
        src/timespec.c
        src/timespec-sub.c
        src/timespec-add.c
        src/window.c
        src/xml.c
        src/xfaces.c
        src/keymap.c
        src/casetab.c
        src/timefns.c
        src/time_rz.c
        src/insdel.c
        src/undo.c
        src/mini-gmp.c
        src/charset.c
        src/character.c
        src/casefiddle.c
        src/print.c
        src/bidi.c
        src/chartab.c
        src/menu.c
        src/lread.c
        src/sort.c
        src/bytecode.c
        src/callproc.c
        src/cm.c
        src/callint.c
        src/region-cache.c
        src/systhread.c
        src/memrchr.c
        src/syntax.c
        src/xdisp.c
        src/category.c
        src/profiler.c
        src/kqueue.c
        src/floatfns.c
        src/doc.c
        src/decompress.c
        src/dired.c
        src/cmds.c
        src/c-ctype.c
        src/gettime.c
        src/stat-time.c
        src/dtotimespec.c
        src/fingerprint.c
        src/doprnt.c
        src/dynlib.c
        src/careadlinkat.c
        src/scroll.c
        src/comp.c
        src/fsusage.c
        src/sig2str.c
        src/allocator.c
        src/boot-time.c
)
target_link_libraries(ez PRIVATE
        LibXml2::LibXml2
        ${CURSES_LIBRARIES}
        ZLIB::ZLIB
)
target_include_directories(ez
        PRIVATE src
        PRIVATE ${CURSES_INCLUDE_DIRS}
)
target_compile_options(ez PRIVATE -include config.h
        -Wno-deprecated-declarations
        -Wno-tautological-constant-out-of-range-compare
        -Wno-initializer-overrides)
