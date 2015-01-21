#!/bin/sh
ctrlv | pty -0dpCR8 -xf ${1+"$@"}
