CC := x86_64-w64-mingw32-gcc
DLLTOOL := x86_64-w64-mingw32-dlltool
OBJDUMP := x86_64-w64-mingw32-objdump
WCC := x86_64-w64-mingw32-gcc

CFLAGS := -O0 -Wall -Wextra -Werror -ffreestanding -fno-stack-protector
LDFLAGS := -nostdlib -nodefaultlibs -nostartfiles \
	-Wl,--subsystem,native -Wl,--entry,DriverEntry -Wl,--image-base,0x140000000

MATRIX_DRIVERS := \
	ksd-mov8.sys ksd-mov16.sys ksd-mov32.sys ksd-mov64.sys \
	ksd-movzx8.sys ksd-movzx16.sys ksd-or32.sys ksd-xor32.sys \
	ksd-cmp32.sys ksd-test32.sys
DRIVERS := kuser-control.sys kuser-004.sys kuser-320.sys zwload-control.sys zwload-kuser.sys secure-probe.sys link-probe.sys $(MATRIX_DRIVERS)

.PHONY: all clean inspect

all: $(DRIVERS)

libntoskrnl.a: ntoskrnl.def
	$(DLLTOOL) --machine i386:x86-64 --input-def $< --output-lib $@

kuser-control.sys: kuser_probe.c libntoskrnl.a
	$(CC) $(CFLAGS) -DPROBE_WIDTH=0 $(LDFLAGS) -o $@ $< -L. -lntoskrnl

kuser-004.sys: kuser_probe.c libntoskrnl.a
	$(CC) $(CFLAGS) -DPROBE_OFFSET=0x004 -DPROBE_WIDTH=32 $(LDFLAGS) -o $@ $< -L. -lntoskrnl

kuser-320.sys: kuser_probe.c libntoskrnl.a
	$(CC) $(CFLAGS) -DPROBE_OFFSET=0x320 -DPROBE_WIDTH=64 $(LDFLAGS) -o $@ $< -L. -lntoskrnl

zwload-control.sys: zwload_probe.c libntoskrnl.a
	$(CC) $(CFLAGS) -DPROBE_KUSER=0 $(LDFLAGS) -o $@ $< -L. -lntoskrnl

zwload-kuser.sys: zwload_probe.c libntoskrnl.a
	$(CC) $(CFLAGS) -DPROBE_KUSER=1 $(LDFLAGS) -o $@ $< -L. -lntoskrnl

secure-probe.sys: secure_probe.c libntoskrnl.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< -L. -lntoskrnl

secure-client.exe: secure_client.c
	$(WCC) -O2 -Wall -Wextra -Werror -o $@ $< -lkernel32

secure-deny-client.exe: secure-deny-client.c
	$(WCC) -O2 -Wall -Wextra -Werror -o $@ $< -ladvapi32

secure-matrix-client.exe: secure-matrix-client.c
	$(WCC) -O2 -Wall -Wextra -Werror -o $@ $< -ladvapi32

link-probe.sys: link_probe.c libntoskrnl.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< -L. -lntoskrnl

link-client.exe: link_client.c
	$(WCC) -O2 -Wall -Wextra -Werror -o $@ $< -lkernel32

ksd-mov8.sys: instruction_probe.c libntoskrnl.a
	$(CC) $(CFLAGS) -DPROBE_KIND=1 -DPROBE_OFFSET=0x268 -DPROBE_WIDTH=8 -DPROBE_NAME='"mov8"' $(LDFLAGS) -o $@ $< -L. -lntoskrnl

ksd-mov16.sys: instruction_probe.c libntoskrnl.a
	$(CC) $(CFLAGS) -DPROBE_KIND=2 -DPROBE_OFFSET=0x26a -DPROBE_WIDTH=16 -DPROBE_NAME='"mov16"' $(LDFLAGS) -o $@ $< -L. -lntoskrnl

ksd-mov32.sys: instruction_probe.c libntoskrnl.a
	$(CC) $(CFLAGS) -DPROBE_KIND=3 -DPROBE_OFFSET=0x004 -DPROBE_WIDTH=32 -DPROBE_NAME='"mov32"' $(LDFLAGS) -o $@ $< -L. -lntoskrnl

ksd-mov64.sys: instruction_probe.c libntoskrnl.a
	$(CC) $(CFLAGS) -DPROBE_KIND=4 -DPROBE_OFFSET=0x320 -DPROBE_WIDTH=64 -DPROBE_NAME='"mov64"' $(LDFLAGS) -o $@ $< -L. -lntoskrnl

ksd-movzx8.sys: instruction_probe.c libntoskrnl.a
	$(CC) $(CFLAGS) -DPROBE_KIND=5 -DPROBE_OFFSET=0x268 -DPROBE_WIDTH=8 -DPROBE_NAME='"movzx8"' $(LDFLAGS) -o $@ $< -L. -lntoskrnl

ksd-movzx16.sys: instruction_probe.c libntoskrnl.a
	$(CC) $(CFLAGS) -DPROBE_KIND=6 -DPROBE_OFFSET=0x26a -DPROBE_WIDTH=16 -DPROBE_NAME='"movzx16"' $(LDFLAGS) -o $@ $< -L. -lntoskrnl

ksd-or32.sys: instruction_probe.c libntoskrnl.a
	$(CC) $(CFLAGS) -DPROBE_KIND=7 -DPROBE_OFFSET=0x004 -DPROBE_WIDTH=32 -DPROBE_NAME='"or32"' $(LDFLAGS) -o $@ $< -L. -lntoskrnl

ksd-xor32.sys: instruction_probe.c libntoskrnl.a
	$(CC) $(CFLAGS) -DPROBE_KIND=8 -DPROBE_OFFSET=0x004 -DPROBE_WIDTH=32 -DPROBE_NAME='"xor32"' $(LDFLAGS) -o $@ $< -L. -lntoskrnl

ksd-cmp32.sys: instruction_probe.c libntoskrnl.a
	$(CC) $(CFLAGS) -DPROBE_KIND=9 -DPROBE_OFFSET=0x004 -DPROBE_WIDTH=32 -DPROBE_NAME='"cmp32"' $(LDFLAGS) -o $@ $< -L. -lntoskrnl

ksd-test32.sys: instruction_probe.c libntoskrnl.a
	$(CC) $(CFLAGS) -DPROBE_KIND=10 -DPROBE_OFFSET=0x004 -DPROBE_WIDTH=32 -DPROBE_NAME='"test32"' $(LDFLAGS) -o $@ $< -L. -lntoskrnl

inspect: all
	$(OBJDUMP) -p $(DRIVERS)
	$(OBJDUMP) -d $(DRIVERS)

clean:
	rm -f -- $(DRIVERS) libntoskrnl.a
