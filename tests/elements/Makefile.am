noinst_PROGRAMS = \
	test-vaapisink  \
	$(NULL)

TEST_CFLAGS = \
	-DGST_USE_UNSTABLE_API		\
	-I$(top_srcdir)/gst-libs	\
	$(GST_CFLAGS)			\
	$(GST_VIDEO_CFLAGS)		\
	$(LIBVA_CFLAGS)			\
	$(NULL)

TEST_LIBS = \
	$(GST_LIBS) 			\
	$(GST_VIDEO_LIBS)		\
	$(LIBVA_LIBS)			\
	$(NULL)

if USE_X11
TEST_CFLAGS	+= $(X11_CFLAGS)
TEST_LIBS	+= \
	$(X11_LIBS)			\
	$(LIBVA_X11_LIBS)		\
	$(NULL)
endif

if USE_WAYLAND
TEST_CFLAGS	+= $(WAYLAND_CFLAGS)
TEST_LIBS	+= \
	$(WAYLAND_LIBS)			\
	$(NULL)
endif

test_vaapisink_SOURCES	= test-vaapisink.c
test_vaapisink_CFLAGS	= $(TEST_CFLAGS)
test_vaapisink_LDADD	= $(TEST_LIBS)

if USE_GTK3
noinst_PROGRAMS += test-vaapicontext
test_vaapicontext_SOURCES	= test-vaapicontext.c
test_vaapicontext_CFLAGS	= $(TEST_CFLAGS) $(GTK3_CFLAGS)
test_vaapicontext_LDADD		= $(TEST_LIBS) $(GTK3_LIBS)
endif

-include $(top_srcdir)/git.mk
