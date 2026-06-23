#!/usr/bin/env bash
#
# Shared Cargo environment helpers for QSOE Rust workflow scripts.

qsoe_host_id() {
    os=$(uname -s | tr '[:upper:]' '[:lower:]')
    arch=$(uname -m | tr '[:upper:]' '[:lower:]')

    case "$arch" in
        arm64)
            arch=aarch64
            ;;
        amd64)
            arch=x86_64
            ;;
    esac

    printf '%s-%s' "$os" "$arch"
}

qsoe_cargo_set_target_dir() {
    root=$1
    scope=$2

    if [ -z "${CARGO_TARGET_DIR:-}" ]; then
        CARGO_TARGET_DIR="$root/rust/target/$scope-$(qsoe_host_id)"
        export CARGO_TARGET_DIR
    fi
}
