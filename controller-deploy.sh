#!/bin/sh

green=$(tput setaf 2)
bold=$(tput bold)
normal=$(tput sgr0)
small=$(tput sgr0)

read -p "Enter Branch : " BRANCH
read -p "Enter Gitlab access token : " TOKEN

printf "${green}${bold}Downloading deb file from branch $BRANCH ${normal}${small}\n"
curl --header "PRIVATE-TOKEN: $TOKEN" "https://gitlab.com/api/v4/projects/36595523/repository/files/output%2Fzipgateway%2Edeb/raw?ref=$BRANCH" --output zipgateway.deb

printf "${green}${bold}Stopping leo service${normal}${small}\n"
sudo systemctl stop leo.service

install_zgw(){
    sudo apt-get purge zipgateway
    sudo systemctl daemon-reload
    sudo dpkg -i zipgateway.deb
    sudo chmod +x /var/lib/dpkg/info/zipgateway.config
    sudo dpkg --configure -a

    printf "${green}${bold}configuring RF region in zipgateway.cfg..${normal}${small}\n"
    sudo sed -i 's/ZWRFRegion=0x00/ZWRFRegion=0x05/' /usr/local/etc/zipgateway.cfg

    printf "${green}${bold}Restarting leo service${normal}${small}\n"
    sudo systemctl restart leo.service
}

install_zgw
