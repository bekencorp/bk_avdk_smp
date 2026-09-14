#!/bin/bash
usage_str="""
\n+----------------------------------------------------------------------------------------+
\n|                                   RW WTS DUT AGENT RUNNER                            |
\n+----------------------------------------------------------------------------------------+
\nThe RivieraWaves WTSDut Agent Runner is a batch script used as framework for the WFA automated 
test bench. It allows to run configure and run RW WTS DUT Agent.

\n -h | --help : Display RW WTS DUT AGENT Runner manual.

\n+----------------------+
\n| USAGE                |
\n+----------------------+
\nwts_dut_agent_v7.sh -h / --help | -u / --up -f [dut.conf] | -u -o FHOST/FMAC/SMAC AP/STA 1/2 20/40/80 [Feature] | -d / --down |
          

\n+----------------------+
\n| LAUNCH EXAMPLE       |
\n+----------------------+
\n How to start WTS Agent:
\n    wts_dut_agent_v7.sh -u -f dut.conf
\n    wts_dut_agent_v7.sh -u -o FHOST STA 1 20  

\n How to stop WTS Agent:
\n    wts_dut_agent_v7.sh -d
   
  
\n---
\nRelease : wts_dut_agent_v7 v10.1 (August 9 2018)
\nCopyright (c): RivieraWaves SAS 2010-2015
""" 


### Initiate script arguments
args=("$@")

### Local variables for DUT scripts copy
SRC_PATH="/mnt/nx_share/_SIGMA/Sigma_DUT/scripts/*"
DST_PATH="/valid/bin/"
DUT_CONF="${args[1]}"

### Setup
DUT_ENV_PLATFORM='0'
DUT_ENV_PLATFORM_ID='0'
DUT_ENV_UMAC='0'
DUT_ENV_CLOCKG0='0'
DUT_ENV_RADIO='0'
DUT_ENV_WIDTH='0'
DUT_ENV_IP_INDEX='0'
DUT_MACTRACE='0'
DUT_ENV_BRIDGE='0'
DUT_ENV_DEBUG='0'

### Features
DUT_ENV_AMSDUTX='0'
DUT_ENV_AGGTX='0'
DUT_ENV_GF='0'
DUT_ENV_PMF='0'
DUT_ENV_P2P='0'
DUT_ENV_11HAP='0'
DUT_ENV_11DAP='0'
DUT_ENV_DTIM='0'
DUT_ENV_OBSS_SCAN='0'
WPA_GROUP_REKEY='0'
WPA_STRICT_REKEY='0'

### Modules
DUT_ENV_AMSDUNB='0'
DUT_ENV_AMSDURX='0'
DUT_ENV_ANTDIV='0'
DUT_ENV_AP_UAPSD='0'
DUT_ENV_BFMEE='0'
DUT_ENV_BFMER='0'
DUT_ENV_HT_ON='0'
DUT_ENV_LDPC='0'
DUT_ENV_MCS_MAP='0'
DUT_ENV_MESH='0'
DUT_ENV_MURX='0'
DUT_ENV_MUTX='0'
DUT_ENV_MUTXON='0'
DUT_ENV_NSS='0'
DUT_ENV_PHYCFG='0'
DUT_ENV_PS='0'
DUT_ENV_SGI='0'
DUT_ENV_SGI80='0'
DUT_ENV_TDLS='0'
DUT_ENV_UAPSD='0'
DUT_ENV_UAPSD_T='0'
DUT_ENV_USE_2040='0'
DUT_ENV_USE_80='0'
DUT_ENV_VHT_ON='0'
DUT_ENV_VHT_STBC='0'
DUT_ENV_LP_CLK_PPM='0'
DUT_ENV_TX_LFT='0'
DUT_ENV_HE_UL='0'

echo "###########################################################"
echo "################### RW WTS DUT AGENT ######################"
echo "##################   Version 10.5.0   #####################"
echo "###########################################################"
echo -e ""
### Initiate Dini driver, wlan interface and run DUT
if [ "${args[0]}" == "-u" ]
then
	echo "+**********************************************************+"
   	echo "| Initiate Dini driver, wlan interface and run DUT...      |"
   	echo "+**********************************************************+"
	echo -e ""


	if [ "${args[1]}" == "-f" ]
	then
		### Take the path of file DUT_CONF from parameter
		DUT_CONF="${args[2]}"
	
	elif [ "${args[1]}" == "-o" ]
	then
		read -p 'IP (FHOST/FMAC/SMAC): ' ip	
		read -p 'Device Under Test (APUT/STAUT): ' dut
		read -p 'Bandwidth (20M/40M/80M): ' bindwidth 
		read -p 'Spatial Streams (1SS/2SS): ' streams
		read -p 'Feature (BF/BFEE...): ' feature

		DUT_CONF=$dut'_'$ip'_'$streams'_'$bindwidth

		### Add feature at the end of file name, if there is one
		if [ "$feature"  != "" ]
		then 
			DUT_CONF=$DUT_CONF'_'$feature
		fi	
		
		### Construct the path of File DUT_CONF by the option information
		DUT_CONF='conf/'$ip'/'$DUT_CONF'.txt'

		echo Try to find $DUT_CONF
	fi		

	if [ "$DUT_CONF" == "" ]
	then
		echo -e "\n<!> Argument missing: last dut.conf file shall be specified"
	elif [ -f $DUT_CONF ];
	then		
		### echo "File DUT_CONF exists."
		### Parse dut configuration file
		
		while read p; do
	        set -- `echo $p | tr '=' ' '`
	        key=$1
	        val=$2
		temp=param_$key
		printf -v $temp $val
		export $temp
		done <$DUT_CONF
		
		### Setup
		DUT_ENV_RADIO=`grep radio $DUT_CONF | cut -d'=' -f2`
		DUT_ENV_IP_INDEX=`grep ip $DUT_CONF | cut -d'=' -f2`
			
		### Modules
		DUT_ENV_PHYCFG=`grep phycfg $DUT_CONF | cut -d'=' -f2`
		
		if [ "$DUT_ENV_IP_INDEX" = "0" ]
		then
			echo -e "\n<!> Argument missing: last IP address byte shall be specified"
		elif [ "$DUT_ENV_RADIO" = "0" ]
		then	
			echo -e "\n<!> Argument missing: Radio shall be specified"
		elif [ "DUT_ENV_PHYCFG" = "0" ]
		then	
			echo -e "\n<!> Argument missing: radio channel path shall be specified (A, B, C)"	
		else
			
			# Creation of network bridges for Dini test environment
			#echo -e "\n>> Create Bridge br1 if not present"
			#./bridge_init.sh $DUT_ENV_IP_INDEX on
			
			# Create temporary file containing last wlan0 ip field value
			echo $DUT_ENV_IP_INDEX > wlan0ip.txt

			export WFA_ENV_AGENT_IPADDR=192.168.244.$DUT_ENV_IP_INDEX
			export WFA_ENV_AGENT_PORT=9000

			source /usr/lib/rwnx/init.sh
			echo -e "\n -->>> Starting WFA Agent..."
			/valid/bin/wfa_ca enp2s0 9000 #> wfa_ca_log.txt & # To be uncomment for background run
		fi
		
	else
		echo "File $DUT_CONF does not exist."
	fi	
		
# Remove wlan interface, driver, wpa supplicant, hostapd
elif [ "${args[0]}" == "-d" ]
then
	echo "+**********************************************************+"
   	echo "| Remove wlan interface, driver, wpa supplicant, hostapd...|"
   	echo "+**********************************************************+"

	echo -e "\n-->>> Kill DHCP Client or/and DHCP Server"
	pkill -f dhc

	echo -e "\n-->>> Down wlan0 interface"
	ifconfig wlp4s0 down

	echo -e "\n-->>> Remove rwnx driver"
	modprobe -r rwnx_drv 2> /dev/null
	modprobe -r rwnx_fdrv 2> /dev/null

	echo -e "\n-->>> Kill wpa supplicant"
	PID=$(pidof wpa_supplicant)
		if [ "$PID" == "" ]
		then
			echo "<!> wpa_supplicant process not found"
		else
			kill $PID
		fi

    echo -e "\n-->>> Kill WFA Control Agent"
	PID=$(pidof wfa_ca)
		if [ "$PID" == "" ]
		then
			echo "<!> wfa_ca process not found"
		else
			kill $PID
		fi

    echo -e "\n-->>> Kill hostapd"
	PID=$(pidof hostapd)
		if [ "$PID" == "" ]
		then
		   echo "<!> hostapd process not found"
		else
		   kill $PID
		fi
# Help
else
	echo -e $usage_str
fi
