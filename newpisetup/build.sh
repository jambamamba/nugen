#!/bin/bash -xe
set -xe

MOUNTPOINT=/media/oosman/rootfs
SERIALNUMBER=3
DEVICENAME=raspi4

#remove password complexity test
sudo cp resources/common-password $MOUNTPOINT/etc/pam.d/common-password

#wifi script
sudo cp resources/wpa_supplicant.conf $MOUNTPOINT/etc/wpa_supplicant/

#nugen service
sudo cp resources/nugen.service $MOUNTPOINT/etc/systemd/system/
sudo chmod +x $MOUNTPOINT/etc/systemd/system/nugen.service

#discovery service
sudo cp /home/oosman/work/Madusa/DiscoveryService/buildrpi0/discovery.service $MOUNTPOINT/home/pi/
sudo cp resources/discovery.service $MOUNTPOINT/etc/systemd/system/
sudo chmod +x $MOUNTPOINT/etc/systemd/system/discovery.service
echo "serialnumber:$SERIALNUMBER
name:$DEVICENAME" > /tmp/medussa.id
sudo mv /tmp/medussa.id $MOUNTPOINT/usr/local/etc/

#mount usb storage device
cp $MOUNTPOINT/etc/fstab /tmp/fstab
echo "
/dev/disk/by-uuid/31cc5498-c35c-4b67-a23e-31dc5499fc92	/media/usb-drive	ext4	defaults	0	0
" >> /tmp/fstab
sudo cp /tmp/fstab $MOUNTPOINT/etc/fstab

#core dumps
mkdir -p /tmp/cores
chmod a+rwx /tmp/cores
echo "/tmp/cores/core.%e.%p.%h.%t" > /tmp/core_pattern
sudo mv /tmp/core_pattern $MOUNTPOINT/proc/sys/kernel/core_pattern
#todo: need to run this on the pi itself
#ulimit -c unlimited

#lsusb
#Bus 002 Device 003: ID 18d1:9302 Google Inc.
# 18d1:9302  == vendor id : product id

echo "SUBSYSTEM==\"usb\", ATTR{idVendor}==\"1a6e\", ATTR{idProduct}==\"089a\", MODE=\"0664\", GROUP=\"plugdev\", TAG+=\"uaccess\"" > /tmp/edgetpu.rules
echo "SUBSYSTEM==\"usb\", ATTR{idVendor}==\"18d1\", ATTR{idProduct}==\"9302\", MODE=\"0664\", GROUP=\"plugdev\", TAG+=\"uaccess\"" >> /tmp/edgetpu.rules
sudo mv /tmp/edgetpu.rules $MOUNTPOINT/etc/udev/rules.d/65-edgetpu-board.rules
#on device:
#sudo udevadm control --reload-rules

echo "console=serial0,115200 console=tty1 root=PARTUUID=022b4704-02 rootfstype=ext4 elevator=deadline fsck.repair=yes rootwait" /tmp/cmdline.txt
sudo mv /tmp/cmdline.txt $MOUNTPOINT/boot/cmdline.txt

#/boot/config.txt, set gpu_mem=144
sed -i 's/gpu_mem=128/gpu_mem=144/' $MOUNTPOINT/boot/config.txt

#sometimes wlanX has no permission on it, run rfkill to figure out which id#
#sudo rfkill list all
#sudo rfkill unblock <id#>

# on device, run:
#sudo rpi-update

