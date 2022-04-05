#!/bin/bash
make Debug
if [ "$?" != "0" ]; then
	exit -1
fi

sudo ln -sf "$(readlink -f .bin/Debug/udjat-module-sqlite.so)" "/usr/lib64/udjat-modules/1.0"
if [ "$?" != "0" ]; then
	exit -1
fi

