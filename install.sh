#!/bin/sh

## INSTALL GUIDE

user=$USER
home=$HOME

sudo su << EOSU
apt install -y cmake gcc-9 libboost-all-dev rsync linux-tools-common google-perftools
mkdir -p /usr/include/nlohmann
curl https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp | tee /usr/include/nlohmann/json.hpp >/dev/null
EOSU

mkdir -p $home/.ssh
chmod 700 $home/.ssh
cp -a .ssh/id_rsa_pi* $home/.ssh/
chmod 600 $home/.ssh/id_rsa_pi
cat .ssh/config | tee -a  $home/.ssh/config >/dev/null


