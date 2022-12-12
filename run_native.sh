#!/bin/sh
set -e
while true; do

read -p "${green}${bold}1) Native build 2) Docker build. Option : ${normal}${small}" op

case $op in 
    [1] )   cd build_native
            sudo sed -i 's/define PROD/define DEV/' /home/aura/dev/SerialGateway/src/SerialApiCustom/SerialapiProcess.h
            sudo make all -j16
            cd src
            sudo ./zipgateway
        break;;

    [2] )   sudo sed -i 's/define PROD/define DEV/' /home/aura/dev/SerialGateway/src/SerialApiCustom/SerialapiProcess.h
            docker exec zwave_dev /zgw/scripts/docker_build_native.sh
            cd build_native_docker/src/
            sudo ./zipgateway
        break;;
esac
done
