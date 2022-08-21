#!/bin/sh

revision=$(git describe --tags)

branch=$(git branch | grep \* | cut -d ' ' -f2)
if [ "${branch}"="master" ]; then
        branch=''
else
        branch=${branch}_
fi

#datetime=$(date +'%Y%m%d_%H%M%S')

#release_revision=${branch}${revision}_${datetime}
release_revision=${branch}${revision}


if [ ! -z ${release_revision} ]; then 
	echo "${release_revision}"
else
	echo "revX"
fi
