import Options
from os.path import exists
from shutil import copy2 as copy

TARGET = 'blend'
TARGET_FILE = '%s.node' % TARGET

def set_options(opt):
  opt.tool_options("compiler_cxx")

def configure(conf):
  conf.check_tool("compiler_cxx")
  conf.check_tool("node_addon")
  # conf.check(lib='png', libpath=['/Users/kkaefer/Code/compositing/local/lib'], mandatory=True)
  conf.check(lib='png', libpath=['/usr/local/lib', '/usr/X11/lib', '/opt/local/lib'], mandatory=True)

def build(bld):
  obj = bld.new_task_gen("cxx", "shlib", "node_addon")
  obj.cxxflags = ["-O3", "-D_FILE_OFFSET_BITS=64", "-D_LARGEFILE_SOURCE", "-Wall", "-mfpmath=sse", "-march=core2"]
  # obj.cxxflags.append('-I/Users/kkaefer/Code/compositing/local/include')
  obj.cxxflags.append('-I/usr/X11/include')
  obj.target = TARGET
  obj.source = ["src/reader.cc", "src/blend.cc"]
  obj.uselib = "PNG"

def shutdown():
  if Options.commands['clean']:
    if exists(TARGET_FILE):
      unlink(TARGET_FILE)
