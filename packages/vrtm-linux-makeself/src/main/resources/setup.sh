#!/bin/sh

# VRTM install script
# Outline:
# 1.  load existing environment configuration
# 2.  source the "functions.sh" file:  mtwilson-linux-util-*.sh
# 3.  look for ~/vrtm.env and source it if it's there
# 4.  force root user installation
# 5.  define application directory layout 
# 6.  backup current configuration and data, if they exist
# 7.  create application directories and set folder permissions
# 8.  store directory layout in env file
# 9.  install prerequisites
# 10. unzip vrtm archive vrtm-zip-*.zip into VRTM_HOME, overwrite if any files already exist
# 11. copy utilities script file to application folder
# 12. set additional permissions
# 13. run additional setup tasks

#####

# default settings
# note the layout setting is used only by this script
# and it is not saved or used by the app script
export VRTM_HOME=${VRTM_HOME:-/opt/vrtm}
VRTM_LAYOUT=${VRTM_LAYOUT:-home}

# the env directory is not configurable; it is defined as VRTM_HOME/env and
# the administrator may use a symlink if necessary to place it anywhere else
export VRTM_ENV=$VRTM_HOME/env

# load application environment variables if already defined
if [ -d $VRTM_ENV ]; then
  VRTM_ENV_FILES=$(ls -1 $VRTM_ENV/*)
  for env_file in $VRTM_ENV_FILES; do
    . $env_file
    env_file_exports=$(cat $env_file | grep -E '^[A-Z0-9_]+\s*=' | cut -d = -f 1)
    if [ -n "$env_file_exports" ]; then eval export $env_file_exports; fi
  done
fi

# functions script (mtwilson-linux-util-3.0-SNAPSHOT.sh) is required
# we use the following functions:
# java_detect java_ready_report 
# echo_failure echo_warning
# register_startup_script
UTIL_SCRIPT_FILE=$(ls -1 mtwilson-linux-util-*.sh | head -n 1)
if [ -n "$UTIL_SCRIPT_FILE" ] && [ -f "$UTIL_SCRIPT_FILE" ]; then
  . $UTIL_SCRIPT_FILE
fi

# load installer environment file, if present
if [ -f ~/vrtm.env ]; then
  echo "Loading environment variables from $(cd ~ && pwd)/vrtm.env"
  . ~/vrtm.env
  env_file_exports=$(cat ~/vrtm.env | grep -E '^[A-Z0-9_]+\s*=' | cut -d = -f 1)
  if [ -n "$env_file_exports" ]; then eval export $env_file_exports; fi
else
  echo "No environment file"
fi

# enforce root user installation
if [ "$(whoami)" != "root" ]; then
  echo_failure "Running as $(whoami); must install as root"
  exit -1
fi

# define application directory layout
if [ "$VRTM_LAYOUT" == "linux" ]; then
  export VRTM_CONFIGURATION=${VRTM_CONFIGURATION:-/etc/vrtm}
  export VRTM_REPOSITORY=${VRTM_REPOSITORY:-/var/opt/vrtm}
  export VRTM_LOGS=${VRTM_LOGS:-/var/log/vrtm}
elif [ "$VRTM_LAYOUT" == "home" ]; then
  export VRTM_CONFIGURATION=${VRTM_CONFIGURATION:-$VRTM_HOME/configuration}
  export VRTM_REPOSITORY=${VRTM_REPOSITORY:-$VRTM_HOME/repository}
  export VRTM_LOGS=${VRTM_LOGS:-$VRTM_HOME/logs}
fi
export VRTM_BIN=$VRTM_HOME/bin
export VRTM_JAVA=$VRTM_HOME/java

# note that the env dir is not configurable; it is defined as "env" under home
export VRTM_ENV=$VRTM_HOME/env

vrtm_backup_configuration() {
  if [ -n "$VRTM_CONFIGURATION" ] && [ -d "$VRTM_CONFIGURATION" ]; then
    datestr=`date +%Y%m%d.%H%M`
    backupdir=/var/backup/vrtm.configuration.$datestr
    cp -r $VRTM_CONFIGURATION $backupdir
  fi
}

vrtm_backup_repository() {
  if [ -n "$VRTM_REPOSITORY" ] && [ -d "$VRTM_REPOSITORY" ]; then
    datestr=`date +%Y%m%d.%H%M`
    backupdir=/var/backup/vrtm.repository.$datestr
    cp -r $VRTM_REPOSITORY $backupdir
  fi
}

# backup current configuration and data, if they exist
vrtm_backup_configuration
vrtm_backup_repository

if [ -d $VRTM_CONFIGURATION ]; then
  backup_conf_dir=$VRTM_REPOSITORY/backup/configuration.$(date +"%Y%m%d.%H%M")
  mkdir -p $backup_conf_dir
  cp -R $VRTM_CONFIGURATION/* $backup_conf_dir
fi

# create application directories (chown will be repeated near end of this script, after setup)
for directory in $VRTM_HOME $VRTM_CONFIGURATION $VRTM_REPOSITORY $VRTM_JAVA $VRTM_BIN $VRTM_LOGS $VRTM_ENV; do
  mkdir -p $directory
  chmod 700 $directory
done

# store directory layout in env file
echo "# $(date)" > $VRTM_ENV/vrtm-layout
echo "export VRTM_HOME=$VRTM_HOME" >> $VRTM_ENV/vrtm-layout
echo "export VRTM_CONFIGURATION=$VRTM_CONFIGURATION" >> $VRTM_ENV/vrtm-layout
echo "export VRTM_REPOSITORY=$VRTM_REPOSITORY" >> $VRTM_ENV/vrtm-layout
echo "export VRTM_JAVA=$VRTM_JAVA" >> $VRTM_ENV/vrtm-layout
echo "export VRTM_BIN=$VRTM_BIN" >> $VRTM_ENV/vrtm-layout
echo "export VRTM_LOGS=$VRTM_LOGS" >> $VRTM_ENV/vrtm-layout

# install prerequisites
VRTM_YUM_PACKAGES="zip unzip"
VRTM_APT_PACKAGES="zip unzip"
VRTM_YAST_PACKAGES="zip unzip"
VRTM_ZYPPER_PACKAGES="zip unzip"
auto_install "Installer requirements" "VRTM"
if [ $? -ne 0 ]; then echo_failure "Failed to install prerequisites through package installer"; exit -1; fi

# delete existing java files, to prevent a situation where the installer copies
# a newer file but the older file is also there
if [ -d $VRTM_HOME/java ]; then
  rm $VRTM_HOME/java/*.jar 2>/dev/null
fi

# extract vrtm  (vrtm-zip-0.1-SNAPSHOT.zip)
echo "Extracting application..."
VRTM_ZIPFILE=`ls -1 vrtm-*.zip 2>/dev/null | head -n 1`
unzip -oq $VRTM_ZIPFILE -d $VRTM_HOME

# copy utilities script file to application folder
cp $UTIL_SCRIPT_FILE $VRTM_HOME/bin/functions.sh

# set permissions
chmod 700 $VRTM_HOME/bin/*
chmod 700 $VRTM_HOME/dist/*

(cd $VRTM_HOME/dist && ./vRTM_KVM_install.sh)
rm -rf /$VRTM_HOME/dist
ln -s /opt/tbootxm/bin/verifier $INSTALL_DIR/rpcore/bin/debug

echo_success "Installation complete"
