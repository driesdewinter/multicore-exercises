#!/bin/bash

ssh pi sudo date -s "$(date -Ins)"
rsync -avz --delete --exclude build "$(dirname "${BASH_SOURCE[0]}")" pi:multicore-exercises

