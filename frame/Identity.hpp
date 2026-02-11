//
// Created by taganyer on 2026/2/8.
//

#pragma once

#include <tdcf/frame/Identity.hpp>

class Identity : public tdcf::Identity {
public:
    enum MOD {
        ROOT,
        NODE_ROOT,
        NODE
    };

    [[nodiscard]] const char *mod_str() const {
        constexpr const char* msg[] {
            "ROOT",
            "NODE_ROOT",
            "NODE"
        };
        return msg[static_cast<int>(_mod)];
    };

    explicit Identity(uint32_t id, MOD mod, uint32_t weight = 1) :
        _id(id), _mod(mod), _weight(weight) {};

    Uid guid() const override {
        return _id;
    };

    uint64_t serialize_size() const override { return sizeof(Identity); };

    Result serialize(void *buffer, uint64_t buffer_size, uint64_t pos) const override {
        return { 0, true };
    };

    Result deserialize(const void *buffer, uint64_t buffer_size, uint64_t pos) override {
        return { 0, true };
    };

    MOD mod() const { return _mod; };

    uint32_t weight() const { return _weight; };

private:
    uint32_t _id;

    MOD _mod;

    uint32_t _weight = 1;
};
