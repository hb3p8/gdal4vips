
#define DEBUG
#define HAVE_GDAL

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H*/
#include <vips/intl.h>

#ifdef HAVE_GDAL

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vips/vips.h>
#include <vips/buf.h>
#include <vips/internal.h>

#include "gdal2vips.h"

typedef struct _VipsForeignLoadGDAL {
	VipsForeignLoad parent_object;

	/* Filename for load.
	 */
	char *filename; 

} VipsForeignLoadGDAL;

typedef VipsForeignLoadClass VipsForeignLoadGDALClass;

G_DEFINE_TYPE( VipsForeignLoadGDAL, vips_foreign_load_gdal, 
	VIPS_TYPE_FOREIGN_LOAD );

static VipsForeignFlags
vips_foreign_load_gdal_get_flags_filename( const char *filename )
{
	/* We can't tell from just the filename, we need to know what part of
	 * the file the user wants. But it'll usually be partial.
	 */
	return( VIPS_FOREIGN_PARTIAL );
}

static VipsForeignFlags
vips_foreign_load_gdal_get_flags( VipsForeignLoad *load )
{
	VipsForeignLoadGDAL *gdal = (VipsForeignLoadGDAL *) load;
	VipsForeignFlags flags;

	flags = 0;
	flags |= VIPS_FOREIGN_PARTIAL;

	return( flags );
}

static int
vips_foreign_load_gdal_header( VipsForeignLoad *load )
{
	VipsForeignLoadGDAL *gdal = (VipsForeignLoadGDAL *) load;

	if( vips__gdal_read_header( gdal->filename, load->out) )
		return( -1 );

	VIPS_SETSTR( load->out->filename, gdal->filename );

	return( 0 );
}

static int
vips_foreign_load_gdal_load( VipsForeignLoad *load )
{
	VipsForeignLoadGDAL *gdal = (VipsForeignLoadGDAL *) load;

	if( vips__gdal_read( gdal->filename, load->real) )
		return( -1 );

	return( 0 );
}

static const char *vips_foreign_gdal_suffs[] = {
	".jp2", 	/* JPEG-2000 */
	".j2k",
	NULL
};

static void
vips_foreign_load_gdal_class_init( VipsForeignLoadGDALClass *class )
{
	GObjectClass *gobject_class = G_OBJECT_CLASS( class );
	VipsObjectClass *object_class = (VipsObjectClass *) class;
	VipsForeignClass *foreign_class = (VipsForeignClass *) class;
	VipsForeignLoadClass *load_class = (VipsForeignLoadClass *) class;

	gobject_class->set_property = vips_object_set_property;
	gobject_class->get_property = vips_object_get_property;

	object_class->nickname = "gdalload";
	object_class->description = _( "load file with GDAL" );

	foreign_class->priority = -100;
	foreign_class->suffs = vips_foreign_gdal_suffs;

	load_class->is_a = vips__gdal_isgdal;
	load_class->get_flags_filename = 
		vips_foreign_load_gdal_get_flags_filename;
	load_class->get_flags = vips_foreign_load_gdal_get_flags;
	load_class->header = vips_foreign_load_gdal_header;
	load_class->load = vips_foreign_load_gdal_load;

	VIPS_ARG_STRING( class, "filename", 1, 
		_( "Filename" ),
		_( "Filename to load from" ),
		VIPS_ARGUMENT_REQUIRED_INPUT, 
		G_STRUCT_OFFSET( VipsForeignLoadGDAL, filename ),
		NULL );
}

static void
vips_foreign_load_gdal_init( VipsForeignLoadGDAL *gdal )
{
}

#endif /*HAVE_GDAL*/
