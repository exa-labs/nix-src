---
synopsis: "New `forward-aws-credentials` setting lets the daemon use the client's AWS credentials"
---

The Nix daemon resolves AWS credentials for `s3://` substituters as `root`, so it cannot use per-user credential sources such as IAM Identity Center (SSO) profiles in `~/.aws/config`.

With the new [`forward-aws-credentials`](@docroot@/command-ref/conf-file.md#conf-forward-aws-credentials) setting, the client resolves the credentials for every AWS profile referenced by an `s3://` substituter itself and sends them to the daemon along with its other options. The daemon uses them only for that connection and only when the client is a [trusted user](@docroot@/command-ref/conf-file.md#conf-trusted-users).
