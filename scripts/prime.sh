#!/vendor/bin/sh

exec > /dev/kmsg 2>&1

BIND=/vendor/bin/init.kernel.post_boot-kalama.sh

echo "execprog: restarting under tmpfs"

# Run under a new tmpfs to avoid /dev selabel
mkdir /dev/ep
mount -t tmpfs nodev /dev/ep

while [ ! -e "$BIND" ]; do
  if [ -e /tmp/recovery.log ]; then
    echo "execprog: recovery detected"
    # Re-enable SELinux
    echo "3" > /sys/fs/selinux/enforce
    exit
  fi
  sleep 0.1
done

# Setup /dev/ep/execprog
cat "$BIND" > /dev/ep/execprog

echo '
#Additional tweaks will go here
' >> /dev/ep/execprog

rm /dev/execprog
chown root:shell /dev/ep/execprog

mount --bind /dev/ep/execprog "$BIND"
chcon "u:object_r:vendor_file:s0" "$BIND"

# lazy unmount /dev/ep for invisibility
umount -l /dev/ep

echo "3" > /sys/fs/selinux/enforce
