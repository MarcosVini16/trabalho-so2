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

# CPU permissivo: habilita extensões ISA que toolchains modernos (em especial
# Ubuntu ARM64 rodando em Mac/UTM) tendem a usar via libc estática. QEMU virt
# default não emula todas, causando SIGILL no boot. Sem efeito em binários que
# não usam essas extensões (caso x86 nativo), então é seguro deixar ativo
# pra todo mundo do grupo e pro professor.
QEMU_CPU = -cpu rv64,v=true,vext_spec=v1.0,zba=true,zbb=true,zbs=true,zfh=true,zfhmin=true,zicbom=true,zicboz=true,zicond=true,zihintntl=true,zihintpause=true,zfa=true,zca=true,zcb=true,zcd=true

# parâmetros comuns do QEMU (sem --append; cada target define o seu)
QEMU = qemu-system-riscv64 -m 128M -M virt -nographic $(QEMU_CPU) \
       -kernel $(KERNEL_IMAGE) -initrd $(INITRAMFS)

# rede virtual compartilhada por todas as VMs (mesma VLAN/mcast)
NETDEV = -netdev socket,id=net0,mcast=230.0.0.1:1234

# Define um valor padrão caso o Makefile seja chamado direto por `make vm1`
TEST_SCENARIO ?= default

.PHONY: all clean initramfs run shm_test ptp \
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
# Topologia padrão: 4 RSUs (uma por quadrante) + 4 veículos = 8 VMs.
# As RSUs sobem primeiro em background, depois os veículos.
# O último veículo fica em foreground para você ver os logs.

run: initramfs
	@mkdir -p logs/rsu logs/vehicle
	@sh -c ' \
		for i in 0 1 2 3; do \
			mac=$$(printf "00:00:00:00:00:F%x" $$i); \
			$(QEMU) $(NETDEV) \
				-device virtio-net-device,netdev=net0,mac=$$mac \
				--append "root=/dev/ram role=rsu quadrant=$$i timeout=60" \
				> logs/rsu/rsu$$i.log 2>&1 & \
		done; \
		sleep 3; \
		for q in 0 1 2 3; do \
			for i in 1 2; do \
				n=$$(($$q * 5 + $$i + 1)); \
				mac=$$(printf "00:00:00:00:00:%02x" $$n); \
				$(QEMU) $(NETDEV) \
					-device virtio-net-device,netdev=net0,mac=$$mac \
					--append "root=/dev/ram role=vehicle quadrant=$$q timeout=35 TEST_SCENARIO=$(TEST_SCENARIO)" \
					> logs/vehicle/vehicle$$n.log 2>&1 & \
			done; \
		done; \
		sleep 5; \
		for q in 0 1 2 3; do \
			for i in 3 4; do \
				n=$$(($$q * 5 + $$i + 1)); \
				mac=$$(printf "00:00:00:00:00:%02x" $$n); \
				$(QEMU) $(NETDEV) \
					-device virtio-net-device,netdev=net0,mac=$$mac \
					--append "root=/dev/ram role=vehicle quadrant=$$q timeout=25 TEST_SCENARIO=$(TEST_SCENARIO)" \
					> logs/vehicle/vehicle$$n.log 2>&1 & \
			done; \
		done; \
		sleep 10; \
		for q in 0 1 2 3; do \
			n=$$(($$q * 5 + 6)); \
			mac=$$(printf "00:00:00:00:00:%02x" $$n); \
			$(QEMU) $(NETDEV) \
				-device virtio-net-device,netdev=net0,mac=$$mac \
				--append "root=/dev/ram role=vehicle quadrant=$$q timeout=10 TEST_SCENARIO=$(TEST_SCENARIO)" \
				> logs/vehicle/vehicle$$n.log 2>&1 & \
		done; \
	'
	@sleep 1
	@echo ">>> veículo 1 em foreground..."
	$(QEMU) $(NETDEV) \
		-device virtio-net-device,netdev=net0,mac=00:00:00:00:00:01 \
		--append "root=/dev/ram role=vehicle quadrant=0 timeout=40 TEST_SCENARIO=$(TEST_SCENARIO)" \
		| tee logs/vehicle/vehicle1.log

# VMs individuais — cada uma em terminal separado

vm_rsu: initramfs
	$(QEMU) $(NETDEV) \
		-device virtio-net-device,netdev=net0,mac=00:00:00:00:00:FF \
		--append "root=/dev/ram role=rsu"

vm1: initramfs
	$(QEMU) $(NETDEV) \
		-device virtio-net-device,netdev=net0,mac=00:00:00:00:00:01 \
		--append "root=/dev/ram role=vehicle TEST_SCENARIO=$(TEST_SCENARIO)"

vm2: initramfs
	$(QEMU) $(NETDEV) \
		-device virtio-net-device,netdev=net0,mac=00:00:00:00:00:02 \
		--append "root=/dev/ram role=vehicle TEST_SCENARIO=$(TEST_SCENARIO)"

vm3: initramfs
	$(QEMU) $(NETDEV) \
		-device virtio-net-device,netdev=net0,mac=00:00:00:00:00:03 \
		--append "root=/dev/ram role=vehicle TEST_SCENARIO=$(TEST_SCENARIO)"

vm4: initramfs
	$(QEMU) $(NETDEV) \
		-device virtio-net-device,netdev=net0,mac=00:00:00:00:00:04 \
		--append "root=/dev/ram role=vehicle TEST_SCENARIO=$(TEST_SCENARIO)"

# --------------------------------------------------------------------------
# Target PTP — compila com prints de debug do PTP
# --------------------------------------------------------------------------

ptp:
	$(MAKE) CXXFLAGS="$(CXXFLAGS) -DDEBUG_PTP" run

# --------------------------------------------------------------------------
clean:
	rm -rf build $(INITRAMFS) logs/rsu/*.log logs/vehicle/*.log

fix_multipass:
	@echo ">>> corrigindo permissões do Multipass (requer sudo)"
	@chmod -R 755 ~/trabalho-so2
	@sudo chown -R ubuntu:ubuntu ~/trabalho-so2

test_thread:
	@echo ">>> testando periodic_thread (deve imprimir 50 vezes com ~100ms de intervalo)"
	@g++ -std=c++20 thread_test.cpp -o thread_test

test_basic_local: initramfs
	$(MAKE) vm1 TEST_SCENARIO=basic_local

test_silent_producer: initramfs
	$(MAKE) vm1 TEST_SCENARIO=silent_producer

test_fanout_local: initramfs
	$(MAKE) vm1 TEST_SCENARIO=fanout_local
