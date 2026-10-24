/* USB2 hub descriptor prefix and xHCI route bounds, independent of MMIO. */
#include "usb.h"
bool usb_hub_info(const uint8_t *p,unsigned n,unsigned *ports,unsigned *characteristics,unsigned *power_ms){
    if(!p || !ports || !characteristics || !power_ms || n<9 || n>64 || p[1]!=0x29 || p[0]>n || p[0]<9 || !p[2] || p[2]>15)return false;
    /* Fixed seven bytes, DeviceRemovable bitmap, and legacy mask prefix. Some
     * emulated hubs expose only one obsolete PortPwrCtrlMask byte. We neither
     * consume nor require further bytes from that obsolete bitmap. */
    if(p[0]<8+(p[2]+1u+7u)/8u)return false;
    *ports=p[2];*characteristics=p[3]|(unsigned)p[4]<<8;*power_ms=p[5]*2;return true;
}
bool usb_hub_route(unsigned parent,unsigned depth,unsigned port,unsigned *route){
    if(!route || depth>=5 || !port || port>15 || parent>0xfffff || (parent>>(depth*4)))return false;
    *route=parent|(port<<(depth*4));return true;
}
