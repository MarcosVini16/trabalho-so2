// buffer.hpp
#pragma once

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

    // acesso aos dados do payload dentro do frame
    template<typename T>
    T* data() {
        return reinterpret_cast<T*>(_frame.data);
    }

    // metadados
    Metadata&       metadata()       { return _meta; }
    const Metadata& metadata() const { return _meta; }

    size_t size()  const { return _meta.size; }
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
    }

private:
    Frame    _frame;
    Metadata _meta;
    State    _state;
};