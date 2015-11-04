#ifndef VIPS_GDAL2VIPS_H
#define VIPS_GDAL2VIPS_H

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

int vips__gdal_isgdal( const char *filename );
int vips__gdal_read_header( const char *filename, VipsImage *out );
int vips__gdal_read( const char *filename, VipsImage *out );

#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif /*VIPS_GDAL2VIPS_H*/
