#ifndef __LBP_HH__
#define __LBP_HH__

#include <cstddef>
#include <cstdint>
#include <fmt/core.h>

#include "bp.hh"
#include "util.hh"
template <size_t CTR_WIDTH, size_t BHT_WIDTH, size_t HIST_LEN, size_t PC_LEN, size_t PHT_WIDTH,
          IndexAlgo INDEX_ALGO = IndexAlgo::CONCAT, size_t PC_SHIFT_AMT = 3>
    requires(HIST_LEN <= 64 && HIST_LEN <= PHT_WIDTH) // histories are stored as uint64_t
class LocalBranchPredictor : public IDirectionPredictor
{
  private:
    using Ctr = Counter<CTR_WIDTH>;
    Ctr pht[exp2(PHT_WIDTH)];

    uint64_t bht[exp2(BHT_WIDTH)] = {};
    uint64_t getIndex(uint64_t pc, uint64_t history)
    {
        if constexpr (INDEX_ALGO == IndexAlgo::CONCAT)
            return getConcatedIndex<PC_LEN, HIST_LEN, PHT_WIDTH>(pc, history);
        else if (INDEX_ALGO == IndexAlgo::XOR)
            return getXoredIndex<PC_LEN, HIST_LEN, PHT_WIDTH>(pc, history);
    }

  public:
    LocalBranchPredictor()
    {
        for (auto &ctr : pht)
            ctr = Ctr(exp2(CTR_WIDTH - 1));
    }

    const std::string &getName() override
    {
        static const std::string name = BHT_WIDTH ? fmt::format("Local<{}, {}, {}>", BHT_WIDTH, HIST_LEN, PC_LEN)
                                                  : fmt::format("Global<{}, {}>", HIST_LEN, PHT_WIDTH);
        return name;
    }

    bool predict(uint64_t ip) override
    {
        auto pc_used = ip >> PC_SHIFT_AMT;
        return pht[getIndex(pc_used, bht[pc_used & bitmask(BHT_WIDTH)])].get();
    }

    void update(uint64_t ip, bool taken) override
    {
        auto pc_used = ip >> PC_SHIFT_AMT;
        auto bht_index = pc_used & bitmask(BHT_WIDTH);
        pht[getIndex(pc_used, bht[bht_index])].update(taken);
        bht[bht_index] = (bht[bht_index] << 1) + taken;
    }
};

#endif
