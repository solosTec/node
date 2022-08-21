#!/bin/sh

outputPath="outdir"
if [ ! -z ${1} ]; then
	outputPath="outdir"
fi

# Tools directory for utility scripts relative to the current directory
export TOOLS_DIR="tools"

echo "[INFO] >>smf<< Creating project source archives for buildsystem integration"

export REV=$(${TOOLS_DIR}/get_branch_rev.sh)
echo "[INFO] Revision: ${REV}"
echo "[INFO] outputPath: ${outputPath}"

rm -rf ${outputPath}

# All these are part of the libCLS_suite
PROJECTSTORE="smf"
echo "[INFO] Projectstore: ${PROJECTSTORE}"
########################################
# amrd
pkgdir=${outputPath}/smf
srcdir=$(pwd)
docdir="${srcdir}/docs"
mkdir -p ${pkgdir}
${TOOLS_DIR}/create-src-archive.sh \
	'smf' \
	${srcdir} \
	${srcdir}/src_filelist.txt \
	${pkgdir} \
	${REV} \
	${PROJECTSTORE}
mv ${pkgdir}/*.bz2 ${pkgdir}/*.md5 .


#### Clean at exit ####
#rm -rf ${outputPath}
