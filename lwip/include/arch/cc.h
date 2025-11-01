#ifndef __ARCH_CC_H__
#define __ARCH_CC_H__

typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;
typedef uint pde_t;

extern void cprintf(char*, ...);

#define U16_F "u"
#define S16_F "d"
#define X16_F "x"
#define U32_F "u"
#define S32_F "d"
#define X32_F "x"

#define PACK_STRUCT_FIELD(x) x
#define PACK_STRUCT_STRUCT __attribute__((packed))
#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_END

#define LWIP_PLATFORM_DIAG(x) do { cprintf x; } while(0)
#define LWIP_PLATFORM_ASSERT(x) do { cprintf("Assertion \"%s\" failed at line %d in %s\n", \
                                             x, __LINE__, __FILE__); panic("lwIP assert"); } while(0)

#define BYTE_ORDER LITTLE_ENDIAN

#endif
