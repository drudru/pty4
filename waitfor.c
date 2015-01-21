extern char *malloc();

main(argc,argv)
int argc;
char *argv[];
{
 int len;
 char *s;
 int pos;
 char ch;
 int f;
 int p;
 if (!argv[1])
   exit(1);
 len = strlen(argv[1]);
 if (!len)
   len = 1;
 if (!(s = malloc(len)))
   exit(2);
 pos = 0;
 f = 0;
 while (read(0,&ch,1) == 1)
  {
   if (write(2,&ch,1) != 1)
     exit(3);
   if (ch)
    {
     s[pos] = ch;
     ++pos;
     if (pos == len)
      {
       f = 1;
       pos = 0;
      }
     if (f && (ch == argv[1][len - 1]))
      {
       for (p = 1;s[(pos + p) % len] == argv[1][p];++p)
	 ;
       if (!argv[1][p])
	 exit(0);
      }
    }
  }
 exit(4);
}
