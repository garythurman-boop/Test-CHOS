#ifndef CHOS_NVME_FORMAT_H
#define CHOS_NVME_FORMAT_H
#include <stdint.h>
#include <stdbool.h>
struct nvme_capabilities {unsigned stride,timeout_ms;};
struct nvme_namespace {uint64_t sectors;unsigned sector_bytes;bool read_only;};
bool nvme_capabilities(uint64_t,struct nvme_capabilities *);
bool nvme_namespace(const uint8_t *,unsigned,struct nvme_namespace *);
#endif
