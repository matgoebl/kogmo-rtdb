
// These JPEG-Routines (j*.* files) are taken from: http://www.tac.dk/software/viz/src/imageio/
// Added:
// - "int input_components" to write_JPEG_memory()
// - adapted input_components&in_color_space&row_stride
// (mg)

/* memio.c: routines that use the memory destination and source module
** BASED ON example.c
**/

#include <sys/types.h>
#include <stdio.h>

#include "jpeglib.h"
#include "jpegfix.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern GLOBAL(void)
write_JPEG_memory (unsigned char *img_buf, int width, int height,
                   unsigned char *jpeg_buffer, unsigned long jpeg_buffer_size,
                   int quality, unsigned long *jpeg_comp_size, int input_components);

extern GLOBAL(int)
read_JPEG_memory (unsigned char *img_buf, int *width, int *height,
                  const unsigned char *jpeg_buffer,
                  unsigned long jpeg_buffer_size);

#ifdef __cplusplus
}
#endif /* __cplusplus */
