project('pgrid', 'c')

add_global_arguments('-fno-omit-frame-pointer', language : 'c')

incdir = include_directories('include')

gllib = library('glad', 'lib/gl.c', include_directories : incdir)

deps = [dependency('glfw3'), dependency('libturbojpeg'), dependency('cglm'),
	dependency('threads')]

lib = library('pgrid', 'lib/pgrid.c', 'lib/log.c', include_directories : incdir,
	dependencies : deps, link_with : gllib)

executable('pgrid', 'src/main.c', include_directories : incdir,
	dependencies : deps, link_with : [lib, gllib])

executable('minimal', 'src/examples/minimal.c', include_directories : incdir,
	dependencies : deps, link_with : [lib, gllib])

executable('bench', 'src/examples/bench.c', include_directories : incdir,
	dependencies : deps, link_with : [lib, gllib])
