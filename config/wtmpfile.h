#ifndef CONFIG_WTMPFILE_H
#define CONFIG_WTMPFILE_H

/* XXX: we should #include <utmp.h> here, but it's usually not protected,
so we have to pass on to the user the burden of including it first. */

#ifndef WTMP_FILE
#ifdef _PATH_WTMP
#define WTMP_FILE _PATH_WTMP
#else
#define WTMP_FILE "/usr/adm/wtmp"
#endif
#endif

#endif
