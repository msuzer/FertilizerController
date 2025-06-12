
Import("env")
import os
import subprocess
import sys
import re
import glob
from datetime import datetime

build_dir = env.subst("$BUILD_DIR")
project_dir = env.subst("$PROJECT_DIR")

bootloader = os.path.join(build_dir, "bootloader.bin")
partitions = os.path.join(build_dir, "partitions.bin")
firmware = os.path.join(build_dir, "firmware.bin")

# Path to the header file containing FIRMWARE_VERSION and DEVICE_VERSION
system_context_header = os.path.join(project_dir, "src/core", "version.h")

# Use esptool.py from PlatformIO's tool directory
esptool_path = os.path.join(
    env.subst("$PROJECT_PACKAGES_DIR"),
    "tool-esptoolpy",
    "esptool.py"
)

def get_firmware_version():
    try:
        with open(system_context_header, 'r') as f:
            content = f.read()
            match = re.search(r'#define\s+FIRMWARE_VERSION\s+"([^"]+)"', content)
            if match:
                version = match.group(1)
                print(f"Detected FIRMWARE_VERSION = {version}")
                return version
    except Exception as e:
        print("Could not read FIRMWARE_VERSION from header:", e)
    return "unknown"

def get_device_version():
    try:
        with open(system_context_header, 'r') as f:
            content = f.read()
            match = re.search(r'#define\s+DEVICE_VERSION\s+"([^"]+)"', content)
            if match:
                device_version = match.group(1)
                print(f"Detected DEVICE_VERSION = {device_version}")
                return device_version
    except Exception as e:
        print("Could not read DEVICE_VERSION from header:", e)
    return "unknown"

def clean_previous_bins():
    pattern = os.path.join(project_dir, "agro_fertilizer_fw_*.bin")
    old_files = glob.glob(pattern)
    for file_path in old_files:
        try:
            os.remove(file_path)
            print(f"Removed old file: {os.path.basename(file_path)}")
        except Exception as e:
            print(f"Failed to remove {file_path}: {e}")

def merge_bin_files(source, target, env):
    version = get_firmware_version()
    device_version = get_device_version()

    board_label = f"agro_fertilizer_fw_v{version}_{device_version}"

    # Format: DD_MM_YYYY
    date_str = datetime.now().strftime("%d_%m_%Y")
    output_filename = f"{board_label}_{date_str}.bin"

    final_output = os.path.join(project_dir, output_filename)

    clean_previous_bins()

    print("Merging binaries...")

    cmd = [
        sys.executable,
        esptool_path,
        "--chip", "esp32", "merge_bin",
        "-o", final_output,
        "--flash_mode", "dio",
        "--flash_freq", "40m",
        "--flash_size", "4MB",
        "0x1000", bootloader,
        "0x8000", partitions,
        "0x10000", firmware
    ]

    try:
        result = subprocess.run(cmd, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        print(result.stdout)
        print(f"Combined binary created at: {final_output}")
    except subprocess.CalledProcessError as e:
        print("Error during merge_bin:")
        print(e.stderr)

env.AddPostAction("buildprog", merge_bin_files)
