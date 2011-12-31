

#if JPEG_LIB_VERSION <= 60
#undef GLOBAL
#define GLOBAL(type) type
#undef METHODDEF
#define METHODDEF(type) type
#endif

