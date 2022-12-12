# SerialGateway

### Table of Contents
  <ol>
    <li>
      <a href="#about-the-project">About The Project</a>
    </li>
    <li>
      <a href="#build-process">Build Process</a>
      <ul>
        <li><a href="#Dev/Testing-build-using-local-environment">Dev/Testing build using local environment</a></li>
        <li><a href="#Dev/Testing-build-using-Docker-container-env">Dev/Testing build using Docker container env</a></li>
      </ul>
    </li>
    <li><a href="#Deployment-and-Debugging">Deployment and Debugging</a></li>
    <ul>
      <li><a href="#Production-build-using-Docker-(Manual)">Production build using Docker (Manual)</a></li>
      <li><a href="#Manual-Deployment">Manual Deployment</a></li>
      <li><a href="#Production-build-using-Docker-(Script)">Production build using Docker (Script)</a></li>
    </ul>
    <li><a href="#usage-dev-environment">Usage</a></li>
    <li><a href="#faq">FAQ</a></li>
    <li><a href="#SerialGateway-Architecture">SerialGateway Architecture</a></li>
  </ol>

## About the project

## Build Process
 Serial gateway can be built in multiple ways depending on the CPU architecture. Sillicon labs have multiple docker images which support linux/amd64, arm architecture which can be found <a href = "https://hub.docker.com/u/zwave">here.</a> Docker pre built images make it easier to build and test the project, but it can be built independently following <a href = "http://118.24.72.133/">these</a> steps

### Installation

```sh
$ sudo apt-get install -y doxygen graphviz mscgen roffit perl git python3 cmake\
                           gcc xsltproc bison flex gcc-9-multilib \
                          pkg-config:i386 libssl-dev:i386 libc6-dev:i386 \
                          libusb-1.0-0-dev:i386 libjson-c-dev:i386 \
                          openjdk-8-jre curl g++-9-multilib libstdc++-9-dev
```

```sh
$ sudo apt-get install cmake gcc libssl-dev libssl-dev:i386 libusb-1.0-0-dev:i386 libusb-1.0-0-dev libc6-dev-i386 bison:i386 flex:i386
```

```sh
$ cp files/zipgateway.cfg /usr/local/etc/
```
<hr style="border:2px solid gray">

### Dev/Testing build using local environment

```sh
$ mkdir build_native
```
```sh
$ cd build_native
```
```sh
$ cmake ..
```
```sh
$ sudo make all [-j number of threads available for build]
```
```sh
$ cd src
```
```sh
$ sudo ./zipgateway
```
### - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - OR - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

### Dev/Testing build using Docker container env

```sh
$ docker run -d -v [Source path]:/zgw -it zwave/zgw_i386_ubuntu:18.04
```
```sh
$ docker container rename xxx zwave_dev
```
```sh
$ ./run_native.sh
```
<hr style="border:2px solid gray">

### Usage (dev Environment)

```sh
$ ./zipgateway
```
```sh
$ cd socket_tester
```
```sh
$ make all
```
```sh
$ ./client
```
<hr style="border:2px solid gray">

###  Pre-commit script (should only be done once)

Before commiting someting and pushing the code to remote, a pre-commit script is executed which will build the code to the newest update and store it in output/ directory.

For this to work the below steps need to be followed

```sh
pip install pre-commit
```

```sh
pre-commit install
```
```sh
nano .git/hooks/pre-commit
```

add the below command after line no 13 inside if statement

```sh
source pre-commit-build.sh
```

```sh
$ docker run -d -v [Source path]:/zgw -it zwave/zgw_armhf_debian_stretch_cross bash
```

```sh
$ docker container rename xxx zwave_prod
```
```
NOTE : Use git commit --no-verify to disable the above verification process
```

## Deployment and Debugging

Deploying is a process where the code is cross compiled for the architecture of the controller and then the controller is updated with newest version. To do that first a debian package should be created during the cross compilation process which can be done with "make package" command. then on  the controllers end the old package should be uninstalled and the new version should be installed using debian package manager.

### Production build using Docker (Manual)
The below process will create a debian package using the newest version of source code. To deploy it, the deb package should me manually coppied to the controller, the the old version should be removed and the new version should be installed. steps are as follows.

```sh
$ docker run -v [Source path]:/zgw -it zwave/zgw_armhf_debian_stretch_cross bash
```
`example : docker run -v /home/aura/dev/SerialGateway:/zgw -it zwave/ zgw_armhf_debian_stretch_cross bash`

```sh
$ $ cd /zgw
```
```sh
$ mkdir build_rpi
```
```sh
$ cd build_rpi
```
```sh
$ cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/debian_stretch_armhf.cmake ..
```
```sh
$ make -j16 (no of threads supported by the system)
```
```sh
$ make package
```

- `A file with name zipgateway-x. xx . xx-Linux-stretch-armhf.deb will be created in build_prod`

### Manual Deployment

Copy the file

```sh
$ scp zipgateway-x. xx . xx-Linux-stretch-armhf.deb leo@192.168.x.x:/home/leo/SerialGateway/
```
Remove previous version if exists

```sh
$ sudo apt-get purge zipgateway
```

Install the new version

```sh
$ sudo dpkg -i zipgateway-x. xx . xx-Linux-stretch-armhf.deb
```
Once these steps are done, the below message is displyed.

```
Can't exec "/var/lib/dpkg/info/zipgateway.config": Permission denied
```
This is an error due to an unresolved dependency in the debian package creator. The below commands should be executed after the error to resolve the issue.

```sh
$ sudo chmod +x /var/lib/dpkg/info/zipgateway.config
```
```sh
$ sudo dpkg --configure -a
```
After running the above comand, a Package configuration screen is prompted. there the below configuration should be set

```
Default serial port : /dev/ttyS0
```
```
IPv6 address : (should be left blank)
```
```
IPv6 prefix : (should be left blank)
```
```
Wireless network interface : wlan0
```
```
Reboot now : yes
```
After the reboot is complete, login again and change the RF region in the zipgateway.cfg file

```sh
$ cd /usr/local/etc/
```
```sh
$ sudo nano zipgateway.cfg
```
```
ZWRFRegion=0x05 (05 is the frequency constant for India)
```
```sh
$ sudo systemctl restart leo.service
```
This completes the installation process for Serialgateway.

### - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - OR - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

###  Deployment (Script)

The Below script will do the entire installation process (defined in manual deployment section)

```sh
$ scp controller-deploy.sh leo@192.168.x.x:/home/leo/serialgateway/
```
```sh
$ sudo chmod +x controller-deploy.sh
```
```sh
$ ./controller-deploy.sh
```

- `Default Serial port /dev/ttyS0`

### Debugging

Debugging can be done by monitoring the log files and trying to send and recieve data. Log files can be accessed persistanatly using the tail command. for serialgateway, there are 3 main log files to monitor. Gateway logs, zwave gear logs and Serialgateway logs.

__gateway logs__
This log file can be found at
```
/home/leo/Device/tmp/gateway.log
```
and can be accessed using the tail command as follows

```sh
$ tail -f /home/leo/Device/tmp/gateway.log
```

__zwave gear logs__
This log file can be found at
```
/home/leo/Device/tmp/zwave.log
```
and can be accessed using the tail command as follows

```sh
$ tail -f /home/leo/Device/tmp/zwave.log
```

__Serialgateway logs__
This log file can be found at
```
/var/log/zipgateway.log
```
and can be accessed using the tail command as follows

```sh
$ tail -f /var/log/zipgateway.log
```


<hr style="border:2px solid gray">

## FAQ

- `Error response from daemon: Container <Container id> is not running`
```sh
docker container start <container id>
```

## SerialGateway Architecture

The entire workflow of SerialGateway can be divided into three main strcutures. First vertical is the API development on the gateway end which will facilitate communication between app and device. Second major vertical will be the communication between Aura Gateway and Z-wave Gear, and Zwave gear and SerialGateway. And the third will be the processing and exicuting the command recieved from the Z-wave gear.

##### API development
This section handles the reception and delivery of messages through HTTP protocol, these API handler recide on the gateway end and cater to zwave-gear service, essentially working as message brokers with added verification and validation services.

##### Communication between Aura Gateway and Zwave Gear
Unix domain sockets are used as a communication agent between services. A client will receive data from zwave gear and the Server will send data to Zwave gear (from Gateway's frame of reference). both Gateway and Zwave Gear will have independant thread which will handle the task of writing data to and reading data from the socket file.

##### Communication between Zwave Gear and SerialAPI
This Communication is also simliar to the architecture above, but the only difference lies on the Serialgateway end. since Serialgateway operates as a single thread program, it is not possible to assign a seperate thread to just read and write data on the socket, hence SIGNALS are used to indicate the service that a new message has been added on the socket. Only then will the program calls the function to read data from the socket and process it.

pros: This method will allow the program to run in a single thread, hence no data from serial port will be lost.

cons: when the frequency of messages are high, there is a possibility that the SIGNALs go out of sync, which will lead to previous messages getting triggered instead of the current message as socket will not clear old data unless it is read by the client.

##### Processing and executing

![Architecture Diagram](https://gitlab.com/aura-developers/zwave-serialapi/uploads/75a6f92a9a3c09b79a9b1172cd95d57e/SerialGateway.png)
