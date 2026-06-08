// periodic_thread.hpp
#pragma once
#include <atomic>
#include <chrono>
#include <functional>
#include <thread>
#include <condition_variable>

/*
 * PeriodicThread executa uma tarefa em intervalos regulares numa thread própria.
 *
 * - O período é em microssegundos.
 * - A thread inicia automaticamente no construtor e termina no destrutor.
 * - Se a tarefa demorar mais que o período, o próximo disparo acontece
 *   imediatamente após (não acumula um backlog de execuções perdidas).
 * - O período pode ser alterado em runtime via set_period().
 * - stop() é idempotente e pode ser chamado de qualquer thread.
 *
 * Uso típico (de dentro de ResponsiveSmartData, quando chega um Interest):
 *     _thread = std::make_unique<PeriodicThread>(std::move(task), period_us);
 */
class PeriodicThread {
public:
    using Task = std::function<void()>;

    PeriodicThread(Task task, uint64_t period_us)
        : _task(std::move(task)),
          _period_us(period_us),
          _stop(false)
    {
        _thread = std::thread([this]() { run(); });
    }

    // Não-copiável, não-movível (a thread interna referencia `this`)
    PeriodicThread(const PeriodicThread&) = delete;
    PeriodicThread& operator=(const PeriodicThread&) = delete;
    PeriodicThread(PeriodicThread&&) = delete;
    PeriodicThread& operator=(PeriodicThread&&) = delete;

    ~PeriodicThread() {
        stop();
        if (_thread.joinable()) {
            _thread.join();
        }
    }

    void stop() {
        _stop.store(true, std::memory_order_release);
        _cv.notify_all();
    }

    // Permite alterar o período em runtime. A alteração entra em vigor a partir do próximo ciclo.
    void set_period(uint64_t period_us) {
        _period_us.store(period_us, std::memory_order_relaxed);
    }

    uint64_t period() const {
        return _period_us.load(std::memory_order_relaxed);
    }

    bool running() const {
        return !_stop.load(std::memory_order_acquire);
    }

private:
    void run() {
        using clock = std::chrono::steady_clock;
        auto next = clock::now();

        while (!_stop.load(std::memory_order_acquire)) {
            _task();

            const auto period = std::chrono::microseconds(
                _period_us.load(std::memory_order_relaxed)
            );
            next += period;

            // Se a tarefa atrasou demais e o "próximo" já passou,
            // reancora no agora e segue a partir daqui sem catch-up
            const auto now = clock::now();
            if (next < now) {
                next = now + period;
            }

            std::unique_lock<std::mutex> lk(_mtx);
            _cv.wait_until(lk, next, [this]{ return _stop.load(); });
        }
    }

    Task                      _task;
    std::atomic<uint64_t>     _period_us;
    std::atomic<bool>         _stop;
    std::thread               _thread;

    std::mutex _mtx;
    std::condition_variable _cv;
};