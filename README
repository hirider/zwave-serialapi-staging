How to see documentation on Ubuntu 18.04 systems:
----------------------------

Ubuntu 20.04
$ sudo apt-get install -y doxygen graphviz mscgen roffit perl git python3 cmake\
                           gcc xsltproc bison flex gcc-9-multilib \
                          pkg-config:i386 libssl-dev:i386 libc6-dev:i386 \
                          libusb-1.0-0-dev:i386 libjson-c-dev:i386 \
                          openjdk-8-jre curl g++-9-multilib libstdc++-9-dev
On all systems:
$ curl -L http://sourceforge.net/projects/plantuml/files/plantuml.1.2019.7.jar/download --output /opt/plantuml.jar

2. Build documentation
$ export PLANTUML_JAR_PATH=/opt/plantuml.jar
$ mkdir build
$ cd build/
$ cmake ..
$ make doc 

3. For detailed description on compiling zipgateway please refer to user guide 
generated in step 2. 
Open src/doc/html/index.hml in browser

$ xdg-open src/doc/html/index.html
