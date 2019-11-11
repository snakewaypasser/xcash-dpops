#!/bin/bash

# Set the script to exit if any command fails
set -e

# Color print variables
COLOR_PRINT_RED="\033[1;31m"
COLOR_PRINT_GREEN="\033[1;32m"
COLOR_PRINT_YELLOW="\033[1;33m"
END_COLOR_PRINT="\033[0m"

# Configuration settings
INSTALLATION_TYPE_SETTINGS=1 # 1 = Install, 2 = Update, 3 = Uninstall
INSTALLATION_TYPE="Installation"
XCASH_DPOPS_INSTALLATION_DIR="$HOME/x-network/"
XCASH_BLOCKCHAIN_INSTALLATION_DIR="$HOME/.X-CASH/"
MONGODB_INSTALLATION_DIR="/data/db/"
SHARED_DELEGATE="YES"
WALLET_SETTINGS="YES"
WALLET_SEED=""
WALLET_PASSWORD=`< /dev/urandom tr -dc 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789' | head -c${1:-32};echo;`
DPOPS_FEE=0
DPOPS_MINIMUM_AMOUNT=0

# Latest versions
MONGODB_LATEST_VERSION="mongodb-linux-x86_64-ubuntu1804-4.2.1"
MONGOC_DRIVER_LATEST_VERSION="mongo-c-driver-1.15.2"
NODEJS_LATEST_VERSION="node-v13.1.0-linux-x64"

# Settings
XCASH_URL="https://github.com/X-CASH-official/X-CASH.git"
XCASH_DIR=""
XCASH_WALLET_DIR=""
XCASH_SYSTEMPID_DIR=""
XCASH_LOGS_DIR=""
XCASH_DPOPS_URL="https://github.com/X-CASH-official/XCASH_DPOPS.git"
XCASH_DPOPS_DIR=""
XCASH_DPOPS_SHARED_DELEGATE_FOLDER_DIR=""
SHARED_DELEGATES_WEBSITE_URL="https://github.com/X-CASH-official/XCASH_DPOPS_shared_delegates_website.git"
SHARED_DELEGATES_WEBSITE_DIR=""
NODEJS_URL="https://nodejs.org/dist/${NODEJS_LATEST_VERSION:5:7}/${NODEJS_LATEST_VERSION}.tar.xz"
NODEJS_DIR=""
NODEJS_CURRENT_VERSION=""
MONGODB_URL="http://downloads.mongodb.org/linux/${MONGODB_LATEST_VERSION}.tgz"
MONGODB_DIR=""
MONGODB_CURRENT_VERSION=""
MONGOC_DRIVER_URL="https://github.com/mongodb/mongo-c-driver/releases/download/${MONGOC_DRIVER_LATEST_VERSION:15}/${MONGOC_DRIVER_LATEST_VERSION}.tar.gz"
MONGOC_DRIVER_DIR=""
MONGOC_DRIVER_CURRENT_VERSION=""
LOGFILE=""
XCASH_DPOPS_PACKAGES="build-essential cmake pkg-config libboost-all-dev libssl-dev libzmq3-dev libunbound-dev libsodium-dev libminiupnpc-dev libunwind8-dev liblzma-dev libreadline6-dev libldns-dev libexpat1-dev libgtest-dev doxygen graphviz libpcsclite-dev git"
GIT_PULL_ALREADY_UPDATED_MESSAGE="Already up to date."
CURRENT_XCASH_WALLET_INFORMATION=""

# Files
FIREWALL=""
FIREWALL_SHARED_DELEGATES=""
SYSTEMD_SERVICE_FILE_FIREWALL=""
SYSTEMD_SERVICE_FILE_MONGODB=""
SYSTEMD_SERVICE_FILE_XCASH_DAEMON=""
SYSTEMD_SERVICE_FILE_XCASH_DPOPS_SOLO_DELEGATE=""
SYSTEMD_SERVICE_FILE_XCASH_DPOPS_SHARED_DELEGATE=""
SYSTEMD_SERVICE_FILE_XCASH_WALLET=""

# System settings
CPU_THREADS=`nproc`
RAM=`awk '/MemTotal/ { printf "%d \n", $2/1024/1024 }' /proc/meminfo`
RAM_CPU_RATIO=$((RAM / CPU_THREADS))
RAM_CPU_RATIO_ALL_CPU_THREADS=4

# Regex
regex_XCASH_DPOPS_INSTALLATION_DIR="(^\/(.*?)\/$)|(^$)" # anything that starts with / and ends with / and does not contain a space
regex_MNEMONIC_SEED="^\b([a-z]+\s+){24}\b([a-z]+)$" # 25 words exactly
regex_DPOPS_FEE="\b(^[1-9]{1}[0-9]{0,1}.?[0-9]{0,6}$)\b$" # between 1 and 99 with up to 6 decimal places
regex_DPOPS_MINIMUM_AMOUNT="\b(^[1-9]{1}[0-9]{4,6}$)\b$" # between 10000 and 10000000-1



# Functions
function get_installation_settings()
{
  echo -ne "${COLOR_PRINT_YELLOW}Installation Type (Install)\n1 = Install\n2 = Update\n3 = Uninstall\nEnter the number of the installation type: ${END_COLOR_PRINT}"
  read data
  INSTALLATION_TYPE_SETTINGS=$([ "$data" == "2" ] || [ "$data" == "3" ] && echo "$data" || echo "1")
  INSTALLATION_TYPE=$([ "$INSTALLATION_TYPE_SETTINGS" == "1" ] && echo "Installation") || ([ "$INSTALLATION_TYPE_SETTINGS" == "2" ] && echo "Update") || ([ "$INSTALLATION_TYPE_SETTINGS" == "3" ] && echo "Uninstall")
  echo -ne "\r"
  echo
  # Check if XCASH_DPOPS is already installed, if the user choose to install
  if [ "$INSTALLATION_TYPE_SETTINGS" -eq "1" ]; then
    echo -ne "${COLOR_PRINT_YELLOW}Checking if XCASH_DPOPS is already installed${END_COLOR_PRINT}"
    data=`sudo find / -type d -name "XCASH_DPOPS" | wc -l`
    if [ "$data" -ne "0" ]; then
      echo -e "\n${COLOR_PRINT_RED}XCASH_DPOPS is already installed. You can either update or uninstall${END_COLOR_PRINT}"
      exit 1
    fi
    echo -ne "\r                                                    "
    echo
  fi

  # Check if XCASH_DPOPS is not installed, if the user choose to update or uninstall
  if [ "$INSTALLATION_TYPE_SETTINGS" -ne "1" ]; then
    data=`sudo find / -type d -name "XCASH_DPOPS" | wc -l`
    if [ "$data" -eq "0" ]; then
      echo -e "\n${COLOR_PRINT_RED}XCASH_DPOPS is not installed. Please install XCASH_DPOPS before running update or uninstall${END_COLOR_PRINT}"
      exit 1
    fi
    echo -ne "\r                                                     "
    echo
  fi
}

function get_xcash_dpops_installation_directory()
{
  while
    echo -ne "${COLOR_PRINT_YELLOW}Installation Directory, must be in the form of /directory/ ($HOME/x-network/): ${END_COLOR_PRINT}"
    read data    
    echo -ne "\r"
    echo
    [[ ! $data =~ $regex_XCASH_DPOPS_INSTALLATION_DIR ]]
  do true; done
  
  XCASH_DPOPS_INSTALLATION_DIR=$([ "$data" == "" ] && echo "$XCASH_DPOPS_INSTALLATION_DIR" || echo "$data")
}

function get_xcash_blockchain_xcash_dpops_installation_directory()
{
  while
    echo -ne "${COLOR_PRINT_YELLOW}X-CASH Blockchain Installation Directory, must be in the form of /directory/ ($HOME/.X-CASH/): ${END_COLOR_PRINT}"
    read data
    echo -ne "\r"
    echo
    [[ ! $data =~ $regex_XCASH_DPOPS_INSTALLATION_DIR ]]
  do true; done

  XCASH_BLOCKCHAIN_INSTALLATION_DIR=$([ "$data" == "" ] && echo "$XCASH_BLOCKCHAIN_INSTALLATION_DIR" || echo "$data")
}

function get_mongodb_installation_directory()
{
  while
    echo -ne "${COLOR_PRINT_YELLOW}MongoDB Installation Directory, must be in the form of /directory/ (/data/db/): ${END_COLOR_PRINT}"
    read data
    echo -ne "\r"
    echo
    [[ ! $data =~ $regex_XCASH_DPOPS_INSTALLATION_DIR ]]
  do true; done

  MONGODB_INSTALLATION_DIR=$([ "$data" == "" ] && echo "$MONGODB_INSTALLATION_DIR" || echo "$data")
}

function update_global_variables()
{
  XCASH_DIR=${XCASH_DPOPS_INSTALLATION_DIR}X-CASH/
  XCASH_WALLET_DIR=${XCASH_DPOPS_INSTALLATION_DIR}xcash_wallets/
  XCASH_SYSTEMPID_DIR=${XCASH_DPOPS_INSTALLATION_DIR}systemdpid/
  XCASH_LOGS_DIR=${XCASH_DPOPS_INSTALLATION_DIR}logs/
  XCASH_DPOPS_DIR=${XCASH_DPOPS_INSTALLATION_DIR}XCASH_DPOPS/
  XCASH_DPOPS_SHARED_DELEGATE_FOLDER_DIR=${XCASH_DPOPS_DIR}shared_delegates_website/
  SHARED_DELEGATES_WEBSITE_DIR=${XCASH_DPOPS_INSTALLATION_DIR}XCASH_DPOPS_shared_delegates_website/
  NODEJS_DIR=${XCASH_DPOPS_INSTALLATION_DIR}${NODEJS_LATEST_VERSION}/
  MONGODB_DIR=${XCASH_DPOPS_INSTALLATION_DIR}${MONGODB_LATEST_VERSION}/
  LOGFILE=${XCASH_DPOPS_INSTALLATION_DIR}XCASH_DPOPS_INSTALL.log
}

function get_shared_delegate_installation_settings()
{
  echo -ne "${COLOR_PRINT_YELLOW}Shared Delegate (YES): ${END_COLOR_PRINT}"
  read data
  SHARED_DELEGATE=$([ "$data" == "" ] && echo "${SHARED_DELEGATE^^}" || echo "NO")
  echo -ne "\r"
  echo
  if [ "${SHARED_DELEGATE^^}" == "YES" ]; then    
    while
      echo -ne "${COLOR_PRINT_YELLOW}Shared Delegate Fee (in percentage ex: 1 or 1.5 etc): ${END_COLOR_PRINT}"
      read DPOPS_FEE
      echo -ne "\r"
      echo
      [[ ! $DPOPS_FEE =~ $regex_DPOPS_FEE ]]
    do true; done
    
    while
      echo -ne "${COLOR_PRINT_YELLOW}Shared Delegate Minimum Payment Amount, minimum is 10K, maximum is 10M (ex: 10000 in whole numbers and not atomic units etc): ${END_COLOR_PRINT}"
      read DPOPS_MINIMUM_AMOUNT
      echo -ne "\r"
      echo
      [[ ! $DPOPS_MINIMUM_AMOUNT =~ $regex_DPOPS_MINIMUM_AMOUNT ]]
    do true; done
  fi
}



function get_wallet_settings()
{
  echo -ne "${COLOR_PRINT_YELLOW}Create a New Wallet to Collect Block Rewards (YES): ${END_COLOR_PRINT}"
  read data
  echo -ne "\r"
  echo
  WALLET_SETTINGS=$([ "$data" == "" ] && echo "$WALLET_SETTINGS" || echo "NO")
  if [ "$WALLET_SETTINGS" == "NO" ]; then
    while
      echo -ne "${COLOR_PRINT_YELLOW}Please Enter the Existing Wallets Mnemonic Seed: ${END_COLOR_PRINT}"
      read WALLET_SEED
      echo -ne "\r"
      echo
      [[ ! $WALLET_SEED =~ $regex_MNEMONIC_SEED ]]
    do true; done
  fi
}

function get_password_settings()
{
  echo -ne "${COLOR_PRINT_YELLOW}Generate a New Password (YES): ${END_COLOR_PRINT}"
  read data
  echo -ne "\r"
  echo
  WALLET_PASSWORD=$([ "$data" == "" ] && echo "$WALLET_PASSWORD" || echo "NO")
  if [ "$WALLET_PASSWORD" == "NO" ]; then
  echo -ne "${COLOR_PRINT_YELLOW}Please Enter the Custom Password: ${END_COLOR_PRINT}"
  read WALLET_PASSWORD
  echo -ne "\r"
  echo
  fi
}

function get_password()
{
  # ask for the root password if not root so it wont ask again when installing
  if [ "$EUID" -ne 0 ]; then
    sudo echo
  fi
}  

function print_installation_settings()
{
  echo
  echo -e "${COLOR_PRINT_GREEN}############################################################${END_COLOR_PRINT}"
  echo -e "${COLOR_PRINT_GREEN}                    Installation Settings${END_COLOR_PRINT}"
  echo -e "${COLOR_PRINT_GREEN}############################################################${END_COLOR_PRINT}"
  echo
  echo -e "${COLOR_PRINT_GREEN}Installation Type: ${INSTALLATION_TYPE}${END_COLOR_PRINT}"
  echo -e "${COLOR_PRINT_GREEN}Installation Directory: ${XCASH_DPOPS_INSTALLATION_DIR} ${END_COLOR_PRINT}"
  echo -e "${COLOR_PRINT_GREEN}X-CASH Blockchain Installation Directory: ${XCASH_BLOCKCHAIN_INSTALLATION_DIR} ${END_COLOR_PRINT}"
  echo -e "${COLOR_PRINT_GREEN}MongoDB Installation Directory: ${MONGODB_INSTALLATION_DIR} ${END_COLOR_PRINT}"
  echo -e "${COLOR_PRINT_GREEN}Shared Delegate: ${SHARED_DELEGATE} ${END_COLOR_PRINT}"
  echo -e "${COLOR_PRINT_GREEN}Create New Wallet: ${WALLET_SETTINGS} ${END_COLOR_PRINT}"
  echo -e "${COLOR_PRINT_GREEN}Wallet Password: ${WALLET_PASSWORD} ${END_COLOR_PRINT}"
  echo -e "${COLOR_PRINT_GREEN}User: ${USER} ${END_COLOR_PRINT}"
  echo -e "${COLOR_PRINT_GREEN}DPOPS Fee: ${DPOPS_FEE} ${END_COLOR_PRINT}"
  echo -e "${COLOR_PRINT_GREEN}DPOPS Minimum Payment Amount: ${DPOPS_MINIMUM_AMOUNT} ${END_COLOR_PRINT}"
  echo -e "${COLOR_PRINT_GREEN}Installation Log File: ${LOGFILE} ${END_COLOR_PRINT}"
  echo
  echo -e "${COLOR_PRINT_GREEN}If ${INSTALLATION_TYPE} fails please open ${LOGFILE} to find out where it left off${END_COLOR_PRINT}"

  seconds=10
  while [ "$seconds" -ne 0 ]
  do
    echo -ne "${COLOR_PRINT_GREEN}${INSTALLATION_TYPE} will start in ${seconds} seconds, press Ctrl + C to cancel!${END_COLOR_PRINT}"
    seconds=$((seconds-1))
    sleep 1
    echo -ne "\r"
  done 
  echo -ne "${COLOR_PRINT_GREEN}${INSTALLATION_TYPE} will start in 0 seconds, press Ctrl + C to cancel!${END_COLOR_PRINT}"
  echo
  echo
}

function installation_settings()
{
  echo
  echo -e "${COLOR_PRINT_GREEN}############################################################${END_COLOR_PRINT}"
  echo -e "${COLOR_PRINT_GREEN}         Welcome to X-Cash DPoPS auto-install script  ${END_COLOR_PRINT}"
  echo -e "${COLOR_PRINT_GREEN}############################################################${END_COLOR_PRINT}"
  echo
  echo -e "${COLOR_PRINT_YELLOW}Installation configuration (Press ENTER for default)${END_COLOR_PRINT}"
  echo
  get_password
  get_installation_settings
  if [ "$INSTALLATION_TYPE_SETTINGS" -eq "1" ]; then 
    get_xcash_dpops_installation_directory
    get_xcash_blockchain_xcash_dpops_installation_directory
    get_mongodb_installation_directory
    update_global_variables
    get_shared_delegate_installation_settings
    get_wallet_settings
    get_password_settings
    print_installation_settings
  fi
}










function get_current_xcash_wallet_data()
{
  echo -ne "${COLOR_PRINT_YELLOW}Getting Current X-CASH Wallet Data${END_COLOR_PRINT}"

  screen -dmS XCASH_RPC_Wallet ${XCASH_DIR}build/release/bin/xcash-wallet-rpc --wallet-file ${XCASH_DPOPS_INSTALLATION_DIR}xcash_wallets/XCASH_DPOPS_WALLET --password ${WALLET_PASSWORD} --rpc-bind-port 18288 --confirm-external-bind --disable-rpc-login --daemon-address delegates.xcash.foundation:18281 --trusted-daemon
  sleep 10s
  
   while
    data=`curl -s -X POST http://127.0.0.1:18288/json_rpc -d '{"jsonrpc":"2.0","id":"0","method":"get_address"}' -H 'Content-Type: application/json'` 
    sleep 10s
    [[ "$data" == "" ]]
  do true; done

  PUBLIC_ADDRESS=`curl -s -X POST http://127.0.0.1:18288/json_rpc -d '{"jsonrpc":"2.0","id":"0","method":"get_address"}' -H 'Content-Type: application/json' | grep \"address\" | head -1 | sed s"|    \"address\": ||g" | sed s"|\"||g" | sed s"|,||g"`
  SPEND_KEY=`curl -s -X POST http://127.0.0.1:18288/json_rpc -d '{"jsonrpc":"2.0","id":"0","method":"query_key","params":{"key_type":"spend_key"}}' -H 'Content-Type: application/json' | grep \"key\" | sed s"|    \"key\": ||g" | sed s"|\"||g"`
  VIEW_KEY=`curl -s -X POST http://127.0.0.1:18288/json_rpc -d '{"jsonrpc":"2.0","id":"0","method":"query_key","params":{"key_type":"view_key"}}' -H 'Content-Type: application/json' | grep \"key\" | sed s"|    \"key\": ||g" | sed s"|\"||g"`
  MNEMONIC_SEED=`curl -s -X POST http://127.0.0.1:18288/json_rpc -d '{"jsonrpc":"2.0","id":"0","method":"query_key","params":{"key_type":"mnemonic"}}' -H 'Content-Type: application/json' | grep \"key\" | sed s"|    \"key\": ||g" | sed s"|\"||g"`
  CURRENT_XCASH_WALLET_INFORMATION="${COLOR_PRINT_GREEN}############################################################\n                 X-CASH Wallet Data  \n############################################################${END_COLOR_PRINT}\n\n${COLOR_PRINT_YELLOW}Public Address: $PUBLIC_ADDRESS\nMnemonic Seed: $MNEMONIC_SEED\nSpend Key: $SPEND_KEY\nView Key: $VIEW_KEY\nWallet Password: $WALLET_PASSWORD${END_COLOR_PRINT}"

  curl -s -X POST http://127.0.0.1:18288/json_rpc -d '{"jsonrpc":"2.0","id":"0","method":"stop_wallet"}' -H 'Content-Type: application/json' >> ${LOGFILE} 2>&1
  sleep 10s
  
  echo -ne "\r${COLOR_PRINT_GREEN}Getting Current X-CASH Wallet Data${END_COLOR_PRINT}"
  echo
}

function start_systemd_service_files()
{
  echo -ne "${COLOR_PRINT_YELLOW}Starting Processes${END_COLOR_PRINT}"
  screen -dmS MongoDB ${MONGODB_DIR}bin/mongod --dbpath ${MONGODB_INSTALLATION_DIR}
  screen -dmS XCASH_Daemon ${XCASH_DIR}build/release/bin/xcashd --data-dir ${XCASH_BLOCKCHAIN_INSTALLATION_DIR} --rpc-bind-ip 0.0.0.0 --rpc-bind-port 18281 --restricted-rpc --confirm-external-bind
  sleep 10s
  screen -dmS XCASH_Wallet ${XCASH_DIR}build/release/bin/xcash-wallet-rpc --wallet-file ${XCASH_DPOPS_INSTALLATION_DIR}xcash_wallets/XCASH_DPOPS_WALLET --password ${WALLET_PASSWORD} --rpc-bind-port 18285 --confirm-external-bind --daemon-port 18281 --disable-rpc-login --trusted-daemon
  sleep 10s
  screen -dmS XCASH_DPOPS ${XCASH_DPOPS_DIR}build/XCASH_DPOPS --shared_delegates_website --fee ${DPOPS_FEE} --minimum_amount ${DPOPS_MINIMUM_AMOUNT}
  echo -ne "\r${COLOR_PRINT_GREEN}Starting Processes${END_COLOR_PRINT}"
  echo
}

function stop_systemd_service_files()
{
  echo -ne "${COLOR_PRINT_YELLOW}Stoping Processes${END_COLOR_PRINT}"
  screen -XS "MongoDB" quit
  screen -XS "XCASH_Daemon" quit
  screen -XS "XCASH_Wallet" quit
  screen -XS "XCASH_DPOPS" quit
  echo -ne "\r${COLOR_PRINT_GREEN}Stoping Processes${END_COLOR_PRINT}"
  echo
}














function update_packages_list()
{
    i=0
    while fuser /var/{lib/{dpkg,apt/lists},cache/apt/archives}/lock >/dev/null 2>&1; do
        case $(($i % 4)) in
            0 ) j="-" ;;
            1 ) j="\\" ;;
            2 ) j="|" ;;
            3 ) j="/" ;;
        esac
        echo && echo -en "\r${COLOR_PRINT_RED}[$j] Waiting for other package manager to finish...${END_COLOR_PRINT}" 
        sleep 0.25
        ((i=i+1))
    done
    echo -ne "${COLOR_PRINT_YELLOW}Updating Packages List${END_COLOR_PRINT}"
    sudo apt update -y >> ${LOGFILE} 2>&1
    echo -ne "\r${COLOR_PRINT_GREEN}Updating Packages List${END_COLOR_PRINT}"
    echo
}

function install_packages()
{
    i=0
    while fuser /var/{lib/{dpkg,apt/lists},cache/apt/archives}/lock >/dev/null 2>&1; do
        case $(($i % 4)) in
            0 ) j="-" ;;
            1 ) j="\\" ;;
            2 ) j="|" ;;
            3 ) j="/" ;;
        esac
        echo && echo -en "\r${COLOR_PRINT_RED}[$j] Waiting for other package manager to finish...${END_COLOR_PRINT}" 
        sleep 0.25
        ((i=i+1))
    done
    echo -ne "${COLOR_PRINT_YELLOW}Installing Packages (This Might Take A While)${END_COLOR_PRINT}"
    sudo apt install ${XCASH_DPOPS_PACKAGES} -y >> ${LOGFILE} 2>&1
    build_libgtest
    echo -ne "\r${COLOR_PRINT_GREEN}Installing Packages (This Might Take A While)${END_COLOR_PRINT}"
    echo
}

function build_libgtest()
{
  cd /usr/src/gtest >> ${LOGFILE} 2>&1
  sudo cmake . >> ${LOGFILE} 2>&1
  sudo make >> ${LOGFILE} 2>&1
  sudo mv libg* /usr/lib/ >> ${LOGFILE} 2>&1
}










function download_xcash()
{
  echo -ne "${COLOR_PRINT_YELLOW}Downloading X-CASH${END_COLOR_PRINT}"
  cd ${XCASH_DPOPS_INSTALLATION_DIR}
  git clone ${XCASH_URL} >> ${LOGFILE} 2>&1
  echo -ne "\r${COLOR_PRINT_GREEN}Downloading X-CASH${END_COLOR_PRINT}"
  echo
}

function build_xcash()
{
  echo -ne "${COLOR_PRINT_YELLOW}Building X-CASH (This Might Take A While)${END_COLOR_PRINT}"
  cd ${XCASH_DIR}
  if [ "$RAM_CPU_RATIO" -ge "$RAM_CPU_RATIO_ALL_CPU_THREADS" ]; then
    make release -j ${CPU_THREADS} >> ${LOGFILE} 2>&1
  else
    make release -j $((CPU_THREADS / 2)) >> ${LOGFILE} 2>&1
  fi
  echo -ne "\r${COLOR_PRINT_GREEN}Building X-CASH (This Might Take A While)${END_COLOR_PRINT}"
  echo
}

function install_xcash()
{
  echo
  echo
  echo -e "${COLOR_PRINT_GREEN}############################################################${END_COLOR_PRINT}"
  echo -e "${COLOR_PRINT_GREEN}                   Installing X-CASH${END_COLOR_PRINT}"
  echo -e "${COLOR_PRINT_GREEN}############################################################${END_COLOR_PRINT}"
  download_xcash
  build_xcash
}










function create_directories()
{
  echo -ne "${COLOR_PRINT_YELLOW}Creating Directories${END_COLOR_PRINT}"
  if [ ! -d "$XCASH_DPOPS_INSTALLATION_DIR" ]; then
    mkdir -p ${XCASH_DPOPS_INSTALLATION_DIR}
  fi 
  if [ ! -d "$XCASH_BLOCKCHAIN_INSTALLATION_DIR" ]; then
    mkdir -p ${XCASH_BLOCKCHAIN_INSTALLATION_DIR}
  fi 
  if [ ! -d "$MONGODB_INSTALLATION_DIR" ]; then
    sudo mkdir -p ${MONGODB_INSTALLATION_DIR}
    sudo chmod 770 ${MONGODB_INSTALLATION_DIR}
    sudo chown $USER ${MONGODB_INSTALLATION_DIR}
  fi 
  if [ ! -d "$XCASH_WALLET_DIR" ]; then
    mkdir -p ${XCASH_WALLET_DIR}
  fi
  if [ ! -d "$XCASH_SYSTEMPID_DIR" ]; then
    mkdir -p ${XCASH_SYSTEMPID_DIR}
  fi
  if [ ! -d "$XCASH_LOGS_DIR" ]; then
    mkdir -p ${XCASH_LOGS_DIR}
  fi
  echo -ne "\r${COLOR_PRINT_GREEN}Creating Directories${END_COLOR_PRINT}"
  echo
}

function create_files()
{
  touch ${XCASH_SYSTEMPID_DIR}mongod.pid ${XCASH_SYSTEMPID_DIR}xcash_daemon.pid ${LOGFILE}
}

function install_mongodb()
{
  echo -ne "${COLOR_PRINT_YELLOW}Installing MongoDB${END_COLOR_PRINT}"
  cd ${XCASH_DPOPS_INSTALLATION_DIR}
  wget -q ${MONGODB_URL}
  tar -xf mongodb-linux-x86_64-*.tgz >> ${LOGFILE} 2>&1
  sudo rm mongodb-linux-x86_64-*.tgz >> ${LOGFILE} 2>&1
  sudo echo -ne "\nexport PATH=${MONGODB_DIR}bin:" >> ~/.profile 
  sudo echo -ne '$PATH' >> ~/.profile
  source ~/.profile
  echo -ne "\r${COLOR_PRINT_GREEN}Installing MongoDB${END_COLOR_PRINT}"
  echo
}

function install_mongoc_driver()
{
  echo -ne "${COLOR_PRINT_YELLOW}Installing MongoC Driver${END_COLOR_PRINT}"
  cd ${XCASH_DPOPS_INSTALLATION_DIR}
  wget -q ${MONGOC_DRIVER_URL}
  tar -xf mongo-c-driver-*.tar.gz >> ${LOGFILE} 2>&1
  sudo rm mongo-c-driver-*.tar.gz >> ${LOGFILE} 2>&1
  cd mongo-c-driver-*
  mkdir cmake-build >> ${LOGFILE} 2>&1
  cd cmake-build >> ${LOGFILE} 2>&1
  sudo cmake -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF .. >> ${LOGFILE} 2>&1
  sudo make -j ${CPU_THREADS} >> ${LOGFILE} 2>&1
  sudo make install >> ${LOGFILE} 2>&1
  sudo ldconfig
  echo -ne "\r${COLOR_PRINT_GREEN}Installing MongoC Driver${END_COLOR_PRINT}"
  echo
}

function download_xcash_dpops()
{
  echo -ne "${COLOR_PRINT_YELLOW}Downloading XCASH_DPOPS${END_COLOR_PRINT}"
  cd ${XCASH_DPOPS_INSTALLATION_DIR}
  git clone ${XCASH_DPOPS_URL} >> ${LOGFILE} 2>&1
  echo -ne "\r${COLOR_PRINT_GREEN}Downloading XCASH_DPOPS${END_COLOR_PRINT}"
  echo
}

function build_xcash_dpops()
{
  echo -ne "${COLOR_PRINT_YELLOW}Building XCASH_DPOPS${END_COLOR_PRINT}"
  cd ${XCASH_DPOPS_DIR}
  if [ "$RAM_CPU_RATIO" -ge "$RAM_CPU_RATIO_ALL_CPU_THREADS" ]; then
    make release -j ${CPU_THREADS} >> ${LOGFILE} 2>&1
  else
    make release -j $((CPU_THREADS / 2)) >> ${LOGFILE} 2>&1
  fi
  echo -ne "\r${COLOR_PRINT_GREEN}Building XCASH_DPOPS${END_COLOR_PRINT}"
  echo
}

function install_xcash_dpops()
{
  echo
  echo
  echo -e "${COLOR_PRINT_GREEN}############################################################${END_COLOR_PRINT}"
  echo -e "${COLOR_PRINT_GREEN}                Installing XCASH_DPOPS${END_COLOR_PRINT}"
  echo -e "${COLOR_PRINT_GREEN}############################################################${END_COLOR_PRINT}"
  install_mongodb
  install_mongoc_driver
  download_xcash_dpops
  build_xcash_dpops
  echo
  echo
}

function sync_xcash_wallet()
{
  echo -e "${COLOR_PRINT_GREEN}############################################################${END_COLOR_PRINT}"
  echo -e "${COLOR_PRINT_GREEN}      Syncing X-CASH Wallet (This Might Take A While)${END_COLOR_PRINT}"
  echo -e "${COLOR_PRINT_GREEN}############################################################${END_COLOR_PRINT}"

  screen -dmS XCASH_RPC_Wallet ${XCASH_DIR}build/release/bin/xcash-wallet-rpc --wallet-file ${XCASH_DPOPS_INSTALLATION_DIR}xcash_wallets/XCASH_DPOPS_WALLET --password ${WALLET_PASSWORD} --rpc-bind-port 18288 --confirm-external-bind --disable-rpc-login --daemon-address delegates.xcash.foundation:18281 --trusted-daemon
  
   while
    data=`curl -s -X POST http://127.0.0.1:18288/json_rpc -d '{"jsonrpc":"2.0","id":"0","method":"get_address"}' -H 'Content-Type: application/json'` 
    sleep 10s
    [[ "$data" == "" ]]
  do true; done

  curl -s -X POST http://127.0.0.1:18288/json_rpc -d '{"jsonrpc":"2.0","id":"0","method":"stop_wallet"}' -H 'Content-Type: application/json' >> ${LOGFILE} 2>&1
  sleep 10s
}

function create_xcash_wallet()
{
  echo -e "${COLOR_PRINT_GREEN}############################################################${END_COLOR_PRINT}"
  echo -e "${COLOR_PRINT_GREEN}      Creating X-CASH Wallet (This Might Take A While)  ${END_COLOR_PRINT}"
  echo -e "${COLOR_PRINT_GREEN}############################################################${END_COLOR_PRINT}"
  echo "exit" | ${XCASH_DIR}build/release/bin/xcash-wallet-cli --generate-new-wallet ${XCASH_DPOPS_INSTALLATION_DIR}xcash_wallets/XCASH_DPOPS_WALLET --password ${WALLET_PASSWORD} --mnemonic-language English --restore-height 0 --daemon-address delegates.xcash.foundation:18281 >> ${LOGFILE} 2>&1
  echo
  echo
}

function import_xcash_wallet()
{
  echo -e "${COLOR_PRINT_GREEN}############################################################${END_COLOR_PRINT}"
  echo -e "${COLOR_PRINT_GREEN}                   Importing X-CASH Wallet ${END_COLOR_PRINT}"
  echo -e "${COLOR_PRINT_GREEN}############################################################${END_COLOR_PRINT}"
  (echo -ne "\n"; echo "${WALLET_PASSWORD}"; echo "exit") | ${XCASH_DIR}build/release/bin/xcash-wallet-cli --restore-deterministic-wallet --electrum-seed "${WALLET_SEED}" --generate-new-wallet ${XCASH_DPOPS_INSTALLATION_DIR}xcash_wallets/XCASH_DPOPS_WALLET --password ${WALLET_PASSWORD} --mnemonic-language English --restore-height 0 --daemon-address delegates.xcash.foundation:18281 >> ${LOGFILE} 2>&1
  echo
  echo
}










function install_nodejs()
{
  echo -ne "${COLOR_PRINT_YELLOW}Installing Node.js${END_COLOR_PRINT}"
  cd ${XCASH_DPOPS_INSTALLATION_DIR}
  wget -q ${NODEJS_URL}
  tar -xf node*.tar.xz >> ${LOGFILE} 2>&1
  sudo rm node*.tar.xz >> ${LOGFILE} 2>&1
  sudo echo -ne "\nexport PATH=${NODEJS_DIR}bin:" >> ~/.profile 
  sudo echo -ne '$PATH' >> ~/.profile
  source ~/.profile
  echo -ne "\r${COLOR_PRINT_GREEN}Installing Node.js${END_COLOR_PRINT}"
  echo
}

function configure_npm()
{
  if [ "$EUID" -eq 0 ]; then
    echo -ne "${COLOR_PRINT_YELLOW}Configuring NPM For Root User${END_COLOR_PRINT}"
    npm config set user 0 >> ${LOGFILE} 2>&1
    npm config set unsafe-perm true >> ${LOGFILE} 2>&1
    echo -ne "\r${COLOR_PRINT_GREEN}Configuring NPM For Root User${END_COLOR_PRINT}"
    echo
  fi
}

function update_npm()
{
  echo -ne "${COLOR_PRINT_YELLOW}Updating NPM${END_COLOR_PRINT}"
  npm install -g npm >> ${LOGFILE} 2>&1
  echo -ne "\r${COLOR_PRINT_GREEN}Updating NPM${END_COLOR_PRINT}"
  echo
}

function install_npm_global_packages()
{
  echo -ne "${COLOR_PRINT_YELLOW}Installing Global NPM Packages${END_COLOR_PRINT}"
  npm install -g @angular/cli@latest uglify-js >> ${LOGFILE} 2>&1
  echo -ne "\r${COLOR_PRINT_GREEN}Installing Global NPM Packages${END_COLOR_PRINT}"
  echo
}

function download_shared_delegate_website()
{
  echo -ne "${COLOR_PRINT_YELLOW}Downloading Shared Delegates Website${END_COLOR_PRINT}"
  cd ${XCASH_DPOPS_INSTALLATION_DIR}
  git clone ${SHARED_DELEGATES_WEBSITE_URL} >> ${LOGFILE} 2>&1
  echo -ne "\r${COLOR_PRINT_GREEN}Downloading Shared Delegates Website${END_COLOR_PRINT}"
  echo
}

function install_shared_delegates_website_npm_packages()
{
  echo -ne "${COLOR_PRINT_YELLOW}Updating node_modules${END_COLOR_PRINT}"
  cd ${SHARED_DELEGATES_WEBSITE_DIR}
  npm update >> ${LOGFILE} 2>&1
  echo -ne "\r${COLOR_PRINT_GREEN}Updating node_modules${END_COLOR_PRINT}"
  echo
}

function build_shared_delegates_website()
{
  echo -ne "${COLOR_PRINT_YELLOW}Building shared delegates website${END_COLOR_PRINT}"
  cd ${SHARED_DELEGATES_WEBSITE_DIR}
  ng build --prod --aot >> ${LOGFILE} 2>&1
  cd dist
  for f in *.js; do uglifyjs $f --compress --mangle --output "{$f}min"; rm $f; mv "{$f}min" $f; done
  if [ -d "$XCASH_DPOPS_SHARED_DELEGATE_FOLDER_DIR" ]; then
    sudo rm -r ${XCASH_DPOPS_SHARED_DELEGATE_FOLDER_DIR}
  fi 
  cd ../
  cp -a dist ${XCASH_DPOPS_SHARED_DELEGATE_FOLDER_DIR} 
  echo -ne "\r${COLOR_PRINT_GREEN}Building shared delegates website${END_COLOR_PRINT}"
  echo
}

function install_shared_delegates_website()
{
  echo -e "${COLOR_PRINT_GREEN}############################################################${END_COLOR_PRINT}"
  echo -e "${COLOR_PRINT_GREEN}            Installing Shared Delegate Website${END_COLOR_PRINT}"
  echo -e "${COLOR_PRINT_GREEN}############################################################${END_COLOR_PRINT}"
  install_nodejs
  configure_npm
  update_npm
  install_npm_global_packages
  download_shared_delegate_website
  install_shared_delegates_website_npm_packages
  build_shared_delegates_website
  source ~/.profile
  echo
  echo
}










function get_installation_directory()
{
  echo -ne "${COLOR_PRINT_YELLOW}Getting Installation Directories${END_COLOR_PRINT}"
  XCASH_DPOPS_INSTALLATION_DIR=`sudo find / -type d -name "XCASH_DPOPS" -exec dirname {} \;`/
  WALLET_PASSWORD=`cat /lib/systemd/system/XCASH_Wallet.service | awk '/password/ {print $5}'`
  XCASH_DIR=${XCASH_DPOPS_INSTALLATION_DIR}X-CASH/
  XCASH_WALLET_DIR=${XCASH_DPOPS_INSTALLATION_DIR}xcash_wallets/
  XCASH_SYSTEMPID_DIR=${XCASH_DPOPS_INSTALLATION_DIR}systemdpid/
  XCASH_LOGS_DIR=${XCASH_DPOPS_INSTALLATION_DIR}logs/
  XCASH_DPOPS_DIR=${XCASH_DPOPS_INSTALLATION_DIR}XCASH_DPOPS/
  XCASH_DPOPS_SHARED_DELEGATE_FOLDER_DIR=${XCASH_DPOPS_DIR}shared_delegates_website/
  SHARED_DELEGATES_WEBSITE_DIR=${XCASH_DPOPS_INSTALLATION_DIR}XCASH_DPOPS_shared_delegates_website/
  NODEJS_DIR=`sudo find / -type d -name "node-*-linux-x64"`/
  MONGODB_DIR=`sudo find / -type d -name "mongodb-linux-x86_64-ubuntu1804-*"`/
  MONGOC_DRIVER_DIR=`sudo find / -type d -name "mongo-c-driver-*"`/
  LOGFILE=${XCASH_DPOPS_INSTALLATION_DIR}XCASH_DPOPS_INSTALL.log
  echo -ne "\r${COLOR_PRINT_GREEN}Getting Installation Directories${END_COLOR_PRINT}"
  echo

  # check to make sure it found the installed programs
  if [ "$XCASH_DPOPS_INSTALLATION_DIR" == "/" ]; then
    echo -e "${COLOR_PRINT_RED}Can not find the installation directory, Please run the auto installer in installation mode${END_COLOR_PRINT}"
  fi
  if [ "$XCASH_DIR" == "X-CASH/" ]; then
    echo -e "${COLOR_PRINT_RED}Can not find X-CASH${END_COLOR_PRINT}"
    install_xcash
  fi
  if [ "$XCASH_DPOPS_DIR" == "XCASH_DPOPS/" ]; then
    echo -e "${COLOR_PRINT_RED}Can not find XCASH_DPOPS${END_COLOR_PRINT}"
    download_xcash_dpops
    build_xcash_dpops
  fi
  if [ "$MONGODB_DIR" == "/" ]; then
    echo -e "${COLOR_PRINT_RED}Can not find MongoDB${END_COLOR_PRINT}"
    install_mongodb
  fi
  if [ "$MONGOC_DRIVER_DIR" == "/" ]; then
    echo -e "${COLOR_PRINT_RED}Can not find Mongo C Driver${END_COLOR_PRINT}"
    install_mongoc_driver
  fi
  if [ "${SHARED_DELEGATE^^}" == "YES" ] ; then
    if [ "$SHARED_DELEGATES_WEBSITE_DIR" == "XCASH_DPOPS_shared_delegates_website/" ]; then
      echo -e "${COLOR_PRINT_RED}Can not find XCASH_DPOPS_shared_delegates_website${END_COLOR_PRINT}"
      download_shared_delegate_website
      install_shared_delegates_website_npm_packages
      build_shared_delegates_website
    fi
    if [ "$NODEJS_DIR" == "/" ]; then
      echo -e "${COLOR_PRINT_RED}Can not find NodeJS${END_COLOR_PRINT}"
      install_nodejs
      configure_npm
      update_npm
      install_npm_global_packages
      source ~/.profile
    fi
  fi
}

function get_dependencies_current_version()
{
  echo -ne "${COLOR_PRINT_YELLOW}Getting Dependencies Current Versions${END_COLOR_PRINT}"
  NODEJS_CURRENT_VERSION=`sudo find / -type d -name "node-*-linux-x64" -exec basename {} \;`
  MONGODB_CURRENT_VERSION=`sudo find / -type d -name "mongodb-linux-x86_64-ubuntu1804-*" -exec basename {} \;`
  MONGOC_DRIVER_CURRENT_VERSION=`sudo find / -type d -name "mongo-c-driver-*" -exec basename {} \;`
  echo -ne "\r${COLOR_PRINT_GREEN}Getting Dependencies Current Versions${END_COLOR_PRINT}"
  echo
}

function update_packages()
{
    i=0
    while fuser /var/{lib/{dpkg,apt/lists},cache/apt/archives}/lock >/dev/null 2>&1; do
        case $(($i % 4)) in
            0 ) j="-" ;;
            1 ) j="\\" ;;
            2 ) j="|" ;;
            3 ) j="/" ;;
        esac
        echo && echo -en "\r${COLOR_PRINT_RED}[$j] Waiting for other package manager to finish...${END_COLOR_PRINT}" 
        sleep 0.25
        ((i=i+1))
    done
    echo -ne "${COLOR_PRINT_YELLOW}Updating Packages${END_COLOR_PRINT}"
    sudo apt install --only-upgrade ${XCASH_DPOPS_PACKAGES} -y >> ${LOGFILE} 2>&1
    echo -ne "\r${COLOR_PRINT_GREEN}Updating Packages${END_COLOR_PRINT}"
    echo
}

function update_xcash()
{
  echo -ne "${COLOR_PRINT_YELLOW}Updating X-CASH (This Might Take A While)${END_COLOR_PRINT}"
  cd ${XCASH_DIR}
  data=`git pull`
  if [ ! "$data" == "$GIT_PULL_ALREADY_UPDATED_MESSAGE" ]; then
    if [ "$RAM_CPU_RATIO" -ge "$RAM_CPU_RATIO_ALL_CPU_THREADS" ]; then
      make release -j ${CPU_THREADS} >> ${LOGFILE} 2>&1
    else
      make release -j $((CPU_THREADS / 2)) >> ${LOGFILE} 2>&1
    fi 
  fi
  echo -ne "\r${COLOR_PRINT_GREEN}Updating X-CASH (This Might Take A While)${END_COLOR_PRINT}"
  echo
}

function update_xcash_dpops()
{
  echo -ne "${COLOR_PRINT_YELLOW}Updating XCASH_DPOPS${END_COLOR_PRINT}"
  cd ${XCASH_DPOPS_DIR}
  data=`git pull`
  if [ ! "$data" == "$GIT_PULL_ALREADY_UPDATED_MESSAGE" ]; then
    if [ "$RAM_CPU_RATIO" -ge "$RAM_CPU_RATIO_ALL_CPU_THREADS" ]; then
      make release -j ${CPU_THREADS} >> ${LOGFILE} 2>&1
    else
      make release -j $((CPU_THREADS / 2)) >> ${LOGFILE} 2>&1
    fi 
  fi
  echo -ne "\r${COLOR_PRINT_GREEN}Updating XCASH_DPOPS${END_COLOR_PRINT}"
  echo
}

function update_shared_delegates_website()
{
  echo -ne "${COLOR_PRINT_YELLOW}Updating Shared Delegates Website${END_COLOR_PRINT}"
  cd ${SHARED_DELEGATES_WEBSITE_DIR}
  data=`git pull`
  if [ ! "$data" == "$GIT_PULL_ALREADY_UPDATED_MESSAGE" ]; then
    npm update >> ${LOGFILE} 2>&1
    ng build --prod --aot >> ${LOGFILE} 2>&1
    cd dist
    for f in *.js; do uglifyjs $f --compress --mangle --output "{$f}min"; rm $f; mv "{$f}min" $f; done
    if [ -d "$XCASH_DPOPS_SHARED_DELEGATE_FOLDER_DIR" ]; then
      sudo rm -r ${XCASH_DPOPS_SHARED_DELEGATE_FOLDER_DIR}
    fi 
    cd ../
    cp -a dist ${XCASH_DPOPS_SHARED_DELEGATE_FOLDER_DIR}
  fi
  echo -ne "\r${COLOR_PRINT_GREEN}Updating Shared Delegates Website${END_COLOR_PRINT}"
  echo
}

function update_mongodb()
{
  echo -ne "${COLOR_PRINT_YELLOW}Updating MongoDB${END_COLOR_PRINT}"
  sudo rm -r ${MONGODB_DIR}  
  cd ${XCASH_DPOPS_INSTALLATION_DIR}
  wget -q ${MONGODB_URL}
  tar -xf mongodb-linux-x86_64-*.tgz >> ${LOGFILE} 2>&1
  sudo rm mongodb-linux-x86_64-*.tgz >> ${LOGFILE} 2>&1
  MONGODB_DIR=`sudo find / -type d -name "mongodb-linux-x86_64-ubuntu1804-*"`/
  echo "${SYSTEMD_SERVICE_FILE_MONGODB}"
  echo
  echo
  echo "${MONGODB_DIR}"
  sudo sed '/mongodb-linux-x86_64-ubuntu1804-/d' -i ~/.profile
  sudo sed '/^[[:space:]]*$/d' -i ~/.profile
  sudo echo -ne "\nexport PATH=${MONGODB_DIR}bin:" >> ~/.profile 
  sudo echo -ne '$PATH' >> ~/.profile
  source ~/.profile
  echo -ne "\r${COLOR_PRINT_GREEN}Updating MongoDB${END_COLOR_PRINT}"
  echo
}

function update_mongoc_driver()
{
  echo -ne "${COLOR_PRINT_YELLOW}Updating Mongo C Driver${END_COLOR_PRINT}"
  sudo rm -r ${MONGOC_DRIVER_DIR}
  cd ${XCASH_DPOPS_INSTALLATION_DIR}
  wget -q ${MONGOC_DRIVER_URL}
  tar -xf mongo-c-driver-*.tar.gz >> ${LOGFILE} 2>&1
  sudo rm mongo-c-driver-*.tar.gz >> ${LOGFILE} 2>&1
  cd mongo-c-driver-*
  mkdir cmake-build >> ${LOGFILE} 2>&1
  cd cmake-build >> ${LOGFILE} 2>&1
  sudo make -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF .. >> ${LOGFILE} 2>&1
  sudo make -j ${CPU_THREADS} >> ${LOGFILE} 2>&1
  sudo make install >> ${LOGFILE} 2>&1
  sudo ldconfig
  MONGOC_DRIVER_DIR=`sudo find / -type d -name "mongo-c-driver-*"`/
  echo -ne "\r${COLOR_PRINT_GREEN}Updating Mongo C Driver${END_COLOR_PRINT}"
  echo
}

function update_nodejs()
{
  echo -ne "${COLOR_PRINT_YELLOW}Updating NodeJS${END_COLOR_PRINT}"
  sudo rm -r ${NODEJS_DIR}  
  cd ${XCASH_DPOPS_INSTALLATION_DIR}
  wget -q ${NODEJS_URL}
  tar -xf node*.tar.xz >> ${LOGFILE} 2>&1
  sudo rm node*.tar.xz >> ${LOGFILE} 2>&1
  NODEJS_DIR=`sudo find / -type d -name "node-*-linux-x64"`/
  sudo sed '/node-v/d' -i ~/.profile
  sudo sed '/PATH=\/bin:/d' -i ~/.profile
  sudo sed '/^[[:space:]]*$/d' -i ~/.profile
  sudo echo -ne "\nexport PATH=${NODEJS_DIR}bin:" >> ~/.profile 
  sudo echo -ne '$PATH' >> ~/.profile
  source ~/.profile
  echo -ne "\r${COLOR_PRINT_GREEN}Updating NodeJS${END_COLOR_PRINT}"
  echo
}





















function uninstall_packages()
{
    i=0
    while fuser /var/{lib/{dpkg,apt/lists},cache/apt/archives}/lock >/dev/null 2>&1; do
        case $(($i % 4)) in
            0 ) j="-" ;;
            1 ) j="\\" ;;
            2 ) j="|" ;;
            3 ) j="/" ;;
        esac
        echo && echo -en "\r${COLOR_PRINT_RED}[$j] Waiting for other package manager to finish...${END_COLOR_PRINT}" 
        sleep 0.25
        ((i=i+1))
    done
    echo -ne "${COLOR_PRINT_YELLOW}Uninstalling Packages${END_COLOR_PRINT}"
    sudo apt --purge remove ${XCASH_DPOPS_PACKAGES} -y >> ${LOGFILE} 2>&1
    echo -ne "\r${COLOR_PRINT_GREEN}Uninstalling Packages${END_COLOR_PRINT}"
    echo
}

function uninstall_systemd_service_files()
{
  echo -ne "${COLOR_PRINT_YELLOW}Uninstall Systemd Service Files${END_COLOR_PRINT}"
  sudo rm /lib/systemd/system/firewall.service /lib/systemd/system/MongoDB.service /lib/systemd/system/XCASH_Daemon.service /lib/systemd/system/XCASH_Daemon_Block_Verifier.service /lib/systemd/system/XCASH_DPOPS.service /lib/systemd/system/XCASH_Wallet.service
  sudo systemctl daemon-reload
  echo -ne "\r${COLOR_PRINT_GREEN}Uninstall Systemd Service Files${END_COLOR_PRINT}"
  echo
}

function uninstall_shared_delegates_website()
{
  echo
  echo
  echo -e "${COLOR_PRINT_GREEN}############################################################${END_COLOR_PRINT}"
  echo -e "${COLOR_PRINT_GREEN}            Uninstalling Shared Delegate Website${END_COLOR_PRINT}"
  echo -e "${COLOR_PRINT_GREEN}############################################################${END_COLOR_PRINT}"

  sudo rm -r ${XCASH_DPOPS_SHARED_DELEGATE_FOLDER_DIR} ${SHARED_DELEGATES_WEBSITE_DIR} ${NODEJS_DIR} >> ${LOGFILE} 2>&1

  sudo sed '/node-v/d' -i ~/.profile
  sudo sed '/PATH=\/bin:/d' -i ~/.profile
  sudo sed '/^[[:space:]]*$/d' -i ~/.profile
  source ~/.profile
  echo
  echo
}











function install()
{
  echo
  echo -e "${COLOR_PRINT_GREEN}############################################################${END_COLOR_PRINT}"
  echo -e "${COLOR_PRINT_GREEN}                  Starting Installation${END_COLOR_PRINT}"
  echo -e "${COLOR_PRINT_GREEN}############################################################${END_COLOR_PRINT}"

  # Create directories
  create_directories

  # Create files
  create_files

  # Update the package list
  update_packages_list
 
  # Install packages
  install_packages

  # Install X-CASH
  install_xcash

  # Install XCASH_DPOPS
  install_xcash_dpops

  # Install shared delegates website
  if [ "${SHARED_DELEGATE^^}" == "YES" ]; then
    install_shared_delegates_website
  fi

  # Create or import the wallet
  if [ "${WALLET_SETTINGS^^}" == "YES" ]; then
    create_xcash_wallet
  else
    import_xcash_wallet
  fi

  # Start the processes
  start_processes

  # Display X-CASH current wallet data  
  echo
  echo
  echo -e "${COLOR_PRINT_GREEN}############################################################${END_COLOR_PRINT}"
  echo -e "${COLOR_PRINT_GREEN}          Installation Has Completed Successfully  ${END_COLOR_PRINT}"
  echo -e "${COLOR_PRINT_GREEN}############################################################${END_COLOR_PRINT}"
  echo
  echo
  echo -e "${CURRENT_XCASH_WALLET_INFORMATION}"
}

function update()
{
  echo -e "${COLOR_PRINT_GREEN}############################################################${END_COLOR_PRINT}"
  echo -e "${COLOR_PRINT_GREEN}                  Updating XCASH_DPOPS${END_COLOR_PRINT}"
  echo -e "${COLOR_PRINT_GREEN}############################################################${END_COLOR_PRINT}"
  echo
  echo

  # Check if solo node
  check_if_solo_node

  # Get the installation directory
  get_installation_directory

  # Get the current version of the dependencies
  get_dependencies_current_version

  # Stop the process
  stop_processes

  # Check if upgrade from a solo delegate to a shared delegate or a shared delegate to a solo delegate
  check_if_upgrade_solo_delegate_and_shared_delegate

  # Update the package list
  update_packages_list

  # Update all system packages that are XCASH_DPOPS dependencies
  update_packages

  # Update all repositories
  update_xcash
  update_xcash_dpops
  if [ "${SHARED_DELEGATE^^}" == "YES" ]; then
    update_shared_delegates_website
  fi

  # Update all dependencies
  if [ ! "$MONGODB_CURRENT_VERSION" == "$MONGODB_LATEST_VERSION" ]; then
    update_mongodb
  else
    echo -e "${COLOR_PRINT_GREEN}MongoDB Is Already Up To Date${END_COLOR_PRINT}"
  fi
  if [ ! "$MONGOC_DRIVER_CURRENT_VERSION" == "$MONGOC_DRIVER_LATEST_VERSION" ]; then
    update_mongoc_driver
  else
    echo -e "${COLOR_PRINT_GREEN}Mongo C Driver Is Already Up To Date${END_COLOR_PRINT}"
  fi
  if [ "${SHARED_DELEGATE^^}" == "YES" ]; then
    if [ ! "$NODEJS_CURRENT_VERSION" == "$NODEJS_LATEST_VERSION" ]; then
      update_nodejs
    else
      echo -e "${COLOR_PRINT_GREEN}NodeJS Is Already Up To Date${END_COLOR_PRINT}"
    fi
    update_npm
  fi

  # Start the processes
  start_processes

  echo
  echo
  echo -e "${COLOR_PRINT_GREEN}############################################################${END_COLOR_PRINT}"
  echo -e "${COLOR_PRINT_GREEN}          Update Has Completed Successfully  ${END_COLOR_PRINT}"
  echo -e "${COLOR_PRINT_GREEN}############################################################${END_COLOR_PRINT}"
}

function uninstall()
{
  echo -ne "${COLOR_PRINT_RED}Please Confirm You Want To Uninstall By Typing \"Uninstall\":${END_COLOR_PRINT}"
  read data
  if [ ! "$data" == "Uninstall" ]; then
    exit
  fi
  echo -e "${COLOR_PRINT_GREEN}############################################################${END_COLOR_PRINT}"
  echo -e "${COLOR_PRINT_GREEN}                Uninstalling XCASH_DPOPS${END_COLOR_PRINT}"
  echo -e "${COLOR_PRINT_GREEN}############################################################${END_COLOR_PRINT}"
  echo
  echo

  # Get the installation directory
  get_installation_directory

  # Restart the X-CASH Daemon and stop the X-CASH Wallet RPC
  echo -ne "${COLOR_PRINT_YELLOW}Shutting Down X-CASH Wallet Systemd Service File and Restarting XCASH Daemon Systemd Service File${END_COLOR_PRINT}"
  sudo systemctl restart XCASH_Daemon
  sleep 10s
  sudo systemctl stop XCASH_Wallet
  sleep 10s
  echo -ne "\r${COLOR_PRINT_GREEN}Shutting Down X-CASH Wallet Systemd Service File and Restarting XCASH Daemon Systemd Service File${END_COLOR_PRINT}"
  echo

  # Get the current xcash wallet data
  get_current_xcash_wallet_data

  # Stop processes
  stop_processes

  # Uninstall packages
  uninstall_packages

  # Uninstall Systemd Service Files
  uninstall_systemd_service_files

  # Uninstall the Mongo C Driver  
  echo -ne "${COLOR_PRINT_YELLOW}Uninstalling Mongo C Driver${END_COLOR_PRINT}"
  sudo /usr/local/share/mongo-c-driver/uninstall.sh  >> ${LOGFILE} 2>&1
  sudo ldconfig
  echo -ne "\r${COLOR_PRINT_GREEN}Uninstalling Mongo C Driver${END_COLOR_PRINT}"
  echo

  # Uninstall the installation folder
  echo -ne "${COLOR_PRINT_YELLOW}Uninstalling XCASH_DPOPS Installation Directory${END_COLOR_PRINT}"
  sudo rm -r ${XCASH_DPOPS_INSTALLATION_DIR}
  echo -ne "\r${COLOR_PRINT_GREEN}Uninstalling XCASH_DPOPS Installation Directory${END_COLOR_PRINT}"
  echo

  # Update profile
  echo -ne "${COLOR_PRINT_YELLOW}Updating Profile${END_COLOR_PRINT}"
  sudo sed '/mongodb-linux-x86_64-ubuntu1804-/d' -i ~/.profile
  sudo sed '/node-v/d' -i ~/.profile
  sudo sed '/PATH=\/bin:/d' -i ~/.profile
  sudo sed '/^[[:space:]]*$/d' -i ~/.profile
  source ~/.profile
  echo -ne "\r${COLOR_PRINT_GREEN}Updating Profile${END_COLOR_PRINT}"
  echo

  echo
  echo
  echo -e "${COLOR_PRINT_GREEN}############################################################${END_COLOR_PRINT}"
  echo -e "${COLOR_PRINT_GREEN}          Uninstall Has Completed Successfully  ${END_COLOR_PRINT}"
  echo -e "${COLOR_PRINT_GREEN}############################################################${END_COLOR_PRINT}"
  
  # Display X-CASH current wallet data  
  echo
  echo
  echo -e "${CURRENT_XCASH_WALLET_INFORMATION}"
}



  







# Get the installation settings
installation_settings

if [ "$INSTALLATION_TYPE_SETTINGS" -eq "1" ]; then
  install
elif [ "$INSTALLATION_TYPE_SETTINGS" -eq "2" ]; then
  update
elif [ "$INSTALLATION_TYPE_SETTINGS" -eq "3" ]; then
  uninstall
fi