# compilador cross para RISC-V
CXX      = riscv64-linux-gnu-g++

CXXFLAGS = -std=c++20 -Wall -Wextra -Iinclude -pthread -static
LDFLAGS  = -lpthread -static

# Define o target padrão ao executar `make` sem argumentos
.DEFAULT_GOAL := run

# binários
VEHICLE   = build/vehicle
RSU       = build/rsu
SHM_TEST  = build/shm_test

# sistema de arquivos base da VM (busybox pré compilado)
BUSYBOX_INSTALL = env/initramfs
# kernel RISC-V pré-compilado
KERNEL_IMAGE = env/Image

# fontes do kernel para compilar módulos
KDIR := $(HOME)/linux-6.15.5
KERNEL_MODULE = kernel/position.ko

# sistema de arquivos empacotado que a VM carrega na inicialização
INITRAMFS = initramfs.cpio

# parâmetros comuns do QEMU
QEMU = qemu-system-riscv64 -m 128M -M virt -nographic \
       -kernel $(KERNEL_IMAGE) -initrd $(INITRAMFS) \
       --append "root=/dev/ram"

# rede virtual compartilhada por todas as VMs (mesma VLAN/mcast)
NETDEV = -netdev socket,id=net0,mcast=230.0.0.1:1234

.PHONY: all clean initramfs run shm_test \
        vm1 vm2 vm3 vm4 vm_rsu

# --------------------------------------------------------------------------
# Targets de compilação
# --------------------------------------------------------------------------

all: $(VEHICLE) $(RSU)

# binário do veículo (cross-compilado para RISC-V)
$(VEHICLE): app/vehicle_main.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

# binário da RSU (cross-compilado para RISC-V)
$(RSU): app/rsu_main.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

# teste de shm — compilado para a arquitetura nativa do host (sem cross)
# útil para validar ShmEngine sem precisar subir QEMU
$(SHM_TEST): app/shm_test.cpp
	@mkdir -p build
	g++ -std=c++20 -Wall -Wextra -Iinclude -pthread $^ -o $@ -lpthread

shm_test: $(SHM_TEST)
	@echo ">>> executando shm_test (requer permissão para IPC SysV)"
	sudo ./$(SHM_TEST)

# --------------------------------------------------------------------------
# Initramfs — empacota AMBOS os binários (vehicle e rsu)
# Cada VM decide qual rodar via /init ou via append na linha de boot.
# --------------------------------------------------------------------------

$(KERNEL_MODULE):
	@if [ ! -f kernel/position.ko ]; then \
		rsync -a --exclude='*.ko' --exclude='*.o' kernel/ /tmp/so2_kernel_src/; \
		make -C $(KDIR) M=/tmp/so2_kernel_src ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- modules; \
		cp /tmp/so2_kernel_src/position.ko kernel/; \
	fi

initramfs: all $(KERNEL_MODULE)
	cp $(VEHICLE) $(BUSYBOX_INSTALL)/
	cp $(RSU)     $(BUSYBOX_INSTALL)/
	cp $(KERNEL_MODULE) $(BUSYBOX_INSTALL)/
	cd $(BUSYBOX_INSTALL) && find . | cpio -o -H newc > ../../$(INITRAMFS)


# --------------------------------------------------------------------------
# Targets de VM
# --------------------------------------------------------------------------
# Topologia padrão: 1 RSU + 4 veículos = 5 VMs.
# A RSU sobe primeiro em background, depois os veículos.
# O último veículo fica em foreground para você ver os logs.
# MACs: RSU = 00:...:FF, veículos = 00:...:01..04

run: initramfs
	@echo ">>> subindo 4 RSUs em background..."
	@for i in 0 1 2 3; do \
		$(QEMU) $(NETDEV) \
			-device virtio-net-device,netdev=net0,mac=00:00:00:00:00:FC \
			--append "root=/dev/ram role=rsu quadrant=$$i" \
			> rsu$$i.log 2>&1 & \
	done
	@sleep 1
	@echo ">>> subindo veículos 1..3 em background..."
	@for i in 1 2 3; do \
		$(QEMU) $(NETDEV) \
			-device virtio-net-device,netdev=net0,mac=00:00:00:00:00:0$$i \
			--append "root=/dev/ram role=vehicle" \
			> vehicle$$i.log 2>&1 & \
	done
	@sleep 1
	@echo ">>> subindo veículo 4 em foreground"
	$(QEMU) $(NETDEV) \
		-device virtio-net-device,netdev=net0,mac=00:00:00:00:00:04 \
		--append "root=/dev/ram role=vehicle"

# VMs individuais — cada uma em terminal separado

vm_rsu: initramfs
	$(QEMU) $(NETDEV) \
		-device virtio-net-device,netdev=net0,mac=00:00:00:00:00:FF \
		--append "root=/dev/ram role=rsu"

vm1: initramfs
	$(QEMU) $(NETDEV) \
		-device virtio-net-device,netdev=net0,mac=00:00:00:00:00:01 \
		--append "root=/dev/ram role=vehicle"

vm2: initramfs
	$(QEMU) $(NETDEV) \
		-device virtio-net-device,netdev=net0,mac=00:00:00:00:00:02 \
		--append "root=/dev/ram role=vehicle"

vm3: initramfs
	$(QEMU) $(NETDEV) \
		-device virtio-net-device,netdev=net0,mac=00:00:00:00:00:03 \
		--append "root=/dev/ram role=vehicle"

vm4: initramfs
	$(QEMU) $(NETDEV) \
		-device virtio-net-device,netdev=net0,mac=00:00:00:00:00:04 \
		--append "root=/dev/ram role=vehicle"

# --------------------------------------------------------------------------
# Target PTP — compila com prints de debug do PTP
# --------------------------------------------------------------------------

ptp: 
	$(MAKE) CXXFLAGS="$(CXXFLAGS) -DDEBUG_PTP" run

# --------------------------------------------------------------------------
clean:
	rm -rf build $(INITRAMFS) rsu*.log vehicle*.log

fix_multipass:
	@echo ">>> corrigindo permissões do Multipass (requer sudo)"
	@chmod -R 755 ~/trabalho-so2
	@sudo chown -R ubuntu:ubuntu ~/trabalho-so2