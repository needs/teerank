#!/bin/sh
#
# This file is included to the beginning of each script file
# It sets all the necessary environment variables, it must have the same
# logic than load_config() in src/config.c .
#
: ${TEERANK_ROOT:=.teerank}
: ${TEERANK_CACHE_ROOT:=.}
: ${TEERANK_TMP_ROOT:=$TEERANK_CACHE_ROOT}
: ${TEERANK_VERBOSE:=0}

export TEERANK_ROOT
export TEERANK_CACHE_ROOT
export TEERANK_TMP_ROOT
