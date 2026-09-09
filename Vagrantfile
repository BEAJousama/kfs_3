# -*- mode: ruby -*-
# vi: set ft=ruby :
#
# Build VM for the KFS kernels. The host has no i686-elf cross toolchain, so
# builds happen in here; run the resulting ISO either on the host or inside
# the VM (see below).
#
#   vagrant up                # boot + provision (first run builds cross-gcc)
#   vagrant ssh
#   cd /home/vagrant/projects/kfs_2 && make        # -> build/kernel.iso
#   cd /home/vagrant/projects/kfs3  && make
#
# Run a built ISO:
#   - on the host:  qemu-system-i386 -cdrom build/kernel.iso -m 512
#   - in the VM  :  make run                       (text mode, in the SSH term)
#
# Both kfs_2 and kfs3 are mounted under /home/vagrant/projects.

Vagrant.configure("2") do |config|
  config.vm.box      = "bento/ubuntu-22.04"
  config.vm.hostname = "kfs"

  # ".." is the folder that contains both kfs_2/ and kfs3/
  config.vm.synced_folder "..", "/home/vagrant/projects"

  config.vm.provider "virtualbox" do |vb|
    vb.gui    = false
    vb.name   = "kfs-build"
    vb.memory = 4096
    vb.cpus   = 4
  end

  # Optional: libvirt/KVM instead of VirtualBox  ->  vagrant up --provider=libvirt
  config.vm.provider "libvirt" do |lv|
    lv.memory = 4096
    lv.cpus   = 4
  end

  config.vm.provision "shell", path: "scripts/provision.sh"
end
