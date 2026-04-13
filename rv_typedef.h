#if !defined(RV_TYPEDEF_H)
#define RV_TYPEDEF_H

#if defined(__LLVM_C2RTL__)
#define _BW(N)		    __attribute__((C2RTL_bit_width(N)))
#define _BWT(N)         __attribute__((C2RTL_bit_width(#N)))
#define _T(N)		    __attribute__((C2RTL_type(N)))
#define _C2R_FUNC(N)    __attribute__((C2RTL_function(N)))
#define _C2R_MODULE_    __attribute__((C2RTL_module))
#define _C2R_CLK_       __attribute__((C2RTL_clock(0)))
#define _C2R_CLK_SET_   __attribute__((C2RTL_clock(1)))
#define _C2R_CLK_RST_   __attribute__((C2RTL_clock(2)))
#else
#define _BW(N)
#define _BWT(N)
#define _T(N)
#define _C2R_FUNC(N)
#define _C2R_MODULE_
#define _C2R_CLK_
#define _C2R_CLK_SET_
#define _C2R_CLK_RST_
#endif

typedef unsigned char UCHAR;
typedef unsigned short USHORT;

typedef UCHAR BIT     _BW(1);
typedef UCHAR U2      _BW(2);
typedef UCHAR U3      _BW(3);
typedef UCHAR U4      _BW(4);
typedef UCHAR U5      _BW(5);
typedef UCHAR U6      _BW(6);
typedef UCHAR U7      _BW(7);
typedef UCHAR U8      _BW(8);
typedef USHORT U12    _BW(12);
typedef USHORT U15    _BW(15);
typedef USHORT U16    _BW(16);
typedef unsigned U32  _BW(32);
typedef signed   S32  _BW(32);


#endif
