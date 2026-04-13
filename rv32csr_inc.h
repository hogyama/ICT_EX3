
/// WARL : Write Any Values, Read Legal Values

#if defined(CSR_ENUM)
#define CSR_E(name, addr)               CSR_##name,
#define CSR_RO(name, addr, val)         CSR_##name,
#define CSR_RW(name, addr)              CSR_##name,
#define CSR_RW_WARL(name, addr, mask)   CSR_##name,
#define CSR_Rm(name, addr)              CSR_##name,
#elif defined(CSR_REG_DEF)
#define CSR_E(name, addr)       /// register defined in SYSCTRL
#define CSR_RO(name, addr, val) /// ReadOnly
#define CSR_RW(name, addr)              U32 name;
#define CSR_RW_WARL(name, addr, mask)   U32 name;
#define CSR_Rm(name, addr)      /// register defined in machine-level CSR
#elif defined(CSR_READ)
#define CSR_E(name, addr)               /// no read
#define CSR_RO(name, addr, val)         case addr: return (val);
#define CSR_RW(name, addr)              case addr: return name;
#define CSR_RW_WARL(name, addr, mask)   case addr: return name;
#define CSR_Rm(name, addr)              case addr: return m##name; /// mirror register in machine-level CSR
#elif defined(CSR_WRITE)
#define CSR_E(name, addr)       /// no write
#define CSR_RO(name, addr, val) /// no write
#define CSR_RW(name, addr)              case addr: name = wdata; break;
#define CSR_RW_WARL(name, addr, mask)   case addr: name = wdata & (mask); break;
#define CSR_Rm(name, addr)      /// no write
#elif defined(CSR_CHECK)
#define CSR_E(name, addr)               case addr:
#define CSR_RO(name, addr, val)         case addr:
#define CSR_RW(name, addr)              case addr:
#define CSR_RW_WARL(name, addr, mask)   case addr:
#define CSR_Rm(name, addr)              case addr:
#else
#define CSR_E(name, addr)
#define CSR_RO(name, addr)
#define CSR_RW(name, addr)
#define CSR_RW_MASK(name, addr, mask)
#define CSR_Rm(name, addr)
#endif


//////// unpriviledged counter/timers /////////
CSR_Rm(cycle,       0xc00) /// mirror register in machine-level CSR
CSR_E (time,        0xc01) /// mirror register in SYSCTRL
CSR_Rm(instret,     0xc02) /// mirror register in machine-level CSR
CSR_Rm(cycleh,      0xc80) /// mirror register in machine-level CSR
CSR_E (timeh,       0xc81) /// mirror register in SYSCTRL
CSR_Rm(instreth,    0xc82) /// mirror register in machine-level CSR
//////// machine-level csrs /////////
CSR_RO(mvendorid,       0xf11, 0)
CSR_RO(marchid,         0xf12, 0)
CSR_RO(mimpid,          0xf13, 0)
CSR_RO(mhartid,         0xf14, 0)
CSR_RW_WARL(mstatus,    0x300, (1 << 3) | (1 << 7) | (3 << 11))         /// mie[3] | mpie[7] | mpp[12:11]
CSR_RO(misa,            0x301, (1 << ('I' - 'A')) | (1 << (32 - 2)))    /// RV32I, 32-bit : MXL = 1
CSR_RW_WARL(mie,        0x304, (1 << 3) | (1 << 7) | (1 << 11))         /// msie[3] | mtie[7] | meie[11] 
CSR_RW_WARL(mtvec,      0x305, 0xfffffffc)  /// Direct mode only : All exceptions set pc to BASE
CSR_RW(mscratch,        0x340)
CSR_RW(mepc,            0x341)
CSR_RW(mcause,          0x342)
CSR_RW(mtval,           0x343)
CSR_RW_WARL(mip,        0x344, (1 << 3) | (1 << 7) | (1 << 11))         /// msip[3] | mtip[7] | meip[11] 
CSR_RW(mcycle,          0xb00)
CSR_RW(minstret,        0xb02)
CSR_RW(mcycleh,         0xb80)
CSR_RW(minstreth,       0xb82)


#undef CSR_RO
#undef CSR_Rm
#undef CSR_W
#undef CSR_RW
#undef CSR_RW_WARL
#undef CSR_E
#undef CSR_ENUM
#undef CSR_REG_DEF
#undef CSR_READ
#undef CSR_WRITE
#undef CSR_CHECK
