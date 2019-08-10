#!/bin/sh

require_env(){
	env=`eval echo '$'"$1"`
	if test -z "$env"
	then
		echo >&2 "$0: need \$$1"
		exit 1
	fi
}
