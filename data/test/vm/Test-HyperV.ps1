param(
    [Parameter(Mandatory=$false)]
    [ValidateScript({ Test-Path $_ })]
    [string]$KernelImage,

    [Parameter(Mandatory=$false)]
    [string]$SerialOutputPath
)

# If we can't get the VMs, we're not running as admin
$ErrorActionPreference = 'SilentlyContinue'
$AllMachines = Get-VM
$ErrorActionPreference = 'Continue'

if ($AllMachines -eq $null) {
    Write-Output "This account does not have permission to run Hyper-V VMs. Please add your user to the Hyper-V Administrators group and try again."
    exit 0
}

$RootPath = $env:USERPROFILE + '\Documents\KernelTest\HyperV'

$ImagePath = $RootPath + '\bezos.iso'

$VmName = 'Test-BezOS'
$VhdPath = $RootPath + '\bezos.vhdx'
$SerialPort = $RootPath + '\com1.txt'

$VM = @{
    Name = $VmName
    Generation = 2
    NewVHDPath = $VhdPath
    BootDevice = 'VHD'
    Path = $RootPath
    SwitchName = (Get-VMSwitch)[0].Name
    NewVHDSizeBytes = 256MB
}

# Create the hyperv directory
New-Item -ItemType Directory -Path $RootPath -Force

# Copy the kernel image to the hyperv directory
Copy-Item -Path $KernelImage -Destination $ImagePath -Force

Stop-VM -Name $VmName -Force -TurnOff

Remove-VM -Name $VmName -Force

if (Test-Path $VhdPath) {
    Remove-Item $VhdPath -Force
}

$Vm = New-VM @VM

$BootDrive = Add-VMDvdDrive -VMName $VmName -Path $ImagePath -Passthru

# Set-VMComPort $VmName 1 -Path $SerialPort

Set-VMFirmware -VMName $VmName -FirstBootDevice $BootDrive -EnableSecureBoot Off

Start-VM -Name $VmName
