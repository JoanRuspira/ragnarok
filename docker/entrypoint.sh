#!/bin/bash

# generate npcorchestra files
cd /ragnarok/npc/npcorchestra
python3 npcorchestra.py all
cd ../../

# start mysql service
service mysql start

# build server
chmod +x /ragnarok/configure
/ragnarok/configure --enable-packetver=20200401
# make clean
make server

# start server
chmod +x /ragnarok/athena-start
/ragnarok/athena-start start

# keep container alive
sleep infinity