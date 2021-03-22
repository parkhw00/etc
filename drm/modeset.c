
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <xf86drm.h>
#include <xf86drmMode.h>

#define debug(fmt,args...)	printf("%s.%d: " fmt, __func__, __LINE__, ##args)

#define fatal(fmt,args...)	_fatal(__func__, __LINE__, fmt, ##args)
void _fatal (const char *func, int line, const char *fmt, ...)
{
	va_list ap;

	printf ("%s:%d. error: %s(%d)\n", func, line, strerror (errno), errno);

	va_start (ap, fmt);
	vprintf (fmt, ap);
	va_end (ap);

	exit (1);
}

static int modeset_find_crtc(int fd, drmModeRes *res, drmModeConnector *conn)
{
	drmModeEncoder *enc;
	unsigned int i, j;

	/* iterate all encoders of this connector */
	for (i = 0; i < conn->count_encoders; ++i) {
		enc = drmModeGetEncoder(fd, conn->encoders[i]);
		if (!enc) {
			/* cannot retrieve encoder, ignoring... */
			continue;
		}

		/* iterate all global CRTCs */
		for (j = 0; j < res->count_crtcs; ++j) {
			/* check whether this CRTC works with the encoder */
			if (!(enc->possible_crtcs & (1 << j)))
				continue;

			/* Here you need to check that no other connector
			 * currently uses the CRTC with id "crtc". If you intend
			 * to drive one connector only, then you can skip this
			 * step. Otherwise, simply scan your list of configured
			 * connectors and CRTCs whether this CRTC is already
			 * used. If it is, then simply continue the search here. */
			//if (res->crtcs[j] "is unused")
			{
				drmModeFreeEncoder(enc);
				return res->crtcs[j];
			}
		}

		drmModeFreeEncoder(enc);
	}

	/* cannot find a suitable CRTC */
	return -ENOENT;
}

#define define_connection_name(n)	[DRM_MODE_##n] = #n
const char *connection_name[] =
{
	define_connection_name(CONNECTED),
	define_connection_name(DISCONNECTED),
	define_connection_name(UNKNOWNCONNECTION),
};
#undef define_connection_name

const char *get_connection_name(unsigned int type)
{
	if (type >= (sizeof (connection_name) / sizeof (connection_name[0])))
		return "_Unknown_";

	if (connection_name[type])
		return connection_name[type];

	return "_Unknown_";
}

#define define_conn_type_name(n)	[DRM_MODE_CONNECTOR_##n] = #n
const char *conn_type_name[] =
{
	define_conn_type_name(Unknown),
	define_conn_type_name(VGA),
	define_conn_type_name(DVII),
	define_conn_type_name(DVID),
	define_conn_type_name(DVIA),
	define_conn_type_name(Composite),
	define_conn_type_name(SVIDEO),
	define_conn_type_name(LVDS),
	define_conn_type_name(Component),
	define_conn_type_name(9PinDIN),
	define_conn_type_name(DisplayPort),
	define_conn_type_name(HDMIA),
	define_conn_type_name(HDMIB),
	define_conn_type_name(TV),
	define_conn_type_name(eDP),
	define_conn_type_name(VIRTUAL),
	define_conn_type_name(DSI),
	define_conn_type_name(DPI),
	define_conn_type_name(WRITEBACK),
#ifdef DRM_MODE_CONNECTOR_SPI
	define_conn_type_name(SPI),
#endif
};
#undef define_conn_type_name

const char *get_conn_type_name(unsigned int type)
{
	if (type >= (sizeof (conn_type_name) / sizeof (conn_type_name[0])))
		return "_Unknown_";

	if (conn_type_name[type])
		return conn_type_name[type];

	return "_Unknown_";
}

#define define_enc_type_name(n)	[DRM_MODE_ENCODER_##n] = #n
const char *enc_type_name[] =
{
	define_enc_type_name(NONE),
	define_enc_type_name(DAC),
	define_enc_type_name(TMDS),
	define_enc_type_name(LVDS),
	define_enc_type_name(TVDAC),
	define_enc_type_name(VIRTUAL),
	define_enc_type_name(DSI),
	define_enc_type_name(DPMST),
	define_enc_type_name(DPI),
};
#undef define_enc_type_name

const char *get_enc_type_name(unsigned int type)
{
	if (type >= (sizeof (enc_type_name) / sizeof (enc_type_name[0])))
		return "_Unknown_";

	if (enc_type_name[type])
		return enc_type_name[type];

	return "_Unknown_";
}

int main (int argc, char **argv)
{
	int fd;
	int i;
	drmModeResPtr res;
	drmVersionPtr ver;

	fd = open ("/dev/dri/card0", O_RDWR);
	if (fd < 0)
		fatal ("open() failed.\n");

	ver = drmGetVersion (fd);
	if (ver->name) printf ("name %s\n", ver->name);
	if (ver->date) printf ("date %s\n", ver->date);
	if (ver->desc) printf ("desc %s\n", ver->desc);

	res = drmModeGetResources (fd);
	if (!res)
		fatal ("drmModeGetResources() failed.\n");

	for (i=0; i<res->count_connectors; i++)
	{
		int j;
		drmModeConnectorPtr conn;

		conn = drmModeGetConnector (fd, res->connectors[i]);
		if (!conn)
			fatal ("drmModeGetConnector() failed\n");

#define member(f,m)	printf("connector[%d].%s : "f"\n", i, #m, conn->m)
		member ("%d", connector_id);
		member ("%d", encoder_id);
		printf("connector[%d].connector_type : %s(%d)\n", i, get_conn_type_name (conn->connector_type), conn->connector_type);
		member ("%d", connector_type_id);
		printf("connector[%d].connection : %s(%d)\n", i, get_connection_name (conn->connection), conn->connection);
		member ("%d", mmWidth);
		member ("%d", mmHeight);
		member ("%d", subpixel);
		member ("%d", count_modes);
		member ("%d", count_props);
		member ("%d", count_encoders);
#undef member

		for (j=0; j<conn->count_modes; j++)
			debug ("connector[%d].modes[%d].name %s\n", i, j, conn->modes[j].name);
		for (j=0; j<conn->count_props; j++)
		{
			drmModePropertyPtr prop;

			prop = drmModeGetProperty (fd, conn->props[j]);
			debug ("connector[%d].props[%d].name %s = %"PRIu64"\n", i, j, prop->name, conn->prop_values[j]);
			drmModeFreeProperty (prop);
		}
		for (j=0; j<conn->count_encoders; j++)
		{
			drmModeEncoderPtr enc;

			enc = drmModeGetEncoder (fd, conn->encoders[j]);
			debug ("connector[%d].encoders[%d].id %d\n", i, j, enc->encoder_id);
			debug ("connector[%d].encoders[%d].type %s(%d)\n", i, j, get_enc_type_name (enc->encoder_type), enc->encoder_type);
			drmModeFreeEncoder (enc);
		}

		printf ("\n");

		modeset_find_crtc (fd, res, conn);

		drmModeFreeConnector (conn);
	}

	drmModeFreeResources (res);

	return 0;
}
