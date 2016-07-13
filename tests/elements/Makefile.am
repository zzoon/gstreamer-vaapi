noinst_PROGRAMS = \
	test-vaapisink  \
	test-vaapicontext  \
	$(NULL)

TEST_CFLAGS = \
	-DGST_USE_UNSTABLE_API		\
	-I$(top_srcdir)/gst-libs	\
	$(GST_CFLAGS)			\
	$(GST_VIDEO_CFLAGS)		\
	$(NULL)

TEST_LIBS = \
	$(GST_LIBS)			\
	$(NULL)

GST_VAAPI_LIBS = $(top_builddir)/gst-libs/gst/vaapi/libgstvaapi-public.la

test_vaapisink_SOURCES	= test-vaapisink.c
test_vaapisink_CFLAGS	= $(TEST_CFLAGS)
test_vaapisink_LDADD	= $(TEST_LIBS)

test_vaapicontext_SOURCES	= test-vaapicontext.c
test_vaapicontext_CFLAGS	= $(TEST_CFLAGS)
test_vaapicontext_LDFLAGS  = $(GST_VAAPI_LIBS)
test_vaapicontext_LDADD	= $(TEST_LIBS) $(X11_LIBS)

-include $(top_srcdir)/git.mk
