apt update
apt install -y git qemu-system-x86 make gcc
export DISPLAY=:0
make && make qemu