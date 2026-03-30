// buffer.hpp
#pragma once

class NICBase; // forward declaration to avoid circular dependency

/*
 * A buffer for holding Ethernet frames.
 */
template<typename Frame>
class Buffer {
public:
    // -- estado do slot no pool --
    enum class State { FREE, ALLOCATED, RECEIVED };

    // -- metadados preenchidos pela NIC no alloc() ou no recebimento --
    struct Metadata {
        size_t size   = 0;      // tamanho real dos dados (pode ser < MTU)
        bool   valid  = false;  // frame passou validação básica?
    };

public:
    Buffer() : _state(State::FREE) {}

    // acesso ao frame físico
    Frame*       frame()       { return &_frame; }
    const Frame* frame() const { return &_frame; }
    void set_frame(const Frame& f) { _frame = f; }

    // acesso aos dados do payload dentro do frame
    template<typename T>
    T* data() {
        return reinterpret_cast<T*>(_frame.data);
    }

    // metadados
    Metadata&       metadata()       { return _meta; }
    const Metadata& metadata() const { return _meta; }

    // ownership e gerenciamento do buffer
    NICBase* owner() const { return _owner; }
    void set_owner(NICBase* nic) { _owner = nic; }

    size_t size()  const { return _meta.size; }
    void   set_size(size_t s) { _meta.size = s; }
    bool   valid() const { return _meta.valid; }

    // gerenciamento do slot
    bool is_free() const { return _state == State::FREE; }

    void allocate() {
        _state      = State::ALLOCATED;
        _meta       = Metadata{};   // limpa metadados anteriores
    }

    void received(size_t size) {
        _state      = State::RECEIVED;
        _meta.size  = size;
        _meta.valid = (size > 0);
    }

    void release() {
        _state      = State::FREE;
        _meta       = Metadata{};
        _owner      = nullptr;
        set_size(0);
    }

private:
    Frame    _frame;
    Metadata _meta;
    State    _state;
    NICBase* _owner; // ponteiro para a NIC que gerencia este buffer (pode ser útil para liberar o buffer de volta ao pool da NIC)
};