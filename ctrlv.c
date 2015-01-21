#include "ptymisc.h"

static char buf[64];
static char outbuf[1000];

main()
{
 int r;
 int i;

 while ((r = read(0,buf,sizeof(buf))) > 0)
  {
   for (i = 0;i < r;++i)
    {
     outbuf[2 * i] = 22;
     outbuf[2 * i + 1] = buf[i];
    }
   outbuf[2 * r] = 4;
   bwrite(1,outbuf,2 * r + 1);
  }
 for (i = 0;i < 30;++i)
   outbuf[i] = 4;
 bwrite(1,outbuf,i);
 exit(0);
}
