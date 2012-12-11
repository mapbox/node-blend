import os
from shutil import copy2 as copy

import Options
import Utils

TARGET = 'blend'
TARGET_FILE = '%s.node' % TARGET
built = 'build/Release/%s' % TARGET_FILE
builtV4 = 'build/default/%s' % TARGET_FILE
dest = 'lib/%s' % TARGET_FILE


test_prog = '''
#include "stdio.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "%s"
#ifdef __cplusplus
}
#endif

int main() {
    return 0;
}
'''


jpeg_inc_name = 'jpeglib.h'
jpeg_search_paths = ['/usr', '/usr/local']
jpeg_min_version = 8
jpeg_test_version = '''
#include "stdio.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "%s"
#ifdef __cplusplus
}
#endif

int main() {
#if JPEG_LIB_VERSION < 80
    #error "JPEG_LIB_VERSION must be greater than or equal to 80"
#else
    return 0;
#endif
}
'''


png_inc_name = 'png.h'
png_search_paths = ['/usr', '/usr/local']
if Options.platform == 'darwin':
    # X11 has png headers (not jpeg)
    png_search_paths.insert(0, '/usr/X11')


def set_options(opt):
    opt.tool_options("compiler_cxx")
    opt.tool_options('misc')
    opt.add_option('--with-jpeg',
        action='store',
        default=None,
        help='Directory prefix containing jpeg "lib" and "include" files',
        dest='jpeg_dir'
    )

    opt.add_option('--with-png',
        action='store',
        default=None,
        help='Directory prefix containing png "lib" and "include" files',
        dest='png_dir'
    )

    opt.add_option('--without-densehashmap',
        action='store_true',
        default=None,
        help='Do not build with densehashmap support - you can also do `export NODE_BLEND_NO_DENSE=1` to disable',
        dest='wo_densehash'
    )

def _conf_exit(conf, msg):
    conf.fatal('\n\n' + msg + '\n...check the build/config.log for details')

def _check_jpeg(conf, path):
    norm_path = os.path.normpath(os.path.realpath(path))
    lib = os.path.join(norm_path, 'lib')
    inc = os.path.join(norm_path, 'include')
    header = os.path.join(inc, jpeg_inc_name)
    if conf.check(
            lib='jpeg',
            fragment=test_prog % header,
            uselib_store='JPEG',
            libpath=lib,
            includes=inc,
            msg='Checking for libjpeg at %s' % norm_path):
        # confirm adequate version
        header = os.path.join(inc, jpeg_inc_name)
        if conf.check(
                lib='jpeg',
                uselib_store='JPEG',
                fragment=jpeg_test_version % header,
                libpath=lib,
                includes=inc,
                msg='Checking for libjpeg version >= %s' % jpeg_min_version
            ):
            return True
        else:
            _conf_exit(conf, 'jpeg version >= 8 not found (upgrade to http://www.ijg.org/files/jpegsrc.v8d.tar.gz)')

    return False

def _check_png(conf, path):
    norm_path = os.path.normpath(os.path.realpath(path))
    lib = os.path.join(norm_path, 'lib')
    inc = os.path.join(norm_path, 'include')
    header = os.path.join(inc, png_inc_name)
    if conf.check(
            lib='png',
            fragment=test_prog % header,
            uselib_store='PNG',
            libpath=lib,
            includes=inc,
            msg='Checking for libpng at %s' % norm_path):
        return True

    return False

def configure(conf):
    conf.check_tool("compiler_cxx")
    conf.check_tool("node_addon")

    o = Options.options

    if os.environ.has_key('NODE_BLEND_NO_DENSE'):
        key = os.environ.get('NODE_BLEND_NO_DENSE')
        try:
            if bool(int(key)):
                o.wo_densehash = True
        except: pass

    # jpeg checks
    found_jpeg = False
    if o.jpeg_dir:
        # manual configuration
        found_jpeg = _check_jpeg(conf, o.jpeg_dir)
    else:
        # automatic configuration
        for path in jpeg_search_paths:
            found_jpeg = _check_jpeg(conf, path)
            if found_jpeg:
                break

    if not found_jpeg:
        _conf_exit(conf, 'jpeg not found: searched %s \nuse --with-jpeg to point to the location of your jpeg libs and headers' % jpeg_search_paths)


    # png checks
    found_png = False
    if o.png_dir:
        # manual configuration
        found_png = _check_png(conf, o.png_dir)
    else:
        # automatic configuration
        for path in png_search_paths:
            found_png = _check_png(conf, path)
            if found_png:
                break

    if not found_png:
        _conf_exit(conf, 'png not found: searched %s \nuse --with-png to point to the location of your png libs and headers' % png_search_paths)

    if not o.wo_densehash:
        conf.env.append_value("CXXFLAGS",["-I../deps","-DUSE_DENSE_HASH_MAP"])
        conf.env.WO_DENSEHASH = o.wo_densehash


def build(bld):
    obj = bld.new_task_gen("cxx", "shlib", "node_addon")
    obj.cxxflags = ["-O3", "-DNDEBUG", "-D_FILE_OFFSET_BITS=64", "-D_LARGEFILE_SOURCE", "-Wall", "-Wno-unused-value", "-Wno-unused-function", "-mfpmath=sse", "-march=core2",
        "-funroll-loops", "-fomit-frame-pointer"]
    obj.target = TARGET
    obj.source = ["src/reader.cpp", "src/blend.cpp", "src/palette.cpp"]
    obj.uselib = ["PNG", "JPEG"]

def shutdown():
    if Options.commands['clean']:
        if os.path.exists(TARGET_FILE):
            unlink(TARGET_FILE)
    else:
        if os.path.exists(builtV4):
            copy(builtV4, dest)
        if os.path.exists(built):
            copy(built, dest)

