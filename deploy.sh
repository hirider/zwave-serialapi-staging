#!/bin/sh
#set -e

sudo sed -i 's/define DEV/define PROD/' src/SerialApiCustom/SerialapiProcess.h
docker exec zwave_prod /zgw/scripts/docker_build_prod.sh
cp build_prod/zipgateway-7.17.01-Linux-stretch-armhf.deb output/
mv output/zipgateway-7.17.01-Linux-stretch-armhf.deb output/zipgateway.deb

printf "\n"
normal=$(tput sgr0)
green=$(tput setaf 2)
bold=$(tput bold)
small=$(tput sgr0)

IP=192.168.0.85
SRC_ZIPPATH=build_prod
SRC_ZWGPATH=/home/aura/dev/project-geisha/module-agents/zwave-agent
DES_ZIPPATH=/home/leo/Serial-gateway/
DES_ZWGPATH=/home/leo/Engine/zwave-agent/

install_zgw(){
	ssh leo@$IP 'sudo dpkg -i /home/leo/Serial-gateway/zipgateway-7.17.01-Linux-stretch-armhf.deb'
	ssh leo@$IP 'sudo chmod +x /var/lib/dpkg/info/zipgateway.config'
	ssh leo@$IP 'sudo dpkg --configure -a'

	printf "${green}${bold}configuring RF region in zipgateway.cfg..${normal}${small}\n"
	ssh leo@$IP 'sudo sed -i 's/ZWRFRegion=0x00/ZWRFRegion=0x05/' /usr/local/etc/zipgateway.cfg'

	printf "${green}${bold}restarting leo service..\n${normal}${small}\n"
	ssh leo@$IP 'sudo systemctl restart leo.service'

	printf "${green}${bold}Installation Complete!\n${normal}${small}\n"
}

validate(){
	STATUS="$(ssh leo@$IP dpkg-query -W -f='\${Status}' zipgateway)"
	if [ "${STATUS}" = "install ok installed" ]; then
		printf "${green}${bold}\nold version of zipgateway found!${normal}${small}\n"
		printf "${green}${bold}purging old zipgateway...${normal}${small}\n"
		ssh leo@$IP 'sudo apt-get purge zipgateway'
		ssh leo@$IP 'sudo systemctl daemon-reload'

		printf "${green}${bold}\n\ninstalling new zipgateway...${normal}${small}\n"

		install_zgw
	else
		printf "${green}${bold}\nno previous version of zipgateway found ${normal}${small}\n"

		install_zgw
	fi
}

while true; do

read -p "${green}${bold}Do you wish to build and deploy zwave_gear too? (y/n)?${normal}${small}" yn

case $yn in
	[yY] ) 	printf "${green}${bold}\nbuilding zwave gear${normal}${small}\n"

			make clean -C /home/aura/dev/project-geisha/module-agents/zwave-agent
			make all -C /home/aura/dev/project-geisha/module-agents/zwave-agent

			printf "${green}${bold}\n\nstopping leo service${normal}${small}\n"
			ssh leo@$IP 'sudo systemctl stop leo.service'

			printf "${green}${bold}deploying files ......${normal}${small}\n"
			scp $SRC_ZIPPATH/zipgateway-7.17.01-Linux-stretch-armhf.deb leo@$IP:$DES_ZIPPATH
			scp $SRC_ZWGPATH/zwave_gear leo@$IP:$DES_ZWGPATH

			printf "${green}${bold}\n\nInstalling Zipgateway..${normal}${small}\n"

			validate

		break;;

	[nN] )  printf "${green}${bold}\nignoring zwave_gear${normal}${small}\n"

			printf "${green}${bold}\n\nstopping leo service${normal}${small}\n"
			ssh leo@$IP 'sudo systemctl stop leo.service'

			printf "${green}${bold}deploying files ....${normal}${small}\n"
	        scp $SRC_ZIPPATH/zipgateway-7.17.01-Linux-stretch-armhf.deb leo@$IP:$DES_ZIPPATH

	        printf "${green}${bold}\n\nInstalling Zipgateway..${normal}${small}\n"

			validate

		exit;;
	* ) echo invalid response;;
esac

done
