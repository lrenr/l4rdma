-- vim:set ft=lua:

-- io configuration for l4rdma

local hw = Io.system_bus()

Io.add_vbusses
{
    mlxdevs = Io.Vi.System_bus(function ()
        Property.num_msis = 10;
        PCI0 = Io.Vi.PCI_bus(function ()
            -- Testing with display devices
            pci_hd = wrap(hw:match("PCI/CC_03"))
        end)
    end)
}
