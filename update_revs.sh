#!/bin/bash

if [ $# -lt 2 ]; then
	echo "Usage: update_revs <script> <file>";
	exit 1
fi

python $1 -o $2 
