CXX      = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -Iinclude -pthread
LDFLAGS  = -lpthread -static

SRCS = src/engine/raw_socket_engine.cpp \
       src/engine/shm_engine.cpp \
       src/components/component.cpp

OBJS     = $(SRCS:.cpp=.o)
VEHICLE  = build/vehicle

.PHONY: all clean network test

all: $(VEHICLE)

$(VEHICLE): app/vehicle_main.cpp $(OBJS)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# configura rede virtual (requer root)
network:
	@sudo scripts/setup_network.sh

# compila, sobe rede e inicia VMs
test: all network
	@sudo scripts/start_vms.sh $(VEHICLE)

# para testes locais sem QEMU — usa veth pairs
test_local: all
	@sudo ip link add veth0 type veth peer name veth1 2>/dev/null || true
	@sudo ip link set veth0 up
	@sudo ip link set veth1 up
	@sudo ./$(VEHICLE) veth0 & \
	 sudo ./$(VEHICLE) veth1 & \
	 wait

clean:
	rm -rf build $(OBJS)
	@sudo scripts/teardown_network.sh 2>/dev/null || true
