/* fmt.c, fmt.h: formatting library
Daniel J. Bernstein, brnstnd@nyu.edu.
No dependencies.
No environment requirements.
10/5/91: Cleaned up fmt_rvis, added fmt_unrvis.
9/1/91: Added fmt_nvis, fmt_rvis.
8/28/91: Added fmt_vis.
7/18/91: Baseline. fmt 1.0, public domain.
No known patent problems.

XXX: still need floating-point formatting

*/

#include "fmt.h"

/* To find out the actual length of the formatted value, pass a first
argument of (char *) 0. */

#define zero '0'
#define alow 'a'
#define plus '+'
#define minus '-'
#define xlow 'x'

unsigned int fmt_ulong(s,u) char *s; unsigned long u;
{
 unsigned int len; unsigned long q;
 len = 1; q = u;
 while (q > 9) { ++len; q /= 10; }
 if (s)
  {
   s += len;
   do { *--s = zero + (u % 10); u /= 10; } while(u); /* handles u == 0 */
  }
 return len;
}

unsigned int fmt_xlong(s,u) char *s; unsigned long u;
{
 unsigned int len; unsigned long q; unsigned long c;
 len = 1; q = u;
 while (q > 15) { ++len; q /= 16; }
 if (s)
  {
   s += len;
   do { c = u & 15; *--s = (c > 9 ? alow - 10 : zero) + c; u /= 16; } while(u);
  }
 return len;
}

unsigned int fmt_nbblong(s,n,base,bext,u)
char *s; unsigned int n; unsigned int base; unsigned int bext; unsigned long u;
/* I hope this meaning of n (min, not max) doesn't hurt anything. */
{
 unsigned int len; unsigned long q; unsigned long c;
 len = 1; q = u; bext += base;
 while (q > bext - 1) { ++len; q /= bext; } if (len < n) len = n;
 if (s)
  {
   s += len;
   do { c = u % bext; *--s = (c >= base ? alow - base : zero) + c; u /= bext; }
   while(u);
  }
 return len;
}

unsigned int fmt_ushort(s,u) char *s; unsigned short u;
{
 unsigned long l; l = u; return fmt_ulong(s,l);
}

unsigned int fmt_xshort(s,u) char *s; unsigned short u;
{
 unsigned long l; l = u; return fmt_xlong(s,l);
}

unsigned int fmt_nbbshort(s,n,base,bext,u)
char *s; unsigned int n; unsigned int base; unsigned int bext; unsigned short u;
{
 unsigned long l; l = u; return fmt_nbblong(s,n,base,bext,l);
}

unsigned int fmt_uint(s,u) char *s; unsigned int u;
{
 unsigned long l; l = u; return fmt_ulong(s,l);
}

unsigned int fmt_xint(s,u) char *s; unsigned int u;
{
 unsigned long l; l = u; return fmt_xlong(s,l);
}

unsigned int fmt_nbbint(s,n,base,bext,u)
char *s; unsigned int n; unsigned int base; unsigned int bext; unsigned int u;
{
 unsigned long l; l = u; return fmt_nbblong(s,n,base,bext,l);
}

unsigned int fmt_plusminus(s,sign) char *s; int sign;
{
 if (s) *s = ((sign < 0) ? minus : plus); return 1;
}

unsigned int fmt_minus(s,sign) char *s; int sign;
{
 if (sign > 0) return 0;
 if (s) *s = minus; return 1;
}

unsigned int fmt_0x(s,base) char *s; int base;
{
 if (base == 10) return 0;
 if (s) *s = zero; if (base == 8) return 1;
 if (s) s[1] = xlow; return 2;
}

unsigned int fmt_strncpy(s,t,n) char *s; char *t; unsigned int n;
{
 unsigned int len;
 len = 0;
 if (s) { while (s[len] = t[len]) if (++len == n) break; }
 else { while (t[len]) if (++len == n) break; }
 return len;
}

unsigned int fmt_memcpy(s,t,n) char *s; char *t; unsigned int n;
/* This and fmt_vis are the only functions where n == 0 means do nothing. */
{
 unsigned int len;
 if (s)
   for (len = 0;len < n;++len)
     s[len] = t[len];
 return n;
}

static unsigned int fmt_xvis(s,t,n,x) char *s; char *t; unsigned int n; int x;
{
 unsigned int len;
 for (len = 0;n;--n,++t)
  {
   int ch;
   ch = (int) (unsigned int) (unsigned char) *t;
   /* XXX: ASCII dependent! */
   if (ch > 127)
    { if (s) { s[len] = 'M'; s[len + 1] = '-'; } len += 2; ch -= 128; }
   if (((ch >= 32) && (ch <= 126)) || (ch == x))
    { if (s) s[len] = ch; ++len; continue; }
   if (s) s[len] = '^'; ++len;
   if (s) s[len] = 64 + (ch & 31) - 32 * (ch == 127); ++len;
  }
 return len;
}

unsigned int fmt_vis(s,t,n) char *s; char *t; unsigned int n;
{
 return fmt_xvis(s,t,n,-1);
}

unsigned int fmt_nvis(s,t,n) char *s; char *t; unsigned int n;
{
 return fmt_xvis(s,t,n,'\n');
}

/* invertible! */
unsigned int fmt_rvis(s,t,n) char *s; char *t; unsigned int n;
{
 unsigned int len;
 for (len = 0;n;--n,++t)
  {
   int ch;
   ch = (int) (unsigned int) (unsigned char) *t;
   /* XXX: ASCII dependent! */
   if ((ch >= 32) && (ch <= 126) && (ch != '^'))
    { if (s) s[len] = ch; ++len; continue; }
   if (ch == 127)
    { if (s) { s[len] = '^'; s[len + 1] = '?'; } len += 2; continue; }
   if (ch == '^')
    { if (s) { s[len] = '^'; s[len + 1] = ' '; } len += 2; continue; }
   if (ch == 10)
    { if (s) { s[len] = '^'; s[len + 1] = '$'; } len += 2; continue; }
   if ((ch >= 0) && (ch <= 31))
    { if (s) { s[len] = '^'; s[len + 1] = 64 + (ch & 31); } len += 2; continue; }
   if (s) { s[len] = '^'; s[len + 1] = 'x'; } len += 2;
   if (s) { s[len] = (ch >= 160) ? alow + ((ch/16) - 10) : zero + (ch/16); }
   ++len; ch = ch & 15;
   if (s) { s[len] = (ch >= 10) ? alow + (ch - 10) : zero + ch; }
   ++len;
  }
 s[len] = '\n';
 return len + 1;
}

unsigned int fmt_unrvis(s,t,n) char *s; char *t; unsigned int n;
{
 unsigned int len;
 for (len = 0;n;--n,++t)
  {
   if (*t == '\n')
     continue;
   if (*t != '^')
    {
     if (s) *s++ = *t; ++len;
     continue;
    }
   if (n < 2)
     return len;
   ++t; --n;
   if (*t == '?')
    { if (s) *s++ = 127; ++len; continue; }
   if (*t == ' ')
    { if (s) *s++ = '^'; ++len; continue; }
   if ((*t >= 64) && (*t <= 95))
    { if (s) *s++ = *t - 64; ++len; continue; }
   if (*t == '$')
    { if (s) *s++ = 10; ++len; continue; }
   if (n < 3)
     return len;
   if (*t != 'x')
     return len; /* XXX */
   ++t;
   if (s)
     if (*t < alow)
       *s = *t - zero;
     else
       *s = *t - alow + 10;
   if (s)
     *s <<= 4;
   ++t;
   if (s)
     if (*t < alow)
       *s += *t - zero;
     else
       *s += *t - alow + 10;
   if (s) ++s;
   ++len; n -= 2;
  }
 return len;
}
