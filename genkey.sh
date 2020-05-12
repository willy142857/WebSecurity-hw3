#!/bin/bash

mkdir -p .key && cd .key

echo "generate CA key and certificate"

openssl genrsa -out ca.key 4096
openssl req -x509 -new -nodes -key ca.key \
    -subj "/C=TW/ST=Taiwan/L=Taipei City/O=MyOrg/OU=MyUnit/CN=my.ca" \
    -sha256 -days 365 -out ca.crt
echo "CA key is ca.key"
echo "CA cert is ca.crt"

printf "\ngenerate server key and certificate\n"
openssl genrsa -out host.key 4096
openssl req -new -key host.key -out host.csr -config ./ssl.conf
openssl x509 -req -in host.csr -out host.crt -CA ca.crt -CAkey ca.key -CAcreateserial -days 30 -extensions v3_req -extfile ssl.conf
echo "server key is host.key"
echo "server cert is host.crt"
