srcdir = "."
blddir = "build"
VERSION = "0.0.2"

def set_options(opt):
  opt.tool_options("compiler_cxx")

def configure(conf):
  conf.find_program('gfortran', var="FC")
  conf.check_tool("compiler_cxx")
  conf.check_tool("node_addon")

def build(bld):
  #print(bld.env)

  fortran_libs = ["aind", "daxpy", "ddot", "dpofa", "dscal", "solve.QP", "util"]

  if bld.env['FC'] != '':
    bld.add_group("fortran")
    for f in fortran_libs:
      bld(
        rule = bld.env['FC'] + " -fPIC -o ${TGT} -c ${SRC}",
        source = 'src/' + f + '.f',
        target = f + '.o',
        name = f
      )
  else:
    bld.add_group("cp")
    for f in fortran_libs:
      bld(
        rule = "cp ${SRC} ${TGT}",
        source = 'obj/' + f + '.o',
        target = f + '.o',
        name = f
      )

  bld.add_group("c++")

  obj = bld.new_task_gen("cxx", "shlib", "node_addon")
  obj.cxxflags = [
    "-D_FILE_OFFSET_BITS=64",
    "-D_LARGEFILE_SOURCE"
  ]
  obj.linkflags = []
  for f in fortran_libs:
    obj.linkflags.append( "Release/" + f + ".o")
  obj.target = "quadprog"
  obj.source = "src/quadprog.cc"