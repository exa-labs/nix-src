#include "nix/store/aws-creds.hh"

#if NIX_WITH_AWS_AUTH

#  include <gtest/gtest.h>
#  include <gmock/gmock.h>

namespace nix {

TEST(ForwardedAwsCredentials, roundTrip)
{
    ForwardedAwsCredentials creds{
        {"", AwsCredentials("AKIADEFAULT", "default-secret")},
        {"exa", AwsCredentials("ASIAEXA", "exa-secret", "exa-session-token")},
    };

    ASSERT_EQ(decodeForwardedAwsCredentials(encodeForwardedAwsCredentials(creds)), creds);
}

TEST(ForwardedAwsCredentials, roundTripEmpty)
{
    ASSERT_EQ(decodeForwardedAwsCredentials(encodeForwardedAwsCredentials({})), ForwardedAwsCredentials{});
}

TEST(ForwardedAwsCredentials, sessionTokenIsOptional)
{
    auto creds = decodeForwardedAwsCredentials(R"({"p": {"accessKeyId": "a", "secretAccessKey": "s"}})");
    ASSERT_EQ(creds.size(), 1u);
    EXPECT_EQ(creds.at("p").accessKeyId, "a");
    EXPECT_EQ(creds.at("p").secretAccessKey, "s");
    EXPECT_EQ(creds.at("p").sessionToken, std::nullopt);
}

struct MalformedForwardedAwsCredentialsTest : ::testing::TestWithParam<std::string>
{};

TEST_P(MalformedForwardedAwsCredentialsTest, rejectsWithoutEchoingInput)
{
    const auto & input = GetParam();
    try {
        decodeForwardedAwsCredentials(input);
        FAIL() << "expected decoding to fail";
    } catch (Error & e) {
        EXPECT_THAT(e.what(), ::testing::HasSubstr("malformed"));
        EXPECT_THAT(e.what(), ::testing::Not(::testing::HasSubstr("LEAK")));
    }
}

INSTANTIATE_TEST_SUITE_P(
    Inputs,
    MalformedForwardedAwsCredentialsTest,
    ::testing::Values(
        "LEAK",
        R"("LEAK")",
        R"(["LEAK"])",
        R"({"p": "LEAK"})",
        R"({"p": {"accessKeyId": "LEAK"}})",
        R"({"p": {"accessKeyId": "LEAK", "secretAccessKey": 1}})",
        R"({"p": {"accessKeyId": "a", "secretAccessKey": "s", "sessionToken": ["LEAK"]}})"));

} // namespace nix

#endif
