cmd_/root/kila/kila.ko := ld -r -m elf_x86_64 -z max-page-size=0x200000 -T ./scripts/module-common.lds --build-id  -o /root/kila/kila.ko /root/kila/kila.o /root/kila/kila.mod.o ;  true
