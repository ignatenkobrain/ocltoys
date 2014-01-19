bin_PROGRAMS = smallptgpu
smallptgpu_CPPFLAGS = \
				$(GL_CFLAGS) \
				$(GLU_CFLAGS) \
				$(GLUT_CFLAGS) \
				$(BOOST_CPPFLAGS) \
				-I ../common
smallptgpu_LDFLAGS = \
				$(GL_LIBS) \
				$(GLU_LIBS) \
				$(GLUT_LIBS) \
				$(BOOST_LDFLAGS) \
				$(BOOST_PROGRAM_OPTIONS_LIB) \
				$(BOOST_REGEX_LIB) \
				$(BOOST_FILESYSTEM_LIB) \
				$(BOOST_SYSTEM_LIB) \
				$(BOOST_THREAD_LIB)
smallptgpu_SOURCES = \
				smallptgpu.cpp \
				preprocessed_rendering_kernel.cl
smallptgpu_LDADD = ../common/libocltoys_common.a
smallptgpudir = @datadir@/ocltoys/smallptgpu
smallptgpu_DATA = preprocessed_rendering_kernel.cl
smallptgpuscenesdir = ${smallptgpudir}/scenes
smallptgpuscenes_DATA = scenes/*
smallptgpuwebcldir = ${smallptgpudir}/webcl
smallptgpuwebcl_DATA = webcl/*

preprocessed_rendering_kernel.cl:
				$(RM) preprocessed_rendering_kernel.cl
				cpp -I. -I ../common <rendering_kernel.cl \
				                     >preprocessed_rendering_kernel.cl
smallptgpu.cpp: preprocessed_rendering_kernel.cl
