# -*- mode: ruby -*-
# vi: set ft=ruby :
#
# Build/run VM for the KFS kernels.
#
#   vagrant up                     # boot + fast provision (a few minutes)
#   vagrant ssh
#   cd /home/vagrant/projects/kfs3 && make        # -> build/kernel.iso (host gcc -m32)
#   make run-curses                                # boot the kernel in this terminal
#
# Both kfs_2 and kfs3 are mounted under /home/vagrant/projects.
#
# Requires Vagrant >= 2.4 for VirtualBox 7.x  (2.2.x only knows VirtualBox <= 6.1).

Vagrant.configure("2") do |config|
  config.vm.box      = "bento/ubuntu-22.04"
  config.vm.hostname = "kfs"

  # ".." is the folder that holds both kfs_2/ and kfs3/
  config.vm.synced_folder "..", "/home/vagrant/projects"

  config.vm.provider "virtualbox" do |vb|
    vb.gui    = false
    vb.name   = "kfs-build"
    vb.memory = 2048
    vb.cpus   = 2
  end

  # Alternative: KVM  ->  vagrant up --provider=libvirt
  config.vm.provider "libvirt" do |lv|
    lv.memory = 2048
    lv.cpus   = 2
  end

  config.vm.provision "shell", path: "scripts/provision.sh"
end
