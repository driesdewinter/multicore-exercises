#!/bin/bash

ssh pi sudo date -s "$(date -Ins)"

cd $(dirname "${BASH_SOURCE[0]}")
excludeflags=$(for f in $(cat .gitignore) .ssh .git* sync-pi.sh; do echo --exclude $f; done) 
rsync -avz --delete $excludeflags . pi:multicore-exercises

