#ifndef FMT_H
#define FMT_H

#define FMT_ULONG 39 /* enough space to hold 2^128 - 1 in decimal */
#define FMT_LEN ((char *) 0) /* convenient abbreviation */

extern unsigned int fmt_uint();
extern unsigned int fmt_xint();
extern unsigned int fmt_nbbint();
extern unsigned int fmt_ushort();
extern unsigned int fmt_xshort();
extern unsigned int fmt_nbbshort();
extern unsigned int fmt_ulong();
extern unsigned int fmt_xlong();
extern unsigned int fmt_nbblong();

extern unsigned int fmt_plusminus();
extern unsigned int fmt_minus();
extern unsigned int fmt_0x();

extern unsigned int fmt_strncpy();
extern unsigned int fmt_memcpy();
extern unsigned int fmt_vis();
extern unsigned int fmt_nvis();
extern unsigned int fmt_rvis();
extern unsigned int fmt_unrvis();

#endif
