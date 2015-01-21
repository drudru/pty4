/* Public domain. */
#include <sys/ioctl.h>

main()
{
 (void) ioctl(1,(unsigned long) TIOCNXCL,(char *) 0);
}
