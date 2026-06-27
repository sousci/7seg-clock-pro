# Test firmware asset

`firmware.bin` is a temporary online-update test image.

- Source firmware version to install manually: `0.1.0`
- Test update firmware version embedded in this binary: `0.1.1-test`
- Download URL used by the firmware:
  `https://raw.githubusercontent.com/sousci/7seg-clock-pro/main/test_firmware/firmware.bin`

This directory is for validating ESP32 self-update from GitHub. For production,
use GitHub Releases with a manifest and hash/signature verification.
