#!/bin/sh
# XXX: propagate stops!
# Public domain.
case "$@" in
"") set tapescript ${1+"$@"} ;;
"-a") set tapescript ${1+"$@"} ;;
esac
echo "Tape started, teeing $@"
{ echo 'Tape started on '`date`;
  pty -s "${SHELL-/bin/sh}";
  echo 'Tape done on '`date`
} | trecord | tee ${1+"$@"} | tplay
