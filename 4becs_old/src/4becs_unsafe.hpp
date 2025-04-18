// 4becs Entity Component System, less safe version
// version 1.1.0

#pragma once

#include <iostream>
#include <unordered_map>
#include <cstdlib>
#include <cstdint>
#include <cassert>

// this version of 4becs requires manual cleanup of components with the FREECOMPS macro
// slightly faster performance than the safe one

// RELEVANT KEYWORDS

// Comp - component baseclass
// COMP - macro which prepares a Comp for the system
// must be in the member list

// Ent - entity baseclass
// incl(...) - takes in Comp derived structs and adds them to the Ent
// must be in the constructor
// FREECOMPS - macro which frees all of an Ent's components
// must be in the destructor of each Ent with components
// is in Ent's destructor

// Functions

// bool has<C>(Ent*) - takes in an Ent pointer and returns true if the component type C is included in the Ent
// C* get<C>(Ent*) - returns a pointer to an Ent's component bases on the input type C, otherwise returns a nullptr


// NOTES

// REMEMBER TO USE ALL OF THE RELEVANT KEYWORDS OR YOU'LL LEAK MEMORY
// ESPECIALLY FREECOMPS
// all components should be declared in the global scope, aka before main() runs




// the amount of bits used to store the index
// this should be in a range of 2-5, but technically can be from 0 to 63
// a value of 2 will allow for a max of 248 components
// 3 - 488; 4 - 976; 5 - 1920;
constexpr uint64_t IDX_BITS = 2; 
constexpr uint64_t MAX_IDX = (1ULL << IDX_BITS) - 1;
// the amount of bits in each sub-bitfield
constexpr uint64_t BF_BITS = 64 - IDX_BITS; 

// the rightmost {IDX_BITS} bits hold an index, the leftmost {BF_BITS} bits are a bitfield
using u64bit = uint64_t;

namespace becs {

    // gives out a unique u64bit to each component 
    struct CompRegistry {
        CompRegistry() = default;
        static CompRegistry& instance() {
            static CompRegistry reg;
            return reg;
        }
        u64bit getbit() {
            static uint64_t idx = 0;
            static uint64_t bit = 0;
        
            u64bit fullbit = 0;
            if (bit == BF_BITS) {
                bit = 0;
                idx++;
                assert(idx <= MAX_IDX && "too many components, increase Idx_Bits");
            }
            fullbit = idx;
            fullbit |= 1ULL << (IDX_BITS + bit);
            bit++;
            //std::cout << idx << " " << MAX_IDX << "\n";
            return fullbit;
        }
    };

}

// ever Comp must include this
#define COMP inline static const u64bit b = becs::CompRegistry::instance().getbit()

// component baseclass
// needed for freeing components
struct Comp {
    virtual ~Comp() = 0;
};

Comp::~Comp() {}

// must be included in an Ent's destructor
// frees all of the components an Ent has
#define FREECOMPS \
    for (auto& [key, ptr] : compMap) delete ptr; \
    free(compMask)


// entity base class
struct Ent {
    u64bit* compMask = nullptr;
    std::unordered_map<u64bit, Comp*> compMap = {};
    virtual ~Ent() {
        FREECOMPS;
    }
};

// checks whether a component is included in an entity, return a bool
template <typename C>
bool has(Ent* ent) {
    if (!ent->compMask) return false;
    return (ent->compMask[C::b & MAX_IDX] & (C::b & (~MAX_IDX))) != 0;
}

// returns a pointer to a component, or a nullptr
// checks if the component exists first, to avoid useless nullptrs being added to the umap
template <typename C>
C* get(Ent* ent) {
    if (has<C>(ent)) {
        return static_cast<C*>(ent->compMap[C::b]);
    }
    return nullptr;
}

// use this to include the Comps for an Ent
// takes in the Typenames of the Comps
#define incl(...) \
    if (compMask) { free(compMask); compMask = nullptr; } \
    if (!(compMask = (u64bit*)(calloc(MAX_IDX + 1, sizeof(u64bit))))) { std::cerr << "malloc failed, constructor fail" << std::endl; } \
    becs::setmaskbit<__VA_ARGS__>(compMask); \
    becs::setComps<__VA_ARGS__>(compMap)


namespace becs {
    // places a Comp's bit into the Ent's mask
    template <typename... C>
    void setmaskbit(u64bit* mask) {
        ((mask[C::b & MAX_IDX] |= (C::b & (~MAX_IDX))), ...);
    }

    // creates instances of the included components
    template <typename... C>
    void setComps(std::unordered_map<u64bit, Comp*>& comps) {
        ((comps[C::b] = new C()), ...);
    }
}
