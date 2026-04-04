# compilador cross para RISC-V
CXX      = riscv64-linux-gnu-g++

# inclui todas as libs
CXXFLAGS = -std=c++20 -Wall -Wextra -Iinclude -pthread -static
LDFLAGS  = -lpthread -static

# arquivos fonte do projeto
SRCS = src/engine/raw_socket_engine.cpp \
       src/engine/shm_engine.cpp \
       src/components/component.cpp

# arquivos objeto gerados na pela compilação
OBJS     = $(SRCS:.cpp=.o)
# binário final
VEHICLE  = build/vehicle

# sistema de arquivos base da VM (busybox pré compilado)
BUSYBOX_INSTALL = env/initramfs
# kernel RISC-V pré-compilado
KERNEL_IMAGE = env/Image
# sistema de arquivos empacotado que a VM carrega na inicialização
INITRAMFS = initramfs.cpio

.PHONY: all clean initramfs run vm1 vm2 vm3 vm4 vm5

# compilar o binário RISC-V
all: $(VEHICLE)

$(VEHICLE): app/vehicle_main.cpp $(OBJS)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

# compila cada .cpp em .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Copia o binário para dentro do BusyBox e reempacota o initramfs
# O initramfs é o sistema de arquivos que a VM usa ao inicializar
initramfs: all
	cp $(VEHICLE) $(BUSYBOX_INSTALL)/
	cd $(BUSYBOX_INSTALL) && find . | cpio -o -H newc > ../../$(INITRAMFS)

# Compila, empacota o initramfs e sobe 2 VMs QEMU
# As VMs se comunicam via socket multicast no grupo 230.0.0.1:1234
# Cada VM tem um MAC diferente para ser identificada na rede
run: initramfs
	qemu-system-riscv64 -m 128M -M virt -nographic \
		-kernel $(KERNEL_IMAGE) -initrd $(INITRAMFS) \
		--append "root=/dev/ram" \
		-netdev socket,id=net0,mcast=230.0.0.1:1234 \
		-device virtio-net-device,netdev=net0,mac=00:00:00:00:00:01 \
		> /dev/null 2>&1 &
	qemu-system-riscv64 -m 128M -M virt -nographic \
		-kernel $(KERNEL_IMAGE) -initrd $(INITRAMFS) \
		--append "root=/dev/ram" \
		-netdev socket,id=net0,mcast=230.0.0.1:1234 \
		-device virtio-net-device,netdev=net0,mac=00:00:00:00:00:02 \
		> /dev/null 2>&1 &
	qemu-system-riscv64 -m 128M -M virt -nographic \
		-kernel $(KERNEL_IMAGE) -initrd $(INITRAMFS) \
		--append "root=/dev/ram" \
		-netdev socket,id=net0,mcast=230.0.0.1:1234 \
		-device virtio-net-device,netdev=net0,mac=00:00:00:00:00:03 \
		> /dev/null 2>&1 &
	qemu-system-riscv64 -m 128M -M virt -nographic \
		-kernel $(KERNEL_IMAGE) -initrd $(INITRAMFS) \
		--append "root=/dev/ram" \
		-netdev socket,id=net0,mcast=230.0.0.1:1234 \
		-device virtio-net-device,netdev=net0,mac=00:00:00:00:00:04 \
		> /dev/null 2>&1 &
	qemu-system-riscv64 -m 128M -M virt -nographic \
		-kernel $(KERNEL_IMAGE) -initrd $(INITRAMFS) \
		--append "root=/dev/ram" \
		-netdev socket,id=net0,mcast=230.0.0.1:1234 \
		-device virtio-net-device,netdev=net0,mac=00:00:00:00:00:05

# sobe a VM 1 isolada — rode em um terminal separado
vm1: initramfs
	qemu-system-riscv64 -m 128M -M virt -nographic \
		-kernel $(KERNEL_IMAGE) -initrd $(INITRAMFS) \
		--append "root=/dev/ram" \
		-netdev socket,id=net0,mcast=230.0.0.1:1234 \
		-device virtio-net-device,netdev=net0,mac=00:00:00:00:00:01

# sobe a VM 2 isolada — rode em um terminal separado
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

# Remove binários e arquivos gerados
clean:
	rm -rf build $(OBJS) $(INITRAMFS)