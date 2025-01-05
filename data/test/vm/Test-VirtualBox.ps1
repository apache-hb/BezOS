param(
    [Parameter(Mandatory=$true)]
    [ValidateScript({ Test-Path $_ })]
    [string]$KernelImage,

    [Parameter(Mandatory=$false)]
    [string]$SerialOutputPath
)

$RootPath = $env:USERPROFILE + '\Documents\KernelTest\VirtualBox'

$ImagePath = $RootPath + '\bezos.iso'

$VBoxManage = $env:VBOX_MSI_INSTALL_PATH + '\VBoxManage.exe'

$VmGuid = [guid]::NewGuid().ToString()
$VmName = 'Test-BezOS'
$SerialPath = $RootPath + '\com1.txt'
$BootDrive = 'BOOT'

# Create the vbox directory
New-Item -ItemType Directory -Path $RootPath -Force

# Copy the kernel image to the vbox directory
Copy-Item -Path $KernelImage -Destination $ImagePath -Force

# Delete old instance if it exists

& $VBoxManage unregistervm $VmName --delete-all

if (Test-Path $SerialPath) {
    Remove-Item $SerialPath
}

# Create new VM

& $VBoxManage createvm `
    --name $VmName `
    --ostype "Other_64" `
    --basefolder $RootPath `
    --platform-architecture "x86" `
    --uuid $VmGuid `
    --register

# Configure boot drive

& $VBoxManage storagectl $VmGuid `
    --name $BootDrive `
    --add sata `
    --controller IntelAHCI `
    --bootable on

& $VBoxManage storageattach $VmGuid `
    --storagectl $BootDrive `
    --medium $ImagePath `
    --device 0 `
    --port 0 `
    --type dvddrive `
    --mtype readonly

& $VBoxManage modifyvm $VmGuid `
    --boot1 disk

# Configure serial port

& $VBoxManage modifyvm $VmGuid `
    --uart1 0x3F8 4 `
    --uartmode1 file $SerialPath

# Don't reset on triple fault

& $VBoxManage modifyvm $VmGuid `
    --triple-fault-reset off

# Start the VM

& $VBoxManage startvm $VmGuid
