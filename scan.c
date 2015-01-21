/* scan.c, scan.h: scanning library
Daniel J. Bernstein, brnstnd@nyu.edu.
No dependencies.
No environment requirements.
7/18/91: Baseline. scan 1.0, public domain.
No known patent problems.

XXX: still need floating-point scanning

*/

#include "scan.h"

/* just to keep track of what special characters we're using */
#define zero '0'
#define plus '+'
#define minus '-'
#define alow 'a'
#define acap 'A'
#define space ' '
#define tab '\t'
#define xlow 'x'
#define xcap 'x'

/* Note that the digits here are defined as '0', '0' + 1, '0' + 2, etc. */
/* The letters are defined similarly, starting from 'a' and 'A'. */
/* This may produce unintuitive results with a weird character set. */

unsigned int scan_plusminus(s,sign) char *s; int *sign;
{
 if (*s == plus) { *sign = 1; return 1; }
 if (*s == minus) { *sign = -1; return 1; }
 *sign = 1; return 0;
}

unsigned int scan_0x(s,base) char *s; unsigned int *base;
{
 if (*s == zero)
  {
   if ((s[1] == xlow) || (s[1] == xcap))
    { *base = 16; return 2; }
   *base = 8; return 1;
  }
 *base = 10; return 0;
}

unsigned int scan_ulong(s,u) char *s; unsigned long *u;
{
 unsigned int pos; unsigned long result; unsigned long c;
 pos = 0; result = 0;
 while ((c = (unsigned long) (unsigned char) (s[pos] - zero)) < 10)
  { result = result * 10 + c; ++pos; }
 *u = result; return pos;
}

unsigned int scan_xlong(s,u) char *s; unsigned long *u;
{
 unsigned int pos; unsigned long result; unsigned long c;
 pos = 0; result = 0;
 while (((c = (unsigned long) (unsigned char) (s[pos] - zero)) < 10)
      ||(((c = (unsigned long) (unsigned char) (s[pos] - alow)) < 6)
       &&(c = c + 10))
      ||(((c = (unsigned long) (unsigned char) (s[pos] - acap)) < 6)
       &&(c = c + 10))
       ) /* XXX: this gets the job done */
  { result = result * 16 + c; ++pos; }
 *u = result; return pos;
}

unsigned int scan_nbblong(s,n,base,bext,u)
char *s; unsigned int n; unsigned int base; unsigned int bext; unsigned long *u;
/* Note that n == 0 means scan forever. Hopefully this is a good choice. */
{
 unsigned int pos; unsigned long result; unsigned long c;
 pos = 0; result = 0;
 while (((c = (unsigned long) (unsigned char) (s[pos] - zero)) < base)
      ||(((c = (unsigned long) (unsigned char) (s[pos] - alow)) < bext)
       &&(c = c + base))
      ||(((c = (unsigned long) (unsigned char) (s[pos] - acap)) < bext)
       &&(c = c + base))
       ) /* XXX: this gets the job done */
  { result = result * (base + bext) + c; ++pos; if (pos == n) break; }
 *u = result; return pos;
}

unsigned int scan_uint(s,u) char *s; unsigned int *u;
{
 unsigned int pos; unsigned long result;
 pos = scan_ulong(s,&result);
 *u = result; return pos;
}

unsigned int scan_xint(s,u) char *s; unsigned int *u;
{
 unsigned int pos; unsigned long result;
 pos = scan_xlong(s,&result);
 *u = result; return pos;
}

unsigned int scan_nbbint(s,n,base,bext,u)
char *s; unsigned int n; unsigned int base; unsigned int bext; unsigned int *u;
{
 unsigned int pos; unsigned long result;
 pos = scan_nbblong(s,n,base,bext,&result);
 *u = result; return pos;
}

unsigned int scan_ushort(s,u) char *s; unsigned short *u;
{
 unsigned int pos; unsigned long result;
 pos = scan_ulong(s,&result);
 *u = result; return pos;
}

unsigned int scan_xshort(s,u) char *s; unsigned short *u;
{
 unsigned int pos; unsigned long result;
 pos = scan_xlong(s,&result);
 *u = result; return pos;
}

unsigned int scan_nbbshort(s,n,base,bext,u)
char *s; unsigned int n; unsigned int base; unsigned int bext; unsigned short *u;
{
 unsigned int pos; unsigned long result;
 pos = scan_nbblong(s,n,base,bext,&result);
 *u = result; return pos;
}

unsigned int scan_charsetnskip(s,chars,n) char *s; char *chars; unsigned int n;
{
 unsigned int pos;
 pos = 0;
 while (chars[s[pos]]) /* user's responsibility to check for null */
   if (++pos == n)
     break;
 return pos;
}

unsigned int scan_noncharsetnskip(s,chars,n) char *s; char *chars; unsigned int n;
{
 unsigned int pos;
 pos = 0;
 while (!chars[s[pos]]) /* again, user's responsibility to check for null */
   if (++pos == n)
     break;
 return pos;
}

unsigned int scan_whitenskip(s,n) char *s; unsigned int n;
{
 unsigned int pos; char c;
 pos = 0;
 while (((c = s[pos]) == space) || (c == tab)) /* XXX: this is slow */
   if (++pos == n)
     break;
 return pos;
}

unsigned int scan_nonwhitenskip(s,n) char *s; unsigned int n;
{
 unsigned int pos; char c;
 pos = 0;
 /* This is the only function without ``str'' in its name where we
    check specially for nulls. */
 while ((c = s[pos]) && (c != space) && (c != tab)) /* XXX: this is slow */
   if (++pos == n)
     break;
 return pos;
}

unsigned int scan_strncmp(s,t,n) char *s; char *t; unsigned int n;
{
 unsigned int pos; char c;
 pos = 0;
 while ((c = s[pos]) && (c == t[pos]))
   if (++pos == n)
     break;
 return pos;
}

unsigned int scan_memcmp(s,t,n) char *s; char *t; unsigned int n;
/* This is the only function where n == 0 means do nothing. */
{
 unsigned int pos;
 pos = 0;
 while (n) if (s[pos] != t[pos]) break; else { --n; ++pos; }
 return pos;
}
