#!/bin/sh
# Public domain.
exec sed 's/3$//
:x
s/[^8]8//
t x'
