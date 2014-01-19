bin_PROGRAMS = juliagpu
juliagpu_CPPFLAGS = \
				$(GL_CFLAGS) \
				$(GLU_CFLAGS) \
				$(GLUT_CFLAGS) \
				$(BOOST_CPPFLAGS) \
				-I ../common
juliagpu_LDFLAGS = \
				$(GL_LIBS) \
				$(GLU_LIBS) \
				$(GLUT_LIBS) \
				$(BOOST_LDFLAGS) \
				$(BOOST_PROGRAM_OPTIONS_LIB) \
				$(BOOST_REGEX_LIB) \
				$(BOOST_FILESYSTEM_LIB) \
				$(BOOST_SYSTEM_LIB)
juliagpu_SOURCES = \
				juliagpu.cpp \
				preprocessed_rendering_kernel.cl
juliagpu_LDADD = ../common/libocltoys_common.a
juliagpudir = @datadir@/ocltoys/juliagpu
juliagpu_DATA = preprocessed_rendering_kernel.cl

preprocessed_rendering_kernel.cl:
				$(RM) preprocessed_rendering_kernel.cl
				cpp -I. -I ../common <rendering_kernel.cl \
				                     >preprocessed_rendering_kernel.cl
juliagpu.cpp: preprocessed_rendering_kernel.cl
