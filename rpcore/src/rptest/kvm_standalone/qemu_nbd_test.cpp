#include "stdio.h"

int main(int argc, char** argv) {
    char sz_qemu_nbd_attach[1024];
    printf("qemu-nbd -c %s %s\n", "/dev/nbd0", "/root/ubuntu-12.04.img");
    //return system(sz_qemu_nbd_attach);
}
