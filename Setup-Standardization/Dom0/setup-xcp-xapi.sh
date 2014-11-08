function set_keyvaluepair(){
	TARGET_KEY=$1
	REPLACEMENT_VALUE=$2
	CONFIG_FILE=$3
#	echo $REPLACEMENT_VALUE
	sed  -i "s|\($TARGET_KEY *= *\).*|\1$REPLACEMENT_VALUE|" $CONFIG_FILE
}
apt-get update
apt-get --force-yes -y install xcp-xapi
set_keyvaluepair TOOLSTACK xapi /etc/default/xen
update-rc.d xendomains disable
mkdir /usr/share/qemu
ln -s /usr/share/qemu-linaro/keymaps /usr/share/qemu/keymaps
set_keyvaluepair GRUB_DEFAULT "\"Ubuntu GNU/Linux, with Xen hypervisor\"" /etc/default/grub 
set_keyvaluepair GRUB_CMDLINE_LINUX "\"biosdevname=0\"" /etc/default/grub 
update-grub
rm /etc/xcp/network.conf
touch /etc/xcp/network.conf
echo "bridge" > /etc/xcp/network.conf

apt-get --force-yes -y install bc g++ make python-dev tpm-tools tpm-tools-dbg trousers libtspi1 libtspi-dev tboot libxml2-dev libssl-dev cryptsetup-bin
update-grub
echo "Please set network configuration as below and reboot"

echo "auto lo"
echo "iface lo inet loopback"
echo "auto eth0"
echo "auto xenbr0"
echo "iface xenbr0 inet dhcp"
echo "bridge_ports eth0"


