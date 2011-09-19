import os
from shutil import copy2 as copy

import Options
import Utils

TARGET = 'blend'
TARGET_FILE = '%s.node' % TARGET

jpeg_inc_name = 'jpeglib.h'
min_jpeg_version = 8

test_program = '''
#include "stdio.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "%s"
#ifdef __cplusplus
}
#endif

int
main() {
    return 0;
}
'''

jpeg_test_prog = '''
#include "stdio.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "%s"
#ifdef __cplusplus
}
#endif

int
main() {
    #if JPEG_LIB_VERSION_MAJOR < 8
     #error "JPEG_LIB_VERSION_MAJOR must be greater than or equal to 8"
    #else
    return 0;
    #endif
}
'''
jpeg_test_version = jpeg_test_prog % jpeg_inc_name


jpeg_search_paths = ['/usr','/usr/local']

png_inc_name = 'png.h'
png_test_prog = test_program  % png_inc_name

png_search_paths = ['/usr','/usr/local']

if Options.platform == 'darwin':
   # x11 has png headers (not jpeg)
   png_search_paths.append('/usr/X11')

def set_options(opt):
  opt.tool_options("compiler_cxx")
  opt.tool_options('misc')
  opt.add_option( '--with-jpeg'
                , action='store'
                , default=None
                , help='Directory prefix containing jpeg "lib" and "include" files'
                , dest='jpeg_dir'
                )

  opt.add_option( '--with-png'
                , action='store'
                , default=None
                , help='Directory prefix containing png "lib" and "include" files'
                , dest='png_dir'
                )

def _build_paths(conf,prefix):
    if not os.path.exists(prefix):
        _conf_exit(conf,'configure path of %s not found' % prefix)
    norm_path = os.path.normpath(os.path.realpath(prefix))
    if norm_path.endswith('lib') or norm_path.endswith('include'):
        norm_path = os.path.dirname(norm_path)
    return os.path.join('%s' % norm_path,'lib'),os.path.join('%s' % norm_path,'include')

def _conf_exit(conf,msg):
    conf.fatal('\n\n' + msg + '\n...check the build/config.log for details')
    
def configure(conf):
  conf.check_tool("compiler_cxx")
  conf.check_tool("node_addon")
  
  o = Options.options
 
  # jpeg checks
  if o.jpeg_dir:
      lib, inc = _build_paths(conf,o.jpeg_dir)
      header = os.path.join(inc,jpeg_inc_name)

      if conf.check_cxx(lib='jpeg',
                fragment=jpeg_test_prog % header,
                uselib_store='JPEG',
                libpath=lib,
                msg='Checking for libjpeg at %s' % header,
                includes=inc):
          Utils.pprint('GREEN', 'Sweet, found viable jpeg dependency at: %s ' % o.jpeg_dir)
      else:
          _conf_exit(conf,'jpeg libs/headers not found at %s' % o.jpeg_dir)
  else:
      # attempt to autoconfigure
      found = False
      found_lib = '/usr'
      found_inc = '/usr'
      for p in jpeg_search_paths:
          lib = os.path.join(p,'lib')
          inc = os.path.join(p,'include')
          header = os.path.join(inc,jpeg_inc_name)
          if os.path.exists(header):
              lib = os.path.join(p,'lib')
              if conf.check(lib='jpeg',
                        fragment=jpeg_test_prog % header,
                        uselib_store='JPEG',
                        libpath=lib,
                        msg='Checking for libjpeg at %s' % header,
                        includes=inc):
                  found = True
                  found_lib = lib
                  found_inc = inc
                  break
      
      if not found:
          _conf_exit(conf,'jpeg not found: searched %s \nuse --with-jpeg to point to the location of your jpeg libs and headers' % jpeg_search_paths)
      else:
          # confirm adequite version
          if not conf.check(lib='jpeg',
                    uselib_store='JPEG',
                    fragment=jpeg_test_version,
                    libpath=found_lib,
                    msg='Checking for libjpeg version >= %s' % min_jpeg_version,
                    includes=found_inc):
              _conf_exit(conf,'jpeg version >= 8 not found (upgrade to http://www.ijg.org/files/jpegsrc.v8c.tar.gz)')

  # png checks
  if o.png_dir:
      lib, include = _build_paths(conf,o.png_dir)

      if conf.check_cxx(lib='png',
                fragment=png_test_prog,
                uselib_store='PNG',
                libpath=lib,
                msg='Checking for libpng at %s' % lib,
                includes=include):
          Utils.pprint('GREEN', 'Sweet, found viable jpeg dependency at: %s ' % o.png_dir)
      else:
          _conf_exit(conf,'png libs/headers not found at %s' % o.png_dir)
  else:
      # attempt to autoconfigure
      found = False
      for p in png_search_paths:
          inc = os.path.join(p,'include')
          header = os.path.join(inc,png_inc_name)
          if os.path.exists(header):
              lib = os.path.join(p,'lib')
              if conf.check(lib='png',
                        uselib_store='PNG',
                        libpath=lib,
                        msg='Checking for libpng at %s' % lib,
                        includes=inc):
                  found = True
                  break
      
      if not found:
          _conf_exit(conf,'png not found: searched %s \nuse --with-png to point to the location of your png libs and headers' % png_search_paths)


def build(bld):
  obj = bld.new_task_gen("cxx", "shlib", "node_addon")
  obj.cxxflags = ["-O3", "-D_FILE_OFFSET_BITS=64", "-D_LARGEFILE_SOURCE", "-Wall", "-mfpmath=sse", "-march=core2",
        "-funroll-loops", "-fomit-frame-pointer"]
  obj.target = TARGET
  obj.source = ["src/reader.cc", "src/writer.cc", "src/blend.cc"]
  obj.uselib = ["PNG","JPEG"]

def shutdown():
  if Options.commands['clean']:
    if os.path.exists(TARGET_FILE):
      unlink(TARGET_FILE)
