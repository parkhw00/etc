
#define _GNU_SOURCE

#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <linux/dma-heap.h>
#include <drm/drm_fourcc.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#define debug(fmt, args...)	fprintf(stderr, "%s.%d: "fmt, __func__, __LINE__, ##args)
#define fatal(fmt, args...)	do{debug("FATAL - "fmt, ##args); exit(1);}while(0)
#define check_egl_error()	do{int err = eglGetError(); if(err != EGL_SUCCESS) fatal("egl error. 0x%x\n", err);}while(0)

int get_dmabuf(int size)
{
	int fd = open("/dev/dma_heap/system", O_RDWR);
	if (fd < 0)
		fatal("no dma heap. %s\n", strerror(errno));

	struct dma_heap_allocation_data alloc;
	memset(&alloc, 0, sizeof(alloc));
	alloc.len = size;
	alloc.fd_flags = O_CLOEXEC | O_RDWR;
	int ret = ioctl(fd, DMA_HEAP_IOCTL_ALLOC, &alloc);
	if (ret < 0)
		fatal("alloc failed. %s\n", strerror(errno));

	close(fd);

	debug("dmabuf %d(size %d)\n", alloc.fd, size);
	return alloc.fd;
}

struct fourcc_info
{
	unsigned int fourcc;
	char *name;
} fourcc_infos[] =
{
#define define_drm_format(n)	{ DRM_FORMAT_##n, #n, }
	define_drm_format(C8),
	define_drm_format(R8),
	define_drm_format(R10),
	define_drm_format(R12),
	define_drm_format(R16),
	define_drm_format(RG88),
	define_drm_format(GR88),
	define_drm_format(RG1616),
	define_drm_format(GR1616),
	define_drm_format(RGB332),
	define_drm_format(BGR233),
	define_drm_format(XRGB4444),
	define_drm_format(XBGR4444),
	define_drm_format(RGBX4444),
	define_drm_format(BGRX4444),
	define_drm_format(ARGB4444),
	define_drm_format(ABGR4444),
	define_drm_format(RGBA4444),
	define_drm_format(BGRA4444),
	define_drm_format(XRGB1555),
	define_drm_format(XBGR1555),
	define_drm_format(RGBX5551),
	define_drm_format(BGRX5551),
	define_drm_format(ARGB1555),
	define_drm_format(ABGR1555),
	define_drm_format(RGBA5551),
	define_drm_format(BGRA5551),
	define_drm_format(RGB565),
	define_drm_format(BGR565),
	define_drm_format(RGB888),
	define_drm_format(BGR888),
	define_drm_format(XRGB8888),
	define_drm_format(XBGR8888),
	define_drm_format(RGBX8888),
	define_drm_format(BGRX8888),
	define_drm_format(ARGB8888),
	define_drm_format(ABGR8888),
	define_drm_format(RGBA8888),
	define_drm_format(BGRA8888),
	define_drm_format(XRGB2101010),
	define_drm_format(XBGR2101010),
	define_drm_format(RGBX1010102),
	define_drm_format(BGRX1010102),
	define_drm_format(ARGB2101010),
	define_drm_format(ABGR2101010),
	define_drm_format(RGBA1010102),
	define_drm_format(BGRA1010102),
	define_drm_format(XRGB16161616),
	define_drm_format(XBGR16161616),
	define_drm_format(ARGB16161616),
	define_drm_format(ABGR16161616),
	define_drm_format(XRGB16161616F),
	define_drm_format(XBGR16161616F),
	define_drm_format(ARGB16161616F),
	define_drm_format(ABGR16161616F),
	define_drm_format(AXBXGXRX106106106106),
	define_drm_format(YUYV),
	define_drm_format(YVYU),
	define_drm_format(UYVY),
	define_drm_format(VYUY),
	define_drm_format(AYUV),
	define_drm_format(XYUV8888),
	define_drm_format(VUY888),
	define_drm_format(VUY101010),
	define_drm_format(Y210),
	define_drm_format(Y212),
	define_drm_format(Y216),
	define_drm_format(Y410),
	define_drm_format(Y412),
	define_drm_format(Y416),
	define_drm_format(XVYU2101010),
	define_drm_format(XVYU12_16161616),
	define_drm_format(XVYU16161616),
	define_drm_format(Y0L0),
	define_drm_format(X0L0),
	define_drm_format(Y0L2),
	define_drm_format(X0L2),
	define_drm_format(YUV420_8BIT),
	define_drm_format(YUV420_10BIT),
	define_drm_format(XRGB8888_A8),
	define_drm_format(XBGR8888_A8),
	define_drm_format(RGBX8888_A8),
	define_drm_format(BGRX8888_A8),
	define_drm_format(RGB888_A8),
	define_drm_format(BGR888_A8),
	define_drm_format(RGB565_A8),
	define_drm_format(BGR565_A8),
	define_drm_format(NV12),
	define_drm_format(NV21),
	define_drm_format(NV16),
	define_drm_format(NV61),
	define_drm_format(NV24),
	define_drm_format(NV42),
	define_drm_format(NV15),
	define_drm_format(P210),
	define_drm_format(P010),
	define_drm_format(P012),
	define_drm_format(P016),
	define_drm_format(P030),
	define_drm_format(Q410),
	define_drm_format(Q401),
	define_drm_format(YUV410),
	define_drm_format(YVU410),
	define_drm_format(YUV411),
	define_drm_format(YVU411),
	define_drm_format(YUV420),
	define_drm_format(YVU420),
	define_drm_format(YUV422),
	define_drm_format(YVU422),
	define_drm_format(YUV444),
	define_drm_format(YVU444),
#undef define_drm_format
	{ },
};

char *fourcc_name(int fourcc)
{
	int i;
	for (i=0; fourcc_infos[i].name; i++)
		if (fourcc_infos[i].fourcc == fourcc)
			return fourcc_infos[i].name;
	return NULL;
}

int test_import(void)
{
	int i;

#define load_egl(t,n)	t n = (t)eglGetProcAddress(#n); if(!n) fatal("no "#n"\n");
	load_egl(PFNEGLCREATEIMAGEKHRPROC, eglCreateImageKHR);
	load_egl(PFNEGLDESTROYIMAGEPROC, eglDestroyImageKHR);
	load_egl(PFNEGLQUERYDMABUFFORMATSEXTPROC, eglQueryDmaBufFormatsEXT);
	load_egl(PFNEGLQUERYDMABUFMODIFIERSEXTPROC, eglQueryDmaBufModifiersEXT);

	EGLDisplay dpy = eglGetDisplay(NULL);
	check_egl_error();

	int major = 0, minor = 0;
	eglInitialize(dpy, &major, &minor);
	check_egl_error();
	debug("%d.%d\n", major, minor);

	int num_formats = 0;
	eglQueryDmaBufFormatsEXT(dpy, 0, NULL, &num_formats);
	check_egl_error();
	debug("num dmabuf formats %d\n", num_formats);

	int dmabuf_formats[num_formats];
	memset(dmabuf_formats, 0, sizeof(dmabuf_formats));
	eglQueryDmaBufFormatsEXT(dpy, num_formats, dmabuf_formats, &num_formats);
	check_egl_error();
	for (i=0; i<num_formats; i++)
	{
		int num_modifiers = 0;
		eglQueryDmaBufModifiersEXT(dpy, dmabuf_formats[i], 0, NULL, NULL, &num_modifiers);
		check_egl_error();

		char *mods = NULL;
		if (num_modifiers > 0)
		{
			EGLuint64KHR modifiers[num_modifiers];
			EGLBoolean external_only[num_modifiers];

			eglQueryDmaBufModifiersEXT(dpy, dmabuf_formats[i], num_modifiers, modifiers, external_only, &num_modifiers);

			int j;
			for (j=0; j<num_modifiers; j++)
			{
				char *t = NULL;

				asprintf(&t, "%s0x%016llx%c", mods?mods:" ", (unsigned long long)modifiers[j], external_only[j]?'!':' ');
				if (mods)
					free(mods);
				mods = t;
			}
		}

		debug("%2d. dmabuf format (0x%08x)DRM_FORMAT_%-14s modifiers(%d)%s\n", i,
				dmabuf_formats[i],
				fourcc_name(dmabuf_formats[i]),
				num_modifiers, mods?mods:"");
		if (mods)
			free(mods);
	}

#if 0
	EGLConfig config;
	int num_config = 0;
	int config_attrs[] =
	{
		EGL_ALPHA_SIZE, 1,
		EGL_RED_SIZE, 1,
		EGL_GREEN_SIZE, 1,
		EGL_BLUE_SIZE, 1,
		EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
		EGL_NONE,
	};
	eglChooseConfig(dpy, config_attrs, &config, 1, &num_config);
	check_egl_error();
	if (num_config != 1)
		fatal("no config.\n");

	EGLContext ctx;
	ctx = eglCreateContext(dpy, config, EGL_NO_CONTEXT, NULL);
	check_egl_error();
#endif

	int width = 400;
	int height = 300;
	int pitch = width * 4;//2;
	int size = pitch * height;
	int fd = get_dmabuf(size);
	int dmabuf_attrs[] =
	{
		EGL_WIDTH, width,
		EGL_HEIGHT, height,
		EGL_LINUX_DRM_FOURCC_EXT, DRM_FORMAT_Y410,//DRM_FORMAT_Y0L2,
		EGL_DMA_BUF_PLANE0_FD_EXT, fd,
		EGL_DMA_BUF_PLANE0_OFFSET_EXT, 0,
		EGL_DMA_BUF_PLANE0_PITCH_EXT, pitch,
		EGL_NONE,
	};
	eglCreateImageKHR(dpy, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, NULL, dmabuf_attrs);
	check_egl_error();

	eglTerminate(dpy);

	return 0;
}

int main (int argc, char **argv)
{
	return test_import();
}
