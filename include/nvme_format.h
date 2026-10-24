#ifndef CHOS_NVME_FORMAT_H
#define CHOS_NVME_FORMAT_H
#include <stdint.h>
#include <stdbool.h>
struct nvme_capabilities {unsigned stride,timeout_ms;};
struct nvme_namespace {uint64_t sectors;unsigned sector_bytes;bool read_only;};
/* Canonical identities retain leading/interior spaces. Only trailing ASCII
 * spaces and NUL padding are interchangeable; embedded NUL/control is invalid. */
enum nvme_check { NVME_CHECK_OK, NVME_CHECK_NOT_READY, NVME_CHECK_CONTROLLER_IDENTIFY,
    NVME_CHECK_SERIAL_INVALID, NVME_CHECK_SERIAL_CHANGED, NVME_CHECK_NAMESPACE_IDENTIFY,
    NVME_CHECK_NAMESPACE_FORMAT, NVME_CHECK_NAMESPACE_ID, NVME_CHECK_SECTORS,
    NVME_CHECK_SECTOR_SIZE, NVME_CHECK_READONLY, NVME_CHECK_POLICY };
struct nvme_identity { char serial[21]; uint8_t namespace_id[24]; struct nvme_namespace ns; };
bool nvme_serial_normalize(const uint8_t *,unsigned,char [21]);
unsigned nvme_identity_compare(const struct nvme_identity *,const struct nvme_identity *);
const char *nvme_check_message(unsigned);
bool nvme_capabilities(uint64_t,struct nvme_capabilities *);
bool nvme_namespace(const uint8_t *,unsigned,struct nvme_namespace *);
#endif
