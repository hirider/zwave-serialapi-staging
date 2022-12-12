#!/bin/sh
#set -e

sudo sed -i 's/define DEV/define PROD/' src/SerialApiCustom/SerialapiProcess.h
rm output/zipgateway.deb
docker container restart zwave_prod
docker exec zwave_prod /zgw/scripts/docker_build_prod.sh
cp build_prod/zipgateway-7.17.01-Linux-stretch-armhf.deb output/
mv output/zipgateway-7.17.01-Linux-stretch-armhf.deb output/zipgateway.deb
git add output/zipgateway.deb
