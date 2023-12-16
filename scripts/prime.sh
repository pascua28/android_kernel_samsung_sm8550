#!/vendor/bin/sh

exec > /dev/kmsg 2>&1

echo "3" > /sys/fs/selinux/enforce
