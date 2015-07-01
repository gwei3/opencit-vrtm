#!/bin/bash

# VRTM uninstall script
# Outline:
# 1.  define HOME directory
# 2.  define ENV directory and load application environment variables
# 3.  source functions script
# 4.  define application directory layout
# 5.  remove existing mtwilson monit configuration scripts
# 6.  stop running services
# 7.  delete contents of HOME directory
# 8.  if root:
#     remove startup scripts
#     remove PATH symlinks

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

# source functions script
. $VRTM_HOME/bin/functions.sh

# define application directory layout
if [ "$VRTM_LAYOUT" == "linux" ]; then
  export VRTM_CONFIGURATION=${VRTM_CONFIGURATION:-/etc/vrtm}
  export VRTM_REPOSITORY=${VRTM_REPOSITORY:-/var/opt/vrtm}
  export VRTM_LOGS=${VRTM_LOGS:-/var/log/vrtm}
elif [ "$VRTM_LAYOUT" == "home" ]; then
  export VRTM_CONFIGURATION=${VRTM_CONFIGURATION:-$VRTM_HOME/configuration}
  export VRTM_REPOSITORY=${VRTM_REPOSITORY:-$VRTM_HOME/repository}
  export VRTM_LOGS=${VRTM_LOGS:-/var/log/vrtm}
#  export VRTM_LOGS=${VRTM_LOGS:-$VRTM_HOME/logs}
fi
export VRTM_BIN=$VRTM_HOME/bin
export VRTM_JAVA=$VRTM_HOME/java

# note that the env dir is not configurable; it is defined as "env" under home
export VRTM_ENV=$VRTM_HOME/env

# if monit is running and there are mtwilson monit configurations, remove it to
# prevent monit from trying to restart services during uninstall
service monit status >/dev/null 2>&1
if [ $? -eq 0 ]; then
  rm -rf ${VRTM_HOME}/share/monit/conf.d/*.mtwilson
  if [ "$(whoami)" == "root" ] && [ -d /etc/monit/conf.d ]; then
    rm -rf /etc/monit/conf.d/*.mtwilson
    service monit restart
  fi
fi

# if existing services are running, stop them
existing_vrtm="$(which vrtm 2>/dev/null)"
if [ -n "$existing_vrtm" ]; then
  $existing_vrtm stop
fi
existing_rplistener="$(which vrtmlistener 2>/dev/null)"
if [ -n "$existing_rplistener" ]; then
  $existing_rplistener stop
fi

#remove logrotate files
#rm -rf /etc/logrotate.d/vrtm

# delete VRTM_HOME
if [ -d $VRTM_HOME ]; then
  rm -rf $VRTM_HOME 2>/dev/null
fi

if [ "$(whoami)" == "root" ]; then
  # remove startup scripts
  remove_startup_script vrtm
  remove_startup_script vrtmlistener

  # remove PATH symlinks
  rm -rf /usr/local/bin/vrtm
  rm -rf /usr/local/bin/vrtmlistener

  ## delete user
  #export VRTM_USERNAME=${VRTM_USERNAME:-vrtm}
  #if getent passwd $VRTM_USERNAME >/dev/null 2>&1; then
  #  userdel $VRTM_USERNAME
  #fi
fi

if [ "$(whoami)" == "root" ]; then
  # Replace vRTM proxy binary with original qemu-system-x86_64 binary
  if [ -e "/usr/bin/qemu-system-x86_64_orig" ]; then
    if [ -e "/usr/bin/qemu-system-x86_64" ]; then
      cp --preserve=all /usr/bin/qemu-system-x86_64_orig /usr/bin/qemu-system-x86_64
    fi

    if [ -e "/usr/libexec/qemu-kvm" ]; then
      cp --preserve=all /usr/bin/qemu-system-x86_64_orig /usr/libexec/qemu-kvm
    fi
  fi

  # Remove library path
  if [ -e "/etc/ld.so.conf.d/vrtm.conf" ]; then
    rm -rf /etc/ld.so.conf.d/vrtm.conf
  fi
fi

if [ -e $VRTM_LOGS ]; then
  rm -rf $VRTM_LOGS
fi

echo_success "VRTM uninstall complete"








