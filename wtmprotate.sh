WARNING: Do not change this file without changing Makefile accordingly!
#include <utmp.h>
#include "config/wtmpfile.h"
X!/bin/sh
X if you want to save WTMP_FILE.7, do it now!
mv WTMP_FILE.6 WTMP_FILE.7
mv WTMP_FILE.5 WTMP_FILE.6
mv WTMP_FILE.4 WTMP_FILE.5
mv WTMP_FILE.3 WTMP_FILE.4
mv WTMP_FILE.2 WTMP_FILE.3
mv WTMP_FILE.1 WTMP_FILE.2
mv WTMP_FILE.0 WTMP_FILE.1
ln WTMP_FILE WTMP_FILE.0
cp WTMP_FILE WTMP_FILE.new; : > WTMP_FILE.new
chmod 644 WTMP_FILE.new; mv WTMP_FILE.new WTMP_FILE
