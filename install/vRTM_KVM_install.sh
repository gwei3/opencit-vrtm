#!/bin/bash

#This script installs RPCore, RPProxy, RPListener, and openstack patches


RES_DIR=$PWD
DEFAULT_INSTALL_DIR=/opt/vrtm

OPENSTACK_DIR="Openstack/patch"
DIST_LOCATION=`/usr/bin/python -c "from distutils.sysconfig import get_python_lib; print(get_python_lib())"`

LINUX_FLAVOUR="ubuntu"
NON_TPM="false"
BUILD_LIBVIRT="FALSE"
KVM_BINARY=""
LOG_DIR="/var/log/vrtm"

function valid_ip()
{
    local  ip=$1
    local  stat=1

    if [[ $ip =~ ^[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}$ ]]; then
        OIFS=$IFS
        IFS='.'
        ip=($ip)
        IFS=$OIFS
        [[ ${ip[0]} -le 255 && ${ip[1]} -le 255 \
            && ${ip[2]} -le 255 && ${ip[3]} -le 255 ]]
        stat=$?
    fi
    return $stat
}

# This function returns either rhel fedora ubuntu suse
# TODO : This function can be moved out to some common file
function getFlavour()
{
        flavour=""
        grep -c -i ubuntu /etc/*-release > /dev/null
        if [ $? -eq 0 ] ; then
                flavour="ubuntu"
        fi
        grep -c -i "red hat" /etc/*-release > /dev/null
        if [ $? -eq 0 ] ; then
                flavour="rhel"
        fi
        grep -c -i fedora /etc/*-release > /dev/null
        if [ $? -eq 0 ] ; then
                flavour="fedora"
        fi
        grep -c -i suse /etc/*-release > /dev/null
        if [ $? -eq 0 ] ; then
                flavour="suse"
        fi
        if [ "$flavour" == "" ] ; then
                echo "Unsupported linux flavor, Supported versions are ubuntu, rhel, fedora"
                exit
        else
                echo $flavour
        fi
}

function updateFlavourVariables()
{
        linuxFlavour=`getFlavour`
        if [ $linuxFlavour == "fedora" ]
        then
             export KVM_BINARY="/usr/bin/qemu-kvm"
	     export QEMU_INSTALL_LOCATION="/usr/bin/qemu-system-x86_64"
	elif [ $linuxFlavour == "rhel" ]
	then
	     export KVM_BINARY="/usr/bin/qemu-kvm"
	     if [ -x /usr/bin/qemu-system-x86_64 ] ; then
		export QEMU_INSTALL_LOCATION="/usr/bin/qemu-system-x86_64"
	     else
	        export QEMU_INSTALL_LOCATION="/usr/libexec/qemu-kvm"
	     fi
	elif [ $linuxFlavour == "ubuntu" ]
	then
              export KVM_BINARY="/usr/bin/kvm"
	      export QEMU_INSTALL_LOCATION="/usr/bin/qemu-system-x86_64"
	elif [ $linuxFlavour == "suse" ]
	then
              export KVM_BINARY="/usr/bin/kvm"
              export QEMU_INSTALL_LOCATION="/usr/bin/qemu-system-x86_64"
        fi
}


function untarResources()
{
	cd "$RES_DIR"
	cp KVM_install.tar.gz "$INSTALL_DIR"
	cd "$INSTALL_DIR"
	tar xvzf KVM_install.tar.gz
        if [ $? -ne 0 ]; then
                echo "ERROR : Untarring of $RES_DIR/*.tar.gz unsuccessful"
                exit
        fi
	rm -rf KVM_install.tar.gz
}

function installKVMPackages_rhel()
{
        echo "Installing Required Packages ....."
        yum install -y "kernel-devel-uname-r == $(uname -r)"
        if [ $FLAVOUR == "rhel" ]; then
          yum install -y yum-utils
          yum-config-manager --enable rhel-6-server-optional-rpms
        fi
        # Install the openstack repo
	rpm -q rdo-release
	if [ $? -ne 0 ] ; then
	        yum install -y yum-plugin-priorities
        	yum install -y https://repos.fedorapeople.org/repos/openstack/openstack-icehouse/rdo-release-icehouse-3.noarch.rpm
	fi

        yum install -y libvirt libvirt-python libguestfs-tools-c libxml2
        #Libs required for compiling libvirt
        yum install -y openssh-server
	yum install -y trousers tpm-tools cryptsetup 
	yum install -y tar procps binutils
	selinuxenabled
	if [ $? -eq 0 ] ; then
		yum install -y policycoreutils-python
	fi

}

function installKVMPackages_ubuntu()
{
	echo "Installing Required Packages ....."
	apt-get -y  install python-software-properties
	apt-get -y install bridge-utils dnsmasq pm-utils ebtables ntp
	apt-get -y install openssh-server
	apt-get -y install tboot trousers tpm-tools cryptsetup-bin
	apt-get -y install qemu-utils kpartx binutils
	apt-get -y install libvirt-bin qemu-kvm libguestfs-tools 
	apt-get -y install iptables libblkid1 libc6 libcap-ng0 libdevmapper1.02.1 libgnutls26 libnl-3-200 libnuma1  libparted0debian1  libpcap0.8 libpciaccess0 libreadline6 libxml2 libyajl2 procps
	
	echo "Starting ntp service ....."
	service ntp start
}

function installKVMPackages_suse()
{
	zypper addrepo -f obs://Cloud:OpenStack:Icehouse/openSUSE_13.1 Icehouse
	zypper -n in openstack-utils
	zypper -n refresh
        zypper -n in libvirt qemu-kvm
	zypper -n in libyajl2 libpciaccess0 libnl3-200 libxml2-2
        zypper -n in bridge-utils dnsmasq pm-utils ebtables ntp wget
        zypper -n in openssh
        zypper -n in dos2unix 
	zypper -n in tboot  
}

function installKVMPackages()
{
        if [ $FLAVOUR == "ubuntu" ] ; then
		installKVMPackages_ubuntu
        elif [  $FLAVOUR == "rhel" -o $FLAVOUR == "fedora" ] ; then
		installKVMPackages_rhel
	elif [ $FLAVOUR == "suse" ] ; then
		installKVMPackages_suse
        fi
}

installLibvirtPackages_ubuntu()
{
        apt-get -y install python-software-properties
        if [ $? -ne 0 ] ; then
               echo "apt-get update failed, kindly resume after manually executing apt-get update"
        fi
        apt-get -y install libvirt-bin libvirt0 python-libvirt
	grep -c libvirtd /etc/group
        if [ $? -eq 1 ] ; then
                groupadd libvirtd
        fi
        id nova > /dev/null
        if [ $? -eq 0 ] ; then
                echo "Found nova user"
                isNovaInLibvirtGroup=`id nova | grep -i -c libvirtd`
                if [ "$isNovaInLibvirtGroup" -eq 0 ] ; then
                        echo "nova user is present but not a part of libvirtd group"
                        echo -n "Adding nova as a part of libvirtd group... "
                        usermod -a -G libvirtd nova
                        if [ $? -eq 0 ] ; then
                                echo "success"
                        else
                                echo "failed"
                        fi
                fi      
        fi
}

installLibvirtPackages_rhel()
{
	yum -y install libvirt libvirt-python
        # This is required because if one installs only libvirt then the eco-system for libvirt is not ready
        # For e.g the virtualisation group also creates libvirtd group over the system.
        yum groupinstall -y Virtualization	
	grep -c libvirtd /etc/group
	if [ $? -eq 1 ] ; then
		groupadd libvirtd
	fi
	id nova > /dev/null
	if [ $? -eq 0 ] ; then
		echo "Found nova user"
		isNovaInLibvirtGroup=`id nova | grep -i -c libvirtd`
		if [ "$isNovaInLibvirtGroup" -eq 0 ] ; then
			echo "nova user is present but not a part of libvirtd group"
			echo -n "Adding nova as a part of libvirtd group... "
			usermod -a -G libvirtd nova
			if [ $? -eq 0 ] ; then
				echo "success"
			else
				echo "failed"
			fi
		fi
	fi
}

installLibvirtPackages_suse()
{
	zypper -n in libvirt libvirt-python
        grep -c libvirtd /etc/group
        if [ $? -eq 1 ] ; then
                groupadd libvirtd
        fi
	id nova > /dev/null
        if [ $? -eq 0 ] ; then
                echo "Found nova user"
                isNovaInLibvirtGroup=`id nova | grep -i -c libvirtd`
                if [ "$isNovaInLibvirtGroup" -eq 0 ] ; then
                        echo "nova user is present but not a part of libvirtd group"
                        echo -n "Adding nova as a part of libvirtd group... "
                        usermod -a -G libvirtd nova
                        if [ $? -eq 0 ] ; then
                                echo "success"
                        else
                                echo "failed"
                        fi
                fi
        fi
}

installLibvirtPackages()
{
        if [ $FLAVOUR == "ubuntu" ] ; then
		installLibvirtPackages_ubuntu
        elif [  $FLAVOUR == "rhel" -o $FLAVOUR == "fedora" ] ; then
		installLibvirtPackages_rhel
	elif [ $FLAVOUR == "suse" ] ; then
		installLibvirtPackages_suse
        fi
	
}


function installLibvirt()
{
	# Openstack icehouse repo contains libvirt 1.2.2
	# Adding the repository
	
	if [ $BUILD_LIBVIRT == "TRUE" ] ; then
		if [ -e libvirt-1.2.2.tar.gz ] ; then
			echo "Using the packaged libvirt found in dist"
		else
			echo "This dist package does not contain custom libvirt"
			echo "Please create a dist package using --with-libvirt option"
			echo "Aborting install process.."
			exit
		fi

	    tar xvzf libvirt-1.2.2.tar.gz
	    cd libvirt-1.2.2
	    ./configure --prefix=/usr --localstatedir=/var --sysconfdir=/etc --with-xen=no --with-esx=no
	    make -j 4
	        if [ $? -ne 0 ]; then
	                echo "ERROR : make failed for libvirtd "
	                exit
	        else
	                echo "INFO : Libvirtd make PASSED"
	        fi
	    make install
	        if [ $? -ne 0 ]; then
	                echo "ERROR : make install failed for libvirtd "
	                exit
	        else
	                echo "INFO : make install PASSED"
	        fi
	    echo "libvirt version is ....."
	    libvirtd --version
	    sleep 2
	else	
		installLibvirtPackages
	fi
	# Touch them only if they are commented	
	sed -i 's/^#.*unix_sock_group.*/unix_sock_group="libvirtd"/g' /etc/libvirt/libvirtd.conf
	sed -i 's/^#.*unix_sock_rw_perms.*/unix_sock_rw_perms="0770"/g' /etc/libvirt/libvirtd.conf
	sed -i 's/^#.*unix_sock_ro_perms.*/unix_sock_ro_perms="0777"/g' /etc/libvirt/libvirtd.conf
	sed -i 's/^#.*auth_unix_ro.*/auth_unix_ro="none"/g' /etc/libvirt/libvirtd.conf
	sed -i 's/^#.*auth_unix_rw.*/auth_unix_rw="none"/g' /etc/libvirt/libvirtd.conf

	if [ $FLAVOUR == "ubuntu" ]; then
		# Disable the apparmor profile for libvirt for ubuntu
		if [ -e /etc/apparmor.d/disable/usr.sbin.libvirtd ] ; then
			echo "libvirt apparmor already disabled"
		else
			ln -s /etc/apparmor.d/usr.sbin.libvirtd /etc/apparmor.d/disable/
			apparmor_parser -R /etc/apparmor.d/usr.sbin.libvirtd 
		fi
	fi

}

function installRPProxyAndListner()
{
	echo "Installing RPProxy and Starting RPListener...."

	if [ -e $KVM_BINARY ] ; then
		echo "#! /bin/sh" > $KVM_BINARY
		echo "exec $QEMU_INSTALL_LOCATION -enable-kvm \"\$@\""  >> $KVM_BINARY
		chmod +x $KVM_BINARY
	fi
	is_already_replaced=`strings "$QEMU_INSTALL_LOCATION" | grep -c -i "rpcore"`
	if [ $is_already_replaced -gt 0 ]
	then	
		echo "RP-Proxy binary is already updated, might be old and will be replaced" 
	else
		echo "Backup of /usr/bin/qemu-system-x86_64 taken"
		cp "$QEMU_INSTALL_LOCATION" /usr/bin/qemu-system-x86_64_orig
	fi
	cp "$INSTALL_DIR/rpcore/bin/debug/rp_proxy" "$QEMU_INSTALL_LOCATION"
	
	#Verify rp-proxy replacement
	diff "$INSTALL_DIR/rpcore/bin/debug/rp_proxy" "$QEMU_INSTALL_LOCATION" > /dev/null
	if [ $? -eq 0 ] ; then
		echo "RP-Proxy replaced successfully"
	else
		echo "ERROR : Could not replace rp_proxy with $QEMU_INSTALL_LOCATION"
		echo "Please execute following after ensuring VMs are shut-down and $QEMU_INSTALL_LOCATION is not is use"
		echo "\$ cp $INSTALL_DIR/rpcore/bin/debug/rp_proxy $QEMU_INSTALL_LOCATION"
	fi

	chmod +x "$QEMU_INSTALL_LOCATION"
	touch $LOG_DIR/rp_proxy.log
	chmod 666 $LOG_DIR/rp_proxy.log
	chown nova:nova $LOG_DIR/rp_proxy.log
	cp "$INSTALL_DIR/rpcore/bin/scripts/rppy_ifc.py" $DIST_LOCATION/.
	chmod 754 "$DIST_LOCATION/rppy_ifc.py"
	cp "$INSTALL_DIR/rpcore/lib/librpchannel-g.so" /usr/lib
	if [ $FLAVOUR == "rhel" -o $FLAVOUR == "fedora" ]; then
		selinuxenabled
		if [ $? -eq 0 ] ; then
			echo "Updating the selinux policies for vRTM files"
			 semanage fcontext -a -t virt_log_t $LOG_DIR/rp_proxy.log
			 restorecon -v $LOG_DIR/rp_proxy.log
			 semanage fcontext -a -t qemu_exec_t "$QEMU_INSTALL_LOCATION"
			 restorecon -v "$QEMU_INSTALL_LOCATION"
			 semanage fcontext -a -t qemu_exec_t /usr/lib/librpchannel-g.so
			 restorecon -v /usr/lib/librpchannel-g.so
                         echo " 
                               module svirt_for_links 1.0;
                                
                               require {
                               type nova_var_lib_t;
                               type svirt_t;
                               class lnk_file read;
                               }
                               #============= svirt_t ==============
                               allow svirt_t nova_var_lib_t:lnk_file read;
                          " > svirt_for_links.te
                          /usr/bin/checkmodule -M -m -o svirt_for_links.mod svirt_for_links.te
                          /usr/bin/semodule_package -o svirt_for_links.pp -m svirt_for_links.mod
                          /usr/sbin/semodule -i svirt_for_links.pp
						  rm -rf svirt_for_links.mod svirt_for_links.pp svirt_for_links.te
		else
			echo "WARN : Selinux is disabled, enabling SELinux later will conflict vRTM"
		fi
	fi
	ldconfig
	cd "$INSTALL_DIR"
}

function startNonTPMRpCore()
{
    /usr/local/bin/vrtm stop
	echo "Starting non-TPM RPCORE...."
	/usr/local/bin/vrtm start

    /usr/local/bin/rplistener stop
	echo "Starting rp_listener...."
	/usr/local/bin/rplistener start
}

function createvRTMStartScript()
{

	if [ $FLAVOUR = "ubuntu" ] ;then 
		export	LIBVIRT_SERVICE_NAME="libvirt-bin"
	elif [ $FLAVOUR = "rhel" -o $FLAVOUR = "fedora" -o $FLAVOUR = "suse" ] ;then
		export LIBVIRT_SERVICE_NAME="libvirtd"
	fi

	VRTM_SCRIPT="$INSTALL_DIR/rpcore/scripts/vrtm.sh"
	echo "Creating the startup script.... $VRTM_SCRIPT"
	touch $VRTM_SCRIPT 
	echo "#!/bin/bash

### BEGIN INIT INFO
# Provides:          vrtm
# Required-Start:    \$local_fs \$network \$remote_fs
# Required-Stop:     \$local_fs \$network \$remote_fs
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Should-Start:    $LIBVIRT_SERVICE_NAME
# Should-Stop:	     $LIBVIRT_SERVICE_NAME
# Short-Description: VRTM
# Description:       Virtual Root Trust Management
### END INIT INFO

	startVrtm()
	{
		chown -R nova:nova /var/run/libvirt/
	        export RPCORE_IPADDR=$CURRENT_IP
        	export RPCORE_PORT=16005
        	cp -r \"$INSTALL_DIR/rpcore/rptmp\" /tmp
		cp /tmp/rptmp/config/TrustedOS/privatekey /tmp/rptmp/config/TrustedOS/privatekey.pem
        	cd \"$INSTALL_DIR/rpcore/bin/debug\"
        	nohup ./nontpmrpcore >> $LOG_DIR/nontpmrpcore.log 2>&1 &
		sleep 5
	}
	
	case \"\$1\" in
	 start)
	    pgrep nontpmrpcore
	    if [ \$? -ne 0 ] ; then
	        echo \"Starting vrtm...\"
	        startVrtm
	    else
	        echo \"VRTM is already running...\"
	    fi  
	   ;;
	 stop)
	        echo \"Stopping all vrtm processes (if any ) ...\"
	        pkill -9 nontpmrpcore
	   ;;
	 *)
	   echo \"Usage: {start|stop}\" >&2
	   exit 3
	   ;;
	esac
	" > "$VRTM_SCRIPT"
	chmod +x "$VRTM_SCRIPT"
	rm -rf /usr/local/bin/vrtm
	ln -s "$VRTM_SCRIPT" /usr/local/bin/vrtm

	RP_LISTNER_SCRIPT="$INSTALL_DIR/rpcore/scripts/rp_listener.sh"
	echo "Creating the startup script.... $RP_LISTNER_SCRIPT"
	touch $RP_LISTNER_SCRIPT
	echo "#!/bin/bash

### BEGIN INIT INFO
# Provides:          rplistener
# Required-Start:    \$all
# Required-Stop:     \$all
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Should-Start:    $LIBVIRT_SERVICE_NAME
# Should-Stop:      $LIBVIRT_SERVICE_NAME
# Short-Description: rp_listener
# Description:       rp_listener
### END INIT INFO

   RPLISTENER_PID_FILE=/var/run/rplistener.pid

    startRpListner()
    {
 		export RPCORE_IPADDR=$CURRENT_IP
        export RPCORE_PORT=16005
        export LD_LIBRARY_PATH=\"$INSTALL_DIR/rpcore/lib:$LD_LIBRARY_PATH\"
        cd \"$INSTALL_DIR/rpcore/bin/debug\"
        nohup ./rp_listener >> $LOG_DIR/rp_listener.log 2>&1 &
		echo \$! > \$RPLISTENER_PID_FILE
    }
    installMonitFile()
	{
		 if [ -e /etc/monit/conf.d/rplistener.monit ] ; then
			 echo \"INFO : monitor file for rp_listener already present\"
		 else
	         if [ -d /etc/monit/conf.d ] ; then
    	         echo \"INFO : monit conf dir already exists\"
        	 else
            	 echo \"WARN : monit dir was not existing, is monit installed with trust agent installed ?\"
	             mkdir -p /etc/monit/conf.d
    	     fi
			 cp \"$INSTALL_DIR/rpcore/scripts/rplistener.monit\" /etc/monit/conf.d/.
		  	 service monit restart > /dev/null 2>&1 &
		 fi
	}
	case \"\$1\" in
         start)
            pgrep rp_listener
            if [ \$? -ne 0 ] ; then
                echo \"Starting rp_listener...\"
                startRpListner
            else
                echo \"RPListner already running...\"
            fi  
			installMonitFile
           ;;
         stop)
                echo \"Stopping all rp_listener processes and its monitor (if any) ...\"
                pkill -9 rp_listener
				echo \"INFO : Removing pid file for rp_listener\"
				rm -rf \$RPLISTENER_PID_FILE
				echo \"INFO : Removing monitor file for rp_listener\"
				rm -rf /etc/monit/conf.d/rplistener.monit
				service monit restart > /dev/null 2>&1 &
           ;;
         *)
           echo \"Usage: {start|stop}\" 
           exit 3
           ;;
        esac
        " > "$RP_LISTNER_SCRIPT"
        chmod +x "$RP_LISTNER_SCRIPT"
        rm -rf /usr/local/bin/rplistener
        ln -s "$RP_LISTNER_SCRIPT" /usr/local/bin/rplistener
}


function patchOpenstackComputePkgs()
{
	cd "$OPENSTACK_DIR"
	chmod +x "$INSTALL_DIR/Openstack_applyPatches.sh"
	"$INSTALL_DIR/Openstack_applyPatches.sh" --compute
	cd "$INSTALL_DIR"
}

function validate()
{
	# Validate the following 
	# qemu-kvm is libvirt 1.2.2 is installed
	# nova-compute is installed
	# checks for xenbr0 interface

	# Validate qemu-kmv installation	
	if [ ! -e $QEMU_INSTALL_LOCATION ] ; then
		echo "ERROR : Could not find $QEMU_INSTALL_LOCATION installed on this machine"
		echo "Please install qemu kvm"
		exit
	fi
	
	# Validate xenbr0 installation
	ip addr | grep -i -c xenbr0 > /dev/null 
        if [ $? -ne 0 ]; then
                echo "ERROR : xenbr0 device not available, please setup xenbr0 over this machine"
                exit
        fi
	
}

function main_default()
{
  if [ -z "$INSTALL_DIR" ]; then
    INSTALL_DIR="$DEFAULT_INSTALL_DIR"
  fi
  mkdir -p "$INSTALL_DIR"
  mkdir -p "$LOG_DIR" 
  chmod 777 "$LOG_DIR"
 
  if ! valid_ip $CURRENT_IP; then
    while : ; do
      echo "Please enter current machine IP"
      read CURRENT_IP
      if valid_ip $CURRENT_IP; then break; else echo "Incorrect IP format : Please Enter Again"; fi
    done
  fi

	FLAVOUR=`getFlavour`
	updateFlavourVariables
        cd "$INSTALL_DIR"

        echo "Installing pre-requisites ..."
        installKVMPackages

	echo "Untarring Resources ..."
        untarResources

        echo "Installing libvirt ..."
        installLibvirt

	echo "Validating installation ... "
	validate

	echo "Creating VRTM startup scripts"
	createvRTMStartScript

	echo "Installing nontpmrpcore ..."

	echo "Installing RPProxy and RPListener..."
	installRPProxyAndListner
	
	startNonTPMRpCore

    #verifier symlink
    tbootxmVerifier="/opt/tbootxm/bin/verifier"
    vrtmVerifier="$INSTALL_DIR/rpcore/bin/debug/verifier"
    if [ ! -f "$tbootxmVerifier" ]; then
      echo "Could not find $tbootxmVerifier"
    fi
    if [ -f "$vrtmVerifier" ]; then
      rm -f "$vrtmVerifier"
    fi
    ln -s "$tbootxmVerifier" "$vrtmVerifier"

    echo "Install completed successfully !"
}

function help_display()
{
	echo "Usage : ./vRTM_KVM_install.sh [Options]"
        echo "This script creates the installer tar for RPCore"
        echo "    default : Installs RPCore components"
	echo "	  --with-libvirt : This searches and installs the libvirt version "
	echo "			 packaged along with vRTM dist"
	exit
}

MY_SCRIPT_NAME=$0

if [ "$#" -gt 1 ] ; then
	help_display
fi

if [ "$1" == "--help" ] ; then
       help_display
elif [ "$1" == "--with-libvirt" ] ; then
	BUILD_LIBVIRT="TRUE"
	main_default
else
	echo "Installing RPCore components and applies patch for Openstack compute"
	main_default
fi


