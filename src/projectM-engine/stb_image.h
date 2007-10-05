/* stbi-1.00 - public domain JPEG/PNG reader - http://nothings.org/stb_image.c
                      when you control the images you're loading

   QUICK NOTES:
      Primarily of interest to game developers and other people who can
          avoid problematic images and only need the trivial interface

      JPEG baseline (no JPEG progressive, no oddball channel decimations)
      PNG non-interlaced
      BMP non-1bpp, non-RLE
      writes BMP,TGA (define STBI_NO_WRITE to remove code)
      decoded from memory or through stdio FILE (define STBI_NO_STDIO to remove code)

   JLD:
      added DDS decoding code
      added a few header guards, re-arrangements, etc.
      added the TGA loading code (though that is in the cannonical version now [8^)

   TODO:
      stbi_info_*
      PSD loader

   history:
      1.00   interface to zlib that skips zlib header
      0.99   correct handling of alpha in palette
      0.98   TGA loader by lonesock; dynamically add loaders (untested)
      0.97   jpeg errors on too large a file; also catch another malloc failure
      0.96   fix detection of invalid v value - particleman@mollyrocket forum
      0.95   during header scan, seek to markers in case of padding
      0.94   STBI_NO_STDIO to disable stdio usage; rename all #defines the same
      0.93   handle jpegtran output; verbose errors
      0.92   read 4,8,16,24,32-bit BMP files of several formats
      0.91   output 24-bit Windows 3.0 BMP files
      0.90   fix a few more warnings; bump version number to approach 1.0
      0.61   bugfixes due to Marc LeBlanc, Christopher Lloyd
      0.60   fix compiling as c++
      0.59   fix warnings: merge Dave Moore's -Wall fixes
      0.58   fix bug: zlib uncompressed mode len/nlen was wrong endian
      0.57   fix bug: jpg last huffman symbol before marker was >9 bits but less
                      than 16 available
      0.56   fix bug: zlib uncompressed mode len vs. nlen
      0.55   fix bug: restart_interval not initialized to 0
      0.54   allow NULL for 'int *comp'
      0.53   fix bug in png 3->4; speedup png decoding
      0.52   png handles req_comp=3,4 directly; minor cleanup; jpeg comments
      0.51   obey req_comp requests, 1-component jpegs return as 1-component,
             on 'test' only check type, not whether we support this variant
*/


/*
////   begin header file  ////////////////////////////////////////////////////
//
// Limitations:
//    - no progressive/interlaced support (jpeg, png)
//    - 8-bit samples only (jpeg, png)
//    - not threadsafe
//    - channel subsampling of at most 2 in each dimension (jpeg)
//    - no delayed line count (jpeg) -- IJG doesn't support either
//
// Basic usage:
//    int x,y,n;
//    unsigned char *data = stbi_load(filename, &x, &y, &n, 0);
//    // ... process data if not NULL ...
//    // ... x = width, y = height, n = # 8-bit components per pixel ...
//    // ... replace '0' with '1'..'4' to force that many components per pixel
//    stbi_image_free(data)
//
// Standard parameters:
//    int *x       -- outputs image width in pixels
//    int *y       -- outputs image height in pixels
//    int *comp    -- outputs # of image components in image file
//    int req_comp -- if non-zero, # of image components requested in result
//
// The return value from an image loader is an 'unsigned char *' which points
// to the pixel data. The pixel data consists of *y scanlines of *x pixels,
// with each pixel consisting of N interleaved 8-bit components; the first
// pixel pointed to is top-left-most in the image. There is no padding between
// image scanlines or between pixels, regardless of format. The number of
// components N is 'req_comp' if req_comp is non-zero, or *comp otherwise.
// If req_comp is non-zero, *comp has the number of components that _would_
// have been output otherwise. E.g. if you set req_comp to 4, you will always
// get RGBA output, but you can check *comp to easily see if it's opaque.
//
// An output image with N components has the following components interleaved
// in this order in each pixel:
//
//     N=#comp     components
//       1           grey
//       2           grey, alpha
//       3           red, green, blue
//       4           red, green, blue, alpha
//
// If image loading fails for any reason, the return value will be NULL,
// and *x, *y, *comp will be unchanged. The function stbi_failure_reason()
// can be queried for an extremely brief, end-user unfriendly explanation
// of why the load failed. Define STBI_NO_FAILURE_STRINGS to avoid
// compiling these strings at all, and STBI_FAILURE_USERMSG to get slightly
// more user-friendly ones.
//
// Paletted PNG and BMP images are automatically depalettized.
*/

#ifndef HEADER_STB_IMAGE
#define HEADER_STB_IMAGE

#ifndef STBI_NO_STDIO
#include <stdio.h>
#endif

enum
{
   STBI_default = 0, // only used for req_comp

   STBI_grey       = 1,
   STBI_grey_alpha = 2,
   STBI_rgb        = 3,
   STBI_rgb_alpha  = 4
};

typedef unsigned char stbi_uc;

#ifdef __cplusplus
extern "C" {
#endif

// WRITING API

#if !defined(STBI_NO_WRITE) && !defined(STBI_NO_STDIO)
// write a BMP/TGA file given tightly packed 'comp' channels (no padding, nor bmp-stride-padding)
// (you must include the appropriate extension in the filename).
// returns TRUE on success, FALSE if couldn't open file, error writing file
extern int      stbi_write_bmp       (char *filename,           int x, int y, int comp, void *data);
extern int      stbi_write_tga       (char *filename,           int x, int y, int comp, void *data);
#endif

// PRIMARY API - works on images of any type

// load image by filename, open file, or memory buffer
#ifndef STBI_NO_STDIO
extern stbi_uc *stbi_load            (char *filename,           int *x, int *y, int *comp, int req_comp);
extern stbi_uc *stbi_load_from_file  (FILE *f,                  int *x, int *y, int *comp, int req_comp);
extern int      stbi_info_from_file  (FILE *f,                  int *x, int *y, int *comp);
#endif
extern stbi_uc *stbi_load_from_memory(stbi_uc *buffer, int len, int *x, int *y, int *comp, int req_comp);
// for stbi_load_from_file, file pointer is left pointing immediately after image

// get a VERY brief reason for failure
extern char    *stbi_failure_reason  (void);

// free the loaded image -- this is just free()
extern void     stbi_image_free      (stbi_uc *retval_from_stbi_load);

// get image dimensions & components without fully decoding
extern int      stbi_info_from_memory(stbi_uc *buffer, int len, int *x, int *y, int *comp);
#ifndef STBI_NO_STDIO
extern int      stbi_info            (char *filename,           int *x, int *y, int *comp);
#endif

// ZLIB client - used by PNG, available for other purposes

extern char *stbi_zlib_decode_malloc_guesssize(int initial_size, int *outlen);
extern char *stbi_zlib_decode_malloc(char *buffer, int len, int *outlen);
extern int   stbi_zlib_decode_buffer(char *obuffer, int olen, char *ibuffer, int ilen);

extern char *stbi_zlib_decode_noheader_malloc(char *buffer, int len, int *outlen);
extern int   stbi_zlib_decode_noheader_buffer(char *obuffer, int olen, char *ibuffer, int ilen);


// TYPE-SPECIFIC ACCESS

// is it a jpeg?
extern int      stbi_jpeg_test_memory     (stbi_uc *buffer, int len);
extern stbi_uc *stbi_jpeg_load_from_memory(stbi_uc *buffer, int len, int *x, int *y, int *comp, int req_comp);
extern int      stbi_jpeg_info_from_memory(stbi_uc *buffer, int len, int *x, int *y, int *comp);

#ifndef STBI_NO_STDIO
extern stbi_uc *stbi_jpeg_load            (char *filename,           int *x, int *y, int *comp, int req_comp);
extern int      stbi_jpeg_test_file       (FILE *f);
extern stbi_uc *stbi_jpeg_load_from_file  (FILE *f,                  int *x, int *y, int *comp, int req_comp);

extern int      stbi_jpeg_info            (char *filename,           int *x, int *y, int *comp);
extern int      stbi_jpeg_info_from_file  (FILE *f,                  int *x, int *y, int *comp);
#endif

extern int      stbi_jpeg_dc_only; // only decode DC component

// is it a png?
extern int      stbi_png_test_memory      (stbi_uc *buffer, int len);
extern stbi_uc *stbi_png_load_from_memory (stbi_uc *buffer, int len, int *x, int *y, int *comp, int req_comp);
extern int      stbi_png_info_from_memory (stbi_uc *buffer, int len, int *x, int *y, int *comp);

#ifndef STBI_NO_STDIO
extern stbi_uc *stbi_png_load             (char *filename,           int *x, int *y, int *comp, int req_comp);
extern int      stbi_png_info             (char *filename,           int *x, int *y, int *comp);
extern int      stbi_png_test_file        (FILE *f);
extern stbi_uc *stbi_png_load_from_file   (FILE *f,                  int *x, int *y, int *comp, int req_comp);
extern int      stbi_png_info_from_file   (FILE *f,                  int *x, int *y, int *comp);
#endif

// is it a bmp?
extern int      stbi_bmp_test_memory      (stbi_uc *buffer, int len);

extern stbi_uc *stbi_bmp_load             (char *filename,           int *x, int *y, int *comp, int req_comp);
extern stbi_uc *stbi_bmp_load_from_memory (stbi_uc *buffer, int len, int *x, int *y, int *comp, int req_comp);
#ifndef STBI_NO_STDIO
extern int      stbi_bmp_test_file        (FILE *f);
extern stbi_uc *stbi_bmp_load_from_file   (FILE *f,                  int *x, int *y, int *comp, int req_comp);
#endif

// is it a tga?
extern int      stbi_tga_test_memory      (stbi_uc *buffer, int len);

extern stbi_uc *stbi_tga_load             (char *filename,           int *x, int *y, int *comp, int req_comp);
extern stbi_uc *stbi_tga_load_from_memory (stbi_uc *buffer, int len, int *x, int *y, int *comp, int req_comp);
#ifndef STBI_NO_STDIO
extern int      stbi_tga_test_file        (FILE *f);
extern stbi_uc *stbi_tga_load_from_file   (FILE *f,                  int *x, int *y, int *comp, int req_comp);
#endif

//	is it a DDS file?
extern int      stbi_dds_test_memory      (stbi_uc *buffer, int len);

extern stbi_uc *stbi_dds_load             (char *filename,           int *x, int *y, int *comp, int req_comp);
extern stbi_uc *stbi_dds_load_from_memory (stbi_uc *buffer, int len, int *x, int *y, int *comp, int req_comp);
#ifndef STBI_NO_STDIO
extern int      stbi_dds_test_file        (FILE *f);
extern stbi_uc *stbi_dds_load_from_file   (FILE *f,                  int *x, int *y, int *comp, int req_comp);
#endif

// define new loaders
typedef struct
{
   int       (*test_memory)(stbi_uc *buffer, int len);
   stbi_uc * (*load_from_memory)(stbi_uc *buffer, int len, int *x, int *y, int *comp, int req_comp);
   #ifndef STBI_NO_STDIO
   int       (*test_file)(FILE *f);
   stbi_uc * (*load_from_file)(FILE *f, int *x, int *y, int *comp, int req_comp);
   #endif
} stbi_loader;

// register a loader by filling out the above structure (you must defined ALL functions)
// returns 1 if added or already added, 0 if not added (too many loaders)
extern int stbi_register_loader(stbi_loader *loader);

#ifdef __cplusplus
}
#endif

//
//
////   end header file   /////////////////////////////////////////////////////
#endif // HEADER_STB_IMAGE