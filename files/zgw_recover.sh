#!/bin/bash
# © 2019 Silicon Laboratories Inc.
#
# This script may be used to restore a gateway backup. It is a prerequisite 
# that a working gateway is installed, but is not running
#
# The script will perform the following tasks. 
# 1) Verify that the install bundle is valid.
# 2) Install the zipgateway data files from the backup bundle at the location in the filesystem written in the backup manifest file. 
#    By default, this will be the location they had when the backup was taken. If there are existing 
#    configuration files the old files will be backed up as <filename>.old
# 3) Update the NVM of Z-Wave controller.
#

PROGRAMMER="/usr/local/bin/zw_programmer"
NVMCONVERTER="/usr/local/bin/zw_nvm_converter"
EERPOM2SQLITECONVERTER="/usr/local/bin/zgw_eeprom_to_sqlite"
GW_Databasefilename="zipgateway.db"
ZGW_PNAME="zipgateway"

[ -x "${PROGRAMMER}" ] || { 
    echo "zw_programmer not found in ${PROGRAMMER}" 
    exit 1 
}

[ -x "${NVMCONVERTER}" ] || { 
    echo "zw_nvm_converter not found ${NVMCONVERTER}" 
    exit 1 
}


nvm_json_restore() {
    JSON_FILE=$1

    NVM_DST=`mktemp -t nvm_dstXXX`

    ${NVMCONVERTER} -i ${PROTOCOL_VERSION_SHORT} ${JSON_FILE} ${NVM_DST} || {
        echo "Unable to convert NVM to ${PROTOCOL_VERSION_SHORT}. This should not happen."
        exit 1
    }

    ${PROGRAMMER} -w ${NVM_DST} -s ${SERIAL_PORT} || {
        echo "Error writing the protocol NVM"
        exit 1
    }
    rm -f ${NVM_DST}
}

nvm_binary_restore() {
    NVM_DST=$1

    ${PROGRAMMER} -w ${NVM_DST} -s ${SERIAL_PORT} || {
        echo "Error writing the protocol NVM"
        exit 1
    }
}


print_help() {    
    echo "Usage: $0 -s <serial dev> -b <target_firmware>"
    echo "   -s <serial dev>      : device file to which the Z-Wave controller is attached, the default is the setting in inside the backup zipgateway.cfg"
    echo "   -b <backupfile>      : ZIP file containing a gateway backup file."
    exit 1
}

do_install() {
    DST=$2
    SRC="${UNPACK_DIR}/$(basename $DST)"
    #Just skip files which are not in the backup
    [ -f $SRC ] || return

    [ -d `dirname $DST` ] || {
      echo "Paths on current system may not match paths on in the backup manifest."
      echo "Unable to place $DST"
      exit 1
    }

    install -b -v -g 0 -o 0 -m $1 "${SRC}" "${DST}" || {
      echo "Unable to restore backup bundle."
      exit 1
    }
}

do_cleanup() {
  rm -rf ${UNPACK_DIR}
}

while getopts "s:b:" o; do
    case "${o}" in
        s)
            SERIAL_PORT=${OPTARG}
            ;;
        b)
            BACKUP_FILE=${OPTARG}
            ;;
        *)
            print_help
            ;;
    esac
done


[ -f "${BACKUP_FILE}" ] || { 
    echo "backup file was not found."
    print_help
}

#check if a gateway is running
PIDS=$(pidof $ZGW_PNAME | wc -w)
if [ $PIDS -ne 0 ]
then
  echo "A Z/IP Gateway is currently running. Please shutdown the gateway and run this script again."
  exit 1
fi

#Extract the backup
UNPACK_DIR=$(mktemp -d)
trap do_cleanup EXIT
unzip -j  "${BACKUP_FILE}" -d ${UNPACK_DIR} || {
  echo "unable to extract the backup bundle."
  exit 1w
}

[ -f ${UNPACK_DIR}/manifest ] ||
{
  echo "Missing manifest file. Backup data is invalid."
  exit 1
}

eval $(cat ${UNPACK_DIR}/manifest)

if [ -z "${SERIAL_PORT}" ]; then
  #Try to get the serial port name from the zipgateway.cfg
  [ -f ${GW_CONFIG_FILE_PATH} ] && {
      eval `grep ZipSerialAPIPortName ${GW_CONFIG_FILE_PATH} | sed -E "s/[[:space:]]+//g"`
      SERIAL_PORT=${ZipSerialAPIPortName}
  }
fi

if [ ! -z "${GW_Eepromfile}" ]; then
  #Convert eeprom.dat to zipgateway.db if eeprom exists
  GW_Unpackdatabase="${UNPACK_DIR}/"${GW_Databasefilename}
  GW_Unpackeepromfile="${UNPACK_DIR}/$(basename ${GW_Eepromfile})"
  ${EERPOM2SQLITECONVERTER} -e ${GW_Unpackeepromfile} -d ${GW_Unpackdatabase} || {
    echo "Unable to convert ${GW_Unpackeepromfile} to zipgateway.db. Unexpected error."
    exit 1
  }
  #Set the GW_Databasefile to be in the same folder as GW_Eepromfile
  GW_Databasepath=`dirname ${GW_Eepromfile}`
  GW_Databasefile="${GW_Databasepath}/${GW_Databasefilename}"
fi


#collect the firmware version of the currently attached controller.
PROTOCOL_VERSION_FULL=`${PROGRAMMER} -t -s ${SERIAL_PORT}| grep "NVM:"` || {
    echo "Unable to connect to existing controller on device ${SERIAL_PORT}. Please check your serial path."
    exit 1
}
PROTOCOL_VERSION_SHORT=${PROTOCOL_VERSION_FULL#*NVM: }
SOURCE_PROTOCOL_VERSION_SHORT=${GW_PROTOCOL_VERSION}
echo "Current controller firmware is ${PROTOCOL_VERSION_SHORT}"

echo "Backup created on: ${GW_BackupDate}"

do_install 644 ${GW_ZipCaCert}
do_install 644 ${GW_ZipCert}
do_install 600 ${GW_ZipPrivKey}
do_install 644 ${GW_CONFIG_FILE_PATH}
do_install 644 ${GW_Databasefile}
do_install 644 ${GW_ProvisioningConfigFile}
do_install 644 ${GW_PVSStorageFile}

if [[ ${SOURCE_PROTOCOL_VERSION_SHORT:6} > "7.15" ]]; then
  nvm_binary_restore ${UNPACK_DIR}/nvm_backup
else
  nvm_json_restore ${UNPACK_DIR}/nvm.json
fi

#check if this SDK >= 7xx controller
if [[ ${PROTOCOL_VERSION_SHORT:6} > "7" ]]; then
  eval $(cat ${UNPACK_DIR}/zipgateway.cfg)
  [ -z "$ZWRFRegion" ] && {
    echo -e "\033[1;33mTarget CHIP requires ZWRFRegion to be set in the config file. Please update the config file \033[0m"
  }
fi

echo "Restore was successful. The Z/IP gateway may now be started."
exit 0






