#define VIPS_DEBUG
#define HAVE_GDAL

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H*/
#include <vips/intl.h>

#ifdef HAVE_GDAL

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>

#include <vips/vips.h>
#include <vips/debug.h>

#include "gdal.h"
#include "cpl_conv.h" /* for CPLMalloc() */

#include "gdal2vips.h"

typedef struct {
	GDALDatasetH hDataset;

	int data_type;

	int tile_width;
	int tile_height;
} Read;

int
vips__gdal_isgdal( const char *filename )
{
	int ok = 0;

  GDALAllRegister();
  GDALDatasetH hDataset = GDALOpen( filename, GA_ReadOnly );
	if( hDataset != NULL ) {
		ok = 1;
		GDALClose (hDataset);
	}

	VIPS_DEBUG_MSG( "vips__gdal_isgdal: %s - %d\n", filename, ok );

	return( ok );
}

static void
read_destroy_cb( VipsImage *image, Read *read )
{
	VIPS_FREEF( GDALClose, read->hDataset );
}

static Read *
read_new( const char *filename, VipsImage *out )
{
	Read *read;
	int64_t w, h;

	read = VIPS_NEW( out, Read );
	memset( read, 0, sizeof( *read ) );
	g_signal_connect( out, "close", G_CALLBACK( read_destroy_cb ),
		read );

  GDALAllRegister();
  read->hDataset = GDALOpen( filename, GA_ReadOnly );
	if( read->hDataset == NULL ) {
		vips_error( "gdal2vips", 
			"%s", _( "unsupported gdal format" ) );
		return( NULL );
	}

	vips_image_pipelinev( out, VIPS_DEMAND_STYLE_SMALLTILE, NULL );

	w = GDALGetRasterXSize (read->hDataset);
	h = GDALGetRasterYSize (read->hDataset);

	GDALRasterBandH hBand;
	hBand = GDALGetRasterBand( read->hDataset, 1 );
	if( hBand == NULL ) {
		vips_error( "gdal2vips", 
			"%s", _( "can not open raster band" ) );
		return( NULL );
	}

	GDALGetBlockSize( hBand, &read->tile_width, &read->tile_height );

	read->data_type = GDALGetRasterDataType( hBand );
	// int vipsFormat = VIPS_FORMAT_UCHAR;

	// switch( dataType ) {
	// 	case GDT_Byte: vipsFormat = VIPS_FORMAT_UCHAR; break;
	// 	case GDT_UInt16: vipsFormat = VIPS_FORMAT_USHORT; break;

	// }

	vips_image_init_fields( out, w, h, 3, VIPS_FORMAT_UCHAR,
		VIPS_CODING_NONE, VIPS_INTERPRETATION_RGB, 1.0, 1.0 );

	return( read );
}

int
vips__gdal_read_header( const char *filename, VipsImage *out )
{
	if( !read_new( filename, out ) )
		return( -1 );

	return( 0 );
}

static int
vips__gdal_generate( VipsRegion *out, 
	void *_seq, void *_read, void *unused, gboolean *stop )
{
	Read *read = _read;
	VipsRect *r = &out->valid;
	int n = r->width * r->height;
	unsigned char *buf = (unsigned char *) VIPS_REGION_ADDR( out, r->left, r->top );

	VIPS_DEBUG_MSG( "vips__gdal_generate: %dx%d @ %dx%d\n",
		r->width, r->height, r->left, r->top );

	/* We're inside a cache, so requests should always be
	 * tile_width by tile_height pixels and on a tile boundary.
	 */
	g_assert( (r->left % read->tile_width) == 0 );
	g_assert( (r->top % read->tile_height) == 0 );
	g_assert( r->width <= read->tile_width );
	g_assert( r->height <= read->tile_height );

	g_assert( read->data_type == GDT_Byte );

	g_assert( GDALGetRasterCount );

  for (int channel = 0; channel < 3; ++channel) {

		GDALRasterBandH hBand;
		hBand = GDALGetRasterBand( read->hDataset, channel + 1 );

		unsigned char* gdal_data;
		gdal_data = (unsigned char*) CPLMalloc(sizeof(unsigned char) * r->width * r->height);
		GDALRasterIO( hBand, GF_Read, r->left, r->top, r->width, r->height, 
		              gdal_data, r->width, r->height, GDT_Byte, 
		              0, 0 );

		for (int i = 0; i < r->width * r->height; ++i) {
			buf[i * 3 + channel] = gdal_data[i];
		}

	  CPLFree (gdal_data);
	}

	return( 0 );
}

int
vips__gdal_read( const char *filename, VipsImage *out )
{
	Read *read;
	VipsImage *raw;
	VipsImage *t;

	VIPS_DEBUG_MSG( "vips__gdal_read: %s\n", 
		filename );

	raw = vips_image_new();
	vips_object_local( out, raw );

	if( !(read = read_new( filename, raw )) )
		return( -1 );

	if( vips_image_generate( raw, 
		NULL, vips__gdal_generate, NULL, read, NULL ) )
		return( -1 );

	/* Copy to out, adding a cache. Enough tiles for a complete row, plus
	 * 50%.
	 */
	if( vips_tilecache( raw, &t, 
		"tile_width", read->tile_width, 
		"tile_height", read->tile_height,
		"max_tiles", 
			(int) (1.5 * (1 + raw->Xsize / read->tile_width)),
		"threaded", FALSE,
		NULL ) ) 
		return( -1 );
	if( vips_image_write( t, out ) ) {
		g_object_unref( t );
		return( -1 );
	}
	g_object_unref( t );

	return( 0 );
}

#endif /*HAVE_GDAL*/
