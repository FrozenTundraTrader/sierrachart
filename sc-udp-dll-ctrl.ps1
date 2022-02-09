<#
.SYNOPSIS
    Load/unload all SierraChart Custom Study DLLs using UDP commands.

.DESCRIPTION
    Load/unload all SierraChart Custom Study DLLs using UDP commands.
    See https://www.sierrachart.com/index.php?page=doc/UDPAPI.html

.NOTES
    Authors: cxxxxx, Frozen Tundra, malykubo
#>


Param(
    # Hostname, Default = localhost
	[parameter(Mandatory=$false)]
	[string]$Targethost = "127.0.0.1",
    # Load or unload?
	[parameter(Mandatory=$true)]
	[switch]$UnloadAllDLLs = $true,
    # Disable verbose help messages
	[parameter(Mandatory=$false)]
	[switch]$DisableHelp = $false

)


# ################################################################################
# SETTINGS
# ################################################################################

# Halt on errors
$ErrorActionPreference = "Stop"

# Destination IP address / hostname
$IpAddress = $Targethost
# Destination SierraChart port, as defined in
# Global Settings -> Sierra Chart Server Settings -> "UDP Port"
$SCPortUDP = 4242



# --------------------------------------------------------------------------------
# Issue message and die
# --------------------------------------------------------------------------------
function Die
{
    Param(
    # Output message
    [parameter(Mandatory=$true)]
    [string]$Msg,
    [parameter(Mandatory=$false)]
    [int]$RetCode
    )

    Write-Output "`n`n`n"
    Write-Output "***************************************************************************"
    Write-Output "*****"
    Write-Output "***** DIE: $Msg"
    Write-Output "*****"
    Write-Output "***************************************************************************"
    Write-Output "`n`n`n"

    if ($RetCode -ne 0)
    {
        # Use user-defined return code
        exit $RetCode
    }
    exit 1 
}


# --------------------------------------------------------------------------------
# Send UDP string
# --------------------------------------------------------------------------------
function Send-UdpDatagram 
{ 
    Param (
    # Destination address
    [string] $EndPoint,  
    # Destination port
    [int] $Port,
    # THE message
    [string] $Message
    ) 

    $IP = [System.Net.Dns]::GetHostAddresses($EndPoint)  
    $Address = [System.Net.IPAddress]::Parse($IP)  
    $EndPoints = New-Object System.Net.IPEndPoint($Address, $Port)  
    $Socket = New-Object System.Net.Sockets.UDPClient  

    $EncodedText = [Text.Encoding]::ASCII.GetBytes($Message)  
    $SendMessage = $Socket.Send($EncodedText, $EncodedText.Length, $EndPoints)  
    $Socket.Close()  
} 



# ################################################################################
#
# START
#
# ################################################################################

# Write-Output "PS Version is: $($PSVersionTable.PSVersion.Major)"
if ($PSVersionTable.PSVersion.Major -lt 5)
{
    Die "PS Version not compatible -- At least version 5 is required!" -immediateNoCollect $true
}

$MyName = $MyInvocation.MyCommand.Name



# Some explanatory text. Maybe removed later ...
if (!$DisableHelp)
{
    if ($UnloadAllDLLs)
    {
        Write-Host "### ********************************************************************************"
        Write-Host "### $($MyName):"
        Write-Host "###"
        Write-Host "### Using SierraChart UDP port: $($SCPortUDP)"
        Write-Host "###"
        Write-Host "### If not already done, you have to define/activate the port first -- use:"
        Write-Host "###"
        Write-Host "###     <Global Settings | Sierra Chart Server Settings | UDP Port>"
        Write-Host "###"
        Write-Host "### ********************************************************************************"
    }
}



if ($UnloadAllDLLs)
{
    Write-Host "### Unloading DLLs ..."
    Send-UdpDatagram -EndPoint $IpAddress -Port $SCPortUDP -Message "RELEASE_ALL_DLLS" 
    Write-Host "(ok)"
}
else
{
    if (!$DisableHelp)
    {
        Write-Host "### Allow loading of DLLs ..."
        Write-Host "### ********************************************************************************"
        Write-Host "### To reload chart(s) in SierraChart:"
        Write-Host "###"
        Write-Host "###     Choose <Chart | Recalculate> [CTRL-Insert]"
        Write-Host "###"
        Write-Host "### If defaults (sc.SetDefaults) or params/subgraphs have been changed:"
        Write-Host "###"
        Write-Host "###     Remove and add the study!"
        Write-Host "### ********************************************************************************"
    }

    Send-UdpDatagram -EndPoint $IpAddress -Port $SCPortUDP -Message "ALLOW_LOAD_ALL_DLLS" 
    Write-Host "(ok)"
}

Write-Host "(done - ok)"
exit 0


# ################################################################################


