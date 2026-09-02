#pragma once
///@file
#include "nix/store/config.hh"

#include <string_view>

namespace nix {

/**
 * Name of the `SetOptions` entry carrying AWS credentials that a client
 * resolved on its own side of the connection (see the
 * `forward-aws-credentials` setting). Not a real setting; the daemon
 * only honours it for trusted clients.
 */
constexpr std::string_view forwardedAwsCredentialsOption = "forwarded-aws-credentials";

} // namespace nix

#if NIX_WITH_AWS_AUTH

#  include "nix/store/s3-url.hh"
#  include "nix/store/store-reference.hh"
#  include "nix/util/json-impls.hh"
#  include "nix/util/ref.hh"
#  include "nix/util/error.hh"

#  include <map>
#  include <memory>
#  include <optional>
#  include <string>
#  include <vector>

namespace nix {

/**
 * AWS credentials obtained from credential providers
 */
struct AwsCredentials
{
    std::string accessKeyId;
    std::string secretAccessKey;
    std::optional<std::string> sessionToken;

    AwsCredentials(
        const std::string & accessKeyId,
        const std::string & secretAccessKey,
        const std::optional<std::string> & sessionToken = std::nullopt)
        : accessKeyId(accessKeyId)
        , secretAccessKey(secretAccessKey)
        , sessionToken(sessionToken)
    {
    }

    bool operator==(const AwsCredentials &) const = default;
};

/**
 * Credentials resolved by a client process on behalf of the daemon,
 * keyed by AWS profile name (`""` for the default profile).
 */
using ForwardedAwsCredentials = std::map<std::string, AwsCredentials>;

/**
 * Encode `creds` as the value of the `forwardedAwsCredentialsOption`.
 */
std::string encodeForwardedAwsCredentials(const ForwardedAwsCredentials & creds);

/**
 * @throws Error if `encoded` is not a valid encoding. The message never
 * echoes the input.
 */
ForwardedAwsCredentials decodeForwardedAwsCredentials(std::string_view encoded);

/**
 * Resolve credentials in the current process for every AWS profile
 * referenced by an `s3://` store among `substituters`. Profiles whose
 * credentials cannot be resolved are skipped (a warning is emitted by
 * the provider).
 */
ForwardedAwsCredentials resolveForwardedAwsCredentials(const std::vector<StoreReference> & substituters);

class AwsAuthError final : public CloneableError<AwsAuthError, Error>
{
    std::optional<int> errorCode;

public:
    using CloneableError::CloneableError;

    AwsAuthError(int errorCode);

    std::optional<int> getErrorCode() const
    {
        return errorCode;
    }
};

class AwsCredentialProvider
{
public:
    /**
     * Get AWS credentials for the given URL.
     *
     * @param url The S3 url to get the credentials for
     * @return AWS credentials
     * @throws AwsAuthError if credentials cannot be resolved
     */
    virtual AwsCredentials getCredentials(const ParsedS3URL & url) = 0;

    /**
     * Serve `creds` for their profiles instead of consulting the
     * provider chain. Used by the daemon to honour credentials that a
     * trusted client resolved on its own side of the connection; since
     * the daemon handles each connection in its own process, this state
     * never leaks to other clients.
     */
    virtual void setForwardedCredentials(ForwardedAwsCredentials creds) = 0;

    std::optional<AwsCredentials> maybeGetCredentials(const ParsedS3URL & url)
    {
        try {
            return getCredentials(url);
        } catch (AwsAuthError & e) {
            return std::nullopt;
        }
    }

    virtual ~AwsCredentialProvider() {}
};

/**
 * Create a new instancee of AwsCredentialProvider.
 */
ref<AwsCredentialProvider> makeAwsCredentialsProvider();

/**
 * Get a reference to the global AwsCredentialProvider.
 */
ref<AwsCredentialProvider> getAwsCredentialsProvider();

} // namespace nix

JSON_IMPL(AwsCredentials)

#endif
