#!/bin/sh
# 
# Call script without any parameters on a bplrouter device with installed version 0.8.x
# The script dumps the database of the current node 0.8 installation into a textfile so it can be processed 
# at a later step by another script.
#
#


SMF_DIR="/usr/local/etc/smf"

# these variables will be replaced by CMAKE
#VERSION_SHORT=${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}
#VERSION_LONG=${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.${${PROJECT_NAME}_VERSION_PATCH}.${${PROJECT_NAME}_VERSION_TWEAK}

create_backups ()
{

    echo "[db dump] create_backups"

    echo "[db dump] ... create backups folder"
    backup_folder="backups"
    backup_folder_path=${SMF_DIR}/${backup_folder}
    mkdir -p ${backup_folder_path}
    echo "[db dump] ... created folder backups: ${backup_folder_path}"


    version=$(opkg list | grep -i node | awk '{print $3}')
    echo "version: $version"
    backup_version_folder=$version
    backup_version_folder_path=${backup_folder_path}/$version
    mkdir -p ${SMF_DIR}/${backup_folder}/${backup_version_folder}
    echo "[db dump] ... created backup_version_folder_path: ${backup_version_folder_path}"


    echo "[db dump] ... dump database"
    dump_file="database_dump.txt"
    dump_file_path=${backup_version_folder_path}/${dump_file}

    /usr/local/sbin/segw -l >> ${dump_file_path}
    
    echo "[db dump] ... db dump: ${dump_file_path}"


    cfg_file_v08="segw_v0.8.cfg"
    cfg_file_path="/usr/local/etc/${cfg_file_v08}"

    echo "[db dump] ... save cfg file"
    cp ${cfg_file_path} ${backup_version_folder_path}

    echo "[db dump] ... saved cfg file ${cfg_file_path} to ${backup_version_folder_path}"



    return 0
}


main ()
{
    echo ""
	echo " ######################################################## "
	echo " #            Performing db dump         ...            # "
	echo " ######################################################## "
	echo ""
    
    
    echo "[db dump] stop application"
    systemctl stop segw

    create_backups "$SMF_DIR/backups/$VERSION_LONG"

    echo ""
    echo " ######################################################## "
    echo " #        db dump Completed!                            # "
    echo " ######################################################## "
    echo ""

    return 0
}

main
