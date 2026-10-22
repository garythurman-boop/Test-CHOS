FROM debian:trixie-slim@sha256:62bd423aeee912639ca8ee681c2b762cb3e25137e0a92b20be455898a9f034ad
RUN --mount=type=secret,id=proxy_ca \
    sed -i "s|http://deb.debian.org|https://deb.debian.org|g" /etc/apt/sources.list.d/debian.sources && \
    if [ -f /run/secrets/proxy_ca ]; then mkdir -p /usr/local/share/ca-certificates; cp /run/secrets/proxy_ca /usr/local/share/ca-certificates/build-proxy.crt; chmod 644 /usr/local/share/ca-certificates/build-proxy.crt; printf 'Acquire::https::CaInfo "/usr/local/share/ca-certificates/build-proxy.crt";\n' > /etc/apt/apt.conf.d/99proxy-ca; fi && \
    apt-get update -o APT::Update::Error-Mode=any && apt-get install -y --no-install-recommends clang lld llvm make python3 qemu-system-arm qemu-efi-aarch64 xorriso mtools ca-certificates && rm -rf /var/lib/apt/lists/*
WORKDIR /src
