# compilador cross para RISC-V
CXX      = riscv64-linux-gnu-g++

CXXFLAGS = -std=c++20 -Wall -Wextra -Iinclude -pthread -static
LDFLAGS  = -lpthread -static

# binários
VEHICLE   = build/vehicle
SHM_TEST  = build/shm_test

# sistema de arquivos base da VM (busybox pré compilado)
BUSYBOX_INSTALL = env/initramfs
# kernel RISC-V pré-compilado
KERNEL_IMAGE = env/Image
# sistema de arquivos empacotado que a VM carrega na inicialização
INITRAMFS = initramfs.cpio

# fontes comuns às engines (compilados junto por ser header-only ou .cpp)
ENGINE_SRCS = src/engine/raw_socket_engine.cpp \
              src/engine/shm_engine.cpp

.PHONY: all clean initramfs run shm_test \
        vm1 vm2 vm3 vm4 vm5 vm_responder vm_rtt

# --------------------------------------------------------------------------
# Targets de compilação
# --------------------------------------------------------------------------

all: $(VEHICLE)

# binário principal do veículo (cross-compilado para RISC-V)
$(VEHICLE): app/vehicle_main.cpp $(ENGINE_SRCS)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

# teste de shm — compilado para a arquitetura nativa do host (sem cross)
# útil para validar ShmEngine sem precisar subir QEMU
$(SHM_TEST): app/shm_test.cpp $(ENGINE_SRCS)
	@mkdir -p build
	g++ -std=c++20 -Wall -Wextra -Iinclude -pthread $^ -o $@ -lpthread

shm_test: $(SHM_TEST)
	@echo ">>> executando shm_test (requer permissão para IPC SysV)"
	sudo ./$(SHM_TEST)

# --------------------------------------------------------------------------
# Targets de VM
# --------------------------------------------------------------------------

# Copia o binário para dentro do BusyBox e reempacota o initramfs
initramfs: all
	cp $(VEHICLE) $(BUSYBOX_INSTALL)/
	cd $(BUSYBOX_INSTALL) && find . | cpio -o -H newc > ../../$(INITRAMFS)

# Sobe 5 VMs em modo normal (background), cada uma com MAC único
run: initramfs
	@for i in 1 2 3 4; do \
		qemu-system-riscv64 -m 128M -M virt -nographic \
			-kernel $(KERNEL_IMAGE) -initrd $(INITRAMFS) \
			--append "root=/dev/ram" \
			-netdev socket,id=net0,mcast=230.0.0.1:1234 \
			-device virtio-net-device,netdev=net0,mac=00:00:00:00:00:0$$i \
			> /dev/null 2>&1 & \
	done
	qemu-system-riscv64 -m 128M -M virt -nographic \
		-kernel $(KERNEL_IMAGE) -initrd $(INITRAMFS) \
		--append "root=/dev/ram" \
		-netdev socket,id=net0,mcast=230.0.0.1:1234 \
		-device virtio-net-device,netdev=net0,mac=00:00:00:00:00:05

# VMs individuais — cada uma em terminal separado
vm1: initramfs
	qemu-system-riscv64 -m 128M -M virt -nographic \
		-kernel $(KERNEL_IMAGE) -initrd $(INITRAMFS) \
		--append "root=/dev/ram" \
		-netdev socket,id=net0,mcast=230.0.0.1:1234 \
		-device virtio-net-device,netdev=net0,mac=00:00:00:00:00:01

vm2: initramfs
	qemu-system-riscv64 -m 128M -M virt -nographic \
		-kernel $(KERNEL_IMAGE) -initrd $(INITRAMFS) \
		--append "root=/dev/ram" \
		-netdev socket,id=net0,mcast=230.0.0.1:1234 \
		-device virtio-net-device,netdev=net0,mac=00:00:00:00:00:02

vm3: initramfs
	qemu-system-riscv64 -m 128M -M virt -nographic \
		-kernel $(KERNEL_IMAGE) -initrd $(INITRAMFS) \
		--append "root=/dev/ram" \
		-netdev socket,id=net0,mcast=230.0.0.1:1234 \
		-device virtio-net-device,netdev=net0,mac=00:00:00:00:00:03

vm4: initramfs
	qemu-system-riscv64 -m 128M -M virt -nographic \
		-kernel $(KERNEL_IMAGE) -initrd $(INITRAMFS) \
		--append "root=/dev/ram" \
		-netdev socket,id=net0,mcast=230.0.0.1:1234 \
		-device virtio-net-device,netdev=net0,mac=00:00:00:00:00:04

vm5: initramfs
	qemu-system-riscv64 -m 128M -M virt -nographic \
		-kernel $(KERNEL_IMAGE) -initrd $(INITRAMFS) \
		--append "root=/dev/ram" \
		-netdev socket,id=net0,mcast=230.0.0.1:1234 \
		-device virtio-net-device,netdev=net0,mac=00:00:00:00:00:05

# VM respondedora para teste de RTT — responde "pong" a cada "ping"
vm_responder: initramfs
	qemu-system-riscv64 -m 128M -M virt -nographic \
		-kernel $(KERNEL_IMAGE) -initrd $(INITRAMFS) \
		--append "root=/dev/ram mode=responder" \
		-netdev socket,id=net0,mcast=230.0.0.1:1234 \
		-device virtio-net-device,netdev=net0,mac=00:00:00:00:00:01

# VM medidora de RTT — envia ping e mede round-trip time
vm_rtt: initramfs
	qemu-system-riscv64 -m 128M -M virt -nographic \
		-kernel $(KERNEL_IMAGE) -initrd $(INITRAMFS) \
		--append "root=/dev/ram mode=rtt" \
		-netdev socket,id=net0,mcast=230.0.0.1:1234 \
		-device virtio-net-device,netdev=net0,mac=00:00:00:00:00:02

# --------------------------------------------------------------------------
clean:
	rm -rf build $(INITRAMFS)