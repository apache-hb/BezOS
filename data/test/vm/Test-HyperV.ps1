param(
    [Parameter(Mandatory=$false)]
    [ValidateScript({ Test-Path $_ })]
    [string]$KernelImage,

    [Parameter(Mandatory=$false)]
    [string]$SerialOutputPath
)

# Check if the user has permission to run Hyper-V VMs
$LocalAdminGroup = [System.Security.Principal.SecurityIdentifier]::new("S-1-5-32-544")
$HyperVAdminGroup = [System.Security.Principal.SecurityIdentifier]::new("S-1-5-32-578")
$CurrentUser = [Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()
$CanUserHyperV = $CurrentUser.IsInRole($LocalAdminGroup) -or $CurrentUser.IsInRole($HyperVAdminGroup)

if (!$CanUserHyperV) {
    Write-Output "This account does not have permission to run Hyper-V VMs. Please add your user to the Hyper-V Administrators group and try again."
    exit 0
}

$RootPath = $env:USERPROFILE + '\Documents\KernelTest\HyperV'

$ImagePath = $RootPath + '\bezos.iso'

$VmName = 'Test-BezOS'
$VhdPath = $RootPath + '\bezos.vhdx'
$SerialPipe = '\\.\pipe\SerialBezOS'

$VM = @{
    Name = $VmName
    Generation = 2
    NewVHDPath = $VhdPath
    BootDevice = 'VHD'
    Path = $RootPath
    SwitchName = (Get-VMSwitch)[0].Name
    NewVHDSizeBytes = 256MB
    MemoryStartupBytes = 512MB
}

# Create the hyperv directory
New-Item -ItemType Directory -Path $RootPath -Force

# Copy the kernel image to the hyperv directory
Copy-Item -Path $KernelImage -Destination $ImagePath -Force

$OldVm = Get-VM -Name $VmName -ErrorAction SilentlyContinue
if ($OldVm) {
    Stop-VM -Name $VmName -Force -TurnOff -ErrorAction SilentlyContinue

    Remove-VM -Name $VmName -Force
}

if (Test-Path $VhdPath) {
    Remove-Item $VhdPath -Force
}

$Vm = New-VM @VM

$BootDrive = Add-VMDvdDrive -VMName $VmName -Path $ImagePath -Passthru

Set-VMComPort $VmName 1 -Path $SerialPipe

Set-VMFirmware -VMName $VmName -FirstBootDevice $BootDrive -EnableSecureBoot Off

Set-VMProcessor -VMName $VmName -Count 4

Start-VM -Name $VmName
