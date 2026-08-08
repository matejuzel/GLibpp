#pragma once

#include <atomic>
#include <bit>
#include <cstdint>
#include <type_traits>

namespace GLibpp::Core {

// Jednomistna atomicka schranka typu "posledni vyhrava": producent set()
// prepisuje obsah, konzument consume() vybira a soucasne vyprazdnuje.
// Cely payload T se pakuje do jednoho uint64_t (bit_cast), takze obe
// operace jsou nedelitelne - zadny roztrzeny par hodnot a set() doruceny
// behem consume() se neztrati (prezije do pristiho consume()).
//
// Urceno pro signalizaci mezi vlakny s jedinym konzumentem (napr. resize
// request logika -> render); producentu smi byt i vic (set je prosty store).
//
// Pozor: "prazdno" je vsech 64 bitu nulovych - hodnota T s celonulovou
// reprezentaci je nerozlisitelna od prazdne schranky (consume ji ohlasi
// jako "nic"). Pro payload, kde je nula platna hodnota, pridej do T
// explicitni priznak platnosti.
template <typename T>
class AtomicMailbox {

    static_assert(std::is_trivially_copyable_v<T>, "T se prenasi bitovou kopii");
    static_assert(sizeof(T) == sizeof(uint64_t), "T musi presne vyplnit 64 bitu (padding doplnit explicitne)");

    std::atomic<uint64_t> slot{ 0 };

public:

    void set(const T& value) {
        slot.store(std::bit_cast<uint64_t>(value), std::memory_order_release);
    }

    // vraci false, kdyz je schranka prazdna; jinak vybere obsah do out
    bool consume(T& out) {
        // levna kontrola bez RMW pro bezny prazdny pripad
        if (slot.load(std::memory_order_relaxed) == 0) {
            return false;
        }
        uint64_t bits = slot.exchange(0, std::memory_order_acquire);
        if (bits == 0) return false; // producent mezitim prepsal celonulovou hodnotou
        out = std::bit_cast<T>(bits);
        return true;
    }
};

}
