#!/bin/sh
#
# One-time key generation helper for the security server.
#
# Generates:
#   1) EC256 signing keypair (BL1 + BL2 root key) -> config/key/
#   2) A 256-bit flash AES key (AES-128-XTS)      -> printed and config/flash_aes_key.txt
#
# Run this ONCE per product line. The generated public key must match the key
# built into the firmware and provisioned in OTP, otherwise secure boot fails.
#
# Usage:
#   ./gen_keys.sh            # generate both keys (refuses to overwrite existing)
#   ./gen_keys.sh --force    # overwrite existing keys
#

set -e

THIS_PATH=$(cd "$(dirname "$0")" && pwd)
KEY_DIR="${THIS_PATH}/config/key"

FORCE=0
[ "$1" = "--force" ] && FORCE=1

mkdir -p "$KEY_DIR"

priv_key="${KEY_DIR}/root_ec256_privkey.pem"
pub_key="${KEY_DIR}/root_ec256_pubkey.pem"

# ----------------------------------------------------------------------------
# 1. EC256 signing keypair
# ----------------------------------------------------------------------------
if [ -f "$priv_key" ] && [ "$FORCE" -ne 1 ]; then
    echo "EC256 private key already exists: $priv_key (use --force to overwrite)"
else
    echo "Generating EC256 signing keypair (prime256v1)..."
    openssl ecparam -name prime256v1 -genkey -out "$priv_key"
    openssl ec -in "$priv_key" -pubout -out "$pub_key"
    echo "  private key: $priv_key"
    echo "  public  key: $pub_key"
fi

# ----------------------------------------------------------------------------
# 2. Flash AES key (256-bit XTS key material for AES-128-XTS)
# ----------------------------------------------------------------------------
flash_aes_key=$(openssl rand -hex 32)

echo "$flash_aes_key" > "${THIS_PATH}/config/flash_aes_key.txt"

echo ""
echo "Flash AES key (AES-128-XTS, 64 hex chars): $flash_aes_key"
echo "  saved to: ${THIS_PATH}/config/flash_aes_key.txt"
echo "  (set it in input_dir/security.csv 'flash_aes_key', or export FLASH_AES_KEY)"

# ----------------------------------------------------------------------------
# Warnings
# ----------------------------------------------------------------------------
echo ""
echo "========================================================================"
echo "IMPORTANT"
echo "  - Changing the EC256 root key requires rebuilding the firmware"
echo "    (mcuboot keys.c) AND re-provisioning the OTP ROTPK hashes."
echo "  - Changing the flash AES key requires re-provisioning the OTP"
echo "    flash_aes_key fields."
echo "  - Keep the private key on the security server only."
echo "========================================================================"
