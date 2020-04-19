#pragma once

#include <atomic>
#include <algorithm>
#include <cstdio>
#include <sys/mman.h>
#include <thread>

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
    uint64_t& e;

    Linear(TREE& tree, uint64_t& e): tree(tree), e(e) {}

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
    uint64_t& e;
    uint64_t eBase;
    uint64_t eOrig;
    uint64_t inc;

    QuadLinear(TREE& tree, uint64_t& e): tree(tree), e(e), eBase(e & ~0x7ULL), eOrig(e & 0x7ULL), inc(1) {}

    __attribute__((always_inline))
    void next() {
        e = (e + 1) & 0x7ULL;
        if(e == eOrig) {
            uint32_t diff = (inc*2);
            diff -= __builtin_popcount(diff);
            eBase = (eBase + diff*8) & tree._entriesMask;

            if(TREE::REPORT_HS) printf("  jump to %zu (%zu)\n", eBase, inc);
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
    uint64_t& e;
    uint64_t eBase;
    uint64_t eOrig;

    LinearLinear(TREE& tree, uint64_t& e): tree(tree), e(e), eBase(e & ~0x7ULL), eOrig(e & 0x7ULL) {}

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

class HashSetBase {
public:
    struct probeStats {

        probeStats()
                : insertsExisting(0)
                , insertsNew(0)
                , finds(0)
                , probeCount(0)
                , firstProbe(0)
                , finalProbe(0)
                , failedCAS(0)
        {}

        size_t insertsExisting;
        size_t insertsNew;
        size_t finds;
        size_t probeCount;
        size_t firstProbe;
        size_t finalProbe;
        size_t failedCAS;

        probeStats& operator+=(const probeStats& other) {
            this->insertsExisting += other.insertsExisting;
            this->insertsNew += other.insertsNew;
            this->finds += other.finds;
            this->probeCount += other.probeCount;
            this->firstProbe += other.firstProbe;
            this->finalProbe += other.finalProbe;
            this->failedCAS += other.failedCAS;
            return *this;
        }
    };

    struct mapStats {
        size_t bytesReserved;
        size_t bytesUsed;
        size_t elements;

        mapStats(): bytesReserved(0), bytesUsed(0), elements(0) {}

        mapStats& operator+=(const mapStats& other) {
            this->bytesReserved += other.bytesReserved;
            this->bytesUsed += other.bytesUsed;
            this->elements += other.elements;
            return *this;
        }
    };

    static constexpr uint64_t NotFound() {
        return 0xFFFFFFFFFFFFFFFFULL;
    }
};

template< template<typename> typename REHASHER
        , template<typename> typename BUCKETFINDER = QuadLinear
        , template<typename> typename HASH = HashCompare
        , int GLOBAL_TRACKING = 0
        >
class HashSet: public HashSetBase, REHASHER<HashSet<REHASHER, BUCKETFINDER, HASH, GLOBAL_TRACKING>> {
public:
    static constexpr bool REPORT = 0;
    static constexpr bool REPORT_HS = 0;

    using Bucketfinder = BUCKETFINDER<HashSet<REHASHER, BUCKETFINDER, HASH, GLOBAL_TRACKING>>;
    friend Bucketfinder;
public:

//    HashSet(): _scale(0), _buckets(0), _entriesMask(0), _map(nullptr) {
//    }

    HashSet(): _scale(28), _buckets(1ULL << _scale), _entriesMask(_buckets - 1), _map(nullptr) {
        if(HASH<uint64_t>().hash(0) != 0) {
            printf("0 should be hashed to 0\n");
            abort();
        }
    }

    ~HashSet() {
        if(_map) munmap(_map, _buckets * sizeof(uint64_t));
        _map = nullptr;
    }

    HashSet& setScale(size_t scale) {
        _scale = scale;
        _buckets = 1ULL << _scale;
        _entriesMask = _buckets - 1;
        return *this;
    }

    HashSet& init() {
        assert(!_map && "map already in use");
        _map = (decltype(_map))mmap(nullptr, _buckets * sizeof(uint64_t), PROT_READ|PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
        assert(_map && "failed to mmap data");
        return *this;
    }

    uint32_t entry(uint64_t key) {
//        uint32_t h = MurmurHash64(key);
//        uint32_t h = MurmurHash64(&key, sizeof(size_t), seedForZero);
        uint32_t h = HASH<uint64_t>().hash(key);
        return h & _entriesMask;
    }

    __attribute__((always_inline))
    constexpr uint64_t newlyInserted(uint64_t v) const {
        return v | 0x8000000000000000ULL;
    }

    template<int INSERT, int TRACKING>
    uint64_t insertOrContains(uint64_t key, probeStats& ps) {
        assert(_map && "storage not initialized");
        if(!key) return 0ULL;
        uint64_t e = entry(key);
        e += e == 0;
        Bucketfinder searcher(*this, e);
        if(TRACKING) {
            ps.firstProbe = e;
            ps.probeCount = 1;
            ps.failedCAS = 0;
        }
        if(GLOBAL_TRACKING) _probeStats.probeCount++;
        std::atomic<uint64_t>* current = &_map[e];

        size_t probeCount = 1;

//        size_t maxProbes = _buckets / 8;
        if(REPORT || REPORT_HS) {
            printf("Inserting/finding %16zx\n", key);
        }

        while(probeCount < _buckets) {
            if(REPORT_HS) printf("e: %zx\n", e);
            uint64_t k = current->load(std::memory_order_relaxed);
            if(k == 0ULL) {

                // Make sure we do not use the 0th index
                if(e == 0) goto findnext;
                if constexpr(!INSERT) {
                    if(REPORT) printf("\033[34mNo such mapping %16zx -> ?\033[0m\n", key);
                    if(GLOBAL_TRACKING) _probeStats.finds++;
                    return NotFound();
                }
                if(current->compare_exchange_strong(k, key, std::memory_order_release, std::memory_order_relaxed)) {
                    if(REPORT_HS) printf("  inserted\n");
                    if(REPORT) printf("\033[34mMapped %16zx -> %16zx\033[0m\n", key, e);
                    if(GLOBAL_TRACKING) _probeStats.insertsNew++;
                    return newlyInserted(e);
                } else {
                    if(TRACKING) ps.failedCAS++;
                    if(GLOBAL_TRACKING) _probeStats.failedCAS++;
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
                if(REPORT) printf("\033[34mUsed   %16zx -> %16zx\033[0m\n", key, e);
                if(REPORT_HS) printf("  found\n");
                if(GLOBAL_TRACKING) _probeStats.insertsExisting++;
                return e;
            }
            findnext:
            searcher.next();
            if(TRACKING) ps.probeCount++;
            if(GLOBAL_TRACKING) _probeStats.probeCount++;
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
        printf("Hash map full\n");
        exit(-1);
        return NotFound();
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
        assert(0 <= idx);
        assert(idx < _buckets);
        return _map[idx];
    }

    template<typename CONTAINER>
    mapStats getDensityStats(size_t bars, CONTAINER& elements) {

        size_t entriesTotal = 0;
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
            entriesTotal += elementsInThisBar;
            elements.push_back(elementsInThisBar);
        }

        _mapStats.bytesReserved = _buckets * sizeof(std::atomic<uint64_t>);
        _mapStats.bytesUsed = entriesTotal * sizeof(std::atomic<uint64_t>);
        _mapStats.elements = entriesTotal;
        return _mapStats;
    }

    template<typename FUNC>
    void forAll(FUNC&& func) {
        for(size_t idx = 0; idx < _buckets; ++idx) {
            size_t value = _map[idx].load(std::memory_order_relaxed);
            if(value) {
                func(value);
            }
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

    mapStats getStats() {
        std::atomic<uint64_t>* current = _map;
        std::atomic<uint64_t>* end = _map + _buckets;

        size_t elements = 0;
        while(current < end) {
            if(current->load(std::memory_order_relaxed)) {
                ++elements;
            }
            ++current;
        }

        _mapStats.bytesReserved = _buckets * sizeof(std::atomic<uint64_t>);
        _mapStats.bytesUsed = elements * sizeof(std::atomic<uint64_t>);;
        _mapStats.elements = elements;
        return _mapStats;
    }

    probeStats const& getProbeStats() const {
        return _probeStats;
    }

public:
    size_t _scale;
    size_t _buckets;
    size_t _entriesMask;
    std::atomic<uint64_t>* _map;
    mapStats _mapStats;
    probeStats _probeStats;
};

template< template<typename> typename REHASHER
        , template<typename> typename BUCKETFINDER = QuadLinear
        , template<typename> typename HASH = HashCompare
        , int GLOBAL_TRACKING = 0
>
class HashSet128: public HashSetBase, REHASHER<HashSet128<REHASHER, BUCKETFINDER, HASH, GLOBAL_TRACKING>> {
public:
    static constexpr bool REPORT = 0;
    static constexpr bool REPORT_HS = 0;

    using Bucketfinder = BUCKETFINDER<HashSet128<REHASHER, BUCKETFINDER, HASH, GLOBAL_TRACKING>>;
    friend Bucketfinder;
public:

//    HashSet(): _scale(0), _buckets(0), _entriesMask(0), _map(nullptr) {
//    }

    HashSet128(): _scale(28), _buckets(1ULL << _scale), _entriesMask(_buckets - 1), _map(nullptr) {
        if(HASH<uint64_t>().hash(0) != 0) {
            printf("0 should be hashed to 0\n");
            abort();
        }
    }

    ~HashSet128() {
        if(_map) munmap(_map, _buckets * sizeof(uint64_t) * 2);
        _map = nullptr;
    }

    HashSet128&  setScale(size_t scale) {
        _scale = scale;
        _buckets = 1ULL << _scale;
        _entriesMask = _buckets - 1;
        return *this;
    }

    HashSet128& init() {
        assert(!_map && "map already in use");
        _map = (decltype(_map))mmap(nullptr, _buckets * sizeof(uint64_t) * 2, PROT_READ|PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
        assert(_map && "failed to mmap data");
        return *this;
    }

    uint64_t entry(uint64_t key, uint64_t key2) {
//        uint32_t h = MurmurHash64(key);
//        uint32_t h = MurmurHash64(&key, sizeof(size_t), seedForZero);
//        uint64_t h = HASH<__int128>().hash( ((__int128)key) | (((__int128)key2) << 64));
        uint64_t h = HASH<uint64_t>().hash(key);
        return h & _entriesMask;
    }

    __attribute__((always_inline))
    constexpr uint64_t newlyInserted(uint64_t v) const {
        return v | 0x8000000000000000ULL;
    }

    __attribute__((always_inline))
    static constexpr uint64_t toID(uint64_t v) {
        return v & ~0x8000000000000000ULL;
    }

    __attribute__((always_inline))
    static constexpr uint64_t toIsInserted(uint64_t v) {
        return v & 0x8000000000000000ULL;
    }

    template<int INSERT, int TRACKING>
    uint64_t insertOrContains(uint64_t key, uint64_t key2, probeStats& ps) {
        assert(_map && "storage not initialized");
        assert(key);
        assert(key2);
        uint64_t e = entry(key, key2);
        e += e == 0;
        Bucketfinder searcher(*this, e);
        if(TRACKING) {
            ps.firstProbe = e;
            ps.probeCount = 1;
            ps.failedCAS = 0;
        }
        if(GLOBAL_TRACKING) _probeStats.probeCount++;
        std::atomic<uint64_t>* current = &_map[e*2];

        size_t probeCount = 1;

//        size_t maxProbes = _buckets / 8;

        while(probeCount < _buckets) {
            if(REPORT_HS) printf("e: %zx\n", e);
            uint64_t k = current->load(std::memory_order_relaxed);
            if(k == 0ULL) {

                if(e == 0) goto findnext;
                if constexpr(!INSERT) {
                    if(REPORT) printf("\033[31mNo such mapping %16zx|%16zx -> ?\033[0m\n", key, key2);
                    if(GLOBAL_TRACKING) _probeStats.finds++;
                    return NotFound();
                }
                if(current->compare_exchange_strong(k, key, std::memory_order_release, std::memory_order_relaxed)) {
                    _map[e*2+1] = key2;
                    //std::atomic_thread_fence(std::memory_order_release); // this should be a flush on ARM
                    if(REPORT_HS) printf("  inserted\n");
                    if(REPORT) printf("\033[31mMapped %16zx|%16zx -> %16zx\033[0m\n", key, key2, e);
                    if(GLOBAL_TRACKING) _probeStats.insertsNew++;
                    return newlyInserted(e);
                } else {
                    if(TRACKING) ps.failedCAS++;
                    if(GLOBAL_TRACKING) _probeStats.failedCAS++;
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
                uint64_t k2 = (current+1)->load(std::memory_order_relaxed);
                while(k2 == 0) {
                    std::this_thread::yield();
                    k2 = (current+1)->load(std::memory_order_acquire);
                }
                if(k2 == key2) {
                    if(REPORT) printf("\033[31mUsed   %16zx|%16zx -> %16zx\033[0m\n", key, key2, e);
                    if(REPORT_HS) printf("  found\n");
                    if(GLOBAL_TRACKING) _probeStats.insertsExisting++;
                    return e;
                }
            }
            findnext:
            searcher.next();
            if(TRACKING) ps.probeCount++;
            if(GLOBAL_TRACKING) _probeStats.probeCount++;
            current = &_map[e*2];
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
        printf("Hash map full\n");
        exit(-1);
        return NotFound();
    }

    uint64_t insert(uint64_t key, uint64_t key2) {
        return insertOrContains<1, 0>(key, key2, *(probeStats*)nullptr);
    }

    uint64_t find(uint64_t key, uint64_t key2) {
        return insertOrContains<0, 0>(key, key2, *(probeStats*)nullptr);
    }

    uint64_t insertTracked(uint64_t key, uint64_t key2, probeStats& ps) {
        return insertOrContains<1, 1>(key, key2, ps);
    }

    uint64_t findTracked(uint64_t key, uint64_t key2, probeStats& ps) {
        return insertOrContains<0, 1>(key, key2, ps);
    }

    __int128 get(uint64_t idx) {
        assert(0 <= idx);
        assert(idx < _buckets);
        return ((__int128*)_map)[idx];
    }

    uint64_t get(uint64_t idx, uint64_t& key2) {
        assert(0 <= idx);
        assert(idx < _buckets);
        size_t idx2 = idx * 2;
        uint64_t key = _map[idx2].load(std::memory_order_relaxed);
        key2 = _map[idx2 + 1].load(std::memory_order_relaxed);;
        if(key) {
            while(key2 == 0) {
                std::this_thread::yield();
                key2 = _map[idx2 + 1].load(std::memory_order_acquire);;
            }
        }
        return key;
    }

    template<typename FUNC>
    void forAll(FUNC&& func) {
        for(size_t idx = 0; idx < _buckets; idx+=2) {
            size_t value = _map[idx].load(std::memory_order_relaxed);
            if(value) {
                size_t value2 = _map[idx+1].load(std::memory_order_relaxed);
                func(value, value2);
            }
        }
    }

    template<typename CONTAINER>
    mapStats getDensityStats(size_t bars, CONTAINER& elements) {

        size_t entriesTotal = 0;
        size_t entriesPerBar = _buckets / bars;
        entriesPerBar += entriesPerBar == 0;

        for(size_t idx = 0; idx < _buckets;) {
            size_t elementsInThisBar = 0;
            size_t max = std::min(_buckets, idx + entriesPerBar);
            for(; idx < max; idx += 2) {
                if(_map[idx].load(std::memory_order_relaxed)) {
                    elementsInThisBar++;
                }
            }
            entriesTotal += elementsInThisBar;
            elements.push_back(elementsInThisBar);
        }

        _mapStats.bytesReserved = _buckets * sizeof(std::atomic<uint64_t>) * 2;
        _mapStats.bytesUsed = entriesTotal * sizeof(std::atomic<uint64_t>) * 2;
        _mapStats.elements = entriesTotal;
        return _mapStats;
    }

    template<typename CONTAINER>
    void getProbeStats(size_t bars, CONTAINER& elements) {

        size_t entriesPerBar = _buckets / bars;
        entriesPerBar += entriesPerBar == 0;

        elements.resize(bars);

        for(size_t idx = 0; idx < _buckets;) {
            size_t max = std::min(_buckets, idx + entriesPerBar);
            for(; idx < max; idx += 2) {
                uint64_t key = _map[idx].load(std::memory_order_relaxed);
                uint64_t key2 = _map[idx+1].load(std::memory_order_relaxed);
                if(key) {
                    probeStats ps;
                    findTracked(key, key2, ps);
                    elements[ps.firstProbe/entriesPerBar] += ps.probeCount - 1;
                }
            }
        }
    }

    mapStats getStats() {
        std::atomic<uint64_t>* current = _map;
        std::atomic<uint64_t>* end = _map + _buckets * 2;

        size_t elements = 0;
        while(current < end) {
            if(current->load(std::memory_order_relaxed)) {
                ++elements;
            }
            current += 2;
        }

        _mapStats.bytesReserved = _buckets * sizeof(std::atomic<uint64_t>) * 2;
        _mapStats.bytesUsed = elements * sizeof(std::atomic<uint64_t>) * 2;
        _mapStats.elements = elements;
        return _mapStats;
    }

    probeStats const& getProbeStats() const {
        return _probeStats;
    }

public:
    size_t _scale;
    size_t _buckets;
    size_t _entriesMask;
    std::atomic<uint64_t>* _map;
    mapStats _mapStats;
    probeStats _probeStats;
};