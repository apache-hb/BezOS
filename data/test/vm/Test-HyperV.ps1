param(
    [Parameter(Mandatory=$true)][string]$KernelImage
)

$RootPath = $env:USERPROFILE + '\Documents\KernelTest\HyperV'

$ImagePath = $RootPath + '\bezos.iso'

# Create the hyperv directory
New-Item -ItemType Directory -Path $RootPath -Force

# Copy the kernel image to the hyperv directory
Copy-Item -Path $KernelImage -Destination $ImagePath -Force

$VmName = 'Test-BezOS'
$VhdPath = $RootPath + '\bezos.vhdx'

$VM = @{
    Name = $VmName
    Generation = 2
    NewVHDPath = $VhdPath
    BootDevice = 'VHD'
    Path = $RootPath
    SwitchName = (Get-VMSwitch)[0].Name
    NewVHDSizeBytes = 256MB
}

Stop-VM -Name $VmName -Force -TurnOff

Remove-VM -Name $VmName -Force

if (Test-Path $VhdPath) {
    Remove-Item $VhdPath -Force
}

$Vm = New-VM @VM

$BootDrive = Add-VMDvdDrive -VMName $VmName -Path $ImagePath -Passthru

Set-VMComPort -VMName $VmName -Number 1 -Path $RootPath + '\com1.txt'

Set-VMFirmware -VMName $VmName -FirstBootDevice $BootDrive -EnableSecureBoot Off

Start-VM -Name $VmName
