param(
    [Parameter(Mandatory=$false)]
    [ValidateScript({ Test-Path $_ })]
    [string]$KernelImage,

    [Parameter(Mandatory=$false)]
    [string]$VMwarePath = 'C:\Program Files (x86)\VMware\VMware Workstation',

    [Parameter(Mandatory=$false)]
    [string]$SerialOutputPath
)

$RootPath = $env:USERPROFILE + '\Documents\KernelTest\VMware'

$ImagePath = $RootPath + '\bezos.iso'

$VmCli = $VMwarePath + '\vmcli.exe'
$VmRun = $VMwarePath + '\vmrun.exe'

$VmName = 'Test-BezOS'
$VmPath = $RootPath + '\' + $VmName
$VmxPath = $VmPath + '\' + $VmName + '.vmx'
$SerialPath = $RootPath + '\com1.txt'

# Create the vmware directory
New-Item -ItemType Directory -Path $RootPath -Force

# Copy the kernel image to the vmware directory
Copy-Item -Path $KernelImage -Destination $ImagePath -Force

# Delete old instance if it exists

if (Test-Path $VmxPath) {
    & $VmRun stop $VmxPath
    & $VmRun deleteVM $VmxPath
}

# Remove old serial output file if it exists

if (Test-Path $SerialPath) {
    Remove-Item $SerialPath
}

# Create the VM

& $VmCli VM Create -n $VmName -d $VmPath -g other6xlinux-64

& $VmCli ConfigParams SetEntry displayName $VmName $VmxPath

& $VmCli ConfigParams SetEntry guestOS other-64 $VmxPath

# Configure main drive

& $VmCli nvme SetPresent nvme0 1 $VmxPath

& $VmCli Disk SetBackingInfo nvme0:0 disk $VmPath\$VmName.vmdk 1 $VmxPath

& $VmCli Disk SetPresent nvme0:0 1 $VmxPath

# Configure boot drive

& $VmCli Sata SetPresent sata0 1 $VmxPath

& $VmCli Disk SetBackingInfo sata0:0 cdrom_image $ImagePath 1 $VmxPath

& $VmCli Disk SetPresent sata0:0 1 $VmxPath

# Configure serial port

& $VmCli Serial SetPresent serial0 1 $VmxPath

& $VmCli Serial SetBackingInfo serial0 file $SerialPath null null null $VmxPath

# Set boot order

& $VmCli ConfigParams SetEntry bios.bootOrder "cdrom,hdd" $VmxPath

# Start the VM

& $VmRun start $VmxPath
