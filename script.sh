#!/bin/sh
# XXX: propagate stops!
# Public domain.
case "$@" in
"") set typescript ${1+"$@"} ;;
"-a") set typescript ${1+"$@"} ;;
esac
echo "Script started, teeing $@"
{ echo 'Script started on '`date`;
  pty -s "${SHELL-/bin/sh}";
  echo 'Script done on '`date`
} | tee ${1+"$@"}
