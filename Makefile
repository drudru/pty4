CC=cc -s
OPTMZ=-O
# OPTMZ is used below for time-critical modules
CPP=cc -E
# alternative: CPP=/lib/cpp <
PTYLIBS=

all: pty clones sessstuff ttymisc misc

target-list:
	@echo 'target-list all clones sessstuff ttymisc misc INSTALL CHECKCONF shar clean lint'

clones: biff mesg wall write lock script users tty who whoami

sessstuff: sess sesslist sessname sessmenu disconnect reconnect sesswho sesswhere sesskill

ttymisc: tiocsti condom ttyprotect utmpinit ttydetach checkptys exclon excloff wtmprotate sessrotate sclogrotate sessnowinit scnowinit

misc: tscript script.tidy waitfor argv0 ctrlv nobuf tplay trecord

INSTALL: INSTALL.c
	$(CC) -o INSTALL INSTALL.c

CHECKCONF: CHECKCONF.c
	$(CC) -o CHECKCONF CHECKCONF.c

sessnowinit: sessnowinit.sh
	$(CPP) sessnowinit.sh | tail -2 | tr -d '"' | sed 's/^X/#/' > sessnowinit
	chmod 700 sessnowinit

scnowinit: scnowinit.sh
	$(CPP) scnowinit.sh | tail -2 | tr -d '"' | sed 's/^X/#/' > scnowinit
	chmod 700 scnowinit

sessrotate: sessrotate.sh
	$(CPP) sessrotate.sh | tail -12 | tr -d '"' | sed 's/^X/#/' > sessrotate
	chmod 700 sessrotate

sclogrotate: sclogrotate.sh
	$(CPP) sclogrotate.sh | tail -12 | tr -d '"' | sed 's/^X/#/' > sclogrotate
	chmod 700 sclogrotate

wtmprotate: wtmprotate.sh
	$(CPP) wtmprotate.sh | tail -12 | tr -d '"' | sed 's/^X/#/' > wtmprotate
	chmod 700 wtmprotate

nobuf: nobuf.sh
	cp nobuf.sh nobuf
	chmod 700 nobuf

script.tidy: script.tidy.sh
	tr '83' '\010\015' < script.tidy.sh > script.tidy
	chmod 700 script.tidy

sess: sess.sh
	cp sess.sh sess
	chmod 700 sess

tscript: tscript.sh
	cp tscript.sh tscript
	chmod 700 tscript

script: script.sh
	cp script.sh script
	chmod 700 script

ttyprotect: ttyprotect.sh
	cp ttyprotect.sh ttyprotect
	chmod 700 ttyprotect

condom: ttyprotect.sh
	cp ttyprotect.sh condom
	chmod 700 condom

sesskill: sesskill.o ptycomm.o getopt.o env.o ralloc.o ptymisc.o fmt.o scan.o
	$(CC) -o sesskill sesskill.o ptycomm.o getopt.o env.o ralloc.o ptymisc.o fmt.o scan.o

tplay: tplay.o sigsched.o timer.o ralloc.o
	$(CC) -o tplay tplay.o sigsched.o timer.o ralloc.o

trecord: trecord.o sigsched.o timer.o ralloc.o
	$(CC) -o trecord trecord.o sigsched.o timer.o ralloc.o

ctrlv: ctrlv.o ptymisc.o scan.o
	$(CC) -o ctrlv ctrlv.o ptymisc.o scan.o

checkptys: checkptys.o
	$(CC) -o checkptys checkptys.o

ttydetach: ttydetach.o env.o ralloc.o
	$(CC) -o ttydetach ttydetach.o env.o ralloc.o

excloff: excloff.o
	$(CC) -o excloff excloff.o

exclon: exclon.o
	$(CC) -o exclon exclon.o

argv0: argv0.o
	$(CC) -o argv0 argv0.o

waitfor: waitfor.o
	$(CC) -o waitfor waitfor.o

tty: tty.o getopt.o
	$(CC) -o tty tty.o getopt.o

utmpinit: utmpinit.o ptymisc.o scan.o
	$(CC) -o utmpinit utmpinit.o ptymisc.o scan.o

who: who.o getopt.o fmt.o
	$(CC) -o who who.o getopt.o fmt.o

whoami: whoami.o username.o fmt.o scan.o
	$(CC) -o whoami whoami.o username.o fmt.o scan.o

users: users.o radixsort.o ralloc.o
	$(CC) -o users users.o radixsort.o ralloc.o

lock: lock.o
	$(CC) -o lock lock.o -lcurses

sesswhere: sesswhere.o sessconnlog.o getopt.o fmt.o ptymisc.o scan.o
	$(CC) -o sesswhere sesswhere.o sessconnlog.o getopt.o fmt.o ptymisc.o scan.o

sesswho: sesswho.o sesslog.o getopt.o ptymisc.o scan.o
	$(CC) -o sesswho sesswho.o sesslog.o getopt.o ptymisc.o scan.o

sesslist: sesslist.o ptycomm.o fmt.o getopt.o ptymisc.o scan.o
	$(CC) -o sesslist sesslist.o ptycomm.o fmt.o getopt.o ptymisc.o scan.o

disconnect: disconnect.o ptycomm.o getopt.o env.o ralloc.o ptymisc.o fmt.o scan.o
	$(CC) -o disconnect disconnect.o ptycomm.o getopt.o env.o ralloc.o ptymisc.o fmt.o scan.o

reconnect: reconnect.o ptycomm.o getopt.o env.o ralloc.o ptymisc.o fmt.o scan.o
	$(CC) -o reconnect reconnect.o ptycomm.o getopt.o env.o ralloc.o ptymisc.o fmt.o scan.o

sessmenu: sessmenu.o
	$(CC) -o sessmenu sessmenu.o

sessname: sessname.o ptycomm.o getopt.o env.o ralloc.o ptymisc.o fmt.o scan.o
	$(CC) -o sessname sessname.o ptycomm.o getopt.o env.o ralloc.o ptymisc.o fmt.o scan.o

biff: biff.o
	$(CC) -o biff biff.o

mesg: mesg.o
	$(CC) -o mesg mesg.o

tiocsti: tiocsti.o
	$(CC) -o tiocsti tiocsti.o

write: write.o fmt.o
	$(CC) -o write write.o fmt.o

wall: wall.o fmt.o
	$(CC) -o wall wall.o fmt.o

pty: env.o fmt.o getoptquiet.o ptycomm.o ptyerr.o ptyget.o ptylogs.o ptymain.o ptymaster.o ptymisc.o ptysecure.o ptysigler.o ptyslave.o ptytexts.o ptytty.o ralloc.o scan.o sessconnlog.o sesslog.o sigdfl.o sigsched.o username.o
	$(CC) -o pty env.o fmt.o getoptquiet.o ptycomm.o ptyerr.o ptyget.o ptylogs.o ptymain.o ptymaster.o ptymisc.o ptysecure.o ptysigler.o ptyslave.o ptytexts.o ptytty.o ralloc.o scan.o sessconnlog.o sesslog.o sigdfl.o sigsched.o username.o $(PTYLIBS)

env.o: env.c
	$(CC) -c env.c

fmt.o: fmt.c
	$(CC) -c fmt.c

getoptquiet.o: getoptquiet.c
	$(CC) -c getoptquiet.c

getopt.o: getopt.c
	$(CC) -c getopt.c

ptycomm.o: ptycomm.c
	$(CC) -c ptycomm.c

ptyerr.o: ptyerr.c
	$(CC) -c ptyerr.c

ptyget.o: ptyget.c
	$(CC) -c ptyget.c

ptylogs.o: ptylogs.c
	$(CC) -c ptylogs.c

ptymain.o: ptymain.c
	$(CC) -c ptymain.c

ptymaster.o: ptymaster.c
	$(CC) -c ptymaster.c

ptymisc.o: ptymisc.c
	$(CC) -c ptymisc.c

ptysecure.o: ptysecure.c
	$(CC) -c ptysecure.c

ptysigler.o: ptysigler.c
	$(CC) -c ptysigler.c

ptyslave.o: ptyslave.c
	$(CC) -c ptyslave.c

ptytexts.o: ptytexts.c
	$(CC) -c ptytexts.c

ptytty.o: ptytty.c
	$(CC) -c ptytty.c

ralloc.o: ralloc.c
	$(CC) -c ralloc.c

scan.o: scan.c
	$(CC) -c scan.c

sessconnlog.o: sessconnlog.c
	$(CC) -c sessconnlog.c

sesslog.o: sesslog.c
	$(CC) -c sesslog.c

sigdfl.o: sigdfl.c
	$(CC) -c sigdfl.c

sigsched.o: sigsched.c
	$(CC) -c sigsched.c

username.o: username.c
	$(CC) -c username.c

write.o: write.c
	$(CC) -c write.c

wall.o: wall.c
	$(CC) -c wall.c

tiocsti.o: tiocsti.c
	$(CC) -c tiocsti.c

biff.o: biff.c
	$(CC) -c biff.c

mesg.o: mesg.c
	$(CC) -c mesg.c

sesslist.o: sesslist.c
	$(CC) -c sesslist.c

sessname.o: sessname.c
	$(CC) -c sessname.c

sessmenu.o: sessmenu.c
	$(CC) -c sessmenu.c

disconnect.o: disconnect.c
	$(CC) -c disconnect.c

reconnect.o: reconnect.c
	$(CC) -c reconnect.c

sesswho.o: sesswho.c
	$(CC) -c sesswho.c

sesswhere.o: sesswhere.c
	$(CC) -c sesswhere.c

lock.o: lock.c
	$(CC) -c lock.c

users.o: users.c
	$(CC) -c users.c

tty.o: tty.c
	$(CC) -c tty.c

utmpinit.o: utmpinit.c
	$(CC) -c utmpinit.c

who.o: who.c
	$(CC) -c who.c

whoami.o: whoami.c
	$(CC) -c whoami.c

sesskill.o: sesskill.c
	$(CC) -c sesskill.c

waitfor.o: waitfor.c
	$(CC) -c waitfor.c

ctrlv.o: ctrlv.c
	$(CC) -c ctrlv.c

ttydetach.o: ttydetach.c
	$(CC) -c ttydetach.c

checkptys.o: checkptys.c
	$(CC) -c checkptys.c

excloff.o: excloff.c
	$(CC) -c excloff.c

exclon.o: exclon.c
	$(CC) -c exclon.c

argv0.o: argv0.c
	$(CC) -c argv0.c

radixsort.o: radixsort.c
	$(CC) $(OPTMZ) -c radixsort.c

tplay.o: tplay.c
	$(CC) -c tplay.c

trecord.o: trecord.c
	$(CC) -c trecord.c

timer.o: timer.c
	$(CC) -c timer.c

shar: FILES
	shar `cat FILES` > pty.shar
	chmod 400 pty.shar

clean: OBJECTS BINARIES
	rm -f `cat OBJECTS BINARIES`

lint:
	lint -haxc pty*.c getopt.c ralloc.c env.c fmt.c scan.c sessconnlog.c sesslog.c sigdfl.c username.c sigsched.c

# automatically generated past this

CHECKCONF: config/fdsettrouble.h config/genericptr.h config/devmty.h
CHECKCONF: config/devsty.h config/posix.h config/ptybin.h config/ptydir.h
CHECKCONF: config/ptyext.h config/ptygroup.h config/ptylongname.h
CHECKCONF: config/ptymodes.h config/ptyopts.h config/ttyopts.h
CHECKCONF: config/sessconnfile.h config/sessfile.h
CHECKCONF: config/utmpfile.h config/wtmpfile.h
INSTALL: config/ptybin.h config/ptydir.h
INSTALL: config/ptygroup.h config/sessconnfile.h config/sessfile.h
INSTALL: config/utmpfile.h config/wtmpfile.h
INSTALL: config/ptygroup.h
checkptys.o: config/devmty.h config/devsty.h config/posix.h config/ptybin.h
checkptys.o: config/ptydir.h config/ptyext.h config/ptygroup.h
checkptys.o: config/ptymodes.h config/ptyopts.h config/sessconnfile.h
checkptys.o: config/sessfile.h config/utmpfile.h
checkptys.o: config/wtmpfile.h
ctrlv.o: ptymisc.h
disconnect.o: ptymisc.h ptycomm.h
disconnect.o: config/ptydir.h getopt.h env.h
env.o: env.h ralloc.h
fmt.o: fmt.h
getopt.o: getopt.h
getoptquiet.o: getoptquiet.h
ptycomm.o: fmt.h config/ptyext.h ptycomm.h ptymisc.h
ptyerr.o: getoptquiet.h fmt.h ptyerr.h ralloc.h
ptyget.o: ptyget.h
ptyget.o: config/ptyext.h config/devmty.h config/devsty.h ptysecure.h
ptylogs.o: fmt.h config/utmpfile.h config/wtmpfile.h
ptylogs.o: ptymisc.h ptylogs.h config/sysv.h
ptymain.o: sigsched.h getoptquiet.h ralloc.h env.h fmt.h config/ptydir.h
ptymain.o: config/ttyopts.h config/ptyopts.h config/posix.h ptyget.h ptytty.h
ptymain.o: config/ttyopts.h ptylogs.h ptymisc.h
ptymain.o: ptytexts.h ptycomm.h ptymaster.h ptysigler.h ptyerr.h ptyslave.h
ptymain.o: sesslog.h sessconnlog.h
ptymaster.o: sigsched.h ralloc.h
ptymaster.o: config/ttyopts.h config/ptylongname.h sesslog.h sessconnlog.h
ptymaster.o: ptyget.h ptymisc.h ptyerr.h ptytty.h
ptymaster.o: config/ttyopts.h ptycomm.h ptymaster.h
ptymisc.o: ptymisc.h config/fdsettrouble.h
ptysecure.o: ptysecure.h ptytty.h
ptysecure.o: config/ttyopts.h config/ptymodes.h config/ptygroup.h
ptysigler.o: sigsched.h
ptysigler.o: sigdfl.h fmt.h sessconnlog.h ptytty.h
ptysigler.o: config/ttyopts.h ptycomm.h ptymisc.h ptyerr.h config/ttyopts.h
ptysigler.o: ptysigler.h
ptyslave.o: fmt.h ptyerr.h
ptyslave.o: ptyslave.h ralloc.h
ptytexts.o: ptytexts.h
ptytty.o: config/genericptr.h
ptytty.o: config/ttyopts.h ptytty.h config/ttyopts.h
ralloc.o: ralloc.h sod.h
reconnect.o: ptymisc.h ptycomm.h
reconnect.o: config/ptydir.h getopt.h env.h
scan.o: scan.h
sessconnlog.o: sessconnlog.h ptymisc.h
sessconnlog.o: config/sessconnfile.h config/ptyext.h
sesskill.o: ptycomm.h config/ptydir.h getopt.h
sesskill.o: env.h
sesslist.o: config/ptydir.h
sesslist.o: config/ptylongname.h fmt.h scan.h getopt.h ptycomm.h
sesslog.o: sesslog.h
sesslog.o: ptymisc.h config/sessfile.h config/ptyext.h
sessname.o: ptycomm.h config/ptydir.h
sessname.o: config/ptylongname.h getopt.h env.h
sesswhere.o: fmt.h getopt.h sessconnlog.h
sesswhere.o: config/sessconnfile.h
sesswho.o: getopt.h sesslog.h config/sessfile.h
sigdfl.o: sigdfl.h
sigsched.o: config/fdsettrouble.h sigsched.h
sigsched.o: ralloc.h sod.h
timer.o: sigsched.h sod.h
timer.o: timer.h sigsched.h ralloc.h
tplay.o: sigsched.h timer.h sigsched.h
trecord.o: timer.h sigsched.h
tty.o: getopt.h
ttydetach.o: env.h config/posix.h
username.o: username.h fmt.h scan.h
users.o: config/utmpfile.h
users.o: radixsort.h sod.h ralloc.h
utmpinit.o: config/utmpfile.h
utmpinit.o: ptymisc.h
wall.o: fmt.h config/utmpfile.h
who.o: config/utmpfile.h getopt.h fmt.h
whoami.o: username.h
write.o: config/utmpfile.h fmt.h
