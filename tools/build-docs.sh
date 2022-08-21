#!/bin/sh -e

if [ $# -ne 3 ]; then
	echo "usage: $0 <input> <outpath> <rootdoc>"
	echo "  inputpath - path to the doc input folder"
	echo "  outpath   - name of the output path"
	echo "  rootdoc   - name of the main document"
	exit 1
fi

echo "[INFO] >>>build-docs.sh<<< called with followig parameters:"

inputPath="${1}"
echo "[INFO] inputPath: ${inputPath}"
outPath="${2}"
echo "[INFO] outPath:    ${outPath}"
rootDoc="${3}"
echo "[INFO] rootDoc:    ${rootDoc}"

scriptDir=$(dirname $0)
echo "[INFO] scriptDir: '${scriptDir}'"
tmpDir=$(echo ${inputPath} | sed -e 's/\//_/g')
echo "[INFO] tmpDir: '${tmpDir}'"


#=======================================================================
# create temp dir
if [ -e /tmp/${tmpDir}_docs ] ; then
	rm -r /tmp/${tmpDir}_docs
fi
mkdir /tmp/${tmpDir}_docs


#=======================================================================
# copy content to temp folder
cp -rv ${inputPath}/doc/* /tmp/${tmpDir}_docs


#=======================================================================
# set git versioning information
revision=$(${scriptDir}/get_branch_rev.sh)
echo "revision: '${revision}'"

#case $revision in
#    *-*) RELEASE="" ;;
#      *) RELEASE="YES" ;;
#esac

sed -ie "s/:version:/:version: Version ${revision}/g" /tmp/${tmpDir}_docs/*.adoc
sed -ie "s/@AMRD_VERSION@/${revision}/g" /tmp/${tmpDir}_docs/*.adoc

#=======================================================================
# process docs

checkhtmltopdf=`which wkhtmltopdf || true`
checkascdocpdf=`which asciidoctor-pdf || true`

mkdir -p ${outPath}

# give priority to the asciidoctor-pdf renderer
if [ "$checkascdocpdf" != "" ]; then
# Render book style (-d book) with numbered chapters.
#   asciidoctor-pdf -n -d book /tmp/${tmpDir}_docs/${rootDoc}.adoc -o ${outPath}/${rootDoc}.pdf
# Render default style with numbered (-n) chapters.
    asciidoctor-pdf -n /tmp/${tmpDir}_docs/${rootDoc}.adoc -o ${outPath}/${rootDoc}.pdf
else
    asciidoc -v -o ${outPath}/$(basename ${rootDoc}).html /tmp/${tmpDir}_docs/$(basename ${rootDoc}).adoc
    if [ "$checkhtmltopdf" != "" ] ; then
	checkx=`ps uax | grep [X]org || true`
	if [ "$checkx" != "" ] ; then
		wkhtmltopdf -q --title "${rootDoc}"  ${outPath}/$(basename ${rootDoc}).html ${outPath}/$(basename ${rootDoc}).pdf
	else
		xvfb-run -a -s "-screen 0 640x480x16" wkhtmltopdf -q --title "$(basename ${rootDoc})"  ${outPath}/$(basename ${rootDoc}).html ${outPath}/$(basename ${rootDoc}).pdf
	fi
    fi
fi

rm -rf /tmp/${tmpDir}_docs

