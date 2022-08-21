#!/bin/bash
#
# create-src-archive.sh <projectname> <projectfolder> <fileslistfile> <outputpath> <projectstore>
# Examples:
# create-src-archive.sh 'clsNTPproxy' libCLS_Apps/clsNTPproxy libCLS_Apps/clsNTPproxy/src_filelist.txt output/clsNTPproxy rev-1.3 libCLS-suite
#

PROJECTNAME=""
if [ ! -z "${1}" ]; then
	PROJECTNAME=${1}
else
	exit 1
fi

PROJECTFOLDER=""
if [ ! -z "${2}" ]; then
	PROJECTFOLDER=${2}
else
	exit 1
fi

FILESLISTFILE=""
if [ ! -z "${3}" ]; then
	FILESLISTFILE=${3}
else
	exit 1
fi

OUTPUTFOLDER=""
if [ ! -z "${4}" ]; then
	OUTPUTFOLDER=${4}
else
	exit 1
fi

REVISION=""
if [ ! -z "${5}" ]; then
	REVISION=${5}
else
	exit 1
fi

# This parameter is necessary ony when we have related sub-projects
# of a project and they share the same downlad directory on the Jenkins server.
PROJECTSTORE=""
if [ ! -z "${6}" ]; then
	PROJECTSTORE=${6}
else
	PROJECTSTORE=${PROJECTNAME}
fi

echo "[INFO] Projectname:   ${PROJECTNAME}"
echo "[INFO] Projectfolder: ${PROJECTFOLDER}"
echo "[INFO] Fileslistfile: ${FILESLISTFILE}"
echo "[INFO] Outputfolder:  ${OUTPUTFOLDER}"
echo "[INFO] Revision:      ${REVISION}"
echo "[INFO] Projectstore:  ${PROJECTSTORE}"

tmp_path=/tmp/${PROJECTNAME}_${REVISION}/
echo "[INFO] tmp_path: ${tmp_path}"

rm -rf ${tmp_path}
mkdir ${tmp_path}

for filename in $(cat ${FILESLISTFILE}); do
	echo "--------------"
	#echo "filename: ${filename}"
	#echo "filename: $(basename ${filename})"
	#echo "dirname:  $(dirname ${filename})"
	
	if [ "." = "$(dirname ${filename})" ]; then
		cp -r ${PROJECTFOLDER}/${filename} ${tmp_path}/
	elif [[ $(dirname ${filename}) = ..* ]]; then
		cp -r ${PROJECTFOLDER}/${filename} ${tmp_path}/
	elif [ "<output>" = "$(dirname ${filename})" ]; then
		cp -r ${OUTPUTFOLDER}/$(basename ${filename}) ${tmp_path}/
	else
		mkdir -p ${tmp_path}/$(dirname ${filename})
		cp -r ${PROJECTFOLDER}/${filename} ${tmp_path}/$(dirname ${filename})
	fi
done

# Create the project source archive with reproducible MD5SUM.
tar -C /tmp/ -cjvf ${OUTPUTFOLDER}/${PROJECTNAME}_${REVISION}_src.tar.bz2 ${PROJECTNAME}_${REVISION}/
md5sum ${OUTPUTFOLDER}/${PROJECTNAME}_${REVISION}_src.tar.bz2 > ${OUTPUTFOLDER}/${PROJECTNAME}_${REVISION}_src.md5


# Copy the archive where Jenkins can reach it.
# TODO: Only tagged releases, not intermmediate commits.
if [ "$JENKINS_HOME" != "" ] ; then
	echo "[INFO] Running as Jenkins job"
	latesttag=`git tag --list | sort -V | tail -n1`
	if [ "$latesttag" = "${REVISION}" ] ; then
		if [ ! -f ${JENKINS_HOME}/jobs/${PROJECTSTORE}/builds/Downloads/${PROJECTNAME}_${REVISION}_src.tar.bz2  ] ; then
		    echo "[INFO] New git tag/release -> copy archive/MD5 in Downloads"
		    cp -v ${OUTPUTFOLDER}/${PROJECTNAME}_${REVISION}_src.* ${JENKINS_HOME}/jobs/${PROJECTSTORE}/builds/Downloads/
		else
		    echo "[WARNING] Archive for this release is already created, skipping..."
		fi
	else
		echo "[INFO] Revision [${REVISION}] is no release, no release archive will be created."
	fi
else
    echo "[INFO] Not running under Jenkins, only create local archives."
fi

rm -rf ${tmp_path}
