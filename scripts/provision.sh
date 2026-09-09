#!/usr/bin/env bash
#
# Fast provisioner for the KFS build VM: just the packages needed to build
# and run the kernels. Runs in a couple of minutes.
#
#   make          -> uses the host gcc in -m32 mode (fine for these kernels)
#   make + i686-elf cross toolchain -> run  sudo scripts/build-cross.sh  once
#                                      (~30-45 min; adds /opt/cross to PATH)

set -euo pipefail
export DEBIAN_FRONTEND=noninteractive

echo "==> apt packages"
apt-get update -y
apt-get install -y --no-install-recommends \
    build-essential gcc-multilib \
    nasm \
    grub-pc-bin grub-common xorriso mtools \
    qemu-system-x86 \
    make file curl ca-certificates xz-utils \
    bison flex texinfo libgmp3-dev libmpc-dev libmpfr-dev libisl-dev

cat <<'EOF'

==> ready.

  cd /home/vagrant/projects/kfs_2   (or kfs3)
  make              # build build/kernel.iso  (host gcc -m32)
  make run-curses   # boot it in QEMU, in this terminal

  # optional: the i686-elf cross toolchain the subject recommends
  sudo /home/vagrant/projects/kfs3/scripts/build-cross.sh
EOF
