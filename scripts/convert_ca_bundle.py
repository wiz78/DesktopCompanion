data = open("x509_crt_bundle", "rb").read()
hex_bytes = [f"0x{b:02x}" for b in data]
lines = [", ".join(hex_bytes[i:i+16]) for i in range(0, len(hex_bytes), 16)]
num_certs = (data[0] << 8) | data[1]
header = f"""// Auto-generated ESP-IDF 4 / Arduino-ESP32 2.x X.509 certificate bundle
// Total certificates: {num_certs}, Total size: {len(data)} bytes
#pragma once

#include <pgmspace.h>

static const unsigned char x509_crt_bundle[] PROGMEM = {{
    """ + ",\n    ".join(lines) + f"""
}};
static const unsigned int x509_crt_bundle_len = sizeof(x509_crt_bundle);
"""
open("../include/x509_crt_bundle.h", "w").write(header)
print(f"Successfully generated include/x509_crt_bundle.h with {num_certs} certificates ({len(data)} bytes)!")
