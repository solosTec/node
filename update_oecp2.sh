#!/bin/bash
# This script copies the source archives to your OECP project
# and updates the corresponding makefiles

# Example for default
OECPDIR="../../ppc/oecp2-ext"
if [ "$1" != "" ]; then
	OECPDIR=$1
fi


if [ ! -d ${OECPDIR} ]; then
        echo "Directory ${OECPDIR} to OECP project does not exist!"
        echo "Please call ${0} <directory-to-oecp-project>"
        exit 1
fi

OECPDIR=$(readlink -f ${OECPDIR})
TARGETDIR="${OECPDIR}/Software/PPC/Firmware/src"

echo "Using OECP directory ${OECPDIR}"



rm *.bz2
rm *.md5
./build_src_archive.sh


for archive in *.bz2; do
	tool=$(echo ${archive} | cut -d "_" -f 1)
	version=$(echo ${archive} | cut -d "_" -f 2)
	md5=$(md5sum ${archive} | cut -d " " -f 1)
	makefile="${OECPDIR}/Software/PPC/Firmware/rules/${tool}.make"

	echo "========================================================"
	echo "Tool    : ${tool}"
	echo "Version : ${version}"
	echo "md5     : ${md5}"
	echo "makefile: ${makefile}"	
	echo "========================================================"

	makefile_present=$(ls "${makefile}" 2>/dev/null)
	if [ "${makefile_present}" == "" ]; then
		echo "Makefile NOT found, skipping update!"
	else
		filevar=$(cat ${makefile} | grep -E "^[A-Z].*_VERSION[[:space:]]*:=.*" | grep -oE ".*_VERSION")
		md5var=$(cat ${makefile} | grep -E "^[A-Z].*_MD5[[:space:]]*:=.*" | grep -oE ".*_MD5")
		suffixvar=$(cat ${makefile} | grep -E "^[A-Z].*_SUFFIX[[:space:]]*:=.*" | grep -oE ".*_SUFFIX")
		if [ "${filevar}" == "" ]; then
			echo "Variable for filename not found, skipping"
			continue
		fi

		if [ "${md5var}" == "" ]; then
			echo "Variable for MD5 sum not found, skipping"
			continue
		fi

		if [ "${suffixvar}" == "" ]; then
			echo "Variable for suffix not found, skipping"
			continue
		fi
		
		echo "Remove old copies"
		find ${TARGETDIR}/.. -type f -name "smf_v*" -delete		
		
		echo "Copying ${archive} to ${TARGETDIR}"
		cp ${archive} ${TARGETDIR}

		echo "Adapting ${makefile} :"
		
		echo "Setting ${filevar} := ${version}"
		mycmd="sed -i 's/^${filevar}[[:space:]]*:=.*/${filevar} := ${version}/' ${makefile}"
		bash -c "${mycmd}"
		
		echo "Setting ${md5var} := ${md5}"
		mycmd="sed -i 's/^${md5var}[[:space:]]*:=.*/${md5var} := ${md5}/' ${makefile}"
       	bash -c "${mycmd}"
       	
		echo "Setting SMF := smf_${version}_src"
		mycmd="sed -i 's/^SMF[[:space:]]*:=.*/SMF := smf_${version}_src/' ${makefile}"
		bash -c "${mycmd}"
		
		echo "Setting ${suffixvar} := tar.bz2"
		mycmd="sed -i 's/^${suffixvar}[[:space:]]*:=.*/${suffixvar} := tar.bz2/' ${makefile}"
       	bash -c "${mycmd}"
	fi
done
echo "Done."

