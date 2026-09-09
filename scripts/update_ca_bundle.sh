#!/bin/sh

if [ ! -f gen_crt_bundle.py ]; then
  curl -sO https://raw.githubusercontent.com/espressif/esp-idf/release/v4.4/components/mbedtls/esp_crt_bundle/gen_crt_bundle.py
fi

curl -sO https://curl.se/ca/cacert.pem

python3 gen_crt_bundle.py -i cacert.pem
python3 convert_ca_bundle.py -i cacert.pem

rm cacert.pem