#pragma once

// declaração antecipada pra dependência
class NICBase;

template<typename Frame>
class Buffer {
public:

    // estado do buffer: 3 possibilidades:
    // livre, disponível pra uso
    // alocado, alguém está usando
    // received: recebeu dados
    enum class State { FREE, ALLOCATED, RECEIVED };

    // informações auxiliares
    struct Metadata {
        // tamanho real dos dados no frame (pode ser menor que MTU)
        size_t size   = 0;
        // se passou pela verificação básica
        bool   valid  = false;
    };

public:
    // construtor: quando o buffer nasce, está livre
    Buffer() : _state(State::FREE) {}

    // acesso ao frame físico:
    // permite modificar
    Frame*       frame()       { return &_frame; }
    // não permite modificar
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
        // crime de guerra abaixo
        //_owner      = nullptr;
        set_size(0);
    }

private:
    Frame    _frame;
    Metadata _meta;
    State    _state;
    NICBase* _owner; // ponteiro para a NIC que gerencia este buffer (pode ser útil para liberar o buffer de volta ao pool da NIC)
};