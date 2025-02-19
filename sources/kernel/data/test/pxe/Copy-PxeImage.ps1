param(
    [Parameter(Mandatory=$true)][string]$KernelFolder,
    [Parameter(Mandatory=$true)][string]$PxeServerFolder
)

Copy-Item -Path ($KernelFolder + '/*') -Destination $PxeServerFolder -Recurse -Force
