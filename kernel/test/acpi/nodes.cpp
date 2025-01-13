#include <gtest/gtest.h>

#include "acpi/aml.hpp"

#include "aml_test_common.hpp"

using namespace acpi::literals;

TEST(AmlNodeTest, AddNode) {
    AmlTestContext ctx;

    acpi::AmlNodeBuffer buffer = { &ctx.arena, 0 };

    auto id = buffer.add(acpi::AmlNameTerm{ "TEST"_aml });

    ASSERT_EQ(buffer.getType(id), acpi::AmlTermType::eName);

    acpi::AmlNameTerm *term = buffer.get(id);

    ASSERT_NE(term, nullptr);

    ASSERT_EQ(term->name, "TEST"_aml);
}
