param(
    [Parameter(Mandatory=$true)][string]$KernelImage
)

$RootPath = $env:USERPROFILE + '\Documents\KernelTest\VirtualBox'

$ImagePath = $RootPath + '\bezos.iso'

# Create the vbox directory
New-Item -ItemType Directory -Path $RootPath -Force

# Copy the kernel image to the vbox directory
Copy-Item -Path $KernelImage -Destination $ImagePath -Force

$VBoxManage = $env:VBOX_MSI_INSTALL_PATH + '\VBoxManage.exe'

$VmGuid = [guid]::NewGuid().ToString()
$VmName = 'Test-BezOS'
$SerialPort = $RootPath + '\com1.txt'
$BootDrive = 'BOOT'

& $VBoxManage unregistervm $VmName --delete-all

& $VBoxManage createvm `
    --name $VmName `
    --ostype "Other_64" `
    --basefolder $RootPath `
    --platform-architecture "x86" `
    --uuid $VmGuid `
    --register

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
    --triple-fault-reset off

& $VBoxManage modifyvm $VmGuid `
    --uart1 0x3F8 4 `
    --uartmode1 file $SerialPort

& $VBoxManage modifyvm $VmGuid `
    --boot1 disk

& $VBoxManage startvm $VmGuid
