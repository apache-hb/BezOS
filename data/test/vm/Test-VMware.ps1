param(
    [Parameter(Mandatory=$true)][string]$KernelImage
)

$VMwarePath = 'C:\Program Files (x86)\VMware\VMware Workstation'

$RootPath = $env:USERPROFILE + '\Documents\KernelTest\VMware'

$ImagePath = $RootPath + '\bezos.iso'

# Create the vmware directory
New-Item -ItemType Directory -Path $RootPath -Force

# Copy the kernel image to the vmware directory
Copy-Item -Path $KernelImage -Destination $ImagePath -Force

$VmCli = $VMwarePath + '\vmcli.exe'
$VmRun = $VMwarePath + '\vmrun.exe'

$VmName = 'Test-BezOS'
$VmPath = $RootPath + '\' + $VmName
$VmxPath = $VmPath + '\' + $VmName + '.vmx'
$SerialPort = $RootPath + '\com1.txt'

# Delete old instance if it exists

if (Test-Path $VmxPath) {
    & $VmRun stop $VmxPath
    & $VmRun deleteVM $VmxPath
}

# Create the VM

& $VmCli VM Create -n $VmName -d $VmPath -g other6xlinux-64

& $VmCli ConfigParams SetEntry displayName $VmName $VmxPath

& $VmCli ConfigParams SetEntry guestOS other-64 $VmxPath

# Configure main drive

& $VmCli nvme SetPresent nvme0 1 $VmxPath

& $VmCli Disk SetBackingInfo nvme0:0 disk $VmPath\bezos.vmdk 1 $VmxPath

& $VmCli Disk SetPresent nvme0:0 1 $VmxPath

# Configure boot drive

& $VmCli Sata SetPresent sata0 1 $VmxPath

& $VmCli Disk SetBackingInfo sata0:0 cdrom_image $ImagePath 1 $VmxPath

& $VmCli Disk SetPresent sata0:0 1 $VmxPath

# Configure serial port

& $VmCli Serial SetPresent serial0 1 $VmxPath

& $VmCli Serial SetBackingInfo serial0 file $SerialPort null null null $VmxPath

# Set boot order

& $VmCli ConfigParams SetEntry bios.bootOrder "cdrom,hdd" $VmxPath

# Start the VM

& $VmRun start $VmxPath
