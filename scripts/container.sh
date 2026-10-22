#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
export DOCKER_CONFIG="${DOCKER_CONFIG:-/tmp/chos-docker}"
case "${1:-}" in
    image)
        args=(--build-arg HTTP_PROXY --build-arg HTTPS_PROXY --build-arg http_proxy --build-arg https_proxy)
        if [ -n "${SSL_CERT_FILE:-}" ]; then
            args+=(--secret "id=proxy_ca,src=$SSL_CERT_FILE")
        fi
        # Cloud's proxy hostname is not inherited by Docker DNS.
        if proxy_ip=$(getent ahostsv4 proxy | awk 'NR==1{print $1}'); then
            [ -z "$proxy_ip" ] || args+=(--add-host "proxy:$proxy_ip")
        fi
        docker build --network host "${args[@]}" -t chos-build:m1 .
        ;;
    *)
        docker run --rm --network none --user "$(id -u):$(id -g)" \
            -v "$PWD:/src" -w /src chos-build:m1 "${@:-make}"
        ;;
esac
