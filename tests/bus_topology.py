# Controlled QEMU topology exercising nonzero buses, not a VMware device emulator.
BRIDGED={'ram':2048,'input_backend':'usb','input_pci_bus':'usbroot','storage_pci_bus':'nvmeroot','early_devices':['pcie-root-port,id=usbroot,chassis=1','pcie-root-port,id=nvmeroot,chassis=2']}
