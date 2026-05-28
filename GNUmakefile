# Nuke built-in rules.
.SUFFIXES:

ARCH := x86_64

QEMUFLAGS := -m 2G -cpu IvyBridge -debugcon stdio -d int

override IMAGE_NAME := eucalypt-$(ARCH)

HOST_CC := cc
HOST_CFLAGS := -g -O2 -pipe
HOST_CPPFLAGS :=
HOST_LDFLAGS :=
HOST_LIBS :=

ARCHIVE := ./archive

.PHONY: all
all: $(IMAGE_NAME).iso

.PHONY: all-hdd
all-hdd: $(IMAGE_NAME).hdd

.PHONY: run
run: run-$(ARCH)

.PHONY: run-hdd
run-hdd: run-hdd-$(ARCH)

.PHONY: run-x86_64
run-x86_64: edk2-ovmf $(IMAGE_NAME).iso
	qemu-system-$(ARCH) \
		-M q35 \
		-drive if=pflash,unit=0,format=raw,file=edk2-ovmf/ovmf-code-$(ARCH).fd,readonly=on \
		-cdrom $(IMAGE_NAME).iso \
		$(QEMUFLAGS)

.PHONY: run-hdd-x86_64
run-hdd-x86_64: edk2-ovmf $(IMAGE_NAME).hdd
	qemu-system-$(ARCH) \
		-M q35 \
		-drive if=pflash,unit=0,format=raw,file=edk2-ovmf/ovmf-code-$(ARCH).fd,readonly=on \
		-hda $(IMAGE_NAME).hdd \
		$(QEMUFLAGS)

.PHONY: run-aarch64
run-aarch64: edk2-ovmf $(IMAGE_NAME).iso
	qemu-system-$(ARCH) \
		-M virt \
		-cpu cortex-a72 \
		-device ramfb \
		-device qemu-xhci \
		-device usb-kbd \
		-device usb-tablet \
		-drive if=pflash,unit=0,format=raw,file=edk2-ovmf/ovmf-code-$(ARCH).fd,readonly=on \
		-cdrom $(IMAGE_NAME).iso \
		$(QEMUFLAGS)

.PHONY: run-hdd-aarch64
run-hdd-aarch64: edk2-ovmf $(IMAGE_NAME).hdd
	qemu-system-$(ARCH) \
		-M virt \
		-cpu cortex-a72 \
		-device ramfb \
		-device qemu-xhci \
		-device usb-kbd \
		-device usb-tablet \
		-drive if=pflash,unit=0,format=raw,file=edk2-ovmf/ovmf-code-$(ARCH).fd,readonly=on \
		-hda $(IMAGE_NAME).hdd \
		$(QEMUFLAGS)

.PHONY: run-riscv64
run-riscv64: edk2-ovmf $(IMAGE_NAME).iso
	qemu-system-$(ARCH) \
		-M virt \
		-cpu rv64 \
		-device ramfb \
		-device qemu-xhci \
		-device usb-kbd \
		-device usb-tablet \
		-drive if=pflash,unit=0,format=raw,file=edk2-ovmf/ovmf-code-$(ARCH).fd,readonly=on \
		-cdrom $(IMAGE_NAME).iso \
		$(QEMUFLAGS)

.PHONY: run-hdd-riscv64
run-hdd-riscv64: edk2-ovmf $(IMAGE_NAME).hdd
	qemu-system-$(ARCH) \
		-M virt \
		-cpu rv64 \
		-device ramfb \
		-device qemu-xhci \
		-device usb-kbd \
		-device usb-tablet \
		-drive if=pflash,unit=0,format=raw,file=edk2-ovmf/ovmf-code-$(ARCH).fd,readonly=on \
		-hda $(IMAGE_NAME).hdd \
		$(QEMUFLAGS)

.PHONY: run-loongarch64
run-loongarch64: edk2-ovmf $(IMAGE_NAME).iso
	qemu-system-$(ARCH) \
		-M virt \
		-cpu la464 \
		-device ramfb \
		-device qemu-xhci \
		-device usb-kbd \
		-device usb-tablet \
		-drive if=pflash,unit=0,format=raw,file=edk2-ovmf/ovmf-code-$(ARCH).fd,readonly=on \
		-cdrom $(IMAGE_NAME).iso \
		$(QEMUFLAGS)

.PHONY: run-hdd-loongarch64
run-hdd-loongarch64: edk2-ovmf $(IMAGE_NAME).hdd
	qemu-system-$(ARCH) \
		-M virt \
		-cpu la464 \
		-device ramfb \
		-device qemu-xhci \
		-device usb-kbd \
		-device usb-tablet \
		-drive if=pflash,unit=0,format=raw,file=edk2-ovmf/ovmf-code-$(ARCH).fd,readonly=on \
		-hda $(IMAGE_NAME).hdd \
		$(QEMUFLAGS)

.PHONY: run-bios
run-bios: $(IMAGE_NAME).iso
	qemu-system-$(ARCH) \
		-M q35 \
		-cdrom $(IMAGE_NAME).iso \
		-boot d \
		$(QEMUFLAGS)

.PHONY: run-hdd-bios
run-hdd-bios: $(IMAGE_NAME).hdd
	qemu-system-$(ARCH) \
		-M q35 \
		-hda $(IMAGE_NAME).hdd \
		$(QEMUFLAGS)

.PHONY: archive
archive:
	$(MAKE) -C archive ARCH=$(ARCH)

.PHONY: create_archive
create_archive: archive
	tar --format=ustar -cf main_archive.tar -C archive main

edk2-ovmf:
	curl -L https://github.com/osdev0/edk2-ovmf-nightly/releases/latest/download/edk2-ovmf.tar.gz | gunzip | tar -xf -

limine-binary/limine:
	rm -rf limine-binary
	curl -L https://github.com/Limine-Bootloader/Limine/releases/latest/download/limine-binary.tar.gz | gunzip | tar -xf -

	$(MAKE) -C limine-binary \
		CC="$(HOST_CC)" \
		CFLAGS="$(HOST_CFLAGS)" \
		CPPFLAGS="$(HOST_CPPFLAGS)" \
		LDFLAGS="$(HOST_LDFLAGS)" \
		LIBS="$(HOST_LIBS)"

kernel/.deps-obtained:
	./kernel/get-deps

.PHONY: kernel
kernel: kernel/.deps-obtained
	$(MAKE) -C kernel ARCH=$(ARCH)

.PHONY: disks
disks:
	mkdir -p disks

	rm -f disks/ram.img
	mkfs.fat -F 12 -C disks/ram.img 4096

	$(foreach f,$(wildcard z_files_to_copy/*), \
		mcopy -i disks/ram.img $(f) ::/$(notdir $(f));)

	rm -f disks/ide_disk.img disks/ahci_disk.img

	mkfs.fat -F 12 -C disks/ide_disk.img 32768
	mkfs.fat -F 12 -C disks/ahci_disk.img 32768

	$(foreach f,$(wildcard z_files_to_copy/*), \
		mcopy -i disks/ide_disk.img $(f) ::/$(notdir $(f));)

	$(foreach f,$(wildcard z_files_to_copy/*), \
		mcopy -i disks/ahci_disk.img $(f) ::/$(notdir $(f));)

$(IMAGE_NAME).iso: limine-binary/limine kernel archive disks create_archive
	rm -rf iso_root

	mkdir -p iso_root/boot
	cp -v kernel/bin-$(ARCH)/kernel iso_root/boot/

	mkdir -p iso_root/boot/limine
	cp -v limine.conf iso_root/boot/limine/

	mkdir -p iso_root/EFI/BOOT

	mkdir -p iso_root/archive
	cp -v main_archive.tar iso_root/archive/

ifeq ($(ARCH),x86_64)
	cp -v \
		limine-binary/limine-bios.sys \
		limine-binary/limine-bios-cd.bin \
		limine-binary/limine-uefi-cd.bin \
		iso_root/boot/limine/

	cp -v limine-binary/BOOTX64.EFI iso_root/EFI/BOOT/
	cp -v limine-binary/BOOTIA32.EFI iso_root/EFI/BOOT/

	xorriso -as mkisofs \
		-R -r -J \
		-b boot/limine/limine-bios-cd.bin \
		-no-emul-boot \
		-boot-load-size 4 \
		-boot-info-table \
		-hfsplus \
		-apm-block-size 2048 \
		--efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part \
		--efi-boot-image \
		--protective-msdos-label \
		iso_root \
		-o $(IMAGE_NAME).iso

	./limine-binary/limine bios-install $(IMAGE_NAME).iso
endif

ifeq ($(ARCH),aarch64)
	cp -v limine-binary/limine-uefi-cd.bin iso_root/boot/limine/
	cp -v limine-binary/BOOTAA64.EFI iso_root/EFI/BOOT/

	xorriso -as mkisofs \
		-R -r -J \
		-hfsplus \
		-apm-block-size 2048 \
		--efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part \
		--efi-boot-image \
		--protective-msdos-label \
		iso_root \
		-o $(IMAGE_NAME).iso
endif

ifeq ($(ARCH),riscv64)
	cp -v limine-binary/limine-uefi-cd.bin iso_root/boot/limine/
	cp -v limine-binary/BOOTRISCV64.EFI iso_root/EFI/BOOT/

	xorriso -as mkisofs \
		-R -r -J \
		-hfsplus \
		-apm-block-size 2048 \
		--efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part \
		--efi-boot-image \
		--protective-msdos-label \
		iso_root \
		-o $(IMAGE_NAME).iso
endif

ifeq ($(ARCH),loongarch64)
	cp -v limine-binary/limine-uefi-cd.bin iso_root/boot/limine/
	cp -v limine-binary/BOOTLOONGARCH64.EFI iso_root/EFI/BOOT/

	xorriso -as mkisofs \
		-R -r -J \
		-hfsplus \
		-apm-block-size 2048 \
		--efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part \
		--efi-boot-image \
		--protective-msdos-label \
		iso_root \
		-o $(IMAGE_NAME).iso
endif

	rm -rf iso_root

$(IMAGE_NAME).hdd: limine-binary/limine kernel archive create_archive
	rm -f $(IMAGE_NAME).hdd

	dd if=/dev/zero bs=1M count=0 seek=64 of=$(IMAGE_NAME).hdd

ifeq ($(ARCH),x86_64)
	PATH=$$PATH:/usr/sbin:/sbin \
	sgdisk $(IMAGE_NAME).hdd -n 1:2048 -t 1:ef00 -m 1

	./limine-binary/limine bios-install $(IMAGE_NAME).hdd
else
	PATH=$$PATH:/usr/sbin:/sbin \
	sgdisk $(IMAGE_NAME).hdd -n 1:2048 -t 1:ef00
endif

	mformat -i $(IMAGE_NAME).hdd@@1M

	mmd -i $(IMAGE_NAME).hdd@@1M \
		::/EFI \
		::/EFI/BOOT \
		::/boot \
		::/boot/limine \
		::/archive

	mcopy -i $(IMAGE_NAME).hdd@@1M \
		kernel/bin-$(ARCH)/kernel ::/boot

	mcopy -i $(IMAGE_NAME).hdd@@1M \
		main_archive.tar ::/archive

	mcopy -i $(IMAGE_NAME).hdd@@1M \
		limine.conf ::/boot/limine

ifeq ($(ARCH),x86_64)
	mcopy -i $(IMAGE_NAME).hdd@@1M \
		limine-binary/limine-bios.sys ::/boot/limine

	mcopy -i $(IMAGE_NAME).hdd@@1M \
		limine-binary/BOOTX64.EFI ::/EFI/BOOT

	mcopy -i $(IMAGE_NAME).hdd@@1M \
		limine-binary/BOOTIA32.EFI ::/EFI/BOOT
endif

ifeq ($(ARCH),aarch64)
	mcopy -i $(IMAGE_NAME).hdd@@1M \
		limine-binary/BOOTAA64.EFI ::/EFI/BOOT
endif

ifeq ($(ARCH),riscv64)
	mcopy -i $(IMAGE_NAME).hdd@@1M \
		limine-binary/BOOTRISCV64.EFI ::/EFI/BOOT
endif

ifeq ($(ARCH),loongarch64)
	mcopy -i $(IMAGE_NAME).hdd@@1M \
		limine-binary/BOOTLOONGARCH64.EFI ::/EFI/BOOT
endif

.PHONY: clean
clean:
	$(MAKE) -C kernel clean
	$(MAKE) -C archive clean

	rm -rf \
		iso_root \
		$(IMAGE_NAME).iso \
		$(IMAGE_NAME).hdd \
		main_archive.tar

.PHONY: distclean
distclean:
	$(MAKE) -C kernel distclean
	$(MAKE) -C archive clean

	rm -rf \
		iso_root \
		*.iso \
		*.hdd \
		limine-binary \
		edk2-ovmf \
		main_archive.tar