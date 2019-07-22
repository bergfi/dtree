#pragma once

#include <atomic>
#include <algorithm>
#include <cstdio>
#include <sys/mman.h>

//
//class HashSet {
//public:
//    HashSet(size_t scale): _scale(scale) {
//        _buckets = 1ULL << _scale;
//        _entriesMask = _buckets - 1;
//        _map = (decltype(_map))MMapper::mmapForMap(_buckets * sizeof(uint64_t));
//
//        // Claim 0th element for 0
//        // The datum 0x3b2ffe8c36c3d6f4 maps to position 1, so it is the
//        // least likely to ever map to position 0.
//        //*_map = 0x3b2ffe8c36c3d6f4ULL;
//    }
//
//    ~HashSet() {
//        MMapper::munmap(_map, _buckets * sizeof(uint64_t));
//    }
//
//    uint32_t entry(uint64_t key) {
////        uint32_t h = MurmurHash64(key);
//
//        uint64_t h = MurmurHash64(&key, sizeof(uint64_t), 0);
//        return h & _entriesMask;
//        //return (h ^ hashForZero) & _entriesMask;
//    }
//
//    template<int INSERT>
//    uint64_t insertOrContains(uint64_t key) {
//        uint32_t e = entry(key);
//        e += e == 0;
////        size_t inc = 1;
//        std::atomic<uint64_t>* current = &_map[e];
//        do {
//            while(true) {
//                uint64_t k = current->load(std::memory_order_relaxed);
//                if(k == 0ULL) {
//                    if constexpr(!INSERT) return 0x100000000ULL;
//                    if(current->compare_exchange_strong(k, key, std::memory_order_release, std::memory_order_relaxed)) {
//                        if(REPORT_HS) printf("  inserted\n");
//                        if(REPORT) printf("Mapped %16zx -> %8x\n", key, e);
//                        return e;
//                    }
//                    if(k==0) {
//                        printf("cas spuriously failed\n");
//                    }
//                }
//                if(k == key) {
////                    if(key > 0xffffffffull) {
////                        char* c = (char*)&key;
////                        size_t n = 8;
////                        printf("Used ");
////                        while(*c && n--) {
////                            printf("%c", *c++);
////                        }
////                        printf("-> %8x\n", e);
////                    } else
//                        if(REPORT) printf("Used %16zx -> %8x\n", key, e);
//                    return e;
//                }
//                e = (e+1) & _entriesMask;
////                e = (e+inc*2-1) & _entriesMask;
////                inc++;
//                e += e == 0;
//                current = &_map[e];
//            }
//        } while(true);
////        if(key > 0xffffffffull) {
////            char* c = (char*)&key;
////            size_t n = 8;
////            printf("Mapped ");
////            while(*c && n--) {
////                printf("%c", *c++);
////            }
////            printf("-> %8x\n", e);
////        } else
//            if(REPORT) printf("Mapped %16zx -> %8x\n", key, e);
//        return e;
//    }
//
//    uint64_t insert(uint64_t key) {
//        return insertOrContains<1>(key);
//    }
//
//    uint64_t find(uint64_t key) {
//        return insertOrContains<0>(key);
//    }
//
//    uint64_t get(uint32_t idx) {
//        return _map[idx].load(std::memory_order_relaxed);
//    }
//
//    template<typename CONTAINER>
//    void getDensityStats(size_t bars, CONTAINER& elements) {
//
//        size_t entriesPerBar = _buckets / bars;
//        entriesPerBar += entriesPerBar == 0;
//
//        char* hasElement = (char*)malloc(_buckets);
//
//        for(size_t idx = 0; idx < _buckets;) {
//            size_t elementsInThisBar = 0;
//            size_t max = std::min(_buckets, idx + entriesPerBar);
//            for(; idx < max; idx++) {
//                if(_map[idx].load(std::memory_order_relaxed)) {
//                    hasElement[idx] = true;
//                    elementsInThisBar++;
//                } else {
//                    hasElement[idx] = false;
//                }
//            }
//            elements.push_back(elementsInThisBar);
//        }
//
//        printf("\nhash of map: %16zx\n", MurmurHash64(hasElement, _buckets, 0));
//        free(hasElement);
//    }
//
//    template<typename CONTAINER>
//    void getProbeStats(size_t bars, CONTAINER& elements) {
//    }
//
//public:
//    size_t _buckets;
//    size_t _scale;
//    size_t _entriesMask;
//    std::atomic<uint64_t>* _map;
//};

template<typename TREE>
class RehasherExit {
protected:
    __attribute__((always_inline))
    void rehash() {
//        printf("taking too long\n");
//        exit(0);
    }
};

template<typename TREE>
class Rehasher {
protected:

    std::atomic<size_t> _nextRehashPart;

    __attribute__((always_inline))
    void rehash() {
    }
    __attribute__((always_inline))
    bool rehashing() const {
        return _nextRehashPart.load(std::memory_order_relaxed) > 0;
    }
};

template<typename TREE>
class Linear {
public:
    TREE& tree;
    uint32_t& e;

    Linear(TREE& tree, uint32_t& e): tree(tree), e(e) {}

    __attribute__((always_inline))
    void next() {
        e = (e+1) & tree._entriesMask;
//        e += e == 0;
    }
};

template<typename TREE>
class QuadLinear {
public:
    TREE& tree;
    uint32_t& e;
    uint32_t eBase;
    uint32_t eOrig;
    uint32_t inc;

    QuadLinear(TREE& tree, uint32_t& e): tree(tree), e(e), eBase(e & ~0x7ULL), eOrig(e & 0x7ULL), inc(1) {}

    __attribute__((always_inline))
    void next() {
        e = (e + 1) & 0x7ULL;
        if(e == eOrig) {
            uint32_t diff = (inc*2);
            diff -= __builtin_popcount(diff);
            eBase = (eBase + diff*8) & tree._entriesMask;

            if(TREE::REPORT_HS) printf("  jump to %u (%u)\n", eBase, inc);
            inc++;
            if(inc==1000) {
                tree.rehash();
            }
        }
        e += eBase;
//        e += e == 0;
    }
};

template<typename TREE>
class LinearLinear {
public:
    TREE& tree;
    uint32_t& e;
    uint32_t eBase;
    uint32_t eOrig;

    LinearLinear(TREE& tree, uint32_t& e): tree(tree), e(e), eBase(e & ~0x7ULL), eOrig(e & 0x7ULL) {}

    __attribute__((always_inline))
    void next() {
        e = (e + 1) & 0x7ULL;
        if(e == eOrig) {
            eBase = (eBase + 8) & tree._entriesMask;

            if(TREE::REPORT_HS) printf("  jump to %u\n", eBase);
        }
        e += eBase;
//        e += e == 0;
    }
};

template<typename TREE>
class LinearLinear2 {
public:
    TREE& tree;
    uint32_t& e;
    uint32_t eOrig;

    LinearLinear2(TREE& tree, uint32_t& e): tree(tree), e(e), eOrig(e) {}

    __attribute__((always_inline))
    void next() {
        e++;
        if((e & 0x7ULL) == 0) {
            if(e-8 == eOrig) {
                eOrig += 8;
                eOrig &= tree._entriesMask;
                if(TREE::REPORT_HS) printf("  jump to %u\n", eOrig);
            } else {
                e -= 8;
            }
        } else {
            if(e == eOrig) {
                e += 8;
                eOrig += 8;
                eOrig &= tree._entriesMask;
                if(TREE::REPORT_HS) printf("  jump to %u\n", eOrig);
            }
        }
//        e += e == 0;
    }
};

template<typename TREE>
class LinearDiv8 {
public:
    TREE& tree;
    uint32_t& e;

    LinearDiv8(TREE& tree, uint32_t& e): tree(tree), e(e) {
        e &= ~0x7ULL;
    }

    __attribute__((always_inline))
    void next() {
        e = (e+1) & tree._entriesMask;
//        e += e == 0;
    }
};

template<typename TREE>
class LinearDiv2 {
public:
    TREE& tree;
    uint32_t& e;

    LinearDiv2(TREE& tree, uint32_t& e): tree(tree), e(e) {
        e &= ~0x1ULL;
    }

    __attribute__((always_inline))
    void next() {
        e = (e+1) & tree._entriesMask;
//        e += e == 0;
    }
};

template<typename T>
struct HashCompare {

    __attribute__((always_inline))
    bool equal( const T& j, const T& k ) const {
        return j == k;
    }

    __attribute__((always_inline))
    size_t hash( const T& k ) const {
        return k;
    }
};

template< template<typename> typename REHASHER
        , template<typename> typename BUCKETFINDER = QuadLinear
        , template<typename> typename HASH = HashCompare
        >
class HashSet: REHASHER<HashSet<REHASHER, BUCKETFINDER>> {
public:
    static constexpr bool REPORT = 0;
    static constexpr bool REPORT_HS = 0;
    static constexpr uint64_t hashForZero = 0x7208f7fa198a2d81ULL;
    static constexpr uint64_t seedForZero = 0xc6a4a7935bd1e995ULL*8ull;

    using Bucketfinder = BUCKETFINDER<HashSet<REHASHER, BUCKETFINDER>>;
    friend Bucketfinder;
public:

    struct probeStats {
        size_t probeCount;
        size_t firstProbe;
        size_t finalProbe;
        size_t failedCAS;
    };

    struct mapStats {
        size_t bytesReserved;
        size_t bytesUsed;

        mapStats& operator+=(const mapStats& other) {
            this->bytesReserved += other.bytesReserved;
            this->bytesUsed += other.bytesUsed;
            return *this;
        }
    };

//    HashSet(): _scale(0), _buckets(0), _entriesMask(0), _map(nullptr) {
//    }

    HashSet(size_t scale): _scale(scale) {
        _buckets = 1ULL << _scale;
        _entriesMask = _buckets - 1;
        _map = (decltype(_map))mmap(nullptr, _buckets * sizeof(uint64_t), PROT_READ|PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
        assert(_map && "failed to mmap data");
    }

    ~HashSet() {
        munmap(_map, _buckets * sizeof(uint64_t));
        _map = nullptr;
    }

    uint32_t entry(uint64_t key) {
//        uint32_t h = MurmurHash64(key);
//        uint32_t h = MurmurHash64(&key, sizeof(size_t), seedForZero);
        uint32_t h = HashCompare<uint64_t>().hash(key);
        return h & _entriesMask;
    }

    template<int INSERT, int TRACKING>
    uint64_t insertOrContains(uint64_t key, probeStats& ps) {
        uint32_t e = entry(key);
        Bucketfinder searcher(*this, e);
        e += e == 0;
        if(TRACKING) {
            ps.firstProbe = e;
            ps.probeCount = 1;
            ps.failedCAS = 0;
        }
        std::atomic<uint64_t>* current = &_map[e];

        size_t probeCount = 1;
        while(probeCount < _buckets) {
            if(REPORT_HS) printf("e: %u\n", e);
            uint64_t k = current->load(std::memory_order_relaxed);
            if(k == 0ULL) {

                // Make sure we do not use the 0th index
                if(e == 0) goto findnext;
                if constexpr(!INSERT) {
                    if(REPORT) printf("No such mapping %16zx -> ?\n", key);
                    return 0x100000000ULL;
                }
                if(current->compare_exchange_strong(k, key, std::memory_order_release, std::memory_order_relaxed)) {
                    if(REPORT_HS) printf("  inserted\n");
                    if(REPORT) printf("Mapped %16zx -> %8x\n", key, e);
                    return e;
                } else {
                    if(TRACKING) ps.failedCAS++;
                }
            }
            if(k == key) {
//                if(key > 0xffffffffull) {
//                    char* c = (char*)&key;
//                    size_t n = 8;
//                    printf("Used ");
//                    while(*c && n--) {
//                        printf("%c", *c++);
//                    }
//                    printf("-> %8x\n", e);
//                } else
                    if(REPORT) printf("Used   %16zx -> %8x\n", key, e);
                if(REPORT_HS) printf("  found\n");
                return e;
            }
            findnext:
            searcher.next();
            if(TRACKING) ps.probeCount++;
            current = &_map[e];
            probeCount++;
        }
//        if(key > 0xffffffffull) {
//            char* c = (char*)&key;
//            size_t n = 8;
//            printf("Mapped ");
//            while(*c && n--) {
//                printf("%c", *c++);
//            }
//            printf("-> %8x\n", e);
//        } else
        if(REPORT_HS) printf("Hash map full\n");
        return 0ULL;
    }

    uint64_t insert(uint64_t key) {
        return insertOrContains<1, 0>(key, *(probeStats*)nullptr);
    }

    uint64_t find(uint64_t key) {
        return insertOrContains<0, 0>(key, *(probeStats*)nullptr);
    }

    uint64_t insertTracked(uint64_t key, probeStats& ps) {
        return insertOrContains<1, 1>(key, ps);
    }

    uint64_t findTracked(uint64_t key, probeStats& ps) {
        return insertOrContains<0, 1>(key, ps);
    }

    uint64_t get(uint32_t idx) {
        return _map[idx].load(std::memory_order_relaxed);
    }

    template<typename CONTAINER>
    void getDensityStats(size_t bars, CONTAINER& elements) {

        size_t entriesPerBar = _buckets / bars;
        entriesPerBar += entriesPerBar == 0;

        for(size_t idx = 0; idx < _buckets;) {
            size_t elementsInThisBar = 0;
            size_t max = std::min(_buckets, idx + entriesPerBar);
            for(; idx < max; idx++) {
                if(_map[idx].load(std::memory_order_relaxed)) {
                    elementsInThisBar++;
                }
            }
            elements.push_back(elementsInThisBar);
        }
    }

    template<typename CONTAINER>
    void getProbeStats(size_t bars, CONTAINER& elements) {

        size_t entriesPerBar = _buckets / bars;
        entriesPerBar += entriesPerBar == 0;

        elements.resize(bars);

        for(size_t idx = 0; idx < _buckets;) {
            size_t max = std::min(_buckets, idx + entriesPerBar);
            for(; idx < max; idx++) {
                uint64_t key = _map[idx].load(std::memory_order_relaxed);
                if(key) {
                    probeStats ps;
                    findTracked(key, ps);
                    elements[ps.firstProbe/entriesPerBar] += ps.probeCount - 1;
                }
            }
        }
    }

    mapStats const& getStats() {
        std::atomic<uint64_t>* current = _map;
        std::atomic<uint64_t>* end = _map + _buckets;

        size_t bytesUsed = 0;
        while(current < end) {
            if(current->load(std::memory_order_relaxed)) {
                ++bytesUsed;
            }
            ++current;
        }

        _mapStats.bytesReserved = _buckets * sizeof(std::atomic<uint64_t>);
        _mapStats.bytesUsed = bytesUsed;
        return _mapStats;
    }

public:
    size_t _scale;
    size_t _buckets;
    size_t _entriesMask;
    std::atomic<uint64_t>* _map;
    mapStats _mapStats;
};
