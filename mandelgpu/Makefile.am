bin_PROGRAMS = mandelgpu
mandelgpu_CPPFLAGS = \
				$(GL_CFLAGS) \
				$(GLU_CFLAGS) \
				$(GLUT_CFLAGS) \
				$(BOOST_CPPFLAGS) \
				-I ../common
mandelgpu_LDFLAGS = \
				$(GL_LIBS) \
				$(GLU_LIBS) \
				$(GLUT_LIBS) \
				$(BOOST_LDFLAGS) \
				$(BOOST_PROGRAM_OPTIONS_LIB) \
				$(BOOST_REGEX_LIB) \
				$(BOOST_FILESYSTEM_LIB) \
				$(BOOST_SYSTEM_LIB)
mandelgpu_SOURCES = \
				mandelgpu.cpp
mandelgpu_LDADD = ../common/libocltoys_common.a
mandelgpudir = @datadir@/ocltoys/mandelgpu
mandelgpu_DATA = rendering_kernel_float4.cl
