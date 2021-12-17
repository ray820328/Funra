#!/bin/sh

START_TIME=`date +%Y%m%d_%H%M%S`
ROOT_PATH=`pwd`
FUNRA_SERVER_ROOT_3RD=3rd
FUNRA_SERVER_ROOT_BASE=rbase
FUNRA_SERVER_ROOT_SERVER=rserver
SCRIPT_FILE="make.sh"
BUILD_LEVEL=$2
FUNRA_SERVER_OUTPUT_PATH=${ROOT_PATH}/../build
FUNRA_SERVER_OUTPUT_NAME=funsvr

check_out()
{
	cd $ROOT_PATH

	#git clone
}

init()
{
	cd $ROOT_PATH

	#git pull

	return 0
}

backup()
{
	return 0
}

build()
{
	cd ${ROOT_PATH}
	cd ${FUNRA_SERVER_ROOT_3RD}
	chmod 777 $SCRIPT_FILE
	if [ ${BUILD_LEVEL} -gt 1 ]; then
		yes|./$SCRIPT_FILE
	fi

	sleep 1

	cd ${ROOT_PATH}
	cd ${FUNRA_SERVER_ROOT_BASE}
	chmod 777 $SCRIPT_FILE
	if [ ${BUILD_LEVEL} -gt 0 ]; then
		yes|./$SCRIPT_FILE
	fi

	# sleep 1

	# cd ${ROOT_PATH}
	# cd ${FUNRA_SERVER_ROOT_SERVER}
	# chmod 777 $SCRIPT_FILE
	# yes|./$SCRIPT_FILE

	cd ${ROOT_PATH}

	return 0
}

upload_dist()
{
	
	return 0
}

clean()
{
	if [[ -e $FUNRA_SERVER_OUTPUT_PATH ]]; then
		rm -rf $FUNRA_SERVER_OUTPUT_PATH
	fi

	echo "Clean finished. [$FUNRA_SERVER_OUTPUT_PATH]"
	return 0
}

usage()
{
	echo "Usage: $0 [backup|build|clean 0|1|2"
}

init

if [ $# -lt 1 ];then
    usage
    exit 0
fi

if [ $# -lt 2 ];then
    BUILD_LEVEL=0
fi

if [ "$1" = "backup" ];then
    backup

elif [ "$1" = "build" ];then
	clean
    build
    #upload_dist
    retValue=$?

	if [ "${retValue}" != "0" ]; then
		exit 1
	fi

elif [ "$1" = "clean" ];then
    clean

else
    usage
fi

exit 0
