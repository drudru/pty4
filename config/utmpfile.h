#ifndef CONFIG_UTMPFILE_H
#define CONFIG_UTMPFILE_H

/* XXX: we should #include <utmp.h> here, but it's usually not protected,
so we have to pass on to the user the burden of including it first. */

#ifndef UTMP_FILE
#ifdef _PATH_UTMP
#define UTMP_FILE _PATH_UTMP
#else
#define UTMP_FILE "/etc/utmp"
#endif
#endif

#endif
