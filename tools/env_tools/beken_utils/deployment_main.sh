#!/bin/sh
#
# Standalone secure firmware generator for the security server.
#
# Inputs required in <input_dir> (copied from the build output install/pack):
#   bl2.bin tfm_s.bin cpu0_app.bin ap_app.bin
#   partitions.csv security.csv ota.csv pack.json ppc.csv gpio_dev.csv
#   (ppc_config.bin / ppc_config_ap.bin optional)
#
# Signing keys are taken from config/key/ (the security server owns the private
# key); the flash encryption key comes from security.csv (or is injected here).
#
# Usage:
#   ./deployment_main.sh <input_dir> <output_dir>
# Example:
#   ./deployment_main.sh ./input_dir ./output_dir
#
# Optional flash AES key injection (overrides security.csv flash_aes_key):
#   FLASH_AES_KEY=<hex> ./deployment_main.sh ./input_dir ./output_dir
#   or place the hex string in config/flash_aes_key.txt
#

THIS_PATH=$(cd "$(dirname "$0")" && pwd)
BUILDDIR=${THIS_PATH}/../../..

BK_DOCKER_MODE=${BK_DOCKER_MODE:-0}

usage() {
    echo "Usage: $0 <input_dir> <output_dir>"
    echo "  <input_dir>   directory holding the raw firmware and config (from build install/pack)"
    echo "  <output_dir>  directory to receive the signed/encrypted firmware"
    echo "Example:"
    echo "  $0 ./input_dir ./output_dir"
    exit 1
}

# Update the flash_aes_key field in security.csv (both "Field,Value" and bare
# "key,value" layouts are supported).
inject_flash_aes_key() {
    security_csv="$1"
    aes_key="$2"

    if [ ! -f "$security_csv" ]; then
        echo "Error: security.csv not found: $security_csv"
        return 1
    fi

    key_len=${#aes_key}
    if [ "$key_len" -ne 64 ] && [ "$key_len" -ne 128 ]; then
        echo "Error: invalid flash AES key length: $key_len (expected 64 or 128 hex chars)"
        return 1
    fi

    tmp_file="${security_csv}.tmp"
    awk -F',' -v OFS=',' -v k="$aes_key" '{
        if ($1 == "flash_aes_key") { $2 = k }
        print
    }' "$security_csv" > "$tmp_file" && mv "$tmp_file" "$security_csv"
    echo "Injected flash_aes_key into $security_csv"
}

# ============================================================================
# Argument parsing and validation
# ============================================================================
[ -z "$1" ] || [ -z "$2" ] && usage

input_dir=$(realpath "$1" 2>/dev/null)
if [ $? -ne 0 ] || [ ! -d "$input_dir" ]; then
    echo "Error: input directory not found: $1"
    exit 1
fi

output_dir=$(realpath "$2" 2>/dev/null)
if [ $? -ne 0 ]; then
    mkdir -p "$2" || { echo "Error: failed to create output directory: $2"; exit 1; }
    output_dir=$(realpath "$2")
fi
mkdir -p "$output_dir"

echo "========================================"
echo "BK7259 secure firmware deployment"
echo "========================================"
echo "Input  dir: $input_dir"
echo "Output dir: $output_dir"

# ============================================================================
# 1. Clean the output directory (keep .gitignore)
# ============================================================================
find "$output_dir" -type f -not -name ".gitignore" -delete 2>/dev/null

# ============================================================================
# 2. Put the security-server signing keys into the input directory
# ============================================================================
priv_key="${THIS_PATH}/config/key/root_ec256_privkey.pem"
pub_key="${THIS_PATH}/config/key/root_ec256_pubkey.pem"
if [ ! -f "$priv_key" ] || [ ! -f "$pub_key" ]; then
    echo "Error: EC256 signing keypair not found under ${THIS_PATH}/config/key/"
    echo "       Run ./gen_keys.sh first, or place root_ec256_{priv,pub}key.pem there."
    exit 1
fi
cp -f "$priv_key" "$input_dir/root_ec256_privkey.pem"
cp -f "$pub_key" "$input_dir/root_ec256_pubkey.pem"
echo "Copied EC256 signing keypair into input directory"

# ============================================================================
# 3. Optionally inject the flash AES key
# ============================================================================
flash_aes_key="$FLASH_AES_KEY"
if [ -z "$flash_aes_key" ] && [ -f "${THIS_PATH}/config/flash_aes_key.txt" ]; then
    flash_aes_key=$(tr -d ' \t\r\n' < "${THIS_PATH}/config/flash_aes_key.txt")
fi
if [ -n "$flash_aes_key" ]; then
    inject_flash_aes_key "$input_dir/security.csv" "$flash_aes_key" || exit 1
else
    echo "Info: using flash_aes_key already present in security.csv"
fi

# ============================================================================
# 4. Python environment
# ============================================================================
if [ ! -d "${BUILDDIR}/bk_py_libs" ] && [ -f "${BUILDDIR}/bk_py_libs.tgz" ]; then
    echo "Extracting bundled python libraries..."
    tar --no-same-owner -zxf "${BUILDDIR}/bk_py_libs.tgz" -C "${BUILDDIR}"
    find "${BUILDDIR}/bk_py_libs" -type d -exec chmod a+wx {} \; 2>/dev/null
fi
if [ -d "${BUILDDIR}/bk_py_libs" ]; then
    export PYTHONPATH=${BUILDDIR}/bk_py_libs
    echo "PYTHONPATH: $PYTHONPATH"
fi

# ============================================================================
# 5. Sign + encrypt + pack
# ============================================================================
cd "$THIS_PATH" || exit 1
echo "Running: main.py pack all --config_dir $input_dir"
if ! python3 -B ./main.py pack all --config_dir "$input_dir" --debug; then
    echo "Error: pack all failed"
    exit 1
fi

# ============================================================================
# 6. Collect the deliverables
# ============================================================================
echo "Collecting deliverables into $output_dir ..."
for f in all-app.bin bootloader.bin ota.bin otp_efuse_config.json; do
    if [ -f "$input_dir/$f" ]; then
        cp -f "$input_dir/$f" "$output_dir/$f"
        echo "  - $f"
    else
        echo "  ! $f not found (skipped)"
    fi
done
if [ -d "$input_dir/install" ]; then
    cp -f "$input_dir/install/"*.bin "$output_dir/" 2>/dev/null
fi

# ============================================================================
# 7. Clean temporary files
# ============================================================================
rm -f "$input_dir/bl1_rotpk_digest.bin" "$input_dir/bl1_rotpk_digest.txt" "$input_dir/rotpk_digest.json"
rm -f ./bl1_rotpk_digest.bin ./bl1_rotpk_digest.txt ./rotpk_digest.json

echo "========================================"
echo "Deployment completed. Deliverables in: $output_dir"
echo "  Program to flash : all-app.bin, bootloader.bin"
echo "  OTA payload      : ota.bin"
echo "  OTP provisioning : otp_efuse_config.json"
echo "========================================"
