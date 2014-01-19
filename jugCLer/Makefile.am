bin_PROGRAMS = jugCLer
jugCLer_CPPFLAGS = \
				$(GL_CFLAGS) \
				$(GLU_CFLAGS) \
				$(GLUT_CFLAGS) \
				$(BOOST_CPPFLAGS) \
				-I ../common
jugCLer_LDFLAGS = \
				$(GL_LIBS) \
				$(GLU_LIBS) \
				$(GLUT_LIBS) \
				$(BOOST_LDFLAGS) \
				$(BOOST_PROGRAM_OPTIONS_LIB) \
				$(BOOST_REGEX_LIB) \
				$(BOOST_FILESYSTEM_LIB) \
				$(BOOST_SYSTEM_LIB)
jugCLer_SOURCES = \
				jugCLer.cpp \
				scene.cpp \
				animation.cpp
jugCLer_LDADD = ../common/libocltoys_common.a
jugCLerdir = @datadir@/ocltoys/jugCLer
jugCLer_DATA = trace.cl
